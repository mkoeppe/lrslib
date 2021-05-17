ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = subdir-objects

SUBDIRS = . mplrs

# Add -DLRS_QUIET to CPPFLAGS to get no error  messages, warnings etc.
# These flags should *not* include the arithmetic selecting define.
AM_CPPFLAGS = -DTIMES -DSIGNALS

bin_PROGRAMS = 2nash lrs lrs1 lrsnash redund redund1 setnash setnash2
noinst_PROGRAMS = nashdemo

# No arithmetic
## original makefile (version 062) target 'lrsnash' builds without a library,
## whereas 'all' would build it with 2nash_LIBADD = liblrsgmp.la
2nash_SOURCES = 2nash.c

# Long arithmetic
lrs1_SOURCES = lrs.c lrslib.c lrslong.c lrsdriver.c
lrs1_CPPFLAGS = -DLRSLONG
redund1_SOURCES = lrs.c lrslib.c lrslong.c lrsdriver.c
redund1_CPPFLAGS = -DLRSLONG

# Home-grown multi-precision arithmetic
setnash_SOURCES = setupnash.c lrslib.c lrsmp.c lrsdriver.c
setnash2_SOURCES = setupnash2.c lrslib.c lrsmp.c lrsdriver.c
## original makefile (version 062) target 'allmp' builds redund, lrs with home-grown mp;
## whereas 'all' builds it with GMP; see below.
#redund_SOURCES = redund.c lrslib.c lrsmp.c
#lrs_SOURCES = lrs.c lrslib.c lrsmp.c


# Libtool convenience libraries
noinst_LTLIBRARIES = liblrs1.la liblrs2.la liblrsgmp.la

liblrs1_la_SOURCES = lrslib.c lrslong.c
liblrs1_la_CPPFLAGS = -DMA -DSAFE -DLRSLONG

liblrs2_la_SOURCES = lrslib.c lrslong.c
liblrs2_la_CPPFLAGS = -DMA -DSAFE -DB128 -DLRSLONG

liblrsgmp_la_SOURCES = lrslib.c lrsgmp.c
liblrsgmp_la_CPPFLAGS = -DMA -DGMP $(GMP_CFLAGS)
liblrsgmp_la_LIBADD = $(GMP_LDFLAGS) -lgmp


# Installed library (shared and/or static) -- all arithmetics
lib_LTLIBRARIES = liblrs.la
liblrs_la_SOURCES = lrsdriver.c
liblrs_la_CPPFLAGS = $(AM_CPPFLAGS)
liblrs_la_LDFLAGS = -version-info 0:0:0 -no-undefined
liblrs_la_LIBADD = $(noinst_LTLIBRARIES)

# Binaries using the shared library
# Upstream makefile calls these binaries "lrs-shared", "lrsnash-shared"
lrsnash_SOURCES = lrsnash.c lrsnashlib.c
# note original makefile (version 062) builds lrsnash with -DLRS_QUIET
lrsnash_CPPFLAGS = -DGMP -DMA $(GMP_CFLAGS)
lrsnash_LDADD = liblrs.la

redund_SOURCES = lrs.c
redund_CPPFLAGS = -DMA $(GMP_CFLAGS)
redund_LDADD = liblrs.la

lrs_SOURCES = lrs.c
lrs_CPPFLAGS = -DMA $(GMP_CFLAGS)
lrs_LDADD = liblrs.la

nashdemo_SOURCES = nashdemo.c lrsnashlib.c
nashdemo_CPPFLAGS = -DGMP -DMA $(GMP_CFLAGS)
nashdemo_LDADD = liblrs.la


include_HEADERS = lrslib.h lrsdriver.h lrsgmp.h lrslong.h lrsmp.h
noinst_HEADERS = lrsnashlib.h mplrs.h lrsrestart.h

examplesdir = $(pkgdatadir)/doc/examples
nobase_dist_examples_DATA =			\
	games/game				\
	games/game1				\
	games/game2				\
	games/gamec				\
	games/gamem				\
	games/gamem.2				\
	games/gamem.gmp				\
	games/gamemp				\
	games/gamemp.1				\
	cube.ext				\
	cube.ine				\
	mit.ine					\
	mp5.ine					\
	ext/metric/cp4.ext			\
	ext/metric/cp5.ext			\
	ext/metric/cp6.ext			\
	ext/metric/cp7.ext			\
	ext/metric/mp5.ext			\
	ext/test/cut16_11.ext			\
	ext/test/cut32_16.ext			\
	ext/test/cyclic25_13.ext		\
	ext/test/tsp5.ext			\
	ine/cocoa13/bv10.ine			\
	ine/cocoa13/bv4.ine			\
	ine/cocoa13/bv5.ine			\
	ine/cocoa13/bv6.ine			\
	ine/cocoa13/bv7.ine			\
	ine/cocoa13/bv8.ine			\
	ine/cocoa13/bv9.ine			\
	ine/cocoa13/c28-14.ext			\
	ine/cocoa13/c30-15.ext			\
	ine/cocoa13/c40-20.ext			\
	ine/cocoa13/mit.ine			\
	ine/cocoa13/perm10.ine			\
	ine/cocoa13/perm4.ine			\
	ine/cocoa13/perm5.ine			\
	ine/cocoa13/perm6.ine			\
	ine/cocoa13/perm7.ine			\
	ine/cocoa13/perm8.ine			\
	ine/cocoa13/perm9.ine			\
	ine/metric/cp4.ine			\
	ine/metric/cp5.ine			\
	ine/metric/cp6.ine			\
	ine/metric/mp5.ine			\
	ine/metric/mp6.ine			\
	ine/mit/mit.ine				\
	ine/mit/mit288-281.ine			\
	ine/mit/mit31-20.ine			\
	ine/mit/mit41-16.ine			\
	ine/mit/mit708-9.ine			\
	ine/mit/mit71-61.ine			\
	ine/mit/mit90-86.ine			\
	ine/test/cross4.ine			\
	ine/test/cyclic17_8.ine			\
	ine/test/diamond.ine			\
	ine/test/in0.ine			\
	ine/test/in1.ine			\
	ine/test/in2.ine			\
	ine/test/in3.ine			\
	ine/test/in4.ine			\
	ine/test/in5.ine			\
	ine/test/in6.ine			\
	ine/test/in7.ine			\
	ine/test/inf.ine			\
	ine/test/kkd38_6.ine			\
	ine/test/kq20_11.ine			\
	ine/test/kq20_11a.ine			\
	ine/test/metric40_11.ine		\
	ine/test/metric80_16.ine		\
	ine/test/mit31_20.ine			\
	ine/test/trunc10.ine			\
	ine/test/trunc7.ine			\
	ine/test/truss2.ine			\
	ine/test/tsp5.ine			\
	ine/test-062/bv7.ine			\
	ine/test-062/c30-15.ext			\
	ine/test-062/c40-20.ext			\
	ine/test-062/cp6.ine			\
	ine/test-062/fq48-19.ine		\
	ine/test-062/m6.ine			\
	ine/test-062/mit.ine			\
	ine/test-062/mit71-61.ine		\
	ine/test-062/normaliz/bv7.in		\
	ine/test-062/normaliz/c30-15.ext.in	\
	ine/test-062/normaliz/c40-20.ext.in	\
	ine/test-062/normaliz/cp6.in		\
	ine/test-062/normaliz/fq48-19.in	\
	ine/test-062/normaliz/m6.in		\
	ine/test-062/normaliz/mit.in		\
	ine/test-062/normaliz/mit71-61.in	\
	ine/test-062/normaliz/perm10.in		\
	ine/test-062/normaliz/zfw91.in		\
	ine/test-062/perm10.ine			\
	ine/test-062/porta/bv7.ine.ieq		\
	ine/test-062/porta/c30-15.ext.poi	\
	ine/test-062/porta/c40-20.ext.poi	\
	ine/test-062/porta/cp6.ine.ieq		\
	ine/test-062/porta/fq48-19.ine.ieq	\
	ine/test-062/porta/m6.ine.ieq		\
	ine/test-062/porta/mit.ine.ieq		\
	ine/test-062/porta/mit71-61.ine.ieq	\
	ine/test-062/porta/perm10.ine.ieq	\
	ine/test-062/porta/zfw91.ine.ieq	\
	ine/test-062/zfw91.ine			\
	ine/test-062/zfw91nn.ine

EXTRA_DIST =					\
	chdemo.c				\
	float2rat.c				\
	fourier.c				\
	lpdemo.c				\
	lpdemo1.c				\
	lpdemo2.c				\
	lrsgmp.cpp				\
	lrslib.cpp				\
	lrslong.cpp				\
	lrsmp.cpp				\
	plotD.gp				\
	plotL.gp				\
	rat2float.c				\
	readme					\
	setupnash.c				\
	setupnash2.c				\
	vedemo.c

# From Sage package:
check-local:
	cat ${srcdir}/ext/test/cut16_11.ext > testin
	echo "volume" >> testin
	${builddir}/lrs testin > testout1
	echo `grep -G Volume testout1` > testout1
	echo "*Volume=32768/14175" > testout2
	diff testout1 testout2
	cat ${srcdir}/ine/test/cyclic17_8.ine > testin
	${builddir}/lrs testin > testout1
	echo `grep -G Totals testout1` > testout1
	echo "*Totals: vertices=935 rays=0 bases=935 integer_vertices=0" > testout2
	diff testout1 testout2
	rm -f testin
	rm -f testout1
	rm -f testout2

CLEANFILES = testout1 testout2
