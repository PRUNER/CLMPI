#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "mpi.h"
#include "util.h"
#include "clmpi.h"


int main(int argc, char **argv)
{
  int size, rank;
  double s, e, time;
  int *send, *recv;
  int i, j;
  int repeat = 1;
  int left, right;
  MPI_Request send_req, recv_req;
  int length[2] = {1, 8 * 1024 * 1024};
  size_t local_clock = 0, received_clock = 0, dummy;

  s = get_dtime();
  MPI_Init(&argc, &argv);
  signal(SIGSEGV, SIG_DFL);
  e = get_dtime();

  MPI_Comm_size(MPI_COMM_WORLD, &size);
  left = 0; right = size - 1;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (rank == left) {
    fprintf(stderr, "-----\n");
    fprintf(stderr, "Init time: %f\n", e - s);
    fprintf(stderr, "-----\n");
    fprintf(stderr, "Pingpong message size(bytes)\tPingpong time(usec)\tPingpong throughput(MB/sec)\trepeat\n");
  }

  for (i = 0; i < 2; i++) {
    if (rank != left && rank != right) break;

    send = (int*)malloc(length[i]);
    tu_init_buf_int(send, length[i] / sizeof(int), rank);
    recv = (int*)malloc(length[i]);
    tu_init_buf_int(recv, length[i] / sizeof(int), rank);

    repeat = 500;
    s = get_dtime();
    for (j = 0; j < repeat; j++) {

      if (rank == left) {
	MPI_Isend(send, length[i] / sizeof(int), MPI_INT, right, 0, MPI_COMM_WORLD, &send_req);
	CLMPI_register_recv_clocks(&dummy, 1);
	MPI_Wait(&send_req, NULL);
	MPI_Irecv(recv, length[i] / sizeof(int), MPI_INT, right, 0, MPI_COMM_WORLD, &recv_req);
	CLMPI_register_recv_clocks(&received_clock, 1);
	MPI_Wait(&recv_req, NULL);
      } else {
	MPI_Irecv(recv, length[i] / sizeof(int), MPI_INT,  left, 0, MPI_COMM_WORLD, &recv_req);
	CLMPI_register_recv_clocks(&received_clock, 1);
	MPI_Wait(&recv_req, NULL);
	MPI_Isend(send, length[i] / sizeof(int), MPI_INT,  left, 0, MPI_COMM_WORLD, &send_req);
	CLMPI_register_recv_clocks(&dummy, 1);
	MPI_Wait(&send_req, NULL);
      }
    }
    e = get_dtime();

    if (rank == left) {
      tu_verify_int(recv, length[i] / sizeof(int),  right);      
      time = (e - s)/repeat;
      fprintf(stderr, "%d\t%f\t%f\t%d\n", 
	      length[i], time * 1000 * 1000, (length[i] * 2) / time / 1000 / 1000,  repeat);

    }
    
    free(send);
    free(recv);
  }

  if (rank == left) fprintf(stderr, "-----\n");
  MPI_Barrier(MPI_COMM_WORLD);
  CLMPI_get_local_clock(&local_clock);
  fprintf(stderr, "Rank %d: Last received clock: %d, Local clock: %d\n", rank, received_clock, local_clock);
  CLMPI_collective_sync_clock(MPI_COMM_WORLD);
  CLMPI_get_local_clock(&local_clock);
  sleep(1);
  if (rank == left) fprintf(stderr, "-----\n");
  sleep(1);
  fprintf(stderr, "Rank %d: Last received clock: %d, Local clock: %d\n", rank, received_clock, local_clock);

  MPI_Finalize();

  return 0;
}
