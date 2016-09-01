noinst_PROGRAMS = \
	clmpi_pingpong \
	pbmpi_pingpong


clmpi_pingpong_SOURCES = clmpi_pingpong.c util.c
clmpi_pingpong_CPPFLAGS = -I../src
clmpi_pingpong_LDFLAGS  = -L../src/.libs/ 
clmpi_pingpong_LIBS  = -lclmpi

pbmpi_pingpong_SOURCES = clmpi_pingpong.c util.c
pbmpi_pingpong_CPPFLAGS = -I../src
pbmpi_pingpong_LDFLAGS  = -L../src/.libs/
pbmpi_pingpong_LIBS  = -lpbmpi




.PHONY: clean_core
clean_core:
	-rm -f *.core
	-rm -f *.btr


