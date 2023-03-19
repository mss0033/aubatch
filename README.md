# AU Batch
AU Batch is a program designed to emulate a scheduler.
The program takes as input user submitted jobs that will be scheduled
and run using a selected scheduling algorith.

## Compilation
```
cd aubatch
cd src
make
```
to clean up
```
make clean
```

# Usage
```
./aubatch
```

## Example usage
To run the provided batch_job executable, a user would enter:
```
> run batch_job 10 1
```
The provided batch_job is passed the CPU time and will sleep for
that amount of time. The process will also log output to the directory
to indicate that it is running and when it is complete.

To run a test, user would enter:
```
> test batch_job sjf 5 5 5 15
```

# clean-up 
```
make cleanAll
```

# Notes
* When a job is submitted, a '> ' fails to be appended to the console, however users can still enter input