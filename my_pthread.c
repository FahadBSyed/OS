// File:	my_pthread.c
// Author:	Yujie REN
// Date:	09/23/2017

// name: Fahad Syed, Travis Thiel, Austin Tsai
// username of iLab: fbs14
// iLab Server: cd.cs.rutgers.edu

#include "my_pthread_t.h"


//Helper function used in scheduler to determine which queue should be ran.
float random_0_100()
{	  
      float r = (float)rand() / (float)RAND_MAX;
      return r;
}

//Helper function used in the scheduler to initialize the signal handler.
void init_signal_handler(){
	
	//seed the scheduler's random number generator
	srand(time(NULL));

	gettimeofday(&my_pthread_start_time, NULL);
	//printf("start:%d:%d\n",my_pthread_start_time.tv_sec, my_pthread_start_time.tv_usec);
	
	sigemptyset(&block_mask);
	sigaddset(&block_mask, SIGALRM);
	
	sa = malloc(sizeof(struct sigaction));
	timer = malloc(sizeof(struct itimerval)); 
	memset(sa, 0, sizeof(*sa));
	memset(timer, 0, sizeof(*timer));
	
	sa->sa_handler = (void*)&my_pthread_yield;
	sigaction(SIGALRM, sa, NULL);
	sa->sa_mask = block_mask;
	
	timer->it_value.tv_sec = 0;
	timer->it_value.tv_usec = 8000;
	timer->it_interval.tv_sec = 0;
	timer->it_interval.tv_usec = 8000;
	setitimer (ITIMER_REAL, timer, NULL);
}

//Debug function used in the scheduler to show queues.
void print_queue(tcb_node** queue, char * str){
	printf("\t\t%s:\n", str);
	if(*queue == NULL){
		printf("\t\t\x1b[33m queue is null.\x1b[0m\n");
	}
	else{
		tcb_node* queue_ptr = *queue;
		while(queue_ptr != NULL){
			printf("\t\t\x1b[33mthread (tcb*) %x\twait time:%d:%d\x1b[0m\n", queue_ptr->tcb, queue_ptr->tcb->wait_time.tv_sec, queue_ptr->tcb->wait_time.tv_usec);
			queue_ptr = queue_ptr->next;
		}
	}
	//printf("\n");
}

//This helper function takes the address of one of our queues. It then removes the first node in the queue.
tcb* dequeue(tcb_node** queue){
	//printf("\n\tDequeue.\n");
	if(*queue == NULL || (*queue)->tcb == NULL){
		printf("ERROR: queue or queue's tcb is null.\n");
		return NULL;
	}
	//if(*queue == short_run_queue){print_queue(&short_run_queue, "short run queue");}
	//if(*queue == med_run_queue){print_queue(&med_run_queue, "medium run queue");}
	//if(*queue == FIFO_run_queue){print_queue(&FIFO_run_queue, "FIFO run queue");}
	//if(*queue == terminated_queue){print_queue(&terminated_queue, "terminated queue");}
	//if(*queue == waiting_queue){print_queue(&waiting_queue, "wait queue");}
	tcb_node* queue_ptr = *queue;
	*queue = (*queue)->next;
	queue_ptr->next = NULL;
	
	return queue_ptr->tcb;
}

//This helper function takes the address of one of our queues and a pointer to a tcb. It then adds the tcb
//to the end of the queue. If the queue doesn't exist, it allocates it. 
void enqueue(tcb_node** queue, tcb* block){
	//printf("\n\tEnqueue.\n");
	if(*queue == NULL){
		*queue = malloc(sizeof(tcb_node));
		(*queue)->tcb = block;
		(*queue)->next = NULL;
		//if(*queue == short_run_queue){print_queue(&short_run_queue, "short run queue");}
		//if(*queue == med_run_queue){print_queue(&med_run_queue, "medium run queue");}
		//if(*queue == FIFO_run_queue){print_queue(&FIFO_run_queue, "FIFO run queue");}
		//if(*queue == terminated_queue){print_queue(&terminated_queue, "terminated queue");}
		//if(*queue == waiting_queue){print_queue(&waiting_queue, "wait queue");}
	}
	else{
		//traverse tcb_node queue until node's next is null.
		tcb_node* ptr = *queue;
		while(ptr->next != NULL){
			ptr = ptr->next;
		}
		//create a next node and assign it to the end of the queue.
		ptr->next = malloc(sizeof(tcb_node));
		ptr->next->tcb = block;
		ptr->next->next = NULL;
		//if(*queue == short_run_queue){print_queue(&short_run_queue, "short run queue");}
		//if(*queue == med_run_queue){print_queue(&med_run_queue, "medium run queue");}
		//if(*queue == FIFO_run_queue){print_queue(&FIFO_run_queue, "FIFO run queue");}
		//if(*queue == terminated_queue){print_queue(&terminated_queue, "terminated queue");}
		//if(*queue == waiting_queue){print_queue(&waiting_queue, "wait queue");}
	}

}

//This helper function runs the queue given by tcb_node** queue and does proper maintenance and state alteration.
void run_from_queue(tcb_node** queue){
	
	schedule_lock = 1;
	//if(*queue == short_run_queue){printf("RUN: short run queue\n");}
	//if(*queue == med_run_queue){printf("RUN: medium run queue\n");}
	//if(*queue == FIFO_run_queue){printf("RUN: FIFO run queue\n");}
	
	//update wait times.
	tcb_node* queue_ptr = short_run_queue;
	tcb_node* prev_ptr = NULL;
	struct timeval elapsed;
	elapsed.tv_sec = my_pthread_end_time.tv_sec - my_pthread_start_time.tv_sec;
	elapsed.tv_usec = my_pthread_end_time.tv_usec - my_pthread_start_time.tv_usec;
	
	//printf("\telapsed: %d:%d\n", elapsed.tv_sec, elapsed.tv_usec);
	while(queue_ptr != NULL){
		queue_ptr->tcb->wait_time.tv_sec = elapsed.tv_sec;
		queue_ptr->tcb->wait_time.tv_usec = elapsed.tv_usec;
		queue_ptr = queue_ptr->next;
	}

	queue_ptr = med_run_queue;
	while(queue_ptr != NULL){
		queue_ptr->tcb->wait_time.tv_sec = elapsed.tv_sec;
		queue_ptr->tcb->wait_time.tv_usec = elapsed.tv_usec;
		if(queue_ptr->tcb->wait_time.tv_sec == 1){
			//printf("PROMOTING.\n");
			if(prev_ptr != NULL){
				prev_ptr->next = queue_ptr->next;
				queue_ptr->next = NULL;
				enqueue(&short_run_queue, queue_ptr->tcb);
				queue_ptr = prev_ptr->next;
			}
			else{
				tcb* temp_block = dequeue(&med_run_queue);
				enqueue(&short_run_queue, temp_block);
				queue_ptr = med_run_queue;
			}
		}
		else{
			prev_ptr = queue_ptr;
			queue_ptr = queue_ptr->next;
		}
	}
	
	queue_ptr = FIFO_run_queue;
	while(queue_ptr != NULL){
		queue_ptr->tcb->wait_time.tv_sec = elapsed.tv_sec;
		queue_ptr->tcb->wait_time.tv_usec = elapsed.tv_usec;
		if(queue_ptr->tcb->wait_time.tv_sec == 2){
			//printf("PROMOTING.\n");
			if(prev_ptr != NULL){
				prev_ptr->next = queue_ptr->next;
				queue_ptr->next = NULL;
				enqueue(&short_run_queue, queue_ptr->tcb);
				queue_ptr = prev_ptr->next;
			}
			else{
				tcb* temp_block = dequeue(&FIFO_run_queue);
				enqueue(&short_run_queue, temp_block);
				queue_ptr = FIFO_run_queue;
			}
		}
		else{
			prev_ptr = queue_ptr;
			queue_ptr = queue_ptr->next;
		}
	}
	queue_ptr = waiting_queue;
	while(queue_ptr != NULL){
		queue_ptr->tcb->wait_time.tv_sec = elapsed.tv_sec;
		queue_ptr->tcb->wait_time.tv_usec = elapsed.tv_usec;
		queue_ptr = queue_ptr->next;
	}
	
	//demote current thread to lower queue.
	tcb* block = currently_running_thread;
	if(block != NULL){
		
		if(elapsed.tv_sec != 0 && elapsed.tv_usec < timer->it_interval.tv_usec - 1000){
			
			//do not demote.
			//printf("currently running thread (tcb*) %x.\n", currently_running_thread);
			//printf("\nshift queues for current thread.\n");
			if(timer->it_interval.tv_usec == 8000){enqueue(&short_run_queue, block);}
			else if(timer->it_interval.tv_usec == 16000){enqueue(&med_run_queue, block);}
		}
		else{
			
			//demote.
			//printf("currently running thread (tcb*) %x.\n", currently_running_thread);
			//printf("\nshift queues for current thread.\n");
			if(timer->it_interval.tv_usec == 8000){enqueue(&med_run_queue, block);}
			else if(timer->it_interval.tv_usec == 16000){enqueue(&FIFO_run_queue, block);}				
		}
	}
	
	if(*queue == short_run_queue){
		//set timer to short.
		timer->it_value.tv_usec = 8000;
		timer->it_interval.tv_usec = 8000;
		setitimer (ITIMER_REAL, timer, NULL);
		
		currently_running_thread = dequeue(&short_run_queue);
		//printf("\trunning thread (tcb*) %x from short.\n", currently_running_thread);	

	}
	else if(*queue == med_run_queue){
		//set timer to medium.
		timer->it_value.tv_usec = 16000;
		timer->it_interval.tv_usec = 16000;
		setitimer (ITIMER_REAL, timer, NULL);	

		currently_running_thread = dequeue(&med_run_queue);
		//printf("\trunning thread (tcb*) %x from med.\n", currently_running_thread);	
	}
	else if(*queue == FIFO_run_queue){
		//set timer to FIFO.
		timer->it_value.tv_usec = 0;
		timer->it_value.tv_sec = 0;
		timer->it_interval.tv_usec = 0;
		timer->it_interval.tv_sec = 0;
		setitimer (ITIMER_REAL, timer, NULL);

		currently_running_thread = dequeue(&FIFO_run_queue);
		//printf("\trunning thread (tcb*) %x from FIFO.\n", currently_running_thread);	
	}
	
	
	if(block != NULL){ 
		gettimeofday(&my_pthread_start_time, NULL);
		gettimeofday(&my_pthread_end_time, NULL);
		block->wait_time.tv_sec = 0;
		block->wait_time.tv_usec = 0;
		currently_running_thread->wait_time.tv_sec = 0;
		currently_running_thread->wait_time.tv_usec = 0;
		schedule_lock = 0;
		swapcontext(block->context_ptr, currently_running_thread->context_ptr); 
	}
	else{ 
		gettimeofday(&my_pthread_start_time, NULL);
		gettimeofday(&my_pthread_end_time, NULL);
		currently_running_thread->wait_time.tv_sec = 0;
		currently_running_thread->wait_time.tv_usec = 0;
		schedule_lock = 0;
		setcontext(currently_running_thread->context_ptr); 
	}
	return;
}


int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {

	schedule_lock = 1;
	//printf("\nCREATE:\n");	
	
	//create the main context if all of our data structures are unallocated. 
	//The main context is the thread that represents main()
	char make_main = 0;
	if(short_run_queue == NULL && med_run_queue == NULL 
	   && FIFO_run_queue == NULL && waiting_queue == NULL && currently_running_thread == NULL){
		make_main = 1;
		init_signal_handler();
	}
	
	//Creates a new context pointed to by block.	
	tcb* block = malloc(sizeof(tcb));
	block->context_ptr = malloc(sizeof(ucontext_t));
	*thread = block; 
	getcontext(block->context_ptr);
	block->wait_time.tv_sec = 0;
	block->wait_time.tv_usec = 0;
	
	//@DEBUG: Do error handing on getcontext().
	
	//initialize context members including uc_link and stack.
	block->context_ptr->uc_link=0;	//TODO: set this to call exit. Figure out what to do if the thread already exited.
	block->context_ptr->uc_stack.ss_sp=malloc(4096);
	block->context_ptr->uc_stack.ss_size=4096;
	block->context_ptr->uc_stack.ss_flags=0;
	
	makecontext(block->context_ptr, (void*)function, 1, arg);
	
	//@DEBUG: Do error handing on makecontext().
	enqueue(&short_run_queue, block);
	//printf("\tadded thread (tcb*) %x to running queue.\n", block);
	
	//Make the context at the end so we don't redo the creation of the thread.
	if(make_main){
		//allocate our main context.
		tcb* main_block = malloc(sizeof(tcb));
		main_block->context_ptr = malloc(sizeof(ucontext_t));
		currently_running_thread = main_block;
		//printf("\nMAIN:\trunning main thread (tcb*) %x.\n", main_block);
		getcontext(main_block->context_ptr);
	}
	schedule_lock = 0;
	return 0; 
};


int my_pthread_yield() {

	if(schedule_lock == 1){
		return;
	}
	
	if(short_run_queue == NULL && med_run_queue == NULL && FIFO_run_queue == NULL){
		//printf("all run queues were empty.\n");
		return 0;
	}
	
	gettimeofday(&my_pthread_end_time, NULL);
	float random = random_0_100();
	//printf("YIELD:\n\trandom: %f\n", random);
	//printf("\tstart:%d:%d\tend:%d:%d\n", my_pthread_start_time.tv_sec, my_pthread_start_time.tv_usec, my_pthread_end_time, my_pthread_end_time.tv_usec);
	if(random <= .5){
		if(short_run_queue != NULL){
			run_from_queue(&short_run_queue);
		}
		else if(med_run_queue != NULL){
			//printf("\tshort queue was empty.\n");
			run_from_queue(&med_run_queue);
		}
		else if(FIFO_run_queue != NULL){
			//printf("\tshort and medium queues were empty.\n");
			run_from_queue(&FIFO_run_queue);
		}
	}
	if(random <= .8){
		if(med_run_queue != NULL){
			run_from_queue(&med_run_queue);
		}
		else if(short_run_queue != NULL){
			//printf("\tmedium queue was empty.\n");
			run_from_queue(&short_run_queue);
		}
		else if(FIFO_run_queue != NULL){
			//printf("\tshort and medium queues were empty.\n");
			run_from_queue(&FIFO_run_queue);
		}
	}
	else{
		if(FIFO_run_queue != NULL){
			run_from_queue(&FIFO_run_queue);
		}
		else if(short_run_queue != NULL){
			//printf("\tFIFO queue was empty.\n");
			run_from_queue(&short_run_queue);
		}
		else if(med_run_queue != NULL){
			//printf("\tFIFO and medium queues were empty.\n");
			run_from_queue(&med_run_queue);
		}
	}
	return;
};

/* terminate a thread */
//TODO: figure out how to store return value correctly.
void my_pthread_exit(void *value_ptr) {
	
	schedule_lock = 1;
	
	if(short_run_queue == NULL && med_run_queue == NULL && FIFO_run_queue == NULL 
	   && waiting_queue == NULL && currently_running_thread == NULL && terminated_queue == NULL
	   && sa == NULL){
		init_signal_handler();
		tcb* main_block = malloc(sizeof(tcb));
		main_block->context_ptr = malloc(sizeof(ucontext_t));
		currently_running_thread = main_block;
		//printf("\nMAIN:\trunning main thread (tcb*) %x.\n", main_block);
		getcontext(main_block->context_ptr);
	}
	
	
	//reset scheduler timers.
	timer->it_value.tv_sec = 0;
	timer->it_value.tv_usec = 8000;
	timer->it_interval.tv_sec = 0;
	timer->it_interval.tv_usec = 8000;
	setitimer (ITIMER_REAL, timer, NULL);
	
	
	//printf("\nEXIT:\n");
	//printf("currently running thread (tcb*): %x\n", currently_running_thread);

	//move the currently_running_thread to block and put block on terminated queue
	tcb* block = currently_running_thread;
	tcb* joiner = NULL;
	
	if(block == NULL){
		printf("ERROR: currently running thread is null.\n");
		printf("exiting...\n");
		exit(1);
	}
	
	//printf("\tthread (tcb*) %x is exiting.\n", block);
	if(block->joining_thread_ptr){
		
		joiner = block->joining_thread_ptr;
		void* ptr = block->value_ptr;
		*(double*)ptr = *(double*)value_ptr;
		//printf("\tthread (tcb *) %x was joined by thread (tcb*) %x.\n", block, joiner);
	}
	else{
		block->value_ptr = value_ptr;
	}
	enqueue(&terminated_queue, block);
	currently_running_thread = NULL;

	//if there was a thread waiting on us, put it in the short running queue.
	if(joiner){
												//Travis use this for unlock!!!!!
		//find the joiner, remove it from the list. 
		if(waiting_queue != NULL){
			
			tcb_node* prev_ptr = NULL;
			tcb_node* waiting_ptr = waiting_queue;
			
			while(waiting_ptr != NULL && waiting_ptr->tcb != joiner){
				prev_ptr = waiting_ptr;
				waiting_ptr = waiting_ptr->next;
			}
			if(waiting_ptr == NULL){
				printf("ERROR: Could not find the joiner.\n");
				printf("exiting...\n");
				exit(1);
			}
			
			//printf("\tfound the joiner.\n");
			//covers case where joiner is at the top of the list.
			if(prev_ptr == NULL){
				tcb* new_block = dequeue(&waiting_queue);
				enqueue(&short_run_queue, new_block);
			}
			//covers all other cases. 
			else{
				prev_ptr->next = waiting_ptr->next;
				tcb* new_block = waiting_ptr->tcb;
				waiting_ptr->next = NULL;
				enqueue(&short_run_queue, new_block);
			}			
		}
		else{
			printf("ERROR: the terminating thread was joined, but no waiting queue exists.\n");
			schedule_lock = 0;
			return;
		}
	}
	
	schedule_lock = 0;
	my_pthread_yield();
	return;
};

int my_pthread_join(my_pthread_t thread, void **value_ptr) {
	
	schedule_lock = 1;
	
	//printf("\nJOIN:\n");
	//printf("currently running thread (tcb*): %x\n", currently_running_thread);

	//check the terminated_queue for the thread we joined. 
	if(terminated_queue != NULL){
		
		tcb_node* queue_ptr = terminated_queue;
		while(queue_ptr != NULL && queue_ptr->tcb != thread){
			queue_ptr = queue_ptr->next;
		}
		if(queue_ptr == NULL){
			//printf("\tthread (tcb*) %x not in terminated queue.\n", thread);
		}
		else if(queue_ptr->tcb == thread){
			//printf("\tfound thread (tcb*): %x in terminated queue\n", currently_running_thread);
			*value_ptr = thread->value_ptr; 
			return;
		}
	}
	
	
	thread->joining_thread_ptr = currently_running_thread;
	thread->value_ptr = *value_ptr;	
	//printf("\tthread (tcb *) %x joins thread (tcb*) %x.\n", currently_running_thread, thread);
	
	tcb* block = currently_running_thread;
	enqueue(&waiting_queue, block);
	getcontext(block->context_ptr);
	
	schedule_lock = 0;
	my_pthread_yield();
	return;
	
};

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {

	if(mutex == NULL){
		printf("Error: mutex was not declared\n");
        	return -1;
        }

        if(mutex->flag!='\0'){
		printf("Error: mutex already initualized");
                return -1;
        }
        mutex->flag = '0';
        
        return 0;



};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
        if(mutex == NULL) {
               	printf("Error: mutex was not declared\n");
		return -1;
        }
        if(mutex->flag=='\0'){
		printf("Error: mutex not initualized");	
                return -1;
        }
 
        if(!__atomic_test_and_set(&(mutex->flag),__ATOMIC_SEQ_CST)){
		tcb* block = currently_running_thread;
                enqueue(&mutex->lock_wait_queue, block);
		my_pthread_yield();
        }

        return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	if (mutex==NULL){
               	printf("Error: mutex was not declared\n");
		return -1;
        }

        if(mutex->flag == '\0'){
		printf("Error: mutex not initualized");	 
                return -1;
        }
        __atomic_store_n(&(mutex->flag),'0',__ATOMIC_SEQ_CST);
	tcb* next_waiting_on_lock = dequeue(&mutex->lock_wait_queue);
	enqueue(&short_run_queue, next_waiting_on_lock);


        return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
        if(mutex == NULL){
            	printf("Error: mutex was not declared\n");
	        return -1;
        }

        if(mutex->flag=='\0'){
                printf("Error: mutex not initualized");	 
                return-1;
        }

        if(!__atomic_test_and_set(&(mutex->flag),__ATOMIC_SEQ_CST)){
             	tcb* block = currently_running_thread;
                enqueue(&mutex->lock_wait_queue, block);
		my_pthread_yield();
        }
	free(mutex->lock_wait_queue);
        mutex->flag = '\0';
        return 0;
};

