#ifndef IO_H
#define IO_H

#include "types.h"

#define IRQ(n) (n+32)

#define CR0_PG  (1U<<31)  /* Paging enable */
#define CR0_WP  (1U<<16)  /* Write-protect - allow page faults in kernel mode */

static inline void outb(unsigned short port, uint8_t value) {
  __asm__ volatile ("outb %1, %0" : : "dN" (port), "a" (value));
}

static inline void outw(unsigned short port, unsigned short value) {
  __asm__ volatile ("outw %1, %0" : : "dN" (port), "a" (value));
}

static inline void outl(unsigned short port, unsigned int value) {
  __asm__ volatile ("outl %1, %0" : : "dN" (port), "a" (value));
}

static inline uint8_t inb(unsigned short port) {
  uint8_t ret;
  __asm__ volatile("inb %1, %0" : "=a" (ret) : "dN" (port));
  return ret;
}

static inline unsigned short inw(unsigned short port) {
  unsigned short ret;
  __asm__ volatile ("inw %1, %0" : "=a" (ret) : "dN" (port));
  return ret;
}

static inline unsigned int inl(unsigned short port) {
  unsigned int ret;
  __asm__ volatile ("inl %1, %0" : "=a" (ret) : "dN" (port));
  return ret;
}

static inline unsigned int read_cr0() {
  unsigned int ret;
  __asm__ volatile("mov %%cr0, %0" : "=r" (ret));
  return ret;
}
static inline unsigned int read_cr2() {
  unsigned int ret;
  __asm__ volatile("mov %%cr2, %0" : "=r" (ret));
  return ret;
}
static inline unsigned int read_cr3() {
  unsigned int ret;
  __asm__ volatile("mov %%cr3, %0" : "=r" (ret));
  return ret;
}

static inline void write_cr0(unsigned int val) {
  __asm__ volatile("mov %0, %%cr0" : : "r" (val));
}
static inline void write_cr2(unsigned int val) {
  __asm__ volatile("mov %0, %%cr2" : : "r" (val));
}
static inline void write_cr3(unsigned int val) {
  __asm__ volatile("mov %0, %%cr3" : : "r" (val));
}

#endif
