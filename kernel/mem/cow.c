#include "assert.h"
#include "hal.h"
#include "mmap.h"
#include "string.h"
#include "stdio.h"

static uint32_t *cow_refcnt_array = (uint32_t*) MMAP_COW_REFCNTS;

static void init_page(uint64_t p) {
  uintptr_t backing_page =
    (uintptr_t)(&cow_refcnt_array[p >> get_page_shift()]) & ~get_page_mask();

  if (!is_mapped(backing_page)) {
    uint64_t page = alloc_page(PAGE_REQ_NONE);
    assert(page != ~0ULL && "alloc_page failed!");
    int ret = map(backing_page, page, 1, PAGE_WRITE);
    assert(ret != -1 && "map failed!");

    memset((void*)backing_page, 0, get_page_size());
  }
}

int init_cow_refcnts(range_t *ranges, unsigned nranges) {
  for (unsigned i = 0; i < nranges; ++i) {
    for (uint64_t j = 0; j < ranges[i].extent; j += get_page_size()) {
      init_page(ranges[i].start + j);
    }
  }
  return 0;
}

void cow_refcnt_inc(uint64_t p) {
  ++cow_refcnt_array[p >> get_page_shift()];
}

void cow_refcnt_dec(uint64_t p) {
  --cow_refcnt_array[p >> get_page_shift()];
}

unsigned cow_refcnt(uint64_t p) {
  return cow_refcnt_array[p >> get_page_shift()];
}

bool cow_handle_page_fault(uintptr_t cr2, uintptr_t error_code) {
  unsigned flags;

  uint32_t p = (uint32_t)get_mapping(cr2, &flags);

  if ((error_code & (X86_PRESENT|X86_WRITE)) &&
      p != ~0UL && (flags & PAGE_COW) ) {
    /* Page was marked copy-on-write. */
    uint32_t p2 = (uint32_t)alloc_page(PAGE_REQ_UNDER4GB);

    /* We have to copy the page. In order to avoid a costly and 
       non-reentrant map/unmap pair to temporarily have them
       both mapped into memory, copy first into a buffer on
       the stack (this means the stack must be >4KB). */
    uint8_t buffer[4096];

    uint32_t v = cr2 & 0xFFFFF000;
    memcpy(buffer, (uint8_t*)v, 0x1000);
    
    if (unmap(v, 1) == -1)
      panic("unmap() failed during copy-on-write!");

    unsigned f = ((flags & X86_USER) ? PAGE_USER : 0) |
      ((flags & X86_EXECUTE) ? PAGE_EXECUTE : 0) |
      PAGE_WRITE;
    if (map(v, p2, 1, f) == -1)
      panic("map() failed during copy-on-write!");

    memcpy((uint8_t*)v, buffer, 0x1000);

    /* Mark the old page as having one less reference. */
    cow_refcnt_dec(p);

    return true;
  }
  return false;
}
