#ifndef __CLMPI_H__
#define __CLMPI_H__

#include "mpi.h"

#define PNMPI_MODULE_CLMPI_PB_TYPE size_t

#define PNMPI_MODULE_CLMPI "clock-mpi"

#define PNMPI_MODULE_CLMPI_INITIAL_CLOCK            (10)
#define PNMPI_MODULE_CLMPI_COLLECTIVE                (3)
#define PNMPI_MODULE_CLMPI_UNMATCHED_RECV_REQ_CLOCK  (2)
#define PNMPI_MODULE_CLMPI_SEND_REQ_CLOCK            (1)
#define PNMPI_MODULE_CLMPI_TEST (22)

#define CLMPI_TICK_AT_UNMATCHED (1<<0)


#ifdef __cplusplus
extern "C" {
#endif
typedef int (*PNMPIMOD_register_recv_clocks_t)(size_t*, int);
int PNMPIMOD_register_recv_clocks(size_t*, int);
#define CLMPI_register_recv_clocks PNMPIMOD_register_recv_clocks

typedef int (*PNMPIMOD_clock_control_t)(size_t);
int PNMPIMOD_clock_control(size_t);
#define CLMPI_clock_control PNMPIMOD_clock_control

typedef int (*PNMPIMOD_sync_clock_t)(size_t);
int PNMPIMOD_sync_clock(size_t);
//#define CLMPI_sync_clock PNMPIMOD_sync_clock 

typedef int (*PNMPIMOD_get_local_clock_t)(size_t*);
int PNMPIMOD_get_local_clock(size_t*);
#define CLMPI_get_local_clock PNMPIMOD_get_local_clock

typedef int (*PNMPIMOD_get_local_sent_clock_t)(size_t*);
int PNMPIMOD_get_local_sent_clock(size_t*);
//#define CLMPI_get_local_sent_clock PNMPIMOD_get_local_clock

typedef int (*PNMPIMOD_collective_sync_clock_t)(MPI_Comm comm);
int PNMPIMOD_collective_sync_clock(MPI_Comm comm);
#define CLMPI_collective_sync_clock PNMPIMOD_collective_sync_clock

typedef int (*PNMPIMOD_get_num_of_incomplete_sending_msg_t)(size_t*);
int PNMPIMOD_get_num_of_incomplete_sending_msg(size_t*);
//#define CLMPI_get_num_of_incomplete_sending_msg PNMPIMOD_get_num_of_incomplete_sending_msg

void CLMPI_tick_clock();
void clmpi_tick_clock();
void CLMPI_configure(size_t flags);
void CLMPI_set_pb_clock(size_t *clock);

#ifdef __cplusplus
}
#endif

//typedef int (*PNMPIMOD_fetch_next_clocks_t)(int, int*, size_t*);
//typedef int (*PNMPIMOD_get_next_clock_t)(size_t*);
//typedef int (*PNMPIMOD_set_next_clock_t)(size_t);

#endif /* _PNMPI_MOD_STATUS */
