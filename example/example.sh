#!/bin/sh

LD_PRELOAD=../src/.libs/libclmpi.so srun -n 2 ./clmpi_pingpong
#LD_PRELOAD=../src/.libs/libpbmpi.so srun -n 2 ./pbmpi_pingpong
