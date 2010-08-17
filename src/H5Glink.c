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
 * Created:		H5Glink.c
 *			Nov 13 2006
 *			Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:		Functions for handling links in groups.
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
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Gpkg.h"		/* Groups		  		*/
#include "H5HLprivate.h"	/* Local Heaps				*/
#include "H5Iprivate.h"         /* IDs                                  */
#include "H5Lprivate.h"		/* Links                                */
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Ppublic.h"		/* Property Lists                       */


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/


/*********************/
/* Package Variables */
/*********************/


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:	H5G_link_cmp_name_inc
 *
 * Purpose:	Callback routine for comparing two link names, in
 *              increasing alphabetic order
 *
 * Return:	An integer less than, equal to, or greater than zero if the
 *              first argument is considered to be respectively less than,
 *              equal to, or greater than the second.  If two members compare
 *              as equal, their order in the sorted array is undefined.
 *              (i.e. same as strcmp())
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Sep  5 2005
 *
 *-------------------------------------------------------------------------
 */
int
H5G_link_cmp_name_inc(const void *lnk1, const void *lnk2)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5G_link_cmp_name_inc)

    FUNC_LEAVE_NOAPI(HDstrcmp(((const H5O_link_t *)lnk1)->name, ((const H5O_link_t *)lnk2)->name))
} /* end H5G_link_cmp_name_inc() */


/*-------------------------------------------------------------------------
 * Function:	H5G_link_cmp_name_dec
 *
 * Purpose:	Callback routine for comparing two link names, in
 *              decreasing alphabetic order
 *
 * Return:	An integer less than, equal to, or greater than zero if the
 *              second argument is considered to be respectively less than,
 *              equal to, or greater than the first.  If two members compare
 *              as equal, their order in the sorted array is undefined.
 *              (i.e. opposite strcmp())
 *
 * Programmer:	Quincey Koziol
 *		koziol@ncsa.uiuc.edu
 *		Sep 25 2006
 *
 *-------------------------------------------------------------------------
 */
int
H5G_link_cmp_name_dec(const void *lnk1, const void *lnk2)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5G_link_cmp_name_dec)

    FUNC_LEAVE_NOAPI(HDstrcmp(((const H5O_link_t *)lnk2)->name, ((const H5O_link_t *)lnk1)->name))
} /* end H5G_link_cmp_name_dec() */


/*-------------------------------------------------------------------------
 * Function:	H5G_link_cmp_corder_inc
 *
 * Purpose:	Callback routine for comparing two link creation orders, in
 *              increasing order
 *
 * Return:	An integer less than, equal to, or greater than zero if the
 *              first argument is considered to be respectively less than,
 *              equal to, or greater than the second.  If two members compare
 *              as equal, their order in the sorted array is undefined.
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Nov  6 2006
 *
 *-------------------------------------------------------------------------
 */
int
H5G_link_cmp_corder_inc(const void *lnk1, const void *lnk2)
{
    int ret_value;              /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5G_link_cmp_corder_inc)

    if(((const H5O_link_t *)lnk1)->corder < ((const H5O_link_t *)lnk2)->corder)
        ret_value = -1;
    else if(((const H5O_link_t *)lnk1)->corder > ((const H5O_link_t *)lnk2)->corder)
        ret_value = 1;
    else
        ret_value = 0;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_link_cmp_corder_inc() */


/*-------------------------------------------------------------------------
 * Function:	H5G_link_cmp_corder_dec
 *
 * Purpose:	Callback routine for comparing two link creation orders, in
 *              decreasing order
 *
 * Return:	An integer less than, equal to, or greater than zero if the
 *              second argument is considered to be respectively less than,
 *              equal to, or greater than the first.  If two members compare
 *              as equal, their order in the sorted array is undefined.
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Nov  6 2006
 *
 *-------------------------------------------------------------------------
 */
int
H5G_link_cmp_corder_dec(const void *lnk1, const void *lnk2)
{
    int ret_value;              /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5G_link_cmp_corder_dec)

    if(((const H5O_link_t *)lnk1)->corder < ((const H5O_link_t *)lnk2)->corder)
        ret_value = 1;
    else if(((const H5O_link_t *)lnk1)->corder > ((const H5O_link_t *)lnk2)->corder)
        ret_value = -1;
    else
        ret_value = 0;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_link_cmp_corder_dec() */


/*-------------------------------------------------------------------------
 * Function:	H5G_ent_to_link
 *
 * Purpose:     Convert a symbol table entry to a link
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Sep 16 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_ent_to_link(H5O_link_t *lnk, const H5HL_t *heap,
    const H5G_entry_t *ent, const char *name)
{
    FUNC_ENTER_NOAPI_NOFUNC(H5G_ent_to_link)

    /* check arguments */
    HDassert(lnk);
    HDassert(heap);
    HDassert(ent);
    HDassert(name);

    /* Set (default) common info for link */
    lnk->cset = H5F_DEFAULT_CSET;
    lnk->corder = 0;
    lnk->corder_valid = FALSE;       /* Creation order not valid for this link */
    lnk->name = H5MM_xstrdup(name);
    HDassert(lnk->name);

    /* Object is a symbolic or hard link */
    if(ent->type == H5G_CACHED_SLINK) {
        const char *s;          /* Pointer to link value */

        s = (const char *)H5HL_offset_into(heap, ent->cache.slink.lval_offset);
        HDassert(s);

        /* Copy the link value */
        lnk->u.soft.name = H5MM_xstrdup(s);

        /* Set link type */
        lnk->type = H5L_TYPE_SOFT;
    } /* end if */
    else {
        /* Set address of object */
        lnk->u.hard.addr = ent->header;

        /* Set link type */
        lnk->type = H5L_TYPE_HARD;
    } /* end else */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5G_ent_to_link() */


/*-------------------------------------------------------------------------
 * Function:	H5G_link_to_info
 *
 * Purpose:	Retrieve information from a link object
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Tuesday, November  7 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_link_to_info(const H5O_link_t *lnk, H5L_info_t *info)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5G_link_to_info, FAIL)

    /* Sanity check */
    HDassert(lnk);

    /* Get information from the link */
    if(info) {
        info->cset = lnk->cset;
        info->corder = lnk->corder;
        info->corder_valid = lnk->corder_valid;
        info->type = lnk->type;

        switch(lnk->type) {
            case H5L_TYPE_HARD:
                info->u.address = lnk->u.hard.addr;
                break;

            case H5L_TYPE_SOFT:
                info->u.val_size = HDstrlen(lnk->u.soft.name) + 1; /*count the null terminator*/
                break;

            case H5L_TYPE_ERROR:
            case H5L_TYPE_EXTERNAL:
            case H5L_TYPE_MAX:
            default:
            {
                const H5L_class_t *link_class;      /* User-defined link class */

                if(lnk->type < H5L_TYPE_UD_MIN || lnk->type > H5L_TYPE_MAX)
                    HGOTO_ERROR(H5E_LINK, H5E_BADTYPE, FAIL, "unknown link class")

                /* User-defined link; call its query function to get the link udata size. */
                /* Get the link class for this type of link.  It's okay if the class
                 * isn't registered, though--we just can't give any more information
                 * about it
                 */
                link_class = H5L_find_class(lnk->type);

                if(link_class != NULL && link_class->query_func != NULL) {
                    ssize_t cb_ret;             /* Return value from UD callback */

                    /* Call the link's query routine to retrieve the user-defined link's value size */
                    /* (in case the query routine packs/unpacks the link value in some way that changes its size) */
                    if((cb_ret = (link_class->query_func)(lnk->name, lnk->u.ud.udata, lnk->u.ud.size, NULL, (size_t)0)) < 0)
                        HGOTO_ERROR(H5E_LINK, H5E_CALLBACK, FAIL, "query buffer size callback returned failure")

                    info->u.val_size = (size_t)cb_ret;
                } /* end if */
                else
                    info->u.val_size = 0;
            } /* end case */
        } /* end switch */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_link_to_info() */


/*-------------------------------------------------------------------------
 * Function:	H5G_link_to_loc
 *
 * Purpose:	Build group location from group and link object
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Monday, November 20 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_link_to_loc(const H5G_loc_t *grp_loc, const H5O_link_t *lnk,
    H5G_loc_t *obj_loc)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5G_link_to_loc, FAIL)

    /* Sanity check */
    HDassert(grp_loc);
    HDassert(lnk);
    HDassert(obj_loc);

    /*
     * Build location from the link
     */

    /* Check for unknown library-internal link */
    if(lnk->type > H5L_TYPE_BUILTIN_MAX && lnk->type < H5L_TYPE_UD_MIN)
        HGOTO_ERROR(H5E_SYM, H5E_UNSUPPORTED, FAIL, "unknown link type")

    /* Build object's group hier. location */
    if(H5G_name_set(grp_loc->path, obj_loc->path, lnk->name) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "cannot set name")

    /* Set the object location, if it's a hard link set the address also */
    obj_loc->oloc->file = grp_loc->oloc->file;
    obj_loc->oloc->holding_file = FALSE;
    if(lnk->type == H5L_TYPE_HARD)
        obj_loc->oloc->addr = lnk->u.hard.addr;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_link_to_loc() */


/*-------------------------------------------------------------------------
 * Function:	H5G_link_copy_file
 *
 * Purpose:     Copy a link and the object it points to from one file to
 *              another.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Sep 29 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_link_copy_file(H5F_t *dst_file, hid_t dxpl_id, const H5O_link_t *_src_lnk,
    const H5O_loc_t *src_oloc, H5O_link_t *dst_lnk, H5O_copy_t *cpy_info)
{
    H5O_link_t tmp_src_lnk;             /* Temporary copy of src link, when needed */
    const H5O_link_t *src_lnk = _src_lnk; /* Source link */
    hbool_t dst_lnk_init = FALSE;       /* Whether the destination link is initialized */
    hbool_t expanded_link_open = FALSE; /* Whether the target location has been opened */
    H5G_loc_t tmp_src_loc;              /* Group location holding target object */
    H5G_name_t tmp_src_path;            /* Path for target object */
    H5O_loc_t tmp_src_oloc;             /* Object location for target object */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5G_link_copy_file, FAIL)

    /* check arguments */
    HDassert(dst_file);
    HDassert(src_lnk);
    HDassert(dst_lnk);
    HDassert(cpy_info);

    /* Expand soft or external link, if requested */
    if((H5L_TYPE_SOFT == src_lnk->type && cpy_info->expand_soft_link)
            || (H5L_TYPE_EXTERNAL == src_lnk->type
            && cpy_info->expand_ext_link)) {
        H5G_loc_t   lnk_grp_loc;    /* Group location holding link */
        H5G_name_t  lnk_grp_path;   /* Path for link */
        htri_t      tar_exists;     /* Whether the target object exists */

        /* Set up group location for link */
        H5G_name_reset(&lnk_grp_path);
        lnk_grp_loc.path = &lnk_grp_path;
        lnk_grp_loc.oloc = (H5O_loc_t *)src_oloc;    /* Casting away const OK -QAK */

        /* Check if the target object exists */
        if((tar_exists = H5G_loc_exists(&lnk_grp_loc, src_lnk->name, H5P_DEFAULT,
                dxpl_id)) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, FAIL, "unable to check if target object exists")

        if(tar_exists) {
            /* Make a temporary copy of the link, so that it will not change the
             * info in the cache when we change it to a hard link */
            if(NULL == H5O_msg_copy(H5O_LINK_ID, src_lnk, &tmp_src_lnk))
                HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, FAIL, "unable to copy message")

            /* Set up group location for target object.  Let H5G_traverse expand
             * the link. */
            tmp_src_loc.path = &tmp_src_path;
            tmp_src_loc.oloc = &tmp_src_oloc;
            if(H5G_loc_reset(&tmp_src_loc) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, FAIL, "unable to reset location")

            /* Find the target object */
            if(H5G_loc_find(&lnk_grp_loc, src_lnk->name, &tmp_src_loc,
                    H5P_DEFAULT, dxpl_id) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, FAIL, "unable to find target object")
            expanded_link_open = TRUE;

            /* Convert symbolic link to hard link */
            if(tmp_src_lnk.type == H5L_TYPE_SOFT)
                tmp_src_lnk.u.soft.name =
                        (char *)H5MM_xfree(tmp_src_lnk.u.soft.name);
            else if(tmp_src_lnk.u.ud.size > 0)
                tmp_src_lnk.u.ud.udata = H5MM_xfree(tmp_src_lnk.u.ud.udata);
            tmp_src_lnk.type = H5L_TYPE_HARD;
            tmp_src_lnk.u.hard.addr = tmp_src_oloc.addr;
            src_lnk = &tmp_src_lnk;
        } /* end if */
    } /* end if */

    /* Copy src link information to dst link information */
    if(NULL == H5O_msg_copy(H5O_LINK_ID, src_lnk, dst_lnk))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, FAIL, "unable to copy message")
    dst_lnk_init = TRUE;

    /* Check if object in source group is a hard link & copy it */
    if(H5L_TYPE_HARD == src_lnk->type) {
        H5O_loc_t new_dst_oloc;     /* Copied object location in destination */

        /* Set up copied object location to fill in */
        H5O_loc_reset(&new_dst_oloc);
        new_dst_oloc.file = dst_file;

        if(!expanded_link_open) {
            /* Build temporary object location for source */
            H5O_loc_reset(&tmp_src_oloc);
            tmp_src_oloc.file = src_oloc->file;
            tmp_src_oloc.addr = src_lnk->u.hard.addr;
        } /* end if */
        HDassert(H5F_addr_defined(tmp_src_oloc.addr));

        /* Copy the shared object from source to destination */
        if(H5O_copy_header_map(&tmp_src_oloc, &new_dst_oloc, dxpl_id, cpy_info, TRUE) < 0)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, FAIL, "unable to copy object")

        /* Copy new destination object's information for eventual insertion */
        dst_lnk->u.hard.addr = new_dst_oloc.addr;
    } /* end if */

done:
    /* Check if we used a temporary src link */
    if(src_lnk != _src_lnk) {
        HDassert(src_lnk == &tmp_src_lnk);
        H5O_msg_reset(H5O_LINK_ID, &tmp_src_lnk);
    } /* end if */
    if(ret_value < 0)
        if(dst_lnk_init)
            H5O_msg_reset(H5O_LINK_ID, dst_lnk);
    /* Check if we need to free the temp source oloc */
    if(expanded_link_open)
        if(H5G_loc_free(&tmp_src_loc) < 0)
            HDONE_ERROR(H5E_OHDR, H5E_CANTFREE, FAIL, "unable to free object")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_link_copy_file() */


/*-------------------------------------------------------------------------
 * Function:	H5G_link_sort_table
 *
 * Purpose:     Sort table containing a list of links for a group
 *
 * Return:	Success:        Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *	        Nov 20, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_link_sort_table(H5G_link_table_t *ltable, H5_index_t idx_type,
    H5_iter_order_t order)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5G_link_sort_table)

    /* Sanity check */
    HDassert(ltable);

    /* Pick appropriate sorting routine */
    if(idx_type == H5_INDEX_NAME) {
        if(order == H5_ITER_INC)
            HDqsort(ltable->lnks, ltable->nlinks, sizeof(H5O_link_t), H5G_link_cmp_name_inc);
        else if(order == H5_ITER_DEC)
            HDqsort(ltable->lnks, ltable->nlinks, sizeof(H5O_link_t), H5G_link_cmp_name_dec);
        else
            HDassert(order == H5_ITER_NATIVE);
    } /* end if */
    else {
        HDassert(idx_type == H5_INDEX_CRT_ORDER);
        if(order == H5_ITER_INC)
            HDqsort(ltable->lnks, ltable->nlinks, sizeof(H5O_link_t), H5G_link_cmp_corder_inc);
        else if(order == H5_ITER_DEC)
            HDqsort(ltable->lnks, ltable->nlinks, sizeof(H5O_link_t), H5G_link_cmp_corder_dec);
        else
            HDassert(order == H5_ITER_NATIVE);
    } /* end else */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5G_link_sort_table() */


/*-------------------------------------------------------------------------
 * Function:	H5G_link_iterate_table
 *
 * Purpose:     Iterate over table containing a list of links for a group,
 *              making appropriate callbacks
 *
 * Return:	Success:        Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *	        Nov 20, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_link_iterate_table(const H5G_link_table_t *ltable, hsize_t skip,
    hsize_t *last_lnk, const H5G_lib_iterate_t op, void *op_data)
{
    size_t u;                           /* Local index variable */
    herr_t ret_value = H5_ITER_CONT;   /* Return value */

    FUNC_ENTER_NOAPI_NOERR(H5G_link_iterate_table, -)

    /* Sanity check */
    HDassert(ltable);
    HDassert(op);

    /* Skip over links, if requested */
    if(last_lnk)
        *last_lnk += skip;

    /* Iterate over link messages */
    H5_ASSIGN_OVERFLOW(/* To: */ u, /* From: */ skip, /* From: */ hsize_t, /* To: */ size_t)
    for(; u < ltable->nlinks && !ret_value; u++) {
        /* Make the callback */
        ret_value = (op)(&(ltable->lnks[u]), op_data);

        /* Increment the number of entries passed through */
        if(last_lnk)
            (*last_lnk)++;
    } /* end for */

    /* Check for callback failure and pass along return value */
    if(ret_value < 0)
        HERROR(H5E_SYM, H5E_CANTNEXT, "iteration operator failed");

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_link_iterate_table() */


/*-------------------------------------------------------------------------
 * Function:	H5G_link_release_table
 *
 * Purpose:     Release table containing a list of links for a group
 *
 * Return:	Success:        Non-negative
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *	        Sep  6, 2005
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_link_release_table(H5G_link_table_t *ltable)
{
    size_t      u;                      /* Local index variable */
    herr_t	ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5G_link_release_table)

    /* Sanity check */
    HDassert(ltable);

    /* Release link info, if any */
    if(ltable->nlinks > 0) {
        /* Free link message information */
        for(u = 0; u < ltable->nlinks; u++)
            if(H5O_msg_reset(H5O_LINK_ID, &(ltable->lnks[u])) < 0)
                HGOTO_ERROR(H5E_SYM, H5E_CANTFREE, FAIL, "unable to release link message")

        /* Free table of links */
        H5MM_xfree(ltable->lnks);
    } /* end if */
    else
        HDassert(ltable->lnks == NULL);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_link_release_table() */


/*-------------------------------------------------------------------------
 * Function:	H5G_link_name_replace
 *
 * Purpose:	Determine the type of object referred to (for hard links) or
 *              the link type (for soft links and user-defined links).
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Nov 13 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_link_name_replace(H5F_t *file, hid_t dxpl_id, H5RS_str_t *grp_full_path_r,
    const H5O_link_t *lnk)
{
    H5RS_str_t *obj_path_r = NULL;      /* Full path for link being removed */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5G_link_name_replace, FAIL)

    /* check arguments */
    HDassert(file);

    /* Search the open IDs and replace names for unlinked object */
    if(grp_full_path_r) {
        obj_path_r = H5G_build_fullpath_refstr_str(grp_full_path_r, lnk->name);
        if(H5G_name_replace(lnk, H5G_NAME_DELETE, file, obj_path_r, NULL, NULL, dxpl_id) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTDELETE, FAIL, "unable to replace name")
    } /* end if */

done:
    if(obj_path_r)
        H5RS_decr(obj_path_r);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_link_name_replace() */

