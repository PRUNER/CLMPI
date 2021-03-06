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
   Original code was written by Martin Schulz<schulzm@llnl.gov> as status.h (834325109b2a7d90715395592d67f68fa10c642b), LLNL-CODE-402774.          
   https://github.com/LLNL/PnMPI                                                                                     
                                                                                                                     
   ## v1.0 (12/29/2016) 
   ### Changed                                                                                                       
   - Changed datatype of buffer of send functions (e.g. MPI_Send) to support both MPI-2 and MPI-3                    
   - Fixed the code to support BGQ                                                                                   
                                                                                                                     
   ### Added                                                                                                         
   - Added MPI_Init_thread function                                                                                  
   ################################################################################  */

/*
Copyright (c) 2008
Lawrence Livermore National Security, LLC. 

Produced at the Lawrence Livermore National Laboratory. 
Written by Martin Schulz, schulzm@llnl.gov.
LLNL-CODE-402774,
All rights reserved.

This file is part of P^nMPI. 

Please also read the file "LICENSE" included in this package for 
Our Notice and GNU Lesser General Public License.

This program is free software; you can redistribute it and/or 
modify it under the terms of the GNU General Public License 
(as published by the Free Software Foundation) version 2.1 
dated February 1999.

This program is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY 
OF MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
terms and conditions of the GNU General Public License for more 
details.

You should have received a copy of the GNU Lesser General Public 
License along with this program; if not, write to the 

Free Software Foundation, Inc., 
59 Temple Place, Suite 330, 
Boston, MA 02111-1307 USA
*/

#include <mpi.h>

#ifndef _PNMPI_MOD_STATUS
#define _PNMPI_MOD_STATUS

/*------------------------------------------------------------*/
/* Note: this module must be loaded into the stack before
   any module using it (i.e., must be instantiated between
   any user and the application). Also, any call to its
   services must occur after calling PMPI_Init and services
   are only available to modules requesting it during
   MPI_Init */

/*------------------------------------------------------------*/
/* Additional storage in requests */

#define PNMPI_MODULE_STATUS "status-storage"


/*..........................................................*/
/* Access macros */

#define STATUS_STORAGE(status,offset,type) STATUS_STORAGE_ARRAY(status,offset,0,type,1,0)

#define STATUS_STORAGE_ARRAY(status,offset,total,type,count,num) \
(*( \
   (type*) ( \
	    ((char*) (status)) \
         	    + count * sizeof(MPI_Status) \
	            + num   * total \
        	    + offset \
	    ) \
)) \

#define PNMPIMOD_STATUS_TAG 0x3e1f9

extern int add_status_storage;


/*..........................................................*/
/* Request additional memory in the status object
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

typedef int (*PNMPIMOD_Status_RequestStorage_t)(int);
int PNMPIMOD_Status_RequestStorage(int size);

#ifdef __cplusplus
}
#endif


#endif /* _PNMPI_MOD_STATUS */
