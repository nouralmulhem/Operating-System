#include "headers.h"

void handler(int signum);
void handler2(int signum);
void HPF();
void RR(int);
void SRTN();
bool allocate_proc(int size,int pid);
void deallocate_proc(int pid, int size);
void split(Node2*,int);
void print_linked_list();
void merge();

Node2* memo;
Node* waiting_queue;
Node* queue;
int sch_num;
FILE* ptr;
FILE* ptr2;
struct pair* mypair;

int current_num_of_proc = 0;
int total_running = 0;
float avg_wta = 0;
float avg_for_std_wta = 0;
float avg_waiting = 0;
float std_wta = 0;
bool finish=false;
// struct PCB* process_table;
int main(int argc, char* argv[])
{
	initClk();
	shar_mem();
	memo=newNode2(1024,0,-1,false);
	// declare of memory first as a node with size 1024

	sch_num = atoi(argv[1]);
	int quanta = atoi(argv[2]);
	int num_of_proc = atoi(argv[3]);
	mypair = (struct pair*)malloc(sizeof(struct pair));


	up(sem1);
	// so that we dont receive signal of process arrival before 
	// declaring the message queue 


	// we got two different types of signals
	signal(SIGUSR1, handler); // process arival
	signal(SIGUSR2, handler2); // process finish

	ptr2 = fopen("memory.log", "w");
	fprintf(ptr2, "#At time x allocated y bytes for process z from i to j\n");

	ptr = fopen("scheduler.log", "w");
	fprintf(ptr, "#At time x process y state arr w total z remain y wait k\n");

	// so if queue is empty at any instance and didn't 
	// get all processes from process generator wait in the loop till get the next process
	while (current_num_of_proc != num_of_proc) 
	{
		switch (sch_num) // switch case :)
		{
		case 0:
			HPF();
			break;
        case 1:
            SRTN();
            break;
        case 2:
            RR(quanta);
            break;
		default:
			break;
		}
	}


	fclose(ptr);
	fclose(ptr2);
	free(mypair);

	ptr = fopen("scheduler.perf", "w");
	fprintf(ptr, "CPU utilization = %.2f%%\n", total_running * 100 / ((float)getClk()));
	avg_wta = avg_wta / num_of_proc;
	fprintf(ptr, "Avg WTA = %.2f\n", avg_wta);

	avg_waiting = avg_waiting / num_of_proc;
	fprintf(ptr, "Avg Waiting = %.2f\n", avg_waiting);

	std_wta = (avg_for_std_wta - (num_of_proc * avg_wta * avg_wta)) / (num_of_proc - 1);
	std_wta = sqrtf(std_wta);
	fprintf(ptr, "Std WTA = %.2f\n", std_wta);

	fclose(ptr);

	printf("finish\n");
	fflush(stdout);

	//TODO implement the scheduler :)
	//upon termination release the clock resources.

	destroyClk(true);

}

// #include <sys/time.h>
// long long int current_timestamp() {
// 	struct timeval te;
// 	gettimeofday(&te, NULL); // get current time.
// 	long long int milliseconds = te. tv_sec*1000LL + te. tv_usec/1000; // calculate milliseconds.
// 	// printf("milliseconds: %lld\n", milliseconds);
// 	return milliseconds;
// }


void handler(int signum)		// process arival
{
	struct msgbuff message;
	int rec_val = msgrcv(sender, &message, sizeof(message.proc), 0, !IPC_NOWAIT);

	if (rec_val == -1)
		perror("Error in receive");
	else
		printf("Message received at time %d\n", getClk());

	
	// intialize the data of process and its entry in process table
	struct pair mypair;
	mypair.proc.id = message.proc.id;
	mypair.proc.arrival = message.proc.arrival;
	mypair.proc.run = message.proc.run;
	mypair.proc.priority = message.proc.priority;
	mypair.proc.size = message.proc.size;

	mypair.entry.stat = waiting;
	mypair.entry.exec_time = -1;
	mypair.entry.remaining_time = mypair.proc.run;
	mypair.entry.waiting_time = 0;
	mypair.entry.TA = 0;
	mypair.entry.WTA = 0;


	bool have_size=allocate_proc(mypair.proc.size,mypair.proc.id);
	// try to allocate the process in the memory and if faild
	// move the process to the waiting queue

	if(have_size) {
		switch (sch_num)
		{
		case 0:
			enqueue(&queue, mypair, mypair.proc.priority);
			break;
		case 1:
			enqueue(&queue, mypair, mypair.proc.run); 
			break;
		case 2:
			enqueue(&queue, mypair, getClk()); 
			// we assumed here that priority is the time at 
			// which the process arrive or continue to work 
			break;
		default:
			break;
		}
 	}
	else {
		switch (sch_num)
		{
		case 0:
			enqueue(&waiting_queue, mypair, mypair.proc.priority);
			break;
		case 1:
			enqueue(&waiting_queue, mypair, mypair.proc.run);
			break;
		case 2:
			enqueue(&waiting_queue, mypair, getClk());
			break;
		default:
			break;
		}
 	}
	up(sem1);
}

void handler2(int signum)		// process finish
{
	int stat_loc;
	int pid=wait(&stat_loc); // we wait so the process dont be zombie in the memory
	// long long int c=current_timestamp();
	finish=true;
	mypair->entry.TA = getClk() - mypair->proc.arrival; 
	mypair->entry.WTA = (getClk() - mypair->proc.arrival) / ((float)mypair->proc.run);
	fprintf(ptr, "At time %d process %d finished arr %d total %d remain 0 wait %d TA %d WTA %.2f\n", getClk(), mypair->proc.id, mypair->proc.arrival, mypair->proc.run, mypair->entry.waiting_time, mypair->entry.TA, mypair->entry.WTA);
	// for the output .perf
	total_running += mypair->proc.run;
	avg_waiting += (float)mypair->entry.waiting_time;
	avg_wta += mypair->entry.WTA;
	avg_for_std_wta += mypair->entry.WTA * mypair->entry.WTA; 
	current_num_of_proc++;
	deallocate_proc(mypair->proc.id, mypair->proc.size);
	Node* temp=waiting_queue;
	Node* prev =NULL;
	while(temp != NULL){
		// try to allocate process in the freed space 
		if(allocate_proc(temp->mypair.proc.size,temp->mypair.proc.id)){
			switch (sch_num)
			{
				case 0:
					enqueue(&queue, temp->mypair, temp->mypair.proc.priority);
					break;
				case 1:
					enqueue(&queue, temp->mypair, temp->mypair.proc.run);
					break;
				case 2:
					enqueue(&queue, temp->mypair, getClk());
					break;
				default:
					break;
			}
			if(temp==waiting_queue){
				dequeue(&waiting_queue);
				temp=waiting_queue;
			}
			else
			{ 
				prev->next=temp->next;
				temp->next = NULL;
				Node* del=temp;
				temp =prev->next;
				free(del);
			}
		} else { // serach and try to allocate anothe process
			prev=temp;
			temp = temp->next;
		}
	}
	// for the output .perf
	/*
	insert from waiting queue to queue
	*/
	// printf(" ------------------------------------------------- %lld -----------------------------------\n",current_timestamp()-c);

}

void HPF()
{
	while (!isEmpty(&queue)) // loop untill finish the ready queue
	{
		*mypair = peek(&queue); /// proc entry
		dequeue(&queue);
		printf("we are running proc %d at time %d\n", mypair->proc.id, getClk());
		char str5[256];
		int stat_loc;
		mypair->entry.waiting_time = getClk() - mypair->proc.arrival;
		mypair->entry.stat = running;
		mypair->entry.exec_time = getClk();
		fprintf(ptr, "At time %d process %d started arr %d total %d remain %d wait %d\n", mypair->entry.exec_time, mypair->proc.id, mypair->proc.arrival, mypair->proc.run, mypair->entry.remaining_time, mypair->entry.waiting_time);
		int pid = fork();
		mypair->entry.id=pid;
		if (pid == -1)
			perror("error in forking\n");
		else if (pid == 0)
		{
			char str3[256];
			sprintf(str3, "%d", mypair->entry.remaining_time);
			// run real cpu bound process
			execl("/home/os/Desktop/code/process.out", "./process.out", str3, NULL);
		}
		else {
			while(!finish){
				// wait untill process finish 
			}
			finish =false;
		}
	}
}

void RR(int quanta) {

	while (!isEmpty(&queue)) { // loop untill finish the ready queue
		*mypair = peek(&queue); /// proc entry
		dequeue(&queue);
		finish=false;
		mypair->entry.stat = running;
		if (mypair->entry.exec_time == -1)
		{
			//here the proc will start not resumed so we need to calc waiting time
			int pid = fork();
			if (pid == 0) {

				char temp[50];
				sprintf(temp, "%d", mypair->proc.run);
				// run real cpu bound process
				execl("/home/os/Desktop/code/process.out", "./process.out", temp, NULL);
			}
			mypair->entry.id = pid;
			mypair->entry.exec_time = 0;
			mypair->entry.waiting_time = getClk() - mypair->proc.arrival;
			fprintf(ptr, "At time %d process %d started arr %d total %d remain %d wait %d \n",
				getClk(), mypair->proc.id, mypair->proc.arrival, mypair->proc.run
				, mypair->entry.remaining_time, mypair->entry.waiting_time);
				// we need to print in the out file here
		}
		else {
			// here is process was started and stopped before so continue it
			kill(mypair->entry.id, SIGCONT);
			fprintf(ptr, "At time %d process %d resumed arr %d total %d remain %d wait %d \n",
				getClk(), mypair->proc.id, mypair->proc.arrival, mypair->proc.run
				, mypair->entry.remaining_time, mypair->entry.waiting_time);
				// we need to print in the out file here
		}
		int time = getClk();
		if (mypair->entry.remaining_time <= quanta)
		{
			// handle the last running of process
			int c=getClk();
			while (!finish) { 
				// scheduler will wait until
				// cpu bound process will inform the scheduler
				// after finishing its last quanta 
			}
			mypair->entry.remaining_time = 0;
		}
		else
		{
			// run the quanta for each process
			while ((time + quanta) > getClk()) {

			}
			mypair->entry.remaining_time -= quanta;
		}
		if(!finish ){
			// the process stopped but not finished due to finishing its quanta
			kill(mypair->entry.id, SIGTSTP);
			fprintf(ptr, "At time %d process %d stoped arr %d total %d remain %d wait %d \n" ,
				getClk(), mypair->proc.id, mypair->proc.arrival, mypair->proc.run
				, mypair->entry.remaining_time, mypair->entry.waiting_time);
			mypair->entry.stat = waiting;
			enqueue(&queue, *mypair, getClk()+.1);
		}
	}
}

void SRTN(){
	while (!isEmpty(&queue)) { // loop untill finish the ready queue
		*mypair = peek(&queue);
		dequeue(&queue);
		finish=false;
		if (mypair->proc.run == mypair->entry.remaining_time) {
			//here the proc will start not resumed so we need to calc waiting time
			int pid = fork();
			if (pid == 0) {
				char temp[50];
				sprintf(temp, "%d", mypair->proc.run); 
				// run real cpu bound process
				execl("/home/os/Desktop/code/process.out", "./process.out", temp, NULL);
			}
			mypair->entry.id = pid;
			mypair->entry.waiting_time = getClk() - mypair->proc.arrival;

			fprintf(ptr, "At time %d process %d started arr %d total %d remain %d wait %d \n",
				getClk(), mypair->proc.id, mypair->proc.arrival, mypair->proc.run
				, mypair->entry.remaining_time, mypair->entry.waiting_time);
			// we need to print in the out file here
		}
		else {
			// here is process was started and stopped before so continue it
			kill(mypair->entry.id, SIGCONT);
			fprintf(ptr, "At time %d process %d resumed arr %d total %d remain %d wait %d \n",
				getClk(), mypair->proc.id, mypair->proc.arrival, mypair->proc.run
				, mypair->entry.remaining_time, mypair->entry.waiting_time);
			// we need to print in the out file here
		}
		// second step is work with one clock and check if there is another proc with heigher priority
		mypair->entry.stat = running;
		while (!finish)
		{
			// loop until process finish and save it and send SIGUSR1 
			// OR find another process with smaller remaining time 
			if (!isEmpty(&queue) && mypair->entry.remaining_time > peek(&queue).entry.remaining_time) {
				break;
			}
			int curent_time = getClk();
			bool test=false;
			while (getClk() < curent_time + 1) {
				// loop until finish one second OR get process with smaller remaining time at the begining of the second 
				if (finish || (!isEmpty(&queue) && mypair->entry.remaining_time > peek(&queue).entry.remaining_time)) {
					test=true;
					break;
				}
			}
			if(test)
			{
				break;
			}
			mypair->entry.remaining_time--;
		}

		if (!finish)
		{
			// the process stopped but not finished due to finding another one with higher priority
			fprintf(ptr, "At time %d process %d stoped arr %d total %d remain %d wait %d \n",
				getClk(), mypair->proc.id, mypair->proc.arrival, mypair->proc.run
				, mypair->entry.remaining_time, mypair->entry.waiting_time);
			mypair->entry.stat = waiting;
			enqueue(&queue, *mypair, mypair->entry.remaining_time);
			kill(mypair->entry.id, SIGTSTP);
		}

	}
}

bool allocate_proc(int size,int pid){
	// memo
	Node2* temp=memo;
	Node2* min=NULL;
	bool insert;
	while(temp != NULL){
		// loop on the linked list to find the perfect position with the following properties 
		// 1- not allocated
		// 2- has higher size than OR equal the process
		// 3- the smallest one which follows the first two conditions
		if(! temp->allocated && temp->size >= size ){
			if(min == NULL || min->size > temp->size){
				min =temp;
			}
		}
		temp=temp->next;
	}
	
	if(min!=NULL){
		// if find a block of memory with the two conditions 
		// then allocate the process in two steps
		temp=min;
		///////// 1- resize the memory block to perfect the process
		split(temp , size);
		///////// 2- allocate the rocess in the memory
		temp->pid=pid;
		temp->allocated=true;
		print_linked_list();
		fprintf(ptr2, "At time %d allocated %d bytes for process %d from %d to %d\n",getClk(),size,temp->pid,temp->start,temp->start+temp->size-1);
		return true;
	}else{
		print_linked_list();
		return false;
	}
	
}
void split(Node2* temp , int prc_size){
	if(temp->size < prc_size){
		// the base condition is the size of memory less than process then merge last 
		// two parts into one part and allocate memory in it 
		Node2* temp2=temp->next;
		temp->next=temp2->next;
		temp->size = temp->size * 2;
		printf("the size of the mempory is %d  and the start position is %d and the proc size is %d \n"
		,temp->size,temp->start,prc_size);
		fflush(stdout);
		free( temp2);
		return;
	}else{
		// split the block of the memory in two parts and recall on the left part
		Node2* temp2=newNode2((temp->size)/2,( temp->start + temp->size + temp->start ) / 2,-1,false);
		temp2->next=temp->next;
		temp->next=temp2;
		temp->size = temp->size/2;
		split(temp,prc_size);
	}
}
void deallocate_proc(int pid, int size){ //memo
	printf("started dealo\n");
	fflush(stdout);
	Node2* temp=memo;
	while(temp!=NULL){
		// search on the block of the memory which have this process 
		// and free it 
		if(temp->pid==pid){
			fprintf(ptr2, "At time %d freed %d bytes for process %d from %d to %d\n",getClk(),size,temp->pid,temp->start,temp->start+temp->size-1);
			temp->allocated=false;
			temp->pid=-1;
			break;
		}
		temp=temp->next;
		}
		// try to merge any two blocks after deallocating 
		merge();
	
	print_linked_list();
}

void print_linked_list(){
	// loop on each block of the memory and print its data 
	Node2* temp=memo;
	while(temp != NULL){
		printf("started at %d , the size is %d ,ended at position %d ,allocated is %o , the procees id is %d \n"
		, temp->start,temp->size,temp->start + temp->size,temp->allocated,temp->pid);
		temp=temp->next;
	}
	printf("---------------------------\n");
	fflush(stdout);
}

void merge(){
	Node2* temp=memo;
	bool done=false;
	while(temp!=NULL && temp->next!=NULL){
		// loop on the memory and try to find any two blocks with the following properties:
		/*

			1- be consequetive
			2- equal in size
			3- both are free 
			4- the starting position of the first one is even multiple of the size of any 

		*/
		if(!temp->next->allocated && !temp->allocated 
		&& temp->size==temp->next->size && ( temp->start / temp->size ) % 2 ==0){
			
			temp->size=temp->size*2;
			Node2* temp2=temp->next->next;
			free(temp->next);
			temp->next= temp2;
			done=true;
		}
		temp=temp->next;
	}

	// if we did achive last 4 conditions and could merge any two blocks
	// then recall the merge 
	if(done)
		merge();
}