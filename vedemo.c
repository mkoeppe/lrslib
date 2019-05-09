/* vedemo.c     lrslib vertex enumeration demo           */
/* last modified: May 29, 2001                           */
/* Copyright: David Avis 2001, avis@cs.mcgill.ca         */
/* Demo driver for ve code using lrs                 */
/* This program computes vertices of generated cubes */
/* given by H-representation                         */

#include <stdio.h>
#include <string.h>
#include "lrslib.h"

#define MAXCOL 1000     /* maximum number of colums */

void makecube (lrs_dic *P, lrs_dat *Q);

int
main (int argc, char *argv[])

{
  lrs_dic *P;	/* structure for holding current dictionary and indices  */
  lrs_dat *Q;	/* structure for holding static problem data             */
  lrs_mp_vector output;	/* one line of output:ray,vertex,facet,linearity */
  lrs_mp_matrix Lin;    /* holds input linearities if any are found      */


  long i;
  long col;    	/* output column index for dictionary            */


/* Global initialization - done once */

  if ( !lrs_init ("\n*vedemo:"))
    return 1;

/* compute the vertices of a set of hypercubes given by */
/* their H-representations.                             */

  for(i=1;i<=3;i++)
  {

/* allocate and init structure for static problem data */

    Q = lrs_alloc_dat ("LRS globals");
    if (Q == NULL)
       return 1;

/* now flags in lrs_dat can be set */

    Q->n=i+2;           /* number of input columns         (dimension + 1 )  */
    Q->m=2*i+2;         /* number of input rows = number of inequalities     */ 

    output = lrs_alloc_mp_vector (Q->n);

    P = lrs_alloc_dic (Q);   /* allocate and initialize lrs_dic      */
    if (P == NULL)
        return 1;

/* Build polyhedron: constraints and objective */ 

    makecube(P,Q);

/* code from here is borrowed from lrs_main */

/* Pivot to a starting dictionary                      */

  if (!lrs_getfirstbasis (&P, Q, &Lin, FALSE))
    return 1;

/* There may have been column redundancy               */
/* (although not for this example of hypercubes)       */

/* If so the linearity space is obtained and redundant */
/* columns are removed. User can access linearity space */
/* from lrs_mp_matrix Lin dimensions nredundcol x d+1  */


  for (col = 0L; col < Q->nredundcol; col++)  /* print linearity space */
    lrs_printoutput (Q, Lin[col]);      /* Array Lin[][] holds the coeffs.   */

/* We initiate reverse search from this dictionary       */
/* getting new dictionaries until the search is complete */
/* User can access each output line from output which is */
/* a vertex/ray/facet from the lrs_mp_vector output      */

  do
    {
      for (col = 0; col <= P->d; col++)
        if (lrs_getsolution (P, Q, output, col))
          lrs_printoutput (Q, output);
    }
    while (lrs_getnextbasis (&P, Q, FALSE));

  lrs_printtotals (P, Q);    /* print final totals */

/* free space : do not change order of next 3 lines! */

   lrs_clear_mp_vector (output, Q->n);
   lrs_free_dic (P,Q);           /* deallocate lrs_dic */
   lrs_free_dat (Q);             /* deallocate lrs_dat */

  }    /* end of loop for i= ...  */
 
 lrs_close ("vedemo:");
 printf("\n");
 return 0;
}  /* end of main */

void
makecube (lrs_dic *P, lrs_dat *Q)
/* generate H-representation of a unit hypercube */
{
long num[MAXCOL];
long den[MAXCOL];
long row, j;
long m=Q->m;       /* number of inequalities      */
long n=Q->n;       /* hypercube has dimension n-1 */

for (row=1;row<=m;row++)
     {
      for(j=0;j<n;j++)
          { num [j] = 0;
            den [j] = 1;
          }
       if (row < n)
          { num[0] = 1;
            num[row] = -1;
          }
       else
          { num[0] = 0;
            num[row+1-n] = 1;
          }
       lrs_set_row(P,Q,row,num,den,GE);
     }

}
