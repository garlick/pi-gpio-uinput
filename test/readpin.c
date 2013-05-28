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
/* readpin.c - read an edge triggered gpio input */

#include <stdio.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

/* workarounds */
#define READ_FIRST 1	/* poll returns immediately without this */
#define REOPEN_AFTER 0	/* reading stale value after poll - doesn't help */
#define DELAY_AFTER 1	/* reading stale value after poll - well this does */

void usage(void)
{
	fprintf (stderr, "Usage: getpin value_path\n");
	exit (1);
}

int main (int argc, char *argv[])
{
	struct pollfd fds[1];
	char c;

	if (argc != 2)
		usage();
	fds[0].fd = open (argv[1], O_RDONLY);
	if (fds[0].fd < 0) {
		perror (argv[1]);
		exit (1);
	}
#if READ_FIRST
	if (read (fds[0].fd, &c, 1) != 1) {
		perror ("read");
		exit (1);
	}
#endif
	fds[0].events = POLLPRI;
	fds[0].revents = 0;
	if (poll (fds, 1, -1) < 0) {
		perror ("poll");
		exit (1);
	}
	if (!(fds[0].revents & POLLPRI)) {
		fprintf (stderr, "poll returned with no events\n");
		exit (1);
	}
#if DELAY_AFTER
	usleep (5000); /* 1000 helped a lot, but not 100% */
#endif
#if REOPEN_AFTER
	close (fds[0].fd);
	fds[0].fd = open (argv[1], O_RDONLY);
	if (fds[0].fd < 0) {
		perror (argv[1]);
		exit (1);
	}
#else
	if (lseek (fds[0].fd, 0, SEEK_SET) < 0) {
		perror ("lseek");
		exit (1);
	}
#endif
	if (read (fds[0].fd, &c, 1) != 1) {
		perror ("read");
		exit (1);
	}
	printf ("%c\n", c);
	exit (0);
}
