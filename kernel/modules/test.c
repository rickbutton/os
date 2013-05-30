#include "hal.h"
#include "kmalloc.h"
#include "stdio.h"

static int test() {
  unsigned int p = 65000;
  unsigned int *i = (unsigned int *)kmalloc(sizeof(int) * p);
  kprintf("Address of array is %x\n", i);
  kprintf("That is decimal %u\n", i);
  kprintf("That is at %uKB\n", (unsigned int)i / 1024);
  kprintf("Which is at %uMB\n", (unsigned int)i / (1024 * 1024));
  kprintf("We asked for %uKB\n", p * 4 / 1024);
  for (int z = 0; z < p; z++) {
    i[z] = z;
  }
  kfree(i);
  kprintf("Making sure we got the same address\n");
  i = (unsigned int *)kmalloc(sizeof(int) * p);
  kprintf("New malloc addr is %x\n", i);
  kfree(i);
  int y = 0;
  kprintf("Now gonna test divide by zero\n");
  y = 1 / y;
  return 0;
}

static dependency_t prereqs[] = { {"console",NULL}, 
                                  {"kmalloc", NULL},
                                  {NULL,NULL} };
static module_t run_on_startup module_load = {
  .name = "test",
  .required = prereqs,
  .load_after = NULL,
  .init = &test,
  .fini = NULL
};
