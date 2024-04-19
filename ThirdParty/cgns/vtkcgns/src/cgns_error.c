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
    vsprintf(cgns_error_mess,format, arg);
    va_end(arg);
    if (cgns_error_handler)
        (*cgns_error_handler)(1, cgns_error_mess);
}

CGNSDLL void cgi_warning(const char *format, ...) {
    va_list arg;
    va_start(arg, format);
    if (cgns_error_handler) {
        char warning_msg[200];
        vsprintf(warning_msg, format, arg);
        (*cgns_error_handler)(0, warning_msg);
    }
    else {
        fprintf(stdout,"*** Warning:");
        vfprintf(stdout,format,arg);
        fprintf(stdout," ***\n");
    }
    va_end(arg);
}

CGNSDLL const char *cg_get_error() {
    return cgns_error_mess;
}

CGNSDLL void cg_error_exit() {
    if (cgns_error_handler)
        (*cgns_error_handler)(-1, cgns_error_mess);
    else
        fprintf(stderr,"%s\n",cgns_error_mess);
    cgio_cleanup();
    exit(1);
}

CGNSDLL void cg_error_print() {
    fprintf(stderr,"%s\n",cgns_error_mess);
}

CGNSDLL void cg_io_error (const char *funcname)
{
    char errmsg[CGIO_MAX_ERROR_LENGTH+1];
    cgio_error_message(errmsg);
    cgi_error("%s:%s", funcname, errmsg);
}

