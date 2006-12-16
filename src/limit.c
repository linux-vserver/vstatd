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

static
struct limit_data {
	char *db;
	char *token;
	uint64_t min, cur, max;
} LIMIT[] = {
	{ "file_FILES", "FILES:", 0, 0, 0 },
	{ "file_OFD",   "OFD:",   0, 0, 0 },
	{ "file_LOCKS", "LOCKS:", 0, 0, 0 },
	{ "file_SOCK",  "SOCK:",  0, 0, 0 },
	{ "file_DENT",  "DENT:",  0, 0, 0 },
	{ "ipc_MSGQ",   "MSGQ:",  0, 0, 0 },
	{ "ipc_SHM",    "SHM:",   0, 0, 0 },
	{ "ipc_SEMA",   "SEMA:",  0, 0, 0 },
	{ "ipc_SEMS",   "SEMS:",  0, 0, 0 },
	{ "mem_VM",     "VM:",    0, 0, 0 },
	{ "mem_VML",    "VML:",   0, 0, 0 },
	{ "mem_RSS",    "RSS:",   0, 0, 0 },
	{ "mem_ANON",   "ANON:",  0, 0, 0 },
	{ "sys_PROC",   "PROC:",  0, 0, 0 },
	{ NULL,         NULL,     0, 0, 0 }
};

#define LIMIT_PATH_FMT  PROC_VIRTUAL_XID_FMT "/limit"
#define LIMIT_PATH_MAX  PROC_VIRTUAL_XID_MAX + 6

int limit_parse(xid_t xid, time_t *curtime)
{
	char limit_path[LIMIT_PATH_MAX + 1];
	char *buf, *p, *token;
	int i, fd;
	uint64_t min, cur, max;
	
	
	snprintf(limit_path, LIMIT_PATH_MAX, LIMIT_PATH_FMT, xid);
	
	if ((fd = open_read(limit_path)) == -1)
		return -1;
	
	if (curtime)
		*curtime = time(NULL);
	
	if (io_read_eof(fd, &buf) == -1)
		return -1;
	
	close(fd);
	
	if (vx_limit_reset(xid) == -1)
		log_warn("vx_reset_rlimit(%d): %s", xid, strerror(errno));
	
	while ((p = strsep(&buf, "\n"))) {
		token = malloc(strlen(p));
		
		if (sscanf(p, "%s %" SCNu64 " %" SCNu64 " %*c %" SCNu64,
		           token, &cur, &min, &max) == 4) {
			for (i = 0; LIMIT[i].db; i++)
				if (strcmp(token, LIMIT[i].token) == 0) {
					LIMIT[i].min = min;
					LIMIT[i].cur = cur;
					LIMIT[i].max = max;
				}
		}
		
		free(token);
	}
	
	return 0;
}

static
int limit_rrd_create(char *path)
{
	char timestr[32];
	time_t curtime = time(NULL);
	
	char *argv[] = {
		"create", path, "-b", timestr, "-s", STEP_STR,
		"DS:min:GAUGE:" STEP_STR ":0:9223372036854775807",
		"DS:cur:GAUGE:" STEP_STR ":0:9223372036854775807",
		"DS:max:GAUGE:" STEP_STR ":0:9223372036854775807",
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

int limit_rrd_check(char *name)
{
	const char *datadir;
	char *path;
	int i;
	
	datadir = cfg_getstr(cfg, "datadir");
	
	for (i = 0; LIMIT[i].db; i++) {
		asprintf(&path, "%s/%s/%s.rrd", datadir, name, LIMIT[i].db);
		
		if (!isfile(path) && limit_rrd_create(path) == -1) {
			free(path);
			return -1;
		}
		
		free(path);
	}
	
	return 0;
}

int limit_rrd_update(char *name, time_t curtime)
{
	const char *datadir;
	int i, argc;
	char *buf, *argv[4];
	
	datadir = cfg_getstr(cfg, "datadir");
	
	for (i = 0; LIMIT[i].db; i++) {
		asprintf(&buf,
			"update %s/%s/%s.rrd %ld:%" PRIu64 ":%" PRIu64 ":%" PRIu64,
			datadir,
			name,
			LIMIT[i].db,
			vrrd_align_time(curtime),
			LIMIT[i].min,
			LIMIT[i].cur,
			LIMIT[i].max);
		
		argc = argv_from_str(buf, argv, 4);
		
		if (rrd_update(argc, argv) == -1) {
			log_error("rrd_update(%s): %s", name, rrd_get_error());
			rrd_clear_error();
			return -1;
		}
	}
	
	return 0;
}
