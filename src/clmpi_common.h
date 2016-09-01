#ifndef __CLMPI_COMMOM_H__
#define __CLMPI_COMMOM_H__

#if MPI_VERSION == 1 || MPI_VERSION == 2
#define mpi_const
#else
#define mpi_const const
#endif



#define CLMPI_DBG(format, ...) \
  do { \
  fprintf(stderr, "CLMPI:%d: " format " (%s:%d)\n", my_rank, ## __VA_ARGS__, __FILE__, __LINE__); \
  } while (0)

#endif
