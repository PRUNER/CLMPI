#!/bin/sh

LD_PRELOAD=../src/.libs/libclmpi.so srun -n 4 ./clmpi_test
LD_PRELOAD=../src/.libs/libpbmpi.so srun -n 4 ./pbmpi_test
