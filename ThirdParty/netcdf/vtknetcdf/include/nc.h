/*
 *	Copyright 2018, University Corporation for Atmospheric Research
 *      See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */
#ifndef _NC_H_
#define _NC_H_

#include "config.h"
#include "netcdf.h"

   /* There's an external ncid (ext_ncid) and an internal ncid
    * (int_ncid). The ext_ncid is the ncid returned to the user. If
    * the user has opened or created a netcdf-4 file, then the
    * ext_ncid is the same as the int_ncid. If he has opened or
    * created a netcdf-3 file ext_ncid (which the user sees) is
    * different from the int_ncid, which is the ncid returned by the
    * netcdf-3 layer, which insists on inventing its own ncids,
    * regardless of what is already in use due to previously opened
    * netcdf-4 files. The ext_ncid contains the ncid for the root
    * group (i.e. group zero). */

/* Common Shared Structure for all Dispatched Objects */
typedef struct NC {
	int ext_ncid;
	int int_ncid;
	const struct NC_Dispatch* dispatch;
	void* dispatchdata; /*per-'file' data; points to e.g. NC3_INFO data*/
	char* path;
	int   mode; /* as provided to nc_open/nc_create */
} NC;

/*
 * Counted string for names and such
 */
typedef struct {
	/* all xdr'd */
	size_t nchars;
	char *cp;
} NC_string;

/* Define functions that are used across multiple dispatchers */

/* Begin defined in string.c */
extern void
free_NC_string(NC_string *ncstrp);

extern int
NC_check_name(const char *name);

extern NC_string *
new_NC_string(size_t slen, const char *str);
extern int
set_NC_string(NC_string *ncstrp, const char *str);

/* End defined in string.c */

extern int
NC_check_id(int ncid, NC **ncpp);

/* Create a pseudo file descriptor that does not
   overlap real file descriptors */
extern int nc__pseudofd(void);

/* This function gets a current default create flag */
extern int nc_get_default_format(void);

extern int add_to_NCList(NC*);
extern void del_from_NCList(NC*);/* does not free object */
extern NC* find_in_NCList(int ext_ncid);
extern NC* find_in_NCList_by_name(const char*);
extern int move_in_NCList(NC *ncp, int new_id);
extern void free_NCList(void);/* reclaim whole list */
extern int count_NCList(void); /* return # of entries in NClist */
extern int iterate_NCList(int i,NC**); /* Walk from 0 ...; ERANGE return => stop */

/* Defined in nc.c */
extern void free_NC(NC*);
extern int new_NC(const struct NC_Dispatch*, const char*, int, NC**);

#endif /* _NC_H_ */
