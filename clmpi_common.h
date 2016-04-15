#define CLMPI_DBG(format, ...) \
  do { \
  fprintf(stderr, "CLMPI:%d: " format " (%s:%d)\n", my_rank, ## __VA_ARGS__, __FILE__, __LINE__); \
  } while (0)
