#include "hal.h"
#include "multiboot.h"
#include "string.h"

#define EARLYALLOC_SZ 2048

/* The global multiboot struct, which will have all its pointers pointing to
   memory that has been earlyalloc()d. */
multiboot_t mboot;

extern void main(int, char *);

static uintptr_t earlyalloc(unsigned len) {
  static unsigned char buf[EARLYALLOC_SZ];
  static unsigned idx = 0;

  if (idx + len >= EARLYALLOC_SZ)
    /* Return NULL on failure. It's too early in the boot process to give out a
       diagnostic.*/
    return NULL;

  unsigned char *ptr = &buf[idx];
  idx += len;

  return (uintptr_t)ptr;
}

/* Helper function to split a string on space characters ' ', resulting
   in 'n' different strings. This is used to convert the kernel command line
   into a form suitable for passing to main(). */
static int tokenize(char tok, char *in, char **out, int maxout) {
  int n = 0;
  
  while(*in && n < maxout) {
    out[n++] = in;

    /* Spool until the next instance of 'tok', or end of string. */
    while (*in && *in != tok)
      ++in;
    /* If we exited because we saw a token, make it a NUL character
       and step over it.*/
    if (*in == tok)
      *in++ = '\0';
  }

  return n;
}

/* Entry point from assembly. */
void _main(multiboot_t *_mboot, unsigned int magic) {
  /* Copy the multiboot struct itself. */
  memcpy((unsigned char*)&mboot, (unsigned char*)_mboot, sizeof(multiboot_t));

  /* If the cmdline member is valid, copy it over. */
  if (mboot.flags & MBOOT_CMDLINE) {
    /* We are now operating from the higher half, so adjust the pointer to take
       this into account! */
    _mboot->cmdline += 0xC0000000;
    int len = strlen((char*)_mboot->cmdline) + 1;
    mboot.cmdline = earlyalloc(len);
    if (mboot.cmdline)
      memcpy((unsigned char*)mboot.cmdline, (unsigned char*)_mboot->cmdline, len);
  }

  if (mboot.flags & MBOOT_MODULES) {
    _mboot->mods_addr += 0xC0000000;
    int len = mboot.mods_count * sizeof(multiboot_module_entry_t);
    mboot.mods_addr = earlyalloc(len);
    if (mboot.mods_addr)
      memcpy((unsigned char*)mboot.mods_addr, (unsigned char*)_mboot->mods_addr, len);
  }

  if (mboot.flags & MBOOT_ELF_SYMS) {
    _mboot->addr += 0xC0000000;
    int len = mboot.num * mboot.size;
    mboot.addr = earlyalloc(len);
    if (mboot.addr)
      memcpy((unsigned char*)mboot.addr, (unsigned char*)_mboot->addr, len);
  }

  if (mboot.flags & MBOOT_MMAP) {
    _mboot->mmap_addr += 0xC0000000;
    mboot.mmap_addr = earlyalloc(mboot.mmap_length + 4);
    if (mboot.mmap_addr) {
      memcpy((unsigned char*)mboot.mmap_addr,
             (unsigned char*)_mboot->mmap_addr - 4, mboot.mmap_length+4);
      mboot.mmap_addr += 4;
      mboot.mmap_addr = _mboot->mmap_addr;
    }
  }

  /**
     And then finally all we need to do is take the kernel command line and
     split it for passing to main() - to do this we use a helper function
     ``tokenize()``, defined slightly earlier, to split the string on every
     space character. { */
  static char *argv[256];
  int argc = tokenize(' ', (char*)mboot.cmdline, argv, 256);

  main(argc, (char *)argv);
}


