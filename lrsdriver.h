#ifndef LRS_DRIVER_H
#define LRS_DRIVER_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


long lrs_main (int argc, char *argv[]);    /* lrs driver, argv[1]=input file, [argc-1]=output file */
long lrs1_main(int argc, char *argv[],long overf, char *tmp)/*__attribute__ ((visibility ("default") ))*/;
long lrs2_main(int argc, char *argv[],long overf, char *tmp)/*__attribute__ ((visibility ("default") ))*/;
long lrsgmp_main(int argc, char *argv[],long overf, char *tmp)/*__attribute__ ((visibility ("default") ))*/;

long redund_main (int argc, char *argv[]); /* redund driver, argv[1]=input file, [2]=output file */
long redund1_main(int argc, char *argv[],long overf,char *tmp)/*__attribute__ ((visibility ("default") ))*/;
long redund2_main(int argc, char *argv[],long overf,char *tmp)/*__attribute__ ((visibility ("default") ))*/;
long redundgmp_main(int argc, char *argv[],long overf,char *tmp)/*__attribute__ ((visibility ("default") ))*/;

char** makenewargv(int *argc,char** argv,char* tmp);


extern FILE *lrs_cfp;			/* output file for checkpoint information       */
extern FILE *lrs_ifp;			/* input file pointer       */
extern FILE *lrs_ofp;			/* output file pointer      */
#endif
