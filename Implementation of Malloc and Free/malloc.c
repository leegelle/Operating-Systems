#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#define ALIGN4(s) (((((s)-1) >> 2) << 2) + 4)
#define BLOCK_DATA(b) ((b) + 1)
#define BLOCK_HEADER(ptr) ((struct _block *)(ptr)-1)

static int atexit_registered = 0;
static int num_mallocs = 0;
static int num_frees = 0;
static int num_reuses = 0;
static int num_grows = 0;
static int num_splits = 0;
static int num_coalesces = 0;
static int num_blocks = 0;
static int num_requested = 0;
static int max_heap = 0;
//initializing spot for next fit, to be the spot it last stopped at, for it to continue.
struct _block *spot = NULL;

/*
Liiban Gelle
1001172601
Professor Bakker
3320-003
*/

/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */
void printStatistics(void)
{
   printf("\nheap management statistics\n");
   printf("mallocs:\t%d\n", num_mallocs);
   printf("frees:\t\t%d\n", num_frees);
   printf("reuses:\t\t%d\n", num_reuses);
   printf("grows:\t\t%d\n", num_grows);
   printf("splits:\t\t%d\n", num_splits);
   printf("coalesces:\t%d\n", num_coalesces);
   printf("blocks:\t\t%d\n", num_blocks);
   printf("requested:\t%d\n", num_requested);
   printf("max heap:\t%d\n", max_heap);
}

struct _block
{
   size_t size;         /* Size of the allocated _block of memory in bytes */
   struct _block *prev; /* Pointer to the previous _block of allcated memory   */
   struct _block *next; /* Pointer to the next _block of allcated memory   */
   bool free;           /* Is this _block free?                     */
   char padding[3];
};

struct _block *heapList = NULL; /* Free list to track the _blocks available */

/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes 
 *
 * \return a _block that fits the request or NULL if no free _block matches
 *
 * \TODO Implement Next Fit
 * \TODO Implement Best Fit
 * \TODO Implement Worst Fit
 */

struct _block *findFreeBlock(struct _block **last, size_t size)
{
   struct _block *curr = heapList;

#if defined FIT && FIT == 0
   /* First fit */
   while (curr && !(curr->free && curr->size >= size))
   {
      *last = curr;
      curr = curr->next;
   }
#endif

#if defined BEST && BEST == 0
   struct _block *bigenough = NULL;
   int scope = 5000000;
   //continue to go through the blocks while curr has data
   while (curr)
   {
      *last = curr;
      //Only selecting blocks that are free and are big enough to hold the data
      if (curr->free == true && curr->size >= size)
      {
         //the data being inserted must be smaller than the scope
         if (curr->size < scope)
         {
            bigenough = curr;
            scope = curr->size;
         }
         //if it isn't big enough we'll continue to go through the blocks
         else
         {
            continue;
         }
      }
      //move on to the next block
      curr = curr->next;
   }

   curr = bigenough;
#endif

#if defined WORST && WORST == 0
   struct _block *toobig = NULL;
   int scope = 0;
   //continue to go through the blocks of data till curr doesn't have a value
   while (curr)
   {
      *last = curr;
      //Only will allocate data to the block if it is free and too big to hold the data
      if ((curr->free == true) && (curr->size >= size))
      {
         if (curr->size  > scope)
         {
            //reseting the scope to take the header size into account
            scope = curr->size - size;
            toobig = curr;
         }
         
      }
      //moving on to the next block
      curr = curr->next;
   }

   curr = toobig;
#endif

#if defined NEXT && NEXT == 0
   //If the spot it left off isn't null then I have be my starting point
   if (spot != NULL)
   {
      curr = spot;
   }
   else
   {
      spot = NULL;
   }
   
   //Continue to iterate through curr while it exists, free....
   while (curr && !(curr->free && curr->size >= size))
   {
      *last = curr;
      //Was having my nf off by an offset and this resolved that issue
      curr->size = curr->size - size;
      //Moving on to the next block
      curr = curr->next;
   }
   //Having spot be the next place I start out when it goes to reallocate more data
   spot = curr;
#endif

   return curr;
}

/*
 * \brief growheap
 *
 * Given a requested size of memory, use sbrk() to dynamically 
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param last tail of the free _block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated _block of NULL if failed
 */
struct _block *growHeap(struct _block *last, size_t size)
{
   /* Request more space from OS */
   struct _block *curr = (struct _block *)sbrk(0);
   struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size);

   assert(curr == prev);

   /* OS allocation failed */
   if (curr == (struct _block *)-1)
   {
      return NULL;
   }

   /* Update heapList if not set */
   if (heapList == NULL)
   {
      heapList = curr;
   }

   /* Attach new _block to prev _block */
   if (last)
   {
      last->next = curr;
   }

   /* Update _block metadata */
   curr->size = size;
   max_heap += size;
   curr->next = NULL;
   curr->free = false;

   return curr;
}

/*
 * \brief malloc
 *
 * finds a free _block of heap memory for the calling process.
 * if there is no free _block that satisfies the request then grows the 
 * heap and returns a new _block
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process 
 * or NULL if failed
 */
void *malloc(size_t size)
{

   if (atexit_registered == 0)
   {
      atexit_registered = 1;
      atexit(printStatistics);
   }

   /* Align to multiple of 4 */
   size = ALIGN4(size);

   /* Handle 0 size */
   if (size == 0)
   {
      return NULL;
   }

   /* Look for free _block */
   struct _block *last = heapList;
   struct _block *next = findFreeBlock(&last, size);

   //setting size of struct _block equal to block_size
   const int block_size = sizeof(struct _block);

   /* Could not find free _block, so grow heap */
   if (next == NULL)
   {
      max_heap =+(size);
      num_grows++;
      next = growHeap(last, size);

      num_requested += size;
   }
   /* TODO: Split free _block if possible */
   if ((next->size - size) < block_size && next != NULL)
   {
      size_t old_size = next->size;
      struct _block *old_next = next->next;
      
      //setting next pointer to new spot where it will be split
      uint8_t *ptr = (uint8_t *)next;
      //Only splitting when the block after it is not pointing to null
      if (next->next != NULL)
      {
         
         next->next = (struct _block *)(ptr + size + block_size);
         //Freeing the next block since it got split off and no longer contains the data
         next->next->free = true;
         //Readjust the size since it is being split off
         next->next->size = old_size - size - block_size;
         //Setting the next one after it equal to the original size to keep consistency through
         //the size of the blocks
         next->next->next = old_next;
         num_splits++;
         num_blocks++;
      }

   }

   /* Could not find free _block or grow heap, so just return NULL */
   if (next == NULL)
   {
      return NULL;
   }

   /* Mark _block as in use */
   next->free = false;
   //incrementing malloc
   num_reuses++;
   num_mallocs++;

   /* Return data address associated with _block */
   return BLOCK_DATA(next);
}

/*
 * \brief free
 *
 * frees the memory _block pointed to by pointer. if the _block is adjacent
 * to another _block then coalesces (combines) them
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */
void free(void *ptr)
{
   if (ptr == NULL)
   {
      return;
   }

   /* Make _block as free */
   struct _block *curr = BLOCK_HEADER(ptr);
   assert(curr->free == 0);
   curr->free = true;
   num_frees++;
   
  
 

   //setting size of struct _block equal to block_size
   const int block_size = sizeof(struct _block);
   /* TODO: Coalesce free _blocks if needed */
   struct _block *place = heapList; //A pointer to the heaplist for where it and the next block reference to coalesce into blocks

   while (place->next != NULL && place != NULL)
   {
      if (place->free && place->next->free)
      {
         num_coalesces++;
         //Combining the two blocks only if the next one exists
         if (place->next->next != NULL)
         {
            place->next = place->next->next;
            
      
         }
        
         //Having the next block equal to the size of itself and the next block as well
         place->size = (size_t)(place->size + place->next->size + block_size);
      }
      //Moving on to the block
      place = place->next;
      
   }
}

void *calloc(size_t nmemb, size_t size)
{
   struct _block *ptr;

   ptr = (struct _block *)malloc(nmemb * size);

   memset(ptr, 0, nmemb);

   return ptr;
}
void *realloc(void *ptr, size_t size)
{
   struct _block *new = (struct _block *)malloc(size);

   memcpy(new, ptr, size);

   return new;
}

/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/
