// File:	my_malloc.c
// Author:	Fahad Syed, Austin Tsai, Travis Thiel
// Date:	03/13/2018
// username of iLab: fbs14
// iLab Server: cd.cs.rutgers.edu

#include "my_malloc.h"

//This function goes through the page table and mprotects any page belonging to 'thread'
void protectthreadpages(pthread_t thread, int debug){
	
	unsigned int i = 0;
	table_row* table = table_ptr;
	for (i = user_start/page_size; i < mem_size/page_size; i++){
		
		if((*(table + i)).thread == thread && (*(table + i)).alloc == 1){
			mprotect( (my_memory+(i*page_size)), page_size, PROT_NONE);
			if(debug){printf("page: %d address: %x - %x is protected.\n", i, (my_memory+(i*page_size)), (my_memory+((i+1)*page_size) - 1));}
		}
	}
	if(debug) printf("\n");
}

//This function goes through the page table and removes mprotection from any page belonging to 'thread'
void unprotectthreadpages(pthread_t thread, int debug){
	unsigned int i = 0;
	table_row* table = table_ptr;
	for (i = user_start/page_size; i < mem_size/page_size; i++){
		
		if(table[i].thread == thread && table[i].alloc == 1){
			mprotect( (my_memory+(i*page_size)), page_size, PROT_READ | PROT_WRITE);
			if(debug){printf("page: %d address: %x - %x is unprotected.\n", i, (my_memory+(i*page_size)), (my_memory+((i+1)*page_size) - 1));}
		}
	}
	if(debug) printf("\n");
}


//This debug function prints out which pages are allocated and which threads own them. 
void printmem(){
	
	printf("printing my_memory: \n");
	unsigned int i = 0;
	table_row* table = table_ptr;
	for (i = os_start/page_size; i < mem_size/page_size; i++){
		
		if(table[i].alloc == 0){
			//printf("is free.\n");
			continue;
		}
		else{
			printf("page %d thread: %x vaddr: %x\n", i, table[i].thread, table[i].vaddr);
		}
	}
	printf("\n");
}

//This debug function prints out the segment metadata for a given page.
void printpage(int page, int req){

	table_row* table = table_ptr;
	unsigned int target_vaddr;
	
	if(req == 1){
		target_vaddr = page - os_start/page_size + 1;
	}
	else{
		target_vaddr = page - user_start/page_size + 1;
	}
	
	if(table[page].alloc == 0){
		printf("page %d is unallocated.\n", page);
		return;
	}
	if((page != os_start/page_size && page != user_start/page_size) && table[page - 1].vaddr != table[page].vaddr - 1){
		if(table[page].thread == table[page-1].thread){
			printf("ERROR: pages are out of order. vaddress of page %d is %x while vaddress of page %d is %x\n", page-1, table[page-1].vaddr, page, table[page].vaddr);
			exit(1);
		}
		else if(table[page].thread != table[page-1].thread){
			printf("page %d has been swapped out.\n", page);
			return;
		}
	}

	printf("printing page %d:\n", page);
	
	int start_page = -1;
	if(req == LIBRARYREQ){
		start_page = os_start/page_size;
	}
	else{
		start_page = user_start/page_size;
	}
	int i = start_page * page_size;
	segdata meta;
	meta.alloc =0; meta.size = 0;
	while(i/page_size != page){
		if(i/page_size > page){
			printf("meta data allocated between page boundaries.\n");
			return;
		}
		
		printf("not at our page: target: %d current: %d offset: %d ", page, i/page_size, i%page_size);
		memcpy(&meta, (my_memory+i), sizeof(meta));
		printf("alloc: %d size: %d\n", meta.alloc, meta.size);

		if((int)meta.size < 0){
			printf("ERROR: negative size:\n");
			exit(1);
		}
		if(meta.alloc > 1 || meta.alloc < 0){
			printf("ERROR: alloc is wrong value.\n");
			exit(1);
		}
		
		i += meta.size + sizeof(meta);
	}
	
	if(meta.alloc == 0 && meta.size == 0){
		printf("first page.\n");
		memcpy(&meta, (my_memory+i), sizeof(meta));
	}
	else{
		printf("alloc: %d size: %d <-- from previous page.\n", meta.alloc, meta.size);
	}
	
	for(i; i < (page+1) * page_size; i += 0){
		
		memcpy(&meta, my_memory+i, sizeof(meta));
		printf("alloc: %d size: %d\n", meta.alloc, meta.size);
		if((int)meta.size < 0){
			printf("ERROR: negative size:\n");
			exit(1);
		}
		if(meta.alloc > 1 || meta.alloc < 0){
			printf("ERROR: alloc is wrong value.\n");
		}
		
		if(meta.alloc == 0 && meta.size == 0){
			printf("ERROR: this should never happen, we've probably clobbered data.\n");
			exit(1);
		}
		i += meta.size + sizeof(meta);
	}
	printf("\n");
}

//This function must be called before myallocate and free can perform properly.
//This function initializes my_memory by allocating it via memalign
//This function initializes the page table within my_memory and has table_ptr point to it. 
void initmem(){

	sa_mem.sa_flags = SA_SIGINFO;
	sigemptyset(&sa_mem.sa_mask);
	sa_mem.sa_sigaction = (void*)&handler;

	if (sigaction(SIGSEGV, &sa_mem, NULL) == -1){
		printf("Fatal error setting up signal handler\n");
		exit(EXIT_FAILURE);    //explode!
	}
	
	//main_init
	main_init = 0;
	
	int test = posix_memalign((void**)&my_memory, page_size, mem_size);
	printf("my_memory: %x\n", my_memory);
	
	//conserver first table_size + 1 pages for our metadata.
	table_row table[mem_size / page_size];
	os_first_free = sizeof(table) + 1 * page_size;
	os_start = os_first_free;
	
	user_first_free = os_first_free + (100 * page_size); //os gets 100 pages to do its work in. 
	user_start = user_first_free;
	
	printf("user start: %d user first: %d\n", user_start, user_first_free);
	
	//create virtual my_memory page table.
	int i = 0;
	for(i = 0; i < mem_size / page_size; i++){
		table[i].vaddr = 1;
		table[i].alloc = 0;
	}
	memcpy(my_memory, &table, sizeof(table));
	table_ptr = (table_row*)my_memory;
}


//This function allocates the first segment within a block. 
//It creates metadata for the first segment then creates another metadata for the remainder of the region. 
void* seg_alloc_first(size_t x, int req, int page, int vaddr){
	
	printf("seg alloc first.\n");
	void* mem;
	table_row* table = table_ptr;
	table[page].alloc = 1;
		
	//create meta data for this block.
	segdata meta;
	meta.alloc = 1; meta.size = x;
	memcpy((my_memory + (page * page_size)), &meta, sizeof(meta));
	mem = my_memory+(page * page_size)+sizeof(meta);
	
	//create metadata for the next block, there should only be 2 total blocks.
	segdata nextmeta; memset(&nextmeta, 0, sizeof(nextmeta));
	if(req == THREADREQ){  nextmeta.size = mem_size - (4* page_size) - user_start - x - sizeof(meta) - sizeof(nextmeta); }
	if(req == LIBRARYREQ){ nextmeta.size = user_start - os_start - x - sizeof(meta) - sizeof(nextmeta); }
	int nextmetapos = (page*page_size)+sizeof(meta) + meta.size;
	memcpy((my_memory + nextmetapos), &nextmeta, sizeof(nextmeta));
	
	//change the table to reflect the currently running thread has claimed this page.
	if(req == LIBRARYREQ){ 
		table[page].thread = (pthread_t)currently_running_thread; 
		table[page].vaddr = vaddr; 
	}
	else{
		if(currently_running_thread != NULL){ table[page].thread = (pthread_t)currently_running_thread; }
		else{ table[page].thread = (pthread_t)1; }
		table[page].vaddr = vaddr;
	}
	
	printpage(page, req);
	return mem;
}

//This function allocates a segment within a page. It starts from the beginning of the page and then finds the first
//metadata where alloc == 0 and size >= x. It will then change the metadata to reflect the new data's size and set alloc to 1.
//If necessary, it will make a new metadata block after it allocates the my_memory. 
void* seg_alloc(size_t x, int req, int page, int vaddr){
		
	void* mem;						//pointer to my_memory we eventually return.
	table_row* table = table_ptr;	//pointer to our table. 
	int next_page = page + 1;
	
	//printf("try page: %d\n", page);
	
	pthread_t target; 
	if(req == LIBRARYREQ) target = NULL;
	else target = table[page].thread;
	
	//start at the page, get first metadata;
	segdata meta;
	memcpy(&meta, (my_memory+(page*page_size)), sizeof(meta));
	//printf("meta: %d size: %d alloc: %d\n", page*page_size, meta.size, meta.alloc);
	
	unsigned int nextmetapos = page*page_size;   //this holds the position of the next metadata block in my_memory.
	while(meta.alloc == 1 || x > meta.size){ //find first unallocated metadata.
		
		//printf("alloc: %d size: %d\n", meta.alloc, meta.size);
		nextmetapos += sizeof(meta) + meta.size;
		while(nextmetapos >= (next_page) * page_size){
			//check if the next page is owned by us. If not, call page_swap. 
			if(table[next_page].thread == target && table[next_page].alloc == 1){//we own this page
				vaddr++;
				page++;
				next_page++;
				printf("next meta: %d\n", nextmetapos);
			}
			else if(table[next_page].thread != target && table[next_page].alloc == 1){//we do not own this page.
				
				if(req == 0 && table[next_page].thread != NULL && table[next_page].alloc == 1){ //our scheduler is out of my_memory.
					return NULL;
				}
				
				vaddr++;
				int swap_in = pageswap(next_page, req, vaddr); //swap out next page.
				if(swap_in == 0){ //no page was swapped in.
					//we have an error don't we? the next meta was supposed to take us to the next page. So we failed.
					printf("ERROR: metadata specified data across page %d into page %d, but no page %d was found.\n", page, next_page, next_page);
					exit(1);
				}
				else if(swap_in == 1){ //page was swapped in. 
					page++;
					next_page++;
				}
			}
			else{
				printf("we shouldn't be here.\n");
				exit(1);
			}
		}
		memcpy(&meta, (my_memory+nextmetapos), sizeof(meta));
		printf("meta: %d size: %d alloc: %d\n", nextmetapos, meta.size, meta.alloc);
		if(meta.alloc != 1 && meta.alloc != 0){
			printf("what?\n");
			exit(1);
		}
		if((int)meta.size < 0){
			printf("what?\n");
			exit(1);
		}
	}
	
	//we're at metadata, we need to fill it in now.
	size_t oldsize = meta.size; 	//old size is used to later to check to see if we need to make a new metadata or not. 
	meta.alloc = 1; meta.size = x; 
	
	//If x is not as big as oldsize, but x + sizeof(metadata) is bigger, it means there is not enough room to create a new metadata block.
	if(x + sizeof(meta) > oldsize){ //We'll cheat and pretend x is oldsize to get around this.
		meta.size = oldsize;
		x = oldsize;
	}
	
	//before we memcpy, we should check to see if the pages we need are in my_memory. 	
	while(nextmetapos+sizeof(meta)+meta.size > next_page * page_size){
		
		vaddr++;
		if(table[next_page].thread != target && table[next_page].alloc == 1){//we do not own this page.
			
			if(req == 0 && table[next_page].thread != NULL && table[next_page].alloc == 1){ //our scheduler is out of my_memory.
				return NULL;
			}
			
			int swap_in = pageswap(next_page, req, vaddr); //swap out next page.
			if(swap_in == 0){ //no page was swapped in.
				if(req == THREADREQ){ table[next_page].thread = target; }
				else{ table[next_page].thread = NULL; }
				table[next_page].alloc = 1;
				table[next_page].vaddr = vaddr;
				page++; next_page++;
			}
			else if(swap_in == 1){ //page was swapped in. 
				page++;
				next_page++;
			}
		}
		else if(table[next_page].thread == target && table[next_page].alloc == 1){//we own this page.
			page++; next_page++;
		}
		else if(table[next_page].thread == NULL && table[next_page].alloc == 0){//this page was not allocated, let's claim it.
			if(req == THREADREQ){ table[next_page].thread = target; }
			else{ table[next_page].thread = NULL; }
			table[next_page].alloc = 1; table[next_page].vaddr = vaddr;
			page++; next_page++;
		}
	}
	memcpy((my_memory+nextmetapos), &meta, sizeof(meta)); //copy the metadata into my_memory 
	if(nextmetapos < 0 || nextmetapos > (my_memory + mem_size)){
		printf("what the hell?\n");
		exit(1);
	}
	mem = (my_memory +nextmetapos +sizeof(meta));		   //assign our pointer to the position after the metadat.
	
	if(x != oldsize){ //we only make a new block if our size is not the same as the old size.
	
		//create metadata for the next block after this one, then write it into my_memory.
		
		//check whether nextmeta is made on the same page.
		segdata nextmeta; memset(&nextmeta, 0, sizeof(nextmeta));
		nextmeta.size = oldsize - (meta.size + sizeof(meta));
		nextmetapos = nextmetapos + sizeof(meta) + meta.size;

		//before we copy over the nextmetadata, we need to make sure that its page is loaded in. 
		if(nextmetapos+sizeof(meta) > next_page * page_size){
			printf("next metadata broke page boundary: %d\n", nextmetapos+sizeof(meta) - (next_page * page_size));
			vaddr++;

			if(table[next_page].thread != target && table[next_page].alloc == 1){ //someone else owns that page. 
				
				if(req == 0 && table[next_page].thread != NULL && table[next_page].alloc == 1){ //our scheduler is out of my_memory.
					return NULL;
				}
				
				int swap_in = pageswap(next_page, req, vaddr); //swap out next page.
				if(swap_in == 0){ //no page was swapped in.
					if(req == THREADREQ){ table[next_page].thread = target; }
					else{ table[next_page].thread = NULL; }
					table[next_page].alloc = 1;
					table[next_page].vaddr = vaddr;
					page++; next_page++;
				}
				else if(swap_in == 1){ //page was swapped in. 
					page++;
					next_page++;
				}
			}
			else if(table[next_page].thread == target && table[next_page].alloc == 1){ //we own this page.
				page++; next_page++;
			}
			else if(table[next_page].thread == NULL && table[next_page].alloc == 0){//this page was not allocated, lets claim it.
				if(req == THREADREQ){ table[next_page].thread = target; }
				else{ table[next_page].thread = NULL; }
				table[next_page].alloc = 1; table[next_page].vaddr = vaddr;
				page++; next_page++;
			}
				
		}
			
		memcpy((my_memory+nextmetapos), &nextmeta, sizeof(nextmeta));
		printf("new metadata: alloc: %d size: %d\n", nextmeta.alloc, nextmeta.size);
		
	
	}
	printpage(page, req);
	return mem;
}

//Performs a page swap. Returns 0 if there was no page to swap in (means its first alloc), 1 if another page was swapped in.
int pageswap(int swap_out, int req, int vaddr){
	
	pthread_t target = NULL;
	if(req == THREADREQ){
		if(currently_running_thread == NULL){
			target = (pthread_t)1;
		}
		else{
			target = currently_running_thread;
		}
	}
	printf("\n\ttarget: %x\n", target);
	
	table_row* table = table_ptr;
	pthread_t prev_owner = table[swap_out].thread; 	//figure out who this page belongs to currently.
	printf("\t%x owns page %d and we have to swap it out.\n", prev_owner, swap_out);
	unprotectthreadpages(prev_owner, 0);  			  	//unprotect their pages.
	unprotectthreadpages(target, 0);
	
	user_first_free = user_start;
	while(table[user_first_free/page_size].alloc != 0){ 	//advance user_first_free to the user_first_free page. 
		user_first_free += page_size;
	}
	
	printf("\tfirst free: %u page: %d\n", user_first_free, user_first_free/page_size);
	
	memcpy(my_memory + user_first_free, my_memory + (swap_out*page_size), page_size);  //swap their page to a free page.
	printf("\tmoved page %d to %d.\n\n", swap_out, user_first_free/page_size);
	
	//vaddress should be = to vaddress of where that page just was.
	table[user_first_free/page_size].thread = table[swap_out].thread; //go to the table, mark that free page as allocated, owned by the thread,
	table[user_first_free/page_size].alloc = 1;
	table[user_first_free/page_size].vaddr = table[swap_out].vaddr;
	printf("\tset table[%d]: thread: %x alloc: %d vaddr: %d\n", user_first_free/page_size, table[user_first_free/page_size].thread, table[user_first_free/page_size].alloc, table[user_first_free/page_size].vaddr);
	
	//we know page i faulted.
	int swap_in = user_start/page_size;
	for(swap_in; swap_in < mem_size/page_size; swap_in++){
		
		if(table[swap_in].thread == target && table[swap_in].alloc == 1 && table[swap_in].vaddr == vaddr){break;}
	}
	
	//we should be at our page in my_memory or we had no page.
	if(swap_in == mem_size/page_size){ //we didn't have a previous first page.
		
		printf("\tno page to swap in.\n\n");
		table[swap_out].alloc = 0;
		table[swap_out].thread = NULL;
		protectthreadpages(prev_owner, 0);
		return 0;
	}
	else{ //we found the page.
		
		printf("\tswap_in: %d\n", swap_in);
		memcpy(my_memory + (swap_out * page_size), my_memory + (swap_in * page_size), page_size); //memcpy that page over to the page that faulted. 
		printf("\tpasting page %d over %d\n", swap_in, swap_out);
		table[swap_out].thread = target;
		table[swap_out].alloc = 1;
		table[swap_out].vaddr = table[swap_in].vaddr;
		printf("\ttable[%d]: thread: %x alloc: %d vaddr: %d\n", swap_out, table[swap_out].thread, table[swap_out].alloc, table[swap_out].vaddr );
		
		table[swap_in].thread = NULL;
		table[swap_in].vaddr = 0;
		table[swap_in].alloc = 0;
		printf("\ttable[%d]: thread: %x alloc: %d vaddr: %d\n\n", swap_in, table[swap_in].thread, table[swap_in].alloc, table[swap_in].vaddr );
		
		protectthreadpages(prev_owner, 0);
		return 1;
	}	
}

//@TODO: change this to allocate into next page. If next page is not owned by us, swap it out.
void* myallocate(size_t x, char* file, int line, int req){

	//init metadata on first call.
	if(user_first_free == 0){
		initmem();
	}
	
	printf("req: %d size: %d\n", req, x);
	
	//get table. 
	table_row* table = table_ptr;
	void* mem = NULL; 
	int vaddr = 1;
	pthread_t target;
	if(req == 0){target = NULL;}
	else{
		target = currently_running_thread;
		if(target == NULL){target = (pthread_t)1;}
	}
	printf("target: %x\n", target);
	
	if(req == LIBRARYREQ){
		
		//try to allocate from os start
		//if the page 
		int i = os_start/page_size;
		int vaddr = 1;
		pthread_t target = NULL;
		while(mem == NULL && i < mem_size/page_size){
			if(table[i].alloc == 0){ //if page is free or has never been claimed, then seg_alloc_first.
				mem = seg_alloc_first(x, req, i, vaddr);
				break;
			}
			if(table[i].thread == target && table[i].alloc == 1){//we own this page.
				mem = seg_alloc(x, req, i, vaddr);
				if(mem == NULL){
					vaddr++;
					i++;
				}
				else{
					break;
				}
			} 
		}
		while(table[os_first_free/page_size].alloc != 0){ 	//advance s_first_free to the first free page. 
			os_first_free += page_size;
		}
	}
	else if(req == THREADREQ){
		//Before the scheduler runs, main runs. It won't have a pthread_t so we temporarily assign it the thread id of 1 in the page table.
		int rename = 0;
		if(currently_running_thread != NULL && main_init == 0){rename = 1;} //once we have a thread id, we can change it from 1 to its id.
		
		if(rename){ //rename any page claimed by thread '1' to whatever the address of currently_running_thread is
			int i = 0; 
			printf("renaming.\n");
			for(i = user_start/page_size; i < mem_size / page_size; i++){
				
				if(rename && table[i].thread == (pthread_t)1 && table[i].alloc == 1){ 
					table[i].thread = (pthread_t)currently_running_thread;
				}
			}
			main_init = 1;	//once main's pages have been renamed, we set main_init to 1 to represent that its been initialized.
		}
		
		int i = user_start/page_size; //Always start at page user_start/page_size.
		
		while(mem == NULL && i < mem_size/page_size){ //while we have not allocated, continue to search and evict pages.
			if(table[i].alloc == 0){ //if page is free or has never been claimed, then seg_alloc_first.
				mem = seg_alloc_first(x, req, i, vaddr);
				break;
			}
			
			int swapped_in = -1;
			if(table[i].thread != target && table[i].alloc == 1){ //if someone else owns it, we page swap.
				swapped_in = pageswap(i, req, vaddr);
			}
			if(table[i].thread == target && table[i].alloc == 1){//we own this page.
				
				if(swapped_in == 0){ //we swapped a page out, and nothing in. Therefore this is our first alloc on that page.
					mem = seg_alloc_first(x, req, i, vaddr);
					break;
				}
				else if(swapped_in == 1){//we swapped a page out and another in. We continue an alloc on that page.
					mem = seg_alloc(x, req, i, vaddr);
				}
				else if(swapped_in == -1){//we did not need to swap, so continue allocation on that page.
					mem = seg_alloc(x, req, i, vaddr);
				}
				
				if(mem == NULL){
					vaddr++;
					i++;
				}
				else{
					break;
				}
			} 
		}		
		while(table[user_first_free/page_size].alloc != 0){ 	//advance user_first_free to the first free page. 
			user_first_free += page_size;
		}
	}
	else{
		printf("ERROR: my_memory allocation request is neither from a thread nor from the scheduler.\n");
		exit(1);
	}
	
	printmem();
	return mem;
}

//@TODO: When we start to have contiguous virtual pages, we need to change free so that it can check the next page
//	     in my_memory if and only if our thread owns that page. 
//@TODO: We need to change this so that it starts at the right metadata location. We need to start at our thread's FIRST page. 
void mydeallocate(void* x, char* file, int line, int req){
	
	table_row* table = table_ptr;
	void* metapos = (x - sizeof(segdata));

	printf("Address of memory: %x Address of block to free: %x  difference %x\n",my_memory,metapos,(char*)metapos-my_memory); 
	pthread_t target;
	if(req == 0){target = NULL;}
	else{
		target = currently_running_thread;
		if(target == NULL){target = (pthread_t)1;}
	}
	printf("target: %x\n", target);
	unsigned int page;								//Find what page the block is on
	if(req==LIBRARYREQ){
		page = os_start/page_size;
	}else if(req==THREADREQ){
		page = user_start/page_size;
	}
	printf("Start of page: %x user start: %x os start: %x\n",page*page_size+my_memory,user_start+my_memory,os_start+my_memory);

	printpage(page,req);
	while(((page+1)*page_size)+my_memory < metapos){
		printf("PAGE: %d Page Address: %x\n",page,(page*page_size)+my_memory);
		page = page +1;
	}
	printpage(page,req);								// found page

	unsigned int respage = page;

	if(table[page].thread!= target){						//check owner
		pageswap(page,req,table[page].vaddr);
	}


	segdata meta;
	memcpy(&meta, metapos, sizeof(segdata));
	meta.alloc = 0;
	memcpy(metapos, &meta, sizeof(segdata));

//	memcpy(&meta, metapos, sizeof(segdata));
		
	printf("\n\n\n\n DeAllocate meta.alloc: %d meta.size: %d\n\n\n\n",(*(segdata*)metapos).alloc,meta.size);

//	unsigned int addroff;
//   	addroff = (unsigned int) ((char*)x - (char*)my_memory);
//	unsigned int page = (addroff+sizeof(segdata))/page_size;
//	printf("os start: \n");
//	unsigned int page = os_start/page_size;
//	printpage(page,LIBRARYREQ);
//	printf("user start\n");
//	page = user_start/page_size;
//	printpage(page,THREADREQ);

	if(metapos + sizeof(segdata)+meta.size >= ((page+1)*page_size)+my_memory){
		if(target != table[page+1].thread){
			printf("Swap Vaddr: %d\n",table[page+1].vaddr);
			pageswap(page+1,req,table[page+1].vaddr);
		}
	}
	

	segdata next;
	memcpy(&next, (metapos+sizeof(segdata)+meta.size),sizeof(segdata));
	printf("next.size: %d next.alloc %d\n",next.size,next.alloc);

	// Merges unallocated memory after the free section
	if(next.alloc == 0){
		printf("meta.size: %d next.size: %d metapos: %x\n",meta.size,next.size,metapos);
		meta.size = meta.size + next.size;
		printf("meta.size: %d next.size: %d metapos: %x\n",meta.size,next.size,metapos);
		memcpy(metapos, &meta, sizeof(segdata));
		//Maybe set nexts size to zero?
		memcpy(&next, (metapos + sizeof(segdata)+meta.size),sizeof(segdata));		
		
	}

//	segdata prev;
//	memcopy(&prev, (metapos+sizeof(segdata)+meta.size),sizeof(segdata));

	int vaddr = 1;
	int prevVaddr = 1;
		

	segdata prev;
	unsigned int prevpage;


	if(req = LIBRARYREQ){
		printf("Libraryreq\n");
		void* prevmeta = os_start+my_memory;
		void* currmeta = os_start+my_memory;
		page = os_start/page_size;
		prevpage = page;

		memcpy(&prev,currmeta,sizeof(segdata));
		memcpy(&meta,currmeta+sizeof(segdata)+prev.size,sizeof(segdata));
		currmeta += sizeof(segdata)+prev.size;

		while(currmeta < user_start+my_memory){
			if(prev.alloc == 0 && meta.alloc == 0){
				prev.size = prev.size + meta.size;
				memcpy(prevmeta, &prev, sizeof(segdata));
				memcpy(&meta, currmeta+sizeof(segdata)+meta.size,sizeof(segdata));

				currmeta += sizeof(segdata)+meta.size;
	
			}else{
				memcpy(&prev, &meta, sizeof(segdata));
				prevmeta = currmeta;

				memcpy(&meta, currmeta+sizeof(segdata)+meta.size, sizeof(segdata));
				currmeta += sizeof(segdata)+meta.size;
			}
		
		}
		


	}else if(req = THREADREQ){
		printf("ThreadReq\n");
		void* prevmeta = user_start+my_memory;
		void* currmeta = user_start+my_memory;
		page = user_start/page_size;
		prevpage = page;
		printf("if statment page: %d table: %x\n",page,table[page]);
		printpage(page,req);
		if(table[page].thread!=target){
			printf("pageswap\n");
			pageswap(page,req,prevVaddr);
			printf("after pageswap\n");
		}

		memcpy(&prev,currmeta,sizeof(segdata));
		printf("\n\nPrev at user_start- alloc: %d size: %d\n\n",prev.alloc,prev.size);
		if(currmeta+sizeof(segdata)+prev.size > ((page+1)*page_size)+my_memory){
			vaddr++;							//meta is on next page after prev
			page++;
			if(target != table[page].thread){
				pageswap(page,req,vaddr);
			}
			if(table[page].alloc == 0){
				printpage(page,req);
				printf("FREE SUCCESS ON PAGE ALLOC =0");
				return;
			}
		}

                memcpy(&meta,currmeta+sizeof(segdata)+prev.size,sizeof(segdata));
		printf("\nMeta.alloc: %d meta.size: %d\n",meta.alloc,meta.size);
                currmeta += sizeof(segdata)+prev.size;
		printf("currmeta: %x prevmeta: %x meta.alloc: %d meta.size: %d prev.alloc: %d prev.size: %d\n",currmeta,prevmeta,meta.alloc,meta.size,prev.alloc,prev.size);
		while(currmeta < my_memory + mem_size){
			printf("while loop \n");
			if(prev.alloc == 0 && meta.alloc ==0){
				printf("Prev and Meta alloc = 0    prev.size: %d  meta.size: %d\n",prev.size,meta.size);
				prev.size = prev.size + meta.size;
				printf("prev.size: %d\n",prev.size);
                                memcpy(prevmeta, &prev, sizeof(segdata));

				if(currmeta+sizeof(segdata)+meta.size > ((page+1)*page_size)+my_memory){
					printf("meta is on next page: curr page: %d curr vaddr: %d\n\n",page,vaddr);
					vaddr++;                                                        //meta is on next page after prev
					page++;
					printf("page after: %d vaddr after: %d\n",page,vaddr);
					if(target != table[page].thread){
                                		pageswap(page,req,vaddr);
                        		}
					if(table[page].alloc == 0){
                         			printpage(page,req);
                       				printf("FREE SUCCESS ON PAGE ALLOC =0");
                                		break;
                        		}
                		}

				size_t temp = meta.size;
                                memcpy(&meta, currmeta+sizeof(segdata)+temp,sizeof(segdata));

                                currmeta += sizeof(segdata)+temp;

                        }else{

				printf("prev.alloc: %d prev.size: %d meta.alloc: %d meta.size: %d\n",prev.alloc,prev.size,meta.alloc,meta.size);	
                                memcpy(&prev, &meta, sizeof(segdata));
				printf("prev.alloc: %d prev.size: %d meta.alloc: %d meta.size: %d\n",prev.alloc,prev.size,meta.alloc,meta.size);
                                prevmeta = currmeta;
				prevVaddr=vaddr;
				prevpage = page;


				if(currmeta+sizeof(segdata)+meta.size > ((page+1)*page_size)+my_memory){
					printf("\n\n\nHERE??????\n\n\n");
                 			vaddr++;                                                        //meta is on next page after prev
                        		page++;
				
					if(target != table[page].thread){
                	                	pageswap(page,req,vaddr);
            			        }
					if(table[page].alloc == 0){
                                                printpage(page,req);
                                                printf("FREE SUCCESS ON PAGE ALLOC =0");
                                                break;
                                        }

		                }

				size_t temp = meta.size;
                                memcpy(&meta, currmeta+sizeof(segdata)+temp, sizeof(segdata));
                                currmeta += sizeof(segdata)+temp;
                        }
			
		}


		

	}

	printpage(respage,req);



		
	printf("FREE SUCCESS");
	
	return;
}

static void handler(int sig, siginfo_t *si, void *unused){
	
	
    long addr = (long)(si->si_addr - (void*)my_memory);
	int page = -1;
	if(addr < mem_size && addr >= my_memory){
		page = (int) (addr / page_size); 
		table_row* table = table_ptr;
		pthread_t thread = table[page].thread;
		printf("Got SIGSEGV at my_memory[%u] in page: %d owned by: %x\n",addr, page, thread);	

		int req = -1;
		if(currently_running_thread == NULL){req = 0;}
		else{req = 1;}		
		unsigned int vaddr = page-(user_start/page_size) + 1;
		printf("req: %d vaddr: %u\n", req, vaddr);
		if(req == THREADREQ){
			printf("swapping for: %x\n", currently_running_thread);
		}
		else{
			printf("swapping for: 0\n");
		}
		int swap_in = pageswap(page, req, vaddr);
		printf("\n");
		printmem();
		return;
	}
	else{
		printf("Got SIGSEGV at %x out of bounds. would be page %d\n",si->si_addr, (int)(addr/page_size));
	    exit(1);
	}
}

