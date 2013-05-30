#include "hal.h"
#include "mmap.h"
#include "stdio.h"
#include "string.h"
#include "io.h"
#include "regs.h"

# define dbg(...)

static address_space_t *current = NULL;

static spinlock_t global_vmm_lock = SPINLOCK_RELEASED;

static int from_x86_flags(int flags) {
  int f = 0;
  if (flags & X86_WRITE)   f |= PAGE_WRITE;
  if (flags & X86_EXECUTE) f |= PAGE_EXECUTE;
  if (flags & X86_USER)    f |= PAGE_USER;
  if (flags & X86_COW)     f |= PAGE_COW;
  return f;
}
static int to_x86_flags(int flags) {
  int f = 0;
  if (flags & PAGE_WRITE)   f |= X86_WRITE;
  if (flags & PAGE_USER)    f |= X86_USER;
  if (flags & PAGE_EXECUTE) f |= X86_EXECUTE;
  if (flags & PAGE_COW)     f |= X86_COW;
  return f;
}

address_space_t *get_current_address_space() {
  return current;
}

int switch_address_space(address_space_t *dest) {
  write_cr3((uintptr_t)dest->directory | X86_PRESENT | X86_WRITE);
  return 0;
}

#define RPDT_BASE  1023
#define RPDT_BASE2 1022

#define PAGE_SIZE 4096U
#define PAGE_TABLE_SIZE (PAGE_SIZE * 1024U)
#define PAGE_TABLE_ENTRY(base, v) (uint32_t*)(base*PAGE_TABLE_SIZE + \
                                              ((v)>>12) * 4)
#define PAGE_DIR_ENTRY(base, v) (uint32_t*)(base*PAGE_TABLE_SIZE + \
                                            RPDT_BASE*PAGE_SIZE + \
                                            ((v)>>22) * 4)

static void ensure_page_table_mapped(uintptr_t v) {
  if (((*PAGE_DIR_ENTRY(RPDT_BASE, v)) & X86_PRESENT) == 0) {
    dbg("ensure_page_table_mapped: alloc_page!\n");
    uint64_t p = alloc_page(PAGE_REQ_UNDER4GB);
    dbg("alloc_page finished!\n");
    if (p == ~0ULL)
      panic("alloc_page failed in map()!");

    *PAGE_DIR_ENTRY(RPDT_BASE, v) = p | X86_PRESENT | X86_WRITE | X86_USER;

    /* Ensure that the new table is set to zero first! */
    v = (v >> 22) << 22; /* Clear the lower 22 bits. */
    
    memset(PAGE_TABLE_ENTRY(RPDT_BASE, v), 0, 0x1000);
  }
}

static int map_one_page(uintptr_t v, uint64_t p, unsigned flags) {
  dbg("map: getting lock...\n");
  spinlock_acquire(&current->lock);
  dbg("map: %x -> %x (flags %x)\n", v, (uint32_t)p, flags);
  /* Quick sanity check - a page with CoW must not be writable. */
  if (flags & PAGE_COW) {
    cow_refcnt_inc(p);
    flags &= ~PAGE_WRITE;
  }

  ensure_page_table_mapped(v);
  dbg("map: Made sure page table was mapped.\n");

  if (*PAGE_TABLE_ENTRY(RPDT_BASE, v) & X86_PRESENT) {
    kprintf("*** mapping %x to %x with flags %x\n", v, (uint32_t)p, flags);
    panic("Tried to map a page that was already mapped!");
  }

  *PAGE_TABLE_ENTRY(RPDT_BASE, v) = (p & 0xFFFFF000) |
    to_x86_flags(flags) | X86_PRESENT;
  dbg("map: About to release spinlock\n");
  spinlock_release(&current->lock);
  dbg("map: released spinlock\n");
  return 0;
}

int map(uintptr_t v, uint64_t p, int num_pages, unsigned flags) {
  for (int i = 0; i < num_pages; ++i) {
    if (map_one_page(v+i*0x1000, p+i*0x1000, flags) == -1)
      return -1;
    dbg("MAP DONE\n");
  }
  dbg("RET\n");
  return 0;
}

static int unmap_one_page(uintptr_t v) {
  spinlock_acquire(&current->lock);

  /** We do sanity checks to ensure what we're unmapping actually exists, else we'll
      get a page fault somewhere down the line... { */

  if ((*PAGE_DIR_ENTRY(RPDT_BASE, v) & X86_PRESENT) == 0)
    panic("Tried to unmap a page that doesn't have its table mapped!");

  uint32_t *pte = PAGE_TABLE_ENTRY(RPDT_BASE, v);
  if ((*pte & X86_PRESENT) == 0)
    panic("Tried to unmap a page that isn't mapped!");

  /** Again, ignore this stuff about copy-on-write, we'll cover it later. { */

  uint32_t p = *pte & 0xFFFFF000;
  if (*pte & X86_COW)
    cow_refcnt_dec(p);

 *pte = 0;

  /* Invalidate TLB entry. */
  uintptr_t *pv = (uintptr_t*)v;
  __asm__ volatile("invlpg %0" : : "m" (*pv));

  spinlock_release(&current->lock);
  return 0;
}

int unmap(uintptr_t v, int num_pages) {  
  for (int i = 0; i < num_pages; ++i) {
    if (unmap_one_page(v+i*0x1000) == -1)
      return -1;
  }
  return 0;
}

static int page_fault(regs_t *regs, void *ptr) {
  /* Get the faulting address from the %cr2 register. */
  uint32_t cr2 = read_cr2();

  /** Ignore this copy-on-write stuff for now. { */
  if (cow_handle_page_fault(cr2, regs->error_code))
    return 0;

  /* Just print out a panic message and trap to the debugger if one
     is available. If not, ``debugger_trap()`` will just spin
     infinitely. */
  kprintf("*** Page fault @ 0x%08x (", cr2);
  kprint_bitmask("iruwp", regs->error_code);
  kprintf(")\n");
  debugger_trap(regs);
  return 0;
}

uintptr_t iterate_mappings(uintptr_t v) {
  while (v < 0xFFFFF000) {
    v += 0x1000;
    if (is_mapped(v))
      return v;
  }
  return ~0UL;
}

uint64_t get_mapping(uintptr_t v, unsigned *flags) {
  if ((*PAGE_DIR_ENTRY(RPDT_BASE, v) & X86_PRESENT) == 0)
    return ~0ULL;

  uint32_t *page_table_entry = PAGE_TABLE_ENTRY(RPDT_BASE, v);
  if ((*page_table_entry & X86_PRESENT) == 0)
    return ~0ULL;

  if (flags)
    *flags = from_x86_flags(*page_table_entry & 0xFFF);

  return *page_table_entry & 0xFFFFF000;
}

int is_mapped(uintptr_t v) {
  unsigned flags;
  return get_mapping(v, &flags) != ~0ULL;
}

int init_virtual_memory() {
  /* Initialise the initial address space object. */
  static address_space_t a;
  /** We set up paging earlier during boot. The page directory is stored in the special register ``%cr3``, so we need to fetch it back. { */
  uint32_t d = read_cr3();
  a.directory = (uint32_t*) (d & 0xFFFFF000);

  spinlock_init(&a.lock);
  
  current = &a;

  /* We normally can't write directly to the page directory because it will
     be in physical memory that isn't mapped. However, the initial directory
     was identity mapped during bringup. */

  /* Recursive page directory trick - map the page directory onto itself. */
  a.directory[1023] = (uint32_t)a.directory | X86_PRESENT | X86_WRITE;

  /* Ensure that page tables are allocated for the whole of kernel space. */
  uint32_t *last_table = 0;
  for (uint64_t addr = MMAP_KERNEL_START; addr < MMAP_KERNEL_END; addr += 0x1000) {
    uint32_t *pde = PAGE_DIR_ENTRY(RPDT_BASE, (uint32_t)addr);
    if (pde != last_table) {
      if ((*pde & X86_PRESENT) == 0) {
        *pde = early_alloc_page() | X86_PRESENT | X86_WRITE;

        memset(PAGE_TABLE_ENTRY(RPDT_BASE, (uint32_t)addr), 0, 0x1000);
      }

      last_table = pde;
    }
  }

  /* Register the page fault handler. */
  register_interrupt_handler(14, &page_fault, NULL);

  /* Enable write protection, which allows page faults for read-only addresses
     in kernel mode. We need this for copy-on-write. */
  write_cr0( read_cr0() | CR0_WP );

  return 0;
}

int clone_address_space(address_space_t *dest, int make_cow) {
  spinlock_acquire(&global_vmm_lock);

  /* Allocate a page for the new page directory */
  uint32_t p = alloc_page(PAGE_REQ_NONE);
  
  spinlock_init(&dest->lock);
  dest->directory = (uint32_t*)p;

  /* Map the new directory temporarily in so we can populate it. */
  uint32_t base_addr = (uint32_t)PAGE_TABLE_ENTRY(RPDT_BASE2, 0);
  uint32_t base_dir_addr = (uint32_t)PAGE_DIR_ENTRY(RPDT_BASE2, 0);
  dbg("base_addr = %x\n", base_addr);
  *PAGE_DIR_ENTRY(RPDT_BASE, base_addr) = p | X86_WRITE | X86_PRESENT;
  *PAGE_TABLE_ENTRY(RPDT_BASE, base_dir_addr) = p | X86_WRITE | X86_PRESENT;
  /* FIXME: invlpg */

  /* Iterate over all PDE's in the source directory except the last 
     two which are reserved for the page dir trick. */
  for (uint32_t i = 0; i < MMAP_KERNEL_END; i += PAGE_TABLE_SIZE) {
    dbg("about to map %x, %x\n", i, (uint32_t)PAGE_DIR_ENTRY(RPDT_BASE2,i));
    /** By default every page directory entry in the new address space is the same as in the old address space. */
    *PAGE_DIR_ENTRY(RPDT_BASE2, i) = *PAGE_DIR_ENTRY(RPDT_BASE, i);
    dbg("here1\n");
    int is_user = ! IS_KERNEL_ADDR( PAGE_TABLE_SIZE * i );

    /** However, if the directory entry is present and is user-mode, we need
        to clone it to ensure that updates in the old address space don't affect
        the new address space and vice versa. { */
    /* Now we have to decide whether to copy/clone the current page table.
       We need to clone if it is present, and if it a user-mode page table. */
    if ((*PAGE_DIR_ENTRY(RPDT_BASE, i) & X86_PRESENT) && is_user) {
      dbg("here2\n");
      /* Create a new page table. */
      uint32_t p2 = alloc_page(PAGE_REQ_UNDER4GB);
      *PAGE_DIR_ENTRY(RPDT_BASE2, i) = p2 | X86_WRITE | X86_USER | X86_PRESENT;

      /* Copy every contained page table entry over. */
      for (unsigned j = 0; j < 1024; ++j) {
        uint32_t *d_pte = PAGE_TABLE_ENTRY(RPDT_BASE2, i + j * PAGE_SIZE);
        uint32_t *s_pte = PAGE_TABLE_ENTRY(RPDT_BASE,  i + j * PAGE_SIZE);

        /* If the page is user-mode and writable, make it copy-on-write. */
        if (make_cow && is_user && (*s_pte & X86_WRITE)) {
          *d_pte = (*s_pte & ~X86_WRITE) | X86_COW;
          cow_refcnt_inc(*s_pte & 0xFFFFF000);
        }
      }
    }
  }
  dbg("about to finish\n");

  dbg("finished clone\n");
  spinlock_release(&global_vmm_lock);

  return 0;
}
