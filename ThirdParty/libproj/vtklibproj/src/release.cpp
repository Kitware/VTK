/* <<< Release Notice for library >>> */

#include "proj.h"
#include "proj_internal.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

char const pj_release[] =
    "Rel. "
    STR(PROJ_VERSION_MAJOR)"."
    STR(PROJ_VERSION_MINOR)"."
    STR(PROJ_VERSION_PATCH)", "
    "July 1st, 2021";

const char *pj_get_release() {
    return pj_release;
}
