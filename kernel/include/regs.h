#ifndef REGS_H
#define REGS_H

typedef struct regs {
  uint32_t ds;
  uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
  uint32_t interrupt_num, error_code;
  uint32_t eip, cs, eflags, useresp, ss;
} regs_t;

#endif
