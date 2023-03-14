#ifndef AUBATCH_H_
#define AUBATCH_H_ 

/* Structure of queue */
struct queue
{
    char name[10];
    int burst_time;
    int priority;
};

/* Dispatcher Thread*/
void *dispatcher(void *ptr);      
extern unsigned int lock;
extern int policy;

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

#endif