# Makefile for lrslib including lrs and redund 
# 2000.6.14
#
# choose one of the first 4 and gmp if applicable - see README
#
# make all      normal version for 32bit machines with timing/signals handling
# make all64    for machines with 64bit integers, eg. DEC Alpha
# make ansi     ansi standard version for 32bit machines without signals handling
# make nosigs   ansi standard version for 32bit machines without timing/signals handling

# make clean    to clean all executables

# make gmp      uses gmp arithmetic, set paths for include files *first*

#The following INCLUDE,LIB paths only needed for gmp version
#INCLUDEDIR = /usr/local/include
INCLUDEDIR = /labs/cgm/include
#INCLUDEDIR = /usr/include/gmp2

LIBDIR     = /usr/local/lib
#LIBDIR     = /labs/cgm/lib
#LIBDIR     = /usr/lib

all:	lrs.c lrslib.c lrslib.h lrsmp.c lrsmp.h lrslong.c lrslong.h redund.c buffer.c
	gcc -O3 -DTIMES -DSIGNALS -o lrs  lrs.c lrslib.c lrsmp.c
	gcc -O3 -DTIMES -DSIGNALS -o redund  redund.c lrslib.c lrsmp.c
	gcc -O3 -DTIMES -DSIGNALS -DLONG -o lrs1  lrs.c lrslib.c lrslong.c
	gcc -O3 -DTIMES -DSIGNALS -DLONG -o redund1  redund.c lrslib.c lrslong.c
	gcc -O3 -o buffer buffer.c

gmp:	lrs.c redund.c lrslib.h lrslib.c lrsgmp.h lrsgmp.c
	gcc -O3 -DTIMES -DSIGNALS  -DGMP -I${INCLUDEDIR} lrs.c lrslib.c lrsgmp.c -L${LIBDIR}  -lgmp -o glrs
	gcc -O3 -DTIMES -DSIGNALS -DGMP -I${INCLUDEDIR} redund.c lrslib.c lrsgmp.c -L${LIBDIR} -lgmp -o gredund
	gcc -O3 -o buffer buffer.c

all64:	lrs.c lrslib.c lrslib.h lrsmp.c lrsmp.h lrslong.c lrslong.h redund.c buffer.c
	gcc -DTIMES -DSIGNALS -DB64 -O3 -o lrs  lrs.c lrslib.c lrsmp.c
	gcc -DTIMES -DSIGNALS -DB64 -O3 -o redund  redund.c lrslib.c lrsmp.c
	gcc -DTIMES -DSIGNALS -DLONG -DB64 -O3 -o lrs1  lrs.c lrslib.c lrslong.c
	gcc -DTIMES -DSIGNALS -DLONG -DB64 -O3 -o redund1  redund.c lrslib.c lrslong.c
	gcc -O3 -o buffer buffer.c

ansi:	lrs.c lrslib.c lrslib.h lrsmp.c lrsmp.h lrslong.c lrslong.h redund.c buffer.c
	gcc -ansi -DTIMES   -O3 -o lrs  lrs.c lrslib.c lrsmp.c
	gcc -ansi -DTIMES   -O3 -o redund redund.c lrslib.c lrsmp.c
	gcc -ansi -DTIMES  -DLONG  -O3 -o lrs1  lrs.c lrslib.c lrslong.c
	gcc -ansi -DTIMES  -DLONG  -O3 -o redund1 redund.c lrslib.c lrslong.c
	gcc -O3 -o buffer buffer.c

nosigs:	lrs.c lrslib.c lrslib.h lrsmp.c lrsmp.h lrslong.c lrslong.h redund.c buffer.c
	gcc -ansi  -O3 -o lrs  lrs.c lrslib.c lrsmp.c
	gcc -ansi  -O3 -o redund redund.c lrslib.c lrsmp.c
	gcc -ansi  -O3 -DLONG -o lrs1  lrs.c lrslib.c lrslong.c
	gcc -ansi  -O3 -DLONG -o redund1 redund.c lrslib.c lrslong.c
	gcc -ansi -O3 -o buffer buffer.c

lrs:    lrs.c lrslib.c lrslong.c lrsmp.c
	gcc -Wall -ansi -O3 -o lrs  lrs.c lrslib.c lrsmp.c

clean:
	rm -rf lrs lrs1 redund redund1 buffer glrs gredund


