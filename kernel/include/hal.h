#ifndef HAL_H
#define HAL_H

#include "types.h"
#include "module.h"
#include "console.h"
#include "regs.h"
#include <stdarg.h>

#define MAX_CORES 8

typedef struct spinlock {
  volatile unsigned val;
  volatile unsigned interrupts;
} spinlock_t;

#define SPINLOCK_RELEASED {.val=0, .interrupts=0}
#define SPINLOCK_ACQUIRED {.val=1, .interrupts=0}

/* Initialise a spinlock to the released state. */
void spinlock_init(spinlock_t *lock);
/* Returns a new, initialised spinlock. */
spinlock_t *spinlock_new();
/* Acquire 'lock', blocking until it is available. */
void spinlock_acquire(spinlock_t *lock);
/* Release 'lock'. Nonblocking. */
void spinlock_release(spinlock_t *lock);


void panic(const char *) __attribute__((noreturn));
void assert_fail(const char *cond, const char *file, int line) __attribute__((noreturn));
void kmain(void *, unsigned int);

int kprintf(const char *, ...);

struct regs;
typedef int (*interrupt_handler_t)(struct regs *r, void *p);

int register_interrupt_handler(int num, interrupt_handler_t handler, void *p);
int unregister_interrupt_handler(int num, interrupt_handler_t handler, void *p);
void enable_interrupts();
void disable_interrupts();
int get_interrupt_state();

/* Sets the current interrupt state - 1 is enabled, 0 is disabled. */
void set_interrupt_state(int enable);

/* The state of one core when debugging. */
typedef struct core_debug_state {
  struct regs *registers;
} core_debug_state_t;

/* A handler function for a debugger command. Receives the command given
   in cmd and can handle as appropriate, and the state of all cores
   in the system. The 'core' parameter is the current processor that
   the user is interested in. */
typedef void (*debugger_fn_t)(const char *cmd, core_debug_state_t *states, int core);

/* Cause a debug or breakpoint trap. */
void trap();

static inline void abort() {
  for(;;);
}

/* Invoke the debugger, from a debug/breakpoint trap interrupt handler. */
void debugger_trap(struct regs *regs);
/* Invoke the debugger, from an interrupt handler that is reporting abnormal 
   behaviour (i.e. not a trap or breakpoint exception).

   Description holds an implementation-defined string describing the error. */
void debugger_except(struct regs *regs, const char *description);

/* Registers a function for use in the debugger. */
int register_debugger_handler(const char *name, const char *help,
                              debugger_fn_t fn);

/* Perform a backtrace.
  
   If called with *data=0, return the IP location the current function was called
   from, and save implementation specific information in 'data'.

   Iteratively call until the return value is 0, at which point the backtrace
   is complete. */
uintptr_t backtrace(uintptr_t *data, struct regs *regs);

/* Set an instruction breakpoint. Returns -2 if this is not possible on the
   target, or -1 if it is supported but an error occurred.

   Otherwise the return value is an implementation defined number that
   can be passed to unset_insn_breakpoint. */
int set_insn_breakpoint(uintptr_t loc);
/* Removes an instruction breakpoint previously set with set_insn_breakpoint.
   Returns -2 if not supported, -1 on error or 0 on success. */
int unset_insn_breakpoint(int id);

/* Set a data read breakpoint. Returns -2 if this is not possible on the
   target, or -1 if it is supported but an error occurred.

   Otherwise the return value is an implementation defined number that
   can be passed to unset_read_breakpoint. */
int set_read_breakpoint(uintptr_t loc);
/* Removes a data read breakpoint previously set with set_read_breakpoint.
   Returns -2 if not supported, -1 on error or 0 on success. */
int unset_read_breakpoint(int id);

/* Set a data write breakpoint. Returns -2 if this is not possible on the
   target, or -1 if it is supported but an error occurred.

   Otherwise the return value is an implementation defined number that
   can be passed to unset_write_breakpoint. */
int set_write_breakpoint(uintptr_t loc);
/* Removes a data write breakpoint previously set with set_write_breakpoint.
   Returns -2 if not supported, -1 on error or 0 on success. */
int unset_write_breakpoint(int id);

/* Returns a string giving the name of the symbol the address 'addr' is in,
   and stores the offset from that symbol in '*offs'.

   Returns NULL if not supported or a symbol is unavailable. */
const char *lookup_kernel_symbol(uintptr_t addr, int *offs);

/* Given an implementation defined structure 'regs' (as given to an interrupt
   handler), fill in the given array of name-value pairs.

   names and values will have enough space to hold up to 'max' entries. The
   function will store names of registers and their equivalent values into these
   arrays, and return the number of registers saved.

   Returns -1 on failure. */
int describe_regs(struct regs *regs, int max, const char **names,
                  uintptr_t *values);

int get_processor_id();

/* Returns the number of processors in the system. Returns -1 if not
   implemented. */
int get_num_processors();

/* Returns a pointer to an array of processor IDs representing the processors
   in the system. */
int *get_all_processor_ids();

/* Returns an implementation defined value that can be passed to
   register_interrupt_handler to handle an inter-processor message/interrupt.

   Returns -1 if not implemented. */
int get_ipi_interrupt_num();

/* Given a struct regs from an interrupt handler registered to
   get_ipi_interrupt_num, returns the value that was passed to the
   send_ipi function. */
void *get_ipi_data(struct regs *r);

#define IPI_ALL -1
#define IPI_ALL_BUT_THIS -2

/* Sends an inter-processor interrupt - a message between cores. The value in
   'data' will be available to the receiving core via get_ipi_data.

   The special value -1 (IPI_ALL) for proc_id will send IPIs to all cores in the system.

   The value -2 (IPI_ALL_BUT_THIS) will send IPIs to all cores but this one. */
void send_ipi(int proc_id, void *data);

#define THREAD_STACK_SZ 0x2000  /* 8KB of kernel stack. */

#define X86_PRESENT 0x1
#define X86_WRITE   0x2
#define X86_USER    0x4
#define X86_EXECUTE 0x200
#define X86_COW     0x400

typedef struct address_space {
  uint32_t *directory;
  spinlock_t lock;
} address_space_t;

static inline unsigned get_page_size() {
  return 4096;
}

static inline unsigned get_page_shift() {
  return 12;
}

static inline unsigned get_page_mask() {
  return 0xFFF;
}

static inline uintptr_t round_to_page_size(uintptr_t x) {
  if ((x & 0xFFF) != 0)
    return ((x >> 12) + 1) << 12;
  else
    return x;
}



/*******************************************************************************
 * Memory management
 ******************************************************************************/

#define PAGE_WRITE   1 /* Page is writable */
#define PAGE_EXECUTE 2 /* Page is executable */
#define PAGE_USER    4 /* Page is useable by user mode code (else kernel only) */
#define PAGE_COW     8 /* Page is marked copy-on-write. It must be copied if
                          written to. */

#define PAGE_REQ_NONE     0 /* No requirements on page location */
#define PAGE_REQ_UNDER1MB 1 /* Require that the returned page be < 0x100000 */
#define PAGE_REQ_UNDER4GB 2 /* Require that the returned page be < 0x10000000 */

/* Returns the (default) page size in bytes. Not all pages may be this size
   (large pages etc.) */
unsigned get_page_size();

/* Rounds an address up so that it is page-aligned. */
uintptr_t round_to_page_size(uintptr_t x);

/* Allocate a physical page of the size returned by get_page_size().
   This should only be used between calling init_physical_memory_early() and
   init_physical_memory(). */
uint64_t early_alloc_page();

/* Allocate a physical page of the size returned by get_page_size(), returning
   the address of the page in the physical address space. Returns ~0ULL on
   failure.

   'req' is one of the 'PAGE_REQ_*' flags, indicating a requirement on the
   address of the returned page. */
uint64_t alloc_page(int req);
/* Mark a physical page as free. Returns -1 on failure. */
int free_page(uint64_t page);

uint64_t alloc_pages(int req, size_t num);
int free_pages(uint64_t pages, size_t num);

/* Creates a new address space based on the current one and stores it in
   'dest'. If 'make_cow' is nonzero, all pages marked WRITE are modified so
   that they are copy-on-write. */
int clone_address_space(address_space_t *dest, int make_cow);

/* Switches address space. Returns -1 on failure. */
int switch_address_space(address_space_t *dest);

/* Returns the current address space. */
address_space_t *get_current_address_space();

/* Maps 'num_pages' * get_page_size() bytes from 'p' in the physical address
   space to 'v' in the current virtual address space.

   Returns zero on success or -1 on failure. */
int map(uintptr_t v, uint64_t p, int num_pages,
        unsigned flags);
/* Unmaps 'num_pages' * get_page_size() bytes from 'v' in the current virtual address
   space. Returns zero on success or -1 on failure. */
int unmap(uintptr_t v, int num_pages);

/* If 'v' has a V->P mapping associated with it, return 'v'. Else return
   the next page (multiple of get_page_size()) which has a mapping associated
   with it. */
uintptr_t iterate_mappings(uintptr_t v);

/* If 'v' is mapped, return the physical page it is mapped to
   and fill 'flags' with the mapping flags. Else return ~0ULL. */
uint64_t get_mapping(uintptr_t v, unsigned *flags);

/* Return 1 if 'v' is mapped, else 0, or -1 if not implemented. */
int is_mapped(uintptr_t v);

/* A range of memory, with a start and a size. */
typedef struct range {
  uint64_t start;
  uint64_t extent;
} range_t;

/* Initialise the virtual memory manager.
   
   Returns 0 on success or -1 on failure. */
int init_virtual_memory();

/* Initialise the physical memory manager (stage 1), passing in a set
   of ranges and the maximum extent of physical memory
   (highest address + 1).

   The set of ranges will be copied, not mutated. */
int init_physical_memory_early(range_t *ranges, unsigned nranges,
                               uint64_t extent);

/* Initialise the physical memory manager (stage 2). This should be 
   done after the virtual memory manager is set up. */
int init_physical_memory();

/* Initialise the copy-on-write page reference counts. */
int init_cow_refcnts(range_t *ranges, unsigned nranges);

/* Increment the reference count of a copy-on-write page. */
void cow_refcnt_inc(uint64_t p);

/* Decrement the reference count of a copy-on-write page. */
void cow_refcnt_dec(uint64_t p);

/* Return the reference count of a copy-on-write page. */
unsigned cow_refcnt(uint64_t p);

/* Handle a page fault potentially caused by a copy-on-write access.

   'addr' is the address of the fault. 'error_code' is implementation 
   defined. Returns true if the fault was copy-on-write related and was
   handled, false if it still needs handling. */
bool cow_handle_page_fault(uintptr_t addr, uintptr_t error_code);


#endif
