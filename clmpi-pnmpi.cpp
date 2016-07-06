#include "unordered_map"

#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#include "mpi.h"
#include "pnmpimod.h"
#include "clmpi_status.h"
#include "clmpi_request.h"
#include "clmpi_piggyback.h"
#include "clmpi.h"

using namespace std;

int pb_int = 0;
int run_check=1;
int run_set=1;


typedef struct pb_clocks{
  size_t local_clock;
} pb_clocks_t;

#define DBG_SC
#ifdef DBG_SC
size_t local_sent_clock = PNMPI_MODULE_CLMPI_INITIAL_CLOCK;
unordered_map<MPI_Request, size_t> request_to_local_clock;
#endif

pb_clocks_t *pb_clocks;
//MPI_Comm mpi_clock_win_comm;
//MPI_Win  mpi_clock_win;

//size_t local_clock= PNMPI_MODULE_CLMPI_INITIAL_CLOCK;
size_t local_tick = 0;
char *pbdata;
int my_rank = -1;

#define PBSET \
  if (run_set) { \
    pbdata = (char*)pb_clocks; \ 
    pb_set(pb_size, pbdata); \
  }


#define PBCHECKARRAY(res,status,count,num) { \
  if (res==MPI_SUCCESS) \
    { \
      if ((STATUS_STORAGE_ARRAY(status,*pb_offset,*TotalStatusExtension,int,count,num)!=PB_PATTERN) || (0) ) \
	printf("Received wrong pattern %08x\n",\
	       STATUS_STORAGE_ARRAY(status,*pb_offset,*TotalStatusExtension,int,count,num));\
    } }


#define PBCHECK(res,status) PBCHECKARRAY(res,status,1,0)



PNMPIMOD_Piggyback_t       pb_set;
PNMPIMOD_Piggyback_Size_t  pb_setsize;
static int *pb_offset;
static int pb_size=sizeof(pb_clocks_t);
static int *TotalStatusExtension;
static int *StatusOffsetInRequest;

#define COMM_REQ_FROM_STATUS(status) REQ_FROM_STATUS(status,(*StatusOffsetInRequest))
#define COMM_REQ_FROM_STATUSARRAY(status,count,num) REQ_FROM_STATUSARRAY(status,(*StatusOffsetInRequest),*TotalStatusExtension,count,num)


/*------------------------------------------------------------*/
/* External service routines */

size_t *registered_buff_clocks = NULL;
int     registered_buff_length = -1;
int     sync_clock = 1;
unordered_map<MPI_Request, MPI_Request*> irecv_request_map;

void clmpi_init_registered_clocks(MPI_Request *request, int length) {
  if (registered_buff_clocks == NULL) {
    fprintf(stderr, "CLMPI: registered clock is NULL\n");
    *registered_buff_clocks = 1;
    exit(1);
  }

  for (int i = 0; i < length; i++) {
    // if (my_rank != 0) {
    //   fprintf(stderr, "i: %d/%d: rank: %d:  siez:%d \n", i, length, my_rank, irecv_request_map.size());
    // }
    if (irecv_request_map.find(request[i]) != irecv_request_map.end()) {
      /*If this reuquest[i] is from irecv*/
      registered_buff_clocks[i] = PNMPI_MODULE_CLMPI_UNMATCHED_RECV_REQ_CLOCK;
    } else {
      /*If this reuquest[i] is from isend*/
      //      fprintf(stderr, "rank: %d: nooo request: %p siez:%d \n", my_rank, request[i], irecv_request_map.size());
      // exit(1);
      registered_buff_clocks[i] = PNMPI_MODULE_CLMPI_SEND_REQ_CLOCK;
    }    
    //    fprintf(stderr, "Init: request[%d] = %lu\n",i , registered_buff_clocks[i]);
  }
  
}

void clmpi_irecv_test_erase(MPI_Request request) {
  if (irecv_request_map.find(request) == irecv_request_map.end()) {
    fprintf(stderr, "CLMPI:  %d: request: %p does not exist\n", my_rank, request);
    exit(1);
  }
  irecv_request_map.erase(request);
  //  fprintf(stderr, "CLMPI:  %d: erase request: %p size: %d\n", my_rank, request, irecv_request_map.size());
}

void clmpi_update_clock(size_t recv_clock) {
  if (sync_clock) {
    //    if (rank == 0) fprintf(stderr, "Recv: clock: %d", local_clock);
    if (pb_clocks->local_clock <= recv_clock) {
      pb_clocks->local_clock = recv_clock;	
    }
    pb_clocks->local_clock++;

    fprintf(stderr, "CLMPI:  %d: %f %lu\n", my_rank, MPI_Wtime(), pb_clocks->local_clock);
    // if (pb_clocks->local_clock < pb_clocks->next_clock) {
    //   fprintf(stderr, "CLMPI: local_clock < next_clock\n");
    //   exit(1);
    // }
    // pb_clocks->next_clock = pb_clocks->local_clock;
    //    if(!my_rank) fprintf(stderr, "%d %f %d %lu\n", my_rank, MPI_Wtime() - start, coll_count++, pb_clocks->local_clock);
  }
}

int PNMPIMOD_get_recv_clocks(int *local_clock, int length)
{ 
  return MPI_SUCCESS;
}

int PNMPIMOD_register_recv_clocks(size_t *clocks, int length)
{ 
  registered_buff_clocks = clocks;
  registered_buff_length = length;
  memset(clocks, 0, sizeof(size_t) * length);

  return MPI_SUCCESS;
}

int PNMPIMOD_clock_control(size_t control)
{ 
  sync_clock = control;
  return MPI_SUCCESS;
}

int PNMPIMOD_sync_clock(size_t recv_clock)
{ 
  clmpi_update_clock(recv_clock);
  return 1;
}


int PNMPIMOD_get_local_clock(size_t *clock)
{ 
  *clock = pb_clocks->local_clock;
  return MPI_SUCCESS;
}

size_t start;
int coll_count = 1;
int PNMPIMOD_collective_sync_clock(MPI_Comm comm)
{
  size_t clock_max;
  int ret;
  if (!my_rank) {
    //    fprintf(stderr, "Rank:  %d: ===== MPI_Collective ========: %f\n", my_rank, MPI_Wtime() - start);
    //    if(!my_rank) fprintf(stderr, "%d %f %d %lu\n", my_rank, MPI_Wtime() - start, coll_count++, pb_clocks->local_clock);
    //
  }
  ret = PMPI_Allreduce(&pb_clocks->local_clock, &clock_max, 1, MPI_UNSIGNED_LONG, MPI_MAX, comm);
  // if (!my_rank) {
  //   fprintf(stderr, "Rank:  %d: ===== MPI_Collective ========: %f\n", my_rank, MPI_Wtime() - start);
  // }

  pb_clocks->local_clock = clock_max + 1;
  // TODO: Investigate why this local_sent_clock incrementation causes SEND&GET out of order problem
  // if (request_to_local_clock.size() == 0) {
  //   local_sent_clock = pb_clocks->local_clock;
  // }
  return ret;
}


#ifdef  DBG_SC
int PNMPIMOD_get_local_sent_clock(size_t *clock)
{ 
  *clock = local_sent_clock;
  //  *clock = pb_clocks->local_clock;
  return MPI_SUCCESS;
}
#endif

int PNMPIMOD_get_num_of_incomplete_sending_msg(size_t *size)
{
  *size =  request_to_local_clock.size();
  //  *size = 0;
  return MPI_SUCCESS;
}

// int PNMPIMOD_fetch_next_clocks(int len, int *ranks, size_t *next_clocks)
// { 
//   int i;
//   size_t min_next_clock = 0;

//   for (i = 0; i < len; ++i) {
//     //    PMPI_Get(&next_clocks[i], sizeof(size_t), MPI_BYTE, ranks[i], 1, sizeof(size_t), MPI_BYTE, mpi_clock_win);
//   }
//   /*Only after MPI_Win_flush_local_all, the retrived values by PMPI_Get become visible*/
//   //  PMPI_Win_flush_local_all(mpi_clock_win);
//   exit(1);
//   fprintf(stderr, "is not supported");


//   return MPI_SUCCESS;
// }

// int PNMPIMOD_get_next_clock(size_t *clock)
// { 
//   *clock = pb_clocks->next_clock;
//   return MPI_SUCCESS;
// }

// int PNMPIMOD_set_next_clock(size_t clock)
// { 
//   pb_clocks->next_clock = clock;
//   return MPI_SUCCESS;
// }


char** cmpi_btrace() 
{
  int j, nptrs;
  void *buffer[100];
  char **strings;

  nptrs = backtrace(buffer, 100);

  /* backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)*/
  strings = (char**)backtrace_symbols(buffer, nptrs);
  if (strings == NULL) {
    perror("backtrace_symbols");
    exit(EXIT_FAILURE);
  }   

  /*
    You can translate the address to function name by
    addr2line -f -e ./a.out <address>
  */
  return strings;
}



void cmpi_pring_addr()
{
  char **strings;
  strings = cmpi_btrace();
  fprintf(stderr, "%s", strings[9]);
  free(strings);
}

void cmpi_sync_clock_at(MPI_Status *status, int count, int i, int matched_index)
{
  size_t sender_clock;		
  sender_clock = STATUS_STORAGE_ARRAY(status,*pb_offset, *TotalStatusExtension, size_t, count, i);
  if (registered_buff_clocks != NULL) {
    if (registered_buff_length != count) {
      fprintf(stderr, "Registered clock buffufer lengths (%d) is different to incount (%d)", 
		registered_buff_length, count);
    }
    registered_buff_clocks[matched_index] = sender_clock;
    //    if (my_rank == 1)fprintf(stderr, "ReMPI:%d: [%d |%d|] update index: %d count %d\n", my_rank, sender_clock, status[i].MPI_SOURCE, matched_index, count);
  }
  //  fprintf(stderr, "%lu\t%d\t",sender_clock, status[i].MPI_SOURCE);
  //  cmpi_pring_addr();
  //  fprintf(stderr, "\t%lu\t|%d|\n", local_clock, my_rank);
  clmpi_update_clock(sender_clock);
  return; 
}

void cmpi_sync_clock(MPI_Status *status)
{
  cmpi_sync_clock_at(status, 1, 0, 0);
  return;
}

void cmpi_set_tick()
{
#if 0
  int ticks[] = {
    100,
    100,
    100,
    100,
    100,
    200,
    200,
    100,
    100,
    200,
    200,
    100,
    225,
    300,
    300,
    225,
    225,
    300,
    300,
    225,
    200,
    225,
    225,
    200
  };
#endif

  local_tick = 1; //ticks[my_rank];
}

void cmpi_init_pb_clock()
{
  //  PMPI_Comm_dup(MPI_COMM_WORLD, &mpi_clock_win_comm);
  //  PMPI_Win_allocate(sizeof(struct pb_clocks), sizeof(size_t), MPI_INFO_NULL, mpi_clock_win_comm, &pb_clocks, &mpi_clock_win);
  //  PMPI_Win_lock_all(MPI_MODE_NOCHECK, mpi_clock_win);
  //  PMPI_Win_lock_all(0, mpi_clock_win);
  pb_clocks    = (struct pb_clocks*) malloc(sizeof(struct pb_clocks));
  pb_clocks->local_clock   = PNMPI_MODULE_CLMPI_INITIAL_CLOCK;
  //  pb_clocks->next_clock    = PNMPI_MODULE_CLMPI_INITIAL_CLOCK;
  //  pb_clocks->trigger_clock = PNMPI_MODULE_CLMPI_INITIAL_CLOCK;
  return;
}

void cmpi_update_local_sent_clock(MPI_Request tmp_req)
{
  size_t tmp_clock;
  if (request_to_local_clock.find(tmp_req) != request_to_local_clock.end()) {
    // tmp_clock = request_to_local_clock[tmp_req];
    // if (local_sent_clock < tmp_clock) {
    //   local_sent_clock = tmp_clock;
    // }
    request_to_local_clock.erase(tmp_req);
    //	  fprintf(stderr, "CLMPI:  %d: erase registered request(SEND): %p\n", my_rank, tmp_req);
  } else {
    fprintf(stderr, "no such send reqeust \n");
    exit(1);
  }
  if (request_to_local_clock.empty()) {
    local_sent_clock = pb_clocks->local_clock - 1;
  }
}



/*.......................................................*/
/* Registration */

int PNMPI_RegistrationPoint()
{
  int err;
  PNMPI_Service_descriptor_t service;
  PNMPI_Global_descriptor_t global;
  /* register this module and its services */

  err=PNMPI_Service_RegisterModule(PNMPI_MODULE_CLMPI);
  if (err!=PNMPI_SUCCESS) {
    return MPI_ERROR_PNMPI;
  }

  /*Speicfy buffer for clock*/
  sprintf(service.name,"clmpi_register_recv_clocks");
  service.fct=(PNMPI_Service_Fct_t) PNMPIMOD_register_recv_clocks;
  sprintf(service.sig,"pi");
  err=PNMPI_Service_RegisterService(&service);
  if (err!=PNMPI_SUCCESS) {
    return MPI_ERROR_PNMPI;
  }

  /*Disable/Enable ticking*/
  sprintf(service.name,"clmpi_clock_control");
  service.fct=(PNMPI_Service_Fct_t) PNMPIMOD_clock_control;
  sprintf(service.sig,"i");
  err=PNMPI_Service_RegisterService(&service);
  if (err!=PNMPI_SUCCESS) {
    return MPI_ERROR_PNMPI;
  }

  /*Sync and update local clock given received clock*/
  sprintf(service.name,"clmpi_sync_clock");
  service.fct=(PNMPI_Service_Fct_t) PNMPIMOD_sync_clock;
  sprintf(service.sig,"i");
  err=PNMPI_Service_RegisterService(&service);
  if (err!=PNMPI_SUCCESS) {
    return MPI_ERROR_PNMPI;
  }

  /*Get local clock*/
  sprintf(service.name,"clmpi_get_local_clock");
  service.fct=(PNMPI_Service_Fct_t) PNMPIMOD_get_local_clock;
  sprintf(service.sig,"p");
  err=PNMPI_Service_RegisterService(&service);
  if (err!=PNMPI_SUCCESS) {
    return MPI_ERROR_PNMPI;
  }

#ifdef  DBG_SC
  /*Get local clock*/
  sprintf(service.name,"clmpi_get_local_sent_clock");
  service.fct=(PNMPI_Service_Fct_t) PNMPIMOD_get_local_sent_clock;
  sprintf(service.sig,"p");
  err=PNMPI_Service_RegisterService(&service);
  if (err!=PNMPI_SUCCESS) {
    return MPI_ERROR_PNMPI;
  }
#endif

  /*Get collective sync clock*/
  sprintf(service.name,"clmpi_collective_sync_clock");
  service.fct=(PNMPI_Service_Fct_t) PNMPIMOD_collective_sync_clock;
  sprintf(service.sig,"p");
  err=PNMPI_Service_RegisterService(&service);
  if (err!=PNMPI_SUCCESS) {
    return MPI_ERROR_PNMPI;
  }

  /*Get get_num_of_incomplete_sending_msg*/
  sprintf(service.name,"clmpi_get_incomplete_smsg_num");
  service.fct=(PNMPI_Service_Fct_t) PNMPIMOD_get_num_of_incomplete_sending_msg;
  sprintf(service.sig,"p");
  err=PNMPI_Service_RegisterService(&service);
  if (err!=PNMPI_SUCCESS) {
    return MPI_ERROR_PNMPI;
  }

  // /*Retrieve remote next_clocks*/
  // sprintf(service.name,"clmpi_fetch_next_clocks");
  // service.fct=(PNMPI_Service_Fct_t) PNMPIMOD_fetch_next_clocks;
  // sprintf(service.sig,"ipp");
  // err=PNMPI_Service_RegisterService(&service);
  // if (err!=PNMPI_SUCCESS) {
  //   return MPI_ERROR_PNMPI;
  // }


  // /*Get my next_clock */
  // sprintf(service.name,"clmpi_get_next_clock");
  // service.fct=(PNMPI_Service_Fct_t) PNMPIMOD_get_next_clock;
  // sprintf(service.sig,"p");
  // err=PNMPI_Service_RegisterService(&service);
  // if (err!=PNMPI_SUCCESS) {
  //   return MPI_ERROR_PNMPI;
  // }

  // /*Set my next_clock*/
  // sprintf(service.name,"clmpi_set_next_clock");
  // service.fct=(PNMPI_Service_Fct_t) PNMPIMOD_set_next_clock;
  // sprintf(service.sig,"p");
  // err=PNMPI_Service_RegisterService(&service);
  // if (err!=PNMPI_SUCCESS) {
  //   return MPI_ERROR_PNMPI;
  // }

  return err;
}

/*.......................................................*/
/* Init PNMPI*/

int cmpi_init_pnmpi() {
  int err;
  // PNMPI_modHandle_t handle_pb,handle_pbd,handle_status,handle_req;
  // PNMPI_Service_descriptor_t serv;
  // PNMPI_Global_descriptor_t global;
  // const char *vlevel_s;

  // /* query pb module */


  // err=PNMPI_Service_GetModuleByName(PNMPI_MODULE_PB,&handle_pb);
  // if (err!=PNMPI_SUCCESS)
  //   return err;

  // err=PNMPI_Service_GetServiceByName(handle_pb,"piggyback","ip",&serv);
  // if (err!=PNMPI_SUCCESS)
  //   return err;
  // pb_set=(PNMPIMOD_Piggyback_t) ((void*)serv.fct);
  pb_set=PNMPIMOD_Piggyback;

  // err=PNMPI_Service_GetServiceByName(handle_pb,"piggyback-size","i",&serv);
  // if (err!=PNMPI_SUCCESS)
  //   return err;
  // pb_setsize=(PNMPIMOD_Piggyback_Size_t) ((void*)serv.fct);
  pb_setsize=PNMPIMOD_Piggyback_Size;

  // err=PNMPI_Service_GetGlobalByName(handle_pb,"piggyback-offset",'i',&global);
  // if (err!=PNMPI_SUCCESS)
  //   return err;
  pb_offset=&piggyback_offset;


  // /* query the status module */
  // err=PNMPI_Service_GetModuleByName(PNMPI_MODULE_STATUS,&handle_status);
  // if (err!=PNMPI_SUCCESS)
  //   return err;

  // err=PNMPI_Service_GetGlobalByName(handle_status,"total-status-extension",'i',&global);
  // if (err!=PNMPI_SUCCESS)
  //   return err;
  TotalStatusExtension=&add_status_storage;

  // /* query the request module */
  // err=PNMPI_Service_GetModuleByName(PNMPI_MODULE_REQUEST,&handle_req);
  // if (err!=PNMPI_SUCCESS)
  //   return err;

  // err=PNMPI_Service_GetGlobalByName(handle_req,"status-offset",'i',&global);
  // if (err!=PNMPI_SUCCESS)
  //   return err;
  StatusOffsetInRequest=&PNMPIMOD_Request_offsetInStatus;

  // /* query own module */
  // err=PNMPI_Service_GetModuleByName(PNMPI_MODULE_CLMPI, &handle_pbd);
  // if (err!=PNMPI_SUCCESS)
  //   return err;

  pb_size= sizeof(size_t);
  run_set=1;
  run_check=1;

  err=pb_setsize(pb_size);
  if (err!=PNMPI_SUCCESS)
    return err;

  pbdata=(char *)malloc(pb_size);
  if (pbdata==NULL) return MPI_ERROR_MEM;

  return PNMPI_SUCCESS;
}


/*.......................................................*/
/* Init */


int MPI_Init(int *argc, char ***argv)
{
  int err;
  int provided;

  cmpi_init_pb_clock();
  err = cmpi_init_pnmpi();
  if (err != PNMPI_SUCCESS) {
    fprintf(stderr, "cmpi_init_pnmpi failed\n");
  }

  err = PMPI_Init(argc, argv);

  PMPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  cmpi_set_tick();
  //  cmpi_create_window();
  return err;
}

int MPI_Init_thread(int *argc, char ***argv, int required, int *provided)
{
  int err;

  cmpi_init_pb_clock();
  err = cmpi_init_pnmpi();
  if (err != PNMPI_SUCCESS) {
    fprintf(stderr, "cmpi_init_pnmpi failed\n");
  }

  err = PMPI_Init_thread(argc,argv, required, provided);

  PMPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  cmpi_set_tick();
  //  cmpi_create_window();
  start = MPI_Wtime();
  return err;
}


/*.......................................................*/
/* setting PB if enabled */


#if MPI_VERSION == 1 || MPI_VERSION == 2
int MPI_Send(void *buf, int num, MPI_Datatype dtype, int node, int tag, MPI_Comm comm)
#else
int MPI_Send(const void *buf, int num, MPI_Datatype dtype, int node, int tag, MPI_Comm comm)
#endif
{
  int res;
  PBSET;
  res=PMPI_Send(buf,num,dtype,node,tag,comm);
  fprintf(stderr, " Rank: %d,  Send: dest: %d, tag: %d, clock: %lu\n", my_rank, node, tag, pb_clocks->local_clock);
  pb_clocks->local_clock++;
  local_sent_clock =  pb_clocks->local_clock;
  return res;
}

#if MPI_VERSION == 1  || MPI_VERSION == 2
int MPI_Bsend(void* buf, int num, MPI_Datatype dtype, int node, int tag, MPI_Comm comm)
#else
int MPI_Bsend(const void* buf, int num, MPI_Datatype dtype, int node, int tag, MPI_Comm comm)
#endif
{
  int res;
  PBSET;
  res=PMPI_Bsend(buf,num,dtype,node,tag,comm);
  pb_clocks->local_clock++;
  return res;
}

#if MPI_VERSION == 1 || MPI_VERSION == 2
int MPI_Ssend(void* buf, int num, MPI_Datatype dtype, int node, int tag, MPI_Comm comm)
#else
int MPI_Ssend(const void* buf, int num, MPI_Datatype dtype, int node, int tag, MPI_Comm comm)
#endif
{
  int res;
  PBSET;
  res=PMPI_Ssend(buf,num,dtype,node,tag,comm);

  pb_clocks->local_clock++;
  return res;
}

#if MPI_VERSION == 1 || MPI_VERSION == 2
int MPI_Rsend(void* buf, int num, MPI_Datatype dtype, int node, int tag, MPI_Comm comm)
#else
int MPI_Rsend(const void* buf, int num, MPI_Datatype dtype, int node, int tag, MPI_Comm comm)
#endif
{
  int res;
  PBSET;
  res=PMPI_Rsend(buf,num,dtype,node,tag,comm);
  pb_clocks->local_clock++;
  return res;
}

#if MPI_VERSION == 1 || MPI_VERSION == 2
int MPI_Isend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
#else
int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
#endif
{
  int err;
  PBSET;
  int datatype_size;
  //  if (dest == 0) fprintf(stderr, "   %d %d\n", local_clock, rank);
  //  if (my_rank == 0) fprintf(stderr, "request: %p, my_rank: %d Send: dest: %d tag: %d clock: %lu\n", *request, my_rank, dest, tag, local_clock);
  // if (dest == 1) fprintf(stderr, "my_rank: %d Send: dest: %d tag: %d clock: %d\n", my_rank, dest, tag, local_clock);
  err=PMPI_Isend(buf,count,datatype,dest,tag,comm,request);
  //  fprintf(stderr, "CLMPI:  %d: registered request(SEND): %p\n", my_rank, *request);
  // PMPI_Type_size(datatype, &datatype_size);
  // fprintf(stderr, "Rank: %d Send: dest: %d tag: %d clock: %lu, request: %p sent_clock: %lu dsize: %d\n", 
  // 	  my_rank, dest, tag, pb_clocks->local_clock, *request, local_sent_clock, datatype_size * count);

#ifdef  DBG_SC
  request_to_local_clock[*request] = pb_clocks->local_clock;
  // fprintf(stderr, "CLMPI:  %d: registered request(SEND): %p\n", my_rank, *request);
#endif
  //  sleep(1);
  pb_clocks->local_clock++;

  fprintf(stderr, "CLMPI:  %d: %f %lu\n", my_rank, MPI_Wtime(), pb_clocks->local_clock);
  
  return err;
}

#if MPI_VERSION == 1 || MPI_VERSION == 2
int MPI_Ibsend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
#else
int MPI_Ibsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
#endif
{
  int err;
  PBSET;
  err=PMPI_Ibsend(buf,count,datatype,dest,tag,comm,request);
#ifdef  DBG_SC
  request_to_local_clock[*request] = pb_clocks->local_clock;
#endif
  pb_clocks->local_clock++;
  return err;
}

#if MPI_VERSION == 1 || MPI_VERSION == 2
int MPI_Issend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
#else
int MPI_Issend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
#endif
{
  int err;
  PBSET;
  err=PMPI_Issend(buf,count,datatype,dest,tag,comm,request);
#ifdef  DBG_SC
  request_to_local_clock[*request] = pb_clocks->local_clock;
#endif
  pb_clocks->local_clock++;
  return err;
}

#if MPI_VERSION == 1 || MPI_VERSION == 2
int MPI_Irsend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
#else
int MPI_Irsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
#endif
{
  int err;
  PBSET;
  err=PMPI_Irsend(buf,count,datatype,dest,tag,comm,request);
#ifdef  DBG_SC
  request_to_local_clock[*request] = pb_clocks->local_clock;
#endif
  pb_clocks->local_clock++;
  return err;
}



/*.......................................................*/
/* Checks if enabled */

int MPI_Irecv(void* buf, int num, MPI_Datatype dtype, int node,
	      int tag, MPI_Comm comm, MPI_Request *request)
{
  int err;
  err=PMPI_Irecv(buf,num,dtype,node,tag,comm, request);
  irecv_request_map[*request] = request;  
  //  fprintf(stderr, "CLMPI:  %d: registered request(RECV): %p size: %d\n", my_rank, *request, irecv_request_map.size());

  return err;
}

int MPI_Recv(void* buf, int num, MPI_Datatype dtype, int node,
             int tag, MPI_Comm comm, MPI_Status *status)
{
  int err;
  err=PMPI_Recv(buf,num,dtype,node,tag,comm,status);
  if (run_check==0) return err;
  if (err == MPI_SUCCESS) cmpi_sync_clock(status); 
  return err;
}


int MPI_Sendrecv(void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, 
		 void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag,
		 MPI_Comm comm, MPI_Status *status)
{
  int err;
  err=PMPI_Sendrecv(sendbuf,sendcount,sendtype,dest,sendtag,
		    recvbuf,recvcount,recvtype,source,recvtag,comm,status);
  if (run_check==0) return err;
  if (err == MPI_SUCCESS) cmpi_sync_clock(status); 
  return err;
}

int MPI_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag, 
			 MPI_Comm comm, MPI_Status *status)
{
  int err;
  err=PMPI_Sendrecv_replace(buf,count,datatype,sendtag,dest,recvtag,dest,comm,status);
  if (err == MPI_SUCCESS) cmpi_sync_clock(status); 
  return err;
}


int MPI_Wait(MPI_Request *request, MPI_Status *status)
{
  fprintf(stderr, "%s is not supported yet\n", __func__);  exit(1);
  int err;
  clmpi_init_registered_clocks(request, 1);
  err=PMPI_Wait(request,status);
  if (run_check==0) return err;
  if (COMM_REQ_FROM_STATUS(status).inreq!=MPI_REQUEST_NULL) {
      if (COMM_REQ_FROM_STATUS(status).type==PNMPIMOD_REQUESTS_RECV) {
	  if (err == MPI_SUCCESS) cmpi_sync_clock(status); 
	  clmpi_irecv_test_erase(COMM_REQ_FROM_STATUS(status).inreq);
      }
  }
  return err;
}


int MPI_Waitany(int count, MPI_Request *array_of_requests, int *index, MPI_Status *status)
{
  fprintf(stderr, "%s is not supported yet\n", __func__);  exit(1);
  int err;
  clmpi_init_registered_clocks(array_of_requests, count);
  err=PMPI_Waitany(count,array_of_requests,index,status);
  if (run_check==0) return err;
  if (COMM_REQ_FROM_STATUS(status).inreq!=MPI_REQUEST_NULL) {
      if (COMM_REQ_FROM_STATUS(status).type==PNMPIMOD_REQUESTS_RECV) {
	  if (err == MPI_SUCCESS) cmpi_sync_clock(status); 
	  clmpi_irecv_test_erase(COMM_REQ_FROM_STATUS(status).inreq);
      }
  }
  return err;
}

int MPI_Waitsome(int count, MPI_Request *array_of_requests, int *outcount, int *array_of_indices, MPI_Status *array_of_statuses)
{
  fprintf(stderr, "%s is not supported yet\n", __func__);  exit(1);
  int err,i;
  clmpi_init_registered_clocks(array_of_requests, count);
  err=PMPI_Waitsome(count,array_of_requests,outcount,array_of_indices,array_of_statuses);
  if (run_check==0) return err;
  for (i=0; i<*outcount; i++) {
    if (COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).inreq!=MPI_REQUEST_NULL) {
      if (COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).type==PNMPIMOD_REQUESTS_RECV)  {
	if (err == MPI_SUCCESS) cmpi_sync_clock_at(array_of_statuses, count, i, array_of_indices[i]); 
	clmpi_irecv_test_erase(COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).inreq);
      }
    }
  }
  return err;  
}


int MPI_Waitall(int count, MPI_Request *array_of_requests, MPI_Status *array_of_statuses)
{
  int err,i;
  //TODO: for now, I moved this function call into the loop #1
  clmpi_init_registered_clocks(array_of_requests, count);
  err=PMPI_Waitall(count,array_of_requests,array_of_statuses);
  if (run_check==0) return err;
  for (i=0; i<count; i++) {
    if (COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).inreq!=MPI_REQUEST_NULL)  {
      if (COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).type==PNMPIMOD_REQUESTS_RECV)   {
	//	/* Loop: #1 ->*/ clmpi_init_registered_clocks(&array_of_requests[i], 1);
	if (err == MPI_SUCCESS) cmpi_sync_clock_at(array_of_statuses, count, i, i); 
	//	fprintf(stderr, "rank %d: call erase at waitall: Request: %p\n", my_rank, array_of_requests[0]);
	clmpi_irecv_test_erase(COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).inreq);
	//	fprintf(stderr, "rank %d: call erase end at waitall\n", my_rank);
      } else {

#ifdef DBG_SC
	MPI_Request tmp_req = COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).inreq;
	size_t tmp_clock;
	cmpi_update_local_sent_clock(tmp_req);
	// if (request_to_local_clock.find(tmp_req) != request_to_local_clock.end()) {
	//   tmp_clock = request_to_local_clock[tmp_req];
	//   if (local_sent_clock < tmp_clock) {
	//     local_sent_clock = tmp_clock;
	//   }
	//   request_to_local_clock.erase(tmp_req);
	//   //	  fprintf(stderr, "CLMPI:  %d: erase registered request(SEND): %p\n", my_rank, tmp_req);
	// } else {
	//   fprintf(stderr, "no such send reqeust \n");
	//   exit(1);
	// }
#endif
      }
    }
  }
  return err;
}


int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status)
{
  int err;
  size_t tmp_clock;


  MPI_Request req = *request;
  clmpi_init_registered_clocks(request, 1);
  err=PMPI_Test(request, flag, status);
  
  if (run_check==0) return err;
  if ((*flag) && (COMM_REQ_FROM_STATUS(status).inreq!=MPI_REQUEST_NULL)) {
    if (COMM_REQ_FROM_STATUS(status).type==PNMPIMOD_REQUESTS_RECV) {
      if (err == MPI_SUCCESS) cmpi_sync_clock(status);
      //      fprintf(stderr, "CLMPI:  %d: request: %p -> %p\n", my_rank, req, *request);
      clmpi_irecv_test_erase(COMM_REQ_FROM_STATUS(status).inreq);
    } else {
      if (err == MPI_SUCCESS) {
	//	size_t sender_clock;		
	//	sender_clock = STATUS_STORAGE_ARRAY(status,*pb_offset, *TotalStatusExtension, size_t, 0, 1);
	//	fprintf(stderr, "sent clock %lu\n", sender_clock);
	if (registered_buff_clocks != NULL) {
	  registered_buff_clocks[0] = PNMPI_MODULE_CLMPI_SEND_REQ_CLOCK;
	  //	  fprintf(stderr, "[1 |%d|]\n", 1, status->MPI_SOURCE);
	}  else {
	  fprintf(stderr, "registered_buff_clocks is NULL\n" );
	  exit(1);
	}

#ifdef DBG_SC
	MPI_Request tmp_req = COMM_REQ_FROM_STATUS(status).inreq;
	size_t tmp_clock;
	cmpi_update_local_sent_clock(tmp_req);
	// if (request_to_local_clock.find(tmp_req) != request_to_local_clock.end()) {
	//   tmp_clock = request_to_local_clock[tmp_req];
	//   if (local_sent_clock < tmp_clock) {
	//     local_sent_clock = tmp_clock;
	//   }
	//   request_to_local_clock.erase(tmp_req);
	//   //	  fprintf(stderr, "CLMPI:  %d: erase registered request(SEND): %p\n", my_rank, tmp_req);
	// } else {
	//   //	  fprintf(stderr, "CLMPI:  %d: no such send request: %p\n", my_rank, tmp_req);
	//   exit(1);
	// }
#endif
      }

    }
  }

  /*COMM_REQ_FROM_STATUS(status).type works for only matched message -> so coment out*/
  /*
  if (*flag == 0) {
    if (!(COMM_REQ_FROM_STATUS(status).type==PNMPIMOD_REQUESTS_RECV)) {
      if (registered_buff_clocks == NULL) {
  	fprintf(stderr, "registered_buff_clocks is NULL\n" );
  	exit(1);
      }
      registered_buff_clocks[0] = PNMPI_MODULE_CLMPI_SEND_REQ_CLOCK;
    }
  }
  */

  registered_buff_clocks = NULL;
  registered_buff_length = -1;
  return err;
}

int MPI_Testany(int count, MPI_Request *array_of_requests, int *index, int *flag, MPI_Status *status)
{
  fprintf(stderr, "%s is not supported yet\n", __func__);  exit(1);
  int err;
  clmpi_init_registered_clocks(array_of_requests, count);
  err=PMPI_Testany(count,array_of_requests,index,flag,status);
  if (run_check==0) return err;
  if ((*flag) && (COMM_REQ_FROM_STATUS(status).inreq!=MPI_REQUEST_NULL)) {
    if (COMM_REQ_FROM_STATUS(status).type==PNMPIMOD_REQUESTS_RECV) {
      if (err == MPI_SUCCESS) cmpi_sync_clock(status); 
      clmpi_irecv_test_erase(COMM_REQ_FROM_STATUS(status).inreq);
    } else {
      if (err == MPI_SUCCESS) {
	int matched_index;
	if (registered_buff_clocks != NULL) {
	  registered_buff_clocks[0] = PNMPI_MODULE_CLMPI_SEND_REQ_CLOCK;
	  //	  fprintf(stderr, "ReMPI:%d: [%d |%d|] (outcount: %d)\n", my_rank, 1, array_of_statuses[matched_index].MPI_SOURCE, *outcount);
	}  else {
	  fprintf(stderr, "registered_buff_clocks is NULL\n" );
	  exit(1);
	}
      }
    }
  }

  /*COMM_REQ_FROM_STATUS(status).type works for only matched message -> so coment out*/
  /*
  if (*flag == 0) {
    if (!(COMM_REQ_FROM_STATUS(status).type==PNMPIMOD_REQUESTS_RECV)) {
      if (registered_buff_clocks == NULL) {
  	fprintf(stderr, "registered_buff_clocks is NULL\n" );
  	exit(1);
      }
      registered_buff_clocks[0] = PNMPI_MODULE_CLMPI_SEND_REQ_CLOCK;
    }
  }
  */

  registered_buff_clocks = NULL;
  registered_buff_length = -1;
  return err;  
}

int MPI_Testsome(int count, MPI_Request *array_of_requests, int *outcount, int *array_of_indices, MPI_Status *array_of_statuses)
{
  int err,i;
  

  clmpi_init_registered_clocks(array_of_requests, count);
  // for (i = 0; i < incount; i++) {
  //   fprintf(stderr, "CLM: req: %p\n", array_of_requests[i]);
  // }
  err=PMPI_Testsome(count,array_of_requests,outcount,array_of_indices,array_of_statuses);


  if (run_check==0) return err;
#if 0
  if (*outcount > 0) {
        fprintf(stderr, "ReMPI:%d: outcount: %d\n", my_rank, *outcount);
  }
#endif
  for (i=0; i<*outcount; i++) {
    if (COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).inreq!=MPI_REQUEST_NULL) {
      // if (rank == 1) fprintf(stderr, "satatus checking ! %d %d %d\n", COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).type, 
      // 			     PNMPIMOD_REQUESTS_RECV, PNMPIMOD_REQUESTS_SEND);
      if (COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).type==PNMPIMOD_REQUESTS_RECV) {
	if (err == MPI_SUCCESS) cmpi_sync_clock_at(array_of_statuses, count, i, array_of_indices[i]); 
	clmpi_irecv_test_erase(COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).inreq);
      } else {
	if (err == MPI_SUCCESS) {
	  int matched_index;
	  matched_index = array_of_indices[i];
	  if (registered_buff_clocks != NULL) { 
	    registered_buff_clocks[matched_index] = PNMPI_MODULE_CLMPI_SEND_REQ_CLOCK;
	  } else {
	    fprintf(stderr, "registered_buff_clocks is NULL\n" );
	    exit(1);
	  }
	}
#ifdef DBG_SC
	MPI_Request tmp_req = COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).inreq;
	size_t tmp_clock;
	cmpi_update_local_sent_clock(tmp_req);
	// if (request_to_local_clock.find(tmp_req) != request_to_local_clock.end()) {
	//   tmp_clock = request_to_local_clock[tmp_req];
	//   if (local_sent_clock < tmp_clock) {
	//     local_sent_clock = tmp_clock;
	//   }
	//   request_to_local_clock.erase(tmp_req);
	//   //	  fprintf(stderr, "CLMPI:  %d: erase registered request(SEND): %p\n", my_rank, tmp_req);
	// } else {
	//   fprintf(stderr, "no such send reqeust \n");
	//   exit(1);
	// }
#endif
      }
    } else {
      fprintf(stderr, "request is NULL in testsome of clmpi.cpp\n");
      exit(1);
    }
  }

  /*COMM_REQ_FROM_STATUS(status).type works for only matched message -> so coment out*/
  /*
  if (*outcount == 0) {
    for (i = 0; i < count; i++) {
      if (COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).type != PNMPIMOD_REQUESTS_RECV ) {
  	if (registered_buff_clocks == NULL) {
  	  fprintf(stderr, "registered_buff_clocks is NULL\n" );
  	  exit(1);
  	}
  	registered_buff_clocks[i] = PNMPI_MODULE_CLMPI_SEND_REQ_CLOCK;
	if (my_rank == 1) fprintf(stderr, "NOOOOOOOOOO ! %d %d %d\n", COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).type, 
			       PNMPIMOD_REQUESTS_RECV, PNMPIMOD_REQUESTS_SEND);
      }
    }
  }
  */

  registered_buff_clocks = NULL;
  registered_buff_length = -1;
  //  local_clock += local_tick;
  return err;  
}

int MPI_Testall(int count, MPI_Request *array_of_requests, int *flag, MPI_Status *array_of_statuses)
{
  fprintf(stderr, "%s is not supported yet\n", __func__);  exit(1);
  int err,i;
  clmpi_init_registered_clocks(array_of_requests, count);
  err=PMPI_Testall(count, array_of_requests, flag, array_of_statuses);
  if (run_check==0) return err;
  if (*flag)  {
    for (i=0; i<count; i++) {
      if (COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).inreq!=MPI_REQUEST_NULL)  {
	if (COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).type==PNMPIMOD_REQUESTS_RECV) {
	  if (err == MPI_SUCCESS) cmpi_sync_clock_at(array_of_statuses, count, i, i); 
	  clmpi_irecv_test_erase(COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).inreq);
	} else {
	  if (err == MPI_SUCCESS) {
	    if (registered_buff_clocks != NULL) {
	      registered_buff_clocks[i] = PNMPI_MODULE_CLMPI_SEND_REQ_CLOCK;
	      //	  fprintf(stderr, "ReMPI:%d: [%d |%d|] (outcount: %d)\n", my_rank, 1, array_of_statuses[matched_index].MPI_SOURCE, *outcount);
	    }  else {
	      fprintf(stderr, "registered_buff_clocks is NULL\n" );
	      exit(1);
	    }
	  }
	}
      } 
    }
  }

  /*COMM_REQ_FROM_STATUS(status).type works for only matched message -> so coment out*/
  /*
  if (*flag == 0) {
    for (i = 0; i < count; i++) {
      if (!(COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).type==PNMPIMOD_REQUESTS_RECV)) {
  	if (registered_buff_clocks == NULL) {
  	  fprintf(stderr, "registered_buff_clocks is NULL\n" );
  	  exit(1);
  	}
  	registered_buff_clocks[i] = PNMPI_MODULE_CLMPI_SEND_REQ_CLOCK;
      }
    }
  }
  */

  registered_buff_clocks = NULL;
  registered_buff_length = -1;

  return err;
}

int MPI_Get_count(MPI_Status *arg_0, MPI_Datatype arg_1, int *arg_2) {
  int _wrap_py_return_val = 0;
  {
    /*TODO: arg_2 include piggyback message size, so subtract the size
     for now, we subtract in REMPI level, not here*/
    _wrap_py_return_val = PMPI_Get_count(arg_0, arg_1, arg_2);
  }    return _wrap_py_return_val;
}


int MPI_Finalize()
{
  //  fprintf(stderr, "clock: %d %d\n", local_clock, my_rank);
  //  PMPI_Win_unlock_all(mpi_clock_win);
  //  PMPI_Win_free(&mpi_clock_win);
  //  PMPI_Comm_free(&mpi_clock_win_comm);
  return PMPI_Finalize();

}


/** ========================================================
 *   MPI Collectives
 *  ======================================================== **/


/* ================== C Wrappers for MPI_Allreduce ================== */
_EXTERN_C_ int PMPI_Allreduce(const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5);
_EXTERN_C_ int MPI_Allreduce(const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5) {
  int _wrap_py_return_val = 0;
  {
    //    fprintf(stderr, "CLMPI:  %d: Allreduce\n", my_rank);
    _wrap_py_return_val = PMPI_Allreduce(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
  }    return _wrap_py_return_val;
}


/* ================== C Wrappers for MPI_Reduce ================== */
_EXTERN_C_ int PMPI_Reduce(const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, int arg_5, MPI_Comm arg_6);
_EXTERN_C_ int MPI_Reduce(const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, int arg_5, MPI_Comm arg_6) {
  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = PMPI_Reduce(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
  }    return _wrap_py_return_val;
}


/* ================== C Wrappers for MPI_Scan ================== */
_EXTERN_C_ int PMPI_Scan(const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5);
_EXTERN_C_ int MPI_Scan(const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5) {
  int _wrap_py_return_val = 0;
  {
    fprintf(stderr, "CLMPI: %s is not supported yet", __func__);
    exit(1);  
    _wrap_py_return_val = PMPI_Scan(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
  }    return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Allgather ================== */
_EXTERN_C_ int PMPI_Allgather(const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, MPI_Comm arg_6);
_EXTERN_C_ int MPI_Allgather(const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, MPI_Comm arg_6) {
  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = PMPI_Allgather(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
  }    return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Gatherv ================== */
_EXTERN_C_ int PMPI_Gatherv(const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, const int *arg_4, const int *arg_5, MPI_Datatype arg_6, int arg_7, MPI_Comm arg_8);
_EXTERN_C_ int MPI_Gatherv(const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, const int *arg_4, const int *arg_5, MPI_Datatype arg_6, int arg_7, MPI_Comm arg_8) {
  int _wrap_py_return_val = 0;
  {
    fprintf(stderr, "CLMPI: %s is not supported yet", __func__);
    exit(1);  
    _wrap_py_return_val = PMPI_Gatherv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7, arg_8);
  }    return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Reduce_scatter ================== */
_EXTERN_C_ int PMPI_Reduce_scatter(const void *arg_0, void *arg_1, const int *arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5);
_EXTERN_C_ int MPI_Reduce_scatter(const void *arg_0, void *arg_1, const int *arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5) {
  int _wrap_py_return_val = 0;
  {
    fprintf(stderr, "CLMPI: %s is not supported yet", __func__);
    exit(1);  
    _wrap_py_return_val = PMPI_Reduce_scatter(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
  }    return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Scatterv ================== */
_EXTERN_C_ int PMPI_Scatterv(const void *arg_0, const int *arg_1, const int *arg_2, MPI_Datatype arg_3, void *arg_4, int arg_5, MPI_Datatype arg_6, int arg_7, MPI_Comm arg_8);
_EXTERN_C_ int MPI_Scatterv(const void *arg_0, const int *arg_1, const int *arg_2, MPI_Datatype arg_3, void *arg_4, int arg_5, MPI_Datatype arg_6, int arg_7, MPI_Comm arg_8) {
  int _wrap_py_return_val = 0;
  {
    fprintf(stderr, "CLMPI: %s is not supported yet", __func__);
    exit(1);  
    _wrap_py_return_val = PMPI_Scatterv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7, arg_8);
  }    return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Allgatherv ================== */
_EXTERN_C_ int PMPI_Allgatherv(const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, const int *arg_4, const int *arg_5, MPI_Datatype arg_6, MPI_Comm arg_7);
_EXTERN_C_ int MPI_Allgatherv(const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, const int *arg_4, const int *arg_5, MPI_Datatype arg_6, MPI_Comm arg_7) {
  int _wrap_py_return_val = 0;
  {
    fprintf(stderr, "CLMPI: %s is not supported yet", __func__);
    exit(1);
    _wrap_py_return_val = PMPI_Allgatherv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7);
  }    return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Scatter ================== */
_EXTERN_C_ int PMPI_Scatter(const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, int arg_6, MPI_Comm arg_7);
_EXTERN_C_ int MPI_Scatter(const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, int arg_6, MPI_Comm arg_7) {
  int _wrap_py_return_val = 0;
  {
    fprintf(stderr, "CLMPI: %s is not supported yet", __func__);
    exit(1);
    _wrap_py_return_val = PMPI_Scatter(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7);
  }    return _wrap_py_return_val;
}


/* ================== C Wrappers for MPI_Bcast ================== */
_EXTERN_C_ int PMPI_Bcast(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, MPI_Comm arg_4);
_EXTERN_C_ int MPI_Bcast(void *arg_0, int arg_1, MPI_Datatype arg_2, int arg_3, MPI_Comm arg_4) {
  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = PMPI_Bcast(arg_0, arg_1, arg_2, arg_3, arg_4);
  }    return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Alltoall ================== */
_EXTERN_C_ int PMPI_Alltoall(const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, MPI_Comm arg_6);
_EXTERN_C_ int MPI_Alltoall(const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, MPI_Comm arg_6) {
  int _wrap_py_return_val = 0;
  {
    fprintf(stderr, "CLMPI: %s is not supported yet", __func__);
    exit(1);
    _wrap_py_return_val = PMPI_Alltoall(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
  }    return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Gather ================== */
_EXTERN_C_ int PMPI_Gather(const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, int arg_6, MPI_Comm arg_7);
_EXTERN_C_ int MPI_Gather(const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, int arg_6, MPI_Comm arg_7) {
  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = PMPI_Gather(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7);
  }    return _wrap_py_return_val;
}


/* ================== C Wrappers for MPI_Barrier ================== */
_EXTERN_C_ int PMPI_Barrier(MPI_Comm arg_0);
_EXTERN_C_ int MPI_Barrier(MPI_Comm arg_0) {
  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = PMPI_Barrier(arg_0);
  }    return _wrap_py_return_val;
}



/* ================== C Wrappers for MPI_Alltoallv ================== */
_EXTERN_C_ int PMPI_Alltoallv(const void *arg_0, const int *arg_1, const int *arg_2, MPI_Datatype arg_3, void *arg_4, const int *arg_5, const int *arg_6, MPI_Datatype arg_7, MPI_Comm arg_8);
_EXTERN_C_ int MPI_Alltoallv(const void *arg_0, const int *arg_1, const int *arg_2, MPI_Datatype arg_3, void *arg_4, const int *arg_5, const int *arg_6, MPI_Datatype arg_7, MPI_Comm arg_8)
{
  int _wrap_py_return_val = 0;
  {
    fprintf(stderr, "CLMPI: %s is not supported yet", __func__);
    exit(1);
    _wrap_py_return_val = PMPI_Alltoallv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7, arg_8);
  }    return _wrap_py_return_val;
}



/*.......................................................*/
/* The End. */