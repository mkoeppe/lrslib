#include <stdio.h>
#include <string.h>
#include "lrslib.h"
#define MAXLINE 1000


int
main (int argc, char *argv[])

{
  long block,m,n,i;
  char name[MAXLINE];

  if(argc < 4)
    {
     redund_main(argc,argv);
     printf("\n");
     return 0;
    }
  block = atoi(argv[3]);

/* make new input file with first block input lines */

  if ((lrs_ifp = fopen (argv[1], "r")) == NULL)
        {
          printf ("\nBad input file name");
          return (FALSE);
        }
      else
        printf ("\n*Input taken from file %s", argv[1]);

  if ((lrs_ofp = fopen (argv[2], "a")) == NULL)
        {
          printf ("\nBad output file name");
          return (FALSE);
        }
      else
        printf ("\n*Output sent to file %s\n", argv[2]);

/* process input file */
  fscanf (lrs_ifp, "%s", name);

  while ( ( fgets (name, MAXLINE,lrs_ifp) != NULL ) 
             &&  (strcmp (name, "begin\n") != 0))   /*skip until "begin" found processing options */
     fputs (name,lrs_ofp);

  fputs (name,lrs_ofp);

  if (fscanf (lrs_ifp, "%ld %ld %s", &m, &n, name) == EOF)
    {
      fprintf (lrs_ofp, "\nNo data in file");
      return (FALSE);
    }

  if (block > m )
     block =m;

  fprintf(lrs_ofp,"\n%ld %ld %s",block,n,name);

  for (i=1;i<=m;i++)
    {
     if ( fgets (name, MAXLINE,lrs_ifp) == NULL)
        {
          fprintf (lrs_ofp, "\nInsufficient data");
          return (FALSE);
        }

     if (i <= block)
        fputs(name,lrs_ofp);

     }


  while  ( fgets (name, MAXLINE,lrs_ifp) != NULL)
     fputs (name,lrs_ofp);


   fclose(lrs_ofp);

   return(TRUE);
}

