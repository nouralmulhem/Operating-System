#include "headers.h"

void clearResources(int);

union Semun semun;
int send_val;

int main(int argc, char *argv[]) {
    signal(SIGINT, clearResources);
    // TODO Initialization
    shar_mem();

    semun.val = 0; /* initial value of the semaphore, Binary semaphore */
    if (semctl(sem1, 0, SETVAL, semun) == -1)
    {
        perror("Error in semctl");
        exit(-1);
    }
    if (semctl(sem2, 0, SETVAL, semun) == -1)
    {
        perror("Error in semctl");
        exit(-1);
    }
    if (semctl(sem3, 0, SETVAL, semun) == -1)
    {
        perror("Error in semctl");
        exit(-1);
    }
    int sch_num;
    int quanta = -1;
    FILE *ptr;
    char ch;
    int lines = 0;

    struct msgbuff message;

    // ============================================================================================================== //
    // 1. Read the input files.
    ptr = fopen("processes.txt", "r");
    char line[256];

    // loop to count line
    while (!feof(ptr))
    {
        ch = fgetc(ptr);
        if (ch == '\n')
        {
            lines++;
        }
        else if(ch=='#'){ // if comment between lines
            fgets(line, sizeof(line)+1, ptr);
            continue;
        }
    }
    struct process proc_arr[lines];
    fclose(ptr);
    ptr = fopen("processes.txt", "r");
    // ;
    int dec;
    int i = 0;

    // loop to read file
    while (fscanf(ptr, "%d", &dec) != EOF )
    {
        fscanf(ptr,"%c",&ch);
        if(ch=='#'){
            fgets(line, sizeof(line), ptr);
            continue;
        }
        proc_arr[i].id = dec;
        fscanf(ptr, "%d", &proc_arr[i].arrival);
        fscanf(ptr, "%d", &proc_arr[i].run);
        fscanf(ptr, "%d", &proc_arr[i].priority);
        fscanf(ptr, "%d", &proc_arr[i].size);
        i++;
    }
    fclose(ptr);




    // ============================================================================================================== //
    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    printf("enter the choosen scheduling algorithm\nchoose\t 0 for HPF\n \t 1 for SRTN\n \t 2 for RR\n");
    scanf("%d", &sch_num);
    while (sch_num != 0 && sch_num != 1 && sch_num != 2) // loop if input is not one of the three algo
    {
        printf("Please Enter 0 || 1 || 2\n");
        scanf("%d", &sch_num);
    }
    if (sch_num == 2) // the chosen algorithm is Round Robin
    {
        printf("Please enter the Quanta in seconds: ");
        scanf("%d", &quanta);
        printf("\n");
    }





    // ============================================================================================================== //
    // 3. Initiate and create the ((((((      scheduler and clock processes.     ))))))
    int pid, stat_loc;

    pid = fork();
    int sch_id=0; 
    char str2[256];
    sch_id=pid;

    if (pid == -1)
        perror("error in fork");

    else if (pid == 0)
    {   
        char str3[256];
        char str4[256];
        char str5[256];
        sprintf(str3, "%d", sch_num);
        sprintf(str4, "%d", quanta);
        sprintf(str5, "%d", lines);
        // fork scheduler process
        execl("/home/os/Desktop/code/scheduler.out", "./scheduler.out",str3,str4,str5, NULL);
    }
    else
    {
        pid = fork();
        if (pid == 0)
        {
            // fork clock process
            execl("/home/os/Desktop/code/clk.out", "./clk.out", NULL);
        }
    }



    // ============================================================================================================== //
    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this
    int x = getClk();
    printf("current time is %d\n", x);
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
            // DS WILL BE IN SCH (WE ASKED MAHEED)


    

    // ============================================================================================================== //
    // 6. Send the information to the scheduler at the appropriate time.
    int counter=0;

    down(sem1);
    while(counter<lines)
    {
        x =getClk();
        if(proc_arr[counter].arrival==x)
        {
            // if when it is the arrival time of a process then send a signal to the scheduler that a process arrived
            // and send its information through the message queue
            message.proc = proc_arr[counter];
            kill(sch_id,SIGUSR1);
            send_val = msgsnd(sender, &message, sizeof(message.proc), !IPC_NOWAIT);
            if (send_val == -1)
                perror("Errror in send");
            counter++;
            down(sem1); // may be moved to next before signal is finished and process added to the queue
        }
    }
    pid = wait(&stat_loc);
    // 7. Clear clock resources
    destroyClk(true);
    return 0;
}

void clearResources(int signum) // clear :)
{
    printf("\nAbout to destroy the shared memory area !\n");
    fflush(stdout);
    msgctl(sender, IPC_RMID, (struct msqid_ds *)0);
    // shmctl(shmid2, IPC_RMID, (struct shmid_ds *)0); 
    semctl(sem1, 0,IPC_RMID, semun);
    semctl(sem2, 0,IPC_RMID, semun);
    semctl(sem3, 0,IPC_RMID, semun);
    exit(1);
    //TODO Clears all resources in case of interruption
}