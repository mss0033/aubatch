aubatch: aubatch.o batch_job
	gcc aubatch.o -o aubatch -lpthread

aubatch.o: aubatch.c
	gcc -c aubatch.c

batch_job: batch_job.o
	gcc batch_job.o -o batch_job

batch_job.o: batch_job.c
	gcc -c batch_job.c

clean:
	rm *.o

cleanAll:
	rm *.o aubatch batch_job