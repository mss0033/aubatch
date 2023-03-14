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
#define NUM_OF_JOBS     5   /* The number of submitted jobs   */
#define JOB_BUFFER_SIZE 10  /* The size of the command queue */
#define MAX_JOB_LEN     512 /* The longest scheduler length */
/* Error Codes */
#define INVALID  1
#define OVERFLOW 2
/* Maximums */
#define MAXARGS    4 
#define MAXCMDLGTH 64    

/* Global shared variables */
/* Mutexes and CVs */
pthread_mutex_t job_queue_mutex;     /* Mutex for critical sections */
pthread_mutex_t job_queue_run_mutex; /* Mutext for menu to scheduler */
pthread_cond_t job_buffer_not_full;  /* Buffer not full CV  */
pthread_cond_t job_buffer_not_empty; /* Buffer not empty CV */
pthread_cond_t job_submit;	        /* Job submit CV */

unsigned int job_buffer_head;
unsigned int job_buffer_tail;
unsigned int count;
unsigned int lock = 0;
int policy = 0;

char job[30];
char *param_list[3];

struct queue *job_buffer[JOB_BUFFER_SIZE];

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
			pthread_mutex_lock(&job_queue_run_mutex);
			// TODO Add sorting by arrival time
			pthread_mutex_unlock(&job_queue_run_mutex);
			break;
		/* Sorting by Burst Time */
		case 1:
			if (job_buffer_head == 0)
			{
				break;
			}
			pthread_mutex_lock(&job_queue_run_mutex);
			for(i = 0; i < job_buffer_head - 1 ; i++)
			{
				for(j = 0; j < job_buffer_head - i - 1; j++)
				{
					if(job_buffer[j]->burst_time > job_buffer[j+1]->burst_time)
					{
						key = job_buffer[j];
						job_buffer[j] = job_buffer[j+1];
						job_buffer[j+1] = key;
					} 
				}	
			}
			pthread_mutex_unlock(&job_queue_run_mutex);
			break;
		/* Sorting by Priority */
		case 2:
			pthread_mutex_lock(&job_queue_run_mutex);
			if (job_buffer_head == 0)
			{
				break;
			}
			for(i = 0; i < job_buffer_head - 1; i++)
			{
				for(j = 0; j < job_buffer_head - i - 1; j++)
				{
					if(job_buffer[j]->priority > job_buffer[j+1]->priority)
					{
						key = job_buffer[j];
						job_buffer[j] = job_buffer[j+1];
						job_buffer[j+1] = key;
					} 
				}	
			}
			pthread_mutex_unlock(&job_queue_run_mutex);
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
	char *args[3];
	pid_t pid;

	args[0] = param->name;
	sprintf(args[1], "%d", param->burst_time);
	args[2] = NULL;

	/*Fork() to host an execv() process*/
	switch ((pid = fork()))
  	{
   		case -1:
   	  		perror("fork failed");
       		break;
     	case 0:
	 		/* Execution of execv() */
	 		if(execv(args[0], args)==-1)
	 		{ 
	 			perror("Fail\n");
	 			exit(0);
	 		}
       		break;
     	default:
			pthread_mutex_lock(&job_queue_run_mutex);
			sleep(param->burst_time);
			pthread_mutex_unlock(&job_queue_run_mutex);
       		break;
  	 }

	return 0;
}

/*
 *
 */
int run(int nargs, char **args) 
{
	int i=1;
	int counter = nargs;

	pthread_mutex_lock(&job_queue_run_mutex);

	if (nargs != 4) 
	{
		printf("Usage: run <job> <time> <priority>\n");
		return INVALID;
	}

	counter--;
	strcpy(job, args[0]);

	while(counter>0)
	{
		param_list[i-1] = args[i];
		strcat(job, param_list[i-1]);
		strcat(job," ");
		counter--;
		i++;
	}

	lock++;
	pthread_cond_signal(&job_submit);
	pthread_mutex_unlock(&job_queue_run_mutex);
}

/*
 *
 */
int list(int nargs, char **args)
{
	(void)nargs;
	(void)args;

	int i;

	printf("Job Name\t Burst Time\t Priority\n");

	for(i=0; i < job_buffer_head; i++)
	{
		printf("%s\t \t%d\t \t%d\n", job_buffer[i]->name, job_buffer[i]->burst_time, job_buffer[i]->priority);
	}
}

/* Quit command */
/*
 *
 */
int quit(int nargs, char **args) 
{
	printf("Thank you for using AUbatch!\n");
    exit(0);
}

/* First Come, First Served */
/*
 *
 */
int fcfs(int nargs, char **args)
{
	(void)nargs;
	(void)args;

	policy = 0;
	printf("Policy is changed to FCFS\n");
	enforce_policy();

	return 0;
}

/* Shortest Job Firsrt */
/*
 *
 */
int sjf(int nargs, char **args)
{
	(void)nargs;
	(void)args;

	policy = 1;
	printf("Policy is changed to SJF\n");
	enforce_policy();

	return 0;
}

/* Job priority */
/*
 *
 */
int priority(int nargs, char **args)
{
	(void)nargs;
	(void)args;

	policy = 2;
	printf("Policy is changed to Priority scheduling\n");
	enforce_policy();

	return 0;
}

/* User Commands */
/*
 *
 */
static struct 
{
	const char *command_name;
	int (*func)(int nargs, char **args);
} 
commands_dict[] = 
{
	{ "?\n",        display_help_menu },
	{ "h\n",        display_help_menu },
	{ "help\n",     display_help_menu },
	{ "run",        run },
	{ "list\n",     list },
    { "fcfs\n",     fcfs },
	{ "sjf\n",      sjf },
	{ "priority\n", priority },
	{ "quit\n",	    quit },
	{ "q\n",	    quit },
};

/* Execute a command */
/*
 *
 */
int execute_command(char *job_arg)
{
	time_t beforesecs, aftersecs, secs;
	u_int32_t beforensecs, afternsecs, nsecs;

	char *args[MAXARGS];
	int nargs=0;
	char *word;
	char *context;
 	int i, result;

	for (word = strtok_r(job_arg, " ", &context); word != NULL; word = strtok_r(NULL, " ", &context)) 
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

	for (i=0; commands_dict[i].command_name; i++) 
	{
		if (*commands_dict[i].command_name && !strcmp(args[0], commands_dict[i].command_name)) 
		{
			assert(commands_dict[i].func!=NULL);
			result = commands_dict[i].func(nargs, args);
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
    size_t buffer_size = 64;
        
    buffer = (char*) malloc(buffer_size * sizeof(char));
    if (buffer == NULL) 
    {
 	    perror("malloc buffer failed");
 	    exit(1);
	}

    char *menu_body = "Welcome Matthew's AUBatch job scheduler\n" 
         "Please type 'help' or 'h' to find out more about the AUBatch commands\n"
         "*******************************************************************\n"
         "Submit jobs with \"run <job> <time> <priority>\"\n"
		 "			submit a job named: <job>\n"
		 "			estimated execution time is <time>\n"
		 "			job priority is <priority>\n"
         "Then please wait while the job(s) finish\n"
         "*******************************************************************\n";
	
	printf("%s", menu_body);

    while (1) 
    {
		printf("> ");
		getline(&buffer, &buffer_size, stdin);
		execute_command(buffer);
	}
}

/* Help menu */
int display_help_menu(int nargs, char **args)
{
	(void)nargs;
	(void)args;

    char *help_menu_body = 
		"\trun <job> <time> <priority>\"\n"
		"			submit a job named: <job>\n"
		"			estimated execution time is <time>\n"
		"			job priority is <priority>\n"
        "\thelp - Display help menu\n"
        "\tfcfs - Set scheduler to FCFS policy\n"
        "\tsjf - Set scheduler SJF policy\n"
        "\tpriority - Set scheduler Priority policy\n"
        "\tquit - Quit AUbatch\n";

	printf("%s", help_menu_body);

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

    for (i = 0; i < NUM_OF_JOBS; i++) 
    {
        pthread_mutex_lock(&job_queue_mutex);
        while (count == 0) 
        {
            pthread_cond_wait(&job_buffer_not_empty, &job_queue_mutex);
        }

	    if(policy == 0)
        {
	        execv_call(job_buffer[job_buffer_tail]);
            count--;
	        job_buffer_tail++;
	    }
	
	    if(policy == 1)
        {
		    enforce_policy();
		    for(job_buffer_tail=0; count != 0; job_buffer_tail++)
            {
		        execv_call(job_buffer[job_buffer_tail]);
		        count--;
		    }
	        pthread_mutex_unlock(&job_queue_mutex);
	    }
   	
	    if(policy == 2)
        {
		    enforce_policy();
		    for(job_buffer_tail=0; count != 0; job_buffer_tail++)
            {
		        execv_call(job_buffer[job_buffer_tail]);
		        count--;
		    }
	        pthread_mutex_unlock(&job_queue_mutex);
	    }

        if (job_buffer_tail == JOB_BUFFER_SIZE)
            job_buffer_tail = 0;

        pthread_cond_signal(&job_buffer_not_full);
        /* Unlock the shared job queue */
        pthread_mutex_unlock(&job_queue_mutex);

    }/* end for */
}

/*
 * Scheduler
 */
void *scheduler(void *ptr){
    char *message;
    struct queue *temp_job;
    unsigned int i;
    char num_str[8];
    size_t command_size;

    for (i = 0; i < NUM_OF_JOBS; i++) 
    {    
        pthread_mutex_lock(&job_queue_mutex);
        while (count == JOB_BUFFER_SIZE) 
        {
            pthread_cond_wait(&job_buffer_not_full, &job_queue_mutex);
        }

        pthread_mutex_unlock(&job_queue_mutex);
        temp_job = malloc(MAX_JOB_LEN*sizeof(struct queue));

        while(lock == 0)
        {
		    pthread_cond_wait(&job_submit, &job_queue_run_mutex);	
	    }
	    lock--;

        pthread_mutex_lock(&job_queue_mutex); 
	    strcpy(temp_job->name, param_list[0]);

	    temp_job->burst_time = atoi(param_list[1]);
	    temp_job->priority = atoi(param_list[2]);   
        job_buffer[job_buffer_head] = temp_job;
        count++;
        /* Move buf_head_s forward, this is a circular queue */ 
        job_buffer_head = job_buffer_head++ % JOB_BUFFER_SIZE;
        // if (job_buffer_head == JOB_BUFFER_SIZE)
        //     job_buffer_head = 0;
	
        pthread_cond_signal(&job_buffer_not_empty);  
        /* Unlock the shared command queue */
        pthread_mutex_unlock(&job_queue_mutex);
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
	scheduler_return = pthread_create(&scheduler_thread, NULL, scheduler, (void*) scheduler_message);
    dispatcher_return = pthread_create(&dispatcher_thread, NULL, dispatcher, (void*) dispatcher_message);

    /* Initialize the  the two condition variables */
    pthread_mutex_init(&job_queue_mutex, NULL);
    pthread_mutex_init(&job_queue_run_mutex, NULL);
    pthread_cond_init(&job_buffer_not_full, NULL);
    pthread_cond_init(&job_buffer_not_empty, NULL);
    pthread_cond_init(&job_submit, NULL);
     
    pthread_join(dispatcher_thread, NULL);
    pthread_join(scheduler_thread, NULL);
    pthread_join(menu_thread, NULL); 

    return 0;
 }