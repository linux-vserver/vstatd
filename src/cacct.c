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

static
struct cacct_data {
	uint32_t id;
	char *db;
	uint64_t recvp, recvb;
	uint64_t sendp, sendb;
	uint64_t failp, failb;
} CACCT[] = {
	{ NXA_SOCK_UNSPEC, "net_UNSPEC", 0, 0, 0, 0, 0, 0 },
	{ NXA_SOCK_UNIX,   "net_UNIX",   0, 0, 0, 0, 0, 0 },
	{ NXA_SOCK_INET,   "net_INET",   0, 0, 0, 0, 0, 0 },
	{ NXA_SOCK_INET6,  "net_INET6",  0, 0, 0, 0, 0, 0 },
	{ NXA_SOCK_OTHER,  "net_OTHER",  0, 0, 0, 0, 0, 0 },
	{ 0,               NULL,         0, 0, 0, 0, 0, 0 }
};

int cacct_parse(xid_t xid, time_t *curtime)
{
	LOG_TRACEME

	if (curtime)
		*curtime = time(NULL);

	int i;

	for (i = 0; CACCT[i].db; i++) {
		nx_sock_stat_t sb;

		sb.id = CACCT[i].id;

		if (nx_sock_stat(xid, &sb) == -1) {
			log_perror("nx_sock_stat(%d)", xid);
			return -1;
		}

		CACCT[i].recvp = sb.count[0];
		CACCT[i].recvb = sb.total[0];
		CACCT[i].sendp = sb.count[1];
		CACCT[i].sendb = sb.total[1];
		CACCT[i].failp = sb.count[2];
		CACCT[i].failb = sb.total[2];
	}

	return 0;
}

static
int cacct_rrd_create(char *path)
{
	LOG_TRACEME

	char timestr[32];
	time_t curtime = time(NULL);

	char *argv[] = {
		"create", path, "-b", timestr, "-s", STEP_STR,
		"DS:recvp:GAUGE:" STEP_STR ":0:9223372036854775807", /* TODO: max with uint64_t? */
		"DS:recvb:GAUGE:" STEP_STR ":0:9223372036854775807",
		"DS:sendp:GAUGE:" STEP_STR ":0:9223372036854775807",
		"DS:sendb:GAUGE:" STEP_STR ":0:9223372036854775807",
		"DS:failp:GAUGE:" STEP_STR ":0:9223372036854775807",
		"DS:failb:GAUGE:" STEP_STR ":0:9223372036854775807",
		RRA_DEFAULT
	};

	int argc = sizeof(argv) / sizeof(*argv);

	snprintf(timestr, 32, "%ld", curtime - STEP - (curtime % STEP));

	if (mkdirnamep(path, 0700) == -1) {
		log_perror("mkdirnamep(%s)", path);
		return -1;
	}

	if (rrd_create(argc, argv) == -1) {
		log_perror("rrd_create(%s): %s", path, rrd_get_error());
		rrd_clear_error();
		return -1;
	}

	return 0;
}

int cacct_rrd_check(char *name)
{
	LOG_TRACEME

	const char *datadir = cfg_getstr(cfg, "datadir");
	int i;

	for (i = 0; CACCT[i].db; i++) {
		char *path = NULL;

		asprintf(&path, "%s/%s/%s.rrd", datadir, name, CACCT[i].db);

		if (!isfile(path) && cacct_rrd_create(path) == -1) {
			mem_free(path);
			return -1;
		}

		mem_free(path);
	}

	return 0;
}

int cacct_rrd_update(char *name, time_t curtime)
{
	LOG_TRACEME

	const char *datadir = cfg_getstr(cfg, "datadir");
	int i;

	for (i = 0; CACCT[i].db; i++) {
		char *buf = NULL;

		asprintf(&buf,
			"update %s/%s/%s.rrd %ld"
			":%" PRIu64 ":%" PRIu64
			":%" PRIu64 ":%" PRIu64
			":%" PRIu64 ":%" PRIu64,
			datadir,
			name,
			CACCT[i].db,
			vrrd_align_time(curtime),
			CACCT[i].recvp,
			CACCT[i].recvb,
			CACCT[i].sendp,
			CACCT[i].sendb,
			CACCT[i].failp,
			CACCT[i].failb);

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
