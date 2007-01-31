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

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include <syslog.h>
#include <sys/stat.h>

#include "cfg.h"
#include "vrrd.h"

#define _LUCID_PRINTF_MACROS
#define _LUCID_SCANF_MACROS
#include <lucid/log.h>
#include <lucid/mem.h>
#include <lucid/open.h>
#include <lucid/printf.h>
#include <lucid/scanf.h>
#include <lucid/str.h>

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
	LOG_TRACEME

	time_t cacct_time, cvirt_time, limit_time, loadavg_time;

	if (cacct_parse(xid, &cacct_time) == -1 ||
	    cvirt_parse(xid, &cvirt_time) == -1 ||
	    limit_parse(xid, &limit_time) == -1 ||
	    loadavg_parse(xid, &loadavg_time) == -1)
		return;

	vx_uname_t uname;
	uname.id = VHIN_CONTEXT;

	if (vx_uname_get(xid, &uname) == -1) {
		log_perror("vx_uname_get(%d)", xid);
		return;
	}

	char *name = uname.value;
	char *p = str_chr(name, ':', str_len(name));

	if (p)
		*p = '\0';

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
	LOG_TRACEME

	DIR *dirp;

	if ((dirp = opendir("/proc/virtual")) == NULL) {
		log_perror("opendir(/proc/virtual)");
		return;
	}

	struct dirent *ditp;
	xid_t xid = -1;

	while ((ditp = readdir(dirp)) != NULL) {
		if (!str_isdigit(ditp->d_name))
			continue;

		sscanf(ditp->d_name, "%" SCNu32, &xid);
		handle_xid(xid);
	}

	closedir(dirp);
	return;
}

static
void sigsegv_handler(int sig, siginfo_t *info, void *ucontext)
{
	log_error("Received SIGSEGV for virtual address %p (%d,%d)",
			info->si_addr, info->si_errno, info->si_code);

	log_error("You probably found a bug in vstatd!");
	log_error("Please report it to hollow@gentoo.org");

	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	char *cfg_file = SYSCONFDIR "/vcd.conf";
	int c, debug = 0;

	/* install SIGSEGV handler */
	struct sigaction sa;

	sa.sa_sigaction = sigsegv_handler;
	sa.sa_flags = SA_RESETHAND|SA_SIGINFO;

	sigfillset(&sa.sa_mask);

	if (sigaction(SIGSEGV, &sa, NULL) == -1) {
		dprintf(STDERR_FILENO, "sigaction: %s", strerror(errno));
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
		dprintf(STDERR_FILENO, "cfg_parse: %s\n", strerror(errno));
		exit(EXIT_FAILURE);

	case CFG_PARSE_ERROR:
		dprintf(STDERR_FILENO, "cfg_parse: Parse error\n");
		exit(EXIT_FAILURE);

	default:
		break;
	}

	/* free configuration on exit */
	atexit(cfg_atexit);

	/* free all memory on exit */
	atexit(mem_freeall);

	/* start logging & debugging */
	log_options_t log_options = {
		.ident  = argv[0],
		.file   = false,
		.stderr = false,
		.syslog = true,
		.time   = true,
		.flags  = LOG_PID,
	};

	const char *logfile = cfg_getstr(cfg, "logfile");

	if (logfile && str_len(logfile) > 0) {
		log_options.file = true;
		log_options.fd   = open_append(logfile);
	}

	if (debug) {
		log_options.stderr = true;
		log_options.mask   = LOG_UPTO(LOG_DEBUG);
	}

	log_init(&log_options);

	/* close log multiplexer on exit */
	atexit(log_close);

	/* fork to background */
	if (!debug) {
		log_info("Running in background mode ...");

		pid_t pid = fork();

		switch (pid) {
		case -1:
			log_perror_and_die("fork");

		case 0:
			break;

		default:
			exit(EXIT_SUCCESS);
		}

		close(STDOUT_FILENO);
		close(STDERR_FILENO);
	}

	else
		log_info("Running in debugging mode ...");

	/* daemonize */
	close(STDIN_FILENO);
	umask(0);
	setsid();
	chdir("/");

	/* log process id */
	int fd;
	const char *pidfile = cfg_getstr(cfg, "pidfile");

	if (pidfile && (fd = open_trunc(pidfile)) != -1) {
		dprintf(fd, "%d\n", getpid());
		close(fd);
	}

	while (1) {
		read_proc();
		sleep(30);
	}

	exit(EXIT_SUCCESS);
}
