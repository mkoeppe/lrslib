#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(lrslib, 071b+autotools-2021-07-13)
AM_INIT_AUTOMAKE(foreign)
AC_CONFIG_FILES([Makefile mplrs/Makefile])
AC_CONFIG_MACRO_DIRS([m4])
AC_PROG_CC
AC_PROG_LIBTOOL

AC_CHECK_SIZEOF([long])
case $ac_cv_sizeof_long in
    8)
        ;;
    4)
	AC_DEFINE(B32)
	;;
    *)
	AC_MSG_ERROR([lrslib can only be built if sizeof(long) is 4 or 8; found $ac_cv_sizeof_long])
	;;
esac

dnl mplrs -- requires MPI.
AC_ARG_ENABLE([mplrs],
    [AS_HELP_STRING([--enable-mplrs@<:@=ARG@:>@],
      [enable building mplrs @<:@default=check@:>@])],
    [:],
    [enable_mplrs=check])
AS_IF([test "x$enable_mplrs" != xno],
  [AX_MPI(
      [enable_mplrs=yes],
      [AS_IF([test "x$enable_mplrs" == xyes],
          [AC_MSG_ERROR([mplrs requires MPI C compiler])],
          [AC_MSG_NOTICE([Will not build mplrs because MPI C compiler was not found])
           enable_mplrs=no])])
  ])
AM_CONDITIONAL([MPLRS], [test x$enable_mplrs = xyes])

ACX_PTHREAD
LIBS="$PTHREAD_LIBS $LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
CC="$PTHREAD_CC"

AC_OUTPUT
