// Copyright 2006      Remo Lemma <coloss7@gmail.com>
//           2006-2007 Benedikt Böhm <hollow@gentoo.org>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA

#include <inttypes.h>
#include <rrd.h>

#include "cfg.h"
#include "vrrd.h"

#define _LUCID_PRINTF_MACROS
#include <lucid/log.h>
#include <lucid/mem.h>
#include <lucid/misc.h>
#include <lucid/printf.h>
#include <lucid/strtok.h>

static uint32_t LOADAVG[3];

int loadavg_fetch(xid_t xid, time_t *curtime)
{
	LOG_TRACEME

	if (curtime)
		*curtime = time(NULL);

	vx_stat_t sb;

	if (vx_stat(xid, &sb) == -1) {
		log_perror("vx_stat(%d)", xid);
		return -1;
	}

	LOADAVG[0] = sb.load[0];
	LOADAVG[1] = sb.load[1];
	LOADAVG[2] = sb.load[2];

	return 0;
}

static
int loadavg_rrd_create(char *path)
{
	LOG_TRACEME

	char timestr[32];
	time_t curtime = time(NULL);

	char *argv[] = {
		"create", path, "-b", timestr, "-s", TOSTR(STEP),
		"DS:1MIN:GAUGE:"  TOSTR(HEARTBEAT) ":0:" TOSTR(UINT32_MAX),
		"DS:5MIN:GAUGE:"  TOSTR(HEARTBEAT) ":0:" TOSTR(UINT32_MAX),
		"DS:15MIN:GAUGE:" TOSTR(HEARTBEAT) ":0:" TOSTR(UINT32_MAX),
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

int loadavg_rrd_check(char *name)
{
	LOG_TRACEME

	const char *datadir = cfg_getstr(cfg, "datadir");;
	char *path = NULL;

	asprintf(&path, "%s/%s/sys_LOADAVG.rrd", datadir, name);

	if (!isfile(path) && loadavg_rrd_create(path) == -1) {
		mem_free(path);
		return -1;
	}

	mem_free(path);

	return 0;
}

int loadavg_rrd_update(char *name, time_t curtime)
{
	LOG_TRACEME

	const char *datadir = cfg_getstr(cfg, "datadir");
	char *buf = NULL;

	asprintf(&buf,
		"update %s/%s/sys_LOADAVG.rrd %ld:%" PRIu32 ":%" PRIu32 ":%" PRIu32,
		datadir,
		name,
		vrrd_align_time(curtime),
		LOADAVG[0],
		LOADAVG[1],
		LOADAVG[2]);

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

	return 0;
}
