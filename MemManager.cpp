#include <iostream>
#include <algorithm>
#include <iomanip>

using namespace std;

/*******************************************
  Author  : Jonathan Koval 
  Program : Memory Manager
  Due Date: Feb 13th 2014  9:00 AM
  Course  : CSC 300 Data Structures
  Compiled using MS Visual Studio 2012
********************************************
*/

// GLOBAL DATA STRUCTURES =======================
typedef struct FREE_NODE * FREEPTR;
typedef struct ALLOCNODE * ALLOCPTR;

struct FREE_NODE  // FREE LIST NODES
{
  int start_byte;
  int end_byte;
  int size;
  
  FREEPTR next;
};
struct ALLOCNODE // ALLOCTADED LIST NODES
{
  int start_byte;
  int end_byte;
  int size;
  int id;
  
  ALLOCPTR next;
};
// ======   GLOBAL DATA =========================
FREEPTR  freelist = NULL;  // the FREE link list
ALLOCPTR alloclist = NULL; // the ALLOCATED link list
int total_memory_managed = 0; // the amount of memory managed

//======   PROTOTYPES ===========================
//--- test only ---
void dump_freelist(void);
void dump_alloclist(void);

//--- utility ---
void remove_size_zero(void);
void merge_concurrent_nodes(void);
void remove_memory_freelist(void);
void add_job_alloclist(void);
void remove_job(const int job);
int count_job_segs(const int job);
void remove_dup_array(int (&a)[100], int (&b)[100]);
int count_nodes(void);

//--- interface ---
void init_memory_manager(const int amount);
int allocate_memory(const int job, const int amount);
void release_memory(const int job);
int total_free(void);
int total_allocated(void);
int largest_free(void);
int job_allocated(const int job);
void report_memory(void);
void report_jobs(void);

//////////////////////////////////////////////////////////////////////
//======= TESTING FUNCTIONS ====================
//////////////////////////////////////////////////////////////////////
void dump_freelist(void)
{ 
	FREEPTR tmp = freelist; // temp pointer to list
	cout << "==================" << endl;
	cout << "  FREE LIST DUMP  " << endl;
	cout << "==================" << endl;
	while (tmp !=NULL)
	{
		cout << tmp->start_byte << " : ";
		cout << tmp->end_byte << " : ";
		cout << tmp->size << endl;
		tmp = tmp->next; //move pointer to next node (or NULL)
	}
}
//----------------------
void dump_alloclist(void)
{ 
	ALLOCPTR tmp = alloclist; // temp pointer to list
	cout << "========================" << endl;
	cout << "  ALLOCATED LIST DUMP  " << endl;
	cout << "========================" << endl;
	while (tmp !=NULL)
	{
		cout << tmp->start_byte << " : ";
		cout << tmp->end_byte << " : ";
		cout << tmp->size << " : ";
		cout << tmp->id << endl;
		tmp = tmp->next; //move pointer to next node (or NULL)
	}
}

//////////////////////////////////////////////////////////////////////
//======= UTILITY FUNCTIONS ====================
//////////////////////////////////////////////////////////////////////

//Free node size becomes zero remove node from free list
void remove_size_zero(void)
{
	FREEPTR temp = freelist;
	FREEPTR tempprev = freelist;
	while (temp != NULL)
	{
		if (temp->size == 0)
		{
			//if only one node in list
			if ( temp->next == NULL )
			{
				temp = NULL;
				return;
			}
			//remove 0 size node from list
			tempprev->next = temp->next;
			delete temp;
			return;
		}
		tempprev = temp;
		temp = temp->next;
	}
}

//Helper function that traverses free and merges necessary functions
void merge_concurrent_nodes(void)
{
	FREEPTR temp = freelist;
	FREEPTR tempprev = freelist;
	while (temp != NULL)
	{
		if (tempprev->end_byte == (temp->start_byte - 1))
		{
			//Merge is necessary
			tempprev->end_byte = temp->end_byte;
			tempprev->size = ( (tempprev->end_byte - tempprev->start_byte) + 1 );
			//Remove node being merged.
			tempprev->next = temp->next;
			delete temp;
			return;
		}
		tempprev = temp;
		temp = temp->next;
	}
}

// Removes memory from freelist memory 
// ::returns the amount
int remove_memory_freelist(const int job, const int amount, ALLOCPTR & insertnode)
{
	FREEPTR freelist_temp = freelist;

	// if the node being allocated is bigger then the size of available memory
	if ( total_memory_managed < amount || amount <= 0 || amount > largest_free())
	{
		return 0;
	}

	//modify the freelist and find first node large enough
   	while (freelist_temp != NULL)
	{
		//first node in the freelist
		if (freelist_temp->size >= amount)
		{
			//remove a portion of a entry from the freelist
			freelist_temp->size = freelist_temp->size - amount;
			insertnode->end_byte = freelist_temp->end_byte;
			freelist_temp->end_byte = freelist_temp->end_byte - amount;
			insertnode->start_byte = ( freelist_temp->end_byte + 1 );
			return amount;
		}
		freelist_temp = freelist_temp->next;
	}
	return amount;
}

// Modify the allocated list
// ::Returns the amount
int add_job_alloclist(const int job, const int amount, ALLOCPTR & insertnode)
{
	ALLOCPTR alloc_temp = alloclist;
	ALLOCPTR alloc_prev = alloclist;
	//modify the allocated list
	//if list is empty
	if ( alloclist == NULL )
	{
		alloclist = insertnode;
		return amount;
	}
	//if list has one or more then one node
	while ( alloc_temp != NULL )
	{
		alloc_temp = alloc_temp->next;
		if ( alloc_temp == NULL )
		{
			alloc_prev->next = insertnode;
			insertnode->next = alloc_temp;
		}
		alloc_prev = alloc_temp;
	}

	return amount;
}

//Removes a single instance of a job
void remove_job(const int job)
{
	ALLOCPTR temp = alloclist;
	ALLOCPTR prev = alloclist;

	while (temp != NULL)
	{
	
		if (temp->id == job)
		{
			if (temp == alloclist)
			{
				alloclist = temp->next;
				delete temp;
				return;
			}
			prev->next = temp->next;
			delete temp;
			return;
		}
		prev = temp;
		temp = temp->next;
	}
}

//counts the number of instances of a certain job
// ::returns the amount of instances of a job
int count_job_segs(const int job)
{
	ALLOCPTR temp = alloclist;
	int count = 0;

	while (temp != NULL)
	{
		if (temp->id == job)
		{
			count++;
		}
		temp = temp->next;
	}
	return count;
}

//removes one instance of a job
void remove_one_job(const int job)
{
	// Release memory from Allocated Memory list
	ALLOCPTR temp = alloclist;
	FREEPTR return_to_memory = new FREE_NODE;  
	FREEPTR freelist_temp = freelist;
	FREEPTR freelist_prev = freelist;


	while (temp != NULL)
	{
		if (temp->id == job)
		{
			//Traverse freelist to find temp->start 
			while (freelist_temp != NULL)
			{
				if (temp->start_byte == (freelist_temp->end_byte + 1))
				{
					freelist_temp->end_byte = temp->end_byte;
					freelist_temp->size = freelist_temp->end_byte - freelist_temp->start_byte + 1;
					
					//Remove entry from the allocated list
					remove_job(job);
					//Merge any necessary nodes
					merge_concurrent_nodes();
					return;
				}
				else if (freelist_temp->end_byte < temp->start_byte && freelist_temp->next == NULL)
				{
					return_to_memory->end_byte = temp->end_byte;
					return_to_memory->size = temp->size;
					return_to_memory->start_byte = temp->start_byte;

					//Set pointers for freelist
					return_to_memory->next = freelist_temp->next;
					freelist_prev->next = return_to_memory;

					//Remove entry from the allocated list
					remove_job(job);
					//Merge any necessary nodes
					merge_concurrent_nodes();
					return;
				}
				else if (freelist_temp->end_byte < temp->start_byte && freelist_temp->next->start_byte > temp->end_byte)
				{
					return_to_memory->end_byte = temp->end_byte;
					return_to_memory->size = temp->size;
					return_to_memory->start_byte = temp->start_byte;

					//Set pointers for freelist
					return_to_memory->next = freelist_temp->next;
					freelist_prev->next = return_to_memory;

					//Remove entry from the allocated list
					remove_job(job);
					//Merge any necessary nodes
					merge_concurrent_nodes();
					return;
				}
				freelist_prev = freelist_temp;
				freelist_temp = freelist_temp->next;
			}
		}
		temp = temp->next;
	}
}

//removes duplicates from array used to output report_jobs
void remove_dup_array(int (&a)[100], int (&b)[100])
{
	for (int i = 1; i < 100; i++)
	{
		bool matching = false;
		for (int j = 0; (j < i) && (matching == false); j++)
			if (a[i] == a[j]) 
				matching = true;
		
		if (!matching)
		{
			b[i] = a[i];
		//	cout << a[i] << endl;
		}
	}
}

//Counts all the nodes in both lists
int count_nodes(void)
{
	ALLOCPTR temp = alloclist;
	FREEPTR free_temp = freelist;
	int count = 0;

	while (temp != NULL)
	{
		count++;
		temp = temp->next;
	}
	while (free_temp != NULL)
	{
		count++;
		free_temp = free_temp->next;
	}
	return count;
}

//////////////////////////////////////////////////////////////////////
//======= INTERFACE FUNCTIONS ==================
//////////////////////////////////////////////////////////////////////
void init_memory_manager(const int amount)
{
	total_memory_managed = amount;
	// set up the freelist linked list
	freelist = new FREE_NODE;
	freelist -> size = amount;
	freelist -> start_byte = 0;
	freelist -> end_byte = amount -1;
	freelist -> next = NULL;
	// set up the alloclist linked list
	alloclist = NULL;
}
//----------------------
int allocate_memory(const int job, const int amount)
{
	ALLOCPTR insertnode = new ALLOCNODE;
	insertnode->id = job;
	insertnode->size = amount;
	insertnode->next = NULL;

	if( remove_memory_freelist(job, amount, insertnode) != 0 )
	{
		add_job_alloclist(job, amount, insertnode);
		remove_size_zero();
	}
	else
	{
		return 0;
	}

	return amount;  // return amount of memory allocated
}
//----------------------
void release_memory(const int job)
{
	int count = count_job_segs(job);
	for( int i = 0; i <= count; i++ )
	{
		remove_one_job(job);
	}
}
//----------------------
int total_free(void)
{
	//Finds total amount of free memory
	FREEPTR temp = freelist;
	int freememory = 0;

	while (temp != NULL)
	{
		freememory = freememory + temp->size;
		temp = temp->next;
	}
  
	return freememory; // return amount of free memory
}
//-----------------------
int total_allocated(void)
{
	//Finds total memory allocated
	ALLOCPTR temp = alloclist;
	int total = 0;

	while (temp != NULL)
	{
		total = total + temp->size;
		temp = temp->next;
	}
   
	return total; //return amount of allocated memory
};
//----------------------
int largest_free(void)
{
	//Finds the largest free node and returns its size
	FREEPTR temp = freelist;
	int currentnodesize = temp->size;

	while (temp != NULL)
	{
		if (temp->size > currentnodesize)
		{
			currentnodesize = temp->size;
		}
		temp = temp->next;
	}
	return currentnodesize; //returns largest size
}
//----------------------
int job_allocated(const int job)
{
	//finds the job and returns the amount of memory it uses
	ALLOCPTR temp = alloclist;
	int totalmem = 0;

	while (temp != NULL)
	{
		if (job == temp->id)
		{
			totalmem = temp->size;
		}
		temp = temp->next;
	}
	return totalmem; // return amount of allocated memory
}
//----------------------
void report_memory(void)
{
   // prints to screen all the memory in order of bytes
	ALLOCPTR alloc_temp = alloclist;
	FREEPTR free_temp = freelist;
	int last_byte = 0;
	int count = count_nodes();

	cout << endl << "==============================" << endl;
	cout << endl << "Memory Block" << "        JOB" << endl << endl;


	while(count != -1)
	{
		while(free_temp != NULL)
		{
			// if free list is "Empty" do not display
			if(free_temp->end_byte == -1)
			{

			}
			else if(free_temp->start_byte == last_byte )
			{
				last_byte = free_temp->end_byte + 1;
				cout << right << setw(5) << free_temp->start_byte << " - ";
				cout << free_temp->end_byte;
				if(free_temp->end_byte <= 99)
						cout << " ";
				cout << right << setw(15) << "FREE" << endl;
			}
			while(alloc_temp != NULL)
			{
				
				if(alloc_temp->start_byte == last_byte )
				{
					last_byte = alloc_temp->end_byte + 1;
					cout << right << setw(5) << alloc_temp->start_byte << " - ";
					cout << alloc_temp->end_byte;
					if(alloc_temp->end_byte <= 99)
						cout << " ";
					cout << right << setw(15) << alloc_temp->id << endl;
				
				}
				alloc_temp = alloc_temp->next;
			}
			free_temp = free_temp->next;
		}
		count--;
		alloc_temp = alloclist;
		free_temp = freelist;
	}
}
//----------------------
void report_jobs(void)
{
    // prints to screen all the jobs in or of job id
	ALLOCPTR temp = alloclist; // temp pointer to list
	int id[100];
	int idcheck[100];
	int i = 0;
	int instance_count = 0;
	//initialize the array
	std::fill(id, id+100, -1);
	std::fill(idcheck, idcheck+100, -1);

	while (temp !=NULL)
	{
		id[i] = temp->id;
		temp = temp->next;
		i++;
	}
	//sort the array
	std::sort(id, id+100);
	//remove duplicate values and set to -1
	remove_dup_array(id, idcheck);

	//Testing for entire array
/*	for(int q = 0; q < 100; q++)
		cout << idcheck[q] << endl;
	*/
	cout << endl << "==============================" << endl;
	cout << "JOB" << "         " << "Memory Usage" << endl;

	for(int j = 0; j <= 100; j++)
	{
		temp = alloclist; // set the list back to the headptr
		if(idcheck[j] > -1) //id is a valid entry
		{
			instance_count = count_job_segs(id[j]);

			while (temp != NULL) // traverse list
			{
				if(temp->id == idcheck[j])
				{
					if (instance_count == count_job_segs(idcheck[j]))
						cout << temp->id << "           ";
					if (instance_count > 0)
					{
						cout << temp->start_byte << " - ";
						cout << temp->end_byte << "  ";
						instance_count--;
					}
					if (instance_count == 0)
						cout << endl;
				}
				temp = temp->next; //move pointer to next node (or NULL)
			}
		}
	}
	cout << endl << endl;
}
//----------------------


//==========  MAIN =============================
int main(void)
{
//==========  MAIN =============================
  char ch ;  // used to pause between tests
  int r;     // results of allocate_memory

  // ================================================================================
  cout << "====================================" << endl;
  cout << "  AUTHOR : Jonathan Koval  " << endl;
  cout << "====================================" << endl;
  cout << endl;
  cout << "ENTER A CHARACTER ";
  cin >>ch;
  cout << endl;
  //=================================================================================
  cout << "====================================" << endl;
  cout << "TEST # 1" << endl;
  cout << "====================================" << endl << endl;
  init_memory_manager(200);
  
  r = allocate_memory(1,200);  
  cout << "allocate_memory returns : " << r << endl << endl; // ALL memory  
  r = allocate_memory(2,30);
  cout << "allocate_memory returns : " << r << endl << endl; // over allocate
  
  release_memory(1);        // free all memory
  
  r = allocate_memory(1,-1);
  cout << "allocate_memory returns : " << r << endl << endl; // try allocate  -1
  r = allocate_memory(3,0);
  cout << "allocate_memory returns : " << r << endl << endl; // allocate 0
  r = allocate_memory(1,256);
  cout << "allocate_memory returns : " << r << endl << endl;  // over allocate
  r = allocate_memory(1,100);
  cout << "allocate_memory returns : " << r << endl << endl;  //Ok allocate 100
  
  cout << "total free memory is  : " << total_free() << endl;  // 100
  cout << "total alloc memory is : " << total_allocated() << endl; // 100
 

  cout << endl;
  cout << "ENTER A CHARACTER ";
  cin >> ch;
  cout << endl;

  // ================================================================================= 
  cout << "====================================" << endl;
  cout << "TEST # 2 [Deallocate several of same ] " << endl;
  cout << "====================================" << endl << endl;
  init_memory_manager(200);
  r = allocate_memory(1,20);
  cout << "allocate_memory returns : " << r << endl << endl;
  r = allocate_memory(2,30);
  cout << "allocate_memory returns : " << r << endl << endl;
  r = allocate_memory(1,20);
  cout << "allocate_memory returns : " << r << endl << endl;
  r = allocate_memory(3,30);
  cout << "allocate_memory returns : " << r << endl << endl;
  r = allocate_memory(1,20);
  cout << "allocate_memory returns : " << r << endl << endl;
 
  cout << "total free memory is  : " << total_free() << endl;
  cout << "total alloc memory is : " << total_allocated() << endl;
  
  release_memory(1);
  //======================
  report_memory();
  report_jobs();
   
  cout << "total free memory is  : " << total_free() << endl;  // 100
  cout << "total alloc memory is : " << total_allocated() << endl; // 100
 
  cout << endl;
  cout << "ENTER A CHARACTER ";
  cin >>ch;
  cout << endl;

  // =================================================================================
  cout << "====================================" << endl;
  cout << "TEST # 3  BETWEEN [merge to both blocks]" << endl;
  cout << "====================================" << endl << endl;
  init_memory_manager(200);

  r = allocate_memory(1,25);
  cout << "allocate_memory returns : " << r << endl << endl;
  r = allocate_memory(2,25);
  cout << "allocate_memory returns : " << r << endl << endl;
  r = allocate_memory(3,25);
  cout << "allocate_memory returns : " << r << endl << endl;
  r = allocate_memory(4,25);
  cout << "allocate_memory returns : " << r << endl << endl; 
  r = allocate_memory(5,25);
  cout << "allocate_memory returns : " << r << endl << endl; 
  //=====================
  cout << "========================" << endl << endl;
  cout << "total free memory is  : " << total_free() << endl;  // 100
  cout << "total alloc memory is : " << total_allocated() << endl; // 100
  release_memory(1);
  release_memory(3);
  release_memory(2);  
  //======================
  report_memory();
  report_jobs();
    
  system("pause");
  return 0;
}
