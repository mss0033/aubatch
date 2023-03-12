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

/* Menu Thread */
void *menu( void *ptr);  
int cmd_help_menu(int n, char **a);

/* Scheduler Thread */
void *scheduler(void *ptr); 

/* Menu functions definitions referenced from Dr. Qin's code */
void menu_execute(char *line, int isargs);
void policy_check(void);
void show_menu(const char *cmd_name, const char *x[]);
int cmd_dispatch(char *cmd);
int cmd_exit(int nargs, char **args); 
int cmd_fcfs(int n, char **a);
int cmd_help_menu(int n, char **a);
int cmd_priority(int n, char **a);
int cmd_run(int nargs, char **args); 
int cmd_sjf(int n, char **a);

/* Thread definitions */
void *menu( void *ptr );		       /* Handles the menu */
void *scheduler( void *ptr );          /* Handles submissions and scheduling */
void *dispatcher( void *ptr );         /* Handles job execution */
void *execv_call( struct queue *param ); /* Handles processes using execv() */

#endif