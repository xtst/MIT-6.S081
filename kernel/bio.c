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
// * Only one process at a ticks can use a buffer,
//     so do not keep them longer than necessary.

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define BUCKET_NUM 13
#define id(x) ((x) % BUCKET_NUM)

struct {
	struct spinlock lock[BUCKET_NUM];
	struct buf buf[NBUF];

	// Linked list of all buffers, through prev/next.
	// Sorted by how recently the buffer was used.
	// head.next is most recent, head.prev is least.
	struct buf head[BUCKET_NUM];
} bcache;

// int holding[BUCKET_NUM];

int can_lock(int id, int j) {
	int num = BUCKET_NUM / 2;
	if (id <= num) {
		if (j > id && j <= (id + num))
			return 0;
	} else {
		if ((id < j && j < BUCKET_NUM) || (j <= (id + num) % BUCKET_NUM)) {
			return 0;
		}
	}
	return 1;
}

void binit(void) {
	struct buf *b;

	for (int i = 0; i < BUCKET_NUM; i++)
		initlock(&bcache.lock[i], "bcache");

	bcache.head[0].next = &bcache.buf[0];
	// Create linked list of buffers
	for (b = bcache.buf; b < bcache.buf + NBUF - 1; b++) {
		b->next = b + 1;
		initsleeplock(&b->lock, "buffer");
	}
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf *
bget(uint dev, uint blockno) {
	// printf("bget: dev=%d blockno=%d\n", dev, blockno);

	struct buf *b;
	int id = id(blockno);

	acquire(&bcache.lock[id]);
	// Is the block already cached?
	for (b = bcache.head[id].next; b != 0; b = b->next) {
		if (b->dev == dev && b->blockno == blockno) {
			b->refcnt++;
			if (holding(&bcache.lock[id])) release(&bcache.lock[id]);
			acquiresleep(&b->lock);
			return b;
		}
	}

	// Not cached.
	// Recycle the least recently used (LRU) unused buffer.
	uint minticks = 1 << 30;
	// printf("about ticks: %d %d\n", minticks, ticks);
	int index = -1;

	for (int i = 0; i < BUCKET_NUM; i++) {
		// if (i == id && can_lock(id, i)) goto l1;
		// if (!can_lock(id, i)) continue;
		// if (i != id) acquire(&bcache.lock[i]);
		if (i != id && can_lock(id, i)) {
			// if j == id, then lock is already acquired
			// can_lock is to maintain an invariant of lock acquisition order
			// to avoid dead lock
			acquire(&bcache.lock[i]);
		} else if (!can_lock(id, i)) {
			continue;
		}
		for (b = bcache.head[i].next; b != 0; b = b->next) {
			if (b->refcnt == 0) {
				if (minticks > b->ticks /* && holding(&bcache.lock[minticks]) */) {
					// printf("update: %d %d %d\n", i, minticks, b->ticks);
					if (index != -1 && index != i && holding(&bcache.lock[index])) release(&bcache.lock[index]);
					index = i;
					minticks = b->ticks;
				}
			}
		}
		if (i != id && i != index && holding(&bcache.lock[i])) {
			release(&bcache.lock[i]);
		}
	}

	struct buf *selected = 0;
	if (index == -1) panic("bget: no buffers");

	for (b = &bcache.head[index]; b->next != 0; b = b->next) {
		if ((b->next)->refcnt == 0 && (b->next)->ticks == minticks) {
			selected = b->next;
			b->next = (b->next)->next;
			break;
		}
	}

	if (index != id && holding(&bcache.lock[index])) release(&bcache.lock[index]);
	b = &bcache.head[id];

	while (b->next) {
		b = b->next;
	}
	b->next = selected;
	// printf("%x\n", (uint64)selected);
	selected->next = 0; //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!调试我一天，蚌埠住了，千万要清零
	selected->dev = dev;
	selected->blockno = blockno;
	selected->valid = 0;
	selected->refcnt = 1;
	if (holding(&bcache.lock[id]))
		release(&bcache.lock[id]);
	acquiresleep(&selected->lock);
	return selected;
}

// Return a locked buf with the contents of the indicated block.
struct buf *
bread(uint dev, uint blockno) {
	struct buf *b;

	b = bget(dev, blockno);
	if (!b->valid) {
		virtio_disk_rw(b, 0);
		b->valid = 1;
	}
	return b;
}

// Write b's contents to disk.  Must be locked.
void bwrite(struct buf *b) {
	if (!holdingsleep(&b->lock))
		panic("bwrite");
	virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void brelse(struct buf *b) {
	if (!holdingsleep(&b->lock))
		panic("brelse");

	releasesleep(&b->lock);

	acquire(&bcache.lock[id(b->blockno)]);
	b->refcnt--;
	if (b->refcnt == 0) {
		// no one is waiting for it.
		b->ticks = ticks;
	}

	release(&bcache.lock[id(b->blockno)]);
}

void bpin(struct buf *b) {
	acquire(&bcache.lock[id(b->blockno)]);
	b->refcnt++;
	release(&bcache.lock[id(b->blockno)]);
}

void bunpin(struct buf *b) {
	acquire(&bcache.lock[id(b->blockno)]);
	b->refcnt--;
	release(&bcache.lock[id(b->blockno)]);
}
