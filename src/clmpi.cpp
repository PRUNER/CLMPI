#include "unordered_map"

#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#include "mpi.h"
#include "clmpi_common.h"
#include "clmpi_status.h"
#include "clmpi_request.h"
#include "clmpi_piggyback.h"
#include "clmpi.h"

#ifndef _EXTERN_C_
#ifdef __cplusplus
#define _EXTERN_C_ extern "C"
#else /* __cplusplus */
#define _EXTERN_C_
#endif /* __cplusplus */
#endif /* _EXTERN_C_ */

using namespace std;

static int pb_int = 0;
static int run_check=1;
static int run_set=1;


typedef struct pb_clocks{
  size_t local_clock;
} pb_clocks_t;

#define DBG_SC

#ifdef DBG_SC
static size_t local_sent_clock = PNMPI_MODULE_CLMPI_INITIAL_CLOCK;
static unordered_map<MPI_Request, size_t> request_to_local_clock;
#endif

static pb_clocks_t *pb_clocks;

static size_t local_tick = 0;
static char *pbdata;
static int my_rank = -1;

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


static PNMPIMOD_Piggyback_t       pb_set;
static PNMPIMOD_Piggyback_Size_t  pb_setsize;
static int *pb_offset;
static int pb_size=sizeof(pb_clocks_t);
static int *TotalStatusExtension;
static int *StatusOffsetInRequest;

#define COMM_REQ_FROM_STATUS(status) REQ_FROM_STATUS(status,(*StatusOffsetInRequest))
#define COMM_REQ_FROM_STATUSARRAY(status,count,num) REQ_FROM_STATUSARRAY(status,(*StatusOffsetInRequest),*TotalStatusExtension,count,num)


/*------------------------------------------------------------*/
/* External service routines */

static size_t *registered_buff_clocks = NULL;
static int     registered_buff_length = -1;
static int     sync_clock = 1;
static unordered_map<MPI_Request, MPI_Request*> irecv_request_map;

static void clmpi_init_registered_clocks(MPI_Request *request, int length) {
  if (!sync_clock) return;
  
  //  fprintf(stderr, "check reigstered: %p\n", registered_buff_clocks);
  if (registered_buff_clocks == NULL) {
    fprintf(stderr, "CLMPI: registered clock is NULL\n");
    *registered_buff_clocks = 1;
    exit(1);
  }

  for (int i = 0; i < length; i++) {
    if (irecv_request_map.find(request[i]) != irecv_request_map.end()) {
      /*If this reuquest[i] is from irecv*/
      registered_buff_clocks[i] = PNMPI_MODULE_CLMPI_UNMATCHED_RECV_REQ_CLOCK;
    } else {
      /*If this reuquest[i] is from isend*/
      registered_buff_clocks[i] = PNMPI_MODULE_CLMPI_SEND_REQ_CLOCK;
    }    
  }
}

static void clmpi_irecv_test_erase(MPI_Request request) {
  if (irecv_request_map.find(request) == irecv_request_map.end()) {
    fprintf(stderr, "CLMPI:  %d: request: %p does not exist\n", my_rank, (void*)request);
    exit(1);
  }
  irecv_request_map.erase(request);
}

int tick = 1;

static void clmpi_tick_clock()
{
  if (sync_clock) {
    pb_clocks->local_clock += tick;
  }
  return;
}

void CLMPI_tick_clock()
{
  fprintf(stderr, "non expected to be called\n");
  exit(1);
  clmpi_tick_clock();
  return;
}

static void clmpi_update_clock(size_t recv_clock) {
  if (sync_clock) {
    if (pb_clocks->local_clock <= recv_clock) {
      pb_clocks->local_clock = recv_clock;
      //      tick++;
    } else {
      //      if (tick > 1) tick--;
    }
    clmpi_tick_clock();
  }
}


static char** cmpi_btrace() 
{
  int nptrs;
  void *buffer[100];
  char **strings;

  nptrs = backtrace(buffer, 100);
  /* backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)*/
  strings = (char**)backtrace_symbols(buffer, nptrs);
  if (strings == NULL) {
    perror("backtrace_symbols");
    exit(EXIT_FAILURE);
  }   
  return strings;
}

static void cmpi_pring_addr()
{
  char **strings;
  strings = cmpi_btrace();
  fprintf(stderr, "%s", strings[9]);
  free(strings);
}

static void cmpi_sync_clock_at(MPI_Status *status, int count, int i, int matched_index)
{
  size_t sender_clock;		
  sender_clock = STATUS_STORAGE_ARRAY(status,*pb_offset, *TotalStatusExtension, size_t, count, i);
  if (registered_buff_clocks != NULL) {
    if (registered_buff_length != count) {
      fprintf(stderr, "Registered clock buffufer lengths (%d) is different to incount (%d)", 
		registered_buff_length, count);
    }
    registered_buff_clocks[matched_index] = sender_clock;
  }
  clmpi_update_clock(sender_clock);
  return; 
}

static void cmpi_sync_clock(MPI_Status *status)
{
  cmpi_sync_clock_at(status, 1, 0, 0);
  return;
}

static void cmpi_set_tick()
{
  local_tick = 1;
}

static void cmpi_init_pb_clock()
{
  pb_clocks    = (struct pb_clocks*) malloc(sizeof(struct pb_clocks));
  if (pb_clocks == NULL) {
    fprintf(stderr, "clmpi malloc failed\n");
  }
  pb_clocks->local_clock   = PNMPI_MODULE_CLMPI_INITIAL_CLOCK;
  return;
}

static void cmpi_update_local_sent_clock(MPI_Request tmp_req)
{
  if (request_to_local_clock.find(tmp_req) != request_to_local_clock.end()) {
    request_to_local_clock.erase(tmp_req);
  } else {
    fprintf(stderr, "no such send reqeust \n");
    exit(1);
  }
  if (request_to_local_clock.empty()) {
    local_sent_clock = pb_clocks->local_clock - 1;
  }
}


static int mpi_test(MPI_Request *request, int *flag, MPI_Status *status)
{
  int err;

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
	if (registered_buff_clocks != NULL) {
	  registered_buff_clocks[0] = PNMPI_MODULE_CLMPI_SEND_REQ_CLOCK;
	}  else {
	  fprintf(stderr, "registered_buff_clocks is NULL\n" );
	  exit(1);
	}

#ifdef DBG_SC
	MPI_Request tmp_req = COMM_REQ_FROM_STATUS(status).inreq;
	cmpi_update_local_sent_clock(tmp_req);
#endif
      }

    }
  }

  registered_buff_clocks = NULL;
  registered_buff_length = -1;
  return err;
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
  //  fprintf(stderr, "reigstered: %p\n", registered_buff_clocks);
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
  if (pb_clocks == NULL) {
    fprintf(stderr, "pb_clocks is NULL\n");
    exit(1);
  }
  ret = PMPI_Allreduce(&pb_clocks->local_clock, &clock_max, 1, MPI_UNSIGNED_LONG, MPI_MAX, comm);

  pb_clocks->local_clock = clock_max;
  clmpi_tick_clock();
  return ret;
}


#ifdef  DBG_SC
int PNMPIMOD_get_local_sent_clock(size_t *clock)
{ 
  fprintf(stderr, "PNMPIMOD_get_local_sent_cloc is called\n");
  assert(0);
  exit(1);
  *clock = local_sent_clock;
  return MPI_SUCCESS;
}
#endif

int PNMPIMOD_get_num_of_incomplete_sending_msg(size_t *size)
{
  *size =  request_to_local_clock.size();
  return MPI_SUCCESS;
}


/*.......................................................*/
/* Init PNMPI*/

int cmpi_init_pnmpi() {
  int err;
  pb_set=PNMPIMOD_Piggyback;
  pb_setsize=PNMPIMOD_Piggyback_Size;
  pb_offset=&piggyback_offset;
  TotalStatusExtension=&add_status_storage;
  StatusOffsetInRequest=&PNMPIMOD_Request_offsetInStatus;

  pb_size= sizeof(size_t);
  run_set=1;
  run_check=1;

  err=pb_setsize(pb_size);
  if (!err)
    return err;

  pbdata=(char *)malloc(pb_size);
  if (pbdata==NULL) return 1;

  return MPI_SUCCESS;
}


/*.......................................................*/
/* Init */


int MPI_Init(int *argc, char ***argv)
{
  int err;
  cmpi_init_pb_clock();
  err = cmpi_init_pnmpi();
  if (err != MPI_SUCCESS) {
    fprintf(stderr, "cmpi_init_pnmpi failed\n");
  }
  err = PMPI_Init(argc, argv);
  PMPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  cmpi_set_tick();
  return err;
}

int MPI_Init_thread(int *argc, char ***argv, int required, int *provided)
{
  int err;
  cmpi_init_pb_clock();
  err = cmpi_init_pnmpi();
  if (err != MPI_SUCCESS) {
    fprintf(stderr, "cmpi_init_pnmpi failed\n");
  }
  err = PMPI_Init_thread(argc,argv, required, provided);
  PMPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  cmpi_set_tick();
  start = MPI_Wtime();
  return err;
}


/*.......................................................*/
/* setting PB if enabled */

int MPI_Send(mpi_const void *buf, int num, MPI_Datatype dtype, int node, int tag, MPI_Comm comm)
{
  int res;
  PBSET;
  res=PMPI_Send(buf,num,dtype,node,tag,comm);
  local_sent_clock =  pb_clocks->local_clock;
  clmpi_tick_clock();
  return res;
}

int MPI_Send_init(mpi_const void *buf, int count, MPI_Datatype datatype,
		  int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  int res;
  PBSET;
  res=PMPI_Send_init(buf, count, datatype, dest, tag, comm, request);
#ifdef  DBG_SC
  request_to_local_clock[*request] = pb_clocks->local_clock;
#endif
  clmpi_tick_clock();
  return res;
}

int MPI_Bsend(mpi_const void* buf, int num, MPI_Datatype dtype, int node, int tag, MPI_Comm comm)
{
  int res;
  PBSET;
  res=PMPI_Bsend(buf,num,dtype,node,tag,comm);
  local_sent_clock =  pb_clocks->local_clock;
  clmpi_tick_clock();
  return res;
}

int MPI_Bsend_init(mpi_const void* buf, int count, MPI_Datatype datatype,
		   int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  int res;
  PBSET;
  res=PMPI_Bsend_init(buf, count, datatype, dest, tag, comm, request);
#ifdef  DBG_SC
  request_to_local_clock[*request] = pb_clocks->local_clock;
#endif
  clmpi_tick_clock();
  return res;
}


int MPI_Ssend(mpi_const void* buf, int num, MPI_Datatype dtype, int node, int tag, MPI_Comm comm)
{
  int res;
  PBSET;
  res=PMPI_Ssend(buf,num,dtype,node,tag,comm);
  local_sent_clock =  pb_clocks->local_clock;
  clmpi_tick_clock();
  return res;
}

int MPI_Ssend_init(mpi_const void *buf, int count, MPI_Datatype datatype,
		   int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  int res;
  PBSET;
  res=PMPI_Ssend_init(buf, count, datatype, dest, tag, comm, request);
#ifdef  DBG_SC
  request_to_local_clock[*request] = pb_clocks->local_clock;
#endif
  clmpi_tick_clock();
  return res;
}

int MPI_Rsend(mpi_const void* buf, int num, MPI_Datatype dtype, int node, int tag, MPI_Comm comm)
{
  int res;
  PBSET;
  res=PMPI_Rsend(buf,num,dtype,node,tag,comm);
  local_sent_clock =  pb_clocks->local_clock;
  clmpi_tick_clock();
  return res;
}

int MPI_Rsend_init(mpi_const void *buf, int count, MPI_Datatype datatype,
		   int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  int res;
  PBSET;
  res=PMPI_Rsend_init(buf, count, datatype, dest, tag, comm, request);
#ifdef  DBG_SC
  request_to_local_clock[*request] = pb_clocks->local_clock;
#endif
  clmpi_tick_clock();
  return res;
}


int MPI_Isend(mpi_const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  int err;
  PBSET;
  err=PMPI_Isend(buf,count,datatype,dest,tag,comm,request);
#ifdef  DBG_SC
  request_to_local_clock[*request] = pb_clocks->local_clock;
#endif
  clmpi_tick_clock();
  return err;
}

int MPI_Ibsend(mpi_const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  int err;
  PBSET;
  err=PMPI_Ibsend(buf,count,datatype,dest,tag,comm,request);
#ifdef  DBG_SC
  request_to_local_clock[*request] = pb_clocks->local_clock;
#endif
  clmpi_tick_clock();
  return err;
}

int MPI_Issend(mpi_const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  int err;
  PBSET;
  err=PMPI_Issend(buf,count,datatype,dest,tag,comm,request);
#ifdef  DBG_SC
  request_to_local_clock[*request] = pb_clocks->local_clock;
#endif
  clmpi_tick_clock();
  return err;
}

int MPI_Irsend(mpi_const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  int err;
  PBSET;
  err=PMPI_Irsend(buf,count,datatype,dest,tag,comm,request);
#ifdef  DBG_SC
  request_to_local_clock[*request] = pb_clocks->local_clock;
#endif
  clmpi_tick_clock();
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

int MPI_Sendrecv(mpi_const void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, 
		 void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag,
		 MPI_Comm comm, MPI_Status *status)
{
  int err;
  PBSET;
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
  PBSET;
  err=PMPI_Sendrecv_replace(buf,count,datatype,sendtag,dest,recvtag,dest,comm,status);
  if (err == MPI_SUCCESS) cmpi_sync_clock(status); 
  return err;
}


int MPI_Wait(MPI_Request *request, MPI_Status *status)
{
  //  fprintf(stderr, "%s is not supported yet\n", __func__);  exit(1);
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
  //  fprintf(stderr, "%s is not supported yet\n", __func__);  exit(1);
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
  //  fprintf(stderr, "%s is not supported yet\n", __func__);  exit(1);
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
	cmpi_update_local_sent_clock(tmp_req);
#endif
      }
    }
  }
  return err;
}



int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status)
{
  int err;

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
	if (registered_buff_clocks != NULL) {
	  registered_buff_clocks[0] = PNMPI_MODULE_CLMPI_SEND_REQ_CLOCK;
	}  else {
	  fprintf(stderr, "registered_buff_clocks is NULL\n" );
	  exit(1);
	}

#ifdef DBG_SC
	MPI_Request tmp_req = COMM_REQ_FROM_STATUS(status).inreq;
	cmpi_update_local_sent_clock(tmp_req);
#endif
      }

    }
  }

  registered_buff_clocks = NULL;
  registered_buff_length = -1;
  return err;
}

int MPI_Testany(int count, MPI_Request *array_of_requests, int *index, int *flag, MPI_Status *status)
{
  //  fprintf(stderr, "%s is not supported yet\n", __func__);  exit(1);
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
	if (registered_buff_clocks != NULL) {
	  registered_buff_clocks[0] = PNMPI_MODULE_CLMPI_SEND_REQ_CLOCK;
	}  else {
	  fprintf(stderr, "registered_buff_clocks is NULL\n" );
	  exit(1);
	}
      }
    }
  }

  registered_buff_clocks = NULL;
  registered_buff_length = -1;
  return err;  
}

int MPI_Testsome(int count, MPI_Request *array_of_requests, int *outcount, int *array_of_indices, MPI_Status *array_of_statuses)
{
  int err,i;
  clmpi_init_registered_clocks(array_of_requests, count);
  err=PMPI_Testsome(count,array_of_requests,outcount,array_of_indices,array_of_statuses);


  if (run_check==0) return err;
#if 0
  if (*outcount > 0) {
        fprintf(stderr, "ReMPI:%d: outcount: %d\n", my_rank, *outcount);
  }
#endif
  for (i=0; i<*outcount; i++) {
    if (COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).inreq!=MPI_REQUEST_NULL) {
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
	cmpi_update_local_sent_clock(tmp_req);
#endif
      }
    } else {
      fprintf(stderr, "request is NULL in testsome of clmpi.cpp\n");
      exit(1);
    }
  }

  registered_buff_clocks = NULL;
  registered_buff_length = -1;

  return err;  
}

int MPI_Testall(int count, MPI_Request *array_of_requests, int *flag, MPI_Status *array_of_statuses)
{
  //  fprintf(stderr, "%s is not supported yet\n", __func__);  exit(1);
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
	    }  else {
	      fprintf(stderr, "registered_buff_clocks is NULL\n" );
	      exit(1);
	    }
	  }
	}
      } 
    }
  }

  registered_buff_clocks = NULL;
  registered_buff_length = -1;

  return err;
}

int MPI_Get_count(mpi_const MPI_Status *arg_0, MPI_Datatype arg_1, int *arg_2)
{
  int _wrap_py_return_val = 0;
  {
    /*TODO: arg_2 include piggyback message size, so subtract the size
     for now, we subtract in REMPI level, not here*/
    _wrap_py_return_val = PMPI_Get_count(arg_0, arg_1, arg_2);
  }    return _wrap_py_return_val;
}


_EXTERN_C_ int MPI_Finalize()
{
  return PMPI_Finalize();
}


/** ========================================================
 *   MPI Collectives
 *  ======================================================== **/


/* ================== C Wrappers for MPI_Allreduce ================== */
_EXTERN_C_ int PMPI_Allreduce(mpi_const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5);
_EXTERN_C_ int MPI_Allreduce(mpi_const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5) {
  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = PMPI_Allreduce(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
  }    return _wrap_py_return_val;
}


/* ================== C Wrappers for MPI_Reduce ================== */
_EXTERN_C_ int PMPI_Reduce(mpi_const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, int arg_5, MPI_Comm arg_6);
_EXTERN_C_ int MPI_Reduce(mpi_const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, int arg_5, MPI_Comm arg_6) {
  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = PMPI_Reduce(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
  }    return _wrap_py_return_val;
}


/* ================== C Wrappers for MPI_Scan ================== */
_EXTERN_C_ int PMPI_Scan(mpi_const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5);
_EXTERN_C_ int MPI_Scan(mpi_const void *arg_0, void *arg_1, int arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5) {
  int _wrap_py_return_val = 0;
  {
    //    fprintf(stderr, "CLMPI: %s is not supported yet", __func__);
    exit(1);  
    _wrap_py_return_val = PMPI_Scan(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
  }    return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Allgather ================== */
_EXTERN_C_ int PMPI_Allgather(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, MPI_Comm arg_6);
_EXTERN_C_ int MPI_Allgather(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, MPI_Comm arg_6) {
  int _wrap_py_return_val = 0;
  {
    _wrap_py_return_val = PMPI_Allgather(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
  }    return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Gatherv ================== */
_EXTERN_C_ int PMPI_Gatherv(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, mpi_const int *arg_4, mpi_const int *arg_5, MPI_Datatype arg_6, int arg_7, MPI_Comm arg_8);
_EXTERN_C_ int MPI_Gatherv(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, mpi_const int *arg_4, mpi_const int *arg_5, MPI_Datatype arg_6, int arg_7, MPI_Comm arg_8) {
  int _wrap_py_return_val = 0;
  {
    //    fprintf(stderr, "CLMPI: %s is not supported yet", __func__); exit(1);  
    _wrap_py_return_val = PMPI_Gatherv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7, arg_8);
  }    return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Reduce_scatter ================== */
_EXTERN_C_ int PMPI_Reduce_scatter(mpi_const void *arg_0, void *arg_1, mpi_const int *arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5);
_EXTERN_C_ int MPI_Reduce_scatter(mpi_const void *arg_0, void *arg_1, mpi_const int *arg_2, MPI_Datatype arg_3, MPI_Op arg_4, MPI_Comm arg_5) {
  int _wrap_py_return_val = 0;
  {
    //    fprintf(stderr, "CLMPI: %s is not supported yet", __func__); exit(1);  
    _wrap_py_return_val = PMPI_Reduce_scatter(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5);
  }    return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Scatterv ================== */
_EXTERN_C_ int PMPI_Scatterv(mpi_const void *arg_0, mpi_const int *arg_1, mpi_const int *arg_2, MPI_Datatype arg_3, void *arg_4, int arg_5, MPI_Datatype arg_6, int arg_7, MPI_Comm arg_8);
_EXTERN_C_ int MPI_Scatterv(mpi_const void *arg_0, mpi_const int *arg_1, mpi_const int *arg_2, MPI_Datatype arg_3, void *arg_4, int arg_5, MPI_Datatype arg_6, int arg_7, MPI_Comm arg_8) {
  int _wrap_py_return_val = 0;
  {
    //    fprintf(stderr, "CLMPI: %s is not supported yet", __func__);  exit(1);  
    _wrap_py_return_val = PMPI_Scatterv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7, arg_8);
  }    return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Allgatherv ================== */
_EXTERN_C_ int PMPI_Allgatherv(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, mpi_const int *arg_4, mpi_const int *arg_5, MPI_Datatype arg_6, MPI_Comm arg_7);
_EXTERN_C_ int MPI_Allgatherv(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, mpi_const int *arg_4, mpi_const int *arg_5, MPI_Datatype arg_6, MPI_Comm arg_7) {
  int _wrap_py_return_val = 0;
  {
    // fprintf(stderr, "CLMPI: %s is not supported yet", __func__);  exit(1);
    _wrap_py_return_val = PMPI_Allgatherv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7);
  }    return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Scatter ================== */
_EXTERN_C_ int PMPI_Scatter(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, int arg_6, MPI_Comm arg_7);
_EXTERN_C_ int MPI_Scatter(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, int arg_6, MPI_Comm arg_7) {
  int _wrap_py_return_val = 0;
  {
    // fprintf(stderr, "CLMPI: %s is not supported yet", __func__);   exit(1);
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
_EXTERN_C_ int PMPI_Alltoall(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, MPI_Comm arg_6);
_EXTERN_C_ int MPI_Alltoall(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, MPI_Comm arg_6) {
  int _wrap_py_return_val = 0;
  {
    // fprintf(stderr, "CLMPI: %s is not supported yet", __func__);  exit(1);
    _wrap_py_return_val = PMPI_Alltoall(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6);
  }    return _wrap_py_return_val;
}

/* ================== C Wrappers for MPI_Gather ================== */
_EXTERN_C_ int PMPI_Gather(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, int arg_6, MPI_Comm arg_7);
_EXTERN_C_ int MPI_Gather(mpi_const void *arg_0, int arg_1, MPI_Datatype arg_2, void *arg_3, int arg_4, MPI_Datatype arg_5, int arg_6, MPI_Comm arg_7) {
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
_EXTERN_C_ int PMPI_Alltoallv(mpi_const void *arg_0, mpi_const int *arg_1, mpi_const int *arg_2, MPI_Datatype arg_3, void *arg_4, mpi_const int *arg_5, mpi_const int *arg_6, MPI_Datatype arg_7, MPI_Comm arg_8);
_EXTERN_C_ int MPI_Alltoallv(mpi_const void *arg_0, mpi_const int *arg_1, mpi_const int *arg_2, MPI_Datatype arg_3, void *arg_4, mpi_const int *arg_5, mpi_const int *arg_6, MPI_Datatype arg_7, MPI_Comm arg_8)
{
  int _wrap_py_return_val = 0;
  {
    // fprintf(stderr, "CLMPI: %s is not supported yet", __func__);     exit(1);
    _wrap_py_return_val = PMPI_Alltoallv(arg_0, arg_1, arg_2, arg_3, arg_4, arg_5, arg_6, arg_7, arg_8);
  }    return _wrap_py_return_val;
}



/*.......................................................*/
/* The End. */
