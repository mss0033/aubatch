#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#ifndef AUBATCH_H_
#define AUBATCH_H_ 

#define FCFS     0
#define SJF      1
#define PRIORITY 2

#define WAITING  0
#define RUNNING  1
#define COMPLETE 2

/* Structure of queue */
struct job
{
    char name[10];
    int status;
    int cpu_time;
    int priority;
    time_t arrival_time;
    time_t start_time;
    time_t complete_time;
};

/* Dispatcher Thread*/
void *dispatcher(void *ptr);
extern unsigned int policy;

/* Executor Thread */
void *execv_call(struct job *new_job);

/* Menu Thread */
void *menu(void *ptr);
int display_help_menu(int nargs, char **args);

/* Scheduler Thread */
void *scheduler(void *ptr); 

/* Menu functions definitions referenced from Dr. Qin's code */
void enforce_policy(void);
int execute_command(char *command);
int fcfs(int nargs, char **args);
int list(int nargs, char **args);
int priority(int nargs, char **args);
int run(int nargs, char **args); 
int sjf(int nargs, char **args);
int quit(int nargs, char **args); 
int test(int nargs, char **args);

/* MISC functions */
int get_num_jobs_in_queue();
int get_job_status_as_string(struct job *job, char* status_string);
int display_job_queue_info(struct job *job_to_display);
int get_scheduling_policy_as_string(char* policy_string);
int print_stats();

#endif