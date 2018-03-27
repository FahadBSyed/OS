// File:	my_malloc.h
// Author:	Fahad Syed, Austin Tsai, Travis Thiel
// Date:	03/13/2018
// username of iLab: fbs14
// iLab Server: cd.cs.rutgers.edu

/*

README

This is to get you guys caught up on the state of the project.

== IMPORTANT FILES ========================================================================================
here is a list of everything you need.

my_pthread_t.h 	//this has been modified
my_pthread.c   	//this has been modified 
my_malloc.h		//this is new
my_malloc.c		//this is new
pthread_test.c	//this has been modified
Makefile		//this has been modified

== COMPILING AND RUNNING ==================================================================================
make sure those files are in your ilab directory, then type:
make clean; make; make pthread_test.o; ./pthread_test.o

That command will make everything and run for you. Don't be alarmed if there is a shit ton of output, that's
normal and useful for debugging purposes.


== STATE OF THE PROJECT ===================================================================================
Currently we are on phase B, I've divided our memory (my_memory) into an OS region (pages 9 - 108) and a 
user region (pages 109 - 2044). We need to implement a free() that works across page boundaries.
   
1) my_deallocate() which is supposed to be free() doesn't work as of now. It has to be converted to work with the current
   allocation scheme. So here's what you would have to do.
   - first, mark the segment metadata as unallocated like normal.
   - when combining, instead of going to the start of the page, find the first page this thread owns (vaddr == 1), then combine.
	 - if youre stuck, you can look at printpage() for an example of traversing across pages. 
	 
== WHAT YOU SHOULD DO =======================================================================================
1) read all of the functions I wrote. I commented mostly everything. If I missed stuff, its because its still very work in progress.
2) go to pthread_test.c and comment out most of the crazy threading stuff, then run it and look at the output. This should help you figure out
   how everything works. When you think you understand, uncomment out more threads. 
3) Once you get up to speed (which WILL take a while), try and solve problem 2. 
5) If you have done all those, we can move onto phase C.

*/



#ifndef MY_MALLOC_H
#define MY_MALLOC_H

/* include lib header files that you need here: */
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ucontext.h>
#include <time.h>
#include <sys/mman.h>
#include "my_pthread_t.h"

#define THREADREQ 1
#define LIBRARYREQ 0 

#define page_size (sysconf( _SC_PAGE_SIZE ))
#define mem_size (8 * 1024 * 1024)
#define swap_size (16 * 1024 * 1024)

char* my_memory; 	//pointer to our memaligned' 8 MB my_memory.

FILE* swap;

unsigned int pageToEvic;
unsigned int os_start;	 //address in my_memory where our scheduler's useable pages begin (this is used by the scheduler).
unsigned int user_start; //address in my_memory where our useable pages begin (this is used by the threads).
unsigned int shared_start;
unsigned int swap_start;

unsigned int os_first_free;	//address in my_memory of the first page that is unallocated (this is used by the scheduler).
unsigned int user_first_free; //address in my_memory of the first page that is unallocated (this is used by the threads).
unsigned int swap_first_free;


struct sigaction sa_mem; //signal handler

typedef struct table_row {	//struct representing one row of the page table. 
	
	pthread_t thread;
	unsigned int vaddr;
	unsigned char alloc; 
} table_row;

table_row* table_ptr;
table_row* swap_table_ptr;
unsigned char main_init;

typedef struct segdata {	//struct representing metadata for a segment within a page.
	
	size_t size;
	unsigned char alloc;
	
} segdata;


#define malloc(x) myallocate(x, __FILE__, __LINE__, THREADREQ)
void* myallocate(size_t x, char* file, int line, int req);

#define free(x) mydeallocate(x, __FILE__, __LINE__, THREADREQ)
void mydeallocate(void* x, char* file, int line, int req);

static void handler(int sig, siginfo_t *si, void *unused);

#endif





