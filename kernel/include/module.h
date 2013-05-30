#ifndef MODULE_H
#define MODULE_H

enum module_state {
  MODULE_NOT_INITIALIZED,
  MODULE_DEPS_RESOLVED,
  MODULE_INIT_RUN,
  MODULE_FINI_RUN 
};

typedef struct dependency {
  const char *name;
  struct module *module;
} dependency_t;

typedef struct module {
  const char *name;
  dependency_t *required;
  dependency_t *load_after;

  int (*init)(void);
  int (*fini)(void);

  enum module_state state;
  uintptr_t padding[2];
} module_t;

#define module_load __attribute__((__section__(".modules"), used))



module_t *find_module(const char *name);
void resolve_module(module_t *m);
void init_module(module_t *m);
void fini_module(module_t *m);

void init_static_modules();
void fini_static_modules();

#endif
