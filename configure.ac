AC_INIT(Makefile.am)
AM_INIT_AUTOMAKE(hildon-control-panel, 2.2.27)
AM_CONFIG_HEADER(config.h)

AC_ARG_ENABLE(debug, [AC_HELP_STRING([--enable-debug],[Debug (default=no)])],
                            [hildon_use_debug=yes],[hildon_use_debug=no])

AC_CANONICAL_HOST

AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)

AC_PATH_X
AC_PATH_XTRA
AC_SUBST(X_CFLAGS)

AC_HEADER_STDC

if test "x${hildon_use_debug}" = "xyes"
then
    CFLAGS="$CFLAGS -ggdb -O0 -DDEBUG"
else
    CFLAGS="$CFLAGS -O2"
fi

CFLAGS="$CFLAGS -Wall -ansi -Wmissing-prototypes -Wmissing-declarations -Werror -lcrypt" 
AC_SUBST(CFLAGS)

ALL_LINGUAS="en_GB"
AC_SUBST(ALL_LINGUAS)
AM_GLIB_GNU_GETTEXT

AC_ARG_ENABLE(maemo-tools,
	      AS_HELP_STRING([--enable-maemo-tools],[Enable mobile operator, clear and restore user data tools (default=yes)]),
	      [enable_maemo_tools=$enableval], 
              [enable_maemo_tools=yes])

AM_CONDITIONAL(USE_MAEMO_TOOLS, test "x$enable_maemo_tools" = "xyes")

if test "x$enable_maemo_tools" = "xyes"; then
    AC_DEFINE([MAEMO_TOOLS],[1],[Define to enable mobile operator, clear and restore user data tools])
fi

PKG_CHECK_MODULES(OSSOSETTINGS, 
		  [osso-af-settings >= 0.9.0],
		  [
		  localedir=`pkg-config osso-af-settings --variable=localedir`
                  hildondesktopentrydir=`pkg-config osso-af-settings --variable=desktopentrydir`
		  ],[
		  localedir="${datadir}/locale"
                  hildondesktopentrydir="${datadir}/applications"
		  ])

#PKG_CHECK_MODULES(CODELOCKUI, 
#		  [codelockui >= 1:1.5.6
#		  ])

AC_SUBST(hildondesktopentrydir)
AC_SUBST(localedir)

PKG_CHECK_MODULES(HCP_DEPS,
	[
	libosso >= 0.10.0
	hildon-1
	gconf-2.0 >= 2.6.2
	])

hildoncpdesktopentrydir=${datadir}/applications/hildon-control-panel
hildoncplibdir=${libdir}/hildon-control-panel

AC_SUBST(hildondesktopentrydir)
AC_SUBST(hildoncpdesktopentrydir)
AC_SUBST(hildoncplibdir)

AC_OUTPUT(Makefile \
	data/Makefile \
	src/Makefile \
	data/hildon-control-panel.pc \
	data/hildon-control-panel.desktop \
	data/com.nokia.controlpanel.service )

