// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
				   // defined by kernel.ld.

struct run {
	struct run *next;
};

struct {
	struct spinlock lock;
	struct run *freelist;
} kmem;

// For indexing the copy-on-write page reference count array

struct spinlock page_count_lock;
int pageref[(PHYSTOP - KERNBASE) >> PGSHIFT]; // reference count for each physical page
// note:  reference counts are incremented on fork, not on mapping. this means that
//        multiple mappings of the same physical page within a single process are only
//        counted as one reference.
//        this shouldn't be a problem, though. as there's no way for a user program to map
//        a physical page twice within it's address space in xv6.

void kinit() {
	initlock(&kmem.lock, "kmem");
	initlock(&page_count_lock, "COUNT");
	freerange(end, (void *)PHYSTOP);
}

void freerange(void *pa_start, void *pa_end) {
	char *p;
	p = (char *)PGROUNDUP((uint64)pa_start);
	for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
		kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa) {
	struct run *r;

	if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
		panic("kfree");

	acquire(&page_count_lock);
	if (--PA2COUNT(pa) <= 0) {
		memset(pa, 1, PGSIZE);
		r = (struct run *)pa;
		acquire(&kmem.lock);
		r->next = kmem.freelist;
		kmem.freelist = r;
		release(&kmem.lock);
	}
	release(&page_count_lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void) {
	struct run *r;

	acquire(&kmem.lock);
	r = kmem.freelist;
	if (r)
		kmem.freelist = r->next;
	release(&kmem.lock);

	if (r) {
		memset((char *)r, 5, PGSIZE); // fill with junk
		// reference count for a physical page is always 1 after allocation.
		// (no need to lock this operation)
		PA2COUNT(r) = 1;
	}

	return (void *)r;
}

void *cow_transfer(void *pa) {
	acquire(&page_count_lock);

	if (PA2COUNT(pa) <= 1) {
		release(&page_count_lock);
		return pa;
	}

	uint64 newpa = (uint64)kalloc();
	if (newpa == 0) {
		release(&page_count_lock);
		return 0; // out of memory
	}
	memmove((void *)newpa, (void *)pa, PGSIZE);
	PA2COUNT(pa)
	--;

	release(&page_count_lock);
	return (void *)newpa;
}

// increase reference count of the page by one
void krefpage(void *pa) {
	acquire(&page_count_lock);
	PA2COUNT(pa)
	++;
	release(&page_count_lock);
}

void kderefpage(void *pa) {
	// acquire(&page_count_lock);
	PA2COUNT(pa)
	--;
	// release(&page_count_lock);
}