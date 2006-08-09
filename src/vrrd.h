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

#ifndef _VSTATD_VRRD_H
#define _VSTATD_VRRD_H

#include <time.h>
#include <vserver.h>

#define STEP      30
#define STEP_STR "30"

#define RRA_30M \
	"RRA:MIN:0:1:60", \
	"RRA:MAX:0:1:60", \
	"RRA:AVERAGE:0:1:60",

#define RRA_6H \
	"RRA:MIN:0:12:60", \
	"RRA:MAX:0:12:60", \
	"RRA:AVERAGE:0:12:60",

#define RRA_1D \
	"RRA:MIN:0:48:60", \
	"RRA:MAX:0:48:60", \
	"RRA:AVERAGE:0:48:60",

#define RRA_30D \
	"RRA:MIN:0:1440:60", \
	"RRA:MAX:0:1440:60", \
	"RRA:AVERAGE:0:1440:60",

#define RRA_1Y \
	"RRA:MIN:0:17520:60", \
	"RRA:MAX:0:17520:60", \
	"RRA:AVERAGE:0:17520:60",

#define RRA_DEFAULT  RRA_30M RRA_6H RRA_1D RRA_30D RRA_1Y

static inline
time_t vrrd_align_time(time_t curtime)
{
	int rest = curtime % STEP;
	
	if (rest > (STEP / 2))
		return (curtime + STEP - rest);
	else
		return (curtime - rest);
}

int cacct_parse     (xid_t xid, time_t *curtime);
int cacct_rrd_check (char *name);
int cacct_rrd_update(char *name, time_t curtime);

int cvirt_parse     (xid_t xid, time_t *curtime);
int cvirt_rrd_check (char *name);
int cvirt_rrd_update(char *name, time_t curtime);

int limit_parse     (xid_t xid, time_t *curtime);
int limit_rrd_check (char *name);
int limit_rrd_update(char *name, time_t curtime);

int loadavg_parse     (xid_t xid, time_t *curtime);
int loadavg_rrd_check (char *name);
int loadavg_rrd_update(char *name, time_t curtime);

#endif
