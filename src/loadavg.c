// Copyright 2006 Remo Lemma <coloss7@gmail.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the
// Free Software Foundation, Inc.,
// 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <rrd.h>
#include <lucid/argv.h>
#include <lucid/io.h>
#include <lucid/misc.h>
#include <lucid/open.h>

#include "cfg.h"
#include "log.h"
#include "proc.h"
#include "vrrd.h"

static float LOADAVG[3];

#define CVIRT_PATH_FMT  PROC_VIRTUAL_XID_FMT "/cvirt"
#define CVIRT_PATH_MAX  PROC_VIRTUAL_XID_MAX + 6

int loadavg_parse(xid_t xid, time_t *curtime)
{
	char cvirt_path[CVIRT_PATH_MAX];
	char *buf, *p;
	int fd, ok = 0;
	float a, b, c;
	
	snprintf(cvirt_path, CVIRT_PATH_MAX, CVIRT_PATH_FMT, xid);
	
	if ((fd = open_read(cvirt_path)) == -1)
		return -1;
	
	if (curtime)
		*curtime = time(NULL);
	
	if (io_read_eof(fd, &buf) == -1)
		return -1;
	
	close(fd);
	
	while ((p = strsep(&buf, "\n"))) {
		if (sscanf(p, "loadavg: %f %f %f", &a, &b, &c) == 3) {
			LOADAVG[0] = a;
			LOADAVG[1] = b;
			LOADAVG[2] = c;
			ok = 1;
		}
	}
	
	if (!ok) {
		log_warn("no valid loadavg in %s", cvirt_path);
		LOADAVG[0] = -1;
		LOADAVG[1] = -1;
		LOADAVG[2] = -1;
	}
	
	return 0;
}

static
int loadavg_rrd_create(char *path)
{
	char timestr[32];
	time_t curtime = time(NULL);
	
	char *argv[] = {
		"create", path, "-b", timestr, "-s", STEP_STR,
		"DS:1MIN:GAUGE:" STEP_STR ":0:9223372036854775807",
		"DS:5MIN:GAUGE:" STEP_STR ":0:9223372036854775807",
		"DS:15MIN:GAUGE:" STEP_STR ":0:9223372036854775807",
		RRA_DEFAULT
	};
	
	int argc = sizeof(argv) / sizeof(*argv);
	
	snprintf(timestr, 32, "%ld", curtime - STEP - (curtime % STEP));
	
	if (mkdirnamep(path, 0700) == -1) {
		log_error("mkdirnamep(%s): %s", path, strerror(errno));
		return -1;
	}
	
	if (rrd_create(argc, argv) == -1) {
		log_error("rrd_create(%s): %s", path, rrd_get_error());
		rrd_clear_error();
		return -1;
	}
	
	return 0;
}

int loadavg_rrd_check(char *name)
{
	const char *datadir;
	char *path;
	
	datadir = cfg_getstr(cfg, "datadir");
	
	asprintf(&path, "%s/%s/sys_LOADAVG.rrd", datadir, name);
	
	if (!isfile(path) && loadavg_rrd_create(path) == -1) {
		free(path);
		return -1;
	}
	
	free(path);
	
	return 0;
}

int loadavg_rrd_update(char *name, time_t curtime)
{
	const char *datadir;
	int argc;
	char *buf, *argv[4];
	
	datadir = cfg_getstr(cfg, "datadir");
	
	asprintf(&buf,
		"update %s/%s/sys_LOADAVG.rrd %ld:%f:%f:%f",
		datadir,
		name,
		vrrd_align_time(curtime),
		LOADAVG[0],
		LOADAVG[1],
		LOADAVG[2]);
	
	argc = argv_from_str(buf, argv, 4);
	
	if (rrd_update(argc, argv) == -1) {
		log_error("rrd_update(%s): %s", name, rrd_get_error());
		rrd_clear_error();
		return -1;
	}
	
	return 0;
}
