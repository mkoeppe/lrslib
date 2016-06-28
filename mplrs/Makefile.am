# mplrs -- requires MPI.
AUTOMAKE_OPTIONS = subdir-objects

# Use of subdir is needed so we can switch out the C++ compiler.
# See also http://stackoverflow.com/questions/3968656/how-to-compile-mpi-and-non-mpi-version-of-the-same-program-with-automake
CXX = $(MPICXX)

bin_PROGRAMS =
noinst_HEADERS = 

if MPLRS
bin_PROGRAMS += mplrs mplrs1
noinst_HEADERS += ../mplrs.h ../mplrs.c
mplrs_SOURCES = ../mplrs.cpp ../lrslib.cpp ../lrsgmp.cpp
mplrs_CPPFLAGS = -D_WITH_GETLINE -DPLRS -DGMP 
mplrs_LDADD =  -lgmp
mplrs1_SOURCES = ../mplrs.cpp ../lrslib.cpp ../lrslong.cpp
mplrs1_CPPFLAGS = -D_WITH_GETLINE -DPLRS -DLRSLONG
endif

