/*********************************************************************
 *   Copyright 1993, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *   $Header: /upc/share/CVS/netcdf-3/libsrc/nc3dispatch.c,v 2.1 2010/04/11 04:15:40 dmh Exp $
 *********************************************************************/

#include <stdlib.h>
#include "nc.h"
#include "dispatch.h"
#include "nc3dispatch.h"
#include "rename.h"

NC_Dispatch NC3_dispatch_table = {

"NETCDF3 ",

NC3__create_mp,
NC3__open_mp,

NC3_redef,
NC3__enddef,
NC3_sync,
NC3_abort,
NC3_close,
NC3_set_fill,
NC3_inq_base_pe,
NC3_set_base_pe,
NC3_inq_format,

NC3_inq,
NC3_inq_type,

NC3_def_dim,
NC3_inq_dimid,
NC3_inq_dim,
NC3_rename_dim,

NC3_inq_att,
NC3_inq_attid,
NC3_inq_attname,
NC3_rename_att,
NC3_del_att,
NC3_get_att,
NC3_put_att,

NC3_def_var,
NC3_inq_var,
NC3_inq_varid,
NC3_rename_var,
NC3_get_vara,
NC3_put_vara

};
