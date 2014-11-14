/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*-------------------------------------------------------------------------
 *
 * Created:		H5Gname.c
 *			Sep 12 2005
 *			Quincey Koziol <koziol@ncsa.uiuc.edu>
 *
 * Purpose:		Functions for handling group hierarchy paths.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#define H5G_PACKAGE		/*suppress error about including H5Gpkg	  */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Dprivate.h"		/* Datasets				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fprivate.h"		/* File access				*/
#include "H5FLprivate.h"	/* Free Lists                           */
#include "H5Gpkg.h"		/* Groups		  		*/
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5Lprivate.h"		/* Links                                */
#include "H5MMprivate.h"	/* Memory wrappers			*/


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/

/* Struct used by change name callback function */
typedef struct H5G_names_t {
    H5G_names_op_t op;                  /* Operation performed on file */
    H5F_t       *src_file;              /* Top file in src location's mounted file hier. */
    H5RS_str_t  *src_full_path_r;       /* Source location's full path */
    H5F_t       *dst_file;              /* Destination location's file */
    H5RS_str_t  *dst_full_path_r;       /* Destination location's full path */
} H5G_names_t;

/* Info to pass to the iteration function when building name */
typedef struct H5G_gnba_iter_t {
    /* In */
    const H5O_loc_t *loc; 	/* The location of the object we're looking for */
    hid_t lapl_id; 		/* LAPL for operations */
    hid_t dxpl_id; 		/* DXPL for operations */

    /* Out */
    char *path;                 /* Name of the object */
} H5G_gnba_iter_t;

/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/

static htri_t H5G_common_path(const H5RS_str_t *fullpath_r, const H5RS_str_t *prefix_r);
static H5RS_str_t *H5G_build_fullpath(const char *prefix, const char *name);
#ifdef NOT_YET
static H5RS_str_t *H5G_build_fullpath_refstr_refstr(const H5RS_str_t *prefix_r, const H5RS_str_t *name_r);
#endif /* NOT_YET */
static herr_t H5G_name_move_path(H5RS_str_t **path_r_ptr,
    const char *full_suffix, const char *src_path, const char *dst_path);
static int H5G_name_replace_cb(void *obj_ptr, hid_t obj_id, void *key);


/*********************/
/* Package Variables */
/*********************/

/* Declare extern the PQ free list for the wrapped strings */
H5FL_BLK_EXTERN(str_buf);


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:	H5G__component
 *
 * Purpose:	Returns the pointer to the first component of the
 *		specified name by skipping leading slashes.  Returns
 *		the size in characters of the component through SIZE_P not
 *		counting leading slashes or the null terminator.
 *
 * Return:	Success:	Ptr into NAME.
 *
 *		Failure:	Ptr to the null terminator of NAME.
 *
 * Programmer:	Robb Matzke
 *		matzke@llnl.gov
 *		Aug 11 1997
 *
 *-------------------------------------------------------------------------
 */
const char *
H5G__component(const char *name, size_t *size_p)
{
    FUNC_ENTER_PACKAGE_NOERR

    HDassert(name);

    while ('/' == *name)
        name++;
    if (size_p)
        *size_p = HDstrcspn(name, "/");

    FUNC_LEAVE_NOAPI(name)
} /* end H5G__component() */


/*-------------------------------------------------------------------------
 * Function:	H5G_normalize
 *
 * Purpose:	Returns a pointer to a new string which has duplicate and
 *              trailing slashes removed from it.
 *
 * Return:	Success:	Ptr to normalized name.
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Saturday, August 16, 2003
 *
 *-------------------------------------------------------------------------
 */
char *
H5G_normalize(const char *name)
{
    char *norm;         /* Pointer to the normalized string */
    size_t	s,d;    /* Positions within the strings */
    unsigned    last_slash;     /* Flag to indicate last character was a slash */
    char *ret_value;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity check */
    HDassert(name);

    /* Duplicate the name, to return */
    if(NULL == (norm = H5MM_strdup(name)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for normalized string")

    /* Walk through the characters, omitting duplicated '/'s */
    s = d = 0;
    last_slash = 0;
    while(name[s] != '\0') {
        if(name[s] == '/')
            if(last_slash)
                ;
            else {
                norm[d++] = name[s];
                last_slash = 1;
            } /* end else */
        else {
            norm[d++] = name[s];
            last_slash = 0;
        } /* end else */
        s++;
    } /* end while */

    /* Terminate normalized string */
    norm[d] = '\0';

    /* Check for final '/' on normalized name & eliminate it */
    if(d > 1 && last_slash)
        norm[d - 1] = '\0';

    /* Set return value */
    ret_value = norm;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_normalize() */


/*-------------------------------------------------------------------------
 * Function: H5G_common_path
 *
 * Purpose: Determine if one path is a valid prefix of another path
 *
 * Return: TRUE for valid prefix, FALSE for not a valid prefix, FAIL
 *              on error
 *
 * Programmer: Quincey Koziol, koziol@ncsa.uiuc.edu
 *
 * Date: September 24, 2002
 *
 *-------------------------------------------------------------------------
 */
static htri_t
H5G_common_path(const H5RS_str_t *fullpath_r, const H5RS_str_t *prefix_r)
{
    const char *fullpath;       /* Pointer to actual fullpath string */
    const char *prefix;         /* Pointer to actual prefix string */
    size_t  nchars1,nchars2;    /* Number of characters in components */
    htri_t ret_value=FALSE;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Get component of each name */
    fullpath=H5RS_get_str(fullpath_r);
    HDassert(fullpath);
    fullpath=H5G__component(fullpath,&nchars1);
    HDassert(fullpath);
    prefix=H5RS_get_str(prefix_r);
    HDassert(prefix);
    prefix=H5G__component(prefix,&nchars2);
    HDassert(prefix);

    /* Check if we have a real string for each component */
    while(*fullpath && *prefix) {
        /* Check that the components we found are the same length */
        if(nchars1==nchars2) {
            /* Check that the two components are equal */
            if(HDstrncmp(fullpath,prefix,nchars1)==0) {
                /* Advance the pointers in the names */
                fullpath+=nchars1;
                prefix+=nchars2;

                /* Get next component of each name */
                fullpath=H5G__component(fullpath,&nchars1);
                HDassert(fullpath);
                prefix=H5G__component(prefix,&nchars2);
                HDassert(prefix);
            } /* end if */
            else
                HGOTO_DONE(FALSE)
        } /* end if */
        else
            HGOTO_DONE(FALSE)
    } /* end while */

    /* If we reached the end of the prefix path to check, it must be a valid prefix */
    if(*prefix=='\0')
        ret_value=TRUE;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_common_path() */


/*-------------------------------------------------------------------------
 * Function: H5G_build_fullpath
 *
 * Purpose: Build a full path from a prefix & base pair of strings
 *
 * Return: Pointer to reference counted string on success, NULL on error
 *
 * Programmer: Quincey Koziol, koziol@ncsa.uiuc.edu
 *
 * Date: August 19, 2005
 *
 *-------------------------------------------------------------------------
 */
static H5RS_str_t *
H5G_build_fullpath(const char *prefix, const char *name)
{
    char *full_path;            /* Full user path built */
    size_t orig_path_len;       /* Original length of the path */
    size_t path_len;            /* Length of the path */
    size_t name_len;            /* Length of the name */
    unsigned need_sep;          /* Flag to indicate if separator is needed */
    H5RS_str_t *ret_value;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity check */
    HDassert(prefix);
    HDassert(name);

    /* Get the length of the prefix */
    orig_path_len = path_len = HDstrlen(prefix);

    /* Determine if there is a trailing separator in the name */
    if(prefix[path_len - 1] == '/')
        need_sep = 0;
    else
        need_sep = 1;

    /* Add in the length needed for the '/' separator and the relative path */
    name_len = HDstrlen(name);
    path_len += name_len + need_sep;

    /* Allocate space for the path */
    if(NULL == (full_path = (char *)H5FL_BLK_MALLOC(str_buf, path_len + 1)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Build full path */
    HDstrncpy(full_path, prefix, orig_path_len + 1);
    if(need_sep)
        HDstrncat(full_path, "/", 1);
    HDstrncat(full_path, name, name_len);

    /* Create reference counted string for path */
    if(NULL == (ret_value = H5RS_own(full_path)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_build_fullpath() */


/*-------------------------------------------------------------------------
 * Function:	H5G_build_fullpath_refstr_str
 *
 * Purpose:     Append an object path to an existing ref-counted path
 *
 * Return:	Success:	Non-NULL, combined path
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol, koziol@ncsa.uiuc.edu
 *              Tuesday, October 11, 2005
 *
 *-------------------------------------------------------------------------
 */
H5RS_str_t *
H5G_build_fullpath_refstr_str(H5RS_str_t *prefix_r, const char *name)
{
    const char *prefix;         /* Pointer to raw string for path */
    H5RS_str_t *ret_value;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(prefix_r);
    HDassert(name);

    /* Get the raw string for the user path */
    prefix = H5RS_get_str(prefix_r);
    HDassert(prefix);

    /* Create reference counted string for path */
    ret_value = H5G_build_fullpath(prefix, name);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_build_fullpath_refstr_str() */

#ifdef NOT_YET

/*-------------------------------------------------------------------------
 * Function: H5G_name_build_refstr_refstr
 *
 * Purpose: Build a full path from a prefix & base pair of reference counted
 *              strings
 *
 * Return: Pointer to reference counted string on success, NULL on error
 *
 * Programmer: Quincey Koziol, koziol@ncsa.uiuc.edu
 *
 * Date: August 19, 2005
 *
 *-------------------------------------------------------------------------
 */
static H5RS_str_t *
H5G_build_fullpath_refstr_refstr(const H5RS_str_t *prefix_r, const H5RS_str_t *name_r)
{
    const char *prefix;         /* Pointer to raw string of prefix */
    const char *name;           /* Pointer to raw string of name */
    H5RS_str_t *ret_value;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Get the pointer to the prefix */
    prefix = H5RS_get_str(prefix_r);

    /* Get the pointer to the raw src user path */
    name = H5RS_get_str(name_r);

    /* Create reference counted string for path */
    ret_value = H5G_build_fullpath(prefix, name);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_build_fullpath_refstr_refstr() */
#endif /* NOT_YET */


/*-------------------------------------------------------------------------
 * Function:    H5G__name_init
 *
 * Purpose:     Set the initial path for a group hierarchy name
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, September 12, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G__name_init(H5G_name_t *name, const char *path)
{
    FUNC_ENTER_PACKAGE_NOERR

    /* Check arguments */
    HDassert(name);

    /* Set the initial paths for a name object */
    name->full_path_r = H5RS_create(path);
    HDassert(name->full_path_r);
    name->user_path_r = H5RS_create(path);
    HDassert(name->user_path_r);
    name->obj_hidden = 0;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5G__name_init() */


/*-------------------------------------------------------------------------
 * Function:	H5G_name_set
 *
 * Purpose:     Set the name of a symbol entry OBJ, located at LOC
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Pedro Vicente, pvn@ncsa.uiuc.edu
 *              Thursday, August 22, 2002
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_name_set(const H5G_name_t *loc, H5G_name_t *obj, const char *name)
{
    herr_t  ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(loc);
    HDassert(obj);
    HDassert(name);

    /* Free & reset the object's previous paths info (if they exist) */
    H5G_name_free(obj);

    /* Create the object's full path, if a full path exists in the location */
    if(loc->full_path_r) {
        /* Go build the new full path */
        if((obj->full_path_r = H5G_build_fullpath_refstr_str(loc->full_path_r, name)) == NULL)
            HGOTO_ERROR(H5E_SYM, H5E_PATH, FAIL, "can't build user path name")
    } /* end if */

    /* Create the object's user path, if a user path exists in the location */
    if(loc->user_path_r) {
        /* Go build the new user path */
        if((obj->user_path_r = H5G_build_fullpath_refstr_str(loc->user_path_r, name)) == NULL)
            HGOTO_ERROR(H5E_SYM, H5E_PATH, FAIL, "can't build user path name")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_name_set() */


/*-------------------------------------------------------------------------
 * Function:    H5G_name_copy
 *
 * Purpose:     Do a copy of group hier. names
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, September 12, 2005
 *
 * Notes:       'depth' parameter determines how much of the group entry
 *              structure we want to copy.  The depths are:
 *                  H5_COPY_SHALLOW - Copy all the fields from the source
 *                      to the destination, including the user path and
 *                      canonical path. (Destination "takes ownership" of
 *                      user and canonical paths)
 *                  H5_COPY_DEEP - Copy all the fields from the source to
 *                      the destination, deep copying the user and canonical
 *                      paths.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_name_copy(H5G_name_t *dst, const H5G_name_t *src, H5_copy_depth_t depth)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check arguments */
    HDassert(src);
    HDassert(dst);
#if defined(H5_USING_MEMCHECKER) || !defined(NDEBUG)
    HDassert(dst->full_path_r == NULL);
    HDassert(dst->user_path_r == NULL);
#endif /* H5_USING_MEMCHECKER */
    HDassert(depth == H5_COPY_SHALLOW || depth == H5_COPY_DEEP);

    /* Copy the top level information */
    HDmemcpy(dst, src, sizeof(H5G_name_t));

    /* Deep copy the names */
    if(depth == H5_COPY_DEEP) {
        dst->full_path_r = H5RS_dup(src->full_path_r);
        dst->user_path_r = H5RS_dup(src->user_path_r);
    } else {
        /* Discarding 'const' qualifier OK - QAK */
        H5G_name_reset((H5G_name_t *)src);
    } /* end if */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5G_name_copy() */


/*-------------------------------------------------------------------------
 * Function:    H5G_get_name
 *
 * Purpose:     Gets a name of an object from its ID.
 *
 * Notes:	Internal routine for H5Iget_name().

 * Return:	Success:	Non-negative, length of name
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, December 13, 2005
 *
 * Modifications: Leon Arber
 * 		  Oct. 18, 2006
 * 		  Added functionality to get the name for a reference.
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5G_get_name(const H5G_loc_t *loc, char *name/*out*/, size_t size,
    hbool_t *cached, hid_t lapl_id, hid_t dxpl_id)
{
    ssize_t len = 0;            /* Length of object's name */
    ssize_t ret_value;          /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(loc);

    /* If the user path is available and it's not "hidden", use it */
    if(loc->path->user_path_r != NULL && loc->path->obj_hidden == 0) {
        len = H5RS_len(loc->path->user_path_r);

        if(name) {
            HDstrncpy(name, H5RS_get_str(loc->path->user_path_r), MIN((size_t)(len + 1), size));
            if((size_t)len >= size)
                name[size - 1] = '\0';
        } /* end if */

        /* Indicate that the name is cached, if requested */
        /* (Currently only used for testing - QAK, 2010/07/26) */
        if(cached)
            *cached = TRUE;
    } /* end if */
    else if(!loc->path->obj_hidden) {
        hid_t	  file;

        /* Retrieve file ID for name search */
        if((file = H5F_get_id(loc->oloc->file, FALSE)) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't get file ID")

        /* Search for name of object */
        if((len = H5G_get_name_by_addr(file, lapl_id, dxpl_id, loc->oloc, name, size)) < 0) {
            H5I_dec_ref(file);
            HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't determine name")
        } /* end if */

        /* Close file ID used for search */
        if(H5I_dec_ref(file) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTCLOSEFILE, FAIL, "can't determine name")

        /* Indicate that the name is _not_ cached, if requested */
        /* (Currently only used for testing - QAK, 2010/07/26) */
        if(cached)
            *cached = FALSE;
    } /* end else */

    /* Set return value */
    ret_value = len;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_get_name() */


/*-------------------------------------------------------------------------
 * Function:	H5G_name_reset
 *
 * Purpose:	Reset a group hierarchy name to an empty state
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, September 12, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_name_reset(H5G_name_t *name)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check arguments */
    HDassert(name);

    /* Clear the group hier. name to an empty state */
    HDmemset(name, 0, sizeof(H5G_name_t));

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5G_name_reset() */


/*-------------------------------------------------------------------------
 * Function:	H5G_name_free
 *
 * Purpose:	Free the 'ID to name' buffers.
 *
 * Return:	Success
 *
 * Programmer: Pedro Vicente, pvn@ncsa.uiuc.edu
 *
 * Date: August 22, 2002
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_name_free(H5G_name_t *name)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Check args */
    HDassert(name);

    if(name->full_path_r) {
        H5RS_decr(name->full_path_r);
        name->full_path_r = NULL;
    } /* end if */
    if(name->user_path_r) {
        H5RS_decr(name->user_path_r);
        name->user_path_r = NULL;
    } /* end if */
    name->obj_hidden = 0;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5G_name_free() */


/*-------------------------------------------------------------------------
 * Function:    H5G_name_move_path
 *
 * Purpose:     Update a user or canonical path after an object moves
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, December 13, 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_name_move_path(H5RS_str_t **path_r_ptr, const char *full_suffix, const char *src_path,
    const char *dst_path)
{
    const char *path;                   /* Path to update */
    size_t path_len;                    /* Length of path */
    size_t full_suffix_len;             /* Length of full suffix */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Check arguments */
    HDassert(path_r_ptr && *path_r_ptr);
    HDassert(full_suffix);
    HDassert(src_path);
    HDassert(dst_path);

    /* Get pointer to path to update */
    path = H5RS_get_str(*path_r_ptr);
    HDassert(path);

    /* Check if path needs to be updated */
    full_suffix_len = HDstrlen(full_suffix);
    path_len = HDstrlen(path);
    if(full_suffix_len < path_len) {
        const char *dst_suffix;         /* Destination suffix that changes */
        size_t dst_suffix_len;          /* Length of destination suffix */
        const char *src_suffix;         /* Source suffix that changes */
        size_t path_prefix_len;         /* Length of path prefix */
        const char *path_prefix2;       /* 2nd prefix for path */
        size_t path_prefix2_len;        /* Length of 2nd path prefix */
        const char *common_prefix;      /* Common prefix for src & dst paths */
        size_t common_prefix_len;       /* Length of common prefix */
        char *new_path;                 /* Pointer to new path */
        size_t new_path_len;            /* Length of new path */


        /* Compute path prefix before full suffix*/
        path_prefix_len = path_len - full_suffix_len;

        /* Determine the common prefix for src & dst paths */
        common_prefix = src_path;
        common_prefix_len = 0;
        /* Find first character that is different */
        while(*(src_path + common_prefix_len) == *(dst_path + common_prefix_len))
            common_prefix_len++;
        /* Back up to previous '/' */
        while(*(common_prefix + common_prefix_len) != '/')
            common_prefix_len--;
        /* Include '/' */
        common_prefix_len++;

        /* Determine source suffix */
        src_suffix = src_path + (common_prefix_len - 1);

        /* Determine destination suffix */
        dst_suffix = dst_path + (common_prefix_len - 1);
        dst_suffix_len = HDstrlen(dst_suffix);

        /* Compute path prefix before src suffix*/
        path_prefix2 = path;
        path_prefix2_len = path_prefix_len - HDstrlen(src_suffix);

        /* Allocate space for the new path */
        new_path_len = path_prefix2_len + dst_suffix_len + full_suffix_len;
        if(NULL == (new_path = (char *)H5FL_BLK_MALLOC(str_buf, new_path_len + 1)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

        /* Create the new path */
        if(path_prefix2_len > 0) {
            HDstrncpy(new_path, path_prefix2, path_prefix2_len + 1);
            HDstrncpy(new_path + path_prefix2_len, dst_suffix, dst_suffix_len + 1);
        } /* end if */
        else
            HDstrncpy(new_path, dst_suffix, dst_suffix_len + 1);
        if(full_suffix_len > 0)
            HDstrncat(new_path, full_suffix, full_suffix_len);

        /* Release previous path */
        H5RS_decr(*path_r_ptr);

        /* Take ownership of the new full path */
        *path_r_ptr = H5RS_own(new_path);
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_name_move_path() */


/*-------------------------------------------------------------------------
 * Function: H5G_name_replace_cb
 *
 * Purpose: H5I_iterate callback function to replace group entry names
 *
 * Return: Success: 0, Failure: -1
 *
 * Programmer: Pedro Vicente, pvn@ncsa.uiuc.edu
 *
 * Date: June 5, 2002
 *
 *-------------------------------------------------------------------------
 */
static int
H5G_name_replace_cb(void *obj_ptr, hid_t obj_id, void *key)
{
    const H5G_names_t *names = (const H5G_names_t *)key;        /* Get operation's information */
    H5O_loc_t *oloc;            /* Object location for object that the ID refers to */
    H5G_name_t *obj_path;       /* Pointer to group hier. path for obj */
    H5F_t *top_obj_file;        /* Top file in object's mounted file hier. */
    hbool_t obj_in_child = FALSE;   /* Flag to indicate that the object is in the child mount hier. */
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(obj_ptr);

    /* Get the symbol table entry */
    switch(H5I_get_type(obj_id)) {
        case H5I_GROUP:
            oloc = H5G_oloc((H5G_t *)obj_ptr);
            obj_path = H5G_nameof((H5G_t *)obj_ptr);
            break;

        case H5I_DATASET:
            oloc = H5D_oloc((H5D_t *)obj_ptr);
            obj_path = H5D_nameof((H5D_t *)obj_ptr);
            break;

        case H5I_DATATYPE:
            /* Avoid non-named datatypes */
            if(!H5T_is_named((H5T_t *)obj_ptr))
                HGOTO_DONE(SUCCEED)     /* Do not exit search over IDs */

            oloc = H5T_oloc((H5T_t *)obj_ptr);
            obj_path = H5T_nameof((H5T_t *)obj_ptr);
            break;

        case H5I_UNINIT:
        case H5I_BADID:
        case H5I_FILE:
        case H5I_DATASPACE:
        case H5I_ATTR:
        case H5I_REFERENCE:
        case H5I_VFL:
        case H5I_GENPROP_CLS:
        case H5I_GENPROP_LST:
        case H5I_ERROR_CLASS:
        case H5I_ERROR_MSG:
        case H5I_ERROR_STACK:
        case H5I_NTYPES:
        default:
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "unknown data object")
    } /* end switch */
    HDassert(oloc);
    HDassert(obj_path);

    /* Check if the object has a full path still */
    if(!obj_path->full_path_r)
        HGOTO_DONE(SUCCEED)     /* No need to look at object, it's path is already invalid */

    /* Find the top file in object's mount hier. */
    if(H5F_PARENT(oloc->file)) {
        /* Check if object is in child file (for mount & unmount operations) */
        if(names->dst_file && H5F_SAME_SHARED(oloc->file, names->dst_file))
            obj_in_child = TRUE;

        /* Find the "top" file in the chain of mounted files */
        top_obj_file = H5F_PARENT(oloc->file);
        while(H5F_PARENT(top_obj_file) != NULL) {
            /* Check if object is in child mount hier. (for mount & unmount operations) */
            if(names->dst_file && H5F_SAME_SHARED(top_obj_file, names->dst_file))
                obj_in_child = TRUE;

            top_obj_file = H5F_PARENT(top_obj_file);
        } /* end while */
    } /* end if */
    else
        top_obj_file = oloc->file;

    /* Check if object is in top of child mount hier. (for mount & unmount operations) */
    if(names->dst_file && H5F_SAME_SHARED(top_obj_file, names->dst_file))
        obj_in_child = TRUE;

    /* Check if the object is in same file mount hier. */
    if(!H5F_SAME_SHARED(top_obj_file, names->src_file))
        HGOTO_DONE(SUCCEED)     /* No need to look at object, it's path is already invalid */

    switch(names->op) {
        /*-------------------------------------------------------------------------
         * H5G_NAME_MOUNT
         *-------------------------------------------------------------------------
         */
        case H5G_NAME_MOUNT:
            /* Check if object is in child mount hier. */
            if(obj_in_child) {
                const char *full_path;      /* Full path of current object */
                const char *src_path;       /* Full path of source object */
                size_t src_path_len;        /* Length of source full path */
                char *new_full_path;        /* New full path of object */
                size_t new_full_len;        /* Length of new full path */

                /* Get pointers to paths of interest */
                full_path = H5RS_get_str(obj_path->full_path_r);
                src_path = H5RS_get_str(names->src_full_path_r);
                src_path_len = HDstrlen(src_path);

                /* Build new full path */

                /* Allocate space for the new full path */
                new_full_len = src_path_len + HDstrlen(full_path);
                if(NULL == (new_full_path = (char *)H5FL_BLK_MALLOC(str_buf, new_full_len + 1)))
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

                /* Create the new full path */
                HDstrncpy(new_full_path, src_path, src_path_len + 1);
                HDstrncat(new_full_path, full_path, new_full_len);

                /* Release previous full path */
                H5RS_decr(obj_path->full_path_r);

                /* Take ownership of the new full path */
                obj_path->full_path_r = H5RS_own(new_full_path);
            } /* end if */
            /* Object must be in parent mount file hier. */
            else {
                /* Check if the source is along the entry's path */
                /* (But not actually the entry itself) */
                if(H5G_common_path(obj_path->full_path_r, names->src_full_path_r) &&
                        H5RS_cmp(obj_path->full_path_r, names->src_full_path_r)) {
                    /* Hide the user path */
                    (obj_path->obj_hidden)++;
                } /* end if */
            } /* end else */
            break;

        /*-------------------------------------------------------------------------
         * H5G_NAME_UNMOUNT
         *-------------------------------------------------------------------------
         */
        case H5G_NAME_UNMOUNT:
            if(obj_in_child) {
                const char *full_path;      /* Full path of current object */
                const char *full_suffix;    /* Full path after source path */
                size_t full_suffix_len;     /* Length of full path after source path */
                const char *src_path;       /* Full path of source object */
                char *new_full_path;        /* New full path of object */

                /* Get pointers to paths of interest */
                full_path = H5RS_get_str(obj_path->full_path_r);
                src_path = H5RS_get_str(names->src_full_path_r);

                /* Construct full path suffix */
                full_suffix = full_path + HDstrlen(src_path);
                full_suffix_len = HDstrlen(full_suffix);

                /* Build new full path */

                /* Create the new full path */
                if(NULL == (new_full_path = (char *)H5FL_BLK_MALLOC(str_buf, full_suffix_len + 1)))
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")
                HDstrncpy(new_full_path, full_suffix, full_suffix_len + 1);

                /* Release previous full path */
                H5RS_decr(obj_path->full_path_r);

                /* Take ownership of the new full path */
                obj_path->full_path_r = H5RS_own(new_full_path);

                /* Check if the object's user path should be invalidated */
                if(obj_path->user_path_r && HDstrlen(new_full_path) < (size_t)H5RS_len(obj_path->user_path_r)) {
                    /* Free user path */
                    H5RS_decr(obj_path->user_path_r);
                    obj_path->user_path_r = NULL;
                } /* end if */
            } /* end if */
            else {
                /* Check if file being unmounted was hiding the object */
                if(H5G_common_path(obj_path->full_path_r, names->src_full_path_r) &&
                        H5RS_cmp(obj_path->full_path_r, names->src_full_path_r)) {
                    /* Un-hide the user path */
                    (obj_path->obj_hidden)--;
                } /* end if */
            } /* end else */
            break;

        /*-------------------------------------------------------------------------
         * H5G_NAME_DELETE
         *-------------------------------------------------------------------------
         */
        case H5G_NAME_DELETE:
            /* Check if the location being unlinked is in the path for the current object */
            if(H5G_common_path(obj_path->full_path_r, names->src_full_path_r)) {
                /* Free paths for object */
                H5G_name_free(obj_path);
            } /* end if */
            break;

        /*-------------------------------------------------------------------------
         * H5G_NAME_MOVE
         *-------------------------------------------------------------------------
         */
        case H5G_NAME_MOVE: /* Link move case, check for relative names case */
            /* Check if the src object moved is in the current object's path */
            if(H5G_common_path(obj_path->full_path_r, names->src_full_path_r)) {
                const char *full_path;      /* Full path of current object */
                const char *full_suffix;    /* Suffix of full path, after src_path */
                size_t full_suffix_len;     /* Length of suffix of full path after src_path*/
                char *new_full_path;        /* New full path of object */
                size_t new_full_len;        /* Length of new full path */
                const char *src_path;       /* Full path of source object */
                const char *dst_path;       /* Full path of destination object */
                size_t dst_path_len;        /* Length of destination's full path */

                /* Sanity check */
                HDassert(names->dst_full_path_r);

                /* Get pointers to paths of interest */
                full_path = H5RS_get_str(obj_path->full_path_r);
                src_path = H5RS_get_str(names->src_full_path_r);
                dst_path = H5RS_get_str(names->dst_full_path_r);
                dst_path_len = HDstrlen(dst_path);

                /* Make certain that the source and destination names are full (not relative) paths */
                HDassert(*src_path == '/');
                HDassert(*dst_path == '/');

                /* Get pointer to "full suffix" */
                full_suffix = full_path + HDstrlen(src_path);
                full_suffix_len = HDstrlen(full_suffix);

                /* Update the user path, if one exists */
                if(obj_path->user_path_r)
                    if(H5G_name_move_path(&(obj_path->user_path_r), full_suffix, src_path, dst_path) < 0)
                        HGOTO_ERROR(H5E_SYM, H5E_PATH, FAIL, "can't build user path name")

                /* Build new full path */

                /* Allocate space for the new full path */
                new_full_len = dst_path_len + full_suffix_len;
                if(NULL == (new_full_path = (char *)H5FL_BLK_MALLOC(str_buf, new_full_len + 1)))
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

                /* Create the new full path */
                HDstrncpy(new_full_path, dst_path, dst_path_len + 1);
                HDstrncat(new_full_path, full_suffix, full_suffix_len);

                /* Release previous full path */
                H5RS_decr(obj_path->full_path_r);

                /* Take ownership of the new full path */
                obj_path->full_path_r = H5RS_own(new_full_path);
            } /* end if */
            break;

        default:
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid operation")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5G_name_replace_cb() */


/*-------------------------------------------------------------------------
 * Function: H5G_name_replace
 *
 * Purpose: Search the list of open IDs and replace names according to a
 *              particular operation.  The operation occured on the
 *              SRC_FILE/SRC_FULL_PATH_R object.  The new name (if there is
 *              one) is NEW_NAME_R.  Additional entry location information
 *              (currently only needed for the 'move' operation) is passed in
 *              DST_FILE/DST_FULL_PATH_R.
 *
 * Return: Success: 0, Failure: -1
 *
 * Programmer: Pedro Vicente, pvn@ncsa.uiuc.edu
 *
 * Date: June 11, 2002
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_name_replace(const H5O_link_t *lnk, H5G_names_op_t op, H5F_t *src_file,
    H5RS_str_t *src_full_path_r, H5F_t *dst_file, H5RS_str_t *dst_full_path_r,
    hid_t dxpl_id)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(FAIL)

    /* Check arguments */
    HDassert(src_file);

    /* Check if the object we are manipulating has a path */
    if(src_full_path_r) {
        hbool_t search_group = FALSE;  /* Flag to indicate that groups are to be searched */
        hbool_t search_dataset = FALSE;  /* Flag to indicate that datasets are to be searched */
        hbool_t search_datatype = FALSE; /* Flag to indicate that datatypes are to be searched */

        /* Check for particular link to operate on */
        if(lnk) {
            /* Look up the object type for each type of link */
            switch(lnk->type) {
                case H5L_TYPE_HARD:
                    {
                        H5O_loc_t tmp_oloc;             /* Temporary object location */
                        H5O_type_t obj_type;            /* Type of object at location */

                        /* Build temporary object location */
                        tmp_oloc.file = src_file;
                        tmp_oloc.addr = lnk->u.hard.addr;

                        /* Get the type of the object */
                        if(H5O_obj_type(&tmp_oloc, &obj_type, dxpl_id) < 0)
                            HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't get object type")

                        /* Determine which type of objects to operate on */
                        switch(obj_type) {
                            case H5O_TYPE_GROUP:
                                /* Search and replace names through group IDs */
                                search_group = TRUE;
                                break;

                            case H5O_TYPE_DATASET:
                                /* Search and replace names through dataset IDs */
                                search_dataset = TRUE;
                                break;

                            case H5O_TYPE_NAMED_DATATYPE:
                                /* Search and replace names through datatype IDs */
                                search_datatype = TRUE;
                                break;

                            case H5O_TYPE_UNKNOWN:
                            case H5O_TYPE_NTYPES:
                                /* Search and replace names through datatype IDs */
                            default:
                                HGOTO_ERROR(H5E_SYM, H5E_BADTYPE, FAIL, "not valid object type")
                        } /* end switch */
                    } /* end case */
                    break;

                case H5L_TYPE_SOFT:
                    /* Symbolic links might resolve to any object, so we need to search all IDs */
                    search_group = search_dataset = search_datatype = TRUE;
                    break;

                case H5L_TYPE_ERROR:
                case H5L_TYPE_EXTERNAL:
                case H5L_TYPE_MAX:
                default:  /* User-defined link */
                    /* Check for unknown library-defined link type */
                    if(lnk->type < H5L_TYPE_UD_MIN)
                       HGOTO_ERROR(H5E_SYM, H5E_BADVALUE, FAIL, "unknown link type")

                    /* User-defined & external links automatically wipe out
                     * names (because it would be too much work to track them),
                     * so there's no point in searching them.
                     */
                    break;
            } /* end switch */
        } /* end if */
        else {
            /* We pass NULL as link pointer when we need to search all IDs */
            search_group = search_dataset = search_datatype = TRUE;
        } /* end else */

        /* Check if we need to operate on the objects affected */
        if(search_group || search_dataset || search_datatype) {
            H5G_names_t names;          /* Structure to hold operation information for callback */

            /* Find top file in src location's mount hierarchy */
            while(H5F_PARENT(src_file))
                src_file = H5F_PARENT(src_file);

            /* Set up common information for callback */
            names.src_file = src_file;
            names.src_full_path_r = src_full_path_r;
            names.dst_file = dst_file;
            names.dst_full_path_r = dst_full_path_r;
            names.op = op;

            /* Search through group IDs */
            if(search_group)
                if(H5I_iterate(H5I_GROUP, H5G_name_replace_cb, &names, FALSE) < 0)
		    HGOTO_ERROR(H5E_SYM, H5E_BADITER, FAIL, "can't iterate over groups")

            /* Search through dataset IDs */
            if(search_dataset)
                if(H5I_iterate(H5I_DATASET, H5G_name_replace_cb, &names, FALSE) < 0)
		    HGOTO_ERROR(H5E_SYM, H5E_BADITER, FAIL, "can't iterate over datasets")

            /* Search through datatype IDs */
            if(search_datatype)
                if(H5I_iterate(H5I_DATATYPE, H5G_name_replace_cb, &names, FALSE) < 0)
		    HGOTO_ERROR(H5E_SYM, H5E_BADITER, FAIL, "can't iterate over datatypes")
        } /* end if */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_name_replace() */


/*-------------------------------------------------------------------------
 * Function:    H5G_get_name_by_addr_cb
 *
 * Purpose:     Callback for retrieving object's name by address
 *
 * Return:      Positive if path is for object desired
 * 		0 if not correct object
 * 		negative on failure.
 *
 * Programmer:	Quincey Koziol
 *		November 4 2007
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_get_name_by_addr_cb(hid_t gid, const char *path, const H5L_info_t *linfo,
    void *_udata)
{
    H5G_gnba_iter_t *udata = (H5G_gnba_iter_t *)_udata; /* User data for iteration */
    H5G_loc_t   obj_loc;                /* Location of object */
    H5G_name_t  obj_path;            	/* Object's group hier. path */
    H5O_loc_t   obj_oloc;            	/* Object's object location */
    hbool_t     obj_found = FALSE;      /* Object at 'path' found */
    herr_t ret_value = H5_ITER_CONT;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity check */
    HDassert(path);
    HDassert(linfo);
    HDassert(udata->loc);
    HDassert(udata->path == NULL);

    /* Check for hard link with correct address */
    if(linfo->type == H5L_TYPE_HARD && udata->loc->addr == linfo->u.address) {
        H5G_loc_t	grp_loc;                /* Location of group */

        /* Get group's location */
        if(H5G_loc(gid, &grp_loc) < 0)
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5_ITER_ERROR, "bad group location")

        /* Set up opened object location to fill in */
        obj_loc.oloc = &obj_oloc;
        obj_loc.path = &obj_path;
        H5G_loc_reset(&obj_loc);

        /* Find the object */
        if(H5G_loc_find(&grp_loc, path, &obj_loc/*out*/, udata->lapl_id, udata->dxpl_id) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, H5_ITER_ERROR, "object not found")
        obj_found = TRUE;

        /* Check for object in same file (handles mounted files) */
        /* (re-verify address, in case we traversed a file mount) */
        if(udata->loc->addr == obj_loc.oloc->addr && udata->loc->file == obj_loc.oloc->file) {
            if(NULL == (udata->path = H5MM_strdup(path)))
                HGOTO_ERROR(H5E_SYM, H5E_CANTALLOC, H5_ITER_ERROR, "can't duplicate path string")

            /* We found a match so we return immediately */
            HGOTO_DONE(H5_ITER_STOP)
        } /* end if */
    } /* end if */

done:
    if(obj_found && H5G_loc_free(&obj_loc) < 0)
        HDONE_ERROR(H5E_SYM, H5E_CANTRELEASE, H5_ITER_ERROR, "can't free location")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_get_name_by_addr_cb() */


/*-------------------------------------------------------------------------
 * Function:    H5G_get_name_by_addr
 *
 * Purpose:     Tries to figure out the path to an object from it's address
 *
 * Return:      returns size of path name, and copies it into buffer
 * 		pointed to by name if that buffer is big enough.
 * 		0 if it cannot find the path
 * 		negative on failure.
 *
 * Programmer:	Quincey Koziol
 *		November 4 2007
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5G_get_name_by_addr(hid_t file, hid_t lapl_id, hid_t dxpl_id, const H5O_loc_t *loc,
    char *name, size_t size)
{
    H5G_gnba_iter_t udata;      /* User data for iteration */
    H5G_loc_t root_loc;         /* Root group's location */
    hbool_t found_obj = FALSE;  /* If we found the object */
    herr_t status;              /* Status from iteration */
    ssize_t ret_value;          /* Return value */

    /* Portably clear udata struct (before FUNC_ENTER) */
    HDmemset(&udata, 0, sizeof(udata));

    FUNC_ENTER_NOAPI(FAIL)

    /* Construct the link info for the file's root group */
    if(H5G_loc(file, &root_loc) < 0)
	HGOTO_ERROR(H5E_SYM, H5E_CANTGET, FAIL, "can't get root group's location")

    /* Check for root group being the object looked for */
    if(root_loc.oloc->addr == loc->addr && root_loc.oloc->file == loc->file) {
        if(NULL == (udata.path = H5MM_strdup("")))
            HGOTO_ERROR(H5E_SYM, H5E_CANTALLOC, FAIL, "can't duplicate path string")
        found_obj = TRUE;
    } /* end if */
    else {
        /* Set up user data for iterator */
        udata.loc = loc;
        udata.lapl_id = lapl_id;
        udata.dxpl_id = dxpl_id;
        udata.path = NULL;

        /* Visit all the links in the file */
        if((status = H5G_visit(file, "/", H5_INDEX_NAME, H5_ITER_NATIVE, H5G_get_name_by_addr_cb, &udata, lapl_id, dxpl_id)) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_BADITER, FAIL, "group traversal failed while looking for object name")
        else if(status > 0)
            found_obj = TRUE;
    } /* end else */

    /* Check for finding the object */
    if(found_obj) {
        /* Set the length of the full path */
        ret_value = (ssize_t)(HDstrlen(udata.path) + 1);        /* Length of path + 1 (for "/") */

        /* If there's a buffer provided, copy into it, up to the limit of its size */
        if(name) {
            /* Copy the initial path separator */
            HDstrncpy(name, "/", 2);

            /* Append the rest of the path */
            /* (less one character, for the initial path separator) */
            HDstrncat(name, udata.path, (size - 2));
            if((size_t)ret_value >= size)
                name[size - 1] = '\0';
        } /* end if */
    } /* end if */
    else
        ret_value = 0;

done:
    /* Release resources */
    H5MM_xfree(udata.path);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_get_name_by_addr() */

