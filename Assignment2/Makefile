CC = gcc
CFLAGS = -g -c
AR = ar -rc
RANLIB = ranlib


Target: my_pthread.a

my_pthread.a: my_pthread.o my_malloc.o
	$(AR) libmy_pthread.a my_pthread.o my_malloc.o
	$(RANLIB) libmy_pthread.a

my_pthread.o: my_pthread_t.h
	$(CC) -pthread $(CFLAGS) my_pthread.c
	
my_malloc.o: my_malloc.h
	$(CC) -pthread $(CFLAGS) my_malloc.c

pthread_test.o:
	$(CC) -g -L. pthread_test.c -lmy_pthread -o pthread_test.o


	
clean:
	rm -rf testfile *.o *.a
