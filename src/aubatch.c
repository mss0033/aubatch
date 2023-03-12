/*
 * 
 */

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "aubatch.h"

/* Command Line Processing */
#define NUM_OF_CMD   5   /* The number of submitted jobs   */
#define CMD_BUF_SIZE 10  /* The size of the command queueu */
#define MAX_CMD_LEN  512 /* The longest scheduler length */
/* Error Codes */
#define INVALID  1
#define OVERFLOW 2
/* Maximums */
#define MAXARGS    4 
#define MAXCMDLGTH 64    

/* Global shared variables */
/* Mutexes and CVs */
pthread_mutex_t cmd_queue_mutex;     /* Mutex for critical sections */
pthread_mutex_t cmd_queue_run_mutex; /* Mutext for menu to scheduler */
pthread_cond_t cmd_buffer_not_full;  /* Buffer not full CV  */
pthread_cond_t cmd_buffer_not_empty; /* Buffer not empty CV */
pthread_cond_t job_submit;	        /* Job submit CV */

unsigned int buffer_head;
unsigned int buffer_tail;
unsigned int count;
unsigned int lock = 0;
int policy = 0;

char cmd[30];
char *param_list[3];

struct queue *cmd_buffer[CMD_BUF_SIZE];

/*
 *
 */
void enforce_policy(void)
{
	/* Function to apply proper sorting in job queue */
	struct queue *key;
	int i = 0,j;
	
	/* Check the scheduling  */
	switch(policy)
	{
		/* FCFS */
		case 0:
			break;
		/* Sorting Job First */
		case 1:
			for(i=0;i<buffer_head-1;i++)
			{
				for(j=0;j<buffer_head-i-1;j++)
				{
					if(cmd_buffer[j]->burst_time > cmd_buffer[j+1]->burst_time)
					{
						key = cmd_buffer[j];
						cmd_buffer[j] = cmd_buffer[j+1];
						cmd_buffer[j+1] = key;
					} 
				}	
			}
			break;
		/* Sorting Priority */
		case 2:
			for(i=0;i<buffer_head-1;i++)
			{
				for(j=0;j<buffer_head-i-1;j++)
				{
					if(cmd_buffer[j]->priority > cmd_buffer[j+1]->priority)
					{
						key = cmd_buffer[j];
						cmd_buffer[j] = cmd_buffer[j+1];
						cmd_buffer[j+1] = key;
					} 
				}	
			}
			break;
		/* Unknown */
		default:
			break;	
	}
}

/*
 *
 */
void *execv_call(struct queue *param)
{
	char *arg[5];
	pid_t pid;

	arg[0] = "process";
	arg[1] = param->name;

	sprintf(arg[2],"%d",param->burst_time);
	sprintf(arg[3],"%d",param->priority);

	arg[4] = NULL;

	/*Fork() to host an execv() process*/
	switch ((pid = fork()))
  	{
   		case -1:
   	 		perror("fork failed");
      		break;
    	case 0:
			/* Execution of execv() */
			if(execv("process",arg)==-1)
			{ 
				
				perror("Fail\n");
				exit(0);
			}
      		break;
    	default:
			pthread_mutex_unlock(&cmd_queue_mutex);
			sleep(atoi(arg[2])); /* Make this process last till process finishes */
			pthread_mutex_lock(&cmd_queue_mutex);
      		break;
  	}
	return 0;
}

/*
 *
 */
int cmd_run(int nargs, char **args) 
{
	int i=1;
	int counter = nargs;

	pthread_mutex_lock(&cmd_queue_run_mutex);

	if (nargs != 4) 
	{
		printf("Usage: run <job> <time> <priority>\n");
		return INVALID;
	}

	counter--;
	strcpy(cmd, "./process");

	while(counter>0)
	{
		param_list[i-1] = args[i];
		strcat(cmd, param_list[i-1]);
		strcat(cmd," ");
		counter--;
		i++;
	}

	lock++;
	pthread_cond_signal(&job_submit);
	pthread_mutex_unlock(&cmd_queue_run_mutex);
}

/* Exit command */
/*
 *
 */
int cmd_exit(int nargs, char **args) 
{
	printf("Thank you for using AUbatch!\n");
    exit(0);
}

/* First Come, First Served */
/*
 *
 */
int cmd_fcfs(int n, char **a)
{
	(void)n;
	(void)a;

	policy = 0;
	printf("Policy is changed to FCFS\n");

	return 0;
}

/* Shortest Job Firsrt */
/*
 *
 */
int cmd_sjf(int n, char **a)
{
	(void)n;
	(void)a;

	policy = 1;
	printf("Policy is changed to SJF\n");

	return 0;
}

/* Job priority */
/*
 *
 */
int cmd_priority(int n, char **a)
{
	(void)n;
	(void)a;

	policy = 2;
	printf("Policy is changed to Priority scheduling\n");

	return 0;
}

/* User Commands */
/*
 *
 */
static struct 
{
	const char *cmd_name;
	int (*func)(int nargs, char **args);
} 
cmd_dict[] = 
{
	{ "?\n",        cmd_help_menu },
	{ "h\n",        cmd_help_menu },
	{ "help\n",     cmd_help_menu },
	{ "run",        cmd_run },
    { "fcfs\n",     cmd_fcfs },
	{ "sjf\n",      cmd_sjf },
	{ "priority\n", cmd_priority },
	{ "exit\n",	    cmd_exit },
};

/* Execute a command */
/*
 *
 */
int cmd_dispatch(char *cmd)
{
	time_t beforesecs, aftersecs, secs;
	u_int32_t beforensecs, afternsecs, nsecs;

	char *args[MAXARGS];
	int nargs=0;
	char *word;
	char *context;
 	int i, result;

	for (word = strtok_r(cmd, " ", &context); word != NULL; word = strtok_r(NULL, " ", &context)) 
	{
		if (nargs >= MAXARGS) 
		{
			printf("Command line has too many arguments\n");
			return OVERFLOW;
		}

		args[nargs++] = word;
	}

	if (nargs==0) 
	{
		return 0;
	}

	for (i=0; cmd_dict[i].cmd_name; i++) 
	{
		if (*cmd_dict[i].cmd_name && !strcmp(args[0], cmd_dict[i].cmd_name)) 
		{
			assert(cmd_dict[i].func!=NULL);
			result = cmd_dict[i].func(nargs, args);
			return result;
		}
	}

	printf("%s: Command not found\n", args[0]);
	return INVALID;
}

 /*
 *
 */
void *menu( void *ptr)
{
	char *buffer;
    size_t bufsize = 64;
        
    buffer = (char*) malloc(bufsize * sizeof(char));
    if (buffer == NULL) 
    {
 	    perror("malloc buffer failed");
 	    exit(1);
	}

    char *menu = "Welcome to AUbatch scheduler\n" 
         "Please type h for help menu\n"
         "*******************************************************************\n"
         "HOW TO USE?\n"
         "Type \"run init <time> <\"1\">\"\n"
         "It allows user to choose how much time they need to give input arguments\n"
         "Choose policy and type it i.e. sjf or pri\n"
         "Submit jobs with \"run <Jobname> <Burst_time> <priority>\"\n"
         "Wait while the time you chose to add jobs finish\n"
         "*******************************************************************\n";
	
	printf("%s", menu);

    while (1) 
    {
		printf("> ");
		getline(&buffer, &bufsize, stdin);
		cmd_dispatch(buffer);
	}
}

/* Help menu */
int cmd_help_menu(int n, char **a)
{
	(void)n;
	(void)a;

    char *help_menu = "\t[run] <process> <expected_runtime> <priority>\n"
        "\t[help] Display help menu\n"
        "\t[fcfs] Set scheduler to FCFS policy\n"
        "\t[sjf] Set scheduler SJF policy\n"
        "\t[priority] Set scheduler Priority policy\n"
        "\t[exit] Exit AUbatch\n";

	printf("%s", help_menu);

	return 0;
}

/*
 * Dispatcher
 */
void *dispatcher(void *ptr)
{
    char *message;
    unsigned int i;
 	char *arg[5];
	pid_t pid;

    for (i = 0; i < NUM_OF_CMD; i++) 
    {
        pthread_mutex_lock(&cmd_queue_mutex);
        while (count == 0) 
        {
            pthread_cond_wait(&cmd_buffer_not_empty, &cmd_queue_mutex);
        }

	    if(policy == 0)
        {
	        execv_call(cmd_buffer[buffer_tail]);
            count--;
	        buffer_tail++;
	    }
	
	    if(policy == 1)
        {
		    enforce_policy();

		    for(buffer_tail=0; count != 0; buffer_tail++)
            {
		        execv_call(cmd_buffer[buffer_tail]);
		        count--;
		    }

	        sleep(1);
	        printf("Simulation of SJF is finished\nPRESS q to quit\n");
	        pthread_mutex_unlock(&cmd_queue_mutex);
	    }
   	
	    if(policy == 2)
        {
		    enforce_policy();
		    for(buffer_tail=0; count!=0; buffer_tail++)
            {
		        execv_call(cmd_buffer[buffer_tail]);
		        count--;
		    }
	        sleep(1);
	        printf("Simulation of Priority is finished\nPRESS q to quit\n");
	        pthread_mutex_unlock(&cmd_queue_mutex);
	    }

        if (buffer_tail == CMD_BUF_SIZE)
            buffer_tail = 0;

        pthread_cond_signal(&cmd_buffer_not_full);
        /* Unlock the sharefd command queue */
        pthread_mutex_unlock(&cmd_queue_mutex);

    }/* end for */
}

/*
 * Scheduler
 */
void *scheduler(void *ptr){
    char *message;
    struct queue *temp_cmd;
    unsigned int i;
    char num_str[8];
    size_t command_size;

    for (i = 0; i < NUM_OF_CMD; i++) 
    {    
        pthread_mutex_lock(&cmd_queue_mutex);
        while (count == CMD_BUF_SIZE) 
        {
            pthread_cond_wait(&cmd_buffer_not_full, &cmd_queue_mutex);
        }

        pthread_mutex_unlock(&cmd_queue_mutex);
        temp_cmd = malloc(MAX_CMD_LEN*sizeof(struct queue));

        while(lock == 0)
        {
		    pthread_cond_wait(&job_submit, &cmd_queue_run_mutex);	
	    }
	    lock--;

        pthread_mutex_lock(&cmd_queue_mutex); 
	    strcpy(temp_cmd->name, param_list[0]);

	    temp_cmd->burst_time = atoi(param_list[1]);
	    temp_cmd->priority = atoi(param_list[2]);   
        cmd_buffer[buffer_head] = temp_cmd;
        count++;
        /* Move buf_head_s forward, this is a circular queue */ 
        buffer_head++;
        if (buffer_head == CMD_BUF_SIZE)
            buffer_head = 0;
	
        pthread_cond_signal(&cmd_buffer_not_empty);  
        /* Unlock the shared command queue */
        pthread_mutex_unlock(&cmd_queue_mutex);
    }
}

/*
 *
 */
int main()
{
	/* Initialize the threads */
    pthread_t menu_thread, dispatcher_thread, scheduler_thread; 

	/* Initialize the thread messages */
    char *menu_message = "Menu Thread";
    char *dispatcher_message = "Dispatcher Thread";
    char *scheduler_message = "Scheduler Thread";

	/* Initialize the thread return values */
    int  menu_return, dispatcher_return, scheduler_return;

    /* Create two independent threads:command and executors */
    menu_return = pthread_create(&menu_thread, NULL, menu, (void*) menu_message);
    dispatcher_return = pthread_create(&dispatcher_thread, NULL, dispatcher, (void*) dispatcher_message);
    scheduler_return = pthread_create(&scheduler_thread, NULL, scheduler, (void*) scheduler_message);

    /* Initialize the  the two condition variables */
    pthread_mutex_init(&cmd_queue_mutex, NULL);
    pthread_mutex_init(&cmd_queue_run_mutex, NULL);
    pthread_cond_init(&cmd_buffer_not_full, NULL);
    pthread_cond_init(&cmd_buffer_not_empty, NULL);
    pthread_cond_init(&job_submit, NULL);
     
    pthread_join(dispatcher_thread, NULL);
    pthread_join(scheduler_thread, NULL);
    pthread_join(menu_thread, NULL); 

    return 0;
 }