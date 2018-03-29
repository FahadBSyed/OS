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
			if(debug){
				printf("page: %d address: %x - %x is protected.\n", i, (my_memory+(i*page_size)), (my_memory+((i+1)*page_size) - 1));
			}
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
			if(debug){
				printf("page: %d address: %x - %x is unprotected.\n", i, (my_memory+(i*page_size)), (my_memory+((i+1)*page_size) - 1));
			}
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
	
	user_first_free = os_first_free + (1000 * page_size); //os gets 1000 pages to do its work in. 
	user_start = user_first_free;
	
	printf("user start: %d user first: %d\n", user_start, user_first_free);
	
	//create virtual my_memory page table.
	int i = 0;
	for(i = 0; i < mem_size / page_size; i++){
		table[i].vaddr = 1;
		table[i].alloc = 0;
	}

//	for(i=0; i<mem_size/page_size;i++){
//		printf("table[%d]  alloc: %d vaddr: %d\n",i,table[i].alloc,table[i].vaddr);
//	}
//	exit(1);
	


	memcpy(my_memory, &table, sizeof(table));
	table_ptr = (table_row*)my_memory;

	//create swapfile
	swap = fopen("swapFile.bin","wb");
	ftruncate(fileno(swap), swap_size);

	//check to see if file created is right size
	if(swap == NULL){
		printf("FATAL ERROR Setting up SwapFile\n");
		exit(EXIT_FAILURE);
	}else{
		fseek(swap,0L,SEEK_END);
		long size = ftell(swap);
		printf("The SwapFile size is : %1dB\n",size);
		int err = fseek(swap,0L,SEEK_SET);
		if(err!=0){
			printf("ERROR pointer does not reset to beginning of file");
			exit(EXIT_FAILURE);
		}
		

		//sets up page table for swap file
		table_row swapTable[swap_size/page_size];
	
		int j;
		for(j=0;j<swap_size/page_size;j++){
			swapTable[j].vaddr = 1;
			swapTable[j].alloc = 0;
//			printf("swapTable[%d] alloc: %d vaddr: %d\n",j,swapTable[j].alloc,swapTable[j].vaddr);
		}
		fwrite(&swapTable,sizeof(table_row)*swap_size/page_size,1,swap);
		fclose(swap);

/*													Debug code to see if init write worked
		swap = fopen("swapFile.bin","r");
		table_row t[swap_size/page_size];
		err = fseek(swap,0L,SEEK_SET);
                if(err!=0){
                      	printf("ERROR pointer does not reset to beginning of file");
                       	exit(EXIT_FAILURE);
                }
		fread(&t,sizeof(table_row)*swap_size/page_size,1,swap);
		for(j=0;j<swap_size/page_size;j++){
			printf("t[%d] alloc: %d vaddr: %d\n",j,t[j].alloc,t[j].vaddr);
		}
*/
		
//		printf("swaptable size: %d vs other: %d",sizeof(swapTable),sizeof(table_row)*(swap_size/page_size));

		swap_start= sizeof(swapTable) + 1 * page_size;
	}


	

}


//This function allocates the first segment within a block. 
//It creates metadata for the first segment then creates another metadata for the remainder of the region. 
void* seg_alloc_first(size_t x, int req, int page, int vaddr){
	
	printf("seg alloc first.\n");
	void* mem;
	table_row* table = table_ptr;
		
	//create meta data for this block.
	segdata meta;
	meta.alloc = 1; meta.size = x;
	
	//figure out the total number of pages we overflowed. 
	unsigned int total_bytes = meta.size + sizeof(meta) + sizeof(meta);
	unsigned int total_pages = total_bytes / page_size;
	if(total_bytes % page_size != 0){total_pages++; }
	
	int curr_vaddr = vaddr; //this will hold the vaddr of the oveflowed pages as we claim them. 
	printf("total bytes: %d total pages: %d\n", total_bytes, total_pages);
	
	int curr_page = page;
	while(curr_page < page+total_pages){
		
		
		if(table[curr_page].thread != currently_running_thread && table[curr_page].alloc == 1){
			
			int swap_in = pageswap(curr_page, req, curr_vaddr);			
			if(swap_in == 1){ //this shouldn't ever happen!
				printf("ERROR: if this is our first allocation for this thread, why are we swapping in a page?\n");
				exit(0);
			}
		}
		if(table[curr_page].alloc == 0){
			
			printf("claim page %d\n", curr_page);
			//change the table to reflect the currently running thread has claimed this page.
			if(req == LIBRARYREQ){ 
				table[curr_page].thread = (pthread_t)currently_running_thread; 
				
			}
			else{
				if(currently_running_thread != NULL){ table[curr_page].thread = (pthread_t)currently_running_thread; }
				else{ table[curr_page].thread = (pthread_t)1; }
			}
			table[curr_page].vaddr = curr_vaddr; 
			table[curr_page].alloc = 1;
			
		}
		curr_page++;
		curr_vaddr++;
	}
	
	memcpy((my_memory + (page * page_size)), &meta, sizeof(meta));
	mem = my_memory+(page * page_size)+sizeof(meta);
	
	//create metadata for the next block, there should only be 2 total blocks.
	segdata nextmeta; memset(&nextmeta, 0, sizeof(nextmeta));
	if(req == THREADREQ){  nextmeta.size = mem_size - (4* page_size) - user_start - x - sizeof(meta) - sizeof(nextmeta); }
	if(req == LIBRARYREQ){ nextmeta.size = user_start - os_start - x - sizeof(meta) - sizeof(nextmeta); }
	int nextmetapos = (page*page_size)+sizeof(meta) + meta.size;
	memcpy((my_memory + nextmetapos), &nextmeta, sizeof(nextmeta));
	
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
	char buff[page_size];
	
	printf("\n\n\n\n mem of buff start: %x end: %x mem of os start: %x end: %x \n\n\n\n",&buff,(&(buff))+page_size,my_memory+os_start,my_memory+user_start);

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
	while(table[user_first_free/page_size].alloc != 0 && user_first_free/page_size < 2044){ 	//advance user_first_free to the user_first_free page. 
		user_first_free += page_size;
	}

	int swapfile_out =-1;
	if(user_first_free/page_size >= 2044){					//no free allocs so have to swap out a file.
		swap = fopen("swapFile.bin","rb+");
		printf("\n\nuser_first_free: %x my_memory+mem_size: %x PAGE SWAP IS WRITING TO SWAP FILE\n\n",user_first_free,my_memory+mem_size);
                if(swap == NULL){
                	printf("ERROR: could not open File line 490\n");
                        exit(EXIT_FAILURE);
                }

	
                table_row swapTable[swap_size/page_size];
                fread(&swapTable,sizeof(swapTable),1,swap); 
		
		
		for(swapfile_out=swap_start/page_size;swapfile_out<swap_size/page_size;swapfile_out++){
			if(swapTable[swapfile_out].alloc == 0){
				fseek(swap,swap_start+(swapfile_out*page_size),SEEK_SET);
				fwrite(my_memory+(swap_out*page_size),page_size,1,swap);
				swapTable[swapfile_out].alloc = 1;
				swapTable[swapfile_out].thread = table[swap_out].thread;
				swapTable[swapfile_out].vaddr = table[swap_out].vaddr;
				
				fseek(swap,0L,SEEK_SET);
				fwrite(&swapTable,sizeof(swapTable),1,swap);
				break;
			}
		}
		if(swapfile_out>= swap_size/page_size){
			printf("NO MEMORY LEFT IN SWAPFILE\n");
			fclose(swap);
			return -2;
		}	
		fclose(swap);
	}else{
	

		printf("\tfirst free: %u page: %d\n", user_first_free, user_first_free/page_size);
		//printf("\tmy_memory + mem size - (page_size * 4): %u\n", ((unsigned int)(my_memory + mem_size)) - (4096*4));
		memcpy(my_memory + user_first_free, my_memory + (swap_out*page_size), page_size);  //swap their page to a free page.
		printf("\tmoved page %d to %d.\n\n", swap_out, user_first_free/page_size);
	
	//vaddress should be = to vaddress of where that page just was.
		table[user_first_free/page_size].thread = table[swap_out].thread; //go to the table, mark that free page as allocated, owned by the thread,
		table[user_first_free/page_size].alloc = 1;
		table[user_first_free/page_size].vaddr = table[swap_out].vaddr;
		printf("\tset table[%d]: thread: %x alloc: %d vaddr: %d\n", user_first_free/page_size, table[user_first_free/page_size].thread, table[user_first_free/page_size].alloc, table[user_first_free/page_size].vaddr);
	}



	//we know page i faulted.
	int swap_in = user_start/page_size;
	for(swap_in; swap_in < mem_size/page_size; swap_in++){
		
		if(table[swap_in].thread == target && table[swap_in].alloc == 1 && table[swap_in].vaddr == vaddr){break;}
	}
	
	//we should be at our page in my_memory or we had no page.
	if(swap_in == mem_size/page_size){ //we didn't have a previous first page or resides in swapfile.
		printf("\n\nPAGE to swap in resides in SWAPFILE\n\n");
		swap = fopen("swapFile.bin","rb+");
                if(swap == NULL){
			int errnum = errno;
                	printf("ERROR: could not open File line 542\n");
			perror("Error by perror");
                	exit(EXIT_FAILURE);
                }

                table_row swapTable[swap_size/page_size];
                fread(&swapTable,sizeof(swapTable),1,swap);      	
		
		for(swap_in=swap_start/page_size;swap_in<swap_size/page_size;swap_in++){
			if(swapTable[swap_in].thread == target && swapTable[swap_in].alloc == 1 && swapTable[swap_in].vaddr == vaddr){break;}	
		}
		if(swap_in >= swap_size/page_size){
			printf("\tno page to swap in.\n\n");
			table[swap_out].alloc = 0;
			table[swap_out].thread = NULL;
			protectthreadpages(prev_owner, 0);
			return 0;
		}else{												//found in swapFile
			fseek(swap,swap_start+(swap_in*page_size),SEEK_SET);
			fread(my_memory+(swap_out*page_size),page_size,1,swap);
			table[swap_out].thread = target;
			table[swap_out].alloc = 1;
			table[swap_out].vaddr = swapTable[swap_in].vaddr;

			swapTable[swap_in].thread = NULL;
			swapTable[swap_in].alloc = 0;
			swapTable[swap_in].vaddr = 0;
					
			protectthreadpages(prev_owner,0);
			fclose(swap);

                	swap = fopen("swapFile.bin","rb+");
                	if(swap == NULL){
                        	int errnum = errno;
                        	printf("ERROR: could not open File line 542\n");
                        	perror("Error by perror");
                        	exit(EXIT_FAILURE);
                	}
			fseek(swap,0L,SEEK_SET);
			fwrite(&swapTable,sizeof(swapTable),1,swap);
			fclose(swap);
			return 1;
		}
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
		
		//page table is full, evict first page(pages) to make room for new
		//

		
		if (mem == NULL){
			printf("\n\n\nMUST USE SWAPFILE\n\n\n");
			swap = fopen("swapFile.bin","rb+");
	                if(swap == NULL){
        	                printf("ERROR: could not open File line 653\n");
        	                exit(EXIT_FAILURE);
        	        }



			if(pageToEvic >= mem_size/page_size){
				pageToEvic = 0;
			}
			table_row swapTable[swap_size/page_size];
			fread(&swapTable,sizeof(swapTable),1,swap);			//might have to fix, loop through page table
			fclose(swap);
			int j;

			for(j =swap_start/page_size;j< swap_size/page_size;j++){	//j is page in swap file to be put in pagetable so malloc can use
				swap = fopen("swapFile.bin","rb+");
                        	if(swap == NULL){
                                	printf("ERROR: could not open File line 653\n");
                                	exit(EXIT_FAILURE);
                        	}
				if(swapTable[j].alloc == 0){
					table[pageToEvic].alloc = 0;
					swapTable[j].alloc = 1;
                                        swapTable[j].vaddr = table[pageToEvic].vaddr;
                                        swapTable[j].thread = table[pageToEvic].thread;
					int err = fseek(swap,swap_start+(j*page_size),SEEK_SET);
					if(err!=0){
						printf("\nCould not seek to right page in swapfile\n");
					}
					fwrite(os_start+(pageToEvic*page_size),page_size,1,swap);
					pageToEvic++;
					err = fseek(swap,0L,SEEK_SET);
					
                                        if(err!=0){
                                                printf("\nCould not seek to right page in swapfile\n");
                                        }
					fwrite(&swapTable,sizeof(swapTable),1,swap);
					fclose(swap);

					mem = seg_alloc_first(x,req,pageToEvic,vaddr);	
					break;

				}else if(swapTable[j].thread == target){
					
					swapTable[j].vaddr = table[pageToEvic].vaddr;
					
					
					char buff[page_size];
					memcpy(&buff,os_start+(pageToEvic*page_size),page_size);
					fseek(swap,swap_start+(j*page_size),SEEK_SET);
					fread(os_start+(pageToEvic*page_size),page_size,1,swap);
					fseek(swap,swap_start+(j*page_size),SEEK_SET);
					fwrite(&buff,page_size,1,swap);
					
					mem = seg_alloc(x, req, pageToEvic,vaddr);
					if(mem==NULL){
						vaddr++;
					}else{
						pageToEvic++;
						break;
					}
					fclose(swap);
				}else{
					printf("SHOULD NOT GET HERE OS SWAPFILE\n");	
					fclose(swap);
				}	
			}
			

		}
//FIX THESE WHILE LOOPS TRAVIS TO NOT GO OUT OF BOUNDS
		while(table[os_first_free/page_size].alloc != 0){ 	//advance s_first_free to the first free page. 
			os_first_free += page_size;
		}
	}
	else if(req == THREADREQ){
		//Before the scheduler runs, main runs. It won't have a pthread_t so we temporarily assign it the thread id of 1 in the page table.
		
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
	
		if (mem == NULL){

			swap = fopen("swapFile.bin","rb+");
	                if(swap == NULL){
        	                printf("ERROR: could not open File line 755\n");
        	                exit(EXIT_FAILURE);
        	        }



			if(pageToEvic >= mem_size/page_size){
				pageToEvic = 0;
			}
			table_row swapTable[swap_size/page_size];
			fread(&swapTable,sizeof(swapTable),1,swap);			//might have to fix, loop through page table
			int j;
			for(j =swap_start/page_size;j< swap_size/page_size;j++){	//j is page in swap file to be put in pagetable so malloc can use
				if(swapTable[j].alloc == 0){
					table[pageToEvic].alloc =0;
					
					swapTable[j].alloc = 1;
					swapTable[j].vaddr = table[pageToEvic].vaddr;				
					swapTable[j].thread = table[pageToEvic].thread;
 					rewind(swap);
					fseek(swap,swap_start+(j*page_size),SEEK_SET);
					//ADD CHECK FOR END OF SWAP
					fwrite(os_start+(pageToEvic*page_size),page_size,1,swap);
					printf("page swap table\n");
					mem = seg_alloc_first(x, req, pageToEvic, vaddr);
					if(mem != NULL){
						pageToEvic++;
						break;
					}
				}else if(swapTable[j].thread == target){
					
					swapTable[j].vaddr = table[pageToEvic].vaddr;
					rewind(swap);
					
					char buff[page_size];
					memcpy(&buff,os_start+(pageToEvic*page_size),page_size);
					fseek(swap,swap_start+(j*page_size),SEEK_SET);
					fread(os_start+(pageToEvic*page_size),page_size,1,swap);
					fseek(swap,swap_start+(j*page_size),SEEK_SET);
					fwrite(&buff,page_size,1,swap);
					
					mem = seg_alloc(x, req, pageToEvic,vaddr);
					if(mem==NULL){
						vaddr++;
					}else{
						pageToEvic++;
						break;
					}
				}else{
					printf("SHOULD NOT GET HERE OS SWAPFILE\n");	
				}	
			}
			fclose(swap);

		}


//FIX THESE WHILE LOOPS TRAVIS TO NOT GO OUT OF BOUNDS
		
		while(table[user_first_free/page_size].alloc != 0){ 	//advance user_first_free to the first free page. 
			user_first_free += page_size;
		}
	}
	else{
		printf("ERROR: my_memory allocation request is neither from a thread nor from the scheduler.\n");
		exit(1);
	}
	
	//printmem();
	return mem;
}

/*
*			Shalloc Function
*/

void* shared_seg_alloc_first(size_t x){
	
	printf("shared seg alloc first.\n");
	void* mem;
	table_row* table = table_ptr;
		
	//create meta data for this block.
	segdata meta;
	meta.alloc = 1; meta.size = x;
	
	//figure out the total number of pages we overflowed. 
	unsigned int total_bytes = meta.size + sizeof(meta) + sizeof(meta);
	unsigned int total_pages = total_bytes / page_size;
	if(total_bytes % page_size != 0){total_pages++; }
	
	printf("total bytes: %d total pages: %d\n", total_bytes, total_pages);
	
	int page = 2048-4;

	int curr_page = page;
	while(curr_page < page+total_pages){
		
		if(table[curr_page].alloc == 0){
			
			printf("claim page %d\n", curr_page); 
			table[curr_page].alloc = 1;
			
		}
		curr_page++;
	}
	
	memcpy((my_memory + (page * page_size)), &meta, sizeof(meta));
	mem = my_memory+(page * page_size)+sizeof(meta);
	
	//create metadata for the next block, there should only be 2 total blocks.
	segdata nextmeta; memset(&nextmeta, 0, sizeof(nextmeta));
	nextmeta.size = mem_size - (4* page_size) - shared_start - x - sizeof(meta) - sizeof(nextmeta); 
	int nextmetapos = (page*page_size)+sizeof(meta) + meta.size;
	memcpy((my_memory + nextmetapos), &nextmeta, sizeof(nextmeta));
	
	printpage(page, 0);
	return mem;
}

void* shared_seg_alloc(size_t x){
		
	void* mem;						//pointer to my_memory we eventually return.
	table_row* table = table_ptr;	//pointer to our table. 	
	int page = 2048 - 4;
	int next_page = page+1;
	//start at the page, get first metadata;
	segdata meta;
	memcpy(&meta, (my_memory+(page*page_size)), sizeof(meta));
	printf("meta: %d size: %d alloc: %d\n", page*page_size, meta.size, meta.alloc);
	
	unsigned int nextmetapos = page*page_size;   //this holds the position of the next metadata block in my_memory.
	while(meta.alloc == 1 || x > meta.size){ //find first unallocated metadata.
		
		nextmetapos += sizeof(meta) + meta.size;
		if(nextmetapos > (next_page)*page_size){
			page++;
			next_page++;
			if(table[page].alloc == 0){
				table[page].alloc = 1;
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
	while(nextmetapos+sizeof(meta)+meta.size > next_page * page_size && nextmetapos+sizeof(meta)+meta.size < mem_size){
		
		if(table[next_page].alloc == 0){//this page was not allocated, let's claim it.
			table[next_page].alloc = 1; 
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
			if(table[next_page].alloc == 0){//this page was not allocated, lets claim it.
				table[next_page].alloc = 1; 
				page++; next_page++;
			}
				
		}
		memcpy((my_memory+nextmetapos), &nextmeta, sizeof(nextmeta));
		printf("new metadata: alloc: %d size: %d\n", nextmeta.alloc, nextmeta.size);
		
	
	}
	printpage(page, 0);
	return mem;
}


void* shalloc(size_t x){

	//init metadata on first call.
	if(user_start == 0 && shared_start == 0 && os_start == 0){
		initmem();
	}
	
	table_row* table = table_ptr;
	if(shared_start == 0){
		shared_start = (2048-4)*page_size;
	}
	
	//get table. 
	void* mem = NULL; 
		
	//try to allocate from os start
	//if the page 
	int i = shared_start/page_size;
	while(mem == NULL && i < mem_size/page_size){
		if(table[i].alloc == 0 && i == shared_start/page_size){ //if page is free or has never been claimed, then seg_alloc_first.
			mem = shared_seg_alloc_first(x);
			break;
		}
		if(table[i].alloc == 1){//we own this page.
			mem = shared_seg_alloc(x);
			if(mem == NULL){
				i++;
			}
			else{
				break;
			}
		} 
	}
	
	//printmem();
	return mem;
}


//Have mydeallocate check whether x is in the last four pages, if so, then call this inside. 
void shared_free(void* x){
	
	table_row* table = table_ptr;	//pointer to our table. 
	int page = (int)((x - (void*)my_memory)/page_size);
	
	int vaddr = page - shared_start/page_size + 1;

	void* metapos = x - sizeof(segdata);
	int metapos_page = (int)((metapos - (void*)my_memory)/page_size);
	int meta_vaddr = metapos_page - page + vaddr;
	
	printf("metapos: %d metapos_page: %d\n", metapos - (void*)my_memory, metapos_page);

	//set alloc = 0 and write to memory. 
	segdata meta; 
	memcpy(&meta, metapos, sizeof(segdata));
	meta.alloc = 0;
	memcpy(metapos, &meta, sizeof(meta));
	
	//move to next meta. 
	void* next_metapos = metapos + sizeof(meta) + meta.size;
	int next_metapos_page = (int)((next_metapos - (void*)my_memory)/page_size);
	
	printf("next_metapos: %d next_metapos_page: %d\n", next_metapos - (void*)my_memory, next_metapos_page);
	
	//double check to make sure metadata also doesn't end on a page we don't own.
	int next_metapos_end_page = (int)((next_metapos + sizeof(meta) - (void*)my_memory)/page_size);
	printf("next_metapos_end_page: %d\n",next_metapos_end_page);

	//if next meta is 0, then combine.
	segdata nextmeta;
	memcpy(&nextmeta, next_metapos, sizeof(segdata));
	if(nextmeta.alloc == 0){
		meta.size += nextmeta.size + sizeof(segdata);
		memcpy(metapos, &meta, sizeof(segdata));
		printf("combined meta and next.\n");
	}

	//Find previous segment's metadata.
	void* prev_metapos = (void*)(my_memory + shared_start);
	int prev_metapos_page;
	
	segdata prev_meta;
	while(prev_metapos != metapos){ //traverse from first page to find previous segment's metadata. 
		
		prev_metapos_page = (int)((prev_metapos - (void*)my_memory)/page_size);		
		//printf("prev_metapos_page: %d prev_meta_vaddr: %d\n", prev_metapos_page, prev_meta_vaddr);

		memcpy(&prev_meta, prev_metapos, sizeof(segdata));
		printf("prev_meta: %d prev_meta.size: %d prev_meta.alloc: %d\n", (prev_metapos - (void*)my_memory)%page_size, prev_meta.size, prev_meta.alloc);
		if(prev_metapos + sizeof(segdata) + prev_meta.size == metapos && prev_meta.alloc == 0){
			prev_meta.size += meta.size + sizeof(segdata);
			memcpy(prev_metapos, &prev_meta, sizeof(segdata));
			printf("combined prev and meta.\n");
			break;
		}
		else{
			printpage(prev_metapos_page, 0);
		}
		prev_metapos += sizeof(segdata) + prev_meta.size;
	}
	printf("\n");
	printpage(page, 0);
}


//@TODO: When we start to have contiguous virtual pages, we need to change free so that it can check the next page
//	     in my_memory if and only if our thread owns that page. 
//@TODO: We need to change this so that it starts at the right metadata location. We need to start at our thread's FIRST page. 
void mydeallocate(void* x, char* file, int line, int req){
	
	table_row* table = table_ptr;	//pointer to our table. 
	int page = (int)((x - (void*)my_memory)/page_size);
	int vaddr;
	pthread_t target; 
	if(req == 0){			//set target and vaddr to represent scheduler memory. 
		target = NULL;
		vaddr = page - os_start/page_size + 1;
	}
	else{					//set target and vaddr to represent current thread's memory. 
		target = currently_running_thread;
		vaddr = page - user_start/page_size + 1;
	}

	int checkPage = (int)((x-(void*)my_memory)/page_size);

	if(checkPage > 2043 && checkPage < 2048){
		shared_free(x);
		return;
	}

	
	if(table[page].thread != currently_running_thread){ //make sure the page with memory to free is ours. 
		pageswap(page, req, vaddr);
	}
	
	void* metapos = x - sizeof(segdata);
	int metapos_page = (int)((metapos - (void*)my_memory)/page_size);
	int meta_vaddr = metapos_page - page + vaddr;
	
	printf("metapos: %d metapos_page: %d meta_vaddr: %d\n", metapos - (void*)my_memory, metapos_page, meta_vaddr);
	if(table[metapos_page].thread != currently_running_thread){ //make sure page containing memory to free's metadata is ours. 
		pageswap(metapos_page, req, meta_vaddr);
	}

	//set alloc = 0 and write to memory. 
	segdata meta; 
	memcpy(&meta, metapos, sizeof(segdata));
	meta.alloc = 0;
	memcpy(metapos, &meta, sizeof(meta));
	
	//move to next meta. 
	void* next_metapos = metapos + sizeof(meta) + meta.size;
	int next_metapos_page = (int)((next_metapos - (void*)my_memory)/page_size);
	int next_meta_vaddr = next_metapos_page - page + vaddr; 
	
	printf("next_metapos: %d next_metapos_page: %d next_meta_vaddr: %d\n", next_metapos - (void*)my_memory, next_metapos_page, next_meta_vaddr);
	if(table[next_metapos_page].thread != currently_running_thread){ //make sure page containing memory to free's metadata is ours. 
		pageswap(next_metapos_page, req, next_meta_vaddr);
	}
	
	//double check to make sure metadata also doesn't end on a page we don't own.
	int next_metapos_end_page = (int)((next_metapos + sizeof(meta) - (void*)my_memory)/page_size);
	int next_meta_vaddr_end = next_metapos_page - page + vaddr; 
	printf("next_metapos_end_page: %d next_meta_vaddr_end: %d\n",next_metapos_end_page, next_meta_vaddr_end);
	if(table[next_metapos_end_page].thread != currently_running_thread){ //make sure page containing memory to free's metadata is ours. 
		pageswap(next_metapos_end_page, req, next_meta_vaddr);
	}
	
	//if next meta is 0, then combine.
	segdata nextmeta;
	memcpy(&nextmeta, next_metapos, sizeof(segdata));
	if(nextmeta.alloc == 0){
		meta.size += nextmeta.size + sizeof(segdata);
		memcpy(metapos, &meta, sizeof(segdata));
		printf("combined meta and next.\n");
	}

	//Find previous segment's metadata.
	void* prev_metapos;
	if(req == 0){
		prev_metapos = (void*)(my_memory + os_start);
	}
	else{
		prev_metapos = (void*)(my_memory + user_start);
	}
	
	int prev_metapos_page;
	int prev_meta_vaddr;
	
	segdata prev_meta;
	while(prev_metapos != metapos){ //traverse from first page to find previous segment's metadata. 
		
		prev_metapos_page = (int)((prev_metapos - (void*)my_memory)/page_size);
		prev_meta_vaddr = prev_metapos_page - page + vaddr; 
		
		printf("prev_metapos_page: %d prev_meta_vaddr: %d\n", prev_metapos_page, prev_meta_vaddr);
		
		if(table[prev_metapos_page].thread != currently_running_thread){ //make sure page is ours. 
			pageswap(prev_metapos_page, req, next_meta_vaddr);
		}
		
		memcpy(&prev_meta, prev_metapos, sizeof(segdata));
		printf("prev_meta: %d prev_meta.size: %d prev_meta.alloc: %d\n", (prev_metapos - (void*)my_memory)%page_size, prev_meta.size, prev_meta.alloc);
		if(prev_metapos + sizeof(segdata) + prev_meta.size == metapos && prev_meta.alloc == 0){
			prev_meta.size += meta.size + sizeof(segdata);
			memcpy(prev_metapos, &prev_meta, sizeof(segdata));
			printf("combined prev and meta.\n");
			break;
		}
		else{
			printpage(prev_metapos_page, req);
		}
		prev_metapos += sizeof(segdata) + prev_meta.size;
	}
	printf("\n");
	printpage(page, req);
}

static void handler(int sig, siginfo_t *si, void *unused){
	
	
    long addr = (long)(si->si_addr - (void*)my_memory);
	printf("addr: %x mem size: %x my_memory %x\n",addr, mem_size, my_memory);
	int page = -1;
	if(addr < mem_size){
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
		//printmem();
		return;
	}
	else{
		printf("Got SIGSEGV at %x out of bounds. would be page %d\n",si->si_addr, (int)(addr/page_size));
	    exit(1);
	}
}

