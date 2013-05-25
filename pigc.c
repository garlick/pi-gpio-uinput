/* pigc.c - pi game controller */

#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <linux/input.h> /* for KEY_ definitions only */

#include "gpio.h"
#include "uinput.h"

#define OPTIONS "fds"
#define HAVE_GETOPT_LONG 1

#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long (ac,av,opt,lopt,NULL)
static const struct option longopts[] = {
    {"foreground",      no_argument,        0, 'f'},
    {"debug",           no_argument,        0, 'd'},
    {"shutdown",        no_argument,        0, 's'},
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt (ac,av,opt)
#endif

static void usage (void)
{
    fprintf (stderr,
"Usage: pigc [OPTIONS]\n"
"   -f,--foreground    do not fork and diassociate with tty\n"
"   -d,--debug         show keypresses on stderr\n"
"   -s,--shutdown      shutdown system when 5 and ESC simultaneously pressed\n"
    );
    exit (1);
}

static void _shutdown (void)
{
    system ("/sbin/shutdown -h now");
}

int main (int argc, char *argv[])
{
    uctx_t uctx;
    int fopt = 0;
    int dopt = 0;
    int sopt = 0;
    int key, val;
    int c;

    while ((c = GETOPT (argc, argv, OPTIONS, longopts)) != -1) {
        switch (c) {
            case 'f':
                fopt = 1;
                break;
            case 'd':
                dopt = 1;
                break;
            case 's':
                sopt = 1;
                break;
            default:
                usage ();
        }
    }

    gpio_init ();
    if (sopt) {
        if (gpio_shutdown_set (_shutdown, KEY_5, KEY_ESC) < 0)
            exit (1);
    }
    uctx = uinput_init ();
    if (!uctx) 
        exit (1);
    if (gpio_map_keys ((KeyMapFun)uinput_init_key, uctx) < 0)
        exit (1);
    uinput_create (uctx);

    if (!fopt) {
        if (daemon(0, 0) < 0) {
            perror ("daemon");
            exit (1);
        }
    }

    for (;;) {
        key = gpio_event (&val);
        if (dopt)
            fprintf (stderr, "key_event key=0x%x val=%d\n", key, val);
        if (uinput_key_event (uctx, key, val) < 0) {
            perror ("uinput_key_event");
            exit (1);
        }
    }
    uinput_fini (uctx);
    gpio_fini ();

    exit (0);
}



/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
