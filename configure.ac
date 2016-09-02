#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# https://www.gnu.org/software/autoconf/manual/autoconf-2.60/html_node/index.html#Top

AC_PREREQ([2.63])
AC_INIT([CLMPI],
        m4_esyscmd([git describe --always | awk '/.*/ {sub(/^v/, ""); printf "%s",$1; exit}']), 
	[kento@llnl.gov])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
#AM_INIT_AUTOMAKE([ReMPI], [0.1])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
#LT_INIT([shared static])

### Action Macros
AC_PROG_LIBTOOL
AC_SUBST([LIB_CURRENT], [0])
AC_SUBST([LIB_REVISION], [0])
AC_SUBST([LIB_AGE], [0])

AC_ARG_WITH([stack-pmpi],
            [AS_HELP_STRING([--with-stack-pmpi],
              [path to stack_pmpi directory])],
            [stack_pmpi_DIR=$withval],
            [])
if test "$stack_pmpi_DIR" = ""; then
   AC_MSG_ERROR([This package needs stack_pmpi (in STACKP). (Option: --with-stack-pmpi=<path to stack_pmpi directory>)])
fi
AC_SUBST(stack_pmpi_DIR)

#### Checks for programs.
LX_FIND_MPI # TODO: AC_LANG_CASE([C++], ...) is not executed


AC_CHECK_PROGS([MAKE], [make], [:])
if test "$MAKE" = :; then
   AC_MSG_ERROR([This package needs make.])
fi

#AC_CHECK_PROGS([MPICC], [mpicc], [:])
if test "$MPICC" = :; then
   AC_MSG_ERROR([This package needs mpicc.])
fi
AC_PROG_CC([mpicc])
AM_PROG_CC_C_O([mpicc])
AC_PROG_CXX([mpicc])
AC_SUBST([CC], [$MPICC])
AC_SUBST([CXX], [$MPICC])

AC_CHECK_PROGS([MPIF90], [mpif90], [:])
if test "$MPIF90" = :; then
   AC_MSG_ERROR([This package needs mpif90.])
fi
AC_PROG_F77([mpif90])


#AC_ARG_VAR([CC], [CC for compile])

#### Checks for libraries.

#### Checks for header files.

#### Checks for typedefs, structures, and compiler characteristics.

#### Checks for library functions.

#### Output files.

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES([
	Makefile 
	src/Makefile 
	example/Makefile])
AC_OUTPUT
