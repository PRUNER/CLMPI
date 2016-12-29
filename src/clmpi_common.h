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

#ifndef __CLMPI_COMMOM_H__
#define __CLMPI_COMMOM_H__

#if MPI_VERSION == 1 || MPI_VERSION == 2
#define mpi_const
#else
#define mpi_const const
#endif



#define CLMPI_DBG(format, ...) \
  do { \
      fprintf(stderr, "CLMPI:DEBUG:%d: " format " (%s:%d)\n", my_rank, ## __VA_ARGS__, __FILE__, __LINE__); \
  } while (0)


#define CLMPI_ERR(format, ...) \
  do { \
      fprintf(stderr, "CLMPI:ERROR:%d: " format " (%s:%d)\n", my_rank, ## __VA_ARGS__, __FILE__, __LINE__); \
      exit(1); \
  } while (0)

#endif
