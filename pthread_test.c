#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "my_pthread_t.h"

void* test_func3(){
	
	printf("\n\x1b[36mwe called test_func3.\x1b[0m\n");
	pthread_exit(NULL);
}

void* test_func2(){
	
	printf("\n\x1b[36mwe called test_func 2.\x1b[0m\n");
	int* D = malloc(sizeof(int));
	*D = 25;
	printf("\x1b[36mD is %d\x1b[0m\n", *D);
	pthread_t thread3;
	pthread_create(&thread3, NULL, (void*)&test_func3, NULL);
	
	pthread_join(thread3, NULL);
	pthread_exit(D);
}

void* test_func(void* x){
	
	printf("\n\x1b[36mwe called test_func.\x1b[0m\n");
	int* c = (int*)x;
	*c *=2;
	printf("\x1b[36mC is %d\x1b[0m\n", *c);
	
	pthread_t thread2;
	//printf("\x1b[36mcreating thread 2.\x1b[0m\n");
	//pthread_create(&thread2, NULL, (void*)&test_func2, NULL);
	
	//pthread_join(thread2, NULL);
	pthread_exit(c);
}

int main(int argc, char** argv){
	
	printf("\x1b[36mhello world.\x1b[0m\n");
	
	pthread_t thread;
	int x = 10;
	int error = pthread_create(&thread, NULL, (void*)&test_func, &x);
	if (error != 0){
		printf("\x1b[36mcan't create thread :[%s]\x1b[0m", strerror(error));
		return;
	}

	int* ret; 
	int counter = 0;
	int math = 0;
	while(counter < 10000){
		int math = math + counter * 2;
		counter++;
	}
	
	pthread_join(thread, (void**)&ret);
	printf("\x1b[36mreturn value from the thread is %d\x1b[0m\n", ret);
	pthread_exit(NULL);
}

