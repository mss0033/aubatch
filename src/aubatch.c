/*
 * 
 */

#include "aubatch.h"

/* Command Line Processing */
#define NUM_OF_JOBS     5   /* The number of submitted jobs   */
#define JOB_BUFFER_SIZE 10  /* The size of the command queue */
#define MAX_JOB_LEN     512 /* The longest scheduler length */
/* Error Codes */
#define INVALID  1
#define OVERFLOW 2
/* Maximums */
#define MAXARGS 4   

/* Global shared variables */
/* Mutexes and CVs */
pthread_mutex_t job_queue_mutex;     /* Mutex for critical sections */
pthread_cond_t job_buffer_not_full;  /* Buffer not full CV  */
pthread_cond_t job_buffer_not_empty; /* Buffer not empty CV */
pthread_cond_t job_submit;	         /* Job submit CV */

unsigned int job_buffer_head = 0;
unsigned int job_buffer_tail = 0;
unsigned int count = 0;
unsigned int lock = 0;
unsigned int policy = 0;

unsigned int num_jobs_submitted = 0;
float avg_turnaround_time = 0.0;
float avg_cpu_time = 0.0;
float avg_waiting_time = 0.0;
float throughput = 0.0;

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
	pthread_mutex_lock(&job_queue_mutex);
	switch(policy)
	{
		/* FCFS */
		case 0:
			if (job_buffer_head == 0)
			{
				break;
			}
			for(i = job_buffer_tail; i < job_buffer_head - 1; ++i)
			{
				for(j = job_buffer_tail; j < job_buffer_head - i - 1; ++j)
				{
					if(job_buffer[j]->arrival_time > job_buffer[j+1]->arrival_time)
					{
						key = job_buffer[j];
						job_buffer[j] = job_buffer[j+1];
						job_buffer[j+1] = key;
					} 
				}	
			}
			break;
		/* Sorting by CPU Time */
		case 1:
			if (job_buffer_head == 0)
			{
				break;
			}
			for(i = job_buffer_tail; i < job_buffer_head - 1 ; ++i)
			{
				for(j = job_buffer_tail; j < job_buffer_head - i - 1; ++j)
				{
					if(job_buffer[j]->cpu_time > job_buffer[j+1]->cpu_time)
					{
						key = job_buffer[j];
						job_buffer[j] = job_buffer[j+1];
						job_buffer[j+1] = key;
					} 
				}	
			}
			break;
		/* Sorting by Priority */
		case 2:
			if (job_buffer_head == 0)
			{
				break;
			}
			for(i = job_buffer_tail; i < job_buffer_head - 1; ++i)
			{
				for(j = job_buffer_tail; j < job_buffer_head - i - 1; ++j)
				{
					if(job_buffer[j]->priority > job_buffer[j+1]->priority)
					{
						key = job_buffer[j];
						job_buffer[j] = job_buffer[j+1];
						job_buffer[j+1] = key;
					} 
				}	
			}
			break;
		/* Unknown */
		default:
			break;	
	}
	pthread_mutex_unlock(&job_queue_mutex);
}

/*
 *
 */
void *execv_call(struct queue *param)
{
	char *args[3];
	pid_t pid;

	args[0] = param->name;
	sprintf(args[1], "%d", param->cpu_time);
	args[2] = NULL;
	//args[1] = NULL;

	/* fork() an execv() process */
	 switch ((pid = fork()))
  	 {
		/* If fork() fails */
   	 	case -1:
			/* Throw an error */
   	   		perror("fork failed");
        		break;
     	case 0:
	 		/* Set the run time */
			time(&param->run_time);
			/* Execution of execv() */
			if(execv(args[0], args)==-1)
			{ 
	 			perror("Fail\n");
	 			exit(0);
			}
			/* Sleep this thread until the exe is complete */
			sleep(param->cpu_time);
			/* Set the complete time */
			time(&param->complete_time);
      	default:
	 		//  pthread_mutex_lock(&job_queue_run_mutex);
	 		//  sleep(param->burst_time);
	 		//  pthread_mutex_unlock(&job_queue_run_mutex);
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

	pthread_mutex_lock(&job_queue_mutex);

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
	num_jobs_submitted++;
	pthread_cond_signal(&job_submit);
	pthread_mutex_unlock(&job_queue_mutex);
}

/*
 * List the current job queue information
 */
int list(int nargs, char **args)
{
	(void)nargs;
	(void)args;

	int i, j, num_jobs_in_queue;
	char* sch_policy;

	/* Determine the number of jobs in the queue */
	num_jobs_in_queue = get_num_jobs_in_queue();
	if (num_jobs_in_queue < 0 || num_jobs_in_queue > JOB_BUFFER_SIZE)
	{
		printf("Error encountered when counting jobs in queue");
		return -1;
	}

	/* Determine the policy */
	switch (policy)
	{
		case (0):
			strcpy(sch_policy, "FCFS");
			break;
		case (1):
			strcpy(sch_policy, "SJF");
			break;
		case (2):
			strcpy(sch_policy, "Priority");
			break;
		default:
			strcpy(sch_policy, "Eror determining scheduling policy");
			break;
	}

	/* Print the queue information */
	printf("Total number of jobs in the queue: %d\n"
		"Scheduling Policy: %s\n"
		"Job Name\t CPU Time\t Priority\t Arrival Time\n", 
		num_jobs_in_queue, 
		sch_policy);

	/* Lock the job queue while we read from it */
	pthread_mutex_lock(&job_queue_mutex);
	for(i=0; i < JOB_BUFFER_SIZE; i++)
	{
		j = (job_buffer_tail + i) % JOB_BUFFER_SIZE;
		printf("%s\t \t%d\t \t%d \t%lld\n", 
		job_buffer[j]->name, 
		job_buffer[j]->cpu_time, 
		job_buffer[j]->priority, 
		(long long) job_buffer[j]->arrival_time);
	}
	/* Unlock when we are done */
	pthread_mutex_unlock(&job_queue_mutex);
	return 0; 
}

/*
 * Calcualte the number of jobs in the job queue
 */
int get_num_jobs_in_queue()
{
	int num_jobs_in_queue = -1;

	/* If the head is ahead of the tail */
	if (job_buffer_head > job_buffer_tail)
	{
		/* Simply take the difference */
		num_jobs_in_queue = job_buffer_head - job_buffer_tail - 1;
	}
	/* If the head is behind of the tail */
	else if (job_buffer_head < job_buffer_tail)
	{
		/* Subtract the difference between the tail and the head from the buffer size */
		num_jobs_in_queue = JOB_BUFFER_SIZE - (job_buffer_tail - job_buffer_head) - 1;
	}
	/* If the head is at the tail */
	else
	{
		/* Queue is empty */
		num_jobs_in_queue = 0;
	}

	return num_jobs_in_queue;
}

/*
 * Quit command
 */
int quit(int nargs, char **args) 
{
	print_stats();
	printf("Thank you for using AUbatch!\n");
    exit(0);
}

/*
 * Print the statistics for the schedular
 */
int print_stats()
{	
	/* Lock the job queue while the stats are being calculated */
	pthread_mutex_lock(&job_queue_mutex);
	/* Iterate through the job buffer and calcualte the stats */
	int i, j;
	for(i=0; i < JOB_BUFFER_SIZE; i++)
	{
		j = (job_buffer_tail + i) % JOB_BUFFER_SIZE;
		avg_turnaround_time = (job_buffer[j]->complete_time - job_buffer[j]->arrival_time) / num_jobs_submitted;
		avg_cpu_time = (job_buffer[j]->complete_time - job_buffer[j]->run_time) / num_jobs_submitted;
		avg_waiting_time = (job_buffer[j]->run_time - job_buffer[j]->arrival_time) / num_jobs_submitted;
	}
	/* Throughput is the inverse of average turnaround time */
	throughput = 1.0/avg_turnaround_time;
	/* Print the stats */
	printf("Total number of jobs submitted: %d\n"
		"Average turnaround time: %f seconds\n"
		"Average CPU time: %f seconds\n"
		"Average waiting time: %f seconds\n"
		"Throughput: %f No./second\n", 
		num_jobs_submitted,
		avg_turnaround_time,
		avg_cpu_time,
		avg_turnaround_time,
		throughput);

	/* Unlock the job queue when done */
	pthread_mutex_unlock(&job_queue_mutex);

	return 0;
}

/*
 * Sets the scheduling policy to First Come, First Served
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

/*
 * Sets the scheduling policy to Shortest Job Firsrt
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

/*
 * Sets the scheduling policy to Job priority
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

/*
 * Setup a user user commands dictionary 
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

/*
 * Execute a user command
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
  * Display the main menu
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

/* 
 * Display the help menu 
 */
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
 * Dispatcher function
 */
void *dispatcher(void *ptr)
{
    // char *message;
    unsigned int i;

	// message = (char *) ptr;
    // printf("%s \n", message);

    for (i = 0; i < NUM_OF_JOBS; i++) 
    {
		/* Lock the job queue */
        pthread_mutex_lock(&job_queue_mutex);
		/* While the number of jobs is 0, wait */
        while (count == 0) 
        {
            pthread_cond_wait(&job_buffer_not_empty, &job_queue_mutex);
        }
		/* Decrement the waiting jobs count */
		count--;
		/* Call to execute the job */
	    execv_call(job_buffer[job_buffer_tail]);
		/* Free up the job buffer space */
		// free(job_buffer[job_buffer_tail]);
		/* Increment the job buffer tail, this is a circular buffer */
		job_buffer_tail++;
		job_buffer_tail = job_buffer_tail % JOB_BUFFER_SIZE;
		/* Signal that the job buffer is not full */
        pthread_cond_signal(&job_buffer_not_full);
        /* Unlock the job queue */
        pthread_mutex_unlock(&job_queue_mutex);
    }
}

/*
 * Scheduler function
 */
void *scheduler(void *ptr){
    // char *message;
    struct queue *temp_job;
    unsigned int i;

	// message = (char *) ptr;
    // printf("%s \n", message);

    for (i = 0; i < NUM_OF_JOBS; i++) 
    {   
		/* Lock the job queue */
        pthread_mutex_lock(&job_queue_mutex);
		/* While the job count is equal to the job buffer size, wait */
        while (count == JOB_BUFFER_SIZE) 
        {
            pthread_cond_wait(&job_buffer_not_full, &job_queue_mutex);
        }
		/* Unlock the job queue */
		pthread_mutex_unlock(&job_queue_mutex);
		/* Allocate a temp job */
        temp_job = malloc(MAX_JOB_LEN*sizeof(struct queue));
		/* Lock the job run queue*/
		pthread_mutex_lock(&job_queue_mutex);
		/* While the number of number of locks, aka submitted jobs, is zero, wait */
        while(lock == 0)
        {
		    pthread_cond_wait(&job_submit, &job_queue_mutex);	
	    }
	    lock--;
		/* Unlock the job run queue */
		pthread_mutex_unlock(&job_queue_mutex);
		/* Lock the job queue */
		pthread_mutex_lock(&job_queue_mutex);
		/* Copy the job name from the params */
	    strcpy(temp_job->name, param_list[0]);
		/* Populate the temp job info */
	    temp_job->cpu_time = atoi(param_list[1]);
	    temp_job->priority = atoi(param_list[2]);
		/* Set the arrival time in; Seconds since Epoch */
		time(&temp_job->arrival_time);
		/* Add the job to the job buffer */
        job_buffer[job_buffer_head] = temp_job;
		/* Increment the job count*/
        count++;
        /* Move buf_head_s forward, this is a circular queue */
		job_buffer_head++;
        job_buffer_head = job_buffer_head % JOB_BUFFER_SIZE;	
		/* Signal that the job buffer is not empty */
        pthread_cond_signal(&job_buffer_not_empty);
        /* Unlock the job queue */
        pthread_mutex_unlock(&job_queue_mutex);
    }
}

/*
 * Main funciton.
 */
int main()
{
	/* Initialize the threads */
    pthread_t menu_thread, dispatcher_thread, scheduler_thread;

	/* Initialize the  the two condition variables */
    pthread_mutex_init(&job_queue_mutex, NULL);
    pthread_cond_init(&job_buffer_not_full, NULL);
    pthread_cond_init(&job_buffer_not_empty, NULL);
    pthread_cond_init(&job_submit, NULL);

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
     
	/* Join the threads so that the program only exits when all are complete */
    pthread_join(dispatcher_thread, NULL);
    pthread_join(scheduler_thread, NULL);
    pthread_join(menu_thread, NULL); 

	/* Print a message indicating that execution is complete*/
	printf("AU Batch threads have completed, goodbye and thank you for using AUBatch.");

    return 0;
 }