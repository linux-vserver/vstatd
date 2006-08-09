# Copyright 2006 Benedikt Böhm <hollow@gentoo.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the
# Free Software Foundation, Inc.,
# 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

# Generic configuration
AC_PREREQ(2.59)
AC_INIT(vstatd, 1.0.0, hollow@gentoo.org)
AC_CONFIG_HEADER([config.h])
AC_GNU_SOURCE

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AC_CONFIG_SUBDIRS(lib/libarchive)

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
AC_CHECK_LIB(vserver, vx_clone_namespace, 
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
