#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "my_pthread_t.h"

void* test_func(void* x){
	
	printf("we called test_func\n");
	int c = *(int*)x;
	printf("C is %d\n", c);
	c *=2;
	my_pthread_exit(&c);
}

int main(int argc, char** argv){
	
	printf("hello world.\n");
	
	my_pthread_t thread = 0;
	int x = 10;
	int error = my_pthread_create(&thread, NULL, (void*)&test_func, &x);
	if (error != 0)
		printf("can't create thread :[%s]", strerror(error));
	else
		printf("Thread created successfully\n");

	int* ret; 
	my_pthread_join(thread, (void**)&ret);
	printf("return value from the thread is %d\n", *ret);
	
}

