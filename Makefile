include ./Makefile.config

SRC_DIR=.

#===== edit "clmpi" to your binary name ===========
clmpi_SRCS =	$(SRC_DIR)/clmpi_status.c \
		$(SRC_DIR)/clmpi.cpp \
		$(SRC_DIR)/clmpi_piggyback.c \
		$(SRC_DIR)/clmpi_request.cpp \

clmpi_OBJS =	$(SRC_DIR)/clmpi_status.o \
		$(SRC_DIR)/clmpi.o \
		$(SRC_DIR)/clmpi_piggyback.o \
		$(SRC_DIR)/clmpi_request.o \

clmpi_DEPS =	$(SRC_DIR)/clmpi_status.d \
		$(SRC_DIR)/clmpi.d \
		$(SRC_DIR)/clmpi_piggyback.d \
		$(SRC_DIR)/clmpi_request.d \

clmpi_HEAD = ./clmpi.h
clmpi_o_LIBS  =	./libclmpi.o
clmpi_a_LIBS  =	./libclmpi.a 
clmpi_so_LIBS =	./libclmpi.so	
#===================================================


#===== edit "clmpi" to your binary name ===========
pbmpi_SRCS =	$(SRC_DIR)/clmpi_status.c \
		$(SRC_DIR)/pbmpi.cpp \
		$(SRC_DIR)/clmpi_piggyback.c \
		$(SRC_DIR)/clmpi_request.cpp \

#pbmpi_OBJS =	$(pbmpi_SRCS:%.c=%.o)
pbmpi_OBJS =	$(SRC_DIR)/clmpi_status.o \
		$(SRC_DIR)/pbmpi.o \
		$(SRC_DIR)/clmpi_piggyback.o \
		$(SRC_DIR)/clmpi_request.o \

pbmpi_DEPS =  $(pbmpi_OBJS:%.o=%.d)


pbmpi_HEAD = ./pbmpi.h
pbmpi_o_LIBS  = ./libpbmpi.o
pbmpi_a_LIBS  =	./libpbmpi.a 
pbmpi_so_LIBS =	./libpbmpi.so	
#===================================================

OBJS = $(clmpi_OBJS) $(pbmpi_OBJS)
DEPS = $(clmpi_DEPS) $(pbmpi_DEPS)
LIBS = $(clmpi_a_LIBS) $(clmpi_so_LIBS) $(pbmpi_a_LIBS) $(pbmpi_so_LIBS) 

#$@: target name
#$<: first dependent file
#$^: all indemendent files

all: $(LIBS)

-include $(DEPS)


$(clmpi_a_LIBS):  $(clmpi_OBJS)
	stack_pmpi $(clmpi_o_LIBS) 0 $^
	ar cr  $@ $(clmpi_o_LIBS)
	ranlib $@

$(clmpi_so_LIBS):  $(clmpi_OBJS) $(clmpi_a_LIBS)
	$(CC) -shared -o $@ $(clmpi_o_LIBS)


$(pbmpi_a_LIBS):  $(pbmpi_OBJS)
	stack_pmpi $(pbmpi_o_LIBS) 0 $^
	ar cr  $@ $(pbmpi_o_LIBS)
	ranlib $@

$(pbmpi_so_LIBS):  $(pbmpi_OBJS) $(pbmpi_a_LIBS)
	$(CC) -shared -o $@ $(pbmpi_o_LIBS)




.SUFFIXES: .c .o
.c.o: 
	$(CC) $(CFLAGS) $(LDFLAGS) -c -MMD -MP $< 
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ -c $< 

.SUFFIXES: .cpp .o
.cpp.o:
	$(CC) $(CXXFLAGS) $(LDFLAGS) -c -MMD -MP $<
	$(CC) $(CXXFLAGS) $(LDFLAGS) -o $@ -c $< 

install: $(LIBS)
#	for mymod in $(clmpi_so_LIBS); do ( $(PNMPI_BIN_PATH)/pnmpi-patch $$mymod    $(PNMPI_MOD_LIB_PATH)/$$mymod ); done
	for mymod in $(clmpi_so_LIBS); do ( cp $$mymod    $(PNMPI_MOD_LIB_PATH)/$$mymod ); done
	for mymod in $(clmpi_a_LIBS);  do ( cp $$mymod    $(PNMPI_MOD_LIB_PATH)/$$mymod ); done
	for myheader in $(clmpi_HEAD); do ( cp $$myheader $(PNMPI_INC_PATH)/$$myheader  ); done
	for mymod in $(pbmpi_so_LIBS); do ( cp $$mymod    $(PNMPI_MOD_LIB_PATH)/$$mymod ); done
	for mymod in $(pbmpi_a_LIBS);  do ( cp $$mymod    $(PNMPI_MOD_LIB_PATH)/$$mymod ); done
	for myheader in $(pbmpi_HEAD); do ( cp $$myheader $(PNMPI_INC_PATH)/$$myheader  ); done

uninstall:
	for mymod in $(LIBS);          do ( rm $(PNMPI_MOD_LIB_PATH)/$$mymod ); done
	for myheader in $(clmpi_HEAD); do ( rm $(PNMPI_INC_PATH)/$$myheader  ); done
	for myheader in $(pbmpi_HEAD); do ( rm $(PNMPI_INC_PATH)/$$myheader  ); done

.PHONY: clean
clean:
	-rm -rf $(PROGRAM) $(OBJS) $(DEPS) $(LIBS) $(clmpi_o_LIBS)

.PHONY: clean_core
clean_core:
	-rm -rf *.core

