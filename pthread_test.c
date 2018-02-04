#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "my_pthread_t.h"

void* test_func(void* arg){
	
	int a = *((int *)arg);
	printf("a is %d\n", a);
}

int main(int argc, char** argv){
	
	printf("hello world.\n");
	
	my_pthread_t thread = 0;
	int x = 10;
	my_pthread_create(&thread, NULL, test_func, &x);
	
}

