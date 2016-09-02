#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "mpi.h"
#include "util.h"
#include "pbmpi.h"


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
  size_t send_pbdata = 0, recv_pbdata = 0;

  s = get_dtime();
  MPI_Init(&argc, &argv);
  signal(SIGSEGV, SIG_DFL);
  e = get_dtime();

  MPI_Comm_size(MPI_COMM_WORLD, &size);
  left = 0; right = size - 1;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  send_pbdata = rank + 1;

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
	pbmpi_set_send_pbdata(&send_pbdata);
	MPI_Isend(send, length[i] / sizeof(int), MPI_INT, right, 0, MPI_COMM_WORLD, &send_req);
	MPI_Wait(&send_req, NULL);
	MPI_Irecv(recv, length[i] / sizeof(int), MPI_INT, right, 0, MPI_COMM_WORLD, &recv_req);
	pbmpi_set_recv_pbdata(&recv_pbdata, 1);
	MPI_Wait(&recv_req, NULL);
      } else {
	MPI_Irecv(recv, length[i] / sizeof(int), MPI_INT,  left, 0, MPI_COMM_WORLD, &recv_req);
	pbmpi_set_recv_pbdata(&recv_pbdata, 1);
	MPI_Wait(&recv_req, NULL);
	pbmpi_set_send_pbdata(&send_pbdata);
	MPI_Isend(send, length[i] / sizeof(int), MPI_INT,  left, 0, MPI_COMM_WORLD, &send_req);
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

  if (rank == left) {
    fprintf(stderr, "-----\n");
  }
  MPI_Barrier(MPI_COMM_WORLD);
  if (rank == left || rank == right) { 
    fprintf(stderr, "Rank %d: send_pbdata: %lu, recv_pbdata: %lu\n", rank, send_pbdata, recv_pbdata);
  }
  MPI_Finalize();

  return 0;
}
