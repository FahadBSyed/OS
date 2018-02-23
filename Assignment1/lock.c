#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include "my_pthread_t.h"


int balance = 0;
pthread_mutex_t lock;

void* add100(){


        int l;
	int u;
	printf("Lock flag before: %c\n",lock.flag);
        l = pthread_mutex_lock(&lock);
        printf("\n\x1b[36mwe called add100. lock: %d\x1b[0m\n",l);

        balance = balance + 100;
        printf("\n\x1b[36mBalance: %d  mutex: %c.     NULL: \0 \x1b[0m\n",balance,lock.flag);
        u = pthread_mutex_unlock(&lock);
	printf("\n\x1b[36munlock: %d  mutex: %c.\x1b[0m\n",u,lock.flag);
        pthread_exit(NULL);
}

void* add10(){
        int l;
        l = pthread_mutex_lock(&lock);
//        printf("\n\x1b[36mwe called add10. lock2: %d\x1b[0m\n",l);
	balance+=10;

        pthread_mutex_unlock(&lock);


        pthread_exit(NULL);
}

void* sub100(){


        int l;
        l = pthread_mutex_lock(&lock);
        printf("\n\x1b[36mwe called sub100. lock3: %d\x1b[0m\n",l);

        balance -= 100;
        pthread_mutex_unlock(&lock);


        pthread_exit(NULL);
}


int main(int argc, char** argv){
	 printf("\x1b[36mhello world.\x1b[0m\n");
	int a = 10;
        int  init = pthread_mutex_init(&lock, NULL);

        printf("init: %d\n",init);

	pthread_t tid[a];
        int i = 0;
        while(i<a){
                pthread_create(&tid[i], NULL, add100, NULL);
                i++;
        }


	int* ret;
        int counter = 0;
        int math = 0;
        while(counter < 90000){
                int math = math + counter * 2;
                counter++;
        }
	
	int c =0;
        while(c<a){
                printf(" Join: %d\n",c);
                pthread_join(tid[c],NULL);
                c++;
        }

        printf("balance: %d\n",balance);
        pthread_mutex_destroy(&lock);

        pthread_exit(NULL);
}







