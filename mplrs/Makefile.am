# mplrs -- requires MPI.
AUTOMAKE_OPTIONS = subdir-objects

# Use of subdir is needed so we can switch out the C compiler.
# See also http://stackoverflow.com/questions/3968656/how-to-compile-mpi-and-non-mpi-version-of-the-same-program-with-automake
CC = $(MPICC)

bin_PROGRAMS =
noinst_HEADERS = 

if MPLRS
bin_PROGRAMS += mplrs

AM_CPPFLAGS = -DTIMES -DSIGNALS -DPLRS

# Libtool convenience libraries
noinst_LTLIBRARIES = libmplrs1.la libmplrs2.la libmplrsgmp.la

libmplrs1_la_SOURCES = ../lrslib.c ../lrslong.c
libmplrs1_la_CPPFLAGS = $(AM_CPPFLAGS) -DMA -DSAFE -DLRSLONG

libmplrs2_la_SOURCES = ../lrslib.c ../lrslong.c
libmplrs2_la_CPPFLAGS = $(AM_CPPFLAGS) -DMA -DSAFE -DB128 -DLRSLONG

libmplrsgmp_la_SOURCES = ../lrslib.c ../lrsgmp.c
libmplrsgmp_la_CPPFLAGS = $(AM_CPPFLAGS) -DMA -DGMP $(GMP_CFLAGS)
libmplrsgmp_la_LIBADD = $(GMP_LDFLAGS) -lgmp

mplrs_SOURCES = ../mplrs.c ../lrsdriver.c
mplrs_CPPFLAGS = $(AM_CPPFLAGS) -D_WITH_GETLINE -DMA -DB128
mplrs_LDADD = $(noinst_LTLIBRARIES)
endif
