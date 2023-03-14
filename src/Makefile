aubatch: aubatch.o
	gcc aubatch.o -o aubatch -lpthread

aubatch.o: aubatch.c
	gcc -c aubatch.c

batch_job: batch_job.o
	gcc batch_job.o -o batch_job

batch_job.o: batch_job.c
	gcc -c batch_job.c

process: process.o
	gcc process.c -o process

process.o: process.c
	gcc -c process.c

clean:
	rm *.o

cleanAll:
	rm *.o aubatch process batch_job