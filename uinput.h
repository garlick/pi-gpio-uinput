typedef struct uctx_struct *uctx_t;

uctx_t uinput_init (void);
int uinput_create (uctx_t uc);
int uinput_init_key (uctx_t uc, int key);
void uinput_fini (uctx_t uc);
int uinput_key_event (uctx_t uc, int code, int val);
