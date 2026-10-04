#pragma once

/* This file intended to serve as a drop-in replacement for
 * unistd.h on Windows
 */

#include <BaseTsd.h>
#include <direct.h> // for _getcwd()
#include <io.h>
#include <stdlib.h>
#include <sys/stat.h>

/* Values for the second argument to access.
   These may be OR'd together.  */
#define R_OK 4 ///< test for read permission
#define W_OK 2 ///< test for write permission
#define F_OK 0 ///< test for existence

#define access _access
#define fileno _fileno
// read, write, and close are NOT being #defined here, because while there are
// file handle specific versions for Windows, they probably don't work for
// sockets. You need to look at your app and consider whether to call e.g.
// closesocket().

#define ssize_t SSIZE_T

#define STDIN_FILENO 0
