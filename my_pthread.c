// File:	my_pthread.c
// Author:	Yujie REN
// Date:	09/23/2017

// name:
// username of iLab:
// iLab Server:

#include "my_pthread_t.h"


/*
	NOTES:
		I think the scheduler is supposed to set the current context. 
		We need a way to pass context information from this library to the scheduler.
		The scheduler should have a linked list or priority heap of contexts. 
		
		Dr. Francisco was saying that we should have the scheduler live within this library and every
		thread function call should call the scheduler and get it to do what it needs. 
		Signal handler is basically just a call to yield. 
		
		In scheduler, we have to implement mutexes, mutexes are ways to protect memory. Scheduler can
		also protect memory. We should use the scheduler to check if a mutex is currently in used and
		the scheduler should not schedule them to run until someone calls unlock() on the mutex
		
		When we lock or unlock a mutex, we modify the state of who gets to run, when. If we lock, this
		context cannot be computed unless the scheduler says its okay.
		
		When we create, join, or exit, we change scheduling state.
		Yield does not change scheduling state - it relinquishes remaining runtime
		Yield is a direct call to the scheduler
		Yield is essentially the scheduler.
		
		Join - update state, then yield
		Exit - update state, then yield
		Create - add a context to the system, figure out who runs next, then yield
		Yield - the scheduler 
		
		signal handler .. yield!
		
		TODO: figure out how to return to main if we don't have context for it. 
*/

void start_timer(){
	
	if(timer == NULL){
		printf("ERROR: timer was uninitialized.\n");
		return;
	}
	
	timer->it_value.tv_sec = 0;
	timer->it_value.tv_usec = 250;
	timer->it_interval.tv_sec = 0;
	timer->it_interval.tv_usec = 250;
	setitimer (ITIMER_REAL, timer, NULL);
	return;
}

void pause_timer(){
	
	if(timer == NULL){
		printf("ERROR: timer was uninitialized.\n");
		return;
	}
	timer->it_value.tv_sec = 0;
	timer->it_value.tv_usec = 0;
	timer->it_interval.tv_sec = 0;
	timer->it_interval.tv_usec = 0;
	setitimer (ITIMER_REAL, timer, NULL);
	return;
}

void print_wait_queue(){
	printf("wait queue:\n");
	if(waiting_queue == NULL){
		printf("\t\x1b[33mwait queue is null.\x1b[0m\n");
	}
	else{
		tcb_node* waiting_ptr = waiting_queue;
		while(waiting_ptr != NULL){
			printf("\t\x1b[33mthread (tcb*) %x\x1b[0m\n", waiting_ptr->tcb);
			waiting_ptr = waiting_ptr->next;
		}
	}
	printf("\n");
}

//This function takes the address of one of our queues. It then removes the first node in the queue.
//TODO: possibly clean up and free taken node.
tcb* dequeue(tcb_node** queue){
	
	
	if(*queue == NULL || (*queue)->tcb == NULL){
		printf("ERROR: queue or queue's tcb is null.\n");
		return NULL;
	}
	tcb_node* queue_ptr = *queue;
	*queue = (*queue)->next;
	queue_ptr->next = NULL;
	
	//printf("\t\x1b[32mdequeued thread (tcb*) %x.\x1b[0m\n", queue_ptr->tcb);
	return queue_ptr->tcb;
}

//This function takes the address of one of our queues and a pointer to a tcb. It then adds the tcb
//to the end of the queue. If the queue doesn't exist, it allocates it. 
void enqueue(tcb_node** queue, tcb* block){
	
	if(*queue == NULL){
		*queue = malloc(sizeof(tcb_node));
		(*queue)->tcb = block;
		(*queue)->next = NULL;
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
	}
	//printf("\t\x1b[32menqueued thread (tcb*) %x.\x1b[0m\n", block);

}


/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
	if(timer != NULL){
		pause_timer();
	}
	printf("\nCREATE:");
	
	//create the main context if all of our data structures are unallocated. 
	//The main context is the thread that represents main()
	char make_main = 0;
	if(running_queue == NULL && waiting_queue == NULL && currently_running_thread == NULL){
		
		make_main = 1;
		
		printf("\tMaking timer.\n");
		sa = malloc(sizeof(struct sigaction));
		timer = malloc(sizeof(struct itimerval)); 
		memset(sa, 0, sizeof(*sa));
		memset(timer, 0, sizeof(*timer));
		sa->sa_handler = &my_pthread_yield;
		sigaction(SIGALRM, sa, NULL);
	}
	
	//Creates a new context pointed to by block.	
	tcb* block = malloc(sizeof(tcb));
	block->context_ptr = malloc(sizeof(ucontext_t));
	*thread = block; 
	getcontext(block->context_ptr);
	
	//@DEBUG: Do error handing on getcontext().
	
	//initialize context members including uc_link and stack.
	block->context_ptr->uc_link=0;	//TODO: set this to call exit. Figure out what to do if the thread already exited.
	block->context_ptr->uc_stack.ss_sp=malloc(4096);
	block->context_ptr->uc_stack.ss_size=4096;
	block->context_ptr->uc_stack.ss_flags=0;
	
	makecontext(block->context_ptr, (void*)function, 1, arg);
	
	//@DEBUG: Do error handing on makecontext().
	enqueue(&running_queue, block);
	printf("\tadded thread (tcb*) %x to running queue.\n", block);
	
	//Make the context at the end so we don't redo the creation of the thread.
	if(make_main){
		//allocate our main context.
		tcb* main_block = malloc(sizeof(tcb));
		main_block->context_ptr = malloc(sizeof(ucontext_t));
		currently_running_thread = main_block;
		printf("\nMAIN:\trunning main thread (tcb*) %x.\n", main_block);
		getcontext(main_block->context_ptr);
	}
	start_timer();
	return 0; 
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	
	if(timer != NULL){	
		pause_timer();
	}
	
	printf("\x1b[32m\ttime's up\x1b[0m\n");
	//Fire some sort of signal.
	//swap currently running context with the first context in the queue. 
	
	
	if(running_queue != NULL){
		tcb* block = currently_running_thread;
		enqueue(&running_queue, block);
		currently_running_thread = dequeue(&running_queue);
		printf("\trunning thread (tcb*) %x from running queue.\n", currently_running_thread);
		swapcontext(block->context_ptr, currently_running_thread->context_ptr);
		return;
	}
	else{
		printf("\tRunning queue is empty.\n");
	}
	//figure out how to refactor exit(), create(), and join() scheduling stuff into this.
	
	if(timer != NULL){
		start_timer();
		printf("starting timer.\n");
	}
	return 0;
};

/* terminate a thread */
//TODO: figure out how to store return value correctly.
void my_pthread_exit(void *value_ptr) {
	
	if(timer != NULL){
		pause_timer();
	}
	
	printf("\nEXIT:");
	//@DEBUG: We don't want to do this once we start implementing join. Or do we? 
	
	//move the currently_running_thread to block and free block. 
	tcb* block = currently_running_thread;
	tcb* joiner = NULL;
	printf("\tthread (tcb*) %x is exiting.\n", currently_running_thread);
	if(currently_running_thread->joining_thread_ptr){
		
		joiner = currently_running_thread->joining_thread_ptr;
		joiner->value_ptr = value_ptr;
		printf("\tthread (tcb *) %x was joined by thread (tcb*) %x.\n", currently_running_thread, joiner);
	}
	free(block);

	//if there was a thread waiting on us, put in in the run queue and wake it up.
	if(joiner){
		
		//find the joiner, remove it from the list. 
		if(waiting_queue != NULL){
			
			tcb_node* prev_ptr = NULL;
			tcb_node* waiting_ptr = waiting_queue;
			
			while(waiting_ptr != NULL && waiting_ptr->tcb != joiner){
				prev_ptr = waiting_ptr;
				waiting_ptr = waiting_ptr->next;
			}
			if(waiting_ptr == NULL){
				printf("Could not find the joiner.\n");
			}
			
			printf("\tfound the joiner.\n");
			//covers case where joiner is at the top of the list.
			if(prev_ptr == NULL){
				currently_running_thread = dequeue(&waiting_queue);
				printf("\trunning thread (tcb*) %x from waiting queue.\n", currently_running_thread);

				if(timer != NULL){
					start_timer();
					printf("starting timer.\n");
				}
				setcontext(currently_running_thread->context_ptr);
			}
			else{
				prev_ptr->next = waiting_ptr->next;
				currently_running_thread = waiting_ptr->tcb;
				waiting_ptr->next = NULL;
				printf("\trunning thread (tcb*) %x from waiting queue.\n", currently_running_thread);

				if(timer != NULL){
					start_timer();
					printf("starting timer.\n");
				}
				setcontext(currently_running_thread->context_ptr);
			}
			return;
			
		}
		else{
			printf("ERROR: the terminating thread was joined, but no waiting queue exists.\n");
		}
	}

	if(running_queue != NULL){		
		//set currently running thread to top of running queue and remove first entry from running queue. 
		currently_running_thread = dequeue(&running_queue);
		printf("\trunning thread (tcb*) %x from running queue.\n", currently_running_thread);
		
		//DEBUG
		if(timer != NULL){
			start_timer();
			printf("starting timer.\n");
		}
		setcontext(currently_running_thread->context_ptr);
		return;
	}
	else{
		printf("\tRunning queue is empty.\n");
	}
	return;
};

/* wait for thread termination */
//TODO: figure out how to retrieve return value correctly.
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
	
	if(timer!= NULL){
		pause_timer();
	}
	
	printf("\nJOIN:");
	
	//DEBUG
	
	thread->joining_thread_ptr = currently_running_thread;
	thread->value_ptr = value_ptr;
	printf("\tthread (tcb *) %x joins thread (tcb*) %x.\n", currently_running_thread, thread);
	enqueue(&waiting_queue, currently_running_thread);
	getcontext(currently_running_thread->context_ptr);

	if(running_queue != NULL){		
		//set currently running thread to top of running queue and remove first entry from running queue. 
		
		currently_running_thread = dequeue(&running_queue);
		printf("\trunning thread (tcb *) %x.\n", currently_running_thread);
		
		if(timer != NULL){
			start_timer();
			printf("starting timer.\n");
		}
		
		setcontext(currently_running_thread->context_ptr); 
	}
	

	return;
	/*
	
	The terminating thread would set a flag to indicate termination and broadcast a condition that is part of that state; 
	a joining thread would wait on that condition variable. 
	While such a technique would allow a thread to wait on more complex conditions (for example, waiting for multiple 
	threads to terminate), waiting on individual thread termination is considered widely useful. Also, including the 
	pthread_join() function in no way precludes a programmer from coding such complex waits. Thus, while not a primitive, 
	including pthread_join() in this volume of POSIX.1-2008 was considered valuable.
	
	The pthread_join() function provides a simple mechanism allowing an application to wait for a thread to terminate. 
	After the thread terminates, the application may then choose to clean up resources that were used by the thread. 
	For instance, after pthread_join() returns, any application-provided stack storage could be reclaimed.
	
	The pthread_join() or pthread_detach() function should eventually be called for every thread that is created with 
	the detachstate attribute set to PTHREAD_CREATE_JOINABLE so that storage associated with the thread may be reclaimed.
	
	
	*/
	
};

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
	if(mutex == NULL){
		return -1;
	}

	mutex->flag = 0;	
	return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	if(mutex == NULL) {
		return -1;
		return;
	}

	if(mutex->flag == 0) {
		mutex->flag = 1;
	}	

	while(__atomic_test_and_set(&(mutex->flag), 1) == 1){
		return;
	}

	return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	if (mutex==NULL)
	{
		return -1;
	}
	__sync_synchronize();
	mutex->flag = 0;
	
	return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
	if(mutex == NULL){
		return -1;
	}

	if(mutex->flag == 0){
		free(mutex);
	}
	return 0;
};

