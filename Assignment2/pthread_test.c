#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "my_pthread_t.h"
#include <time.h>

void* test_func3(){
	
	printf("\n\x1b[36mwe called test_func3.\x1b[0m\n");
	pthread_exit(NULL);
}

void* test_func2(){
	
	printf("\n\x1b[36mwe called test_func 2.\x1b[0m\n");
	//int* D = malloc(sizeof(int));
	//*D = 25;
	//printf("\x1b[36m*D = %d\x1b[0m\n", *D);
	pthread_t thread3;
	pthread_create(&thread3, NULL, (void*)&test_func3, NULL);
	
	time_t start, curr_time;
	start = clock();
	while((curr_time - start)/CLOCKS_PER_SEC <= 1){curr_time = clock();}
	
	//pthread_join(thread3, NULL);
	//pthread_exit(D);
	pthread_exit(NULL);
}

void* test_func(void* x){

	printf("\n\x1b[36mwe called test_func.\x1b[0m\n");
	//int* c = (int*)x;
	//printf("\x1b[36m*C = %d\x1b[0m\n", *c);
	//printf("\x1b[36mC = %x\x1b[0m\n", c);
	//*c *=2;
	pthread_t thread2;
	//printf("\x1b[36mcreating thread 2.\x1b[0m\n");
	pthread_create(&thread2, NULL, (void*)&test_func2, NULL);
	
	
	char* temp = malloc(512);
	char* pizza = malloc(512);
	//free(pizza);
	char* temp3 = malloc(256);
	char* temp4 = malloc(240);
	char* temp5 = malloc(30002); //causing problems? WHY?
		
	pthread_join(thread2, NULL);
	
	//pthread_exit(c);
	pthread_exit(NULL);
}


void* free_test(){

	printf("\nCalled free test\n");

	char* temp = malloc(512);
	char* temp1 = malloc(1024);
	char* temp2 = malloc(8);
	char* temp3 = malloc(256);
	char* temp4 = malloc(10000);
	char* temp5 = malloc(20);

	free(temp);
	free(temp4);
	free(temp1);
	free(temp2);
	free(temp3);
	free(temp5);

	char* temp6 = malloc(25000);
	free(temp6);
	pthread_exit(NULL);


}



int main(int argc, char** argv){
	
	printf("\x1b[36mhello world.\x1b[0m\n");
	pthread_t thread;
	pthread_t thread2;
	pthread_t thread3;
	pthread_t thread4;
	pthread_t free1;	
	int* x = malloc(sizeof(int));
	printf("\x1b[36mX = %x\x1b[0m\n", x);
	//*x = 10;
	void* try = malloc(4096 - sizeof(int) - 1 - 32);
//	int error = pthread_create(&thread, NULL, (void*)&test_func, x);
	pthread_t self = (pthread_t)currently_running_thread;
	printf("\x1b[36mcreated thread 1.\x1b[0m\n");
	printf("\x1b[36mcurrently_running_thread: %x\x1b[0m\n", currently_running_thread);
	
	//char* temp = malloc(512);
	//char* pizza = malloc(512);
	//free(pizza);
	//char* temp3 = malloc(256);
	//char* temp4 = malloc(240);
	//char* temp5 = malloc(30002);
	
	int* ret = malloc(sizeof(int));
	time_t start, curr_time;
	start = clock();
	
	printf("\x1b[36mret's address: %x\x1b[0m\n", &ret);
	printf("\x1b[36mret: %x\x1b[0m\n", ret);
	printf("\x1b[36mret's value: %d\x1b[0m\n", *ret);
//	pthread_join(thread, (void**)&ret);
	
//	int error2 = pthread_create(&thread2, NULL, (void*)&test_func2, x);
//	int error3 = pthread_create(&thread3, NULL, (void*)&test_func, x);
//	int error4 = pthread_create(&thread4, NULL, (void*)&test_func, x);
	int error5 = pthread_create(&free1, NULL, (void*)&free_test,NULL);
//	if (error != 0 /*&& error2 != 0 && error3 != 0 && error4 != 0*/){
//		printf("\x1b[36mcan't create thread :[%s]\x1b[0m", strerror(error));
//		return;
//	}
	
	
	while((curr_time - start)/CLOCKS_PER_SEC <= 2){curr_time = clock();}

	
//	pthread_join(thread2, (void**)&ret); //problem is when this has a return value on it. 
//	pthread_join(thread3, (void**)&ret);
//	pthread_join(thread4, (void**)&ret);
	pthread_join(free1, (void**)&ret);
	
	printf("\x1b[36mreturn value from the thread is %d\x1b[0m\n", *ret);
	pthread_exit(NULL);




}

