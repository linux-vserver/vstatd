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

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <sys/stat.h>
#include <dirent.h>
#include <lucid/open.h>
#include <lucid/str.h>

#include "cfg.h"
#include "log.h"
#include "proc.h"
#include "vrrd.h"

static cfg_opt_t CFG_OPTS[] = {
	CFG_STR("logfile", NULL, CFGF_NONE),
	CFG_STR("pidfile", NULL, CFGF_NONE),

	CFG_STR_CB("datadir", LOCALSTATEDIR "/vstatd", CFGF_NONE, &cfg_validate_path),
	CFG_END()
};

cfg_t *cfg;

static inline
void usage (int rc)
{
	printf("Usage: vstatd [<opts>]\n"
	       "\n"
	       "Available options:\n"
	       "   -c <file>     configuration file (default: %s/vstatd.conf)\n"
	       "   -d            debug mode (do not fork to background)\n",
	       SYSCONFDIR);
	exit(rc);
}

static
void handle_xid(xid_t xid)
{
	time_t cacct_time, cvirt_time, limit_time, loadavg_time;
	char *name;
	struct vx_vhi_name vhi_name;
	
	if (cacct_parse(xid, &cacct_time) == -1 ||
	    cvirt_parse(xid, &cvirt_time) == -1 ||
	    limit_parse(xid, &limit_time) == -1 ||
	    loadavg_parse(xid, &loadavg_time) == -1)
		return;
	
	vhi_name.field = VHIN_CONTEXT;
	
	if (vx_get_vhi_name(xid, &vhi_name) == -1) {
		log_error("vx_get_vhi_name(%d): %s", xid, strerror(errno));
		return;
	}
	
	/* For util-vserver guests */
	if (vhi_name.name[0] == '/')
		name = strrchr(vhi_name.name, '/') + 1;
	
	/* For vserver-utils guests */
	else
		name = vhi_name.name;

	if (cacct_rrd_check(name) == -1 ||
	    cvirt_rrd_check(name) == -1 ||
	    limit_rrd_check(name) == -1 ||
	    loadavg_rrd_check(name) == -1)
		return;
	
	if (cacct_rrd_update(name, cacct_time) == -1 ||
	    cvirt_rrd_update(name, cvirt_time) == -1 ||
	    limit_rrd_update(name, limit_time) == -1 ||
	    loadavg_rrd_update(name, loadavg_time) == -1)
		return;
}

static
void read_proc(void)
{
	DIR *dirp;
	struct dirent *ditp;
	xid_t xid;
	
	if ((dirp = opendir(PROC_VIRTUAL)) == NULL) {
		log_error("opendir(%s): %s", PROC_VIRTUAL, strerror(errno));
		return;
	}
	
	while ((ditp = readdir(dirp)) != NULL) {
		if (!str_isdigit(ditp->d_name))
			continue;
		
		xid = atoi(ditp->d_name);
		handle_xid(xid);
	}
	
	closedir(dirp);
	return;
}

int main (int argc, char *argv[]) {
	char *cfg_file = SYSCONFDIR "/vstatd.conf";
	
	char c, *pidfile = NULL;
	int fd, debug = 0;
	pid_t pid;

	if (getuid() != 0) {
		fprintf(stderr, "root access is needed\n");
		exit(EXIT_FAILURE);
	}
	
	/* parse command line */
	while ((c = getopt(argc, argv, "dc:")) != -1) {
		switch (c) {
		case 'c':
			cfg_file = optarg;
			break;
		
		case 'd':
			debug = 1;
			break;
		
		default:
			usage(EXIT_FAILURE);
			break;
		}
	}
	
	if (argc > optind)
		usage(EXIT_FAILURE);
	
	/* load configuration */
	cfg = cfg_init(CFG_OPTS, CFGF_NOCASE);
	
	switch (cfg_parse(cfg, cfg_file)) {
	case CFG_FILE_ERROR:
		perror("cfg_parse");
		exit(EXIT_FAILURE);
	
	case CFG_PARSE_ERROR:
		dprintf(STDERR_FILENO, "cfg_parse: Parse error\n");
		exit(EXIT_FAILURE);
	
	default:
		break;
	}
	
	/* start logging & debugging */
	if (log_init(debug) == -1) {
		perror("log_init");
		exit(EXIT_FAILURE);
	}
	
	/* fork to background */
	if (!debug) {
		pid = fork();
		
		switch (pid) {
		case -1:
			log_error_and_die("fork: %s", strerror(errno));
		
		case 0:
			break;
		
		default:
			exit(EXIT_SUCCESS);
		}
		
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
	}
	
	/* daemonize */
	close(STDIN_FILENO);
	umask(0);
	setsid();
	chdir("/");
	
	/* log process id */
	pidfile = cfg_getstr(cfg, "pidfile");
	
	if (pidfile && (fd = open_trunc(pidfile)) != -1) {
		dprintf(fd, "%d\n", getpid());
		close(fd);
	}
	
	log_info("Starting vstatd with config file: %s", cfg_file);
	
	while (1) {
		read_proc();
		sleep(30);
	}
	
	exit(EXIT_SUCCESS);
}
