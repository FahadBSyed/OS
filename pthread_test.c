#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "my_pthread_t.h"
#include <time.h>

int balance;

pthread_mutex_t lock;

void* test_func4(){
	printf("\n\x1b[36mwe called test_func4.\x1b[0m\n");


	int l = pthread_mutex_lock(&lock);
	printf("balance: %d \n",balance);
	balance+=100;
	balance+=100;
	balance+=100;
	balance+=100;
	balance+=100;
	l = pthread_mutex_unlock(&lock);

	pthread_exit(NULL);

}


void* test_func3(){
	
	printf("\n\x1b[36mwe called test_func3.\x1b[0m\n");
	pthread_exit(NULL);
}

void* test_func2(){
	
	printf("\n\x1b[36mwe called test_func 2.\x1b[0m\n");
	int* D = malloc(sizeof(int));
	*D = 25;
	printf("\x1b[36m*D = %d\x1b[0m\n", *D);
	pthread_t thread3;
	//pthread_create(&thread3, NULL, (void*)&test_func3, NULL);
	
	time_t start, curr_time;
	start = clock();
	while((curr_time - start)/CLOCKS_PER_SEC <= 1){curr_time = clock();}
	
	//pthread_join(thread3, NULL);
	pthread_exit(D);
}

void* test_func(void* x){
	
	printf("\n\x1b[36mwe called test_func.\x1b[0m\n");
	int* c = (int*)x;
	*c *=2;
	printf("\x1b[36m*C = %d\x1b[0m\n", *c);
	printf("\x1b[36mC = %x\x1b[0m\n", c);
	
	pthread_t thread2;
	//printf("\x1b[36mcreating thread 2.\x1b[0m\n");
	//pthread_create(&thread2, NULL, (void*)&test_func2, NULL);
	
	//pthread_join(thread2, NULL);
	pthread_exit(c);
}

int main(int argc, char** argv){
	
	printf("\x1b[36mhello world.\x1b[0m\n");
	
	pthread_mutex_init(&lock,NULL);	

	balance =0;	
	pthread_t thread;
	pthread_t thread2;
	pthread_t thread3;
	pthread_t thread4;

	int* x = malloc(sizeof(int));
	*x = 10;
	int error = pthread_create(&thread, NULL, (void*)&test_func, x);
	int error2 = pthread_create(&thread2, NULL, (void*)&test_func2, x);
	//int error3 = pthread_create(&thread3, NULL, (void*)&test_func, x);
	//int error4 = pthread_create(&thread4, NULL, (void*)&test_func, x);

	if (error != 0 && error2 != 0 /*&& error3 != 0 && error4 != 0*/){
		printf("\x1b[36mcan't create thread :[%s]\x1b[0m", strerror(error));
		return;
	}
	

	int* ret = malloc(sizeof(int));
	time_t start, curr_time;
	start = clock();
	
	printf("\x1b[36mret's address: %x\x1b[0m\n", &ret);
	printf("\x1b[36mret: %x\x1b[0m\n", ret);
	printf("\x1b[36mret's value: %d\x1b[0m\n", *ret);
	
	while((curr_time - start)/CLOCKS_PER_SEC <= 2){curr_time = clock();}

	
	pthread_join(thread, (void**)&ret);
	pthread_join(thread2, (void**)&ret);
	//pthread_join(thread3, (void**)&ret);
	//pthread_join(thread4, (void**)&ret);
	
	printf("\x1b[36mreturn value from the thread is %d\x1b[0m\n", *ret);


	pthread_t tid[10];
	int i = 0;
	int e;	
	while(i<10){
		e =pthread_create(&tid[i],NULL,(void*)&test_func4,NULL);
		if (e!= 0){
			printf("\x1b[36mcan't create thread :[%s]\x1b[0m", strerror(error));
		
		}
		i++;
	}

	
	
	start = clock();
	
	printf("\x1b[36mret's address: %x\x1b[0m\n", &ret);
	printf("\x1b[36mret: %x\x1b[0m\n", ret);
	printf("\x1b[36mret's value: %d\x1b[0m\n", *ret);
	
	while((curr_time - start)/CLOCKS_PER_SEC <= 2){curr_time = clock();}

	i=0;
	while(i<10){
		printf("joining: %d\n",i);
		pthread_join(tid[i], (void**)&ret);
		i++;
	}

	printf("End balance: %d\n",balance);

	pthread_mutex_destroy(&lock);






	pthread_exit(NULL);
}

