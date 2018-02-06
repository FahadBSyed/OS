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
		
		
*/

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
	printf("creating pthread...\n");
	
	tcb* block = malloc(sizeof(tcb));
	//create a context. 
	ucontext_t context;
	block->context_ptr = &context; 
	*thread = block; 
	
	//get a context. 
	getcontext(&context);
	
	//@DEBUG: Do error handing on getcontext().
	
	//initialize context members. 
	context.uc_link=0;
	//TODO: figure out the proper protocol to set this pointer. 
	
	context.uc_stack.ss_sp=malloc(4096);
	context.uc_stack.ss_size=4096;
	context.uc_stack.ss_flags=0;
	
	//make the context. 
	makecontext(&context, (void*)function, 1, arg);
	
	//@DEBUG: Do error handing on makecontext().
	
	//set the running context
	//@DEBUG: This code shouldn't be here since the scheduler will decide when to start a thread. 
	setcontext(&context);
	printf("done.\n");
	return 0; 
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	
	//Fire some sort of signal.
	
	return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {
	
	
	//TODO: get current active tcb. 
	raise(1);
	
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
	
	return 0;
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

