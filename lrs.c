#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <stdlib.h>
#include <limits.h>
#include "lrsdriver.h"


int
main (int argc, char *argv[])

{
#ifndef MA
  lrs_main(argc,argv);    /* legacy lrs */
  return 0;
#else


/* multiple arithmetic lrs */

  char** newargv;    
  char* tmp;          /* when overflow occurs a new input file name is returned */
  long overfl=0;    /*  =0 no overflow =1 restart overwrite =2 restart append */
  int lrs_stdin=0;
  int i;

  if(argc == 1)
     lrs_stdin=1;

  tmp = malloc(PATH_MAX * sizeof (char));

  if ( (overfl=lrs1_main(argc,argv,0,tmp)) == 0)    
          goto byebye;

/* overflow condition triggered: a temporary file was created for restart */
/* create new argv for the remaining calls                                */

  newargv = makenewargv(&argc,argv,tmp);

#ifdef B128

  fprintf(stderr,"\n*lrs:overflow possible: restarting with 128 bit arithmetic\n");

  if  ( (overfl=lrs2_main(argc,newargv,overfl,tmp)) == 0)    
        goto done;
#endif

  fprintf(stderr,"\n*lrs:overflow possible: restarting with GMP arithmetic\n");

/* if you change tmp file name update newargv[1] */

  overfl=lrsgmp_main(argc,newargv,overfl,tmp);  

done:
  for(i = 0; i < argc; ++i)
        free(newargv[i]);
  free(newargv);
  
byebye:
  if(lrs_stdin==1)    /* get rid of temporary file for stdin */
    remove(tmp);
  free(tmp);
  return overfl;

#endif
}

