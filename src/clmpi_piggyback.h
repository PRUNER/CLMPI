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
   Original code was written by Anh Vo<Anh.Vo@microsoft.com> and Martin Schulz<schulzm@llnl.gov> as pb_mod.h (c2f71e70b80).  
   Although the original code is not licensed, we leave change logs as a mark of respect for them.               
   Original code is in https://lc.llnl.gov/bitbucket/projects/PNMPI/repos/pnmpi-modules/                         
                                                                                                                 
   ## v1.0 (12/29/2016)                                                                                                       
   ### Changed                                                                                                   
   - Changee datatype of buffer of send functions (e.g. MPI_Send) to support both MPI-2 and MPI-3                
                                                                                                                 
   ### Added                                                                                                     
   - Added MPI_Init_thread function                                                                              
   - Added other MPI collectives for debugging                                                                   
   - Added several usuful functions                                                                              
                                                                                                                 
   ################################################################################  */

#include <mpi.h>
//#include "clmpi_status.h"

#ifndef _PNMPI_MOD_PIGGYBACK
#define _PNMPI_MOD_PIGGYBACK

#define PNMPI_MODULE_PB "piggyback"


/*..........................................................*/
/* Request additional memory in the status object
   This must be called from within MPI_Init (after
   calling PMPI_Init)

   IN:  size = number of bytes requested
               (if 0, just storage of standard parameters
   OUT: >=0: offset of new storage relative to request pointer
         <0: error message 
*/
extern int piggyback_offset;

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*PNMPIMOD_Piggyback_t)(int,char*);
int PNMPIMOD_Piggyback(int, char*);
typedef int (*PNMPIMOD_Piggyback_Size_t)(int);
int PNMPIMOD_Piggyback_Size(int);

#ifdef __cplusplus
}
#endif

#endif /* _PNMPI_MOD_STATUS */
