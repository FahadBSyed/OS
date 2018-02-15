// File:	my_pthread_t.h
// Author:	Yujie REN
// Date:	09/23/2017

// name:
// username of iLab:
// iLab Server: 
#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H

#define _GNU_SOURCE

/* include lib header files that you need here: */
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

typedef uint my_pthread_t;

typedef struct threadControlBlock {
	
	//the value of the context. 
	ucontext_t* context_ptr;
	
	//the context for the joining thread. 
	struct threadControlBlock* joining_thread_ptr;
	
	//stores the return value of a context. 
	void* value_ptr;
	
	//stores the priority of a thread.
	unsigned int priority; 
	
} tcb; 

/* mutex struct definition */
typedef struct my_pthread_mutex_t {
	/* add something here */
	
	int flag; 
} my_pthread_mutex_t;

/* define your data structures here: */

// Feel free to add your own auxiliary data structures
 typedef struct tcb_node{
	 
	 tcb* tcb; 
	 struct tcb_node* next;
 } tcb_node;

//typedef struct sigaction sigaction;
//typedef struct itimerval itimerval;
 
 tcb* currently_running_thread;
 
 tcb_node* running_queue;
 tcb_node* waiting_queue;
 tcb_node* terminated_queue;
 
struct sigaction* sa;
sigset_t block_mask;
struct itimerval* timer;
int schedule_lock;

/* Function Declarations: */
#define my_pthread_t pthread_t
#define pthread_t tcb*

/* create a new thread */
#define pthread_create(thread,attr,function,arg) my_pthread_create(thread,attr,function,arg)
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg);

/* give CPU pocession to other user level threads voluntarily */
#define pthread_yield() my_pthread_yield()
int my_pthread_yield();

/* terminate a thread */
#define pthread_exit(value_ptr) my_pthread_exit(value_ptr)
void my_pthread_exit(void *value_ptr);

/* wait for thread termination */
#define pthread_join(thread,value_ptr)  my_pthread_join(thread,value_ptr)
int my_pthread_join(my_pthread_t thread, void **value_ptr);

/* initial the mutex lock */
#define pthread_mutex_init(mutex,mutexattr) my__pthread_mutex_init(mutex,mutexattr)
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);

/* aquire the mutex lock */
#define pthread_mutex_lock(mutex) my_pthread_mutex_lock(mutex)
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);

/* release the mutex lock */
#define pthread_mutex_unlock(mutex) my_pthread_mutex_unlock(mutex)
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);

/* destroy the mutex */
#define pthread_mutex_destroy(mutex) my_pthread_mutex_destroy(mutex)
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);

#endif
