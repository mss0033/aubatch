/*
 * Author: Matthew Shiplett
 * Date last modified: March 14th, 2023
 * Purpose: Create a simple, simuldated work load for the AUBatch program
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* The default amount of seconds to sleep */
#define DEFAULT_SLEEP_TIME       30
#define MAX_LOG_FILE_NAME_LENGTH 20

/*
 * Takes in an argument which indicates how long this batch job
 * should sleep for. By default the sleep time is 10 seconds.
 */
int main(int argc, char *argv[] )
{
    /* To ensure the job actually rus, write some output to a file.
     * To do this, first setup and generate a random int.
     */
    srand(time(NULL));
    int random = rand();
    /* Next, prepare a log file */
    FILE *file_pointer;
    char file_name[MAX_LOG_FILE_NAME_LENGTH];
    /* Following that, append the random int to the end of the log file name*/
    snprintf(file_name, MAX_LOG_FILE_NAME_LENGTH, "batch_job_logs_%d", random);
    /* Open the file in write mode */
    file_pointer = fopen(file_name, "w");
    /* Initalize sleep time */
    int sleep_time = DEFAULT_SLEEP_TIME;
    /* Indicate that the batch job is running */
    fprintf(file_pointer, "Batch job is running ...\n");

    /* Check the arguments to see if a sleep time was provided 
     * While we check for more, only the first argument is utilized. 
     */
    if (argc > 1)
    {
        /* Grab the first argument */
        char* arg = argv[1];
        /* Attempt to convert the argument to an integer */
        sleep_time = atoi(arg);
        /* If the converted int is 0, the conversion may have failed */
        if (sleep_time == 0)
        {
            /* Print a warning */
            fprintf(file_pointer, "The sleep time was set to %i seconds," 
                 " meaning the integer conversion may have failed.\n", sleep_time);
        }
    }

    /* Print how long this job will sleep for */
    fprintf(file_pointer, "Batch job is sleeping for: %i seconds\n", sleep_time);

    /* Sleep */
    sleep(sleep_time);

    /* Indicate the job is complete */
    fprintf(file_pointer, "Batch job is complete.\n");

  return 0;
}