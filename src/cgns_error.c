/*-------------------------------------------------------------------------
This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from
the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not
   be misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution.
-------------------------------------------------------------------------*/
/**
 * \defgroup ErrorHandling Error Handling
 **/

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "cgnslib.h"
#include "cgns_header.h"
#include "cgns_io.h"

void (*cgns_error_handler)(int, char *) = 0;

char cgns_error_mess[200] = "no CGNS error reported";

CGNSDLL void cgi_error(const char *format, ...) {
    va_list arg;
    va_start(arg, format);
    vsnprintf(cgns_error_mess, 200, format, arg);
    va_end(arg);
    if (cgns_error_handler)
        (*cgns_error_handler)(1, cgns_error_mess);
}

CGNSDLL void cgi_warning(const char *format, ...) {
    va_list arg;
    va_start(arg, format);
    if (cgns_error_handler) {
        char warning_msg[200];
        vsnprintf(warning_msg, 200, format, arg);
        (*cgns_error_handler)(0, warning_msg);
    }
    else {
        fprintf(stdout,"*** Warning:");
        vfprintf(stdout,format,arg);
        fprintf(stdout," ***\n");
    }
    va_end(arg);
}

/**
 * \ingroup ErrorHandling
 *
 * \brief If an error occurs during the execution of a CGNS library function,
 * signified by a non-zero value of the error status variable \p ier, an error
 * message may be retrieved using the function cg_get_error().
 *
 * \return The error message
 */

const char *cg_get_error() {
    return cgns_error_mess;
}

/**
 * \ingroup ErrorHandling
 *
 * \brief Print the error message and stop the execution of the program
 *
 * \note In C, you may define a function to be called automatically in the case of a
 *       warning or error using the cg_configure() routine. The function is of the form
 *       \code void err_func(int is_error, char *errmsg) \endcode and will be called whenever an error
 *       or warning occurs. The first argument, \p is_error, will be 0 for warning messages,
 *       1 for error messages, and -1 if the program is going to terminate
 *       (i.e., a call to cg_error_exit()). The second argument is the error or warning message.
 *
 */

void cg_error_exit() {
    if (cgns_error_handler)
        (*cgns_error_handler)(-1, cgns_error_mess);
    else
        fprintf(stderr,"%s\n",cgns_error_mess);
    cgio_cleanup();
    exit(1);
}

/**
 * \ingroup ErrorHandling
 *
 * \brief Print the error message and continue execution of the program
 *
 * \note In C, you may define a function to be called automatically in the case of a
 *       warning or error using the cg_configure() routine. The function is of the form
 *       \code void err_func(int is_error, char *errmsg) \endcode and will be called whenever an error
 *       or warning occurs. The first argument, \p is_error, will be 0 for warning messages,
 *       1 for error messages, and -1 if the program is going to terminate
 *       (i.e., a call to cg_error_exit()). The second argument is the error or warning message.
 *
 */

void cg_error_print() {
    fprintf(stderr,"%s\n",cgns_error_mess);
}

CGNSDLL void cg_io_error (const char *funcname)
{
    char errmsg[CGIO_MAX_ERROR_LENGTH+1];
    cgio_error_message(errmsg);
    cgi_error("%s:%s", funcname, errmsg);
}

