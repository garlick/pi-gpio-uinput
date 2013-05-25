typedef struct gctx_struct *gctx_t;

typedef int (*KeyMapFun) (void *arg, int key);
typedef void (*ShutFun) (void);

void gpio_init (void);
int gpio_shutdown_set (ShutFun fun, int key1, int key2);
void gpio_fini (void);
int gpio_map_keys (KeyMapFun fun, void *arg);
int gpio_event (int *valp);
