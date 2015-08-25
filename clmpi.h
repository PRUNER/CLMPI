#ifndef __CLMPI_H__
#define __CLMPI_H__

#define PNMPI_MODULE_CLMPI_PB_TYPE size_t

#define PNMPI_MODULE_CLMPI "clock-mpi"

#define PNMPI_MODULE_CLMPI_INITIAL_CLOCK  (10)
#define PNMPI_MODULE_CLMPI_UNMATCHED_RECV_REQ_CLOCK ( 2)
#define PNMPI_MODULE_CLMPI_SEND_REQ_CLOCK ( 1)

typedef int (*PNMPIMOD_register_recv_clocks_t)(size_t*, int);
//typedef int (*PNMPIMOD_set_local_clock_t)(size_t);
typedef int (*PNMPIMOD_clock_control_t)(size_t);
typedef int (*PNMPIMOD_sync_clock_t)(size_t);
typedef int (*PNMPIMOD_get_local_clock_t)(size_t*);
//typedef int (*PNMPIMOD_fetch_next_clocks_t)(int, int*, size_t*);
//typedef int (*PNMPIMOD_get_next_clock_t)(size_t*);
//typedef int (*PNMPIMOD_set_next_clock_t)(size_t);

#endif /* _PNMPI_MOD_STATUS */
