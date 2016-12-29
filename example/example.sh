#!/bin/sh
#######################CLMPI:LICENSE#############################################
# Copyright (c) 2016, Lawrence Livermore National Security, LLC.                     
# Produced at the Lawrence Livermore National Laboratory.                            
#                                                                                    
# Written by Kento Sato, kento@llnl.gov. LLNL-CODE-711397.                           
# All rights reserved.                                                               
#                                                                                    
# This file is part of CLMPI. For details, see https://github.com/PRUNER/CLMPI      
# Please also see the LICENSE.TXT file for our notice and the LGPL.                      
#                                                                                    
# This program is free software; you can redistribute it and/or modify it under the 
# terms of the GNU General Public License (as published by the Free Software         
# Foundation) version 2.1 dated February 1999.                                       
#                                                                                    
# This program is distributed in the hope that it will be useful, but WITHOUT ANY    
# WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or                  
# FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU          
# General Public License for more details.                                           
#                                                                                    
# You should have received a copy of the GNU Lesser General Public License along     
# with this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
# Place, Suite 330, Boston, MA 02111-1307 USA                                
#######################CLMPI:LICENSE#############################################

LD_PRELOAD=../src/.libs/libclmpi.so srun -n 4 ./clmpi_test
LD_PRELOAD=../src/.libs/libpbmpi.so srun -n 4 ./pbmpi_test
