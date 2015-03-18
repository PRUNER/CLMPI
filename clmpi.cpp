#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>

#include "mpi.h"
#include "pnmpimod.h"
#include "status.h"
#include "requests.h"
#include "pb_mod.h"
#include "clmpi.h"


int pb_int = 0;
int run_check=1;
int run_set=1;


size_t local_clock= PNMPI_MODULE_CLMPI_INITIAL_CLOCK;
size_t local_tick = 0;
char *pbdata;
int rank = -1;

#define PBSET \
  if (run_set) { \
  ((size_t*)pbdata)[0]=local_clock;		\
  pb_set(pb_size, pbdata);}


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
static int pb_size=sizeof(size_t);
static int *TotalStatusExtension;
static int *StatusOffsetInRequest;

#define COMM_REQ_FROM_STATUS(status) REQ_FROM_STATUS(status,(*StatusOffsetInRequest))
#define COMM_REQ_FROM_STATUSARRAY(status,count,num) REQ_FROM_STATUSARRAY(status,(*StatusOffsetInRequest),*TotalStatusExtension,count,num)


/*------------------------------------------------------------*/
/* External service routines */

size_t *registered_buff_clocks = NULL;
int     registered_buff_length = -1;
int     sync_clock = 1;

void clmpi_update_clock(size_t recv_clock) {
  if (sync_clock) {
    //    if (rank == 0) fprintf(stderr, "Recv: clock: %d", local_clock);
    if (local_clock <= recv_clock) {
      local_clock = recv_clock;	
    }
    local_clock++;
    //    if (rank == 0) fprintf(stderr, " to clock: %d\n", local_clock);
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
}

int PNMPIMOD_get_local_clock(size_t *clock)
{ 
  *clock = local_clock;
  return MPI_SUCCESS;
}

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
    //    fprintf(stderr, "ReMPI:%d: [%d |%d|] update index: %d \n", rank, sender_clock, status[i].MPI_SOURCE, matched_index);
  }
  //  fprintf(stderr, "%lu\t%d\t",sender_clock, status[i].MPI_SOURCE);
  //  cmpi_pring_addr();
  //  fprintf(stderr, "\t%lu\t|%d|\n", local_clock, rank);
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

  local_tick = 1; //ticks[rank];
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

  sprintf(service.name,"clmpi_register_recv_clocks");
  service.fct=(PNMPI_Service_Fct_t) PNMPIMOD_register_recv_clocks;
  sprintf(service.sig,"pi");
  err=PNMPI_Service_RegisterService(&service);
  if (err!=PNMPI_SUCCESS) {
    return MPI_ERROR_PNMPI;
  }

  sprintf(service.name,"clmpi_clock_control");
  service.fct=(PNMPI_Service_Fct_t) PNMPIMOD_clock_control;
  sprintf(service.sig,"i");
  err=PNMPI_Service_RegisterService(&service);
  if (err!=PNMPI_SUCCESS) {
    return MPI_ERROR_PNMPI;
  }

  sprintf(service.name,"clmpi_sync_clock");
  service.fct=(PNMPI_Service_Fct_t) PNMPIMOD_sync_clock;
  sprintf(service.sig,"i");
  err=PNMPI_Service_RegisterService(&service);
  if (err!=PNMPI_SUCCESS) {
    return MPI_ERROR_PNMPI;
  }

  sprintf(service.name,"clmpi_get_local_clock");
  service.fct=(PNMPI_Service_Fct_t) PNMPIMOD_get_local_clock;
  sprintf(service.sig,"p");
  err=PNMPI_Service_RegisterService(&service);
  if (err!=PNMPI_SUCCESS) {
    return MPI_ERROR_PNMPI;
  }

  return err;
}

/*.......................................................*/
/* Init PNMPI*/

int cmpi_init_pnmpi() {
  int err;
  PNMPI_modHandle_t handle_pb,handle_pbd,handle_status,handle_req;
  PNMPI_Service_descriptor_t serv;
  PNMPI_Global_descriptor_t global;
  const char *vlevel_s;

  /* query pb module */


  err=PNMPI_Service_GetModuleByName(PNMPI_MODULE_PB,&handle_pb);
  if (err!=PNMPI_SUCCESS)
    return err;

  err=PNMPI_Service_GetServiceByName(handle_pb,"piggyback","ip",&serv);
  if (err!=PNMPI_SUCCESS)
    return err;
  pb_set=(PNMPIMOD_Piggyback_t) ((void*)serv.fct);

  err=PNMPI_Service_GetServiceByName(handle_pb,"piggyback-size","i",&serv);
  if (err!=PNMPI_SUCCESS)
    return err;
  pb_setsize=(PNMPIMOD_Piggyback_Size_t) ((void*)serv.fct);

  err=PNMPI_Service_GetGlobalByName(handle_pb,"piggyback-offset",'i',&global);
  if (err!=PNMPI_SUCCESS)
    return err;
  pb_offset=(global.addr.i);


  /* query the status module */

  err=PNMPI_Service_GetModuleByName(PNMPI_MODULE_STATUS,&handle_status);
  if (err!=PNMPI_SUCCESS)
    return err;

  err=PNMPI_Service_GetGlobalByName(handle_status,"total-status-extension",'i',&global);
  if (err!=PNMPI_SUCCESS)
    return err;
  TotalStatusExtension=(global.addr.i);


  /* query the request module */

  err=PNMPI_Service_GetModuleByName(PNMPI_MODULE_REQUEST,&handle_req);
  if (err!=PNMPI_SUCCESS)
    return err;

  err=PNMPI_Service_GetGlobalByName(handle_req,"status-offset",'i',&global);
  if (err!=PNMPI_SUCCESS)
    return err;
  StatusOffsetInRequest=(global.addr.i);


  /* query own module */

  err=PNMPI_Service_GetModuleByName(PNMPI_MODULE_CLMPI, &handle_pbd);
  if (err!=PNMPI_SUCCESS)
    return err;

  pb_size= sizeof(size_t);
  run_set=1;
  run_check=1;

  err=pb_setsize(pb_size);
  if (err!=PNMPI_SUCCESS)
    return err;

  pbdata=(char *)malloc(pb_size);
  if (pbdata==NULL) return MPI_ERROR_MEM;

}


/*.......................................................*/
/* Init */


int MPI_Init(int *argc, char ***argv)
{
  int err;
  int provided;

  cmpi_init_pnmpi();

  err = PMPI_Init(argc, argv);

  PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
  cmpi_set_tick();
  return err;
}

int MPI_Init_thread(int *argc, char ***argv, int required, int *provided)
{
  int err;

  cmpi_init_pnmpi();

  err = PMPI_Init_thread(argc,argv, required, provided);

  PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
  cmpi_set_tick();
  return err;
}


/*.......................................................*/
/* setting PB if enabled */


int MPI_Send(void* buf, int num, MPI_Datatype dtype, int node, 
	     int tag, MPI_Comm comm)
{
  int res;
  PBSET;
  res=PMPI_Send(buf,num,dtype,node,tag,comm);
  //  if (rank == 0) fprintf(stderr, "Send: clock: %d", local_clock);
  local_clock++;
  //  if (rank == 0) fprintf(stderr, " to clock: %d\n", local_clock);
  return res;
}

int MPI_Bsend(void* buf, int num, MPI_Datatype dtype, int node, 
	     int tag, MPI_Comm comm)
{
  int res;
  PBSET;
  res=PMPI_Bsend(buf,num,dtype,node,tag,comm);
  local_clock++;
  return res;
}

int MPI_Ssend(void* buf, int num, MPI_Datatype dtype, int node, 
	     int tag, MPI_Comm comm)
{
  int res;
  PBSET;
  res=PMPI_Ssend(buf,num,dtype,node,tag,comm);
  local_clock++;
  return res;
}

int MPI_Rsend(void* buf, int num, MPI_Datatype dtype, int node, 
	     int tag, MPI_Comm comm)
{
  int res;
  PBSET;
  res=PMPI_Rsend(buf,num,dtype,node,tag,comm);
  local_clock++;
  return res;
}

int MPI_Isend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  int err;
  PBSET;
  //  if (dest == 0) fprintf(stderr, "   %d %d\n", local_clock, rank);
  err=PMPI_Isend(buf,count,datatype,dest,tag,comm,request);
  local_clock++;
  return err;
}

int MPI_Ibsend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  int err;
  PBSET;
  err=PMPI_Ibsend(buf,count,datatype,dest,tag,comm,request);
  local_clock++;
  return err;
}

int MPI_Issend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  int err;
  PBSET;
  err=PMPI_Issend(buf,count,datatype,dest,tag,comm,request);
  return err;
}

int MPI_Irsend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  int err;
  PBSET;
  err=PMPI_Irsend(buf,count,datatype,dest,tag,comm,request);
  local_clock++;
  return err;
}



/*.......................................................*/
/* Checks if enabled */



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
  int err;
  err=PMPI_Wait(request,status);
  if (run_check==0) return err;
  if (COMM_REQ_FROM_STATUS(status).inreq!=MPI_REQUEST_NULL) {
      if (COMM_REQ_FROM_STATUS(status).type==PNMPIMOD_REQUESTS_RECV) {
	  if (err == MPI_SUCCESS) cmpi_sync_clock(status); 
      }
  }
  return err;
}


int MPI_Waitany(int count, MPI_Request *array_of_requests, int *index, MPI_Status *status)
{
  int err;
  err=PMPI_Waitany(count,array_of_requests,index,status);
  if (run_check==0) return err;
  if (COMM_REQ_FROM_STATUS(status).inreq!=MPI_REQUEST_NULL) {
      if (COMM_REQ_FROM_STATUS(status).type==PNMPIMOD_REQUESTS_RECV) {
	  if (err == MPI_SUCCESS) cmpi_sync_clock(status); 
      }
  }
  return err;
}

int MPI_Waitsome(int count, MPI_Request *array_of_requests, int *outcount, int *array_of_indices, MPI_Status *array_of_statuses)
{
  int err,i;
  err=PMPI_Waitsome(count,array_of_requests,outcount,array_of_indices,array_of_statuses);
  if (run_check==0) return err;
  for (i=0; i<*outcount; i++) {
    if (COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).inreq!=MPI_REQUEST_NULL) {
      if (COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).type==PNMPIMOD_REQUESTS_RECV)  {
	if (err == MPI_SUCCESS) cmpi_sync_clock_at(array_of_statuses, count, i, array_of_indices[i]); 
      }
    }
  }
  return err;  
}


int MPI_Waitall(int count, MPI_Request *array_of_requests, MPI_Status *array_of_statuses)
{
  int err,i;
  err=PMPI_Waitall(count,array_of_requests,array_of_statuses);
  if (run_check==0) return err;
  for (i=0; i<count; i++) {
    if (COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).inreq!=MPI_REQUEST_NULL)  {
      if (COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).type==PNMPIMOD_REQUESTS_RECV)   {
	if (err == MPI_SUCCESS) cmpi_sync_clock_at(array_of_statuses, count, i, i); 
      }
    }
  }
  return err;
}


int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status)
{
  int err;
  err=PMPI_Test(request, flag, status);
  if (run_check==0) return err;
  if ((*flag) && (COMM_REQ_FROM_STATUS(status).inreq!=MPI_REQUEST_NULL)) {
    if (COMM_REQ_FROM_STATUS(status).type==PNMPIMOD_REQUESTS_RECV) {
      if (err == MPI_SUCCESS) cmpi_sync_clock(status); 
    } else {
      if (err == MPI_SUCCESS) {
	if (registered_buff_clocks != NULL) {
	  registered_buff_clocks[0] = PNMPI_MODULE_CLMPI_SEND_REQ_CLOCK;
	  //	  fprintf(stderr, "[1 |%d|]\n", 1, status->MPI_SOURCE);
	}
      }
    }
  }
  registered_buff_clocks = NULL;
  registered_buff_length = -1;
  return err;
}

int MPI_Testany(int count, MPI_Request *array_of_requests, int *index, int *flag, MPI_Status *status)
{
  int err;
  err=PMPI_Testany(count,array_of_requests,index,flag,status);
  if (run_check==0) return err;
  if ((*flag) && (COMM_REQ_FROM_STATUS(status).inreq!=MPI_REQUEST_NULL)) {
    if (COMM_REQ_FROM_STATUS(status).type==PNMPIMOD_REQUESTS_RECV) {
      if (err == MPI_SUCCESS) cmpi_sync_clock(status); 
    } else {
      if (err == MPI_SUCCESS) {
	int matched_index;
	if (registered_buff_clocks != NULL) {
	  registered_buff_clocks[0] = PNMPI_MODULE_CLMPI_SEND_REQ_CLOCK;
	  //	  fprintf(stderr, "ReMPI:%d: [%d |%d|] (outcount: %d)\n", rank, 1, array_of_statuses[matched_index].MPI_SOURCE, *outcount);
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


  err=PMPI_Testsome(count,array_of_requests,outcount,array_of_indices,array_of_statuses);

  if (run_check==0) return err;
#if 0
  if (*outcount > 0) {
        fprintf(stderr, "ReMPI:%d: outcount: %d\n", rank, *outcount);
  }
#endif
  for (i=0; i<*outcount; i++) {
    if (COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).inreq!=MPI_REQUEST_NULL) {
      if (COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).type==PNMPIMOD_REQUESTS_RECV) {
	if (err == MPI_SUCCESS) cmpi_sync_clock_at(array_of_statuses, count, i, array_of_indices[i]); 
      } else {
	if (err == MPI_SUCCESS) {
	  int matched_index;
	  matched_index = array_of_indices[i];
	  if (registered_buff_clocks != NULL) {
	    registered_buff_clocks[matched_index] = PNMPI_MODULE_CLMPI_SEND_REQ_CLOCK;
	    //	  fprintf(stderr, "ReMPI:%d: [%d |%d|] (outcount: %d)\n", rank, 1, array_of_statuses[matched_index].MPI_SOURCE, *outcount);
	  }
	}
      }
    } else {
      fprintf(stderr, "NOoooo in clmpi.cpp\n");
      exit(1);
    }
  }

  registered_buff_clocks = NULL;
  registered_buff_length = -1;
  //  local_clock += local_tick;
  return err;  
}

int MPI_Testall(int count, MPI_Request *array_of_requests, int *flag, MPI_Status *array_of_statuses)
{
  int err,i;
  err=PMPI_Testall(count, array_of_requests, flag, array_of_statuses);
  if (run_check==0) return err;
  if (*flag)  {
    for (i=0; i<count; i++) {
      if (COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).inreq!=MPI_REQUEST_NULL)  {
	if (COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).type==PNMPIMOD_REQUESTS_RECV) {
	  if (err == MPI_SUCCESS) cmpi_sync_clock_at(array_of_statuses, count, i, i); 
	} else {
	  if (err == MPI_SUCCESS) {
	    if (registered_buff_clocks != NULL) {
	      registered_buff_clocks[i] = PNMPI_MODULE_CLMPI_SEND_REQ_CLOCK;
	      //	  fprintf(stderr, "ReMPI:%d: [%d |%d|] (outcount: %d)\n", rank, 1, array_of_statuses[matched_index].MPI_SOURCE, *outcount);
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


int MPI_Finalize()
{
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  //  fprintf(stderr, "clock: %d %d\n", local_clock, rank);
  return PMPI_Finalize();

}




/*.......................................................*/
/* The End. */
