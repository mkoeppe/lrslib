#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <queue>
#include "lrslib.h"
#include "plrs.hpp"
#include <sys/time.h>




using namespace std;


//Synynchrochronization Variables
boost::atomic<bool> 	producers_finished(false);
boost::atomic<bool>	initializing(true);
boost::mutex		cobasis_list_mutex;

//List of starting cobasis
queue<string>		cobasis_list;

//Total counts
int RAYS = 0;
int VERTICIES = 0;
int BASIS = 0;
int FACETS = 0;
int INTVERTICIES = 0;

lrs_mp Tnum, Tden, tN, tD, Vnum, Vden;

int INITDEPTH = 5;
int MAXTHREADS = 12;
int SETUP = FALSE;   // generate threads but do not run them
int cobasislistsize = 0;
string INPUTFILE;
ofstream OUTSTREAM;


plrs_output * reverseList(plrs_output* head){
	plrs_output * last = head, * new_head = NULL;
	while(last) {
      		plrs_output * tmp = last;
      		last = last->next;
      		tmp->next = new_head;
      		new_head = tmp;
    	}
	return new_head;
}


void processCobasis(string cobasis){

	stringstream ss(cobasis);
	string height;
	//split string after h= and retreive height value
	getline(ss, height, '=');
	getline(ss, height, ' ');

	//Check if the cobasis is a leaf node
	if(atoi(height.c_str()) == INITDEPTH){
		//Remove the following characters
		char chars[] = "#VRBh=facetsFvertices/rays";
                unsigned hull = FALSE;
                if (cobasis.compare(0,2,"F#")==0)
                        hull = TRUE;
                    
		for(unsigned int i = 0; i < sizeof(chars); ++i){
			cobasis.erase(remove(cobasis.begin(), cobasis.end(), chars[i]), cobasis.end());
		}
		//Split the string after facets (do not need det, indet etc. for restart)
		ss.str(cobasis);
		getline(ss, cobasis, 'I');
/* 2013.2.14 set depth to zero: hull=F between 3rd and 4th spaces in cobasis;  hull=T 2nd and 3rd*/
                unsigned found = cobasis.find(" ");
                found = cobasis.find(" ",found+1);
                if (hull == FALSE)
                   found = cobasis.find(" ",found+1);
                unsigned found1 = cobasis.find(" ",found+1);
                cobasis.replace(found+1,found1-found-1,"0");


		//Save in cobasis list as a starting point for a thread
		cobasis_list.push(cobasis);
	}
}


void copyFile(string infile, string outfile){
	ifstream input_file (infile.c_str());
	ofstream output_file (outfile.c_str());
	string line;

	if(output_file.is_open()){
		if(input_file.is_open()){
			while(input_file.good()){
				getline(input_file, line);
				output_file<<line<<endl;
			}
			input_file.close();
			output_file.close();
		}else{
			cout<<"Error reading input file!"<<endl;
			exit(1);
		}
	}else{
		cout<<"Error creating temporary file!"<<endl;
		exit(1);
	}
}


void doWork(int thread_number, string starting_cobasis){
	//Create a temporary .ine file 
	char * thread_file = new char[256];
	sprintf(thread_file, "%s_%d.ine", INPUTFILE.c_str(),thread_number);
	copyFile(INPUTFILE, thread_file);
	ofstream out_file (thread_file, ios::app);
/* 2013.2.14 mindepth set to zero */
	out_file<<"mindepth "<< 0 <<endl;
	out_file<<"restart "<<starting_cobasis<<endl;
	out_file.close();

	char * argv[] = { thread_file };
	lrs_main(1, argv);
	//No longer need temporary .ine file so delete it
	if(remove( thread_file ) != 0 ) cout<<"Error deleting thread file!"<<endl;
}



void startThread(int thread_number){

	//keep pulling starting cobasis and do work
	while(true){
		//Get cobasis list lock
		boost::unique_lock<boost::mutex> lock(cobasis_list_mutex);
		//check if starting cobasis left
		if(!cobasis_list.size())
			break;

		//There is a cobasis left store and pop from list
		string starting_cobasis = cobasis_list.front();
		cobasis_list.pop();
		//Release cobasis list lock
		lock.unlock();

		//Begin searching tree with starting cobasis
		doWork(thread_number, starting_cobasis);

	}

	
}

void findInitCobasis(){
	char * argv[] = {"init_temp.ine"};
	lrs_main(1, argv);
	//No longer need temporary ine file so delete it
	if(remove("init_temp.ine") != 0) cout<<"Error deleting init file!"<<endl;
}


void processOutput(){
	// this will atomically pop everything that has been posted so far.
	// consume list is a linked list in 'reverse post order'
	plrs_output* consume_list = output_list.exchange(0,boost::memory_order_acquire);

	//Reverse list since order is important when initializing
	if(initializing){
		consume_list = reverseList(consume_list);		
	}
		

	//proccess the list of output accordingly
	while(consume_list){

		if(consume_list->type == "vertex"){
			OUTSTREAM == NULL ? cout<<consume_list->data<<endl : OUTSTREAM <<consume_list->data<<endl;

		}else if(consume_list->type == "ray"){
			OUTSTREAM == NULL ? cout<<consume_list->data<<endl : OUTSTREAM <<consume_list->data<<endl;

		}else if(consume_list->type =="cobasis"){
			if(initializing){
				//Initializing so process cobasis - if leaf node store in starting cobasis list
				//Note that we will not be piping initial cobasis to output
				processCobasis(consume_list->data);
			}else{
				OUTSTREAM == NULL ? cout<<consume_list->data<<endl : OUTSTREAM <<consume_list->data<<endl;
			}
		}else if(consume_list->type =="V cobasis"){
			if(!initializing){
				OUTSTREAM == NULL ? cout<<consume_list->data<<endl : OUTSTREAM <<consume_list->data<<endl;
			}


		}else if(consume_list->type == "facet count"){
			FACETS += atoi(consume_list->data.c_str());

		}else if(consume_list->type == "ray count"){
			RAYS += atoi(consume_list->data.c_str());

		}else if(consume_list->type == "basis count"){
			BASIS += atoi(consume_list->data.c_str());

		}else if(consume_list->type == "vertex count"){
			VERTICIES += atoi(consume_list->data.c_str());

		}else if(consume_list->type == "integer vertex count"){
			INTVERTICIES += atoi(consume_list->data.c_str());
			
		}else if(consume_list->type == "volume"){
                        const char * c = consume_list->data.c_str();
 			plrs_readrat(Tnum,Tden,c);
                        copy(tN,Vnum);  copy(tD,Vden);
                        linrat(tN,tD,1L,Tnum,Tden,1L,Vnum,Vden);
                       // 	cout << "volume " << prat("",Tnum,Tden) << endl;
                       // 	cout << "tvolume " << prat("",Vnum,Vden) << endl;
			
			
		}else if(consume_list->type == "options warning"){
			//Only pipe warnings if initializing otherwise they are displayed multiple times
			if(initializing){
				OUTSTREAM == NULL ? cout<<consume_list->data<<endl : OUTSTREAM <<consume_list->data<<endl;
			}
		}else if(consume_list->type == "header"){
			//Only pipe headers if initializing otherwise they are displayed multiple times
			if(initializing){
				OUTSTREAM == NULL ? cout<<consume_list->data<<endl : OUTSTREAM <<consume_list->data<<endl;
			}
		}

		//Free memory of current consume_list node
		plrs_output* temp = consume_list->next;
		delete consume_list;
		consume_list = temp;			
	}
}

void consumeOutput() {
	while(!producers_finished) {
		processOutput();
		boost::unique_lock<boost::mutex> lock(consume_mutex);                
		// check one last time while holding the lock before blocking.
		if(!output_list && !producers_finished) consume.wait(lock);
	}
	//Producer thread(s) have finished searching so manage output_list one last time
	processOutput();
}

void notifyProducerFinished(){
	//Get consumer lock
	boost::unique_lock<boost::mutex> lock(consume_mutex);  
	producers_finished = true;
	//notify consumer thread in case it is waiting for producer
	consume.notify_one();
}


void initializeStartingCobasis(){

	cout<<"*Max depth of "<<INITDEPTH<<" to initialize starting cobasis list"<<endl;
        if(OUTSTREAM != NULL)
	    OUTSTREAM <<"*Max depth of "<<INITDEPTH<<" to initialize starting cobasis list"<<endl;
	
	//Copy contents of ine file to temporary file	
	copyFile(INPUTFILE, "init_temp.ine");
	ofstream init_temp_file ("init_temp.ine", ios::app);
	init_temp_file<<"maxdepth "<<INITDEPTH<<endl;
	init_temp_file<<"printcobasis 1"<<endl;
	init_temp_file.close();
	boost::thread consumer_init_thread(consumeOutput);
	boost::thread producer_init_thread(findInitCobasis);

	//Wait for producer thread to find all cobasis at depth <= 1
	producer_init_thread.join();
	//Notify init consumer thread that init producer is finished
	notifyProducerFinished();
	//wait for init consumer thread to finish building starting cobasis list
	consumer_init_thread.join();
	//finished initialization starting cobasis array
	initializing = false;
	producers_finished = false;
	cout<<"*Finished initializing cobasis list with "<<cobasis_list.size()<<" starting cobases"<<endl;
        cobasislistsize = cobasis_list.size();
	
}


int main(int argc, char* argv[]){

//      allocate mp for volume calculation

        lrs_alloc_mp(tN); lrs_alloc_mp(tD); lrs_alloc_mp(Vnum); lrs_alloc_mp(Vden);
        lrs_alloc_mp(Tnum); lrs_alloc_mp(Tden);

        itomp(ZERO,Vnum); itomp(ONE,Vden);

	struct timeval start, end;
	gettimeofday(&start, NULL);

	//Print version
	cout<<"*plrs:"<<TITLE<<VERSION<<"("<<ARITH<<")"<<endl<<AUTHOR<<endl;

	string outputfile;
        int firstfile=1;

	for (int i = 1; i < argc; i++) {
			string arg(argv[i]);
			if (arg == "-in" && i + 1 <= argc) {
				INPUTFILE = string(argv[i + 1]);
                                i++;
			} else if (arg == "-out" && i + 1 <= argc) {
				outputfile = string(argv[i + 1]);
                                i++;
			} else if (arg == "-mt"  && i + 1 <= argc) {
				MAXTHREADS = atoi(argv[i + 1]);
                                i++;
			} else if (arg == "-id"  && i + 1 <= argc){
				INITDEPTH = atoi(argv[i+1]);
                                i++;
			} else if (arg == "-set" ){
				SETUP = TRUE;  // produce threads but do not run them!
                                i++;
			} else {
                                if (firstfile==1){
                                  INPUTFILE=string(argv[i]);
                                  firstfile++;
                                } else if (firstfile==2) {
                                  outputfile=string(argv[i]);
                                  firstfile++;
                                } else {
                                   cout << "Invalid arguments, please try again."<<endl;
			           cout << USAGE <<endl;
				   exit(0);
				}
                              }

        }


	if(INPUTFILE.empty()){
		cout<<"No input file.\n";
		cout<<USAGE<<endl;
		exit(0);
	}

	cout<<"*Input taken from "<<INPUTFILE<<endl;

	if(!outputfile.empty()){
		OUTSTREAM.open(outputfile.c_str());
		if(!OUTSTREAM.is_open()){
			cout<<"Error opening output file!"<<endl;
			exit(0);
		}
		cout<<"*Output written to: "<<outputfile<<endl;
	}
	
        if(OUTSTREAM != NULL)
            {
	     OUTSTREAM <<"*plrs:"<<TITLE<<VERSION<<"("<<ARITH<<")"<<endl<<AUTHOR<<endl;
	     OUTSTREAM <<"*Input taken from "<<INPUTFILE<<endl;
            }
	initializeStartingCobasis();

        if(SETUP == TRUE)
      { // make thread files but do not run any
	//keep pulling starting cobasis and output file
	while(cobasis_list.size()>0){

	string starting_cobasis = cobasis_list.front();
	cobasis_list.pop();

	//Create  .ine file 
	char * thread_file = new char[256];
        int threadnumber = cobasis_list.size();
	sprintf(thread_file, "%s_%d.ine", INPUTFILE.c_str(),threadnumber);
	copyFile(INPUTFILE, thread_file);
	ofstream out_file (thread_file, ios::app);
	out_file<<"mindepth "<< 0 <<endl;
	out_file<<"restart "<<starting_cobasis<<endl;
	out_file.close();

	}

        cout << "*Setup terminates" << endl;
	
      } else {
	//Create one consumer thread to manage output 
	boost::thread consumer_thread(consumeOutput);
	//Create producer threads
	boost::thread_group producer_threads;


	    cout<<"*Starting "<<MAXTHREADS<<" producer thread(s) and 1 consumer thread"<<endl;


	    for(int i = 0; i < MAXTHREADS; i++){
		producer_threads.create_thread(boost::bind(startThread,i));
	    }
	
	    //wait for producer threads to finish searching subtrees
	    producer_threads.join_all();
	    //Notify consumer thread producers are finished
	    notifyProducerFinished();
	    //wait for consumer thread to finish piping output
	    consumer_thread.join();
       }   

	OUTSTREAM == NULL ? cout<<"end"<<endl : OUTSTREAM <<"end"<<endl;;
        if(OUTSTREAM != NULL)
           {
	    OUTSTREAM<<"*Finished initializing cobasis list with "<<cobasislistsize<<" starting cobases"<<endl;
	    OUTSTREAM <<"*Starting "<<MAXTHREADS<<" producer thread(s) and 1 consumer thread"<<endl;
           }
	if(FACETS > 0){
                cout<<prat("*Volume=",Vnum,Vden) << endl ;
		cout<<"*Totals: facets="<<FACETS<<" bases="<<BASIS<< endl ; 
                if (OUTSTREAM != NULL) {
                    OUTSTREAM <<prat("*Volume=",Vnum,Vden) << endl ;
                    OUTSTREAM <<"*Totals: facets="<<FACETS<<" bases="<<BASIS<<endl;
                }
	}else{
		cout<<"*Totals: vertices="<<VERTICIES<<" rays="<<RAYS<<" bases="<<BASIS<< " integer-vertices="<<INTVERTICIES<<endl ; 
                if (OUTSTREAM != NULL)
                    OUTSTREAM<<"*Totals: vertices="<<VERTICIES<<" rays="<<RAYS<<" bases="<<BASIS<< " integer-vertices="<<INTVERTICIES<<endl;
	}

  	gettimeofday(&end, NULL); 
	cout<<"*Elapsed time: "<<end.tv_sec  - start.tv_sec<<" seconds"<<endl;
        if (OUTSTREAM != NULL)
           {
	         OUTSTREAM <<"*Elapsed time: "<<end.tv_sec  - start.tv_sec<<" seconds."<<endl;
           }

        lrs_clear_mp(tN); lrs_clear_mp(tD); lrs_clear_mp(Vnum); lrs_clear_mp(Vden);
        lrs_clear_mp(Tnum); lrs_clear_mp(Tden);

	return 0;
}
