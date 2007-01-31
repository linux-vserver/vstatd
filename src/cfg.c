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

#include <sys/stat.h>

#include <lucid/log.h>
#include <lucid/str.h>

#include "cfg.h"

void cfg_atexit(void)
{
	LOG_TRACEME
	cfg_free(cfg);
}

int cfg_validate_path(cfg_t *cfg, cfg_opt_t *opt,
                      const char *value, void *result)
{
	LOG_TRACEME

	struct stat sb;

	if (!str_path_isabs(value)) {
		cfg_error(cfg, "Invalid absolute path for %s = '%s'", opt->name, value);
		return -1;
	}

	if (lstat(value, &sb) == -1) {
		cfg_error(cfg, "File does not exist for %s = '%s'", opt->name, value);
		return -1;
	}

	*(const char **) result = (const char *) value;
	return 0;
}
