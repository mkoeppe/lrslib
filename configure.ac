#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(lrslib, 062+autotools-2017-03-03)
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

dnl plrs -- requires C++ and Boost.
AC_PROG_CXX
AC_ARG_ENABLE([plrs],
    [AS_HELP_STRING([--enable-plrs@<:@=ARG@:>@],
      [enable building plrs @<:@default=check@:>@])],
    [:],
    [enable_plrs=check])
AS_IF([test "x$enable_plrs" != xno],
  [AX_BOOST_BASE([1.51],
      [dnl AC_MSG_RESULT([yes])
       have_enough_boost=yes
       ],
      [have_enough_boost=no])])
dnl Not using ax_boost_system.m4 and ax_boost_thread.m4 from Autoconf Archive,
dnl because they do not find these libraries when the library locations are
dnl provided via LDFLAGS etc. rather than via --with-boost...
AS_IF([test x$enable_plrs != xno -a x$have_enough_boost != xno],
   [
      dnl adapted from ax_boost_system.m4
      AC_CACHE_CHECK(whether the Boost::System library is available,
                     ax_cv_boost_system,
        [AC_LANG_PUSH([C++])
	   LDFLAGS_SAVE="$LDFLAGS"
	   BOOST_SYSTEM_LIB=-lboost_system
	   LDFLAGS="$LDFLAGS $BOOST_SYSTEM_LIB"
	   AC_LINK_IFELSE([AC_LANG_PROGRAM([[@%:@include <boost/system/error_code.hpp>]],			                                         [[boost::system::error_category *a = 0;]])],
				              ax_cv_boost_system=yes, ax_cv_boost_system=no)
	   LDFLAGS="$LDFLAGS_SAVE"
	 AC_LANG_POP([C++])])
      dnl adapted from ax_boost_thread.m4
      AC_CACHE_CHECK(whether the Boost::Thread library is available,
					   ax_cv_boost_thread,
        [AC_LANG_PUSH([C++])
			 CXXFLAGS_SAVE=$CXXFLAGS
			 LDFLAGS_SAVE=$LDFLAGS
			 BOOST_THREAD_LIB=-lboost_thread
			 LDFLAGS="$LDFLAGS $BOOST_THREAD_LIB $BOOST_SYSTEM_LIB"
			 if test "x$host_os" = "xsolaris" ; then
				 CXXFLAGS="-pthreads $CXXFLAGS"
			 elif test "x$host_os" = "xmingw32" ; then
				 CXXFLAGS="-mthreads $CXXFLAGS"
			 else
				CXXFLAGS="-pthread $CXXFLAGS"
			 fi
	 AC_LINK_IFELSE([AC_LANG_PROGRAM([[@%:@include <boost/thread/thread.hpp>]],
                                   [[boost::thread_group thrds;
                                   return 0;]])],
                   ax_cv_boost_thread=yes, ax_cv_boost_thread=no)
			 CXXFLAGS=$CXXFLAGS_SAVE
			 LDFLAGS=$LDFLAGS_SAVE
         AC_LANG_POP([C++])
	 ])
    ])
AC_SUBST(BOOST_SYSTEM_LIB)
AC_SUBST(BOOST_THREAD_LIB)

AS_IF([test x$have_enough_boost = xyes -a x$ax_cv_boost_system = xyes -a x$ax_cv_boost_thread = xyes],
  [dnl AC_MSG_NOTICE([Will build plrs])
   enable_plrs=yes],
  [AS_IF([test "x$enable_plrs" = xyes],
    [AC_MSG_ERROR([plrs requires Boost (including Boost::System, Boost::Thread) >= 1.51])],
    [AC_MSG_NOTICE([Will not build plrs because Boost (including Boost::System, Boost::Thread) was not found])
     enable_plrs=no])])

AM_CONDITIONAL([PLRS], [test x$enable_plrs = xyes])

dnl mplrs -- requires MPI.
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
