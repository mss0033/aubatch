aubatch: aubatch.o
	gcc aubatch.o -o aubatch -lpthread

aubatch.o: aubatch.c
	gcc -c aubatch.c

process: process.o
	gcc process.c -o process

process.o: process.c
	gcc -c process.c

clean:
	rm *.o

cleanAll:
	rm *.o aubatch process