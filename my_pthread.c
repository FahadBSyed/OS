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


//Need a queue of running threads

//Need a queue of waiting threads 

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
	printf("\ncreating pthread.\n");
	
	tcb* block = malloc(sizeof(tcb));
	block->context_ptr = malloc(sizeof(ucontext_t));
	//create a context.	
	*thread = block; 
	
	//get a context. 
	getcontext(block->context_ptr);
	
	//@DEBUG: Do error handing on getcontext().
	
	//initialize context members. 
	block->context_ptr->uc_link=0;
	//TODO: figure out the proper protocol to set this pointer. 
	
	block->context_ptr->uc_stack.ss_sp=malloc(4096);
	block->context_ptr->uc_stack.ss_size=4096;
	block->context_ptr->uc_stack.ss_flags=0;
	
	//make the context. 
	makecontext(block->context_ptr, (void*)function, 1, arg);
	
	//@DEBUG: Do error handing on makecontext().
	
	//set the running context
	//@DEBUG: This code shouldn't be here since the scheduler will decide when to start a thread. 
	
	if(currently_running_thread == NULL){
		
		currently_running_thread = block;
		setcontext(block->context_ptr);

	}
	else{
		
		if(running_queue == NULL){
			
			//allocate our first tcb_node, fill the tcb, and set next to null.
			running_queue = malloc(sizeof(tcb_node));
			running_queue->tcb = block;
			running_queue->next = NULL;
		}
		else{
			
			//traverse tcb_node queue until node's next is null.
			tcb_node* running_ptr = running_queue;
			while(running_ptr->next != NULL){
				running_ptr = running_ptr->next;
			}
			
			//create a next node and assign it to the end of the queue.
			running_ptr->next = malloc(sizeof(tcb_node));
			running_ptr->next->tcb = block;
			running_ptr->next->next = NULL;
			running_queue->next = running_ptr->next;
		}
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
	
	
	//@DEBUG: We don't want to do this once we start implementing join. Or do we? 
	//move the currently_running_thread to block and free block. 
	tcb* block = currently_running_thread;
	printf("\nfreeing block.\n");
	free(block);
	
	//yielding stuff
	if(running_queue != NULL){
		printf("Running queue is not empty.\n");
		
		//set currently running thread to top of running queue and remove first entry from running queue. 
		currently_running_thread = running_queue->tcb;
		
		tcb_node* running_ptr = running_queue;
		running_queue = running_queue->next;
		running_ptr->next = NULL;
		setcontext(currently_running_thread->context_ptr); //segfaulting line
	}
	else{
		printf("Running queue is empty.\n");
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
	return 0;
};

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
	
	
	//TODO: get the context from the global tcb struct
	/*
	Check if calling thread has terminated.
	
	if target thread's termination flag == true:
		Continue execution.
		Put target thread's ID in the value pointer. 
		Return 0 successfully.
	else:
		Suspend our thread's execution.
		While target thread's termination flag == false:
			get target thread's termination flag.
		Continue execution.
		Put the target thread's ID in the value pointer. 
		Return 0 successfully.
	
	If error:
		return error code. 
	*/
	
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

