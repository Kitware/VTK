/*********************************************************************
   Copyright 2010, UCAR/Unidata See netcdf/COPYRIGHT file for
   copying and redistribution conditions.
 *********************************************************************/

#include <ncconfig.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "nc.h"

#define ID_SHIFT (16)
#define NCFILELISTLENGTH 0x10000
/* Version one just allocates the max space (sizeof(NC*)*2^16)*/
static NC** nc_filelist = NULL;

static int numfiles = 0;

/* Common */
int
count_NCList(void)
{
    return numfiles;
}


void
free_NCList(void)
{
    if(numfiles > 0) return; /* not empty */
    if(nc_filelist != NULL) free(nc_filelist);
    nc_filelist = NULL;
}

int
add_to_NCList(NC* ncp)
{
    int i;
    int new_id;
    if(nc_filelist == NULL) {
        if (!(nc_filelist = calloc(1, sizeof(NC*)*NCFILELISTLENGTH)))
            return NC_ENOMEM;
        numfiles = 0;
    }
    new_id = 0; /* id's begin at 1 */
    for(i=1;i<0x10000;i++) {
        if(nc_filelist[i] == NULL) {new_id = i; break;}
    }
    if(new_id == 0) return NC_ENOMEM; /* no more slots */
    nc_filelist[new_id] = ncp;
    numfiles++;
    new_id = (new_id << ID_SHIFT);
    ncp->ext_ncid = new_id;
    return NC_NOERR;
}

void
del_from_NCList(NC* ncp)
{
   unsigned int ncid = ((unsigned int)ncp->ext_ncid) >> ID_SHIFT;
   if(numfiles == 0 || ncid == 0 || nc_filelist == NULL) return;
   if(nc_filelist[ncid] != ncp) return;
   nc_filelist[ncid] = NULL;
   numfiles--;

   /* If all files have been closed, release the filelist memory. */
   if (numfiles == 0)
      free_NCList();
}

NC *
find_in_NCList(int ext_ncid)
{
   NC* f = NULL;
   unsigned int ncid = ((unsigned int)ext_ncid) >> ID_SHIFT;
   if(numfiles > 0 && nc_filelist != NULL && ncid < NCFILELISTLENGTH)
        f = nc_filelist[ncid];
   return f;
}
