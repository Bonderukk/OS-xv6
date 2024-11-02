// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#define PGINDEX(pa) (((uint64)(pa) - (uint64)kmem.refcount) /PGSIZE);

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
  kmem.refcount=(int *)p;
  // Kolko mame stranok celkovo k dispozicii pre alokator
  uint64 pages = ((uint64)pa_end - (uint64)kmem.refcount) / PGSIZE;
  // Kolko stranok ubde mat pole refcount ?
  uint64 refcount_pages=(pages * sizeof(int) + PGSIZE +- 1) /PGSIZE;
  // uint64 refcount_pages=PGROUNDUP(pages * sizeof(int)) /PGSIZE;
  // uint64 refcount_pages=(pages * sizeof(*kmem.refcount) + PGSIZE +- 1) /PGSIZE;
  // TODO: incializovat
  for(int i = 0; i<refcount_pages;i++) {
    kmem.refcount[i] = 2;;
  }
  for(int i = refcount_pages; i< pages;i++) {
    kmem.refcount_pages[i] = 1;
  }
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

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  kmem.refcount[PGINDEX(r)]--;
  if(kmem.refcount[PGINDEX(r)]) {
    return 
  }
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
  // nastav pocet referencii na nulu
    kmem.refcount[PGINDEX(r)];
  }
  // Nieco este zle tu mam
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

void 
get_page(void *pa) {
  acquire(&kmem.lock);
  kmem.refcount[PGINDEX(r)]--;
  release(&kmem.lock);
  }

  