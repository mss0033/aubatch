#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#ifndef AUBATCH_H_
#define AUBATCH_H_ 

/* Structure of queue */
struct queue
{
    char name[10];
    int cpu_time;
    int priority;
    time_t arrival_time;
    time_t run_time;
    time_t complete_time;
};

/* Dispatcher Thread*/
void *dispatcher(void *ptr);
extern unsigned int policy;

/* Executor Thread */
void *execv_call(struct queue *param);

/* Menu Thread */
void *menu(void *ptr);
int display_help_menu(int n, char **a);

/* Scheduler Thread */
void *scheduler(void *ptr); 

/* Menu functions definitions referenced from Dr. Qin's code */
void enforce_policy(void);
int execute_command(char *command);
int fcfs(int nargs, char **a);
int priority(int n, char **a);
int run(int nargs, char **args); 
int sjf(int n, char **a);
int quit(int nargs, char **args); 

/* MISC functions */
int get_num_jobs_in_queue();
int print_stats();

#endif