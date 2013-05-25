/* uinput.c - emulate keyboard input device */

#include <stdio.h>
#include <stdlib.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <libgen.h>
#include <assert.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "uinput.h"
#include "gpio.h"

#define UINPUT_MAGIC 0x34342664
struct uctx_struct {
    int magic;
    int fd;
    struct uinput_user_dev uinp;
};

void uinput_fini (uctx_t uc)
{
    assert (uc->magic == UINPUT_MAGIC);
    if (uc->fd != -1) {
        ioctl (uc->fd, UI_DEV_DESTROY);
        close (uc->fd);
    }
    free (uc);
}

int uinput_create (uctx_t uc)
{
    if (ioctl(uc->fd, UI_DEV_CREATE) < 0) {
        perror ("ioctl UI_DEV_CREATE");
        return -1;
    }
    return 0;
}

int uinput_init_key (uctx_t uc, int key)
{
    if (ioctl (uc->fd, UI_SET_KEYBIT, key) < 0) {
        perror ("ioctl UI_SET_KEYBIT");
        return -1;
    }
    return 0;
}

uctx_t uinput_init (void)
{
    uctx_t uc;

    uc = malloc (sizeof (*uc));
    if (!uc)
        return NULL;
    memset (uc, 0, sizeof (*uc));
    uc->magic = UINPUT_MAGIC;
    uc->fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (uc->fd < 0) {
        perror ("/dev/uinput");
        uinput_fini (uc);
        return NULL;
    }
    if (ioctl (uc->fd, UI_SET_EVBIT, EV_KEY) < 0
                || ioctl (uc->fd, UI_SET_EVBIT, EV_REL) < 0) {
        perror ("ioctl UI_SET_EVBIT");
        uinput_fini (uc);
        return NULL;
    }

    snprintf (uc->uinp.name, sizeof (uc->uinp.name), "pigc-gamepad device");
    uc->uinp.id.version = 4; /* lies, lies, lies */
    uc->uinp.id.bustype = BUS_USB; 
    uc->uinp.id.product = 1;
    uc->uinp.id.vendor = 1;

    if (write (uc->fd, &uc->uinp, sizeof (uc->uinp)) != sizeof (uc->uinp)) {
        fprintf (stderr, "error initializing uinput\n");
        uinput_fini (uc);
        return NULL;
    }

    return uc;
}


int uinput_key_event (uctx_t uc, int code, int val)
{
    struct input_event ev;

    memset (&ev, 0, sizeof (ev));
    ev.type = EV_KEY;
    ev.code = code;
    ev.value = val;

    if (write (uc->fd, &ev, sizeof (ev)) != sizeof (ev)) {
        perror ("write key_event");
        exit (1);
    }

    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;

    if (write (uc->fd, &ev, sizeof (ev)) != sizeof (ev)) {
        perror ("write key_event");
        exit (1);
    }

    return 0;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
