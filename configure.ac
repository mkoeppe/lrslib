#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(lrslib, 062+autotools)
AM_INIT_AUTOMAKE(foreign)
AC_CONFIG_FILES([Makefile mplrs/Makefile])
AC_CONFIG_MACRO_DIRS([m4])
AC_PROG_CC
AC_PROG_LIBTOOL

# plrs -- requires C++ and Boost.
AC_PROG_CXX
AC_ARG_ENABLE([plrs],
    [AS_HELP_STRING([--enable-plrs@<:@=ARG@:>@],
      [enable building plrs @<:@default=check@:>@])],
    [:],
    [enable_plrs=check])
AS_IF([test "x$enable_plrs" != xno],
  [AX_BOOST_BASE([1.51],
      [dnl AX_BOOST_SYSTEM
       dnl AX_BOOST_THREAD
       AC_MSG_RESULT([yes])
       AC_SUBST(BOOST_SYSTEM_LIB, [-lboost_system])
       AC_SUBST(BOOST_THREAD_LIB, [-lboost_thread])
       enable_plrs=yes],
      [AS_IF([test "x$enable_plrs" == xyes],
      	  [AC_MSG_ERROR([plrs requires Boost >= 1.51])],
	  [AC_MSG_NOTICE([Will not build plrs because Boost was not found])
	   enable_plrs=no])])
  ])
AM_CONDITIONAL([PLRS], [test x$enable_plrs = xyes])

# mplrs -- requires MPI.
AC_ARG_ENABLE([mplrs],
    [AS_HELP_STRING([--enable-mplrs@<:@=ARG@:>@],
      [enable building mplrs @<:@default=check@:>@])],
    [:],
    [enable_mplrs=check])
AS_IF([test "x$enable_mplrs" != xno],
  [AC_LANG(C++)
   AX_MPI(
      [enable_mplrs=yes],
      [AS_IF([test "x$enable_mplrs" == xyes],
      	  [AC_MSG_ERROR([mplrs requires MPI C++ compiler])],
	  [AC_MSG_NOTICE([Will not build mplrs because MPI C++ compiler was not found])
	   enable_mplrs=no])])
  ])
AM_CONDITIONAL([MPLRS], [test x$enable_mplrs = xyes])

AC_OUTPUT
