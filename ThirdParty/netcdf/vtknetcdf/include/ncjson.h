/* Copyright 2018, UCAR/Unidata.
Copyright 2018 Unidata

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef NCJSON_H
#define NCJSON_H 1

#include "vtk_netcdf_mangle.h"

/* Json object sorts (note use of term sort rather than e.g. type or discriminant) */
#define NCJ_UNDEF    0
#define NCJ_STRING   1
#define NCJ_INT      2
#define NCJ_DOUBLE   3
#define NCJ_BOOLEAN  4
#define NCJ_DICT     5
#define NCJ_ARRAY    6
#define NCJ_NULL     7

#define NCJ_NSORTS   8

/* No flags are currently defined, but the argument is a placeholder */


/* Define a struct to store primitive values
   as unquoted strings. The sort will 
   provide more info.
   Do not bother with a union since
   the amount of saved space is minimal.
*/

typedef struct NCjson {
    int sort;     /* of this object */
    char* string; /* sort != DICT|ARRAY */
    struct NCjlist {
	    int len;
	    struct NCjson** contents;
    } list; /* sort == DICT|ARRAY */
} NCjson;

/* Support Windows declspec */  
#ifndef EXTERNL
#  ifdef _WIN32
#    ifdef NCJSON_INTERNAL /* define when compiling code */
#      define EXTERNL __declspec(dllexport) extern
#    else
#      define EXTERNL __declspec(dllimport) extern
#    endif
#  else /* !_WIN32 */
#    define EXTERNL extern
#  endif
#endif /* !defined EXTERNL */

#if defined(__cplusplus)
extern "C" {
#endif

/* int return value is either 1 (ok) or 0 (failure) */

/* Parse */
EXTERNL int NCJparse(const char* text, unsigned flags, NCjson** jsonp);

/* Build */
EXTERNL int NCJnew(int sort, NCjson** object);

/* Recursively free NCjson instance */
EXTERNL void NCJreclaim(NCjson*);

/* Assign a nul terminated string value to an NCjson object as its contents */
EXTERNL int NCJnewstring(int sort, const char* value, NCjson** jsonp);

/* Assign a counted string value to an NCjson object as its contents */
EXTERNL int NCJnewstringn(int sort, size_t len, const char* value, NCjson** jsonp);

/* Append value to an array or dict object. */
EXTERNL int NCJappend(NCjson* object, NCjson* value);

/* Insert key-value pair into a dict object. key will be copied */
EXTERNL int NCJinsert(NCjson* object, char* key, NCjson* value);

/* Unparser to convert NCjson object to text in buffer */
EXTERNL int NCJunparse(const NCjson* json, unsigned flags, char** textp);

/* Utilities */
EXTERNL int NCJaddstring(NCjson*, int sort, const char* s);
EXTERNL int NCJdictget(const NCjson* dict, const char* key, NCjson** valuep);

/* dump NCjson* object to output file */
EXTERNL void NCJdump(const NCjson* json, unsigned flags, FILE*);

/* Convert one json sort to  value of another type; don't use union so we can know when to reclaim sval */
struct NCJconst {int bval; long long ival; double dval; char* sval;};
EXTERNL int NCJcvt(const NCjson* value, int outsort, struct NCJconst* output);

/* Deep clone a json object */
EXTERNL int NCJclone(const NCjson* json, NCjson** clonep);

/* Getters */
#define NCJsort(x) ((x)->sort)
#define NCJstring(x) ((x)->string)
#define NCJlength(x) ((x)==NULL ? 0 : (x)->list.len)
#define NCJcontents(x) ((x)->list.contents)
#define NCJith(x,i) ((x)->list.contents[i])

/* Setters */
#define NCJsetsort(x,s) (x)->sort=(s)
#define NCJsetstring(x,y) (x)->string=(y)
#define NCJsetcontents(x,c) (x)->list.contents=(c)
#define NCJsetlength(x,l) (x)->list.len=(l)

/* Misc */
#define NCJisatomic(j) ((j)->sort != NCJ_ARRAY && (j)->sort != NCJ_DICT && (j)->sort != NCJ_NULL && (j)->sort != NCJ_UNDEF)

#if defined(__cplusplus)
}
#endif

#endif /*NCJSON_H*/
