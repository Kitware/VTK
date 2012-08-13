/*
 Copyright 2010 University Corporation for Atmospheric
 Research/Unidata. See COPYRIGHT file for more info.

 This file defines the dimension functions.

 "$Id: nc4.c,v 1.1 2010/06/01 15:46:50 ed Exp $" 
*/

#include "ncdispatch.h"

int
nc_def_dim(int ncid, const char *name, size_t len, int *idp)
{
    NC* ncp;
    int stat = NC_check_id(ncid, &ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->def_dim(ncid,name,len,idp);
}

int
nc_inq_dimid(int ncid, const char *name, int *idp)
{
    NC* ncp;
    int stat = NC_check_id(ncid, &ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->inq_dimid(ncid,name,idp);
}

int
nc_inq_dim(int ncid, int dimid, char *name, size_t *lenp)
{
    NC* ncp;
    int stat = NC_check_id(ncid, &ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->inq_dim(ncid,dimid,name,lenp);
}

int
nc_rename_dim(int ncid, int dimid, const char *name)
{
    NC* ncp;
    int stat = NC_check_id(ncid, &ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->rename_dim(ncid,dimid,name);
}

int
nc_inq_ndims(int ncid, int *ndimsp)
{
    NC* ncp;
    int stat = NC_check_id(ncid, &ncp);
    if(stat != NC_NOERR) return stat;
    if(ndimsp == NULL) return NC_NOERR;
    return ncp->dispatch->inq(ncid,ndimsp,NULL,NULL,NULL);
}

int
nc_inq_unlimdim(int ncid, int *unlimdimidp)
{
    NC* ncp;
    int stat = NC_check_id(ncid, &ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->inq_unlimdim(ncid,unlimdimidp);
}

int
nc_inq_dimname(int ncid, int dimid, char *name)
{
    NC* ncp;
    int stat = NC_check_id(ncid, &ncp);
    if(stat != NC_NOERR) return stat;
    if(name == NULL) return NC_NOERR;
    return ncp->dispatch->inq_dim(ncid,dimid,name,NULL);
}

int
nc_inq_dimlen(int ncid, int dimid, size_t *lenp)
{
    NC* ncp;
    int stat = NC_check_id(ncid, &ncp);
    if(stat != NC_NOERR) return stat;
    if(lenp == NULL) return NC_NOERR;
    return ncp->dispatch->inq_dim(ncid,dimid,NULL,lenp);
}

