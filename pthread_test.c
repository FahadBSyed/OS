#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "my_pthread_t.h"

void* test_func3(){
	
	printf("\nwe called test_func3.\n");
	pthread_exit(NULL);
}

void* test_func2(){
	
	printf("\nwe called test_func 2\n");
	int c = 0;
	pthread_t thread3;
	pthread_create(&thread3, NULL, (void*)&test_func3, NULL);
	
	pthread_join(thread3, NULL);
	pthread_exit(&c);
}

void* test_func(void* x){
	
	printf("\nwe called test_func\n");
	int c = *(int*)x;
	printf("C is %d\n", c);
	c *=2;
	
	pthread_t thread2;
	printf("creating thread 2\n");
	pthread_create(&thread2, NULL, (void*)&test_func2, NULL);
	
	pthread_join(thread2, NULL);
	pthread_exit(&c);
}

int main(int argc, char** argv){
	
	printf("hello world.\n");
	
	pthread_t thread;
	int x = 10;
	int error = pthread_create(&thread, NULL, (void*)&test_func, &x);
	if (error != 0){
		printf("can't create thread :[%s]", strerror(error));
		return;
	}

	int* ret; 
	int counter = 0;
	int math = 0;
	while(counter < 10000){
		int math = math + counter * 2;
		counter++;
		if(counter % 100 == 0){
			printf("math: %d\n", math);
		}
	}
	
	pthread_join(thread, (void**)&ret);
	printf("\treturn value from the thread is %d\n", ret);
	pthread_exit(NULL);
}

