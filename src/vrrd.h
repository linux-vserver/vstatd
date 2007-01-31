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

#ifndef _VSTATD_VRRD_H
#define _VSTATD_VRRD_H

#include <time.h>
#include <vserver.h>

#include <lucid/log.h>

#define TOSTR(S) #S

/* steps = time/(STEP*rows) */
#define RRA_30M \
	"RRA:AVERAGE:0:" TOSTR(STEPS30M) ":" TOSTR(ROWS),

#define RRA_6H \
	"RRA:MIN:0:" TOSTR(STEPS6H) ":" TOSTR(ROWS), \
	"RRA:MAX:0:" TOSTR(STEPS6H) ":" TOSTR(ROWS), \
	"RRA:AVERAGE:0:" TOSTR(STEPS6H) ":" TOSTR(ROWS),

#define RRA_1D \
	"RRA:MIN:0:" TOSTR(STEPS1D) ":" TOSTR(ROWS), \
	"RRA:MAX:0:" TOSTR(STEPS1D) ":" TOSTR(ROWS), \
	"RRA:AVERAGE:0:" TOSTR(STEPS1D) ":" TOSTR(ROWS),

#define RRA_30D \
	"RRA:MIN:0:" TOSTR(STEPS30D) ":" TOSTR(ROWS), \
	"RRA:MAX:0:" TOSTR(STEPS30D) ":" TOSTR(ROWS), \
	"RRA:AVERAGE:0:" TOSTR(STEPS30D) ":" TOSTR(ROWS),

#define RRA_1Y \
	"RRA:MIN:0:" TOSTR(STEPS1Y) ":" TOSTR(ROWS), \
	"RRA:MAX:0:" TOSTR(STEPS1Y) ":" TOSTR(ROWS), \
	"RRA:AVERAGE:0:" TOSTR(STEPS1Y) ":" TOSTR(ROWS),

#define RRA_DEFAULT  RRA_30M RRA_6H RRA_1D RRA_30D RRA_1Y

static inline
time_t vrrd_align_time(time_t curtime)
{
	LOG_TRACEME

	int rest = curtime % STEP;

	if (rest > (STEP / 2))
		return (curtime + STEP - rest);
	else
		return (curtime - rest);
}

int cacct_fetch     (xid_t xid, time_t *curtime);
int cacct_rrd_check (char *name);
int cacct_rrd_update(char *name, time_t curtime);

int cvirt_fetch     (xid_t xid, time_t *curtime);
int cvirt_rrd_check (char *name);
int cvirt_rrd_update(char *name, time_t curtime);

int limit_fetch     (xid_t xid, time_t *curtime);
int limit_rrd_check (char *name);
int limit_rrd_update(char *name, time_t curtime);

int loadavg_fetch     (xid_t xid, time_t *curtime);
int loadavg_rrd_check (char *name);
int loadavg_rrd_update(char *name, time_t curtime);

#endif
