include ./Makefile.config

# MOD    = libpbdriver.so \
# 	 libpb_datatype1.so libpb_datatype2.so \
# 	 libpb_twomsg1.so libpb_twomsg2.so \
# 	 libpb_pack1.so libpb_pack2.so\
#          libpb_copy1.so libpb_copy2.so

MOD    =  libpbdriver.so \
	  libclmpi.so \
	  libpb_datatype1.so \
	  libpb_datatype2.so 

HEADER = pb_mod.h 

CFLAGS += -I$(PNMPI_INC_PATH) 
CCFLAGS += -I$(PNMPI_INC_PATH)

all: $(MOD) 

pb_dt.o: 

.SUFFIXES: .c .o .so

.o.so: 
#	$(CROSSLDXX) -o $@ $(SFLAGS) $< -ma
	$(MPICC) -shared -o $@ $<

.c.o:
	$(MPICC) -c $(CFLAGS) $<

.cpp.o:
	$(MPIXX) -c $(CFLAGS) $<

libclmpi.o: clmpi.cpp
	$(MPICC) -c $(CFLAGS) -DPBDRIVER_CHECK $<
	mv clmpi.o libclmpi.o

libpbdriver.o: pbdriver.c
	$(MPICC) -c $(CFLAGS) -DPBDRIVER_CHECK $<
	mv pbdriver.o libpbdriver.o

libpb_datatype1.o: piggyback.c
	$(MPICC) -c $(CFLAGS) -DPB_DATATYPE1 $<
	mv piggyback.o libpb_datatype1.o

libpb_datatype2.o: piggyback.c
	$(MPICC) -c $(CFLAGS) -DPB_DATATYPE2 $<
	mv piggyback.o libpb_datatype2.o

libpb_twomsg1.o: piggyback.c
	$(MPICC) -c $(CFLAGS) -DPB_TWOMSG1 $<
	mv piggyback.o libpb_twomsg1.o

libpb_twomsg2.o: piggyback.c
	$(MPICC) -c $(CFLAGS) -DPB_TWOMSG2 $<
	mv piggyback.o libpb_twomsg2.o

libpb_pack1.o: piggyback.c
	$(MPICC) -c $(CFLAGS) -DPB_PACK1 $<
	mv piggyback.o libpb_pack1.o

libpb_pack2.o: piggyback.c
	$(MPICC) -c $(CFLAGS) -DPB_PACK2 $<
	mv piggyback.o libpb_pack2.o

libpb_copy1.o: piggyback.c
	$(MPICC) -c $(CFLAGS) -DPB_COPY1 $<
	mv piggyback.o libpb_copy1.o

libpb_copy2.o: piggyback.c
	$(MPICC) -c $(CFLAGS) -DPB_COPY2 $<
	mv piggyback.o libpb_copy2.o


install: $(MOD)
	for mymod in $(MOD);       do ($(PNMPI_BIN_PATH)/pnmpi-patch $$mymod    $(PNMPI_MOD_LIB_PATH)/$$mymod ); done
	for myheader in $(HEADER); do ( cp                           $$myheader $(PNMPI_INC_PATH)/$$myheader  ); done

uninstall:
	for mymod in $(MOD);       do ( rm $(PNMPI_MOD_LIB_PATH)/$$mymod ); done
	for myheader in $(HEADER); do ( rm $(PNMPI_INC_PATH)/$$myheader  ); done

clean:
	rm -f $(MOD) *.o

clobber: clean
	rm -f *~
	for mymod in $(MOD); do ( rm $(PNMPI_MOD_LIB_PATH)/$$mymod ); done

