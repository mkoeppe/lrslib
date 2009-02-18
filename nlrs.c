// nlrs.c      v1.0  Jan 15, 2009
 
// Supplied by Conor Meagher to run lrs simultaneously on n processors for n input files
// all  procs are terminated when first one terminates. 
// output in file: out
// (see also 2nash.c)

// usage:  nlrs file1 file2 file3 (etc)

#include <sys/wait.h>
       #include <stdlib.h>
       #include <unistd.h>
       #include <stdio.h>

       int main(int argc, char *argv[])
	       {
                  pid_t cpid[argc - 1], w;
		  char buffer [250];	
                  int status,l,j;
		  for(l = 1; l < argc; l++) {  
	              cpid[l -1] = fork();
	              if (cpid[l -1] == -1) {
		          perror("fork");
		          exit(EXIT_FAILURE);
		      }
		      if(cpid[l-1] == 0) {
			 //forked threads
			 int n = sprintf(buffer, "lrs %s > out%i", argv[l], l);
			 int i = system(buffer);
                          _exit(0);
		      }
		  }
		  // main thread
		  w = wait(&status);
		  for(j = 1; j < argc; j++) {
		      if(w == cpid[j-1]) {
			  // this child finished first
			  printf("lrs %s finished first\n", argv[j]);
			  int n = sprintf(buffer, "/bin/mv -f out%i out", j);
			  int i = system(buffer);
		      } else {
			  printf("terminating lrs of file %s\n", argv[j]);
			  int n = sprintf(buffer, "/bin/rm -f out%i", j);
			  int i = system(buffer);
		      }
		  }
		  kill(0,9);

exit(EXIT_SUCCESS);
}
