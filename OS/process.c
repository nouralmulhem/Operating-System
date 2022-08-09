#include "headers.h"

/* Modify this file as needed*/
int remainingtime;
int remainingtime_temp;
void handler(int signum);
void handler2(int signum);
int start;
int c ;
int resume;
int main(int agrc, char * argv[])
{
  
    initClk();
    // shar_mem();
    // shar_mem2();
    signal(SIGCONT,handler);
    signal(SIGTSTP,handler2);
    remainingtime = atoi(argv[1]);
    remainingtime_temp=remainingtime;
    //TODO it needs to get the remaining time from somewhere
    //remainingtime = ??;
     c = getClk();
     start=getClk();
    while (remainingtime > 0)
    {
        if(getClk()-c>=1)
        {
            remainingtime--;
            printf("hello from proc: %d the remaining time is: %d the time is : %d\n",getpid(),remainingtime,getClk());
            fflush(stdout);
            c=getClk();
        }
    }
    kill(getppid(),SIGUSR2);
    destroyClk(false);
    return 0;
}
void handler(int signum){
    c = getClk();
    start=getClk();
    remainingtime_temp=remainingtime;
}
void handler2(int signum){
    remainingtime=remainingtime_temp;
    remainingtime -=(getClk()-start);
    // *shmaddr2 = remainingtime; /* initialize shared memory */
    // up(sem3);
    kill(getpid(), SIGSTOP);
}