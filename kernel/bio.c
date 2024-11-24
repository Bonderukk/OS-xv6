// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKETS 13
#define BUCKET(blockno)((blockno) % NBUCKETS)

struct {
  struct buf buf[NBUF];
  struct spinlock lock[NBUCKETS];
  struct spinlock evictlock;
  struct buf head[NBUCKETS];
} bcache;

void
binit(void)
{
  struct buf *b;
  initlock(&bcache.evictlock, "bcache.evictlock");
  for (int i = 0; i < NBUCKETS; i++) {
  	initlock(&bcache.lock[i], "bcache");
  	
  	bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }
  
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
  	int bucket = BUCKET(b->blockno);
    b->next = bcache.head[bucket].next;
    b->prev = &bcache.head[bucket];
    initsleeplock(&b->lock, "buffer");
    bcache.head[bucket].next->prev = b;
    bcache.head[bucket].next = b;
  }
}

void
moveBucket(struct buf* b, uint bucket) {
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[bucket].next;
    b->prev = &bcache.head[bucket];
    bcache.head[bucket].next->prev = b;
    bcache.head[bucket].next = b;
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int bucket = BUCKET(blockno);
  acquire(&bcache.lock[bucket]);

  // Is the block already cached?
  for(b = bcache.head[bucket].next; b != &bcache.head[bucket]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[bucket]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  release(&bcache.lock[bucket]);
  acquire(&bcache.evictlock);
  acquire(&bcache.lock[bucket]);
  
  for(b = bcache.buf; b != bcache.buf + NBUF; b++){
  	int evbucket = BUCKET(b->blockno);
	if (evbucket != bucket) {
		acquire(&bcache.lock[evbucket]);
	}
	
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
  	  if (evbucket != bucket) {
  	  	moveBucket(b, bucket);
	  	release(&bcache.lock[evbucket]);
	  }
      release(&bcache.evictlock);
      release(&bcache.lock[bucket]);
      acquiresleep(&b->lock);
      return b;
    }

    if (evbucket != bucket) {
  	  release(&bcache.lock[evbucket]);
    }
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int bucket = BUCKET(b->blockno);
  acquire(&bcache.lock[bucket]);
  b->refcnt--;
  release(&bcache.lock[bucket]);
}

void
bpin(struct buf *b) {
  int bucket = BUCKET(b->blockno);
  acquire(&bcache.lock[bucket]);
  b->refcnt++;
  release(&bcache.lock[bucket]);
}

void
bunpin(struct buf *b) {
  int bucket = BUCKET(b->blockno);
  acquire(&bcache.lock[bucket]);
  b->refcnt--;
  release(&bcache.lock[bucket]);
}


