#new makefile for lrslib-050    2012.9.27

#contains multithread version of lrs called plrs, wrapper written by Gary Roumanis
#if Boost libraries are not available, comment out plrs compiles     http://www.boost.org/

#make all               uses gmp and long arithmetic 
#make allmp             uses native mp and long arithmetic

#Select a path below to give location of Boost library
#mai64 mai12
#BOOSTINC = /usr/include/boost152/boost_1_52_0
#BOOSTLIB = /usr/include/boost152/boost_1_52_0/stage/lib


# mune.mcgill.ca
#BOOSTINC = /home/cgm/avis/C/ve/boost/boost_1_51_0/
#BOOSTLIB = /home/cgm/avis/C/ve/boost/boost_1_51_0/stage/lib

# cgm-server.mcgill.ca
BOOSTINC = /usr/include/boost151/boost/include/
BOOSTLIB = /usr/include/boost151/boost/lib/

#Select a path below to give location of gmp library

#cygwin
INCLUDEDIR = /usr/include
LIBDIR     = /usr/lib

#linux at mcgill with gmp version 3
#INCLUDEDIR = /usr/local/include
#LIBDIR     = /usr/local/lib

all:	2nash.c lrs.c redund.c lrslib.h lrslib.c lrsgmp.h lrsgmp.c nash.c
	gcc -O3 -DTIMES -DSIGNALS  -DGMP -I${INCLUDEDIR} lrs.c lrslib.c lrsgmp.c -L${LIBDIR}  -lgmp -o lrs
	gcc -Wall -O3 -DTIMES -DSIGNALS -DLONG -o lrs1 lrs.c lrslib.c lrslong.c
	gcc -O3  -DTIMES -DSIGNALS -DGMP -I${INCLUDEDIR} redund.c lrslib.c lrsgmp.c -L${LIBDIR} -lgmp -o redund
	gcc -O3 -DTIMES -DSIGNALS -DLONG -o redund1  redund.c lrslib.c lrslong.c
	gcc -O3  -DLRS_QUIET -DTIMES -DSIGNALS -DGMP -I${INCLUDEDIR} nash.c lrslib.c lrsgmp.c -L${LIBDIR} -lgmp -o nash
	gcc -O3  -DLRS_QUIET -DTIMES -DSIGNALS -DGMP -I${INCLUDEDIR} 2nash.c lrslib.c lrsgmp.c -L${LIBDIR} -lgmp -o 2nash
	gcc -O3 -o setnash setupnash.c lrslib.c lrsmp.c
	gcc -O3 -o setnash2 setupnash2.c lrslib.c lrsmp.c

lrs:	lrs.c lrslib.h lrslib.c lrsgmp.h lrsgmp.c
	gcc -O3 -DTIMES -DSIGNALS  -DGMP -I${INCLUDEDIR} lrs.c lrslib.c lrsgmp.c -L${LIBDIR}  -lgmp -o lrs

fourier:	fourier.c lrslib.h lrslib.c lrsgmp.h lrsgmp.c
	gcc -O3 -DTIMES -DSIGNALS  -DGMP -I${INCLUDEDIR} fourier.c lrslib.c lrsgmp.c -L${LIBDIR}  -lgmp -o fourier


plrs:	  	plrs.cpp plrs.hpp lrslib.c lrslib.h lrsmp.c lrsmp.h
		g++ -DGMP  -Wall -Wno-write-strings -Wno-sign-compare -I${BOOSTINC}  -L${BOOSTLIB} -Wl,-rpath=${BOOSTLIB} -O3 -DPLRS -DGMP -o plrs plrs.cpp lrslib.c lrsgmp.c -lboost_thread -lgmp

plrsmp:	  	plrs.cpp plrs.hpp lrslib.c lrslib.h lrsmp.c lrsmp.h
		g++ -Wall -Wno-write-strings -Wno-sign-compare -Wno-unused-variable -I${BOOSTINC}  -L${BOOSTLIB} -Wl,-rpath=${BOOSTLIB} -lboost_thread -O3 -DPLRS -o plrsmp plrs.cpp lrslib.c lrsmp.c 
		g++ -Wall -Wno-write-strings -Wno-sign-compare -Wno-unused-variable -I${BOOSTINC}  -L${BOOSTLIB} -Wl,-rpath=${BOOSTLIB} -O3 -DPLRS -DLONG -o plrs1 plrs.cpp lrslib.c lrslong.c -lboost_thread

allmp:		lrs.c lrslib.c lrslib.h lrsmp.c lrsmp.h
		gcc -Wall -O3 -DTIMES -DSIGNALS -o lrs lrs.c lrslib.c lrsmp.c
		gcc -Wall -O3 -DTIMES -DSIGNALS -DLONG -o lrs1 lrs.c lrslib.c lrslong.c
		gcc -O3 -DTIMES -DSIGNALS -o redund  redund.c lrslib.c lrsmp.c
		gcc -O3 -DTIMES -DSIGNALS -DLONG -o redund1  redund.c lrslib.c lrslong.c
		gcc -O3 -DLRS_QUIET  -DTIMES -DSIGNALS -o nash nash.c lrslib.c lrsmp.c
		gcc -O3 -o setnash setupnash.c lrslib.c lrsmp.c
		gcc -O3 -o setnash2 setupnash2.c lrslib.c lrsmp.c
		gcc -O3 -o 2nash 2nash.c


clean:		
		rm -rf 2nash lrs lrs1 redund redund1 nash setnash setnash2 plrs *.o *.exe
