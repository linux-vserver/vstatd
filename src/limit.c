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

#include <inttypes.h>
#include <rrd.h>
#include <sys/resource.h>

#include "cfg.h"
#include "vrrd.h"

#define _LUCID_PRINTF_MACROS
#include <lucid/log.h>
#include <lucid/mem.h>
#include <lucid/misc.h>
#include <lucid/printf.h>
#include <lucid/strtok.h>

static
struct limit_data {
	int id;
	char *db;
	uint64_t min, cur, max;
} LIMIT[] = {
	{ RLIMIT_NOFILE,   "file_FILES", 0, 0, 0 },
	{ VLIMIT_OPENFD,   "file_OFD",   0, 0, 0 },
	{ RLIMIT_LOCKS,    "file_LOCKS", 0, 0, 0 },
	{ VLIMIT_NSOCK,    "file_SOCK",  0, 0, 0 },
	{ VLIMIT_DENTRY,   "file_DENT",  0, 0, 0 },
	{ RLIMIT_MSGQUEUE, "ipc_MSGQ",   0, 0, 0 },
	{ VLIMIT_SHMEM,    "ipc_SHM",    0, 0, 0 },
	{ VLIMIT_SEMARY,   "ipc_SEMA",   0, 0, 0 },
	{ VLIMIT_NSEMS,    "ipc_SEMS",   0, 0, 0 },
	{ RLIMIT_AS,       "mem_VM",     0, 0, 0 },
	{ RLIMIT_MEMLOCK,  "mem_VML",    0, 0, 0 },
	{ RLIMIT_RSS,      "mem_RSS",    0, 0, 0 },
	{ VLIMIT_ANON,     "mem_ANON",   0, 0, 0 },
	{ RLIMIT_NPROC,    "sys_PROC",   0, 0, 0 },
	{ 0,               NULL,         0, 0, 0 }
};

int limit_parse(xid_t xid, time_t *curtime)
{
	LOG_TRACEME

	if (curtime)
		*curtime = time(NULL);

	if (vx_limit_reset(xid) == -1)
		log_pwarn("vx_reset_rlimit(%d)", xid);

	int i;

	for (i = 0; LIMIT[i].db; i++) {
		vx_limit_stat_t sb;

		sb.id = LIMIT[i].id;

		if (vx_limit_stat(xid, &sb) == -1) {
			log_perror("x_limit_stat(%d)", xid);
			return -1;
		}

		LIMIT[i].min = sb.minimum;
		LIMIT[i].cur = sb.value;
		LIMIT[i].max = sb.maximum;
	}

	return 0;
}

static
int limit_rrd_create(char *path)
{
	LOG_TRACEME

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
		log_perror("mkdirnamep(%s)", path);
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
	LOG_TRACEME

	const char *datadir = cfg_getstr(cfg, "datadir");
	int i;

	for (i = 0; LIMIT[i].db; i++) {
		char *path = NULL;

		asprintf(&path, "%s/%s/%s.rrd", datadir, name, LIMIT[i].db);

		if (!isfile(path) && limit_rrd_create(path) == -1) {
			mem_free(path);
			return -1;
		}

		mem_free(path);
	}

	return 0;
}

int limit_rrd_update(char *name, time_t curtime)
{
	LOG_TRACEME

	const char *datadir = cfg_getstr(cfg, "datadir");
	int i;

	for (i = 0; LIMIT[i].db; i++) {
		char *buf = NULL;

		asprintf(&buf,
			"update %s/%s/%s.rrd %ld:%" PRIu64 ":%" PRIu64 ":%" PRIu64,
			datadir,
			name,
			LIMIT[i].db,
			vrrd_align_time(curtime),
			LIMIT[i].min,
			LIMIT[i].cur,
			LIMIT[i].max);

		strtok_t _st, *st = &_st;

		if (!strtok_init_str(st, buf, " ", 0)) {
			mem_free(buf);
			return -1;
		}

		mem_free(buf);

		int argc    = strtok_count(st);
		char **argv = mem_alloc((argc + 1) * sizeof(char *));

		if (!argv) {
			mem_free(argv);
			strtok_free(st);
			return -1;
		}

		if (strtok_toargv(st, argv) < 1) {
			strtok_free(st);
			return -1;
		}

		if (rrd_update(argc, argv) == -1) {
			log_error("rrd_update(%s): %s", name, rrd_get_error());
			rrd_clear_error();
			return -1;
		}
	}

	return 0;
}
