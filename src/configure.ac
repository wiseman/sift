#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(FULL-PACKAGE-NAME, VERSION, BUG-REPORT-ADDRESS)
AM_INIT_AUTOMAKE(sift, 1.0)
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])

AC_PROG_LIBTOOL

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lsift':
AC_CHECK_LIB([sift], [main])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([limits.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STAT
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([memset pow sqrt strerror strrchr strtol])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
