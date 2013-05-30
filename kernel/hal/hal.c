#include "hal.h"
#include "regs.h"
#include "string.h"

#define weak __attribute__((__weak__))

void trap() {
  __asm__ volatile("int $3");
}

void panic(const char *message) weak;
void panic(const char *message) {
    for(;;);

}

void assert_fail(const char *cond, const char *file, int line) weak;
void assert_fail(const char *cond, const char *file, int line) {
    for(;;);

}

void kmain() weak;
void kmain() {
    trap();

}

int register_console(console_t *c) weak;
int register_console(console_t *c) {
  return -1;
}
void unregister_console(console_t *c) weak;
void unregister_console(console_t *c) {
}

void write_console(const char *buf, int len) weak;
void write_console(const char *buf, int len) {
}
int read_console(char *buf, int len) weak;
int read_console(char *buf, int len) {
  return -1;
}

int register_interrupt_handler(int num, interrupt_handler_t handler,
                               void *p) weak;
int register_interrupt_handler(int num, interrupt_handler_t handler,
                               void *p) {
  return -1;
}
int unregister_interrupt_handler(int num, interrupt_handler_t handler,
                                 void *p) weak;
int unregister_interrupt_handler(int num, interrupt_handler_t handler,
                                 void *p) {
  return -1;
}
void enable_interrupts() weak;
void enable_interrupts() {
}
void disable_interrupts() weak;
void disable_interrupts() {
}
int get_interrupt_state() weak;
int get_interrupt_state() {
  return 1;
}
void set_interrupt_state(int enable) weak;
void set_interrupt_state(int enable) {
}

void debugger_trap(struct regs *regs) weak;
void debugger_trap(struct regs *regs) {
}
void debugger_except(struct regs *regs, const char *description) weak;
void debugger_except(struct regs *regs, const char *description) {
}
int register_debugger_handler(const char *name, const char *help,
                              debugger_fn_t fn) weak;
int register_debugger_handler(const char *name, const char *help,
                              debugger_fn_t fn) {
  return -1;
}
uintptr_t backtrace(uintptr_t *data, regs_t *regs) {
  if (*data == 0) {
    if (regs)
      *data = regs->ebp;
    else
      __asm__ volatile("mov %%ebp, %0" : "=r" (*data));
  }

  uintptr_t ip = * (uintptr_t*) (*data+4);
  *data = * (uintptr_t*) *data;

  if (*data == 0)
    return 0;
  return ip;
}
int set_insn_breakpoint(uintptr_t loc) weak;
int set_insn_breakpoint(uintptr_t loc) {
  return -1;
}
int unset_insn_breakpoint(int id) weak;
int unset_insn_breakpoint(int id) {
  return -1;
}
int set_read_breakpoint(uintptr_t loc) weak;
int set_read_breakpoint(uintptr_t loc) {
  return -1;
}
int unset_read_breakpoint(int id) weak;
int unset_read_breakpoint(int id) {
  return -1;
}
int set_write_breakpoint(uintptr_t loc) weak;
int set_write_breakpoint(uintptr_t loc) {
  return -1;
}
int unset_write_breakpoint(int id) weak;
int unset_write_breakpoint(int id) {
  return -1;
}
const char *lookup_kernel_symbol(uintptr_t addr, int *offs) weak;
const char *lookup_kernel_symbol(uintptr_t addr, int *offs) {
  return NULL;
}

int describe_regs(regs_t *regs, int max, const char **names,
                  uintptr_t *values) {
  if (max < 16)
    return -1;
  if (!regs)
    panic("describe_regs(NULL)!");

  static const char *_names[] = {"eax", "ecx", "edx", "ebx", "esi", "edi",
                                 "eip", "ebp", "esp", "eflags", "cs", "U-esp",
                                 "cr0", "cr2", "cr3", "cr4"};
  memcpy((uint8_t*)names, (uint8_t*)_names, sizeof(const char*)*16);

  values[0] = regs->eax; values[1]  = regs->ecx; values[2] = regs->edx;
  values[3] = regs->ebx; values[4]  = regs->esi; values[5] = regs->edi;
  values[6] = regs->eip; values[7]  = regs->esp; values[8] = regs->esp;
  values[9] = regs->eflags; values[10] = regs->cs; values[11] = regs->useresp;

  __asm__ volatile("mov %%cr0, %0" : "=r" (values[12]));
  __asm__ volatile("mov %%cr2, %0" : "=r" (values[13]));
  __asm__ volatile("mov %%cr3, %0" : "=r" (values[14]));
  __asm__ volatile("mov %%cr4, %0" : "=r" (values[15]));

  return 16;
}

int get_processor_id() weak;
int get_processor_id() {
  return -1;
}
int get_num_processors() weak;
int get_num_processors() {
  return -1;
}
int *get_all_processor_ids() weak;
int *get_all_processor_ids() {
  return NULL;
}
int get_ipi_interrupt_num() weak;
int get_ipi_interrupt_num() {
  return -1;
}
void *get_ipi_data(struct regs *r) weak;
void *get_ipi_data(struct regs *r) {
  return NULL;
}
void send_ipi(int proc_id, void *data) weak;
void send_ipi(int proc_id, void *data) {
}
