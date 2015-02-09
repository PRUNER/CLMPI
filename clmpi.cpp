#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>

#include "mpi.h"
#include "pnmpimod.h"
#include "status.h"
#include "requests.h"
#include "pb_mod.h"

#define PNMPI_MODULE_PBD "mpi_clock"

#define PB_PATTERN 0x42434445


int pb_int = 0;
int run_check=1;
int run_set=1;

size_t local_clock=0;
size_t local_tick =0;
char *pbdata;
int rank = -1;

#define PBSET \
  if (run_set) { \
  ((size_t*)pbdata)[0]=local_clock;		\
  pb_set(pb_size, pbdata);}


#define PBCHECKARRAY(res,status,count,num) { \
  if (res==MPI_SUCCESS) \
    { \
      if ((STATUS_STORAGE_ARRAY(status,*pb_offset,*TotalStatusExtension,int,count,num)!=PB_PATTERN) || (0) )\ 
	printf("Received wrong pattern %08x\n",\
	       STATUS_STORAGE_ARRAY(status,*pb_offset,*TotalStatusExtension,int,count,num));\
    } }


#define PBCHECK(res,status) PBCHECKARRAY(res,status,1,0)



PNMPIMOD_Piggyback_t       pb_set;
PNMPIMOD_Piggyback_Size_t  pb_setsize;
static int *pb_offset;
static int pb_size=4;
static int *TotalStatusExtension;
static int *StatusOffsetInRequest;

#define COMM_REQ_FROM_STATUS(status) REQ_FROM_STATUS(status,(*StatusOffsetInRequest))
#define COMM_REQ_FROM_STATUSARRAY(status,count,num) REQ_FROM_STATUSARRAY(status,(*StatusOffsetInRequest),*TotalStatusExtension,count,num)


/*------------------------------------------------------------*/
/* External service routines */

size_t *cmpi_local_clock;
size_t *cmpi_received_local_clock;
size_t *cmpi_global_clock;

int PNMPIMOD_register_local_clock(size_t *local_clock)
{ 
  cmpi_local_clock = local_clock;
  return MPI_SUCCESS;
}

int PNMPIMOD_register_latest_received_local_clock(size_t *received_local_clock)
{ 
  cmpi_received_local_clock = received_local_clock;
  return MPI_SUCCESS;
}

int PNMPIMOD_register_global_clock(size_t *global_clock)
{ 
  cmpi_global_clock = global_clock;
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

void cmpi_sync_clock_at(MPI_Status *status, int count, int matched_index)
{
  size_t sender_clock;		
  sender_clock = STATUS_STORAGE_ARRAY(status,*pb_offset, *TotalStatusExtension, size_t, count, matched_index);
  fprintf(stderr, "%lu\t%d\t",sender_clock, status[matched_index].MPI_SOURCE);
  cmpi_pring_addr();
  fprintf(stderr, "\t%lu\t|%d|\n", local_clock, rank);
  if (local_clock < sender_clock) {
    //    local_clock = sender_clock;	
  }
  return; 
}

void cmpi_sync_clock(MPI_Status *status)
{
 cmpi_sync_clock_at(status, 1, 0);
  return;
}

void cmpi_set_tick()
{
#if 1
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

  err=PNMPI_Service_RegisterModule(PNMPI_MODULE_PBD);
  if (err!=PNMPI_SUCCESS)
    return MPI_ERROR_PNMPI;

  sprintf(service.name,"cmpi_register_local_clock");
  service.fct=(PNMPI_Service_Fct_t) PNMPIMOD_register_local_clock;
  sprintf(service.sig,"p");
  err=PNMPI_Service_RegisterService(&service);
  if (err!=PNMPI_SUCCESS)
    return MPI_ERROR_PNMPI;

  sprintf(service.name,"cmpi_register_received_clock");
  service.fct=(PNMPI_Service_Fct_t) PNMPIMOD_register_latest_received_local_clock;
  sprintf(service.sig,"p");
  err=PNMPI_Service_RegisterService(&service);
  if (err!=PNMPI_SUCCESS)
    return MPI_ERROR_PNMPI;

  sprintf(service.name,"cmpi_register_global_clock");
  service.fct=(PNMPI_Service_Fct_t) PNMPIMOD_register_global_clock;
  sprintf(service.sig,"p");
  err=PNMPI_Service_RegisterService(&service);
  if (err!=PNMPI_SUCCESS)
    return MPI_ERROR_PNMPI;

  return err;
}




/*.......................................................*/
/* Init */


int MPI_Init(int *argc, char ***argv)
{
  fprintf(stderr, "Sorry not supported in clock mpi\n");
  exit(0);
}

int MPI_Init_thread(int *argc, char ***argv, int required, int *provided)
{
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

  err=PNMPI_Service_GetModuleByName(PNMPI_MODULE_PBD,&handle_pbd);
  if (err!=PNMPI_SUCCESS)
    return err;

  err=PNMPI_Service_GetArgument(handle_pbd,"size",&vlevel_s);
  if (err!=PNMPI_SUCCESS)
    {
      if (err==PNMPI_NOARG)
	pb_size= sizeof(size_t);
      else
	return err;
    }
  else
    {
      pb_size=atoi(vlevel_s);
    }

  err=PNMPI_Service_GetArgument(handle_pbd,"set",&vlevel_s);
  if (err!=PNMPI_SUCCESS)
    {
      if (err==PNMPI_NOARG)
	run_set=1;
      else
	return err;
    }
  else
    {
      run_set=atoi(vlevel_s);
    }

  err=PNMPI_Service_GetArgument(handle_pbd,"check",&vlevel_s);
  if (err!=PNMPI_SUCCESS)
    {
      if (err==PNMPI_NOARG)
	run_check=1;
      else
	return err;
    }
  else
    {
      run_check=atoi(vlevel_s);
    }

  err=pb_setsize(pb_size);
  if (err!=PNMPI_SUCCESS)
    return err;

  pbdata=(char *)malloc(pb_size);
  if (pbdata==NULL) return MPI_ERROR_MEM;

  /* printf("Piggyback driver enabled with size %i (set %i, check %i)\n", */
  /* 	 pb_size,run_set,run_check); */

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
  return res;
}

int MPI_Bsend(void* buf, int num, MPI_Datatype dtype, int node, 
	     int tag, MPI_Comm comm)
{
  int res;
  PBSET;
  res=PMPI_Bsend(buf,num,dtype,node,tag,comm);
  return res;
}

int MPI_Ssend(void* buf, int num, MPI_Datatype dtype, int node, 
	     int tag, MPI_Comm comm)
{
  int res;
  PBSET;
  res=PMPI_Ssend(buf,num,dtype,node,tag,comm);
  return res;
}

int MPI_Rsend(void* buf, int num, MPI_Datatype dtype, int node, 
	     int tag, MPI_Comm comm)
{
  int res;
  PBSET;
  res=PMPI_Rsend(buf,num,dtype,node,tag,comm);
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
	if (err == MPI_SUCCESS) cmpi_sync_clock_at(array_of_statuses, count, i); 
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
	if (err == MPI_SUCCESS) cmpi_sync_clock_at(array_of_statuses, count, i); 
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
    }
  }
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
    }
  }
  return err;  
}

int MPI_Testsome(int count, MPI_Request *array_of_requests, int *outcount, int *array_of_indices, MPI_Status *array_of_statuses)
{
  int err,i;
  err=PMPI_Testsome(count,array_of_requests,outcount,array_of_indices,array_of_statuses);
  if (run_check==0) return err;
  for (i=0; i<*outcount; i++) {
    if (COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).inreq!=MPI_REQUEST_NULL) {
      if (COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count,i).type==PNMPIMOD_REQUESTS_RECV) {
	if (err == MPI_SUCCESS) cmpi_sync_clock_at(array_of_statuses, count, i); 
      }
    }
  }
  if (COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count, 0).inreq!=MPI_REQUEST_NULL) {
    if (COMM_REQ_FROM_STATUSARRAY(array_of_statuses,count, 0).type==PNMPIMOD_REQUESTS_RECV) {
      if (*outcount > 0) {
	fprintf(stderr, "--- count: %d -------- |%d|\n", *outcount, rank);
      } 
    }
  }
  
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
	  if (err == MPI_SUCCESS) cmpi_sync_clock_at(array_of_statuses, count, i); 
	}
      }
    }
  }

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
