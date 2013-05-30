#include "hal.h"

void main(int argc, char *argv) {
  
  init_static_modules();
  kprintf("rOS v%d.%d loaded\n", 0, 1);
  kmain(0, 0);
  kprintf("System is going down, unloading kernel modules\n");
  fini_static_modules();
  
}
