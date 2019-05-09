/* mplrs.c: initial release of MPI version 
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Author: Charles Jordan skip@ist.hokudai.ac.jp
Based on plrs.cpp by Gary Roumanis
Initial lrs Author: David Avis avis@cs.mcgill.ca
 */

/* #include "lrslib.h" */ /* included in mplrs.h */

#include "mplrs.h"

#include <mpi.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

/* global variables */
mplrsv mplrs;       /* state of this process */
masterv master;     /* state of the master   */
consumerv consumer; /* state of the consumer */

/******************
 * initialization *
 ******************/

int main(int argc, char **argv)
{

	mplrs_init(argc, argv);

	mprintf2(("%d: initialized on %s\n",mplrs.rank,mplrs.host));

	if (mplrs.size<3)
		return mplrs_fallback();

	if (mplrs.rank == MASTER)
		return mplrs_master();
	else if (mplrs.rank == CONSUMER)
		return mplrs_consumer();
	else
		return mplrs_worker();
}

void mplrs_init(int argc, char **argv)
{
	int i,j;
	int count;
	char c;
	time_t curt = time(NULL);
	char *tim, *tim1;

	/* make timestamp for filenames */
	tim = ctime(&curt);
	tim1 = tim+4;
	tim1[3] = tim1[6] = '_';
	tim1[15]='\0';

	/* start MPI */
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &mplrs.rank);
	MPI_Comm_size(MPI_COMM_WORLD, &mplrs.size);
	MPI_Get_processor_name(mplrs.host, &count);

	/* allocate mp for volume calculation, from plrs */
	lrs_alloc_mp(mplrs.tN);   lrs_alloc_mp(mplrs.tD); 
	lrs_alloc_mp(mplrs.Vnum); lrs_alloc_mp(mplrs.Vden);
	lrs_alloc_mp(mplrs.Tnum); lrs_alloc_mp(mplrs.Tden);
	itomp(ZERO, mplrs.Vnum);  itomp(ONE, mplrs.Vden);

	gettimeofday(&mplrs.start, NULL);

	mplrs_initstrucs(); /* initialize default values of globals */

	/* process commandline arguments, set input and output files */
	mplrs_commandline(argc, argv);

	if (mplrs.rank == MASTER || mplrs.rank == CONSUMER)
	{
		/* open input file for reading on master, open output file for
	 	 * writing on consumer, open histogram file for writing on
	 	 * master -- if these files are to be used.
	 	 */
		mplrs_initfiles();
		if (mplrs.rank == MASTER)
			master_sendfile();
	}
	else
	{
		/* receive input file from master */
		MPI_Recv(&count, 1, MPI_INT, 0, 20, MPI_COMM_WORLD, 
			 MPI_STATUS_IGNORE);
		mplrs.input = (char *)malloc(sizeof(char)*(count+1));
		MPI_Recv(mplrs.input, count, MPI_CHAR, 0, 20, 
			 MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		mplrs.input[count] = '\0';

		/* get number of chars needed for worker files */
		j = mplrs.size;
		for (i=1; j>9; i++)
			j = j/10;
		i += 6+strlen(tim1); /* mplrs_TIMESTAMP */
		i += strlen(mplrs.tfn_prefix) + strlen(mplrs.input_filename) + 
		     i + 6; /* _%d.ine\0 */
		mplrs.tfn = (char *)malloc(sizeof(char) * i);
		sprintf(mplrs.tfn, "%smplrs_%s%s_%d.ine", 
						  mplrs.tfn_prefix, tim1, 
						  mplrs.input_filename,
					 	  mplrs.rank);
		/* flatten directory structure in mplrs.input_filename
		 * for mplrs.tfn, to prevent writing to non-existent
		 * subdirectories in e.g. /tmp
		 */
		i = strlen(mplrs.tfn_prefix) + 6 + strlen(tim1);
		j = strlen(mplrs.tfn);
		for (; i<j; i++)
		{
			c = mplrs.tfn[i];
			if (c == '/' || c == '\\')
				mplrs.tfn[i] = '_';
		}
		
	}

	/* setup signals --
         * TERM checkpoint to checkpoint file, or output file if none & exit
	 * HUP ditto
         */
	signal(SIGTERM, mplrs_caughtsig);
	signal(SIGHUP, mplrs_caughtsig);
}

/* if we catch a signal, set a flag to exit */
void mplrs_caughtsig(int sig)
{
	if (mplrs.caughtsig<2)
		mplrs.caughtsig = 1;
	signal(sig, mplrs_caughtsig);
	return;
}

/* if we've caught a signal, tell the master about it */
void mplrs_handlesigs(void)
{
	float junk = 0; /* 0: caught a signal */
	if (mplrs.caughtsig != 1)
		return;
	/* we want to stop, so no need to hurry and queue the send */
	MPI_Send(&junk, 1, MPI_FLOAT, MASTER, 9, MPI_COMM_WORLD);
	mplrs.caughtsig = 2; /* handled */
	return;
}

/* signal the master that we want to stop,
 * can try to checkpoint.  Use mplrs to give
 * output if desired.
 * Does not return to caller ... ugly
 */
void mplrs_cleanstop(int checkpoint)
{
	MPI_Request req = MPI_REQUEST_NULL;
	char *cobasis = NULL;
	float junk;
	int header[8]={0};
	int flag = 0, len;
	unsigned long ncob = 0;
	mprintf(("%d: in cleanstop\n", mplrs.rank));
	if (checkpoint)
		junk = 1;
	else
		junk = -1;
	/* inform master, wait for reply */
	MPI_Send(&junk, 1, MPI_FLOAT, MASTER, 9, MPI_COMM_WORLD);
	MPI_Recv(&junk, 1, MPI_FLOAT,MASTER,9,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
	/* needs to do usual cleanup / exit of a worker. callstack is
	 * unpredictable though, so exit() and don't return.
	 */
	/* leaks: starting_cobasis (mplrs_worker)
	 */
	/* do_work cleanup*/
	remove(mplrs.tfn); /* delete temporary file */
	/* mplrs_worker cleanup */
	mplrs.outputblock = 0;
	process_output();
	return_unfinished_cobases();
	clean_outgoing_buffers();
	/* mplrs_worker start of loop ... */
	MPI_Isend(&ncob, 1, MPI_UNSIGNED, MASTER, 6, MPI_COMM_WORLD, &req);
	while (1)
	{
		MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
		if (flag)
		{
			mprintf3(("%d: clean_stop incoming message from 0\n",
				  mplrs.rank));
			MPI_Recv(header, 8, MPI_INT, MASTER, MPI_ANY_TAG,
				 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			if (header[0] == -1) /* no more work to do */
			{
				mplrs_worker_finished();
				exit(0);
			}
			else /* forget this cobasis */
			{
				len = header[0];
				cobasis = (char *)malloc(sizeof(char)*(len+1));
				MPI_Recv(cobasis, len, MPI_CHAR, MASTER,
					 MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				cobasis[len] = '\0';
				mprintf3(("%d: got %s, throwing away\n", 
					   mplrs.rank, cobasis));
				free(cobasis);
				return_unfinished_cobases();
				clean_outgoing_buffers();
				MPI_Isend(&ncob, 1, MPI_UNSIGNED, MASTER, 6, 
					  MPI_COMM_WORLD, &req);
			}
		}
		clean_outgoing_buffers();
	}

	/* mplrs_worker */
	
}

/* terminate immediately and ungracefully after printing the string */
void mplrs_emergencystop(const char *msg)
{
	int ret;
	fprintf(stderr, "%s",msg);
	ret = MPI_Abort(MPI_COMM_WORLD, 1);
	exit(ret);
}

/* send the contents of the input file to all workers */
void master_sendfile(void)
{
	char *buf;
	int count=0;
	int c, i;

	c = fgetc(master.input);

	while (c!=EOF)
	{
		count++;
		c = fgetc(master.input);
	}

	buf = (char *)malloc(sizeof(char)*(count));

	fseek(master.input, 0, SEEK_SET);
	
	for (i=0; i<count; i++)
	{
		c = fgetc(master.input);
		buf[i]=c;
	}

	for (i=0; i<mplrs.size; i++)
	{
		if (i==MASTER || i==CONSUMER)
			continue;
		mprintf2(("M: Sending input file to %d\n", i));
		MPI_Send(&count, 1, MPI_INT, i, 20, MPI_COMM_WORLD);
		MPI_Send(buf, count, MPI_CHAR, i, 20, MPI_COMM_WORLD);
	}
	fseek(master.input, 0, SEEK_SET);
	free(buf);
}

/* initialize default values of global structures, before commandline */
void mplrs_initstrucs(void)
{
	mplrs.cobasis_list = NULL;

	mplrs.caughtsig = 0;
	mplrs.overflow = 0;
	mplrs.rays = 0;
	mplrs.vertices = 0;
	mplrs.bases = 0;
	mplrs.facets = 0;
	mplrs.linearities = 0;
	mplrs.intvertices = 0;
	mplrs.deepest = 0;

	mplrs.my_tag = 100;
	mplrs.tfn_prefix = DEF_TEMP;
	mplrs.tfn = NULL;
	mplrs.tfile = NULL;
	mplrs.input_filename = DEF_INPUT;
	mplrs.input = NULL;
	mplrs.output_list = NULL;
	mplrs.ol_tail = NULL;
	mplrs.outnum = 0;
	mplrs.maxbuf = DEF_MAXBUF; /* maximum # lines to buffer */
	mplrs.outputblock = 0; /* don't block initial output */
	mplrs.countonly = 0;

	master.cobasis_list = NULL;
	master.size_L = 0;
	master.tot_L = 0;
	master.num_empty = 0;
	master.num_producers = 0;
	master.checkpointing = 0;
	master.cleanstop = 0;
	master.lmin = DEF_LMIN;
	master.lmax = DEF_LMAX;
	master.scalec = DEF_SCALEC;
	master.initdepth = DEF_ID;
	master.maxdepth = DEF_MAXD;
	master.maxcobases = DEF_MAXC;
	master.maxncob = DEF_MAXNCOB;
	master.lponly = 0;
	master.time_limit = 0;
	master.hist_filename = DEF_HIST;
	master.hist = NULL;
	master.doing_histogram = 0;
	master.freq_filename = DEF_FREQ;
	master.freq = NULL;
	master.restart_filename = DEF_RESTART;
	master.restart = NULL;
	master.checkp_filename = DEF_CHECKP;
	master.checkp = NULL;
	master.stop_filename = DEF_STOP;
	master.stop = NULL;
	master.input = NULL;

	consumer.prodreq = NULL;
	consumer.prodibf = NULL;
	consumer.incoming = NULL;
	consumer.output_filename = DEF_OUTPUT;
	consumer.output = stdout;
	consumer.oflow_flag = 0;
	consumer.num_producers = 0;
	consumer.waiting_initial = 1;
	consumer.final_print = 1;
}

/* process commandline arguments */
void mplrs_commandline(int argc, char **argv)
{
	int i, arg, firstfile=1;
	for (i=1; i<argc; i++)
	{
		if (!strcmp(argv[i], "-lmin"))
		{
			arg = atoi(argv[i+1]);
			i++;
			if (arg < 1  )
				bad_args();
			master.lmin = arg;
                        if (master.lmin > master.lmax)
                           master.lmax = arg;
			continue;
		}
		else if (!strcmp(argv[i], "-lmax"))
		{
			arg = atoi(argv[i+1]);
			i++;
			if (arg<1 )
				bad_args();
			master.lmax = arg;
                        if (master.lmin > master.lmax)
                           master.lmin = arg;
			continue;
		}
		else if (!strcmp(argv[i], "-scale"))
		{
			arg = atoi(argv[i+1]);
			i++;
			if (arg<1)
				bad_args();
			master.scalec = arg;
			continue;
		}
		else if (!strcmp(argv[i], "-hist"))
		{
			master.hist_filename = argv[i+1];
			i++;
			continue;
		}
		else if (!strcmp(argv[i], "-freq"))
		{
			master.freq_filename = argv[i+1];
			i++;
			continue;
		}
		else if (!strcmp(argv[i], "-stopafter"))
		{
			arg = atoi(argv[i+1]);
			i++;
			if (arg<1)
				bad_args();
			master.maxncob = arg;
			continue;
		}
		else if (!strcmp(argv[i], "-countonly"))
		{
			mplrs.countonly = 1;
			continue;
		}
		else if (!strcmp(argv[i], "-maxbuf"))
		{
			arg = atoi(argv[i+1]);
			i++;
			if (arg<1)	
				bad_args();
			mplrs.maxbuf = arg;
			continue;
		}
		else if (!strcmp(argv[i], "-id"))
		{
			arg = atoi(argv[i+1]);
			i++;
			if (arg<0)
				bad_args();
			master.initdepth = arg;
			continue;
		}
		else if (!strcmp(argv[i], "-maxd"))
		{
			arg = atoi(argv[i+1]);
			i++;
			if (arg<1)
				bad_args();
			master.maxdepth = arg;
			continue;
		}
		else if (!strcmp(argv[i], "-maxc"))
		{
			arg = atoi(argv[i+1]);
			i++;
			if (arg<0)
				bad_args();
			master.maxcobases = arg;
			continue;
		}
		else if (!strcmp(argv[i], "-checkp"))
		{
			master.checkp_filename = argv[i+1];
			i++;
			continue;
		}
		else if (!strcmp(argv[i], "-lponly"))
		{
			master.lponly = 1;
			continue;
		}
		else if (!strcmp(argv[i], "-stop"))
		{
			master.stop_filename = argv[i+1];
			i++;
			continue;
		}
		else if (!strcmp(argv[i], "-time"))
		{
			arg = atoi(argv[i+1]);
			i++;
			if (arg<1)
				bad_args();
			master.time_limit = arg;
			continue;
		}
		else if (!strcmp(argv[i], "-restart"))
		{
			master.restart_filename = argv[i+1];
			i++;
			continue;
		}
		else if (!strcmp(argv[i], "-temp"))
		{
			mplrs.tfn_prefix = argv[i+1];
			i++;
			continue;
		}
		else if (firstfile == 1)
		{
			mplrs.input_filename = argv[i];
			firstfile = 2;
		}
		else if (firstfile == 2)
		{
			consumer.output_filename = argv[i];
			firstfile = 3;
		}
		else
			bad_args();
	}
	if (master.lmax == -1)
		master.lmax = (master.lmin>0? master.lmin: 0);
	if (mplrs.input_filename==NULL) /* need an input file */
		bad_args();
	if ((master.stop_filename!=NULL || master.time_limit!=0)  && 
	    master.checkp_filename==NULL)
		bad_args(); /* need checkpoint file if stop condition given */
}

/* open input file on master, histogram (if exists) on master, 
 * output (if exists) on consumer
 */
void mplrs_initfiles(void)
{
	if (mplrs.rank == MASTER)
	{
		master.input = fopen(mplrs.input_filename, "r");
		if (master.input == NULL)
		{
			printf("Unable to open %s for reading [%s].\n",
				mplrs.input_filename, mplrs.host);
			/* MPI_Finalize(); */
			exit(0);
		}
		if (master.hist_filename != NULL)
		{
			master.hist = fopen(master.hist_filename, "w");
			if (master.hist == NULL)
			{
				printf("Unable to open %s for writing [%s].\n",
				       master.hist_filename, mplrs.host);
				/* MPI_Finalize(); */
				exit(0);
			}
			master.doing_histogram = 1;
			mprintf2(("M: Prepared histogram (%s)\n", master.hist_filename));
		}
		if (master.freq_filename != NULL)
		{
			master.freq = fopen(master.freq_filename, "w");
			if (master.freq == NULL)
			{
				printf("Unable to open %s for writing [%s].\n",
					master.freq_filename, mplrs.host);
				exit(0);
			}
			mprintf2(("M: Prepared frequency file (%s)\n",
				 master.freq_filename));
		}
		if (master.checkp_filename !=NULL)
		{
			master.checkp = fopen(master.checkp_filename, "w");
			if (master.checkp == NULL)
			{
				printf("Unable to open %s for writing [%s].\n",
					master.checkp_filename, mplrs.host);
				/* MPI_Finalize(); */
				exit(0);
			}
			mprintf2(("M: Prepared checkpoint file (%s)\n",
				  master.checkp_filename));
		}
		if (master.restart_filename != NULL)
		{
			master.restart = fopen(master.restart_filename, "r");
			if (master.restart == NULL)
			{
				printf("Unable to open %s for reading [%s].\n",
				       master.restart_filename, mplrs.host);
				/* MPI_Finalize(); */
				exit(0);
			}
			mprintf2(("M: Opened restart file (%s)\n",
				  master.restart_filename));
		}
	}
	if (mplrs.rank == CONSUMER)
	{
		if (consumer.output_filename == NULL)
			return;
		consumer.output = fopen(consumer.output_filename, "w");
		if (consumer.output == NULL)
		{
			printf("Unable to open %s for writing [%s].\n",
			       consumer.output_filename, mplrs.host);
			/* MPI_Finalize(); */
			exit(0);
		}
	}
}


/* Bad commandline arguments. Complain and die. */
void bad_args(void)
{
	if (mplrs.rank == CONSUMER)
		printf("Invalid arguments.\n%s\n", USAGE);
	MPI_Finalize();
	exit(0);
}

/* fallback -- meaningless if <3 processes
 * better would be to fallback to normal lrs if <4 processes
 */
int mplrs_fallback(void)
{
	if (mplrs.rank==0)
		printf("mplrs requires at least 3 processes.\n");
	MPI_Finalize();
	exit(0);
	return 0; /* unreachable */
}

/**********		
 * master *
 **********/

int mplrs_master(void)
{
	int i;
	int loopiter = 0;  /* do some things only sometimes */
	MPI_Request ign = MPI_REQUEST_NULL;   /* a request we'll ignore */
	struct timeval cur, last; /* for histograms */
	int flag = 0;;
	int phase = 1;     /* In phase1? */
	int done = -1;     /* need a negative int to send to finished workers */
	int ncob=0; /* for printing sizes of sub-problems and for -stopafter*/
	unsigned long tot_ncob = 0;
	int want_stop = 0;
	int check = 0; /* inform consumer whether checkpointing */

	gettimeofday(&last, NULL);

	master.num_producers = 0; /* nobody working right now */
	master.act_producers = (unsigned int *)malloc(sizeof(unsigned int)*mplrs.size);
	master.live_workers = mplrs.size - 2;
	master.workin = (int *)malloc(sizeof(int)*mplrs.size);
	master.mworkers = (MPI_Request *)malloc(sizeof(MPI_Request)*mplrs.size);
	master.incoming = NULL;
	master.sigbuf = (float*)malloc(sizeof(float)*mplrs.size);
	master.sigcheck = (MPI_Request *)malloc(sizeof(MPI_Request)*mplrs.size);

	if (master.restart!=NULL)
	{
		master_restart();
		phase = 0;
	}

	for (i=0; i<mplrs.size; i++)
	{
		master.act_producers[i] = 0;
		if (i==MASTER)
			continue;
		MPI_Irecv(master.sigbuf+i, 1, MPI_FLOAT, i, 9, MPI_COMM_WORLD,
			  master.sigcheck+i);
		if (i==CONSUMER)
			continue;
		MPI_Irecv(master.workin+i, 1, MPI_UNSIGNED, i, 6, MPI_COMM_WORLD,
			  master.mworkers+i);
	}

	while ((master.cobasis_list!=NULL && !master.checkpointing && !want_stop) || master.num_producers>0 || master.live_workers>0)
	{
		loopiter++;
		/* sometimes check if we should update histogram etc */
		if (!(loopiter&0x1ff))
		{
			if (master.maxncob>0 && master.maxncob<=tot_ncob)
				want_stop = 1;
			if (master.doing_histogram)
				print_histogram(&cur, &last);
		}
		/* sometimes check if we should checkpoint */
		if (!(loopiter&0x7ff))
 // && !master.checkpointing && !master.cleanstop)
		{
			if (master.stop_filename!=NULL || master.time_limit!=0)
				check_stop();
			master_checksigs();
			if (master.cleanstop)
				want_stop = 1;
		}
			
		recv_producer_lists();

		/* check if anyone wants work */
		for (i=0; i<mplrs.size; i++)
		{	/* consumer and master don't want work */
			if (i==CONSUMER || i==MASTER)
				continue;
			/* workers that have exited can't work */
			if (master.mworkers[i]==MPI_REQUEST_NULL)
				continue;
			if (master.num_producers>0 && master.cobasis_list==NULL)
			{
				break; /* no work to give now, but some may
					* appear later
					*/
			}

			if (phase==1 && i!=INITIAL)
				continue; /* INITIAL gets the first bit */
			MPI_Test(master.mworkers+i, &flag, MPI_STATUS_IGNORE);
			if (!flag)
				continue; /* i is not ready for more work */
			
			ncob = master.workin[i];
			tot_ncob+=ncob;
			mprintf2(("M: %d looking for work\n", i));
			if ((master.cobasis_list!=NULL || phase==1) && 
			    !master.checkpointing && !want_stop)
			{ /* and not checkpointing! */
				send_work(i,phase);
				MPI_Irecv(master.workin+i, 1, MPI_UNSIGNED,i, 6,
					  MPI_COMM_WORLD, master.mworkers+i);
				phase=0;
				if (master.freq!=NULL && ncob>0)
					fprintf(master.freq, "%d\n", ncob);
				continue;
			}
			/* else tell worker we've finished */
			mprintf(("M: Saying goodbye to %d, %d left\n", i,
				 master.live_workers-1));
			MPI_Isend(&done, 1, MPI_INT, i, 8, MPI_COMM_WORLD,&ign);
			MPI_Request_free(&ign);
			master.live_workers--;
			if (master.freq!=NULL && ncob>0)
				fprintf(master.freq, "%d\n", ncob);
		}

		clean_outgoing_buffers();
	}

	/* don't checkpoint if we actually finished the run */
	if (master.checkpointing && master.cobasis_list==NULL)
		master.checkpointing = 0;

	if (master.checkpointing)  /* checkpointing */
		check = CHECKFLAG;
	/* inform consumer if checkpointing or not */
	mprintf2(("M: Telling consumer whether or not checkpointing (%d)\n", check));
	MPI_Send(&check, 1, MPI_INT, CONSUMER, 1, MPI_COMM_WORLD);

	if (master.checkpointing)
		master_checkpoint();

	send_master_stats();
	MPI_Finalize();
	free(master.workin);
	free(master.mworkers);
	free(master.act_producers);
	free(master.sigbuf);
	free(master.sigcheck);
	return 0;
}

/* prepare to receive remaining cobases from target.
 * Since we don't yet know the size of buffers needed, we
 * only Irecv the header and will Irecv the actual cobases later
 */
void master_add_incoming(int target)
{
	msgbuf *msg = (msgbuf *)malloc(sizeof(msgbuf));
	msg->req = (MPI_Request *)malloc(sizeof(MPI_Request)*3);
	msg->buf = (void **)malloc(sizeof(void *)*3);
	msg->buf[0] = (int *)malloc(sizeof(int) * 3); /* (strlen,lengths,tag) */
	msg->buf[1] = NULL; /* sizes not known yet */
	msg->buf[2] = NULL;
	msg->count = 3;
	msg->target = target;
	msg->queue = 1;
	msg->tags = NULL;
	msg->sizes = NULL;
	msg->types = NULL;
	msg->next = master.incoming;
	master.incoming = msg;
	MPI_Irecv(msg->buf[0], 3, MPI_INT, target, 10, MPI_COMM_WORLD,msg->req);	return;
}

/* check our list of incoming messages from producers about cobases
 * to add to L.  Add any from messages that have completed.
 * If the header has completed (msg->queue==1 and header completed),
 * add the remaining MPI_Irecv's. 
 * Update num_producers to keep track of how many messages the master
 * is owed (workers are not allowed to exit until num_producers==0 and
 * L is empty).
 * Also update size_L
 */
void recv_producer_lists(void)
{
	msgbuf *msg, *prev=NULL, *next;
	int *header;
	int flag;

	for (msg = master.incoming; msg; msg=next)
	{
		next = msg->next;
		MPI_Test(msg->req, &flag, MPI_STATUS_IGNORE);
		if (!flag) /* header has not completed yet */
		{
			prev = msg;
			continue;
		}
		header = (int *)msg->buf[0];
		if (msg->queue) /* header completed, and need to Irecv now */
		{
			if (header[0]==-1) /* producer returns NOTHING */
			{
				master.num_producers--;
				master.act_producers[msg->target]--;
				free_msgbuf(msg);
				if (prev)
					prev->next = next;
				else
					master.incoming = next;
				continue;
			}
			msg->buf[1]= (char*)malloc(sizeof(char)*header[1]);
			msg->buf[2]= (int *)malloc(sizeof(int)*header[0]);
			MPI_Irecv(msg->buf[1], header[1], MPI_CHAR, msg->target,
				  header[2], MPI_COMM_WORLD, msg->req+1);
			MPI_Irecv(msg->buf[2], header[0], MPI_INT, msg->target,
				  header[2], MPI_COMM_WORLD, msg->req+2);
			msg->queue=0;
			prev = msg;
			continue;
		}
		/* header completed, did the rest? */
		MPI_Testall(2, msg->req+1, &flag, MPI_STATUSES_IGNORE);
		if (!flag) /* not yet */
		{
			prev = msg;
			continue;
		}

		mprintf2(("M: %d returned non-empty producer list (%d, %d)\n",
			  msg->target, header[0], header[1]));
		process_returned_cobases(msg);

		mprintf2(("M: Now have size_L=%lu\n",master.size_L));

		if (prev)
			prev->next = next;
		else
			master.incoming = next;
		master.num_producers--;
		master.act_producers[msg->target]--;
		free_msgbuf(msg);
	}
	return;
}

/* msg is a completed, non-empty buffer containing cobases to add to
 * L.  Process it, add them to L, and update size_L
 * Basically the inverse of return_unfinished_cobases()
 */
void process_returned_cobases(msgbuf *msg)
{
	int *header = (int *)msg->buf[0];
	char *str = (char *)msg->buf[1];
	int *lengths = (int *)msg->buf[2];
	int i;
	char *cob;

	for (i=0; i<header[0]; i++)
	{
		cob = (char *)malloc(sizeof(char)*(lengths[i]+1));
		strncpy(cob, str, lengths[i]);
		cob[lengths[i]] = '\0';
		str+=lengths[i];
		mprintf2(("M: Adding to L: %s\n",cob));
		master.cobasis_list = addlist(master.cobasis_list, cob);
	}
	master.size_L += header[0];
	master.tot_L += header[0];

	return;
}
	
/* send one unit from L to target.  if phase!=0, this is the first
 * unit we're sending (i.e. phase 1).  usually, phase==0.
 * Parameters are scaled and sent in the header.
 */
void send_work(int target, int phase)
{
	slist *cob;
	msgbuf *msg = (msgbuf *)malloc(sizeof(msgbuf));
	int *header;
	msg->req = (MPI_Request *)malloc(sizeof(MPI_Request)*2);
	msg->buf = (void **)malloc(sizeof(void *)*2);
	/*{length of work string, int maxdepth, int maxcobases, bool lponly,
	   4x future use} */
	msg->buf[0] = (int *)malloc(sizeof(int) * 8);
	header = (int *)msg->buf[0];

	if (phase==0)	/* normal */
	{
		cob = master.cobasis_list;
		master.cobasis_list = cob->next;
		header[0] = strlen((char *)cob->data);
		msg->buf[1] = cob->data;
		setparams(header); /* scale if needed */
		master.size_L--;
		if (master.size_L == 0)
			master.num_empty++;
		mprintf(("M: Sending work to %d (%d,%d,%d) %s\n",
			target, header[0], header[1], header[2], 
			(char*)msg->buf[1]));
		msg->count = 2;
		free(cob);
	}
	else		/* phase 1 */
	{
		header[0] = 0; /* header[0]==0 means initial phase 1 */
		header[1] = master.initdepth;
		header[2] = master.maxcobases;
		mprintf(("M: Sending phase 1 to %d (%d,%d)\n", target,
			 header[1], header[2]));
		msg->buf[1] = NULL;
		msg->count = 1;
	}
	msg->target = target;
	msg->queue = 0;
	msg->tags = NULL;
	msg->sizes = NULL;
	msg->types = NULL;

	/* ready to send */
	MPI_Isend(header, 8, MPI_INT, target, 1, MPI_COMM_WORLD, msg->req);
	if (phase==0)
		MPI_Isend(msg->buf[1], header[0], MPI_CHAR, target, 1,
			  MPI_COMM_WORLD, msg->req+1);
	master_add_incoming(target); /* prepare to receive remaining cobases */

	msg->next = mplrs.outgoing;
	mplrs.outgoing = msg;

	master.act_producers[target]++;
	master.num_producers++;

	return;
}

/* header is a work header (length, maxd, maxc) not yet set. 
 * Set the parameters (maxd, maxc) as desired.
 */
void setparams(int *header)
{
	/* if L is too small, use maxdepth */
	if (master.lmin>0 && (master.size_L < mplrs.size*master.lmin))
		header[1] = master.maxdepth;
	else /* don't have too small L, so no maxdepth */
		header[1] = 0;
	header[2] = master.maxcobases;
	if (master.lmax>0 && (master.size_L > mplrs.size * master.lmax))
		header[2] = header[2] * master.scalec;
	header[3] = master.lponly;
}

/* check if we want to stop now.
 * if master.stop_filename exists or time limit exceeded, 
 * set master.checkpointing = 1.
 * this is not an immediate stop -- we wait for current workers to
 * complete the tasks they've been assigned, stopping after that.
 */
void check_stop(void)
{
	struct timeval cur;
	int flag = 0;

	if (master.checkpointing || master.cleanstop)
		return; /* already stopping */
	if (master.stop_filename)
	{
		mprintf2(("M: checking stop file %s\n", master.stop_filename));
		master.stop = fopen(master.stop_filename, "r");
		if (master.stop!=NULL)
		{
			flag=1;
			fclose(master.stop);
		}
	}

	if (master.time_limit!=0)
	{
		mprintf2(("M: checking if time exceeded\n"));
		gettimeofday(&cur, NULL);
		if (cur.tv_sec - mplrs.start.tv_sec > master.time_limit)
			flag=1;
	}
	if (flag!=0)
	{
		mprintf(("M: Stop condition detected, checkpointing!\n"));
		master.checkpointing = 1;
	}
}

/* check if we've caught a signal, or we've received a message from someone
 * that has.  If so, we want to checkpoint like above
 * The similar mplrs_cleanstop also uses this...
 */
void master_checksigs(void)
{
	int i, flag, size=mplrs.size;
	int check[3] = {CHECKFLAG,0,0};
	float junk=0;
	MPI_Request ign;
	int already_stopping = master.checkpointing || master.cleanstop;
	if (mplrs.caughtsig == 1 && !already_stopping)
	{
		mprintf(("M: I caught signal, checkpointing!\n"));
		/* this master.checkpointing must be inside a !already_stopping
		 * condition, see comment below
		 */
		master.checkpointing = 1;
		already_stopping = 1;
	}
	for (i=1; i<size; i++)
	{
		MPI_Test(master.sigcheck+i, &flag, MPI_STATUS_IGNORE);
		if (flag && master.sigbuf[i]==0 && !already_stopping)
		{
			mprintf(("M: %d caught signal, checkpointing!\n",i));
			/* this master.checkpointing must also be inside a
			 * !already_stopping condition, see comment below
			 */
			master.checkpointing = 1;
			already_stopping = 1;
		}
		else if (flag && master.sigbuf[i]==-1)
		{
			mprintf(("M: %d wants cleanstop, no checkpoint\n",i));
			/* still needed, to tell consumer if overflow occurs
			 * in initial job
			 */
			check[0] = STOPFLAG;
			MPI_Isend(&junk, 1, MPI_FLOAT, i,9,MPI_COMM_WORLD,&ign);
			if (!already_stopping)
				MPI_Isend(check, 3, MPI_INT, CONSUMER, 7, 
					  MPI_COMM_WORLD, &ign);
			master.cleanstop = 1;
			already_stopping = 1;
			/* disable a checkpoint in case we notice overflow
			 * after deciding to checkpoint but before workers
			 * return.  in that case we can't guarantee all
			 * output/unfinished cobases are produced so avoid
			 * chance of making an incorrect checkpoint
			 */
			/* note this is not in an !already_stopping condition*/
			master.checkpointing = 0;
		}
		else if (flag && master.sigbuf[i]==1)
		{
			mprintf(("M: %d wants cleanstop, checkpointing!\n",i));
			master.cleanstop = 1;
			/* don't produce a checkpoint if overflow has
			 * already occurred, even if a checkpoint is
			 * requested after overflow but before shutdown,
			 * as in comment above
			 */
			if (!already_stopping)
				master.checkpointing = 1;
			already_stopping = 1;
		}
	}
}

/* we're checkpointing, receive stats from consumer and output checkpoint file
 */
/* two cases: we have a file to checkpoint to (normal)
 * or, we caught a signal and want to checkpoint but have no checkpoint file
 * in this case we append to the output file via the consumer, with
 * comments on where to cut
 */
/* mplrs1 format: counts are 'unsigned int' (%d unfortunately)
 * mplrs2 format: counts are 'unsigned long' (%lu)
 * so mplrs1 files can be read by mplrs2 readers not necc. reverse
 */
void master_checkpoint(void)
{
	int fin = -1;
	mprintf(("M: making checkpoint file\n"));
	recv_counting_stats(CONSUMER);
	if (master.checkp_filename != NULL)
	{
		MPI_Send(&fin, 1, MPI_INT, CONSUMER, 1, MPI_COMM_WORLD);
		master_checkpointfile();
		return;
	}
	else
	{
		fin = 1;
		MPI_Send(&fin, 1, MPI_INT, CONSUMER, 1, MPI_COMM_WORLD);
		master_checkpointconsumer();
		return;
	}
}

/* note: master_checkpointconsumer and master_checkpointfile should
 * produce the *same* format. First two lines of checkpointconsumer
 * done by the consumer directly for now.
 */
void master_checkpointconsumer(void)
{
	int len;
	char *str;
	slist *list, *next;
	for (list=master.cobasis_list; list; list=next)
	{
		next = list->next;
		str = (char*)list->data;
		len = strlen(str)+1; /* include \0 */
		MPI_Send(&len, 1, MPI_INT, CONSUMER, 1, MPI_COMM_WORLD);
		MPI_Send(str, len, MPI_CHAR, CONSUMER, 1, MPI_COMM_WORLD);
		free(str);
		free(list);
	}
	len = -1;
	MPI_Send(&len, 1, MPI_INT, CONSUMER, 1, MPI_COMM_WORLD);
}
void master_checkpointfile(void)
{
	slist *list, *next;
	char *vol = cprat("", mplrs.Vnum, mplrs.Vden);
	fprintf(master.checkp, "mplrs4\n%llu %llu %llu %llu %llu\n%s\n%llu\n", 
		mplrs.rays, mplrs.vertices, mplrs.bases, mplrs.facets,
		mplrs.intvertices,vol,mplrs.deepest);
	free(vol);
	for (list=master.cobasis_list; list; list=next)
	{
		next = list->next;
		fprintf(master.checkp, "%s\n", (char *)list->data);
		free(list->data);
		free(list);
		master.size_L--;
	}
	fclose(master.checkp);
	return;
}

/* we want to restart. load L and counting stats from restart file,
 * send counting stats to consumer (after notifying consumer of restart)
 */
void master_restart(void)
{
	char *line=NULL;
	char *vol=NULL;
	size_t size=0, vsize=0;
	ssize_t len=0;
	int restart[3] = {RESTARTFLAG,0,0};
	int ver;

	/* check 'mplrs1' header */
	len = getline(&line, &size, master.restart);
	if (len!=7 || (strcmp("mplrs1\n",line) && strcmp("mplrs2\n",line) && 
		       strcmp("mplrs3\n",line) && strcmp("mplrs4\n",line)))
	{
		printf("Unknown checkpoint format\n");
		/* MPI_Finalize(); */
		exit(0);
	}

	mprintf2(("M: found checkpoint header\n"));
	sscanf(line,"mplrs%d\n",&ver);

	/* get counting stats */
	fscanf(master.restart, "%llu %llu %llu %llu %llu\n",
	       &mplrs.rays, &mplrs.vertices, &mplrs.bases, &mplrs.facets,
	       &mplrs.intvertices);
	if (ver<3) /* volume added in mplrs3 */
		printf("*Old checkpoint file, volume may be incorrect\n");
	else /* get volume */
	{
		len = getline(&vol, &vsize, master.restart);
		if (len<=1)
		{
			printf("Broken checkpoint file\n");
			exit(0);
		}
		vol[len-1] = '\0'; /* remove '\n' */
		plrs_readrat(mplrs.Tnum, mplrs.Tden, vol);
		copy(mplrs.tN, mplrs.Vnum); copy(mplrs.tD, mplrs.Vden);
		linrat(mplrs.tN, mplrs.tD, 1L, mplrs.Tnum, mplrs.Tden,
		       1L, mplrs.Vnum, mplrs.Vden);
		free(vol);
	}
	if (ver<4) /* tree depth added in mplrs4 */
		printf("*Old checkpoint file, tree depth may be incorrect\n");
	else 
		fscanf(master.restart, "%llu\n", &mplrs.deepest);
		
	/* get L */
	while((len = getline(&line, &size, master.restart))!= -1)
	{
		if (line[0]=='\n') /* ignore blank lines */
		{
			free(line);
			line = NULL;
			size=0;
			continue;
		}
		line[strlen(line)-1]='\0'; /* replace \n by \0 */
		master.cobasis_list = addlist(master.cobasis_list, line);
		master.size_L++;
		line = NULL;
		size = 0;
	}
	master.tot_L = master.size_L; /* maybe should save and retrieve */
	mprintf(("M: Restarted with |L|=%lu\n",master.size_L)); 
	fclose(master.restart);

	MPI_Send(restart, 3, MPI_INT, CONSUMER, 7, MPI_COMM_WORLD);
	send_counting_stats(CONSUMER);
	mplrs.rays = 0;
	mplrs.vertices = 0;
	mplrs.bases = 0;
	mplrs.facets = 0;
	mplrs.intvertices = 0;

	return;
}

/* check if we should update the histogram and then do it */
void print_histogram(struct timeval *cur, struct timeval *last)
{
	float sec;
	int i;
	int act;

	gettimeofday(cur,NULL);
	if (cur->tv_sec > last->tv_sec)
	{
		sec = (float)(cur->tv_sec - mplrs.start.tv_sec) + ((float)(cur->tv_usec - mplrs.start.tv_usec))/1000000;
		act=0;
		for (i=0; i<mplrs.size; i++)
			if (master.act_producers[i])
				act++;
		/* time (seconds), number of active producers, number of
		 * producers owing us a message about remaining cobases,
		 * 0, 0 (existed in mplrs.cpp but no longer)
		 */
		fprintf(master.hist, "%f %d %lu %d %d %d %lu\n",
			sec, act, master.size_L, master.num_producers, 0, 0, master.tot_L);
		fflush(master.hist);
		last->tv_sec = cur->tv_sec;
		last->tv_usec = cur->tv_usec;
	}
}

/**********
 * worker *
 **********/

int mplrs_worker(void)
{
	char *starting_cobasis;
	/* header for incoming work:
	 * {length of work string, int maxdepth, int maxcobases, 5xfuture use}
	 */ 
	int header[8]={0,0,0,0,0,0,0,0};
	MPI_Request req = MPI_REQUEST_NULL;
	unsigned int ncob=0; /* used for # cobases in prev. job */
	unsigned int tot_ncob=0; 
	int len;
	int flag;

	while (1)
	{
		ncob = mplrs.bases - tot_ncob; /* #cobases in last job */
		tot_ncob = mplrs.bases; /* #cobases done so far */
		/* check signals */
		mplrs_handlesigs();
		/* ask for work */
		mprintf2(("%d: Asking master for work\n",mplrs.rank));
		MPI_Isend(&ncob, 1, MPI_UNSIGNED, MASTER, 6, MPI_COMM_WORLD, 
			  &req);
		flag = 0;
		while (1) /* was MPI_Wait(&req, MPI_STATUS_IGNORE); */
		{
			MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
			if (flag)
				break;
			clean_outgoing_buffers();
		}

		starting_cobasis = NULL;
		/* get response */
		MPI_Recv(header, 8, MPI_INT, MASTER, MPI_ANY_TAG,
			 	 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		mprintf2(("%d: Message received from master\n",mplrs.rank));

		len = header[0];	

		if (len==-1) /* no more work to do */
			return mplrs_worker_finished();

		if (len>0)
		{
			starting_cobasis = (char*)malloc(sizeof(char)*(len+1));
			MPI_Recv(starting_cobasis, len, MPI_CHAR, MASTER,
			 	 MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			starting_cobasis[len] = '\0';
		}

		mplrs.outputblock = 0; /* enable maxbuf-based flushing */
		/* do work */
		do_work(header, starting_cobasis);
		free(starting_cobasis);

		/* send output and unfinished cobases */
		mplrs.outputblock = 0; /* enable maxbuf-based flushing */
		process_output();
		return_unfinished_cobases();

		clean_outgoing_buffers(); /* check buffered sends, 
					   * free if finished
					   */
	}
	return 0; /* unreachable */
}

/* This worker has finished.  Tell the consumer, send counting stats
 * and exit.
 */
int mplrs_worker_finished(void)
{
	int done[3] = {-1,-1,-1};

	mprintf((" %d: All finished! Informing consumer.\n",mplrs.rank));

	while (mplrs.outgoing) /* needed? negligible in any case */
	{
		clean_outgoing_buffers();
	}
	MPI_Send(&done, 3, MPI_INT, CONSUMER, 7, MPI_COMM_WORLD);
	send_counting_stats(CONSUMER);
	MPI_Finalize();
	return 0;
}	

/* Go through our outgoing MPI_Isends, free anything that has completed.
 * Also, if any of these are queued and the header has completed, then
 * send the remaining data.
 * Don't use with incoming buffers.
 */
void clean_outgoing_buffers(void)
{
	msgbuf *msg, *next, *prev=NULL;
	
	for (msg = mplrs.outgoing; msg; msg=next)
	{
		next = msg->next;
		if (!outgoing_msgbuf_completed(msg))
		{
			prev = msg;
			continue;
		}
		if (prev)
			prev->next = next;
		else
			mplrs.outgoing = next;
		free_msgbuf(msg);
	}
}

/* had an overflow; discard any buffered output and
 * unfinished cobases.  some output may have already been
 * sent (due to -maxbuf) so duplication still possible.
 * also resets mplrs.outputblock = 0
 */
void overflow_cleanup(void)
{
	outlist *out = mplrs.output_list, *next;
	slist *list, *lnext;

	mplrs.outputblock = 0;

	/* discard output */
        mplrs.outnum = 0; /* clearing buffer */
        mplrs.output_list = NULL;
        mplrs.ol_tail = NULL;
	for (; out; out=next)
	{
		next = out->next;
		free(out->type);
		free(out->data);
		free(out);
	}

	/* discard cobases */
	for (list=mplrs.cobasis_list; list; list=lnext)
	{
		lnext = list->next;
		free(list->data);
		free(list);
	}
	mplrs.cobasis_list = NULL;
}

/* write the worker's temporary file */
/* header[1] gives maxdepth, header[2] gives maxcobases,
 * if header[0] > 0, 
 *     starting_cobasis gives the starting cobasis.
 * if header[0] == 0, starting at initial input (phase 1)
 */
void write_workfile(const int *header, char *starting_cobasis)
{
	int i, len;
	long depth = 0; /* if restarting from earlier checkpoint,
			 * then we want to fall back to old depth-0
			 * behavior */
	/* prepare input file */
	mplrs.initializing = 0;
	mplrs.tfile = fopen(mplrs.tfn, "w");
	fprintf(mplrs.tfile, "%s", mplrs.input);

	if (header[0]>0)
	{
		/* ugly: recover depth after '!' marker process_cobasis adds */
		len = strlen(starting_cobasis);
		for (i=0; i<len-1; i++)
			if (starting_cobasis[i]=='!')
			{
				depth = strtol(starting_cobasis+i+1, NULL, 10);
				starting_cobasis[i] = ' ';
				break;
			}
		mprintf3(("%d: depth %ld in %s\n", mplrs.rank, depth,
			  starting_cobasis));
		fprintf(mplrs.tfile, "\nmindepth %ld\nrestart %s\n", depth,
				     starting_cobasis);
		if (i<len-1) /* reset in case redo because of overflow */
			starting_cobasis[i] = '!';
	}
	else
		mplrs.initializing = 1; /* phase 1 output handled different */

	if (header[1]>0)
		fprintf(mplrs.tfile, "\nmaxdepth %ld\n", header[1] + depth);
	if (header[2]>0)
		fprintf(mplrs.tfile, "\nmaxcobases %d\n", header[2]);

	if (mplrs.countonly == 1)
		fprintf(mplrs.tfile, "countonly\n");

	if (mplrs.initializing != 1 && header[3]==1) /* lponly option, only in*/
		fprintf(mplrs.tfile, "lponly\n");    /* non-initial jobs
						      */
		
	fclose(mplrs.tfile);
	return;
}

/* header[1] gives maxdepth, header[2] gives maxcobases,
 * if header[0] > 0, 
 *     starting_cobasis gives the starting cobasis.
 * if header[0] == 0, starting at initial input (phase 1)
 */
void do_work(const int *header, char *starting_cobasis)
{
	char *argv[] = {"mplrs-internal", mplrs.tfn};
	int ret = 0;
	float junk;

	mprintf3(("%d: Received work (%d,%d,%d)\n",mplrs.rank,header[0],
						   header[1],header[2]));
	write_workfile(header, starting_cobasis);
	mprintf2(("%d: Calling lrs_main\n",mplrs.rank));
#ifdef MA
	if (mplrs.overflow == 0)
		ret = lrs1_main(2, argv, 0, NULL);
	if (mplrs.overflow == 1 || ret == 1) /* lrs1_main overflowed */
	{
		if (ret == 1)
		{
			/*this message is not desired */
			/*send_output(2, dupstr("*changing arithmetic package due to possible overflow: duplicate output possible\n"));*/
			mplrs.overflow = 1;
			overflow_cleanup();
			remove(mplrs.tfn); /* redo file in case removed */
			write_workfile(header, starting_cobasis);
		}
#ifdef B128
		ret = lrs2_main(2, argv, 0, NULL);
#else
		ret = lrsgmp_main(2, argv, 0, NULL);
#endif
	}
	if (mplrs.overflow == 2 || ret == 1) /* lrs2_main overflowed */
	{
		if (ret == 1)
		{
			mplrs.overflow = 2;
			overflow_cleanup();
			remove(mplrs.tfn); /* redo file in case removed */
			write_workfile(header, starting_cobasis);
		}
		ret = lrsgmp_main(2, argv, 0, NULL);
	}
#else
#if defined(GMP)
	ret = lrsgmp_main(2, argv, 0, NULL);
#elif defined(FLINT)
	ret = lrs_main(2, argv);
#elif defined(LRSLONG)
#ifdef B128
	ret = lrs2_main(2, argv, 0, NULL);
#else
	ret = lrs1_main(2, argv, 0, NULL);
#endif
#endif
	if (ret == 1)
	{
		junk = -1;
		send_output(2, dupstr("*possible overflow: try using gmp or hybrid mplrs\n"));
		/* inform master, wait for reply */
		MPI_Send(&junk, 1, MPI_FLOAT, MASTER, 9, MPI_COMM_WORLD);
		MPI_Recv(&junk, 1, MPI_FLOAT,MASTER,9,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
	}
#endif
	mprintf2(("%d: lrs_main returned\n",mplrs.rank)); 
	if (remove(mplrs.tfn) != 0) /* UNsynchronized printf -- should fix */
		printf("Error deleting thread file!\n");
}

/* The worker has finished its work.  Process the output, preparing
 * and sending the output to the consumer, and preparing the unfinished
 * cobases for return_unfinished_cobases().
 */
void process_output(void)
{
	outlist *out = mplrs.output_list, *next;
	char *out_string=NULL; /* for output file if exists */
	char *serr_string=NULL; /* for stdout */
	const char *type; /* because plrs_output is C++ at the moment */
	const char *data; /* because plrs_output is C++ at the moment */
	int len = 1024;
	int len2 = 256;

	mplrs.outnum = 0; /* clearing buffer */
	mplrs.output_list = NULL;
	mplrs.ol_tail = NULL;

	out_string = (char *)malloc(sizeof(char)*len);
	out_string[0]='\0';
	serr_string = (char *)malloc(sizeof(char)*len);
	serr_string[0]='\0';

	/* reverse when initializing to get correct order */
#if 0 /* TODO fixme */
	if (mplrs.initializing)
		out = reverse_list(out);
#endif

	while (out)
	{
		type = out->type;
		data = out->data;
		if (!strcmp(type, "vertex"))
			out_string = append_out(out_string, &len, data);
		else if (!strcmp(type, "ray"))
			out_string = append_out(out_string, &len, data);
		else if (!strcmp(type, "unexp"))
			process_cobasis(data);
		else if (!strcmp(type, "cobasis"))
			out_string = append_out(out_string, &len, data);
		else if (!strcmp(type, "V cobasis"))
			out_string = append_out(out_string, &len, data);
		else if (!strcmp(type, "facet count"))
			mplrs.facets += atoi(data);
		else if (!strcmp(type, "ray count"))
			mplrs.rays += atoi(data);
		else if (!strcmp(type, "basis count"))
			mplrs.bases += atoi(data);
		else if (!strcmp(type, "vertex count"))
			mplrs.vertices += atoi(data);
		else if (!strcmp(type, "integer vertex count"))
			mplrs.intvertices += atoi(data);
		else if (!strcmp(type, "tree depth"))
		{
			if (mplrs.deepest < atol(data))
				mplrs.deepest = atol(data);
		}
		else if (!strcmp(type, "linearities"))
		{
			if (mplrs.linearities < atol(data))
				mplrs.linearities = atol(data);
		}
		else if (!strcmp(type, "volume"))
		{
			plrs_readrat(mplrs.Tnum, mplrs.Tden, data);
			copy(mplrs.tN, mplrs.Vnum); copy(mplrs.tD, mplrs.Vden);
			linrat(mplrs.tN, mplrs.tD, 1L, mplrs.Tnum, mplrs.Tden,
			       1L, mplrs.Vnum, mplrs.Vden);
		}
		else if (!strcmp(type, "options warning"))
		{
			/* only do warnings once, otherwise repeated */
			if (mplrs.initializing)
				out_string = append_out(out_string, &len, data);
		}
		else if (!strcmp(type, "header"))
		{
			/*only do header if initializing, otherwise repeated*/
			if (mplrs.initializing)
				out_string = append_out(out_string, &len, data);
		}
		else if (!strcmp(type, "debug"))
		{
			out_string = append_out(out_string, &len, data);
		}
		else if (!strcmp(type, "warning"))
		{ /* warnings always go to stderr */
			serr_string = append_out(serr_string, &len2, data);
		}

		next = out->next;
		free(out->type);
		free(out->data);
		free(out);
		out = next;
	}

	if (strlen(out_string)>0 && strcmp(out_string, "\n"))
		send_output(1, out_string);
	else
		free(out_string);
	if (strlen(serr_string)>0 && strcmp(serr_string, "\n"))
		send_output(0, serr_string);
	else
		free(serr_string);
}

/* send this string to the consumer to output.
 * If dest==1, then it goes to the output file (stdout if no output file).
 * If dest==0, then it goes to stderr.
 * If dest==2, it's an overflow message: only the first one printed (stderr)
 *
 * The pointer str is surrendered to send_output and should not be changed
 * It will be freed once the send is complete.
 */
/* str should not be NULL */
void send_output(int dest, char *str)
{
	msgbuf *msg = (msgbuf *)malloc(sizeof(msgbuf));
	int *header = (int *)malloc(sizeof(int)*3);

	header[0] = dest;
	header[1] = strlen(str);
	header[2] = mplrs.my_tag; /* to ensure the dest/str pair
				   * remains intact even if another
				   * send happens in between
				   */
	msg->req = (MPI_Request *)malloc(sizeof(MPI_Request)*2);
	msg->buf = (void **)malloc(sizeof(void *)*2);
	msg->buf[0] = header;
	msg->buf[1] = str;
	msg->count = 2;
	msg->target = CONSUMER;
	msg->queue = 1;

	msg->tags = (int *)malloc(sizeof(int)*2);
	msg->sizes = (int *)malloc(sizeof(int)*2);
	msg->types = (MPI_Datatype *)malloc(sizeof(MPI_Datatype)*2);

	msg->types[1] = MPI_CHAR;
	msg->sizes[1] = header[1]+1;

	msg->tags[1] = mplrs.my_tag;

	mplrs.my_tag++;

	msg->next = mplrs.outgoing;
	mplrs.outgoing = msg;
	MPI_Isend(header, 3, MPI_INT, CONSUMER, 7, MPI_COMM_WORLD,
		  msg->req);
}

/* called from process_output to handle a 'cobasis',
 * add to queue to return to master
 */
/* awful string-hacking, basically copied from plrs processCobasis() */
/* First, if first characters are 'F#', it's a hull. otherwise it's not.
 * Then, remove ignore_chars.
 * Then, remove everything starting from the first 'I'
 * Then, copy everything verbatim, except:
 *    if it's a hull, replace everything between second and third spaces
 *                    by '0'
 *    if it's not a hull, replace everything between third and fourth
 *                    spaces by '0'
 * (this is resetting the depth to be 0 for a restart)
 */
void process_cobasis(const char *newcob)
{
	int nlen = strlen(newcob);
	char *buf = (char *)malloc(sizeof(char) * (nlen+1));
	int i,j,k;
	int num_spaces=0; /* we count the number of spaces */
	char ignore_chars[] = "#VRBh=facetsFvertices/rays";
	char c;
	int num_ignore = strlen(ignore_chars);
	int hull = 0;
	/*int replace = 0;*/ /* no longer hacking depth to 0 */

	mprintf3(("%d: process_cobasis( %s )", mplrs.rank, newcob));

	if (nlen>1 && newcob[0]=='F' && newcob[1]=='#')
		hull = 1;

	for (i=0,j=0; i<=nlen; i++)
	{
		c = newcob[i];
		/* ignore ignore_chars */
		for (k=0; k<=num_ignore; k++)
			if (c == ignore_chars[k])
				break;
		if (k<=num_ignore)
			continue;

		if (c=='I')
			break;

		if (c==' ') /* count spaces to set depth to 0 for restart */
		{
			num_spaces++;
			if ( (num_spaces==2 && hull==1) ||
			     (num_spaces==3 && hull==0) )
			{
				/*replace = 1;*/
				buf[j++]='!'; /* mark depth ... */
				continue;
			}
#if 0
			else if ( (num_spaces==3 && hull==1) ||
				  (num_spaces==4 && hull==0) )
				replace = 0;
#endif
			buf[j++] = c; /* copy other spaces */
			continue;
		}
#if 0
		if (replace == 1)    /* remove three lines here */
			buf[j++]='0';/* to play with mindepth */
		else                 /* leaving only the next line */
#endif
			buf[j++]=c;
	}
	buf[j]='\0';

	mprintf3((" produced %s\n",buf));
	mplrs.cobasis_list = addlist(mplrs.cobasis_list, buf);
}

inline slist *addlist(slist *list, void *buf)
{
	slist *n = (slist *)malloc(sizeof(struct slist));
	n->data = buf;
	n->next = list;
	return n;
}

/* mplrs.cobasis_list may have things to send to the master.
 * Send the header, and then the cobases to add to L.
 */
void return_unfinished_cobases(void)
{
	int listsize;
	slist *list, *next;
	int *lengths=NULL;
	char *cobases=NULL;
	int size = 0;
	int i;
	int start;
	/* header is (strlen(cobases), length of lengths, mplrs.my_tag) */
	int *header = (int *)malloc(sizeof(int)*3);
	msgbuf *msg = (msgbuf *)malloc(sizeof(msgbuf));
	msg->target = MASTER;

	for (listsize=0, list=mplrs.cobasis_list; list; list=list->next)
	{
		listsize++;
		size += strlen((char *)list->data);
	}

	if (listsize == 0)
	{
		header[0] = -1;
		header[1] = -1;
		header[2] = -1;
		msg->buf = (void **)malloc(sizeof(void *));
		msg->buf[0] = header;
		msg->count = 1;
		msg->req = (MPI_Request *)malloc(sizeof(MPI_Request));
		msg->queue = 0;
		msg->tags = NULL;
		msg->sizes = NULL;
		msg->types = NULL;
		MPI_Isend(header, 3, MPI_INT, MASTER, 10, MPI_COMM_WORLD,
			  msg->req);
		msg->next = mplrs.outgoing;
		mplrs.outgoing = msg;
		return;
	}

	lengths = (int *)malloc(sizeof(int)*listsize);  /*allows unconcatenate*/
	cobases = (char *)malloc(sizeof(char)*(size+1));/*concatenated + 1 \0*/

	for (start=0, i=0, list=mplrs.cobasis_list; list; list=next, i++)
	{
		next = list->next;

		strcpy(cobases+start, (char *)list->data);
		lengths[i] = strlen((char *)list->data);
		start+=lengths[i];

		free(list->data);
		free(list);
	}
	/* final \0 is there */

	header[0] = listsize;
	header[1] = size+1;
	header[2] = mplrs.my_tag;

	msg->req = (MPI_Request *)malloc(sizeof(MPI_Request) * 3);
	msg->buf = (void **)malloc(sizeof(void *) * 3);
	msg->buf[0] = header;
	msg->buf[1] = cobases;
	msg->buf[2] = lengths;

	msg->count = 3;
	msg->queue = 0;
	msg->tags = NULL;
	msg->sizes = NULL;
	msg->types = NULL;

	mprintf2(("%d: Queued send of %d cobases for L\n",mplrs.rank,listsize));
	MPI_Isend(header, 3, MPI_INT, MASTER, 10, MPI_COMM_WORLD, msg->req);
	MPI_Isend(cobases, header[1], MPI_CHAR, MASTER, mplrs.my_tag,
		  MPI_COMM_WORLD, msg->req+1);
	MPI_Isend(lengths, listsize, MPI_INT, MASTER, mplrs.my_tag, 
		  MPI_COMM_WORLD, msg->req+2);
	mplrs.my_tag++;

	msg->next = mplrs.outgoing;
	mplrs.outgoing = msg;
	mplrs.cobasis_list = NULL;
}

/* dest is a string in a buffer with size *size.
 * Append src and a newline to the string, realloc()ing as necessary,
 * returning the new pointer and updating size.
 */
char *append_out(char *dest, int *size, const char *src)
{
	int len1 = strlen(dest);
	int len2 = strlen(src);
	int newsize = *size;
	char *newp = dest;

	if (len1 + len2 + 2 > *size)
	{
		newsize = newsize<<1;
		while ((newsize < len1+len2+2) && newsize)
			newsize = newsize<<1;

		if (!newsize)
			newsize = len1+len2+2;

		newp = (char *)realloc(dest, sizeof(char) * newsize);
		if (!newp)
		{
			newsize = len1+len2+2;
			newp = (char *)realloc(dest, sizeof(char) * newsize);
			if (!newp)
			{
				printf("%d: Error no memory (%d)\n",mplrs.rank,
								    newsize);
				/* MPI_Finalize(); */
				exit(2);
			}
		}
		*size = newsize;
	}

	strncat(newp, src, len2);
	newp[len1+len2]='\n';
	newp[len1+len2+1]='\0';
	return newp;
}

/************
 * consumer *
 ************/

int mplrs_consumer(void)
{
	int i;
	int check = 0;

	initial_print(); 	/* print version and other information */
	/* initialize MPI_Requests and 3*int buffers for incoming messages */
	consumer.prodreq = (MPI_Request*)malloc(sizeof(MPI_Request)*mplrs.size);
	consumer.prodibf = (int *)malloc(sizeof(int)*3*mplrs.size);
	consumer.num_producers = mplrs.size - 2;
	consumer.overflow = (int *)malloc(sizeof(int)*mplrs.size);
	for (i=0; i<mplrs.size; i++)
	{
		consumer.overflow[i] = 0;
		if (i==CONSUMER)
			continue;
		MPI_Irecv(consumer.prodibf+(i*3), 3, MPI_INT, i, 7,
			  MPI_COMM_WORLD, consumer.prodreq+i);
	}

	/* final_print condition handles overflow cleanstop in initial
	 * job 
	 */
	while (consumer.num_producers>0 || consumer.incoming || 
	       (consumer.waiting_initial && consumer.final_print))
	{
		/* check if someone is trying to send us output */
		/* if so, queue any incoming messages */
		consumer_start_incoming();

		/* check for completed message to process */
		consumer_proc_messages();

		/* check signals */
		mplrs_handlesigs();
	}

	mprintf2(("C: getting stats and exiting\n"));

	for (i=0; i<mplrs.size; i++)
	{
		if (i==CONSUMER || i==MASTER)
			continue;
		recv_counting_stats(i);
	}

	free(consumer.prodreq);
	free(consumer.prodibf);
	consumer.prodreq = NULL; consumer.prodibf = NULL;

	mprintf2(("C: checking with master whether checkpointing ...\n"));
	MPI_Recv(&check, 1, MPI_INT, MASTER,1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
	mprintf2(("C: checkpointing flag is %d\n", check));

	if (check == CHECKFLAG)
		return consumer_checkpoint();

	recv_master_stats(); /* gets stats on size of L, etc */
	if (consumer.final_print)
		final_print();
	free(consumer.overflow);
	MPI_Finalize();
	return 0;
}

/* check if anyone is trying to send us their output */
/* if master is trying, probably means we're going to checkpoint */
/* in any case, queue any incoming messages          */
void consumer_start_incoming(void)
{
	int i;
	int flag;
	for (i=0; i<mplrs.size; i++)
	{
		/* don't check ourself and workers that have exited */
		if (i==CONSUMER || consumer.prodreq[i] == MPI_REQUEST_NULL)
			continue;
		MPI_Test(consumer.prodreq+i, &flag, MPI_STATUS_IGNORE);
		if (!flag) /* not incoming, check next */
			continue;
		mprintf3(("C: received message from %d\n",i));
#if 0
		if (i==MASTER && consumer.prodibf[0]==CHECKFLAG)
		{	/* start checkpoint */
			/* this condition is no longer reachable */
			consumer.checkpoint = 1;
			mprintf(("*Checkpointing\n"));
			continue;
		}
		else
#endif
		if (i==MASTER && consumer.prodibf[0]==STOPFLAG)
		{
			/* reachable, since consumer must know if
			 * overflow happens in initial job ... */
			consumer.final_print = 0;
			mprintf(("C: stopping...\n"));
			continue;
		}
		else if (i==MASTER && consumer.prodibf[0]==RESTARTFLAG)
		{
			/* get counting stats for restart */
			recv_counting_stats(MASTER);
			consumer.waiting_initial = 0;
			mprintf(("C: Restarted\n"));
			/* master may restart and later checkpoint */
			MPI_Irecv(consumer.prodibf+(i*3), 3, MPI_INT, i, 7,
                          	  MPI_COMM_WORLD, consumer.prodreq+i);
			continue;
		}
		
		if (consumer.prodibf[3*i]<=0 && consumer.prodibf[3*i+1]<=0)
		{
			/* producer i has finished and will exit */
			consumer.num_producers--;
			mprintf(("C: Producer %d reported exiting, %d left.\n",
				 i, consumer.num_producers));
			continue;
		}

		/* otherwise, we have the normal situation -- the producer
		 * wants to send us some output.
		 */
		consumer.incoming =
			 consumer_queue_incoming(consumer.prodibf+3*i, i);
		MPI_Irecv(consumer.prodibf+(i*3), 3, MPI_INT, i, 7,
			  MPI_COMM_WORLD, consumer.prodreq+i);
	}
}

/* target has sent us this header. Queue the corresponding receive and
 * return the new value of consumer.incoming.
 */
msgbuf *consumer_queue_incoming(int *header, int target)
{
	msgbuf *curhead = consumer.incoming;
	msgbuf *newmsg = (msgbuf *)malloc(sizeof(msgbuf));

	newmsg->req = (MPI_Request *)malloc(sizeof(MPI_Request)*2);
	newmsg->buf = (void **)malloc(sizeof(void *));
	newmsg->buf[0] = (char *)malloc(sizeof(char)*(header[1]+1));
	newmsg->count = 1;
	newmsg->target = target;
	newmsg->next = curhead;
	newmsg->queue = 0;
	newmsg->tags = NULL;
	newmsg->sizes = NULL;
	newmsg->types = NULL;

	newmsg->data = header[0]; /* bound for stdout or output file */

	/* get my_tag from producer via header[2] to uniquely identify msg */
	MPI_Irecv(newmsg->buf[0], header[1]+1, MPI_CHAR, target, header[2],
		  MPI_COMM_WORLD, newmsg->req);

	mprintf3(("C: Receiving from %d (%d,%d,%d)",target,header[0],header[1],
						    header[2]));
	return newmsg;
}

/* check our incoming messages, process and remove anything that
 * has completed
 */
void consumer_proc_messages(void)
{
	msgbuf *msg, *prev=NULL, *next;
	int i,len;

	for (msg=consumer.incoming; msg; msg=next)
	{
		next=msg->next;
		if (outgoing_msgbuf_completed(msg))
		{
			if (consumer.waiting_initial &&
			    msg->target != INITIAL)
				continue;

			/* we wait on all other output until we've printed
			 * the initial output containing ...begin\n
			 * to ensure pretty output, if this is the begin,
			 * flip the flag.
			 */
			if (consumer.waiting_initial)
			{
				len=strlen((char *)msg->buf[0])-5;
				for (i=0; i<len; i++)
					if (!strncmp("begin\n",
						     (char*)msg->buf[0]+i,6))
					{
						mprintf3(("C: found begin\n"));
						phase1_print();
						consumer.waiting_initial = 0;
						break;
					}
			}

			/* print the 'begin' only after phase1_print */
			if (msg->data == 1)
				fprintf(consumer.output, "%s",
					(char*)msg->buf[0]);
			else if (msg->data == 2)
			{
				if (!consumer.oflow_flag)
				{
					consumer.oflow_flag = 1;
					fprintf(stderr,"%s",(char*)msg->buf[0]);
				}
			}
			else /* headed to stderr */
				fprintf(stderr, "%s", (char*)msg->buf[0]);

			free_msgbuf(msg);
			if (prev)
				prev->next = next;
			else
				consumer.incoming = next;
			continue;
		}
		prev=msg;
	}
}

/* We are checkpointing instead of a normal exit.
 * Send counting stats to master, then exit quietly
 */
int consumer_checkpoint(void)
{
	int len;
	char *str;
	char *vol = cprat("", mplrs.Vnum, mplrs.Vden);
	send_counting_stats(MASTER);
	mprintf(("C: in consumer_checkpoint\n"));
	MPI_Recv(&len, 1, MPI_INT, MASTER, 1, MPI_COMM_WORLD,
		 MPI_STATUS_IGNORE);
	if (len == -1) /* master produces checkpoint file */
	{
		MPI_Finalize();
		return 0;
	}
	fprintf(consumer.output, "*Checkpoint file follows this line\n");
	fprintf(consumer.output, "mplrs4\n%llu %llu %llu %llu %llu\n%s\n%llu\n",
		mplrs.rays, mplrs.vertices, mplrs.bases, mplrs.facets,
		mplrs.intvertices,vol,mplrs.deepest);
	free(vol);
	while (1)
	{
		MPI_Recv(&len, 1, MPI_INT, MASTER, 1, MPI_COMM_WORLD,
			 MPI_STATUS_IGNORE);
		if (len<0)
			break;
		str = (char*)malloc(sizeof(char)*len);
		MPI_Recv(str, len, MPI_CHAR, MASTER, 1, MPI_COMM_WORLD,
			 MPI_STATUS_IGNORE);
		fprintf(consumer.output,"%s\n",str);
		free(str);
	}
	fprintf(consumer.output,"*Checkpoint finished above this line\n");
	MPI_Finalize();
	return 0;
}

/* check whether this outgoing msgbuf has completed.
 * If the msg is queued (msg->queue == 1),
 *    then if the first part has completed, send the remaining parts
 * Don't use with *queued* incoming msgbuf.
 */
inline int outgoing_msgbuf_completed(msgbuf *msg)
{
	int flag;
	int count = msg->count;
	int i;
	if (msg->queue != 1)
	{
		MPI_Testall(count, msg->req, &flag, MPI_STATUSES_IGNORE);
		return flag;
	}
	MPI_Test(msg->req, &flag, MPI_STATUS_IGNORE);
	if (!flag)
		return flag;
	/* first completed, send the rest of the queued send */	
	mprintf3(("%d: Sending second part of queued send to %d\n",
		  mplrs.rank, msg->target));
	for (i=1; i<count; i++)
	{
		if (msg->buf[i])
			MPI_Isend(msg->buf[i], msg->sizes[i], msg->types[i],
			  	msg->target, msg->tags[i], MPI_COMM_WORLD,
			  	msg->req+i);
	}
	msg->queue = 0;
	return 0;
}

inline void free_msgbuf(msgbuf *msg)
{
	int i;
	for (i=0; i<msg->count; i++)
		free(msg->buf[i]);
	free(msg->buf);
	free(msg->req);
	free(msg->tags);
	free(msg->sizes);
	free(msg->types);
	free(msg);
	return;
}

outlist *reverse_list(outlist* head)
{
	outlist * last = head, * new_head = NULL;
	while(last)
	{
		outlist * tmp = last;
		last = last->next;
		tmp->next = new_head;
		new_head = tmp;
	}
	return new_head;
}

/* send stats on size of L, etc */
void send_master_stats(void)
{
	unsigned long stats[4] = {master.tot_L, master.num_empty, 0, 0};
	MPI_Send(stats, 4, MPI_UNSIGNED_LONG, CONSUMER, 1, MPI_COMM_WORLD);
	return;
}
/* get master stats on size of L, etc */
void recv_master_stats(void)
{
	unsigned long stats[4] = {0,0,0,0};
	MPI_Recv(stats, 4, MPI_UNSIGNED_LONG, MASTER, 1, MPI_COMM_WORLD, 
		 MPI_STATUS_IGNORE);
	master.tot_L = stats[0];
	master.num_empty = stats[1];
	return;
}

/* send stats to target for final print */
void send_counting_stats(int target)
{
	char *vol = NULL;
	mprintf3(("%d: sending counting stats to %d\n", mplrs.rank, target));
	if (mplrs.facets>0)
		vol = cprat("", mplrs.Vnum, mplrs.Vden);
	else
	{
		vol = (char *)malloc(sizeof(char)*2);
		vol[0] = '0'; vol[1] = '\0';
	}
	unsigned long long stats[10] = {mplrs.rays, mplrs.vertices, mplrs.bases,
			          mplrs.facets, mplrs.intvertices,
				  strlen(vol)+1, mplrs.deepest, mplrs.overflow,
				  mplrs.linearities, 0};
	MPI_Send(stats, 10, MPI_UNSIGNED_LONG_LONG, target, 1, MPI_COMM_WORLD);
	MPI_Send(vol, stats[5], MPI_CHAR, target, 1, MPI_COMM_WORLD);
	free(vol);
	return;
}

/* gets counting stats from target */
void recv_counting_stats(int target)
{
	char *vol;
	unsigned long long stats[10];
	MPI_Recv(stats, 10, MPI_UNSIGNED_LONG_LONG, target, 1, MPI_COMM_WORLD,
		 MPI_STATUS_IGNORE);
	mprintf3(("%d: got counting stats from %d\n", mplrs.rank, target));
	mplrs.rays+=stats[0];
	mplrs.vertices+=stats[1];
	mplrs.bases+=stats[2];
	mplrs.facets+=stats[3];
	mplrs.intvertices+=stats[4];
	if (stats[6] > mplrs.deepest)
		mplrs.deepest = stats[6];
	if (mplrs.rank == CONSUMER)
		consumer.overflow[target] = stats[7];
	if (stats[8] > mplrs.linearities)
		mplrs.linearities = stats[8];
	vol = (char*)malloc(sizeof(char)*stats[5]);
	MPI_Recv(vol, stats[5], MPI_CHAR, target, 1, MPI_COMM_WORLD,
		 MPI_STATUS_IGNORE);
	plrs_readrat(mplrs.Tnum, mplrs.Tden, vol);
	copy(mplrs.tN, mplrs.Vnum); copy(mplrs.tD, mplrs.Vden);
	linrat(mplrs.tN, mplrs.tD, 1L, mplrs.Tnum, mplrs.Tden,
	       1L, mplrs.Vnum, mplrs.Vden);
	free(vol);
	return;
}

/* do the initial print */
void initial_print(void)
{
#ifdef MA
		fprintf(consumer.output, "*mplrs:%s%s(hybrid arithmetic)%d processes\n",
			TITLE, VERSION, mplrs.size);
#elif defined(GMP)
		fprintf(consumer.output, "*mplrs:%s%s(%s gmp v.%d.%d)%d processes\n",
			TITLE,VERSION,ARITH,__GNU_MP_VERSION,
			__GNU_MP_VERSION_MINOR,mplrs.size);
#elif defined(FLINT)
		fprintf(consumer.output, "*mplrs:%s%s(%s, %dbit flint v.%s)%d processes\n",
			TITLE,VERSION,ARITH,FLINT_BITS,FLINT_VERSION,
			mplrs.size);
#elif defined(SAFE)
		fprintf(consumer.output, "*mplrs:%s%s(%s,%s,overflow checking)%d processes\n",
			TITLE,VERSION,BIT,ARITH,mplrs.size);
#else
		fprintf(consumer.output, "*mplrs:%s%s(%s,%s,no overflow checking)%d processes\n",
			TITLE,VERSION,BIT,ARITH,mplrs.size);
#endif
		fprintf(consumer.output, "*Input taken from %s\n",
			mplrs.input_filename);
		fprintf(consumer.output, "*Starting depth of %d maxcobases=%d ",
			master.initdepth, master.maxcobases);
		fprintf(consumer.output, "maxdepth=%d lmin=%d lmax=%d scale=%d\n",
			master.maxdepth, master.lmin,
			master.lmax, master.scalec);
		if (mplrs.countonly)
			fprintf(consumer.output, "*countonly\n");
		if (consumer.output==stdout)
			return;
#ifdef MA
		printf("*mplrs:%s%s(hybrid arithmetic)%d processes\n", 
			TITLE, VERSION, mplrs.size);
#elif defined(GMP)
		printf("*mplrs:%s%s(%s gmp v.%d.%d)%d processes\n",
			TITLE,VERSION,ARITH,__GNU_MP_VERSION,
			__GNU_MP_VERSION_MINOR,mplrs.size);
#elif defined(FLINT)
		printf("*mplrs:%s%s(%s, %dbit flint v.%s)%d processes\n",
			TITLE,VERSION,ARITH,FLINT_BITS,FLINT_VERSION,
			mplrs.size);
#elif defined(SAFE)
		printf("*mplrs:%s%s(%s,%s,overflow checking)%d processes\n",
			TITLE,VERSION,BIT,ARITH,mplrs.size);
#else
		printf("*mplrs:%s%s(%s,%s,no overflow checking)%d processes\n",
			TITLE,VERSION,BIT,ARITH,mplrs.size);
#endif
		printf("*Input taken from %s\n",mplrs.input_filename);
		printf("*Output written to: %s\n",consumer.output_filename);
		printf("*Starting depth of %d maxcobases=%d ", master.initdepth,
		       master.maxcobases);
		printf("maxdepth=%d lmin=%d lmax=%d scale=%d\n", master.maxdepth,
		       master.lmin, master.lmax, master.scalec);
		if (mplrs.countonly)
			printf("*countonly\n");
}

/* do the "*Phase 1 time: " print */
inline void phase1_print(void)
{
	struct timeval cur;
	gettimeofday(&cur, NULL);
	printf("*Phase 1 time: %ld seconds.\n",cur.tv_sec-mplrs.start.tv_sec);
	return;
}

/* do the final print */
void final_print(void)
{
	struct timeval end;
	char *vol=NULL;
	int i, num64=0, num128=0, numgmp=0;
	gettimeofday(&end, NULL);

	fprintf(consumer.output, "end\n");
	printf("*Total number of jobs: %lu, L became empty %lu times, tree depth %llu\n", master.tot_L, master.num_empty,mplrs.deepest);
#ifdef MA
	for (i=0; i<mplrs.size; i++)
	{
		if (i==MASTER || i==CONSUMER)
			continue;
		if (consumer.overflow[i]==0)
			num64++;
		else if (consumer.overflow[i]==1)
			num128++;
		else
			numgmp++;
	}
#ifdef B128
	printf("*Finished with %d 64-bit, %d 128-bit, %d GMP workers\n",
		num64, num128, numgmp);
#else
	printf("*Finished with %d 64-bit, %d GMP workers\n",
		num64, num128);     /*DA since no 128 bit support, num128 counts gmp */
#endif
#endif
	if (mplrs.facets>0)
	{
                if(!zero(mplrs.Vnum))
                  {
		   vol = cprat("*Volume=",mplrs.Vnum,mplrs.Vden);
		   fprintf(consumer.output,"%s\n",vol);
		   free(vol);
                  }
		fprintf(consumer.output,"*Totals: facets=%llu bases=%llu",
			mplrs.facets, mplrs.bases);
		if (mplrs.linearities > 0)
			fprintf(consumer.output,
				" linearities=%llu facets+linearities=%llu",
			      mplrs.linearities,mplrs.linearities+mplrs.facets);
		fputc('\n', consumer.output);
	}
	else
	{
		fprintf(consumer.output, "*Totals: vertices=%llu rays=%llu bases=%llu integer-vertices=%llu",
			mplrs.vertices,mplrs.rays,mplrs.bases,mplrs.intvertices);
		if (mplrs.linearities > 0)
			fprintf(consumer.output,
			       " linearities=%llu", mplrs.linearities);
		if (mplrs.rays+mplrs.linearities>0)
		{
			fprintf(consumer.output," vertices+rays");
			if (mplrs.linearities>0)
				fprintf(consumer.output, "+linearities");
			fprintf(consumer.output, "=%llu",
				mplrs.vertices+mplrs.rays+mplrs.linearities);
		}
		fputc('\n', consumer.output);
	}
	fprintf(consumer.output, "*Elapsed time: %ld seconds.\n",
		end.tv_sec - mplrs.start.tv_sec);

	if (consumer.output_filename == NULL)
		return;

	if (mplrs.facets>0)
	{
		if (!zero(mplrs.Vnum))
		{
			vol = cprat("*Volume=",mplrs.Vnum,mplrs.Vden);
			printf("%s\n", vol);
			free(vol);
		}
		printf("*Totals: facets=%llu bases=%llu", mplrs.facets,
			mplrs.bases);
		if (mplrs.linearities > 0)
			printf(" linearities=%llu facets+linearities=%llu",
			      mplrs.linearities,mplrs.linearities+mplrs.facets);
		putchar('\n');
	}
	else
	{
		printf("*Totals: vertices=%llu rays=%llu bases=%llu integer-vertices=%llu",
		       mplrs.vertices,mplrs.rays,mplrs.bases,mplrs.intvertices);
		if (mplrs.linearities > 0)
			printf(" linearities=%llu", mplrs.linearities); 
		if (mplrs.rays+mplrs.linearities>0) 
		{ 
			printf(" vertices+rays"); 
			if (mplrs.linearities>0) 
				printf("+linearities"); 
			printf("=%llu",
				mplrs.vertices+mplrs.rays+mplrs.linearities);
		}
		putchar('\n');
	}
	printf("*Elapsed time: %ld seconds.\n", end.tv_sec - mplrs.start.tv_sec);
}

void open_outputblock(void)
{
	mplrs.outputblock++;
}

void close_outputblock(void)
{
	mplrs.outputblock--;
	if (mplrs.outnum++ > mplrs.maxbuf && /* buffer <maxbuf output lines */
	    mplrs.outputblock <= 0 &&
	    mplrs.initializing != 1) /* not in phase 1 (need 'begin' first) */
	{
		process_output();          /* before starting a flush */
		clean_outgoing_buffers();
	}
}

void post_output(const char *type, const char *data)
{
	outlist *out = (outlist *)malloc(sizeof(outlist));
	out->type = dupstr(type);
	out->data = dupstr(data);
	out->next = NULL;
	if (mplrs.output_list == NULL)
		mplrs.output_list = out;
	else
		mplrs.ol_tail->next = out;
	mplrs.ol_tail = out;
	if (mplrs.outnum++ > mplrs.maxbuf && /* buffer <maxbuf output lines */
            mplrs.outputblock <= 0 && 
            mplrs.initializing != 1 && data[strlen(data)-1]=='\n') 
		/* not in phase 1 (need 'begin' first) */
		/* must not flush during phase 1 to avoid multiple 'begin'
		 * statements. otherwise, an overflow could happen after
		 * the flush and we'd produce another 'begin'
		 */
	{
		process_output();	   /* before starting a flush */
		clean_outgoing_buffers();
	}
}

/* strdup */
inline char *dupstr(const char *str)
{
	char *buf = (char *)malloc(sizeof(char)*(strlen(str)+1));
	strcpy(buf,str);
	return buf;
}

/* utilities based on mts code */
/* TODO: changes */
int stream_printf(FILE *str, const char *fmt, ...)
{ /* test if str is stdout/stderr */
	int newlen;
	char *newbuf;
	va_list args;

	va_start(args, fmt);

	/* need a buffer of size newlen + 1 ('\0') */
	newlen = vsnprintf(NULL, 0, fmt, args);
	va_end(args);
	va_start(args, fmt);

	newbuf = (char *)malloc(sizeof(char)*(newlen+1));
	vsprintf(newbuf, fmt, args);
	va_end(args);
	/* todo: fix types or get rid of them entirely */
	if (str!=stderr) /* send to lrs_ofp ... */
		post_output("debug", newbuf);
	else /* send to stderr */
		post_output("warning", newbuf);
	free(newbuf);
	return 1;
}
