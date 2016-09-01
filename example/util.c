#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "util.h"

double get_dtime(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((double)(tv.tv_sec) + (double)(tv.tv_usec) * 0.001 * 0.001);
}

char* init_buf_char (int length, char c) 
{
  char *buf;
  int i;
  if ((buf = (char*)valloc(length)) == NULL) {
    print_abort("vallov failed (%s:%s:%d)", __FILE__, __func__, __LINE__);
  }
  for (i = 0; i < length; i++) {
    buf[i] = c;
  }
  return buf;
}

void tu_init_buf_int(int* vals, int count, int val)
{
  int i;
  for (i = 0; i < count; i++) {
    vals[i] = val;
  }
  return;
}

void tu_verify_int (int* vals, int count, int val)
{
  int i;
  //  for (i = count - 1; i >= 0; i--) {
  for (i = 0; i < count; i++) {
    if (vals[i] != val) {
      print_abort("The val(%d) is not verified index: %d by %d", vals[i], i, val);
      fprintf(stderr, "The val(%d) is not verified index: %d by %d\n", vals[i], i, val);
      return;
    }
  }
  return;
}


int* init_buf_int (int length, int val) 
{
  int *buf;
  int i;
  if ((buf = (int*)valloc(length)) == NULL) {
    print_abort("vallov failed (%s:%s:%d)", __FILE__, __func__, __LINE__);
  }

  if (length % sizeof(int) != 0) {
    print_abort("length must be multiples of sizeof(int) (%s:%s:%d)", __FILE__, __func__, __LINE__);
  }

  for (i = 0; i < length / sizeof(int); i++) {
    buf[i] = val;

  }
  fprintf(stderr, "malloc: %p\n", buf);
  return buf;
}

int print_abort(const char* fmt, ...)
{
  va_list argp;
  fprintf(stderr, "");
  va_start(argp, fmt);
  vfprintf(stderr, fmt, argp);
  va_end(argp);
  fprintf(stderr, "\n");
  exit(1);
}






