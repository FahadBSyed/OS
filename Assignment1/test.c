#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "my_pthread_t.h"
#include <time.h>

pthread_mutex_t lock;
int sum = 0;

void func1(){
	printf("Thread 1 Starts\n");
	printf("Thread 1 Calls Yield \n");
	pthread_yield();
	printf("Thread 1 Locking \n");
	pthread_mutex_lock(&lock);
	sum += 5;
	printf("Thread 1 Unlocking \n");
	pthread_mutex_unlock(&lock);
	printf("Thread 1 Exiting \n");
	pthread_exit(NULL);
}

void func2(){
	printf("Thread 2 Starts\n");
	printf("Thread 2 Locking \n");
	pthread_mutex_lock(&lock);
	sum += 10;
	printf("Thread 2 Unlocking \n");
	pthread_mutex_unlock(&lock);
	printf("Thread 2 Exiting \n");
	pthread_exit(NULL);
}

int main(int argc, char **argv) {

	pthread_t thread1;
	pthread_t thread2;
	//pthread_t thread3;

	sum = 0;

	pthread_mutex_init(&lock,NULL);	

	printf("Program Begins.\n");

	int* x = malloc(sizeof(int));
	*x = 10;

	pthread_create(&thread1, NULL, (void*) &func1, x);
	pthread_create(&thread2, NULL, (void*) &func2, x);

	int* ret = malloc(sizeof(int));

	pthread_join(thread1, (void**)&ret);
	pthread_join(thread2, (void**)&ret);
	
	pthread_mutex_destroy(&lock);

	printf("Program Finished\n");
}
