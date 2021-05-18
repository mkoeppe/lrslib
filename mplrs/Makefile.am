# mplrs -- requires MPI.
AUTOMAKE_OPTIONS = subdir-objects

# Use of subdir is needed so we can switch out the C compiler.
# See also http://stackoverflow.com/questions/3968656/how-to-compile-mpi-and-non-mpi-version-of-the-same-program-with-automake
CC = $(MPICC)

bin_PROGRAMS =
noinst_HEADERS = 

if MPLRS
bin_PROGRAMS += mplrs
noinst_HEADERS += ../mplrs.h ../mplrs.c

# Libtool convenience libraries
noinst_LTLIBRARIES = liblrs1.la liblrs2.la liblrsgmp.la

liblrs1_la_SOURCES = ../lrslib.c ../lrslong.c
liblrs1_la_CPPFLAGS = -DTIMES -DSIGNALS -DMA -DSAFE -DLRSLONG -DPLRS

liblrs2_la_SOURCES = ../lrslib.c ../lrslong.c
liblrs2_la_CPPFLAGS = -DTIMES -DSIGNALS -DMA -DSAFE -DB128 -DLRSLONG -DPLRS

liblrsgmp_la_SOURCES = ../lrslib.c ../lrsgmp.c
liblrsgmp_la_CPPFLAGS = -DTIMES -DSIGNALS -DMA -DGMP -DPLRS $(GMP_CFLAGS)
liblrsgmp_la_LIBADD = $(GMP_LDFLAGS) -lgmp

mplrs_SOURCES = ../mplrs.c ../lrsdriver.c ../lrslib.c ../lrsgmp.c
mplrs_CPPFLAGS = -D_WITH_GETLINE -DPLRS -DGMP 
mplrs_LDADD = $(noinst_LTLIBRARIES) -lgmp
endif
