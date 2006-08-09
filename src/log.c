// Copyright 2006 Benedikt Böhm <hollow@gentoo.org>
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
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <lucid/open.h>
#include <lucid/str.h>

#include "cfg.h"
#include "log.h"

static int log_fd     = -1;
static int log_level  = LOG_INFO;
static int log_stderr = 0;

int log_init(int debug)
{
	const char *logfile = cfg_getstr(cfg, "logfile");
	
	if (debug) {
		log_level  = LOG_DEBG;
		log_stderr = 1;
	}
	
	if (str_isempty(logfile))
		return 0;
	
	if (log_fd >= 0)
		return errno = EALREADY, -1;
	
	log_fd = open_append(logfile);
	
	if (log_fd == -1)
		return -1;
	
	return 0;
}

static
void _log_internal(int level, const char *fmt, va_list ap)
{
	time_t curtime = time(0);
	char *levelstr = NULL, timestr[64];
	va_list ap2;
	
	if (level > log_level)
		return;
	
	switch (level) {
	case LOG_DEBG:
		levelstr = "debug";
		break;
	
	case LOG_INFO:
		levelstr = "info";
		break;
	
	case LOG_WARN:
		levelstr = "warn";
		break;
	
	case LOG_ERR:
		levelstr = "error";
		break;
	}
	
	bzero(timestr, 64);
	strftime(timestr, 63, "%a %b %d %H:%M:%S %Y", localtime(&curtime));
	
	if (log_stderr) {
		va_copy(ap2, ap);
		
		dprintf(STDERR_FILENO, "[%s] [%5d] [%s] ",
		        timestr, getpid(), levelstr);
		vdprintf(STDERR_FILENO, fmt, ap2);
		dprintf(STDERR_FILENO, "\n");
		
		va_end(ap2);
	}
	
	if (log_fd > -1 && level < LOG_DEBG) {
		dprintf(log_fd, "[%s] [%5d] [%s] ", timestr, getpid(), levelstr);
		vdprintf(log_fd, fmt, ap);
		dprintf(log_fd, "\n");
	}
}

void log_debug(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	_log_internal(LOG_DEBG, fmt, ap);
	
	va_end(ap);
}

void log_info(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	_log_internal(LOG_INFO, fmt, ap);
	
	va_end(ap);
}

void log_warn(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	_log_internal(LOG_WARN, fmt, ap);
	
	va_end(ap);
}

void log_error(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	_log_internal(LOG_ERR, fmt, ap);
	
	va_end(ap);
}


void log_debug_and_die(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	_log_internal(LOG_DEBG, fmt, ap);
	
	va_end(ap);
	exit(EXIT_FAILURE);
}

void log_info_and_die(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	_log_internal(LOG_INFO, fmt, ap);
	
	va_end(ap);
	exit(EXIT_FAILURE);
}

void log_warn_and_die(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	_log_internal(LOG_WARN, fmt, ap);
	
	va_end(ap);
	exit(EXIT_FAILURE);
}

void log_error_and_die(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	
	_log_internal(LOG_ERR, fmt, ap);
	
	va_end(ap);
	exit(EXIT_FAILURE);
}
