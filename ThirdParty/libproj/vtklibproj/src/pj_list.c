/* Projection System: default list of projections
** Use local definition of PJ_LIST_H for subset.
*/
#define USE_PJ_LIST_H 1
#include "projects.h"

struct PJ_LIST  *pj_get_list_ref()

{
    return pj_list;
}
