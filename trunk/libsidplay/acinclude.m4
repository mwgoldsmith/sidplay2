dnl -------------------------------------------------------------------------

AC_DEFUN(MY_SUBST_DEF,
[
    eval "$1=\"#define $1\""
    AC_SUBST($1)
])

AC_DEFUN(MY_SUBST_UNDEF,
[
    eval "$1=\"#undef $1\""
    AC_SUBST($1)
])

AC_DEFUN(MY_SUBST,
[
    eval "$1=$2"
    AC_SUBST($1)
])

dnl -------------------------------------------------------------------------
dnl Check whether compiler has a working ``bool'' type.
dnl Will substitute @HAVE_BOOL@ with either a def or undef line.
dnl -------------------------------------------------------------------------

AC_DEFUN(MY_CHECK_BOOL,
[
    AC_MSG_CHECKING([for bool])
    AC_CACHE_VAL(my_cv_have_bool,
    [
        AC_TRY_COMPILE(
            [],
            [bool aBool = true;],
            [my_cv_have_bool=yes],
            [my_cv_have_bool=no]
        )
    ])
    if test "$my_cv_have_bool" = yes; then
        my_cv_have_bool=yes
        AC_DEFINE(HAVE_BOOL)
    fi
    AC_MSG_RESULT($my_cv_have_bool)
])

dnl -------------------------------------------------------------------------
dnl Check whether C++ library has member ios::bin instead of ios::binary.
dnl Will substitute @HAVE_IOS_BIN@ with either a def or undef line.
dnl -------------------------------------------------------------------------

AC_DEFUN(MY_CHECK_IOS_BIN,
[
    AC_MSG_CHECKING(whether standard openmode ios::binary is available)
    AC_CACHE_VAL(my_cv_have_ios_binary,
    [
        AC_TRY_COMPILE(
            [#include <fstream.h>
             #include <iomanip.h>],
            [ifstream myTest("", ios::in|ios::binary);],
            [my_cv_have_ios_binary=yes],
            [my_cv_have_ios_binary=no]
        )
    ])
    AC_MSG_RESULT($my_cv_have_ios_binary)
    if test "$my_cv_have_ios_binary" = no; then
        AC_DEFINE(HAVE_IOS_BIN)
    fi
])

dnl -------------------------------------------------------------------------
dnl Check whether C++ library has member ios::bin instead of ios::binary.
dnl Will substitute @HAVE_IOS_OPENMODE@ with either a def or undef line.
dnl -------------------------------------------------------------------------

AC_DEFUN(MY_CHECK_IOS_OPENMODE,
[
    AC_MSG_CHECKING(whether standard type ios::openmode is available)
    AC_CACHE_VAL(my_cv_have_ios_openmode,
    [
        AC_TRY_COMPILE(
            [#include <fstream.h>
             #include <iomanip.h>],
            [ios::openmode myTest = ios::in;],
            [my_cv_have_ios_openmode=yes],
            [my_cv_have_ios_openmode=no]
        )
    ])
    AC_MSG_RESULT($my_cv_have_ios_openmode)
    if test "$my_cv_have_ios_openmode" = yes; then
        AC_DEFINE(HAVE_IOS_OPENMODE)
    fi
])

dnl -------------------------------------------------------------------------
dnl Check whether C++ environment provides the "nothrow allocator".
dnl Will substitute @HAVE_EXCEPTIONS@ if test code compiles.
dnl -------------------------------------------------------------------------

AC_DEFUN(MY_CHECK_NOTHROW,
[
    AC_MSG_CHECKING([whether nothrow allocator is available])
    AC_CACHE_VAL(my_cv_have_exceptions,
    [
        AC_TRY_COMPILE(
            [#include <new>],
            [char* buf = new(std::nothrow) char[1024];],
            [my_cv_have_exceptions=yes],
            [my_cv_have_exceptions=no]
        )
    ])
    AC_MSG_RESULT($my_cv_have_exceptions)
    if test "$my_cv_have_exceptions" = yes; then
        AC_DEFINE(HAVE_EXCEPTIONS)
    fi
])


dnl -------------------------------------------------------------------------
dnl Pass C++ compiler options to libtool which supports C only.
dnl -------------------------------------------------------------------------

AC_DEFUN(MY_PROG_LIBTOOL,
[
    save_cc=$CC
    save_cflags=$CFLAGS
    CC=$CXX
    CFLAGS=$CXXFLAGS
    AM_PROG_LIBTOOL
    CC=$save_cc
    CFLAGS=$save_cflags
])
