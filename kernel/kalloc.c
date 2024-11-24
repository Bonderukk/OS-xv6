// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#define nameSize sizeof("kmem.cpu0")

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  char name[nameSize];
} kmems[NCPU];

uint
getCPUId() {
  push_off();
  uint cpu = cpuid();
  pop_off();
  return cpu;
}

void
kinit()
{
  for (int i = 0; i < NCPU; i++) {
    snprintf(kmems[i].name, nameSize, "kmem.cpu%d", i);
    initlock(&kmems[i].lock, kmems[i].name);
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
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

  uint cpu = getCPUId();

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmems[cpu].lock);
  r->next = kmems[cpu].freelist;
  kmems[cpu].freelist = r;
  release(&kmems[cpu].lock);
}

uint steal(uint cpu) {
  struct run *stolen = 0;
  short stolenCount = 0;
  for (int i = 0; i < NCPU; i++) {
    if (i == cpu) {
      continue;
    }
    acquire(&kmems[i].lock);
    if (kmems[i].freelist) {
      stolen = kmems[i].freelist;
      kmems[i].freelist = stolen->next;
      kmems[cpu].freelist = stolen;
      kmems[cpu].freelist->next = 0;
      stolenCount++;
      release(&kmems[i].lock);
      break;
    }
    release(&kmems[i].lock);
  }
  return stolenCount;
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r = 0;

  uint cpu = getCPUId();

  acquire(&kmems[cpu].lock);
  r = kmems[cpu].freelist;
  if(!r) {
    uint stolenCount = steal(cpu);
    if (stolenCount == 0) {
      release(&kmems[cpu].lock);
      return 0;
    }
    r = kmems[cpu].freelist;
  }
  kmems[cpu].freelist = r->next;
  release(&kmems[cpu].lock);

  memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
