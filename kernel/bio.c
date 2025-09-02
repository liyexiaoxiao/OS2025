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

struct {
	struct spinlock lock[NBUCKET];
	struct buf buf[NBUF];
	struct buf head[NBUCKET];
} bcache;

void
binit(void)
{
  struct buf *b;

  for (int i=0;i<NBUCKET;i++)
  {//initals
    initlock(&bcache.lock[i], "bcache");
  }
  // Create linked list of buffers
  bcache.head[0].next = &bcache.buf[0];
  for(b = bcache.buf; b < bcache.buf+NBUF-1; b++){
    b->next = b+1;//move
    initsleeplock(&b->lock, "buffer");
  }
  initsleeplock(&b->lock, "buffer");
}

void 
write_cache(struct buf *buf_entry, uint dev, uint blockno) {
  buf_entry->dev     = dev;
  buf_entry->blockno = blockno;
  buf_entry->valid   = 0;
  buf_entry->refcnt  = 1;
  buf_entry->time    = ticks;  // update time
}


// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno) {
  struct buf *cur, *prev;
  struct buf *target_buf = 0;
  int hash_id = HASH(blockno);

  acquire(&bcache.lock[hash_id]);

  // search
  cur  = bcache.head[hash_id].next;
  prev = &bcache.head[hash_id];

  for (; cur; cur = cur->next, prev = prev->next) {
    if (cur->dev == dev && cur->blockno == blockno) {
      cur->time++;
      cur->refcnt++;
      release(&bcache.lock[hash_id]);
      acquiresleep(&cur->lock);
      return cur;
    }
    // empty block
    if (cur->refcnt == 0) {
      target_buf = cur;
    }
  }

  // use empty
  if (target_buf) {
    write_cache(target_buf, dev, blockno);
    release(&bcache.lock[hash_id]);
    acquiresleep(&target_buf->lock);
    return target_buf;
  }

  int held_lock = -1;
  uint64 oldest_time = __UINT64_MAX__;
  struct buf *tmp_buf;
  struct buf *prev_take = 0;

  for (int i = 0; i < NBUCKET; i++) {
    if (i == hash_id) 
      continue;

    acquire(&bcache.lock[i]);

    for (cur = bcache.head[i].next, tmp_buf = &bcache.head[i]; cur; cur = cur->next, tmp_buf = tmp_buf->next) {
      if (cur->refcnt == 0 && cur->time < oldest_time) {
        // choose the longest-no-use block
        oldest_time = cur->time;
        prev_take   = tmp_buf;
        target_buf  = cur;

        // realease the old lock
        if (held_lock != -1 && held_lock != i && holding(&bcache.lock[held_lock])) {
          release(&bcache.lock[held_lock]);
        }
        held_lock = i;
      }
    }

    if (held_lock != i)
      release(&bcache.lock[i]);
  }

  if (!target_buf)
    panic("bget: no buffers");

  prev_take->next = target_buf->next;
  target_buf->next = 0;
  release(&bcache.lock[held_lock]);
  
  prev->next = target_buf;
  write_cache(target_buf, dev, blockno);

  release(&bcache.lock[hash_id]);
  acquiresleep(&target_buf->lock);

  return target_buf;
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

  int h = HASH(b->blockno);
  acquire(&bcache.lock[h]);
  b->refcnt--;
  release(&bcache.lock[h]);
}

void bpin(struct buf *b)
{
  int bucket_id = b->blockno % NBUCKET;
  acquire(&bcache.lock[bucket_id]);
  b->refcnt++;
  release(&bcache.lock[bucket_id]);
}
void bunpin(struct buf *b)
{
  int bucket_id = b->blockno % NBUCKET;
  acquire(&bcache.lock[bucket_id]);
  b->refcnt--;
  release(&bcache.lock[bucket_id]);
}