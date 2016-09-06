SUBDIRS = ../src

noinst_PROGRAMS = \
	clmpi_test \
	pbmpi_test


#mpi_pingpong_perf_SOURCES = mpi_pingpong_perf.c util.c
#mpi_pingpong_perf_CPPFLAGS = -I../src
#mpi_pingpong_perf_LDFLAGS  = -L../src/.libs/ -lclmpi

clmpi_test_SOURCES = clmpi_test.c util.c
clmpi_test_CPPFLAGS = -I../src
clmpi_test_LDFLAGS  = -L../src/.libs/ -lclmpi -lstdc++ -std=c++0x

pbmpi_test_SOURCES = pbmpi_test.c util.c
pbmpi_test_CPPFLAGS = -I../src
pbmpi_test_LDFLAGS  = -L../src/.libs/ -lpbmpi -lstdc++ -std=c++0x


.PHONY: clean_core
clean_core:
	-rm -f *.core
	-rm -f *.btr


