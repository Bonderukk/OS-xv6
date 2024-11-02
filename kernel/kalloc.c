// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#define PGINDEX(pa) (((uint64)(pa) - (uint64)kmem.refcount) / PGSIZE)

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  int *refcount;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  
  // Initialize refcount array at the start of free memory
  kmem.refcount = (int*)p;
  
  // Calculate total pages and refcount array size
  uint64 pages = ((uint64)pa_end - (uint64)kmem.refcount) / PGSIZE;
  uint64 refcount_pages = (pages * sizeof(int) + PGSIZE - 1) / PGSIZE;
  
  // Initialize refcount array
  for(int i = 0; i < refcount_pages; i++) {
    kmem.refcount[i] = 1;  // Pages used by refcount array
  }
  for(int i = refcount_pages; i < pages; i++) {
    kmem.refcount[i] = 0;  // Free pages
  }

  // Skip the pages used by refcount array
  p += refcount_pages * PGSIZE;
  
  // Free the remaining pages
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  acquire(&kmem.lock);
  
  // Decrease reference count
  int idx = PGINDEX(pa);
  if(--kmem.refcount[idx] > 0) {
    release(&kmem.lock);
    return;
  }
  
  // If refcount reaches 0, free the page
  memset(pa, 1, PGSIZE); // Fill with junk
  r = (struct run*)pa;
  r->next = kmem.freelist;
  kmem.freelist = r;
  
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r) {
    kmem.freelist = r->next;
    // Initialize reference count to 1 for newly allocated page
    kmem.refcount[PGINDEX(r)] = 1;
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // Fill with junk
  return (void*)r;
}

// Increment reference count for a page
void 
get_page(void *pa) {
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("get_page");
    
  acquire(&kmem.lock);
  kmem.refcount[PGINDEX(pa)]++;
  release(&kmem.lock);
}

  