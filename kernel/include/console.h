#ifndef CONSOLE_H
#define CONSOLE_H

typedef struct console {
  /* Initialise a console object. */
  int (*open)(struct console *obj);
  /* Finish a console object. */
  int (*close)(struct console *obj);
  /* Read from a console - if no data is available, return zero without blocking.
     Else return as much data as is available, up to 'len'. Return the
     number of bytes read, or -1 on error. */
  int (*read)(struct console *obj, char *buf, int len);
  /* Write to a console - return the number of bytes written, or -1 on
     failure. */
  int (*write)(struct console *obj, const char *buf, int len);
  /* Flush the write buffer of a console. */
  void (*flush)(struct console *obj);

  /* Intrusive linked list, for HAL's use only. */
  struct console *prev, *next;
  /* Implementation dependent data. */
  void *data;
} console_t;

/* Register 'c' as a new console. Returns zero on success. */
int register_console(console_t *c);
/* Unregister 'c' from the set of consoles. */
void unregister_console(console_t *c);
/* Write to all consoles. */
void write_console(const char *buf, int len);
/* Read from the default console into a buffer. If there is no data 
   available, this call will block. Returns the number of bytes actually
   read, or -1 on error. */
int read_console(char *buf, int len);

#endif
