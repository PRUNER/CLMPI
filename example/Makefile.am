SUBDIRS = ../src

noinst_PROGRAMS = \
	mpi_test   \
	clmpi_test \
	pbmpi_test



#mpi_pingpong_perf_SOURCES = mpi_pingpong_perf.c util.c
#mpi_pingpong_perf_CPPFLAGS = -I../src
#mpi_pingpong_perf_LDFLAGS  = -L../src/.libs/ -lclmpi

mpi_test_SOURCES  = mpi_test.c util.c

clmpi_test_SOURCES  = clmpi_test.c util.c
clmpi_test_CFLAGS   = -g	
clmpi_test_CPPFLAGS = -I../src
if WITH_BLUEGENE
clmpi_test_LDADD    = ../src/.libs/libclmpi.a
clmpi_test_LDFLAGS  = -lstdc++ -std=c++0x
else
clmpi_test_LDFLAGS  = -L../src/.libs/ -lstdc++ -std=c++0x -lclmpi 
endif

pbmpi_test_SOURCES = pbmpi_test.c util.c
pbmpi_test_CFLAGS   = -g	
pbmpi_test_CPPFLAGS = -I../src
if WITH_BLUEGENE
pbmpi_test_LDADD    = ../src/.libs/libpbmpi.a
pbmpi_test_LDFLAGS  = -lstdc++ -std=c++0x
else
pbmpi_test_LDFLAGS  = -L../src/.libs/ -lstdc++ -std=c++0x -lpbmpi 
endif



.PHONY: clean_core
clean_core:
	-rm -f *.core
	-rm -f *.btr


