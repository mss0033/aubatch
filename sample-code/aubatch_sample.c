/* 
 * COMP7500/7506
 * Project 3: AUbatch - A Batch Scheduling System
 *
 * Xiao Qin
 * Department of Computer Science and Software Engineering
 * Auburn University
 * Feb. 20, 2018. Version 1.1
 *
 * This sample source code demonstrates the development of 
 * a batch-job scheduler using pthread.
 *
 * Compilation Instruction: 
 * gcc pthread_sample.c -o pthread_sample -lpthread
 *
 * Learning Objecties:
 * 1. To compile and run a program powered by the pthread library
 * 2. To create two concurrent threads: a scheduling thread and a dispatching thread 
 * 3. To execute jobs in the AUbatch system by the dispatching thread
 * 4. To synchronize the two concurrent threads using condition variables
 *
 * How to run aubatch_sample?
 * 1. You need to compile another sample code: process.c
 * 2. The "process" program (see process.c) takes two input arguments
 * from the commandline
 * 3. In aubtach: type ./process 5 10 to submit program "process" as a job.
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>

typedef unsigned int u_int; 

//#define LOW_ARRIVAL_RATE /* Long arrivel-time interval */
#define LOW_SERVICE_RATE   /* Long service time */

/* 
 * Static commands are submitted to the job queue.
 * When comment out the following macro, job are submitted by users.
 */
//#define STATIC_COMMAND 

#define CMD_BUF_SIZE 10 /* The size of the command queueu */
#define NUM_OF_CMD   5  /* The number of submitted jobs   */
#define MAX_CMD_LEN  512 /* The longest commandline length */

/* 
 * When a job is submitted, the job must be compiled before it
 * is running by the executor thread (see also executor()).
 */
void *commandline( void *ptr ); /* To simulate job submissions and scheduling */
void *executor( void *ptr );    /* To simulate job execution */

pthread_mutex_t cmd_queue_lock;  /* Lock for critical sections */
pthread_cond_t cmd_buf_not_full; /* Condition variable for buf_not_full */
pthread_cond_t cmd_buf_not_empty; /* Condition variable for buf_not_empty */

/* Global shared variables */
u_int buf_head;
u_int buf_tail;
u_int count;
char *cmd_buffer[CMD_BUF_SIZE];

int main() {
    pthread_t command_thread, executor_thread; /* Two concurrent threads */
    char *message1 = "Command Thread";
    char *message2 = "Executor Thread";
    int  iret1, iret2;

    /* Initilize count, two buffer pionters */
    count = 0; 
    buf_head = 0;  
    buf_tail = 0; 

    /* Create two independent threads:command and executors */
    iret1 = pthread_create(&command_thread, NULL, commandline, (void*) message1);
    iret2 = pthread_create(&executor_thread, NULL, executor, (void*) message2);

    /* Initialize the lock the two condition variables */
    pthread_mutex_init(&cmd_queue_lock, NULL);
    pthread_cond_init(&cmd_buf_not_full, NULL);
    pthread_cond_init(&cmd_buf_not_empty, NULL);
     
    /* Wait till threads are complete before main continues. Unless we  */
    /* wait we run the risk of executing an exit which will terminate   */
    /* the process and all threads before the threads have completed.   */
    pthread_join(command_thread, NULL);
    pthread_join(executor_thread, NULL); 

    printf("command_thread returns: %d\n",iret1);
    printf("executor_thread returns: %d\n",iret1);
    exit(0);
}

/* 
 * This function simulates a terminal where users may 
 * submit jobs into a batch processing queue.
 * Note: The input parameter (i.e., *ptr) is optional. 
 * If you intend to create a thread from a function 
 * with input parameters, please follow this example.
 */
void *commandline(void *ptr) {
    char *message;
    char *temp_cmd;
    u_int i;
    char num_str[8];
    size_t command_size;
     
    message = (char *) ptr;
    printf("%s \n", message);

    /* Enter multiple commands in the queue to be scheduled */
    for (i = 0; i < NUM_OF_CMD; i++) {    
        /* lock the shared command queue */
        pthread_mutex_lock(&cmd_queue_lock);
 
        printf("In commandline: count = %d\n", count);
        while (count == CMD_BUF_SIZE) {
            pthread_cond_wait(&cmd_buf_not_full, &cmd_queue_lock);
        }

        /* Dynamically create a buffer slot to hold a commandline */
#ifdef STATIC_COMMAND
        cmd_buffer[buf_head] = malloc(strlen("process -help -time ") + 1);
        strcpy(cmd_buffer[buf_head], "./process -help -time "); 
        sprintf(num_str, "%d", i);
        strcat(cmd_buffer[buf_head], num_str);
#else
        pthread_mutex_unlock(&cmd_queue_lock);

        printf("Please submit a batch processing job:\n");
        printf(">"); 
        temp_cmd = malloc(MAX_CMD_LEN*sizeof(char));
        getline(&temp_cmd, &command_size, stdin);  
        pthread_mutex_lock(&cmd_queue_lock);    
        cmd_buffer[buf_head]= temp_cmd; 
        
#endif
        printf("In commandline: cmd_buffer[%d] = %s\n", buf_head, cmd_buffer[buf_head]);  
    
        count++;
 
        /* Move buf_head forward, this is a circular queue */ 
        buf_head++;
        if (buf_head == CMD_BUF_SIZE)
            buf_head = 0;

        pthread_cond_signal(&cmd_buf_not_empty);  
        /* Unlok the shared command queue */
        pthread_mutex_unlock(&cmd_queue_lock);

        /* Simulate a low arrival rate */
#ifdef LOW_ARRIVAL_RATE
        sleep(2); /* Simulate an arrival time of 2 seconds */
#endif
    } /* end for */
}

/*
 * This function simulates a server running jobs in a batch mode.
 * Note: The input parameter (i.e., *ptr) is optional. 
 * If you intend to create a thread from a function 
 * with input parameters, please follow this example.
 */
void *executor(void *ptr) {
    char *message;
    u_int i;

    message = (char *) ptr;
    printf("%s \n", message);

    for (i = 0; i < NUM_OF_CMD; i++) {
        /* lock and unlock for the shared process queue */
        pthread_mutex_lock(&cmd_queue_lock);
        printf("In executor: count = %d\n", count);
        while (count == 0) {
            pthread_cond_wait(&cmd_buf_not_empty, &cmd_queue_lock);
        }

        /* Run the command scheduled in the queue */
        count--;
        printf("In executor: cmd_buffer[%d] = %s\n", buf_tail, cmd_buffer[buf_tail]); 
        
        /* 
         * Note: system() function is a simple example.
         * You should use execv() rather than system() here.
         */
        system(cmd_buffer[buf_tail]); 
        /* Free the dynamically allocated memory for the buffer */
        free(cmd_buffer[buf_tail]);

#ifdef LOW_SERVICE_RATE
        sleep(2); /* Simulate service time of 2 seconds */
#endif
     
        /* Move buf_tail forward, this is a circular queue */ 
        buf_tail++;
        if (buf_tail == CMD_BUF_SIZE)
            buf_tail = 0;

        pthread_cond_signal(&cmd_buf_not_full);
        /* Unlok the shared command queue */
        pthread_mutex_unlock(&cmd_queue_lock);
    } /* end for */
}
