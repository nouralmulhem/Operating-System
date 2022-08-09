#include <stdio.h>      //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

typedef short bool;
#define true 1
#define false 0

#define SHKEY 300

 
///==============================
//don't mess with this variable//
int * shmaddr;                 //
//===============================


struct process
{
    /* data */
    int id;
    int arrival;
    int run;
    int priority;
    int size;
};

enum state {running,waiting};

struct PCB
{
    /* data */
    int id;
    enum state stat;
    int exec_time;
    int remaining_time;
    int waiting_time;
    int TA;
    float WTA;
};

struct msgbuff
{
    long mtype;
    struct process proc;
};

union Semun
{
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT & IPC_SET */
    ushort *array;         /* array for GETALL & SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
    void *__pad;
};

void down(int sem)
{
    struct sembuf p_op;

    p_op.sem_num = 0;
    p_op.sem_op = -1;
    p_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &p_op, 1) == -1)
    {
        perror("Error in down()");
        exit(-1);
    }
}

void up(int sem)
{
    struct sembuf v_op;

    v_op.sem_num = 0;
    v_op.sem_op = 1;
    v_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &v_op, 1) == -1)
    {
        perror("Error in up()");
        exit(-1);
    }
}



// int* shmaddr2;
// int shmid2;
int sem1,sem2,sem3;
int sender;

void shar_mem() {
    key_t key_id;

    key_id = ftok("keyfile", 67);
    sem1 = semget(key_id, 1, 0666 | IPC_CREAT);
    key_id = ftok("keyfile", 68);
    sem2 = semget(key_id, 1, 0666 | IPC_CREAT);
    key_id = ftok("keyfile", 70);
    sem3 = semget(key_id, 1, 0666 | IPC_CREAT);
  
  
    if (sem1 == -1 || sem2 == -1 || sem3 == -1)
    {
        perror("Error in create sem");
        exit(-1);
    }
    

    key_id = ftok("keyfile", 65);
    sender = msgget(key_id, 0666 | IPC_CREAT);
   
    if (sender  == -1)
    {
        perror("Error in create");
        exit(-1);
    }
    printf("Message Queue ID = %d\n", sender );
    

   
}
// void shar_mem2(){
//     int key_id = ftok("keyfile", 66);
//     shmid2 = shmget(key_id, 256, IPC_CREAT | 0666);

//     if (shmid2 == -1)
//     {
//         perror("Error in create");
//         exit(-1);
//     } 
//     else
//         // printf("\nShared memory ID = %d\n", shmid2);

//         shmaddr2 = shmat(shmid2, (void *)0, 0);
//         printf("\nWriter: Shared memory attached at address %p\n", shmaddr2);

// }



/////////////////////////////{ linked list }/////////////////////////////////
typedef struct Node2 {
    int size;
    bool allocated;
    int start;
    int pid;
    struct Node2* next;
 
} Node2;

Node2* newNode2(int s,int i,int p,bool alo)
{
    Node2* temp = (Node2*)malloc(sizeof(Node2));
    temp->size=s;
    temp->allocated=alo;
    temp->start=i;
    temp->pid=p;
    temp->next = NULL;
    return temp;
}



///////////////////////////// { PRIORITY QUEUE IMPLEMENTATION } //////////////////////////


struct pair {
    struct process proc;
    struct PCB entry;
};

typedef struct node {
    struct pair mypair;
    float priorityDepend;
    struct node* next;
 
} Node;
 
Node* newNode(struct pair proce, float p)
{
    Node* temp = (Node*)malloc(sizeof(Node));
    temp->mypair = proce;
    temp->priorityDepend=p;
    temp->next = NULL;
    return temp;
}

struct pair peek(Node** head)
{
    return (*head)->mypair;
}

void dequeue(Node** head)
{
    Node* temp = *head;
    (*head) = (*head)->next;
    free(temp);
}
 
void enqueue(Node** head, struct pair proce, float p)
{
    Node* start = (*head);
 
    Node* temp = newNode(proce,p);
    if((*head) == NULL)
    {
        (*head) = temp;
        return;
    }
 
    if ((*head)->priorityDepend > p) {
        temp->next = *head;
        (*head) = temp;
    }
    else {
 
        while (start->next != NULL &&
            start->next->priorityDepend <= p) {
            start = start->next;
        }
        temp->next = start->next;
        start->next = temp;
    }
}
 
int isEmpty(Node** head)
{
    return (*head) == NULL;
}


///////////////////////////// { PRIORITY QUEUE IMPLEMENTATION } //////////////////////////



int getClk()
{
    return *shmaddr;
}


/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
*/
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        //Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *) shmat(shmid, (void *)0, 0);
}


/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
*/

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}
