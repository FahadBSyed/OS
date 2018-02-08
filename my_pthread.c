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


tcb* dequeue(tcb_node* queue){
	if(queue == NULL || queue->tcb == NULL){
		printf("ERROR: queue or queue's tcb is null.\n");
		return NULL;
	}
	tcb* block = queue->tcb;
	tcb_node * list_ptr = queue;
	queue = queue->next;
	list_ptr->next = NULL;
	return block;
}


/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
	printf("\nCREATE:");
	
	//create the main context if all of our data structures are unallocated.
	char make_main = 0;
	if(running_queue == NULL && waiting_queue == NULL && currently_running_thread == NULL){
		
		make_main = 1;
	}
	
	//create a context.	
	tcb* block = malloc(sizeof(tcb));
	block->context_ptr = malloc(sizeof(ucontext_t));
	*thread = block; 
	getcontext(block->context_ptr);
	
	//@DEBUG: Do error handing on getcontext().
	
	//initialize context members. 
	block->context_ptr->uc_link=0;
	//TODO: set this to call exit. Figure out what to do if the thread already exited.  
	
	block->context_ptr->uc_stack.ss_sp=malloc(4096);
	block->context_ptr->uc_stack.ss_size=4096;
	block->context_ptr->uc_stack.ss_flags=0;
	
	//make the context. 
	makecontext(block->context_ptr, (void*)function, 1, arg);
	//@DEBUG: Do error handing on makecontext().
	

	if(running_queue == NULL){
		running_queue = malloc(sizeof(tcb_node));
		running_queue->tcb = block;
		running_queue->next = NULL;
		printf("\tadded thread (tcb*) %x to running queue.\n", block);
	}
	else{
		//traverse tcb_node queue until node's next is null.
		tcb_node* running_ptr = running_queue;
		while(running_ptr->next != NULL && running_ptr->next->tcb != NULL){
			running_ptr = running_ptr->next;
		}
		//create a next node and assign it to the end of the queue.
		running_ptr->next = malloc(sizeof(tcb_node));
		running_ptr->next->tcb = block;
		running_ptr->next->next = NULL;
		printf("\tadded thread (tcb*) %x to running queue.\n", block);
	}
	
	if(make_main){
		//allocate our main context.
		tcb* main_block = malloc(sizeof(tcb));
		main_block->context_ptr = malloc(sizeof(ucontext_t));
		getcontext(main_block->context_ptr);
		currently_running_thread = main_block;
		printf("\nMAIN:\trunning main thread (tcb*) %x.\n", main_block);
	}
	return 0; 
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	
	//Fire some sort of signal.
	
	return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {
	
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
			
			//move waiting_ptr to the joiner, move prev_ptr to 1 before the waiting_ptr.
			while (waiting_ptr != NULL && waiting_ptr->tcb != joiner){
				prev_ptr = waiting_ptr;
				waiting_ptr = waiting_ptr->next;
			}
			//we are at the joiner.
			if(prev_ptr != NULL){
				prev_ptr->next = waiting_ptr->next;
			}
			waiting_ptr->next = NULL;
			currently_running_thread = waiting_ptr->tcb;
			printf("\trunning thread (tcb*) %x from waiting queue.\n", currently_running_thread);
			setcontext(currently_running_thread->context_ptr);
			return;
			
			
		}
		else{
			printf("ERROR: the terminating thread was joined, but no waiting queue exists.\n");
		}
	}
	//yielding stuff
	if(running_queue != NULL){		
		//set currently running thread to top of running queue and remove first entry from running queue. 
		currently_running_thread = running_queue->tcb;
		tcb_node* running_ptr = running_queue;
		running_queue = running_queue->next;
		running_ptr->next = NULL;
		printf("\trunning thread (tcb*) %x from running queue.\n", currently_running_thread);
		setcontext(currently_running_thread->context_ptr);
		return;
	}
	else{
		printf("\tRunning queue is empty.\n");
	}
	
	//termination flag = true; 
	/*
	terminates the calling thread and returns a value via value_ptr that is available to
	another thread in the same process that calls pthread_join.

    Thread is cleaned up.

	Process-shared resources (e.g., mutexes, condition variables, semaphores, and file descriptors) 
	are not released. User must do this.

	After the last thread in a process terminates, the process terminates as by calling exit(3) with an exit status of zero; 
	thus, process-shared resources are released 
	
	*/
	return;
};

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
	printf("\nJOIN:");
	thread->joining_thread_ptr = currently_running_thread;
	thread->value_ptr = value_ptr;
	printf("\tthread (tcb *) %x joins thread (tcb*) %x.\n", currently_running_thread, thread);
	if(waiting_queue == NULL){
		
		printf("\tadding thread (tcb *) %x to waiting queue.\n", currently_running_thread);
		waiting_queue = malloc(sizeof(tcb_node));
		waiting_queue->tcb = currently_running_thread;
		waiting_queue->next = NULL;
	}
	else{
		
		printf("\tadding thread (tcb *) %x to waiting queue.\n", currently_running_thread);
		//traverse tcb_node queue until node's next is null.
		tcb_node* waiting_ptr = waiting_queue;
		while(waiting_ptr->next != NULL){
			waiting_ptr = waiting_ptr->next;
		}
		
		//create a next node and assign it to the end of the queue.
		waiting_ptr->next = malloc(sizeof(tcb_node));
		waiting_ptr->next->tcb = currently_running_thread;
		waiting_ptr->next->next = NULL;
	}
	
	if(running_queue != NULL){		
		//set currently running thread to top of running queue and remove first entry from running queue. 
		currently_running_thread = running_queue->tcb;
		tcb_node* running_ptr = running_queue;
		running_queue = running_queue->next;
		running_ptr->next = NULL;
		printf("\trunning thread (tcb *) %x.\n", currently_running_thread);
		setcontext(currently_running_thread->context_ptr); 
	}
	else{
		printf("ERROR: running queue is empty. Running queue should never be empty for create to work. \n");
	}
	
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
	
	return;
};

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
	return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
	return 0;
};

