#include "hal.h"
#include "module.h"
#include "string.h"

extern module_t __start_modules, __stop_modules;

static void earlypanic(const char *msg, const char *msg2);
static void log_status(int status, const char *name, const char *text);

void init_static_modules() {
  for (module_t *m = &__start_modules; m < &__stop_modules; m++) {
    m->state = MODULE_NOT_INITIALIZED;
  }  

  for (module_t *m = &__start_modules; m < &__stop_modules; m++) {
    resolve_module(m);
  }

  //hack to make screen debug load before anything else
  module_t *m = find_module("debug/screen");
  if (m)
    init_module(m);

  for (module_t *m = &__start_modules; m < &__stop_modules; m++) {
    init_module(m);
  }
}

void fini_static_modules() {
  for (module_t *m = &__start_modules; m < &__stop_modules; m++) {
    fini_module(m);
  }
}

void resolve_module(module_t *m) {
  if (m->state >= MODULE_DEPS_RESOLVED)
    return;

  for (dependency_t *p = m->required; p != NULL && p->name != NULL; ++p)
    p->module = find_module(p->name);

  for (dependency_t *p = m->load_after; p != NULL && p->name != NULL; ++p)
    p->module = find_module(p->name);

  m->state = MODULE_DEPS_RESOLVED;
}

void init_module(module_t *m) {
  if (m->state >= MODULE_INIT_RUN)
    return;
  m->state = MODULE_INIT_RUN;

  if (m->required)
    for (dependency_t *p = m->required; p != NULL && p->name != NULL; ++p) {
      if (!p->module)
        earlypanic("Module not found: ", p->name);
      else
        init_module(p->module);
    }

  if (m->load_after)
    for (dependency_t *p = m->load_after; p != NULL && p->name != NULL; ++p) {
      if (p->module)
        init_module(p->module);
    }

  int ok = 0;
  if (m->init) {
    ok = m->init();
  }
  log_status(ok, m->name, "Started");
}

void fini_module(module_t *m) {
  if (m->state != MODULE_INIT_RUN)
    return;
  m->state = MODULE_FINI_RUN;

  if (m->required)
    for (dependency_t *p = m->required; p != NULL && p->name != NULL; ++p) {
      if (!p->module)
        earlypanic("Module not found: ", p->name);
      else
        fini_module(p->module);
    }

  if (m->load_after)
    for (dependency_t *p = m->load_after; p != NULL && p->name != NULL; ++p) {
      if (p->module)
        fini_module(p->module);
    }

  int ok = 0;
  if (m->fini) {
    ok = m->fini();
  }
  log_status(ok, m->name, "Stopped");
}

module_t *find_module(const char *name) {
  for (module_t *m = &__start_modules; m < &__stop_modules; m++) {
    if (!strcmp(name, m->name)) return m;
  }
  return NULL;
}

static void log_status(int status, const char *name, const char *text) {
  write_console("[", 1);
  if (status == 0)
    write_console("\033[32m OK \033[0m", 13);
  else
    write_console("\033[31mFAIL\033[0m", 13);
  write_console("] ", 2);
  write_console(text, strlen(text));
  write_console(" ", 1);
  write_console(name, strlen(name));
  write_console("\n", 1);
}

static void earlypanic(const char *msg, const char *msg2) {
  write_console("PANIC! ", 7);
  write_console(msg, strlen(msg));
  if (msg2)
    write_console(msg2, strlen(msg2));
  write_console("\n", 1);

  for (;;) ;
}
