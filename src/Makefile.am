# Assuming foo is a program or library:
#  foo_CFLAGS:   Additional C compiler flags
#  foo_CPPFLAGS: Additional preprocessor flags (-Is and -Ds)
#  foo_LDADD:    Additional link objects, -ls and -Ls (if foo is a program)
#  foo_LIBADD:   Additional link objects, -ls and -Ls (if foo is a library) 
#  foo LDFLAGS:  Additional linker flags
# The default value for foo_XXXFLAGS is $(AM_XXXFLAGS).

AM_CFLAGS = -fPIC -std=c++0x -pthread -g
AM_CXXFLAGS = $(AM_CFLAGS)

libclmpi_la_LIBRARY = libclmpi.la
libpbmpi_la_LIBRARY = libpbmpi.la
lib_LTLIBRARIES = $(libclmpi_la_LIBRARY) $(libpbmpi_la_LIBRARY)

# ===== libclmpi =====
libclmpi_la_SOURCES = 	clmpi_status.c \
			clmpi.cpp \
			clmpi_piggyback.c \
			clmpi_request.cpp 
libclmpi_la_CPPFLAGS = -DPB_DATATYPE2 
# user-defined custom name
libclmpi_la_OBJECT    = ./libclmpi_la-libclmpi.lo
libclmpi_la_OBJECTS_O = $(libclmpi_la_OBJECTS:%.lo=%.o) 
libclmpi_la_STACK_PMPI_OBJECT_O = ./libclmpi.o
libclmpi_la_LIBS_OBJECTS_O = $(libclmpi_la_OBJECTS:%.lo=.libs/%.o) 
libclmpi_la_LIBS_STACK_PMPI_OBJECT_O = ./.libs/.libs_libclmpi.o


# ===== libpbmpi =====
libpbmpi_la_SOURCES = 	clmpi_status.c \
			pbmpi.cpp \
			clmpi_piggyback.c \
			clmpi_request.cpp 
libpbmpi_la_CPPFLAGS = -DPB_DATATYPE2 
# user-defined custom name
libpbmpi_la_OBJECT    = ./libpbmpi_la-libpbmpi.lo
libpbmpi_la_OBJECTS_O = $(libpbmpi_la_OBJECTS:%.lo=%.o) 
libpbmpi_la_STACK_PMPI_OBJECT_O = ./libpbmpi.o
libpbmpi_la_LIBS_OBJECTS_O = $(libpbmpi_la_OBJECTS:%.lo=.libs/%.o) 
libpbmpi_la_LIBS_STACK_PMPI_OBJECT_O = ./.libs/.libs_libpbmpi.o

install_libdir = $(libdir)
dist_install_lib_SCRIPTS = $(libclmpi_la_STACK_PMPI_OBJECT_O) $(libclmpi_la_LIBS_STACK_PMPI_OBJECT_O) $(libpbmpi_la_STACK_PMPI_OBJECT_O) $(libpbmpi_la_LIBS_STACK_PMPI_OBJECT_O)

# $(libclmpi_a_LIB): $(libclmpi_a_OBJECTS)
# 	$(stack_pmpi_DIR)/stack_pmpi $(libclmpi_a_STACK_PMPI_OBJECT) 0 $^
# 	ar cr $@ $(libclmpi_a_STACK_PMPI_OBJECT)
# 	ranlib $@

# $(libclmpi_so_LIB): $(libclmpi_a_LIB)
# 	$(MPICC) $(AM_CXXFLAGS) $(libclmpi_a_CXXFLAGS) $(CXXFLAGS) $(AM_LDFLAGS) -shared -o $@ $(libclmpoi_a_STACK_PMPI_OBJECT)
$(libclmpi_la_LIBRARY): $(libclmpi_la_OBJECTS) $(libclmpi_la_DEPENDENCIES) $(libclmpi_la_OBJECTS_O) $(libclmpi_la_LIBS_OBJECTS_O) 
	$(stack_pmpi_DIR)/stack_pmpi $(libclmpi_la_STACK_PMPI_OBJECT_O) 0 $(libclmpi_la_OBJECTS_O)
	$(stack_pmpi_DIR)/stack_pmpi $(libclmpi_la_LIBS_STACK_PMPI_OBJECT_O) 0 $(libclmpi_la_LIBS_OBJECTS_O)
	head -n 2 ./libclmpi_la-clmpi.lo > $(libclmpi_la_OBJECT)
	echo "pic_object='$(libclmpi_la_LIBS_STACK_PMPI_OBJECT_O)'" >> $(libclmpi_la_OBJECT)
	echo "non_pic_object='$(libclmpi_la_STACK_PMPI_OBJECT_O)'" >> $(libclmpi_la_OBJECT)
	$(CXXLINK) -rpath $(libdir) $(libclmpi_la_OBJECT) $(libclmpi_la_LIBADD) $(LIBS)


# $(libpbmpi_a_LIB): $(libpbmpi_a_OBJECTS)
# 	$(stack_pmpi_DIR)/stack_pmpi $(libpbmpi_a_STACK_PMPI_OBJECT) 0 $^
# 	ar cr $@ $(libpbmpi_a_STACK_PMPI_OBJECT)
# 	ranlib $@

# $(libpbmpi_so_LIB): $(libpbmpi_a_LIB)
# 	$(MPICC) $(AM_CXXFLAGS) $(libpbmpi_a_CXXFLAGS) $(CXXFLAGS) $(AM_LDFLAGS) -shared -o $@ $(libclmpoi_a_STACK_PMPI_OBJECT)
$(libpbmpi_la_LIBRARY): $(libpbmpi_la_OBJECTS) $(libpbmpi_la_DEPENDENCIES) $(libpbmpi_la_OBJECTS_O) $(libpbmpi_la_LIBS_OBJECTS_O) 
	$(stack_pmpi_DIR)/stack_pmpi $(libpbmpi_la_STACK_PMPI_OBJECT_O) 0 $(libpbmpi_la_OBJECTS_O)
	$(stack_pmpi_DIR)/stack_pmpi $(libpbmpi_la_LIBS_STACK_PMPI_OBJECT_O) 0 $(libpbmpi_la_LIBS_OBJECTS_O)
	head -n 2 ./libpbmpi_la-pbmpi.lo > $(libpbmpi_la_OBJECT)
	echo "pic_object='$(libpbmpi_la_LIBS_STACK_PMPI_OBJECT_O)'" >> $(libpbmpi_la_OBJECT)
	echo "non_pic_object='$(libpbmpi_la_STACK_PMPI_OBJECT_O)'" >> $(libpbmpi_la_OBJECT)
	$(CXXLINK) -rpath $(libdir) $(libpbmpi_la_OBJECT) $(libpbmpi_la_LIBADD) $(LIBS)



all-local: $(libclmpi_a_LIB) $(libclmpi_so_LIB) $(libpbmpi_a_LIB) $(libpbmpi_so_LIB)

clean-local:
	-rm -f $(libclmpi_a_LIB) $(libclmpi_so_LIB) $(libclmpi_a_STACK_PMPI_OBJECT)
	-rm -f $(libpbmpi_a_LIB) $(libpbmpi_so_LIB) $(libpbmpi_a_STACK_PMPI_OBJECT)