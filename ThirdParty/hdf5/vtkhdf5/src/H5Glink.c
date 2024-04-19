/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*-------------------------------------------------------------------------
 *
 * Created:		H5Glink.c
 *			Nov 13 2006
 *			Quincey Koziol
 *
 * Purpose:		Functions for handling links in groups.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#include "H5Gmodule.h" /* This source code file is part of the H5G module */

/***********/
/* Headers */
/***********/
#include "H5private.h"   /* Generic Functions			*/
#include "H5Eprivate.h"  /* Error handling		  	*/
#include "H5Gpkg.h"      /* Groups		  		*/
#include "H5HLprivate.h" /* Local Heaps				*/
#include "H5Iprivate.h"  /* IDs                                  */
#include "H5Lprivate.h"  /* Links                                */
#include "H5MMprivate.h" /* Memory management			*/
#include "H5Ppublic.h"   /* Property Lists                       */

#include "H5VLnative_private.h" /* Native VOL connector                     */

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

static int H5G__link_cmp_name_inc(const void *lnk1, const void *lnk2);
static int H5G__link_cmp_name_dec(const void *lnk1, const void *lnk2);
static int H5G__link_cmp_corder_inc(const void *lnk1, const void *lnk2);
static int H5G__link_cmp_corder_dec(const void *lnk1, const void *lnk2);

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
 * Function:	H5G__link_cmp_name_inc
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
 *		Sep  5 2005
 *
 *-------------------------------------------------------------------------
 */
static int
H5G__link_cmp_name_inc(const void *lnk1, const void *lnk2)
{
    FUNC_ENTER_STATIC_NOERR

    FUNC_LEAVE_NOAPI(HDstrcmp(((const H5O_link_t *)lnk1)->name, ((const H5O_link_t *)lnk2)->name))
} /* end H5G__link_cmp_name_inc() */

/*-------------------------------------------------------------------------
 * Function:	H5G__link_cmp_name_dec
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
 *		Sep 25 2006
 *
 *-------------------------------------------------------------------------
 */
static int
H5G__link_cmp_name_dec(const void *lnk1, const void *lnk2)
{
    FUNC_ENTER_STATIC_NOERR

    FUNC_LEAVE_NOAPI(HDstrcmp(((const H5O_link_t *)lnk2)->name, ((const H5O_link_t *)lnk1)->name))
} /* end H5G__link_cmp_name_dec() */

/*-------------------------------------------------------------------------
 * Function:	H5G__link_cmp_corder_inc
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
 *		Nov  6 2006
 *
 *-------------------------------------------------------------------------
 */
static int
H5G__link_cmp_corder_inc(const void *lnk1, const void *lnk2)
{
    int ret_value = -1; /* Return value */

    FUNC_ENTER_STATIC_NOERR

    if (((const H5O_link_t *)lnk1)->corder < ((const H5O_link_t *)lnk2)->corder)
        ret_value = -1;
    else if (((const H5O_link_t *)lnk1)->corder > ((const H5O_link_t *)lnk2)->corder)
        ret_value = 1;
    else
        ret_value = 0;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G__link_cmp_corder_inc() */

/*-------------------------------------------------------------------------
 * Function:	H5G__link_cmp_corder_dec
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
 *		Nov  6 2006
 *
 *-------------------------------------------------------------------------
 */
static int
H5G__link_cmp_corder_dec(const void *lnk1, const void *lnk2)
{
    int ret_value = -1; /* Return value */

    FUNC_ENTER_STATIC_NOERR

    if (((const H5O_link_t *)lnk1)->corder < ((const H5O_link_t *)lnk2)->corder)
        ret_value = 1;
    else if (((const H5O_link_t *)lnk1)->corder > ((const H5O_link_t *)lnk2)->corder)
        ret_value = -1;
    else
        ret_value = 0;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G__link_cmp_corder_dec() */

/*-------------------------------------------------------------------------
 * Function:	H5G__ent_to_link
 *
 * Purpose:     Convert a symbol table entry to a link
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Sep 16 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G__ent_to_link(H5O_link_t *lnk, const H5HL_t *heap, const H5G_entry_t *ent, const char *name)
{
    hbool_t dup_soft  = FALSE;   /* xstrdup the symbolic link name or not */
    herr_t  ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* check arguments */
    HDassert(lnk);
    HDassert(heap);
    HDassert(ent);
    HDassert(name);

    /* Set (default) common info for link */
    lnk->cset         = H5F_DEFAULT_CSET;
    lnk->corder       = 0;
    lnk->corder_valid = FALSE; /* Creation order not valid for this link */
    if ((lnk->name = H5MM_xstrdup(name)) == NULL)
        HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "unable to duplicate link name")

    /* Object is a symbolic or hard link */
    if (ent->type == H5G_CACHED_SLINK) {
        const char *s; /* Pointer to link value */

        if ((s = (const char *)H5HL_offset_into(heap, ent->cache.slink.lval_offset)) == NULL)
            HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "unable to get symbolic link name")

        /* Copy the link value */
        if ((lnk->u.soft.name = H5MM_xstrdup(s)) == NULL)
            HGOTO_ERROR(H5E_LINK, H5E_CANTGET, FAIL, "unable to duplicate symbolic link name")

        dup_soft = TRUE;

        /* Set link type */
        lnk->type = H5L_TYPE_SOFT;
    } /* end if */
    else {
        /* Set address of object */
        lnk->u.hard.addr = ent->header;

        /* Set link type */
        lnk->type = H5L_TYPE_HARD;
    } /* end else */

done:
    if (ret_value < 0) {
        if (lnk->name)
            H5MM_xfree(lnk->name);
        if (ent->type == H5G_CACHED_SLINK && dup_soft)
            H5MM_xfree(lnk->u.soft.name);
    }
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G__ent_to_link() */

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
H5G_link_to_info(const H5O_loc_t *link_loc, const H5O_link_t *lnk, H5L_info2_t *info)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(link_loc);
    HDassert(lnk);

    /* Get information from the link */
    if (info) {
        info->cset         = lnk->cset;
        info->corder       = lnk->corder;
        info->corder_valid = lnk->corder_valid;
        info->type         = lnk->type;

        switch (lnk->type) {
            case H5L_TYPE_HARD:
                /* Serialize the address into a VOL token */
                if (H5VL_native_addr_to_token(link_loc->file, H5I_FILE, lnk->u.hard.addr, &info->u.token) < 0)
                    HGOTO_ERROR(H5E_LINK, H5E_CANTSERIALIZE, FAIL,
                                "can't serialize address into object token")
                break;

            case H5L_TYPE_SOFT:
                info->u.val_size = HDstrlen(lnk->u.soft.name) + 1; /*count the null terminator*/
                break;

            case H5L_TYPE_ERROR:
            case H5L_TYPE_EXTERNAL:
            case H5L_TYPE_MAX:
            default: {
                const H5L_class_t *link_class; /* User-defined link class */

                if (lnk->type < H5L_TYPE_UD_MIN || lnk->type > H5L_TYPE_MAX)
                    HGOTO_ERROR(H5E_LINK, H5E_BADTYPE, FAIL, "unknown link class")

                /* User-defined link; call its query function to get the link udata size. */
                /* Get the link class for this type of link.  It's okay if the class
                 * isn't registered, though--we just can't give any more information
                 * about it
                 */
                link_class = H5L_find_class(lnk->type);

                if (link_class != NULL && link_class->query_func != NULL) {
                    ssize_t cb_ret; /* Return value from UD callback */

                    /* Call the link's query routine to retrieve the user-defined link's value size */
                    /* (in case the query routine packs/unpacks the link value in some way that changes its
                     * size) */
                    if ((cb_ret = (link_class->query_func)(lnk->name, lnk->u.ud.udata, lnk->u.ud.size, NULL,
                                                           (size_t)0)) < 0)
                        HGOTO_ERROR(H5E_LINK, H5E_CALLBACK, FAIL,
                                    "query buffer size callback returned failure")

                    info->u.val_size = (size_t)cb_ret;
                } /* end if */
                else
                    info->u.val_size = 0;
            } /* end case */
        }     /* end switch */
    }         /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G_link_to_info() */

/*-------------------------------------------------------------------------
 * Function:	H5G__link_to_loc
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
H5G__link_to_loc(const H5G_loc_t *grp_loc, const H5O_link_t *lnk, H5G_loc_t *obj_loc)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Sanity check */
    HDassert(grp_loc);
    HDassert(lnk);
    HDassert(obj_loc);

    /*
     * Build location from the link
     */

    /* Check for unknown library-internal link */
    if (lnk->type > H5L_TYPE_BUILTIN_MAX && lnk->type < H5L_TYPE_UD_MIN)
        HGOTO_ERROR(H5E_SYM, H5E_UNSUPPORTED, FAIL, "unknown link type")

    /* Build object's group hier. location */
    if (H5G_name_set(grp_loc->path, obj_loc->path, lnk->name) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "cannot set name")

    /* Set the object location, if it's a hard link set the address also */
    obj_loc->oloc->file         = grp_loc->oloc->file;
    obj_loc->oloc->holding_file = FALSE;
    if (lnk->type == H5L_TYPE_HARD)
        obj_loc->oloc->addr = lnk->u.hard.addr;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G__link_to_loc() */

/*-------------------------------------------------------------------------
 * Function:    H5G__link_sort_table
 *
 * Purpose:     Sort table containing a list of links for a group
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Quincey Koziol
 *              Nov 20, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G__link_sort_table(H5G_link_table_t *ltable, H5_index_t idx_type, H5_iter_order_t order)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE_NOERR

    /* Sanity check */
    HDassert(ltable);

    /* Can't sort when empty since the links table will be NULL */
    if (0 == ltable->nlinks)
        HGOTO_DONE(ret_value);

    /* This should never be NULL if the number of links is non-zero */
    HDassert(ltable->lnks);

    /* Pick appropriate sorting routine */
    if (idx_type == H5_INDEX_NAME) {
        if (order == H5_ITER_INC)
            HDqsort(ltable->lnks, ltable->nlinks, sizeof(H5O_link_t), H5G__link_cmp_name_inc);
        else if (order == H5_ITER_DEC)
            HDqsort(ltable->lnks, ltable->nlinks, sizeof(H5O_link_t), H5G__link_cmp_name_dec);
        else
            HDassert(order == H5_ITER_NATIVE);
    } /* end if */
    else {
        HDassert(idx_type == H5_INDEX_CRT_ORDER);
        if (order == H5_ITER_INC)
            HDqsort(ltable->lnks, ltable->nlinks, sizeof(H5O_link_t), H5G__link_cmp_corder_inc);
        else if (order == H5_ITER_DEC)
            HDqsort(ltable->lnks, ltable->nlinks, sizeof(H5O_link_t), H5G__link_cmp_corder_dec);
        else
            HDassert(order == H5_ITER_NATIVE);
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5G__link_sort_table() */

/*-------------------------------------------------------------------------
 * Function:	H5G__link_iterate_table
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
H5G__link_iterate_table(const H5G_link_table_t *ltable, hsize_t skip, hsize_t *last_lnk,
                        const H5G_lib_iterate_t op, void *op_data)
{
    size_t u;                        /* Local index variable */
    herr_t ret_value = H5_ITER_CONT; /* Return value */

    FUNC_ENTER_PACKAGE_NOERR

    /* Sanity check */
    HDassert(ltable);
    HDassert(op);

    /* Skip over links, if requested */
    if (last_lnk)
        *last_lnk += skip;

    /* Iterate over link messages */
    H5_CHECKED_ASSIGN(u, size_t, skip, hsize_t)
    for (; u < ltable->nlinks && !ret_value; u++) {
        /* Make the callback */
        ret_value = (op)(&(ltable->lnks[u]), op_data);

        /* Increment the number of entries passed through */
        if (last_lnk)
            (*last_lnk)++;
    } /* end for */

    /* Check for callback failure and pass along return value */
    if (ret_value < 0)
        HERROR(H5E_SYM, H5E_CANTNEXT, "iteration operator failed");

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G__link_iterate_table() */

/*-------------------------------------------------------------------------
 * Function:	H5G__link_release_table
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
H5G__link_release_table(H5G_link_table_t *ltable)
{
    size_t u;                   /* Local index variable */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Sanity check */
    HDassert(ltable);

    /* Release link info, if any */
    if (ltable->nlinks > 0) {
        /* Free link message information */
        for (u = 0; u < ltable->nlinks; u++)
            if (H5O_msg_reset(H5O_LINK_ID, &(ltable->lnks[u])) < 0)
                HGOTO_ERROR(H5E_SYM, H5E_CANTFREE, FAIL, "unable to release link message")

        /* Free table of links */
        H5MM_xfree(ltable->lnks);
    } /* end if */
    else
        HDassert(ltable->lnks == NULL);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G__link_release_table() */

/*-------------------------------------------------------------------------
 * Function:	H5G__link_name_replace
 *
 * Purpose:	Determine the type of object referred to (for hard links) or
 *              the link type (for soft links and user-defined links).
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Nov 13 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G__link_name_replace(H5F_t *file, H5RS_str_t *grp_full_path_r, const H5O_link_t *lnk)
{
    H5RS_str_t *obj_path_r = NULL;    /* Full path for link being removed */
    herr_t      ret_value  = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* check arguments */
    HDassert(file);

    /* Search the open IDs and replace names for unlinked object */
    if (grp_full_path_r) {
        obj_path_r = H5G_build_fullpath_refstr_str(grp_full_path_r, lnk->name);
        if (H5G_name_replace(lnk, H5G_NAME_DELETE, file, obj_path_r, NULL, NULL) < 0)
            HGOTO_ERROR(H5E_SYM, H5E_CANTDELETE, FAIL, "unable to replace name")
    }

done:
    if (obj_path_r)
        H5RS_decr(obj_path_r);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5G__link_name_replace() */
