/*****************************************************************************
 *  Copyright (C) 2013 Jim Garlick
 *  Written by Jim Garlick <garlick.jim@gmail.com>
 *  All Rights Reserved.
 *
 *  This file is part of pi-gpio-uinput.
 *  For details, see <https://github.com/garlick/pi-gpio-uinput>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License (as published by the
 *  Free Software Foundation) version 2, dated June 1991.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA or see
 *  <http://www.gnu.org/licenses/>.
 *****************************************************************************/
/* gpio.c - return a stream of keypress events from pi GPIO */

/* Inputs are configured to interrupt on rising and falling edges.
 * We call poll(2), which blocks until one of the inputs receives a
 * POLLPRI event.  When poll returns, we examine the pollfd array
 * to determine which button changed state and return that as a
 * keypress event to our main program, which feeds that to the uinput
 * driver.
 *
 * Inputs read 0 when button is depressed, 1 when released.
 * We have to record the previous state and, after poll(2) returns
 * indicating it changed, wait DEBOUNCE_MS for the state to settle
 * before reading it and comparing to the previous state.
 */


#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <linux/input.h> /* for KEY_ definitions only */

#include "gpio.h"

#ifndef PATH_MAX
#define PATH_MAX    1024
#endif
#define INTBUFLEN   16

#define DEBOUNCE_MS 5

static int shutdown_key1_ix = -1;
static int shutdown_key2_ix = -1;
static ShutFun shutdown_fun = NULL;

typedef struct {
    int pin;    /* pi GPIO pin number */
    int key;    /* key value (from linux/input.h) */
    int state;  /* previous state, for debounce */
    int fd;     /* fd open to sysfs 'value' file, passed to poll(2) */
} map_t;

static map_t map[] = {
    /* player 1 controls */
    { .pin = 4,    .key = KEY_DOWN,     .fd = -1, }, /* down */
    { .pin = 15,   .key = KEY_UP,       .fd = -1, }, /* up */
    { .pin = 17,   .key = KEY_RIGHT,    .fd = -1, }, /* right */
    { .pin = 18,   .key = KEY_LEFT,     .fd = -1, }, /* left */
    { .pin = 27,   .key = KEY_ENTER,    .fd = -1, }, /* green */
    { .pin = 22,   .key = KEY_LEFTALT,  .fd = -1, }, /* blue */
    { .pin = 23,   .key = KEY_SPACE,    .fd = -1, }, /* yellow */
    { .pin = 19,   .key = KEY_1,        .fd = -1, }, /* 1-player */
    { .pin = 16,   .key = KEY_2,        .fd = -1, }, /* 2-player */
    { .pin = 26,   .key = KEY_5,        .fd = -1, }, /* side button (coin) */
    { .pin = 20,   .key = KEY_ESC,      .fd = -1, }, /* side button (esc) */

    /* player 2 controls */
    { .pin = 24,   .key = KEY_F,        .fd = -1, }, /* down */
    { .pin = 10,   .key = KEY_R,        .fd = -1, }, /* up */
    { .pin = 9,    .key = KEY_G,        .fd = -1, }, /* right */
    { .pin = 25,   .key = KEY_D,        .fd = -1, }, /* left */
    { .pin = 11,   .key = KEY_A,        .fd = -1, }, /* green */
    { .pin = 8,    .key = KEY_S,        .fd = -1, }, /* blue */
    { .pin = 7,    .key = KEY_Q,        .fd = -1, }, /* yellow */
};
static const int maplen = sizeof (map) / sizeof (map[0]);

static __inline__ void _shutdown_hook (void)
{
    if (shutdown_fun && map[shutdown_key1_ix].state == 0
                     && map[shutdown_key2_ix].state == 0)
        shutdown_fun ();
}

int gpio_shutdown_set (ShutFun fun, int key1, int key2)
{
    int i;

    for (i = 0; i < maplen; i++) {
        if (map[i].key == key1)
            shutdown_key1_ix = i;
        if (map[i].key == key2)
            shutdown_key2_ix = i;
    }
    if (shutdown_key1_ix == -1 || shutdown_key1_ix == -1) {
        fprintf (stderr, "gpio_shutdown_set: invalid keys\n");
        return -1;
    }
    shutdown_fun = fun;
    return 0;
}

static void _export (int pin)
{
    struct stat sb;
    char path[PATH_MAX];
    char msg[INTBUFLEN];
    FILE *fp;

    snprintf (path, sizeof (path), "/sys/class/gpio/gpio%d", pin);
    if (stat (path, &sb) == 0)
        return;
    snprintf (path, sizeof (path), "/sys/class/gpio/export");
    fp = fopen (path, "w");
    if (!fp) {
        perror (path);
        exit (1);
    }
    snprintf (msg, sizeof (msg), "%d", pin);
    if (fputs (msg, fp) < 0) {
        perror (path);
        exit (1);
    }
    if (fclose (fp) != 0) {
        perror (path);
        exit (1);
    }
}

static void _unexport (int pin)
{
    struct stat sb;
    char path[PATH_MAX];
    char msg[INTBUFLEN];
    FILE *fp;

    snprintf (path, sizeof (path), "/sys/class/gpio/gpio%d", pin);
    if (stat (path, &sb) < 0)
        return;
    snprintf (path, sizeof (path), "/sys/class/gpio/unexport");
    fp = fopen (path, "w");
    if (!fp) {
        perror (path);
        exit (1);
    }
    snprintf (msg, sizeof (msg), "%d", pin);
    if (fputs (msg, fp) < 0) {
        perror (path);
        exit (1);
    }
    if (fclose (fp) != 0) {
        perror (path);
        exit (1);
    }
}

static void _input (int pin)
{
    char path[PATH_MAX];
    FILE *fp;

    snprintf (path, sizeof (path), "/sys/class/gpio/gpio%d/direction", pin);
    fp = fopen (path, "w");
    if (!fp) {
        perror (path);
        exit (1);
    }
    if (fputs ("in", fp) < 0) {
        perror (path);
        exit (1);
    }
    if (fclose (fp) != 0) {
        perror (path);
        exit (1);
    }
}

static void _edge (int pin)
{
    char path[PATH_MAX];
    FILE *fp;

    snprintf (path, sizeof (path), "/sys/class/gpio/gpio%d/edge", pin);
    fp = fopen (path, "w");
    if (!fp) {
        perror (path);
        exit (1);
    }
    if (fputs ("both", fp) < 0) {
        perror (path);
        exit (1);
    }
    if (fclose (fp) != 0) {
        perror (path);
        exit (1);
    }
}

static int _read_value (int fd)
{
    char c;

    if (lseek (fd, 0, SEEK_SET) < 0) {
        perror ("lseek");
        exit (0);
    }
    if (read (fd, &c, 1) != 1) {
        perror ("read");
        exit (0);
    }
    return c == '0' ? 0 : 1;
}

static int _open_value (int pin)
{
    char path[PATH_MAX];
    int fd;

    snprintf (path, sizeof (path), "/sys/class/gpio/gpio%d/value", pin);
    fd = open (path, O_RDONLY);
    if (fd < 0) {
        perror (path);
        exit (1);
    }
    return fd;
}

int gpio_map_keys (KeyMapFun fun, void *arg)
{
    int i;

    for (i = 0; i < maplen; i++) {
        if (fun (arg, map[i].key) < 0)
            return -1;
    }
    return 0;
}

void gpio_init (void)
{
    int i;

    for (i = 0; i < maplen; i++) {
        _export (map[i].pin);
        _input (map[i].pin);
        _edge (map[i].pin);
        map[i].fd = _open_value (map[i].pin);
        map[i].state = _read_value (map[i].fd);
    }
}

void gpio_fini (void)
{
    int i;
    for (i = 0; i < maplen; i++) {
        if (map[i].fd != -1)
            close (map[i].fd);
            _unexport (map[i].pin);
    }
}

int gpio_event (int *vp)
{
    static struct pollfd *pfd = NULL;
    int i, val;

    if (pfd == NULL) {
        pfd = malloc (sizeof (pfd[0]) * maplen);
        if (!pfd) {
            fprintf (stderr, "out of memory\n");
            exit(1);
        } 
        memset (pfd, 0, sizeof (pfd[0]) * maplen);
    }
    for (;;) {
        for (i = 0; i < maplen; i++) {
            if ((pfd[i].revents & POLLPRI)) {
                pfd[i].revents = 0;
                usleep (DEBOUNCE_MS * 1000);
                val = _read_value (pfd[i].fd);
                if (val != map[i].state) {
                    map[i].state = val;
                    _shutdown_hook ();
                    *vp = val == 0 ? 1 : 0; /* flip 0=depressed, 1=released */
                    return map[i].key;
                }
            }
            pfd[i].fd = map[i].fd;
            pfd[i].events = POLLPRI;
            pfd[i].revents = 0;
        }
        if (poll (pfd, maplen, -1) < 0) {
            perror ("poll");
            exit (1);
        }
    }
            
    return -1;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
