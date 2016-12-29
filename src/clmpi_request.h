/* ==========================CLMPI:LICENSE==========================================   
  Copyright (c) 2016, Lawrence Livermore National Security, LLC.                     
  Produced at the Lawrence Livermore National Laboratory.                            
                                                                                    
  Written by Kento Sato, kento@llnl.gov. LLNL-CODE-711397.                           
  All rights reserved.                                                               
                                                                                    
  This file is part of CLMPI. For details, see https://github.com/PRUNER/CLMPI      
  Please also see the LICENSE.TXT file for our notice and the LGPL.                      
                                                                                    
  This program is free software; you can redistribute it and/or modify it under the 
  terms of the GNU General Public License (as published by the Free Software         
  Foundation) version 2.1 dated February 1999.                                       
                                                                                    
  This program is distributed in the hope that it will be useful, but WITHOUT ANY    
  WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or                  
  FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU          
  General Public License for more details.                                           
                                                                                    
  You should have received a copy of the GNU Lesser General Public License along     
  with this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
  Place, Suite 330, Boston, MA 02111-1307 USA                                 
  ============================CLMPI:LICENSE========================================= */
/* ################################################################################                                  
   Change Log                                                                                                        
   ################################################################################                                  
   Original code was written by Martin Schulz<schulzm@llnl.gov> as requests.h (834325109b2a7d90715395592d67f68fa10c642b), LLNL-CODE-402774.          
   https://github.com/LLNL/PnMPI                                                                                     
                                                                                                                     
   ## v1.0 (12/29/2016)                                                                                                                   
   ### Changed                                                                                                       
   - Changed datatype of buffer of send functions (e.g. MPI_Send) to support both MPI-2 and MPI-3                    
   - Fixed the code to support BGQ                                                                                   
                                                                                                                     
   ### Added                                                                                                         
   - Added MPI_Init_thread function                                                                                  
   ################################################################################  */

#include <mpi.h>
#include "clmpi_status.h"

#ifndef _PNMPI_MOD_REQUESTS
#define _PNMPI_MOD_REQUESTS


/*------------------------------------------------------------*/
/* Note: this module must be loaded into the stack after
   any module using it (i.e., must be instantiated between
   any user and the MPI library). Also, any call to its
   services must occur before calling PMPI_Init and services
   are only available to modules requesting it during
   MPI_Init */

/*------------------------------------------------------------*/
/* Additional storage in requests */

#define PNMPI_MODULE_REQUEST "request-storage"


/*..........................................................*/
/* Type constants */

#define PNMPIMOD_REQUESTS_SEND  0x01
#define PNMPIMOD_REQUESTS_BSEND 0x03
#define PNMPIMOD_REQUESTS_RSEND 0x05
#define PNMPIMOD_REQUESTS_SSEND 0x07
#define PNMPIMOD_REQUESTS_RECV  0x00
#define PNMPIMOD_REQUESTS_NULL  0xFF


/*..........................................................*/
/* Transparent type to hold additional parameters */

typedef struct PNMPIMOD_Requests_Parameters_d
{
  int type;
  void *buf;
  int count;
  MPI_Datatype datatype;
  int node;
  int tag;
  int persistent;
  int active;
  int cancelled;
  MPI_Comm comm;
  MPI_Request inreq;
  char *data;
} PNMPIMOD_Requests_Parameters_t;


typedef long MyReq_t;


/*..........................................................*/
/* Access macros */

#define REQ_STORAGE(request,fct,offset,type,val) \
{\
  PNMPIMOD_Requests_Parameters_t* _store;\
  _store=fct(request);\
  (*((type*) (&(_store->data[offset]))))=val;\
}

#define REQ_ENVELOPE(request,fct,_env) \
{\
  _env=fct(request);\
}

#define REQ_FROM_STATUS(status,offset) REQ_FROM_STATUSARRAY(status,offset,0,1,0)
#define INFO_FROM_STATUS(status,offset,type) INFO_FROM_STATUSARRAY(status,offset,0,type,1,0)
#define REQ_FROM_STATUSARRAY(status,offset,totext,count,num) STATUS_STORAGE_ARRAY(status,offset,totext,PNMPIMOD_Requests_Parameters_t,count,num)
#define INFO_FROM_STATUSARRAY(status,offset,totext,type,count,num) STATUS_STORAGE_ARRAY(status,offset+sizeof(PNMPIMOD_Requests_Parameters_t),totext,type,count,num)

extern int PNMPIMOD_Request_offsetInStatus;

/*..........................................................*/
/* Request additional memory in the request object
   This must be called from within MPI_Init (after
   calling PMPI_Init)

   IN:  size = number of bytes requested
               (if 0, just storage of standard parameters
   OUT: >=0: offset of new storage relative to request pointer
         <0: error message 
*/

#ifdef __cplusplus
extern "C" {
#endif
typedef int (*PNMPIMOD_Requests_RequestStorage_t)(int);
int PNMPIMOD_Requests_RequestStorage(int size);

typedef PNMPIMOD_Requests_Parameters_t* (*PNMPIMOD_Requests_MapRequest_t)(MPI_Request);
PNMPIMOD_Requests_Parameters_t* PNMPIMOD_Requests_MapRequest(MPI_Request req);
#ifdef __cplusplus
}
#endif



#endif /* _PNMPI_MOD_REQUESTS */
