dnl $Id$

AC_INIT(src/ekg.c)
AC_PREREQ(2.50)
AC_CONFIG_HEADERS(config.h)

AC_SUBST(OBJS)
AC_SUBST(IOCTLD_OBJS)
AC_SUBST(PC_REQUIRES)

AC_SUBST(strip_ekg, "")
AC_SUBST(strip_ioctld, "")

AC_SUBST(ioctld, "")
AC_SUBST(install_ioctld, "")

AC_SUBST(ekg, "")
AC_SUBST(install_ekg, "")
AC_SUBST(clean_ekg, "")
AC_SUBST(distclean_ekg, "")
AC_SUBST(uninstall_ekg, "")
AC_SUBST(dep_ekg, "")

AC_SUBST(make_ekgwap, "")
AC_SUBST(install_ekgwap, "")

dnl
dnl  Zaczynamy testy...
dnl

AC_PROG_CC

if test "$GCC"; then 
	CFLAGS="$CFLAGS -Wall -fcommon"
dnl	echo "Whoa!, GNU CC, -Wall"
fi

AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_C_CONST

AC_PATH_PROG(GMAKE, gmake, no)

if test "x$GMAKE" = "xno"; then
	AC_PATH_PROG(MAKE, make, no)

	if test "x$MAKE" = "xno"; then
		AC_MSG_ERROR([make ])
	fi
else
	MAKE="$GMAKE"
fi

AC_CHECK_TOOL(AR, ar, no)

if test "x$AR" = "xno"; then
	AC_MSG_ERROR([ar ])
fi

AC_CHECK_TOOL(STRIP, strip, :)

dnl
dnl Wygl�da na zwi�zane z libgadu, ale jest w simlite.c...
dnl

dnl AC_NEED_STDINT_H

dnl if test "x$STDINT_H" = "x"; then
dnl 	cp stdint.h lib/libgadu-stdint.h
dnl 	STDINT_H=libgadu-stdint.h 
dnl fi

dnl SunOS
AC_CHECK_LIB(nsl, t_accept, LIBS="$LIBS -lnsl")
AC_CHECK_LIB(socket, socket, LIBS="$LIBS -lsocket")

dnl BeOS

AC_ARG_WITH(bind,
  [  --without-bind          Disable linking with libbind when found])

if test "x$with_bind" != "xno"; then
	AC_CHECK_LIB(bind, __inet_addr, LIBS="$LIBS -lbind")
fi

dnl inet_pton
AC_CHECK_FUNCS(inet_pton)

AC_ARG_ENABLE(debug, 
	[  --disable-debug         Compile without debugging support

Optional ekg Features:])

if test "x$enable_debug" = "xno"; then
	AC_MSG_WARN([--disable-debug is obsolete.])
fi

dnl if test "x$enable_debug" = "xno"; then 
dnl 	CFLAGS="$CFLAGS -DGG_DEBUG_DISABLE"
dnl 	strip_ekg="strip-ekg"
dnl 	strip_ioctld="strip-ioctld"
dnl else
dnl 	CFLAGS="$CFLAGS -g" 
dnl fi

dnl
dnl  Sprawdzamy OpenSSL
dnl

AC_CHECK_OPENSSL
  
AC_ARG_ENABLE(openssl,
	[  --enable-openssl Enable OpenSSL support])

if test "x$have_openssl" = "xyes"; then
	OBJS="$OBJS simlite.o"
	LIBS="$LIBS $OPENSSL_LIBS"
	CFLAGS="$CFLAGS $OPENSSL_INCLUDES"
fi

dnl  dirname - get rid of libgen
AC_CHECK_HEADERS(libgen.h)
AC_CHECK_FUNC(dirname, , [OBJS="$OBJS ../compat/dirname.o"])

dnl  sprawdzamy, czy system ma getopt_long
AC_CHECK_FUNC(getopt_long, , [OBJS="$OBJS ../compat/getopt.o ../compat/getopt1.o"; CFLAGS="-I../compat $CFLAGS"])

dnl  na SunOSach nie ma scandir()
AC_CHECK_FUNCS(scandir, , [OBJS="$OBJS ../compat/scandir.o"])

dnl  setenv dla pythona
AC_CHECK_FUNCS(setenv)

dnl  strlcat/strlcpy wiele system�w nie posiada
AC_CHECK_FUNCS(strlcat, , [OBJS="$OBJS ../compat/strlcat.o"; IOCTLD_OBJS="$IOCTLD_OBJS ../compat/strlcat.o"])
AC_CHECK_FUNCS(strlcpy, , [OBJS="$OBJS ../compat/strlcpy.o"; IOCTLD_OBJS="$IOCTLD_OBJS ../compat/strlcpy.o"])

dnl  utimes do sprawdzania poczty
AC_CHECK_FUNCS(utimes)

dnl  mkstemp do zapisywania token�w przy braku libjpeg
AC_CHECK_FUNCS(mkstemp)

dnl  strcoll() do sortowania zgodnie z localami
AC_CHECK_HEADERS(locale.h)
AC_CHECK_FUNCS(setlocale)
AC_CHECK_FUNCS(strcoll)

dnl  regexpy
AC_CHECK_HEADERS(regex.h)

dnl  execinfo
AC_CHECK_HEADERS(execinfo.h)
AC_CHECK_FUNCS(backtrace)

dnl  libgadu
AC_CHECK_LIB(gadu, gg_login, LIBS="$LIBS -lgadu", 
	AC_MSG_ERROR([You have to install libgadu. Please see docs/libgadu.txt])
)

AC_CHECK_FUNCS(inet_pton)

AC_ARG_ENABLE(ui-readline,
	[  --enable-ui-readline    Enable readline UI])

AC_ARG_ENABLE(ui-gtk,
	[  --enable-ui-gtk         Enable gtk UI])

AC_ARG_ENABLE(force-ncurses,
	[  --disable-force-ncurses Force use of ncurses UI])

AC_ARG_ENABLE(ui-ncurses,
	[  --disable-ui-ncurses    Disable ncurses UI])

if test "x$enable_ui_ncurses" != "xno"; then
	AC_DEFINE(WITH_UI_NCURSES, 1, [define if you want ui-ncurses])
	OBJS="$OBJS ui-ncurses.o"
	ui_ncurses_enabled=yes
	ui_ncurses_default=yes
fi

dnl 
dnl  Je�li nie chcemy ncurses, budujemy readline.
dnl
  
if test "x$enable_ui_ncurses" = "xno" -a "x$enable_ui_readline" != "xno"; then
	enable_ui_readline=yes
fi

if test "x$enable_ui_readline" = "xyes"; then
	AC_DEFINE(WITH_UI_READLINE, 1, [define if you want ui-readline])
	OBJS="$OBJS ui-readline.o"
	ui_readline_enabled=yes
	ui_ncurses_default=no
	ui_readline_default=yes
fi
  
if test "x$enable_ui_readline" = "xyes" -a "x$enable_force_ncurses" = "xyes"; then 
	AC_DEFINE(WITH_FORCE_NCURSES, 1, [define if you want ui-ncurses as default])
	ui_ncurses_default=yes
	ui_readline_default=no
fi
  
if test "x$enable_ui_ncurses" != "xno"; then 
	AC_CHECK_NCURSES

	if test "x$have_ncurses" = "x"; then
		AC_MSG_ERROR([You need ncurses and ncurses-devel packages.])
	fi

	LIBS="$LIBS $CURSES_LIBS"
	CFLAGS="$CFLAGS $CURSES_INCLUDES"
	echo "CURSES_INCLUDES=$CURSES_INCLUDES"
fi

if test "x$enable_ui_gtk" != "xno"; then
	AC_CHECK_GTK2([], [have_gtk="yes"], [have_gtk=x"no"], [])
	if test "x$have_gtk" = "xyes"; then
		AC_DEFINE(WITH_UI_GTK, 1, [define if you want ui-gtk])
		OBJS="$OBJS ui-gtk.o ui-gtk-maingui.o ui-gtk-xtext.o ui-gtk-chanview.o ui-gtk-palette.o ui-gtk-bindings.o"

		LIBS="$LIBS $GTK_LIBS"
		CFLAGS="$CFLAGS $GTK_CFLAGS"
	fi
fi

AC_ARG_WITH(termcap, 
	[  --without-termcap       Used to force ncurses for readline])

if test "x$enable_ui_readline" = "xyes"; then
	if test "x$with_termcap" = "xno"; then
		TERMCAP_LIB=""
	else
		AC_CHECK_LIB(termcap, tgetent, [TERMCAP_LIB="-ltermcap"])
		LIBS="$LIBS $TERMCAP_LIB"
	fi
		if test "x$TERMCAP_LIB" = "x" -a "x$CURSES_LIBS" = "x"; then
		AC_CHECK_NCURSES
		LIBS="$LIBS $CURSES_LIBS"
		CFLAGS="$CFLAGS $CURSES_INCLUDES"
	fi
     
	AC_CHECK_READLINE

	if test "x$READLINE_LIBS" = "x"; then
		AC_MSG_ERROR([You need both readline and readline-devel packages.])
	fi

	LIBS="$LIBS $READLINE_LIBS"
	CFLAGS="$CFLAGS $READLINE_INCLUDES"

	AC_CHECK_FUNCS([rl_set_prompt rl_filename_completion_function rl_get_screen_size rl_set_key rl_bind_key_in_map])
fi

dnl
dnl  Sprawdzamy libgsm.so, <gsm.h> i <linux/soundcard.h>
dnl
  
AC_ARG_WITH(libgsm,
	[  --without-libgsm        Compile without GSM support])

if test "x$with_libgsm" != "xno"; then
	AC_CHECK_LIB(gsm, gsm_decode,
	[ 
		AC_CHECK_HEADERS(gsm.h,
		[
			have_gsm_h=yes
		], [
			AC_CHECK_HEADERS(gsm/gsm.h,
			[
				have_gsm_h=yes
				CFLAGS="$CFLAGS -I/usr/include/gsm"
			])
		])
	])

	if test "x$have_gsm_h" = "xyes"; then
		AC_CHECK_HEADERS(linux/soundcard.h,
		[
			AC_DEFINE(HAVE_VOIP, 1, [define if you want VOIP support])
			LIBS="$LIBS -lgsm"
			OBJS="$OBJS voice.o"
		])
	fi
fi

dnl
dnl  Sprawdzamy libungif.so i <gif_lib.h>
dnl
  
AC_ARG_WITH(libungif,
	[  --without-libungif      Compile without GIF token support])

if test "x$with_libungif" != "xno"; then
	AC_CHECK_LIB(ungif, DGifSlurp,
	[ 
		AC_CHECK_HEADERS(gif_lib.h,
		[
			AC_DEFINE(HAVE_LIBUNGIF, 1, [define if you have libungif])
			LIBS="$LIBS -lungif"
			have_libungif=yes
		])
	])
fi

dnl
dnl  Sprawdzamy libjpeg.so i <jpeglib.h>
dnl
  
AC_ARG_WITH(libjpeg,
	[  --without-libjpeg       Compile without JPEG token support])

if test "x$with_libjpeg" != "xno"; then
	AC_CHECK_LIB(jpeg, jpeg_start_decompress,
	[ 
		AC_CHECK_HEADERS(jpeglib.h,
		[
			AC_DEFINE(HAVE_LIBJPEG, 1, [define if you have libjpeg])
			LIBS="$LIBS -ljpeg"
			have_libjpeg=yes
		])
	])
fi

dnl
dnl  Sprawdzamy Pythona
dnl
    
AC_CHECK_PYTHON
  
if test "x$have_python" = "xyes"; then
	OBJS="$OBJS python.o"
	LIBS="$LIBS $PYTHON_LIBS"
	CFLAGS="$CFLAGS $PYTHON_INCLUDES"
fi

dnl
dnl  Sprawdzamy libz.so, <zlib.h>
dnl
  
AC_ARG_WITH(zlib,
	[  --without-zlib          Compile without zlib (compressed logs)])

if test "x$with_zlib" != "xno"; then
	AC_CHECK_LIB(z, gzopen,
	[
		AC_CHECK_HEADERS(zlib.h,
		[
			AC_DEFINE(HAVE_ZLIB, 1, [define if you want compressed logs])
			LIBS="$LIBS -lz"
			have_zlib=yes
		])
	])
fi

dnl
dnl  Sprawdzamy, czy u�ytkownik chce RTG
dnl

AC_ARG_ENABLE(wap,
	[  --enable-wap            Compile WAP gateway])

	if test "x$enable_wap" = "xyes"; then
		if test -d ekgwap; then
			make_ekgwap="make-ekgwap"
			install_ekgwap="install-ekgwap"
			AC_DEFINE(WITH_WAP, 1, [define if you want WAP support])
		else
			AC_MSG_ERROR([You have to download RTG source code from http://www.comm.pl/~serek/ekgwap/])
	fi
fi


dnl
dnl  Sprawdzamy, czy u�ytkownik chce aspell
dnl

AC_ARG_ENABLE(aspell,
	[  --enable-aspell         Enable aspell support])

	if test "x$enable_aspell" = "xyes"; then 
	AC_CHECK_HEADERS(aspell.h,
	[
		have_aspell_includes=yes
	])
	if test "x$have_aspell_includes" = "xyes"; then
	    AC_CHECK_LIB(aspell, new_aspell_config,
	    [
		have_aspell_libs=yes
	    ])
 	    if test "x$have_aspell_libs" = "xyes"; then
		if test "x$ui_ncurses_default" = "xyes"; then    
 		    AC_DEFINE(WITH_ASPELL, 1, [define if you want aspell support])
		    LIBS="$LIBS -laspell"
		fi
	    else 
	        enable_aspell=no
	    fi
	else 
	    enable_aspell=no
	fi
fi

dnl
dnl  Sprawdzamy ioctld
dnl

AC_ARG_ENABLE(ioctld,
	[  --enable-ioctld         Compile ioctld])

if test "x$enable_ioctld" = "xyes"; then 
	AC_CHECK_HEADERS(linux/kd.h,
	[
		have_ioctld_includes=yes
	], [
		AC_CHECK_HEADERS(sys/kbio.h,
		[
			have_ioctld_includes=yes
		])
	])

	if test "x$have_ioctld_includes" = "xyes"; then
		CFLAGS="$CFLAGS -DWITH_IOCTLD -DIOCTLD_PATH=\\\"${libexecdir}/ioctld\\\""
		ioctld="ioctld"
		install_ioctld="install-ioctld"
	fi
fi

dnl
dnl  Wszystkie testy nale�y dodawa� POWY�EJ testu ioctld, poniewa� w przypadku
dnl  jego powodzenia zawarto�� CFLAGS skutecznie blokuje dzia�anie gcc
dnl  odpalanego przez ./configure
dnl

lgadu="-lgadu"
ekg="ekg"
install_ekg="install-ekg"
clean_ekg="clean-ekg"
distclean_ekg="distclean-ekg"
uninstall_ekg="uninstall-ekg"
dep_ekg="dep-ekg"

AC_CONFIG_FILES(src/Makefile)

dnl </ekg>

AC_CONFIG_FILES(Makefile examples/Makefile)

AC_OUTPUT

echo
echo "configured options:"

dnl <ekg>

if test "x$have_openssl" = "xyes"; then
	echo " - openssl: enabled"
else
	echo " - openssl: disabled"
fi

if test "x$have_ioctld_includes" = "xyes"; then
	echo " - ioctld: enabled"
else
	echo " - ioctld: disabled"
fi

if test "x$have_python" = "xyes"; then
	echo " - python: enabled"
else
	echo " - python: disabled"
fi

if test "x$have_zlib" = "xyes"; then
	echo " - zlib: enabled"
else
	echo " - zlib: disabled"
fi

if test "x$have_libungif" = "xyes"; then
	echo " - libungif: enabled"
else
	echo " - libungif: disabled"
fi

if test "x$have_libjpeg" = "xyes"; then
	echo " - libjpeg: enabled"
else
	echo " - libjpeg: disabled"
fi

if test "x$ui_readline_enabled" = "xyes"; then
	if test "x$ui_readline_default" = "xyes"; then
		echo " - ui-readline: enabled (default)"
	else
		echo " - ui-readline: enabled"
	fi
else
	echo " - ui-readline: disabled"
fi

if test "x$ui_ncurses_enabled" = "xyes"; then
	if test "x$ui_ncurses_default" = "xyes"; then
		echo " - ui-ncurses: enabled (default)"
	else
		echo " - ui-ncurses: enabled"
	fi
else
	echo " - ui-ncurses: disabled"
fi

if test "x$have_gtk" = "xyes"; then
	echo " - ui-gtk: enabled"
else
	echo " - ui-gtk: disabled"
fi

if test "x$enable_wap" = "xyes"; then
	echo " - rtg: enabled"
fi

if test "x$enable_aspell" = "xyes"; then
	if test "x$ui_ncurses_default" = "xyes"; then    
		echo " - aspell: enabled"
	else 
		echo " - aspell: wanted, but it's not avaiable without ncurses"
	fi
else
	echo " - aspell: disabled"
fi

echo

dnl </ekg>
