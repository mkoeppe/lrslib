/* This file contains functions and variables that should not be duplicated per arithmetic */

#include "lrsdriver.h"

/* Globals; these need to be here, rather than lrsdriver.h, so they are
   not multiply defined. */

FILE *lrs_cfp;			/* output file for checkpoint information       */
FILE *lrs_ifp;			/* input file pointer       */
FILE *lrs_ofp;			/* output file pointer      */

char** makenewargv(int *argc,char** argv,char *tmp)
{
  int i;
  char** newargv;

  newargv = (char**) malloc((*argc+3) * sizeof *newargv);
  for(i = 0; i < *argc; ++i)
    {
      if (i != 1)
       {
        size_t length = strlen(argv[i])+1;
        newargv[i] = (char *) malloc(length);
        strncpy(newargv[i], argv[i], length);
       }
    }
/* make tmp the new input file */
   size_t length = strlen(tmp)+1;
   newargv[1] = (char *)malloc(length);
   strncpy(newargv[1], tmp, length);
   if(*argc == 1)         /* input was stdin*/
       *argc = 2;
   newargv[*argc] = NULL;
   return newargv;
}

