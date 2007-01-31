# Copyright 2006 Benedikt Böhm <hollow@gentoo.org>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA

# Generic configuration
AC_PREREQ(2.59)
AC_INIT(vstatd, 1.0, hollow@gentoo.org)
AC_GNU_SOURCE

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

# Automake
AM_INIT_AUTOMAKE([1.9 gnu dist-bzip2])
AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for libraries.
AC_CHECK_LIB(vserver, vx_stat, 
	VSERVER_LIBS="-lvserver", AC_MSG_ERROR([libvserver >= 2.0 not found]),)

AC_SUBST(VSERVER_LIBS)

AC_PATH_PROG(CONFUSE_CONFIG, confuse-config)
if test -z "$CONFUSE_CONFIG"; then
	AC_MSG_ERROR([confuse-config not found])
fi

CONFUSE_LIBS="$($CONFUSE_CONFIG --libs)"

AC_SUBST(CONFUSE_LIBS)

AC_CHECK_LIB(rrd, rrd_create,
	RRDTOOL_LIBS="-lrrd", AC_MSG_ERROR([librrd not found]),)

AC_SUBST(RRDTOOL_LIBS)

AC_CHECK_LIB(ucid, str_check,
	LUCID_LIBS="-lucid", AC_MSG_ERROR([lucid not found]),)

AC_SUBST(LUCID_LIBS)

AC_ARG_WITH([stepping],
             AC_HELP_STRING([--with-stepping=N],
                            [use stepping of N seconds (default is 5)]),
             [ac_cv_with_stepping=$withval], [ac_cv_with_stepping=5])

AC_DEFINE_UNQUOTED(STEP, $ac_cv_with_stepping)

AC_ARG_WITH([heartbeat],
             AC_HELP_STRING([--with-heartbeat=N],
                            [use heartbeat of N seconds (default is 15)]),
             [ac_cv_with_heartbeat=$withval], [ac_cv_with_heartbeat=15])

AC_DEFINE_UNQUOTED(HEARTBEAT, $ac_cv_with_heartbeat)

AC_ARG_WITH([rows],
             AC_HELP_STRING([--with-rows=N],
                            [use N rows in the RRA (default is 360)]),
             [ac_cv_with_rows=$withval], [ac_cv_with_rows=360])

AC_DEFINE_UNQUOTED(ROWS, $ac_cv_with_rows)

AC_MSG_CHECKING([whether stepping/heartbeat/rows is sane])
if test $ac_cv_with_stepping -lt 1 -o \
		$ac_cv_with_heartbeat -lt 1 -o \
		$ac_cv_with_rows -lt 1 &>/dev/null || test $? -eq 2; then
	AC_MSG_RESULT([no])
	AC_MSG_ERROR([Given stepping/heartbeat/rows values are not a number])
fi
if test `expr \( $ac_cv_with_stepping \* $ac_cv_with_rows \) % 2` -ne 0; then
	AC_MSG_RESULT([no])
	AC_MSG_ERROR([Product of stepping and row is not an even number])
fi
if test `expr \( $ac_cv_with_stepping \* $ac_cv_with_rows \)` -lt 1800; then
	AC_MSG_RESULT([no])
	AC_MSG_ERROR([Product of stepping and row is smaller than 1800])
fi
AC_MSG_RESULT([yes])

AC_DEFINE_UNQUOTED(STEPS30M,
		`expr 1800 / \( $ac_cv_with_stepping \* $ac_cv_with_rows \)`)
AC_DEFINE_UNQUOTED(STEPS6H,
		`expr 21600 / \( $ac_cv_with_stepping \* $ac_cv_with_rows \)`)
AC_DEFINE_UNQUOTED(STEPS1D,
		`expr 86400 / \( $ac_cv_with_stepping \* $ac_cv_with_rows \)`)
AC_DEFINE_UNQUOTED(STEPS30D,
		`expr 2592000 / \( $ac_cv_with_stepping \* $ac_cv_with_rows \)`)
AC_DEFINE_UNQUOTED(STEPS1Y,
		`expr 31536000 / \( $ac_cv_with_stepping \* $ac_cv_with_rows \)`)

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE

# Compiler settings
CFLAGS="${CFLAGS} -std=gnu99 -pedantic"
CFLAGS="${CFLAGS} -Wall -Wpointer-arith -Wcast-qual -Winline"
CFLAGS="${CFLAGS} -Wredundant-decls -Wcast-align -Wno-unused-parameter"

# Site settings
PATH_CPPFLAGS="$PATH_CPPFLAGS -DSYSCONFDIR='\"\$(sysconfdir)\"'"
PATH_CPPFLAGS="$PATH_CPPFLAGS -DPKGDATADIR='\"\$(pkgdatadir)\"'"
PATH_CPPFLAGS="$PATH_CPPFLAGS -DLOCALSTATEDIR='\"\$(localstatedir)\"'"

AC_SUBST(PATH_CPPFLAGS)

# Final info page
AC_CONFIG_COMMANDS_PRE([SUMMARY="$PACKAGE_STRING configured successfully:

                       CC: $CC ($($CC --version | head -n1))
                 CPPFLAGS: '$CPPFLAGS'
                   CFLAGS: '$CFLAGS'
                    build: $build
                     host: $host
                   target: $target

                   prefix: $prefix
               sysconfdir: $sysconfdir
            localstatedir: $localstatedir
"])

# Output
AC_CONFIG_FILES([Makefile
                 src/Makefile])

AC_OUTPUT

AC_MSG_NOTICE([$SUMMARY])
