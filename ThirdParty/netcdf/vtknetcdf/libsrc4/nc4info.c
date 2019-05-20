/*********************************************************************
*    Copyright 2010, UCAR/Unidata
*    See netcdf/COPYRIGHT file for copying and redistribution conditions.
* ********************************************************************/

#include "config.h"
#include <stdlib.h>
#include <string.h>

#define H5Acreate_vers 1

#include <vtk_hdf5.h>
#include "netcdf.h"
#include "nc4internal.h"

#define IGNORE 0

#define HDF5_MAX_NAME 1024

#define NCHECK(expr) {if((expr)!=NC_NOERR) {goto done;}}
#define HCHECK(expr) {if((expr)<0) {ncstat = NC_EHDFERR; goto done;}}

/* Global */
struct NCPROPINFO globalpropinfo;

int
NC4_fileinfo_init(void)
{
    int stat = NC_NOERR;
    unsigned major,minor,release;

    /* Build nc properties */
    memset((void*)&globalpropinfo,0,sizeof(globalpropinfo));
    globalpropinfo.version = NCPROPS_VERSION;
    globalpropinfo.netcdfver[0] = '\0';
    globalpropinfo.hdf5ver[0] = '\0';

    stat = NC4_hdf5get_libversion(&major,&minor,&release);
    if(stat) goto done;
    snprintf(globalpropinfo.hdf5ver,sizeof(globalpropinfo.hdf5ver),
		 "%1u.%1u.%1u",major,minor,release);
    strncpy(globalpropinfo.netcdfver,PACKAGE_VERSION,sizeof(globalpropinfo.netcdfver));
done:
    return stat;
}

static int
NC4_properties_parse(struct NCPROPINFO* ncprops, const char* text)
{
    int ret = NC_NOERR;
    size_t len;
    char* p;
    char* propdata = NULL;

    ncprops->version = 0;
    ncprops->hdf5ver[0] = '\0';
    ncprops->netcdfver[0] = '\0';

    len = strlen(text);
    if(len == 0) return NC_NOERR;
    propdata = (char*)malloc(len+1);
    if(propdata == NULL) return NC_ENOMEM;
    memcpy(propdata,text,len+1);
    propdata[len] = '\0'; /* guarantee */

    /* Walk and fill in ncinfo */
    p = propdata;
    while(*p) {
      char* name = p;
      char* value = NULL;
      char* q = strchr(p,'=');
      if(q == NULL)
	    {ret = NC_EINVAL; goto done;}
      *q++ = '\0';
      value = p = q;
      q = strchr(p,NCPROPSSEP);
      if(q == NULL) q = (p+strlen(p)); else* q++ = '\0';
      p = q;
      if(value != NULL) {
	    if(strcmp(name,NCPVERSION) == 0) {
          int v = atoi(value);
          if(v < 0) v = 0;
          ncprops->version = v;
	    } else if(strcmp(name,NCPNCLIBVERSION) == 0)
          strncpy(ncprops->netcdfver,value,sizeof(ncprops->netcdfver)-1);
	    else if(strcmp(name,NCPHDF5LIBVERSION) == 0)
          strncpy(ncprops->hdf5ver,value,sizeof(ncprops->hdf5ver)-1);
	    /* else ignore */
      }
    }
    /* Guarantee null term */
    ncprops->netcdfver[sizeof(ncprops->netcdfver)-1] = '\0';
    ncprops->hdf5ver[sizeof(ncprops->hdf5ver)-1] = '\0';
done:
    if(propdata != NULL) free(propdata);
    return ret;
}

static int
NC4_get_propattr(NC_HDF5_FILE_INFO_T* h5)
{
    int ncstat = NC_NOERR;
    size_t size;
    H5T_class_t t_class;
    hid_t grp = -1;
    hid_t attid = -1;
    hid_t aspace = -1;
    hid_t atype = -1;
    hid_t ntype = -1;
    herr_t herr = 0;
    char* text = NULL;

    /* Get root group */
    grp = h5->root_grp->hdf_grpid; /* get root group */
    /* Try to extract the NCPROPS attribute */
    if(H5Aexists(grp,NCPROPS) > 0) { /* Does exist */
        attid = H5Aopen_name(grp, NCPROPS);
	herr = -1;
	aspace = H5Aget_space(attid); /* dimensions of attribute data */
        atype = H5Aget_type(attid);
	/* Verify that atype and size */
	t_class = H5Tget_class(atype);
	if(t_class != H5T_STRING) {ncstat = NC_EATTMETA; goto done;}
        size = H5Tget_size(atype);
	if(size == 0) goto done;
	text = (char*)malloc(size+1);
	if(text == NULL)
	    {ncstat = NC_ENOMEM; goto done;}
        HCHECK((ntype = H5Tget_native_type(atype, H5T_DIR_ASCEND)));
        HCHECK((H5Aread(attid, ntype, text)));
	/* Make sure its null terminated */
	text[size] = '\0';
	/* Try to parse text */
	ncstat = NC4_properties_parse(&h5->fileinfo->propattr,text);
	herr = 0;
    }
done:
    if(attid >= 0) HCHECK((H5Aclose(attid)));
    if(aspace >= 0) HCHECK((H5Sclose(aspace)));
    if(ntype >= 0) HCHECK((H5Tclose(ntype)));
    if(atype >= 0) HCHECK((H5Tclose(atype)));
    if(text != NULL) free(text);
    return ncstat;
}

int
NC4_put_propattr(NC_HDF5_FILE_INFO_T* h5)
{
    int ncstat = NC_NOERR;
    hid_t grp = -1;
    hid_t attid = -1;
    hid_t aspace = -1;
    hid_t atype = -1;
    herr_t herr = 0;
    char* text = NULL;

    /* Get root group */
    grp = h5->root_grp->hdf_grpid; /* get root group */
    /* See if the NCPROPS attribute exists */
    if(H5Aexists(grp,NCPROPS) == 0) { /* Does not exist */
      ncstat = NC4_buildpropinfo(&h5->fileinfo->propattr,&text);
      if(text == NULL || ncstat != NC_NOERR) {
        goto done;
      }
      herr = -1;
      /* Create a datatype to refer to. */
      HCHECK((atype = H5Tcopy(H5T_C_S1)));
      HCHECK((H5Tset_cset(atype, H5T_CSET_ASCII)));
      HCHECK((H5Tset_size(atype, strlen(text)+1))); /*keep nul term */
      HCHECK((aspace = H5Screate(H5S_SCALAR)));
      HCHECK((attid = H5Acreate(grp, NCPROPS, atype, aspace, H5P_DEFAULT)));
      HCHECK((H5Awrite(attid, atype, text)));
      herr = 0;
    }
 done:
    if(text != NULL) {
      free(text);
      text = NULL;
    }

    if(attid >= 0) HCHECK((H5Aclose(attid)));
    if(aspace >= 0) HCHECK((H5Sclose(aspace)));
    if(atype >= 0) HCHECK((H5Tclose(atype)));
    return ncstat;
}

int
NC4_get_fileinfo(NC_HDF5_FILE_INFO_T* h5, struct NCPROPINFO* init)
{
    int ncstat = NC_NOERR;

    /* Allocate the fileinfo in h5 */
    h5->fileinfo = (struct NCFILEINFO*)calloc(1,sizeof(struct NCFILEINFO));
    if(h5->fileinfo == NULL)
	{ncstat = NC_ENOMEM; goto done;}

    /* Get superblock version */
    NCHECK((ncstat = NC4_hdf5get_superblock(h5,&h5->fileinfo->superblockversion)));
    /* Get properties attribute if not already defined */
    if(init == NULL) {
        NCHECK((ncstat = NC4_get_propattr(h5)));
    } else { /*init != NULL*/
       h5->fileinfo->propattr = *init; /* Initialize */
    }
done:
    return ncstat;
}

int
NC4_buildpropinfo(struct NCPROPINFO* info,char** propdatap)
{
    size_t total;
    char* propdata = NULL;

    if(info == NULL || info->version == 0)  return NC_EINVAL;
    if(propdatap == NULL)
      return NC_NOERR;
    *propdatap = NULL;

    /* compute attribute length */
    total = 0;
    total += strlen(NCPVERSION);
    total += strlen("=00000000");
    if(strlen(info->netcdfver) > 0) {
        total += 1; /*|NCPROPSEP|*/
        total += strlen(NCPNCLIBVERSION);
        total += strlen("=");
        total += strlen(info->netcdfver);
    }
    if(strlen(info->hdf5ver) > 0) {
        total += 1; /*|NCPROPSEP|*/
        total += strlen(NCPHDF5LIBVERSION);
        total += strlen("=");
        total += strlen(info->hdf5ver);
    }
    propdata = (char*)malloc(total+1);
    if(propdata == NULL)
	return NC_ENOMEM;
    snprintf(propdata,total+1,
            "%s=%d|%s=%s|%s=%s",
	        NCPVERSION,info->version,
	        NCPNCLIBVERSION,info->netcdfver,
	        NCPHDF5LIBVERSION,info->hdf5ver);
    /* Force null termination */
    propdata[total] = '\0';
    *propdatap = propdata;

    return NC_NOERR;
}
