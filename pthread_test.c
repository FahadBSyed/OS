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
}

int main(int argc, char** argv){
	
	printf("hello world.\n");
	
	my_pthread_t thread = 0;
	int x = 10;
	my_pthread_create(&thread, NULL, (void*)&test_func, &x);
	
}

