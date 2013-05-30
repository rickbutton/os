#include "hal.h"

static console_t *consoles = NULL;

/* Lock for all console operations. */
static spinlock_t lock = SPINLOCK_RELEASED;

/* Registers a new console - declared in hal.h */
int register_console(console_t *c) {
  spinlock_acquire(&lock);

  if (consoles)
    consoles->prev = c;
  c->next = consoles;
  c->prev = NULL;
  
  consoles = c;

  /* If an open() function was provided, call it. */
  if (c->open)
    c->open(c);

  spinlock_release(&lock);
  return 0;
}

/** Similarly for unregistering a console - just search through the list of
    consoles and remove the offending item. Again we require thread
    synchronisation. { */

/* Unregisters a console - declared in hal.h */
void unregister_console(console_t *c) {
  spinlock_acquire(&lock);

  console_t *prev = NULL;
  console_t *this = consoles;

  /* Scan through the linked list looking for 'c'. */
  while (this) {
    if (this == c) {
      if (this->next)
        this->next->prev = prev;
      if (this->prev)
        this->prev->next = this->next;
      if (!prev)
        consoles = c;

      /* Found - call flush() then close() if they exist. */
      if (this->flush)
        this->flush(this);
      if (this->close)
        this->close(this);
      break;
    }
    prev = this;
    this = this->next;
  }

  spinlock_release(&lock);
}

/** Then we get to define writing and reading from the console. Writing is a
    simple broadcast operation - take the input and write it to all registered
    consoles. { */

/* Writes to a console - declared in hal.h */
void write_console(const char *buf, int len) {
  spinlock_acquire(&lock);
  console_t *this = consoles;
  while (this) {
    if (this->write)
      this->write(this, buf, len);
    this = this->next;
  }
  spinlock_release(&lock);
}
 
/** Reading is slightly different - the ``read()`` functions defined in
    ``console_t`` are supposed to be non-blocking. So we cycle through all
    registered consoles trying to find one for whom ``read()`` is defined and
    returns a number of bytes greater than zero (0 return value means no data was
    available). { */

/* Reads from a console - declared in hal.h */
int read_console(char *buf, int len) {
  if (len == 0) return 0;

  spinlock_acquire(&lock);
  console_t *this = consoles;
  while (this) {
    if (this->read) {
      int n = this->read(this, buf, len);
      if (n > 0) {
        spinlock_release(&lock);
        return n;
      }
    }
    this = this->next;
    if (!this) this = consoles;
  }
  spinlock_release(&lock);
  return -1;
}

/** Finally we define the function that will clean up any consoles active at
    time of shutdown and flush their contents, and define the ``module_t`` structure
    that will register us as a module! { */

/* Flush and close all consoles. */
static int shutdown_console() {
  console_t *this = consoles;
  while (this) {
    if (this->flush)
      this->flush(this);
    if (this->close)
      this->close(this);
    this = this->next;
  }
  return 0;
}

static module_t x module_load = {
  .name = "console",
  .required = NULL,
  .load_after = NULL,
  .init = NULL,
  .fini = &shutdown_console,
};
