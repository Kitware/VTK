/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*-------------------------------------------------------------------------
 *
 * Created:             H5AC.c
 *                      Jul  9 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             Functions in this file implement a cache for
 *                      things which exist on disk.  All "things" associated
 *                      with a particular HDF file share the same cache; each
 *                      HDF file has it's own cache.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#include "H5ACmodule.h"         /* This source code file is part of the H5AC module */
#define H5C_FRIEND              /* Suppress error about including H5Cpkg            */
#define H5F_FRIEND              /* Suppress error about including H5Fpkg            */


/***********/
/* Headers */
/***********/
#include "H5private.h"          /* Generic Functions                        */
#include "H5ACpkg.h"            /* Metadata cache                           */
#include "H5Clog.h"             /* Cache logging                            */
#include "H5Cpkg.h"             /* Cache                                    */
#include "H5CXprivate.h"        /* API Contexts                             */
#include "H5Eprivate.h"         /* Error handling                           */
#include "H5Fpkg.h"             /* Files                                    */
#include "H5FDprivate.h"        /* File drivers                             */
#include "H5Iprivate.h"         /* IDs                                      */
#include "H5Pprivate.h"         /* Property lists                           */
#include "H5SLprivate.h"        /* Skip Lists                               */


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Local Prototypes */
/********************/

static herr_t H5AC__check_if_write_permitted(const H5F_t *f,
    hbool_t *write_permitted_ptr);
static herr_t H5AC__ext_config_2_int_config(H5AC_cache_config_t *ext_conf_ptr,
    H5C_auto_size_ctl_t *int_conf_ptr);
#if H5AC_DO_TAGGING_SANITY_CHECKS
static herr_t H5AC__verify_tag(const H5AC_class_t * type);
#endif /* H5AC_DO_TAGGING_SANITY_CHECKS */


/*********************/
/* Package Variables */
/*********************/

/* Package initialization variable */
hbool_t H5_PKG_INIT_VAR = FALSE;

/*****************************/
/* Library Private Variables */
/*****************************/

#ifdef H5_HAVE_PARALLEL
/* Environment variable for collective API sanity checks */
hbool_t H5_coll_api_sanity_check_g = false;
#endif /* H5_HAVE_PARALLEL */


/*******************/
/* Local Variables */
/*******************/

/* Metadata entry class list */

/* Remember to add new type ID to the H5AC_type_t enum in H5ACprivate.h when
 *      adding a new class.
 */

static const H5AC_class_t *const H5AC_class_s[] = {
    H5AC_BT,                    /* ( 0) B-tree nodes                    */
    H5AC_SNODE,                 /* ( 1) symbol table nodes              */
    H5AC_LHEAP_PRFX,            /* ( 2) local heap prefix               */
    H5AC_LHEAP_DBLK,            /* ( 3) local heap data block           */
    H5AC_GHEAP,                 /* ( 4) global heap                     */
    H5AC_OHDR,                  /* ( 5) object header                   */
    H5AC_OHDR_CHK,              /* ( 6) object header chunk             */
    H5AC_BT2_HDR,               /* ( 7) v2 B-tree header                */
    H5AC_BT2_INT,               /* ( 8) v2 B-tree internal node         */
    H5AC_BT2_LEAF,              /* ( 9) v2 B-tree leaf node             */
    H5AC_FHEAP_HDR,             /* (10) fractal heap header             */
    H5AC_FHEAP_DBLOCK,          /* (11) fractal heap direct block       */
    H5AC_FHEAP_IBLOCK,          /* (12) fractal heap indirect block     */
    H5AC_FSPACE_HDR,            /* (13) free space header               */
    H5AC_FSPACE_SINFO,          /* (14) free space sections             */
    H5AC_SOHM_TABLE,            /* (15) shared object header message master table */
    H5AC_SOHM_LIST,             /* (16) shared message index stored as a list */
    H5AC_EARRAY_HDR,            /* (17) extensible array header         */
    H5AC_EARRAY_IBLOCK,         /* (18) extensible array index block    */
    H5AC_EARRAY_SBLOCK,         /* (19) extensible array super block    */
    H5AC_EARRAY_DBLOCK,         /* (20) extensible array data block     */
    H5AC_EARRAY_DBLK_PAGE,      /* (21) extensible array data block page */
    H5AC_FARRAY_HDR,            /* (22) fixed array header              */
    H5AC_FARRAY_DBLOCK,         /* (23) fixed array data block          */
    H5AC_FARRAY_DBLK_PAGE,      /* (24) fixed array data block page     */
    H5AC_SUPERBLOCK,            /* (25) file superblock                 */
    H5AC_DRVRINFO,              /* (26) driver info block (supplements superblock) */
    H5AC_EPOCH_MARKER,          /* (27) epoch marker - always internal to cache */
    H5AC_PROXY_ENTRY,           /* (28) cache entry proxy               */
    H5AC_PREFETCHED_ENTRY       /* (29) prefetched entry - always internal to cache */
};



/*-------------------------------------------------------------------------
 * Function:	H5AC_init
 *
 * Purpose:	Initialize the interface from some other layer.
 *
 * Return:	Success:	non-negative
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Saturday, January 18, 2003
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_init(void)
{
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(FAIL)
    /* FUNC_ENTER() does all the work */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5AC_init() */


/*-------------------------------------------------------------------------
 * Function:	H5AC__init_package
 *
 * Purpose:	Initialize interface-specific information
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, July 18, 2002
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC__init_package(void)
{
    FUNC_ENTER_PACKAGE_NOERR

#ifdef H5_HAVE_PARALLEL
    /* check whether to enable strict collective function calling
     * sanity checks using MPI barriers
     */
    {
        const char *s;  /* String for environment variables */

        s = HDgetenv("H5_COLL_API_SANITY_CHECK");
        if(s && HDisdigit(*s)) {
            long env_val = HDstrtol(s, NULL, 0);
            H5_coll_api_sanity_check_g = (0 == env_val) ? FALSE : TRUE;
        }
    }
#endif /* H5_HAVE_PARALLEL */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5AC__init_package() */


/*-------------------------------------------------------------------------
 * Function:	H5AC_term_package
 *
 * Purpose:	Terminate this interface.
 *
 * Return:	Success:	Positive if anything was done that might
 *				affect other interfaces; zero otherwise.
 * 		Failure:	Negative.
 *
 * Programmer:	Quincey Koziol
 *              Thursday, July 18, 2002
 *
 *-------------------------------------------------------------------------
 */
int
H5AC_term_package(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if(H5_PKG_INIT_VAR)
        /* Reset interface initialization flag */
        H5_PKG_INIT_VAR = FALSE;

    FUNC_LEAVE_NOAPI(0)
} /* end H5AC_term_package() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5AC_cache_image_pending()
 *
 * Purpose:     Debugging function that tests to see if the load of a 
 *              metadata cache image load is pending (i.e. will be executed
 *              on the next protect or insert)
 *              
 *              Returns TRUE if a cache image load is pending, and FALSE
 *              if not.  Throws an assertion failure on error.
 *
 * Return:      TRUE if a cache image load is pending, and FALSE otherwise.
 *
 * Programmer:  John Mainzer, 1/10/17
 *
 * Changes:     None.
 *
 *-------------------------------------------------------------------------
 */
hbool_t
H5AC_cache_image_pending(const H5F_t *f)
{
    H5C_t *cache_ptr;
    hbool_t ret_value = FALSE;          /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Sanity checks */
    HDassert(f);
    HDassert(f->shared);
    cache_ptr = f->shared->cache;

    ret_value = H5C_cache_image_pending(cache_ptr);

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_cache_image_pending() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_create
 *
 * Purpose:     Initialize the cache just after a file is opened.  The
 *              SIZE_HINT is the number of cache slots desired.  If you
 *              pass an invalid value then H5AC_NSLOTS is used.  You can
 *              turn off caching by using 1 for the SIZE_HINT value.
 *
 * Return:      Success:        Number of slots actually used.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul  9 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_create(const H5F_t *f, H5AC_cache_config_t *config_ptr, H5AC_cache_image_config_t * image_config_ptr)
{
#ifdef H5_HAVE_PARALLEL
    char 	 prefix[H5C__PREFIX_LEN] = "";
    H5AC_aux_t * aux_ptr = NULL;
#endif /* H5_HAVE_PARALLEL */
    struct H5C_cache_image_ctl_t int_ci_config = H5C__DEFAULT_CACHE_IMAGE_CTL;
    herr_t ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check arguments */
    HDassert(f);
    HDassert(NULL == f->shared->cache);
    HDassert(config_ptr != NULL) ;
    HDassert(image_config_ptr != NULL) ;
    HDassert(image_config_ptr->version == H5AC__CURR_CACHE_IMAGE_CONFIG_VERSION);
    HDcompile_assert(NELMTS(H5AC_class_s) == H5AC_NTYPES);
    HDcompile_assert(H5C__MAX_NUM_TYPE_IDS == H5AC_NTYPES);

    /* Validate configurations */
    if(H5AC_validate_config(config_ptr) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_BADVALUE, FAIL, "Bad cache configuration")
    if(H5AC_validate_cache_image_config(image_config_ptr) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_BADVALUE, FAIL, "Bad cache image configuration")

#ifdef H5_HAVE_PARALLEL
    if(H5F_HAS_FEATURE(f, H5FD_FEAT_HAS_MPI)) {
        MPI_Comm	 mpi_comm;
        int		 mpi_rank;
        int	 	 mpi_size;

        if(MPI_COMM_NULL == (mpi_comm = H5F_mpi_get_comm(f)))
            HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "can't get MPI communicator")

        if((mpi_rank = H5F_mpi_get_rank(f)) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "can't get mpi rank")

        if((mpi_size = H5F_mpi_get_size(f)) < 0)
            HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "can't get mpi size")

        if(NULL == (aux_ptr = H5FL_CALLOC(H5AC_aux_t)))
            HGOTO_ERROR(H5E_CACHE, H5E_CANTALLOC, FAIL, "Can't allocate H5AC auxiliary structure")

        aux_ptr->magic = H5AC__H5AC_AUX_T_MAGIC;
        aux_ptr->mpi_comm = mpi_comm;
        aux_ptr->mpi_rank = mpi_rank;
        aux_ptr->mpi_size = mpi_size;
        aux_ptr->write_permitted = FALSE;
        aux_ptr->dirty_bytes_threshold = H5AC__DEFAULT_DIRTY_BYTES_THRESHOLD;
        aux_ptr->dirty_bytes = 0;
        aux_ptr->metadata_write_strategy = H5AC__DEFAULT_METADATA_WRITE_STRATEGY;
#if H5AC_DEBUG_DIRTY_BYTES_CREATION
        aux_ptr->dirty_bytes_propagations = 0;
        aux_ptr->unprotect_dirty_bytes = 0;
        aux_ptr->unprotect_dirty_bytes_updates = 0;
        aux_ptr->insert_dirty_bytes = 0;
        aux_ptr->insert_dirty_bytes_updates = 0;
        aux_ptr->move_dirty_bytes = 0;
        aux_ptr->move_dirty_bytes_updates = 0;
#endif /* H5AC_DEBUG_DIRTY_BYTES_CREATION */
        aux_ptr->d_slist_ptr = NULL;
        aux_ptr->c_slist_ptr = NULL;
        aux_ptr->candidate_slist_ptr = NULL;
        aux_ptr->write_done = NULL;
        aux_ptr->sync_point_done = NULL;
        aux_ptr->p0_image_len = 0;

        sprintf(prefix, "%d:", mpi_rank);

        if(mpi_rank == 0) {
            if(NULL == (aux_ptr->d_slist_ptr = H5SL_create(H5SL_TYPE_HADDR, NULL)))
                HGOTO_ERROR(H5E_CACHE, H5E_CANTCREATE, FAIL, "can't create dirtied entry list")

            if(NULL == (aux_ptr->c_slist_ptr = H5SL_create(H5SL_TYPE_HADDR, NULL)))
                HGOTO_ERROR(H5E_CACHE, H5E_CANTCREATE, FAIL, "can't create cleaned entry list")
        } /* end if */

        /* construct the candidate slist for all processes.
         * when the distributed strategy is selected as all processes
         * will use it in the case of a flush.
         */
        if(NULL == (aux_ptr->candidate_slist_ptr = H5SL_create(H5SL_TYPE_HADDR, NULL)))
            HGOTO_ERROR(H5E_CACHE, H5E_CANTCREATE, FAIL, "can't create candidate entry list")

        if(aux_ptr != NULL)
            if(aux_ptr->mpi_rank == 0)
                f->shared->cache = H5C_create(H5AC__DEFAULT_MAX_CACHE_SIZE,
                        H5AC__DEFAULT_MIN_CLEAN_SIZE, (H5AC_NTYPES - 1),
                        H5AC_class_s,
                        H5AC__check_if_write_permitted, TRUE, H5AC__log_flushed_entry,
                        (void *)aux_ptr);
            else
                f->shared->cache = H5C_create(H5AC__DEFAULT_MAX_CACHE_SIZE,
                        H5AC__DEFAULT_MIN_CLEAN_SIZE, (H5AC_NTYPES - 1),
                        H5AC_class_s,
                        H5AC__check_if_write_permitted, TRUE, NULL,
                        (void *)aux_ptr);
        else
            f->shared->cache = H5C_create(H5AC__DEFAULT_MAX_CACHE_SIZE,
                    H5AC__DEFAULT_MIN_CLEAN_SIZE, (H5AC_NTYPES - 1),
                    H5AC_class_s,
                    H5AC__check_if_write_permitted, TRUE, NULL, NULL);
    } /* end if */
    else {
#endif /* H5_HAVE_PARALLEL */
        /* The default max cache size and min clean size will frequently be
         * overwritten shortly by the subsequent set resize config call.
         *                                             -- JRM
         */
        f->shared->cache = H5C_create(H5AC__DEFAULT_MAX_CACHE_SIZE,
                H5AC__DEFAULT_MIN_CLEAN_SIZE, (H5AC_NTYPES - 1),
                H5AC_class_s,
                H5AC__check_if_write_permitted, TRUE, NULL, NULL);
#ifdef H5_HAVE_PARALLEL
    } /* end else */
#endif /* H5_HAVE_PARALLEL */

    if(NULL == f->shared->cache)
	HGOTO_ERROR(H5E_CACHE, H5E_CANTALLOC, FAIL, "memory allocation failed")

#ifdef H5_HAVE_PARALLEL
    if(aux_ptr != NULL)
        if(H5C_set_prefix(f->shared->cache, prefix) < 0)
            HGOTO_ERROR(H5E_CACHE, H5E_CANTALLOC, FAIL, "H5C_set_prefix() failed")
#endif /* H5_HAVE_PARALLEL */

    /* Turn on metadata cache logging, if being used
     * This will be JSON until we create a special API call. Trace output
     * is generated when logging is controlled by the struct.
     */
    if(H5F_USE_MDC_LOGGING(f))
        if(H5C_log_set_up(f->shared->cache, H5F_MDC_LOG_LOCATION(f), H5C_LOG_STYLE_JSON, H5F_START_MDC_LOG_ON_ACCESS(f)) < 0)
            HGOTO_ERROR(H5E_CACHE, H5E_LOGGING, FAIL, "mdc logging setup failed")

    /* Set the cache parameters */
    if(H5AC_set_cache_auto_resize_config(f->shared->cache, config_ptr) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTSET, FAIL, "auto resize configuration failed")

    /* Don't need to get the current H5C image config here since the
     * cache has just been created, and thus f->shared->cache->image_ctl 
     * must still set to its initial value (H5C__DEFAULT_CACHE_IMAGE_CTL).  
     * Note that this not true as soon as control returns to the application
     * program, as some test code modifies f->shared->cache->image_ctl.
     */
    int_ci_config.version            = image_config_ptr->version;
    int_ci_config.generate_image     = image_config_ptr->generate_image;
    int_ci_config.save_resize_status = image_config_ptr->save_resize_status;
    int_ci_config.entry_ageout       = image_config_ptr->entry_ageout;
    if(H5C_set_cache_image_config(f, f->shared->cache, &int_ci_config) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTSET, FAIL, "auto resize configuration failed")

done:
    /* If currently logging, generate a message */
    if(f->shared->cache->log_info->logging)
        if(H5C_log_write_create_cache_msg(f->shared->cache, ret_value) < 0)
            HDONE_ERROR(H5E_CACHE, H5E_LOGGING, FAIL, "unable to emit log message")

#ifdef H5_HAVE_PARALLEL
    /* if there is a failure, try to tidy up the auxiliary structure */
    if(ret_value < 0) {
        if(aux_ptr != NULL) {
            if(aux_ptr->d_slist_ptr != NULL)
                H5SL_close(aux_ptr->d_slist_ptr);
            if(aux_ptr->c_slist_ptr != NULL)
                H5SL_close(aux_ptr->c_slist_ptr);
            if(aux_ptr->candidate_slist_ptr != NULL)
                H5SL_close(aux_ptr->candidate_slist_ptr);
            aux_ptr->magic = 0;
            aux_ptr = H5FL_FREE(H5AC_aux_t, aux_ptr);
        } /* end if */
    } /* end if */
#endif /* H5_HAVE_PARALLEL */

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_create() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_dest
 *
 * Purpose:     Flushes all data to disk and destroys the cache.
 *              This function fails if any object are protected since the
 *              resulting file might not be consistent.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul  9 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_dest(H5F_t *f)
{
    hbool_t log_enabled;             /* TRUE if logging was set up */
    hbool_t curr_logging;            /* TRUE if currently logging */
#ifdef H5_HAVE_PARALLEL
    H5AC_aux_t * aux_ptr = NULL;
#endif /* H5_HAVE_PARALLEL */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->cache);

#if H5AC_DUMP_STATS_ON_CLOSE
    /* Dump debugging info */
    H5AC_stats(f);
#endif /* H5AC_DUMP_STATS_ON_CLOSE */

    /* Check if log messages are being emitted */
    if(H5C_get_logging_status(f->shared->cache, &log_enabled, &curr_logging) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_LOGGING, FAIL, "unable to get logging status")
    if(log_enabled && curr_logging)
        if(H5C_log_write_destroy_cache_msg(f->shared->cache) < 0)
            HDONE_ERROR(H5E_CACHE, H5E_LOGGING, FAIL, "unable to emit log message")
    /* Tear down logging */
    if(log_enabled)
        if(H5C_log_tear_down(f->shared->cache) < 0)
            HGOTO_ERROR(H5E_CACHE, H5E_LOGGING, FAIL, "mdc logging tear-down failed")

#ifdef H5_HAVE_PARALLEL
    /* destroying the cache, so clear all collective entries */
    if(H5C_clear_coll_entries(f->shared->cache, FALSE) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTGET, FAIL, "H5C_clear_coll_entries() failed")

    aux_ptr = (H5AC_aux_t *)H5C_get_aux_ptr(f->shared->cache);
    if(aux_ptr)
        /* Sanity check */
        HDassert(aux_ptr->magic == H5AC__H5AC_AUX_T_MAGIC);

    /* If the file was opened R/W, attempt to flush all entries 
     * from rank 0 & Bcast clean list to other ranks.
     *
     * Must not flush in the R/O case, as this will trigger the 
     * free space manager settle routines.
     */
    if(H5F_ACC_RDWR & H5F_INTENT(f))
        if(H5AC__flush_entries(f) < 0)
            HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "Can't flush")
#endif /* H5_HAVE_PARALLEL */

    /* Destroy the cache */
    if(H5C_dest(f) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTFREE, FAIL, "can't destroy cache")
    f->shared->cache = NULL;

#ifdef H5_HAVE_PARALLEL
    if(aux_ptr != NULL) {
        if(aux_ptr->d_slist_ptr != NULL) {
            HDassert(H5SL_count(aux_ptr->d_slist_ptr) == 0);
            H5SL_close(aux_ptr->d_slist_ptr);
        } /* end if */
        if(aux_ptr->c_slist_ptr != NULL) {
            HDassert(H5SL_count(aux_ptr->c_slist_ptr) == 0);
            H5SL_close(aux_ptr->c_slist_ptr);
        } /* end if */
        if(aux_ptr->candidate_slist_ptr != NULL) {
            HDassert(H5SL_count(aux_ptr->candidate_slist_ptr) == 0);
            H5SL_close(aux_ptr->candidate_slist_ptr);
        } /* end if */
        aux_ptr->magic = 0;
        aux_ptr = H5FL_FREE(H5AC_aux_t, aux_ptr);
    } /* end if */
#endif /* H5_HAVE_PARALLEL */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_dest() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_evict
 *
 * Purpose:     Evict all entries except the pinned entries
 *		in the cache.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Vailin Choi
 *		Dec 2013
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_evict(H5F_t *f)
{
    herr_t ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->cache);

    /* Evict all entries in the cache except the pinned superblock entry */
    if(H5C_evict(f) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTFREE, FAIL, "can't evict cache")

done:

    /* If currently logging, generate a message */
    if(f->shared->cache->log_info->logging)
        if(H5C_log_write_evict_cache_msg(f->shared->cache, ret_value) < 0)
            HDONE_ERROR(H5E_CACHE, H5E_LOGGING, FAIL, "unable to emit log message")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_evict() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_expunge_entry
 *
 * Purpose:	Expunge the target entry from the cache without writing it
 * 		to disk even if it is dirty.  The entry must not be either
 * 		pinned or protected.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              6/30/06
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_expunge_entry(H5F_t *f, const H5AC_class_t *type, haddr_t addr,
    unsigned flags)
{
    herr_t  ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->cache);
    HDassert(type);
    HDassert(type->serialize);
    HDassert(H5F_addr_defined(addr));

    if(H5C_expunge_entry(f, type, addr, flags) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTEXPUNGE, FAIL, "H5C_expunge_entry() failed")

done:
    /* If currently logging, generate a message */
    if(f->shared->cache->log_info->logging)
        if(H5C_log_write_expunge_entry_msg(f->shared->cache, addr, type->id, ret_value) < 0)
            HDONE_ERROR(H5E_CACHE, H5E_LOGGING, FAIL, "unable to emit log message")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_expunge_entry() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_flush
 *
 * Purpose:	Flush (and possibly destroy) the metadata cache associated
 *		with the specified file.
 *
 *		If the cache contains protected entries, the function will
 *		fail, as protected entries cannot be flushed.  However
 *		all unprotected entries should be flushed before the
 *		function returns failure.
 *
 * Return:      Non-negative on success/Negative on failure if there was a
 *              request to flush all items and something was protected.
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul  9 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_flush(H5F_t *f)
{
    herr_t ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->cache);

#ifdef H5_HAVE_PARALLEL
    /* flushing the cache, so clear all collective entries */
    if(H5C_clear_coll_entries(f->shared->cache, FALSE) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTGET, FAIL, "H5C_clear_coll_entries() failed")

    /* Attempt to flush all entries from rank 0 & Bcast clean list to other ranks */
    if(H5AC__flush_entries(f) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "Can't flush")
#endif /* H5_HAVE_PARALLEL */

    /* Flush the cache */
    /* (Again, in parallel - writes out the superblock) */
    if(H5C_flush_cache(f, H5AC__NO_FLAGS_SET) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "Can't flush cache")

done:
    /* If currently logging, generate a message */
    if(f->shared->cache->log_info->logging)
        if(H5C_log_write_flush_cache_msg(f->shared->cache, ret_value) < 0)
            HDONE_ERROR(H5E_CACHE, H5E_LOGGING, FAIL, "unable to emit log message")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_flush() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_force_cache_image_load()
 *
 * Purpose:     On rare occasions, it is necessary to run 
 *              H5MF_tidy_self_referential_fsm_hack() prior to the first
 *              metadata cache access.  This is a problem as if there is a 
 *              cache image at the end of the file, that routine will 
 *              discard it.
 *
 *              We solve this issue by calling this function, which will
 *              load the cache image and then call 
 *              H5MF_tidy_self_referential_fsm_hack() to discard it.
 *
 * Return:      SUCCEED on success, and FAIL on failure.
 *
 * Programmer:  John Mainzer
 *              1/11/17
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_force_cache_image_load(H5F_t *f)
{
    herr_t ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->cache);

    if(H5C_force_cache_image_load(f) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTLOAD, FAIL, "Can't load cache image")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_force_cache_image_load() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_get_entry_status
 *
 * Purpose:     Given a file address, determine whether the metadata
 * 		cache contains an entry at that location.  If it does,
 * 		also determine whether the entry is dirty, protected,
 * 		pinned, etc. and return that information to the caller
 * 		in *status.
 *
 * 		If the specified entry doesn't exist, set *status_ptr
 * 		to zero.
 *
 * 		On error, the value of *status is undefined.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              4/27/06
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_get_entry_status(const H5F_t *f, haddr_t addr, unsigned *status)
{
    hbool_t	in_cache;               /* Entry @ addr is in the cache */
    hbool_t	is_dirty;               /* Entry @ addr is in the cache and dirty */
    hbool_t	is_protected;           /* Entry @ addr is in the cache and protected */
    hbool_t	is_pinned;              /* Entry @ addr is in the cache and pinned */
    hbool_t	is_corked;
    hbool_t	is_flush_dep_child;     /* Entry @ addr is in the cache and is a flush dependency child */
    hbool_t	is_flush_dep_parent;    /* Entry @ addr is in the cache and is a flush dependency parent */
    hbool_t	image_is_up_to_date;    /* Entry @ addr is in the cache and has an up to date image */
    herr_t      ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    if((f == NULL) || (!H5F_addr_defined(addr)) || (status == NULL))
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Bad param(s) on entry")

    if(H5C_get_entry_status(f, addr, NULL, &in_cache, &is_dirty,
            &is_protected, &is_pinned, &is_corked, &is_flush_dep_parent, &is_flush_dep_child, &image_is_up_to_date) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5C_get_entry_status() failed")

    if(in_cache) {
	*status |= H5AC_ES__IN_CACHE;
	if(is_dirty)
	    *status |= H5AC_ES__IS_DIRTY;
	if(is_protected)
	    *status |= H5AC_ES__IS_PROTECTED;
	if(is_pinned)
	    *status |= H5AC_ES__IS_PINNED;
	if(is_corked)
	    *status |= H5AC_ES__IS_CORKED;
	if(is_flush_dep_parent)
	    *status |= H5AC_ES__IS_FLUSH_DEP_PARENT;
	if(is_flush_dep_child)
	    *status |= H5AC_ES__IS_FLUSH_DEP_CHILD;
	if(image_is_up_to_date)
	    *status |= H5AC_ES__IMAGE_IS_UP_TO_DATE;
    } /* end if */
    else
        *status = 0;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_get_entry_status() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_insert_entry
 *
 * Purpose:     Adds the specified thing to the cache.  The thing need not
 *              exist on disk yet, but it must have an address and disk
 *              space reserved.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul  9 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_insert_entry(H5F_t *f, const H5AC_class_t *type, haddr_t addr, void *thing,
    unsigned int flags)
{
    herr_t ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->cache);
    HDassert(type);
    HDassert(type->serialize);
    HDassert(H5F_addr_defined(addr));
    HDassert(thing);

    /* Check for invalid access request */
    if(0 == (H5F_INTENT(f) & H5F_ACC_RDWR))
        HGOTO_ERROR(H5E_CACHE, H5E_BADVALUE, FAIL, "no write intent on file")

#if H5AC_DO_TAGGING_SANITY_CHECKS
    if(!H5C_get_ignore_tags(f->shared->cache) && H5AC__verify_tag(type) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTTAG, FAIL, "Bad tag value")
#endif /* H5AC_DO_TAGGING_SANITY_CHECKS */

    /* Insert entry into metadata cache */
    if(H5C_insert_entry(f, type, addr, thing, flags) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTINS, FAIL, "H5C_insert_entry() failed")

#ifdef H5_HAVE_PARALLEL
{
    H5AC_aux_t *aux_ptr;

    if(NULL != (aux_ptr = (H5AC_aux_t *)H5C_get_aux_ptr(f->shared->cache))) {
        /* Log the new entry */
        if(H5AC__log_inserted_entry((H5AC_info_t *)thing) < 0)
            HGOTO_ERROR(H5E_CACHE, H5E_CANTINS, FAIL, "H5AC__log_inserted_entry() failed")

        /* Check if we should try to flush */
        if(aux_ptr->dirty_bytes >= aux_ptr->dirty_bytes_threshold)
            if(H5AC__run_sync_point(f, H5AC_SYNC_POINT_OP__FLUSH_TO_MIN_CLEAN) < 0)
                HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "Can't run sync point")
    } /* end if */
}
#endif /* H5_HAVE_PARALLEL */

done:
    /* If currently logging, generate a message */
    if(f->shared->cache->log_info->logging)
        if(H5C_log_write_insert_entry_msg(f->shared->cache, addr, type->id, flags, ((H5C_cache_entry_t *)thing)->size, ret_value) < 0)
            HDONE_ERROR(H5E_CACHE, H5E_LOGGING, FAIL, "unable to emit log message")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_insert_entry() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_load_cache_image_on_next_protect
 *
 * Purpose:     Load the cache image block at the specified location,
 *              decode it, and insert its contents into the metadata
 *              cache.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              7/6/15
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_load_cache_image_on_next_protect(H5F_t * f, haddr_t addr, hsize_t len,
    hbool_t rw)
{
    herr_t              ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->cache);

    if(H5C_load_cache_image_on_next_protect(f, addr, len, rw) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTLOAD, FAIL, "call to H5C_load_cache_image_on_next_protect failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_load_cache_image_on_next_protect() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_mark_entry_dirty
 *
 * Purpose:	Mark a pinned or protected entry as dirty.  The target
 * 		entry MUST be either pinned, protected, or both.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              5/16/06
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_mark_entry_dirty(void *thing)
{
    H5AC_info_t *entry_ptr = NULL;    /* Pointer to the cache entry */
    H5C_t *cache_ptr = NULL;          /* Pointer to the entry's associated metadata cache */
    herr_t ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(thing);

    /* Set up entry & cache pointers */
    entry_ptr = (H5AC_info_t *)thing;
    cache_ptr = entry_ptr->cache_ptr;

#ifdef H5_HAVE_PARALLEL
{
    H5AC_aux_t *aux_ptr;

    aux_ptr = (H5AC_aux_t *)H5C_get_aux_ptr(cache_ptr);
    if((!entry_ptr->is_dirty) && (!entry_ptr->is_protected) &&
             (entry_ptr->is_pinned) && (NULL != aux_ptr))
        if(H5AC__log_dirtied_entry(entry_ptr) < 0)
            HGOTO_ERROR(H5E_CACHE, H5E_CANTMARKDIRTY, FAIL, "can't log dirtied entry")
}
#endif /* H5_HAVE_PARALLEL */

    if(H5C_mark_entry_dirty(thing) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTMARKDIRTY, FAIL, "can't mark pinned or protected entry dirty")

done:
    /* If currently logging, generate a message */
    if(cache_ptr->log_info->logging)
        if(H5C_log_write_mark_entry_dirty_msg(cache_ptr, entry_ptr, ret_value) < 0)
            HDONE_ERROR(H5E_CACHE, H5E_LOGGING, FAIL, "unable to emit log message")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_mark_entry_dirty() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_mark_entry_clean
 *
 * Purpose:	Mark a pinned entry as clean.  The target
 * 		entry MUST be pinned.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              7/23/16
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_mark_entry_clean(void *thing)
{
    H5AC_info_t *entry_ptr = NULL;    /* Pointer to the cache entry */
    H5C_t *cache_ptr = NULL;          /* Pointer to the entry's associated metadata cache */
    herr_t ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(thing);

    entry_ptr = (H5AC_info_t *)thing;
    cache_ptr = entry_ptr->cache_ptr;

#ifdef H5_HAVE_PARALLEL
{
    H5AC_aux_t *aux_ptr;

    aux_ptr = (H5AC_aux_t *)H5C_get_aux_ptr(cache_ptr);
    if((!entry_ptr->is_dirty) && (!entry_ptr->is_protected) &&
             (entry_ptr->is_pinned) && (NULL != aux_ptr))
        if(H5AC__log_cleaned_entry(entry_ptr) < 0)
            HGOTO_ERROR(H5E_CACHE, H5E_CANTMARKCLEAN, FAIL, "can't log cleaned entry")
}
#endif /* H5_HAVE_PARALLEL */

    if(H5C_mark_entry_clean(thing) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTMARKCLEAN, FAIL, "can't mark pinned or protected entry clean")

done:
    /* If currently logging, generate a message */
    if(cache_ptr->log_info->logging)
        if(H5C_log_write_mark_entry_clean_msg(cache_ptr, entry_ptr, ret_value) < 0)
            HDONE_ERROR(H5E_CACHE, H5E_LOGGING, FAIL, "unable to emit log message")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_mark_entry_clean() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_mark_entry_unserialized
 *
 * Purpose:	Mark a pinned or protected entry as unserialized.  The target
 * 		entry MUST be either pinned, protected, or both.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              12/22/16
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_mark_entry_unserialized(void *thing)
{
    H5AC_info_t *entry_ptr = NULL;    /* Pointer to the cache entry */
    H5C_t *cache_ptr = NULL;          /* Pointer to the entry's associated metadata cache */
    herr_t ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(thing);

    /* Set up entry & cache pointers */
    entry_ptr = (H5AC_info_t *)thing;
    cache_ptr = entry_ptr->cache_ptr;

    if(H5C_mark_entry_unserialized(thing) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTMARKUNSERIALIZED, FAIL, "can't mark entry unserialized")

done:
    /* If currently logging, generate a message */
    if(cache_ptr->log_info->logging)
        if(H5C_log_write_mark_unserialized_entry_msg(cache_ptr, entry_ptr, ret_value) < 0)
            HDONE_ERROR(H5E_CACHE, H5E_LOGGING, FAIL, "unable to emit log message")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_mark_entry_unserialized() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_mark_entry_serialized
 *
 * Purpose:	Mark a pinned entry as serialized.  The target
 * 		entry MUST be pinned.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              12/22/16
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_mark_entry_serialized(void *thing)
{
    H5AC_info_t *entry_ptr = NULL;    /* Pointer to the cache entry */
    H5C_t *cache_ptr = NULL;          /* Pointer to the entry's associated metadata cache */
    herr_t ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(thing);

    entry_ptr = (H5AC_info_t *)thing;
    cache_ptr = entry_ptr->cache_ptr;

    if(H5C_mark_entry_serialized(thing) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTMARKSERIALIZED, FAIL, "can't mark entry serialized")

done:
    /* If currently logging, generate a message */
    if(cache_ptr->log_info->logging)
        if(H5C_log_write_mark_serialized_entry_msg(cache_ptr, entry_ptr, ret_value) < 0)
            HDONE_ERROR(H5E_CACHE, H5E_LOGGING, FAIL, "unable to emit log message")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_mark_entry_serialized() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_move_entry
 *
 * Purpose:     Use this function to notify the cache that an object's
 *              file address changed.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul  9 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_move_entry(H5F_t *f, const H5AC_class_t *type, haddr_t old_addr, haddr_t new_addr)
{
#ifdef H5_HAVE_PARALLEL
    H5AC_aux_t        *aux_ptr;
#endif /* H5_HAVE_PARALLEL */
    herr_t ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(f);
    HDassert(f->shared->cache);
    HDassert(type);
    HDassert(H5F_addr_defined(old_addr));
    HDassert(H5F_addr_defined(new_addr));
    HDassert(H5F_addr_ne(old_addr, new_addr));

#ifdef H5_HAVE_PARALLEL
    /* Log moving the entry */
    if(NULL != (aux_ptr = (H5AC_aux_t *)H5C_get_aux_ptr(f->shared->cache)))
        if(H5AC__log_moved_entry(f, old_addr, new_addr) < 0)
            HGOTO_ERROR(H5E_CACHE, H5E_CANTUNPROTECT, FAIL, "can't log moved entry")
#endif /* H5_HAVE_PARALLEL */

    if(H5C_move_entry(f->shared->cache, type, old_addr, new_addr) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTMOVE, FAIL, "H5C_move_entry() failed")

#ifdef H5_HAVE_PARALLEL
    /* Check if we should try to flush */
    if(NULL != aux_ptr && aux_ptr->dirty_bytes >= aux_ptr->dirty_bytes_threshold)
        if(H5AC__run_sync_point(f, H5AC_SYNC_POINT_OP__FLUSH_TO_MIN_CLEAN) < 0)
            HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "Can't run sync point")
#endif /* H5_HAVE_PARALLEL */

done:
    /* If currently logging, generate a message */
    if(f->shared->cache->log_info->logging)
        if(H5C_log_write_move_entry_msg(f->shared->cache, old_addr, new_addr, type->id, ret_value) < 0)
            HDONE_ERROR(H5E_CACHE, H5E_LOGGING, FAIL, "unable to emit log message")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_move_entry() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_pin_protected_entry()
 *
 * Purpose:	Pin a protected cache entry.  The entry must be protected
 *              at the time of call, and must be unpinned.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              4/27/06
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_pin_protected_entry(void *thing)
{
    H5AC_info_t *entry_ptr = NULL;      /* Pointer to the cache entry */
    H5C_t *cache_ptr = NULL;            /* Pointer to the entry's associated metadata cache */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(thing);

    entry_ptr = (H5AC_info_t *)thing;
    cache_ptr = entry_ptr->cache_ptr;
    HDassert(cache_ptr);

    /* Pin entry */
    if(H5C_pin_protected_entry(thing) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTPIN, FAIL, "can't pin entry")

done:
    /* If currently logging, generate a message */
    if(cache_ptr->log_info->logging)
        if(H5C_log_write_pin_entry_msg(cache_ptr, entry_ptr, ret_value) < 0)
            HDONE_ERROR(H5E_CACHE, H5E_LOGGING, FAIL, "unable to emit log message")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_pin_protected_entry() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5AC_prep_for_file_close
 *
 * Purpose:     This function should be called just prior to the cache
 *              flushes at file close.
 *
 *              The objective of the call is to allow the metadata cache
 *              to do any preparatory work prior to generation of a
 *              cache image.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              7/3/15
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_prep_for_file_close(H5F_t *f)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->cache);

    if(H5C_prep_for_file_close(f) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "cache prep for file close failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_prep_for_file_close() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_create_flush_dependency()
 *
 * Purpose:	Create a flush dependency between two entries in the metadata
 *              cache.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              3/24/09
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_create_flush_dependency(void * parent_thing, void * child_thing)
{
    H5AC_info_t *entry_ptr = NULL;      /* Pointer to the cache entry */
    H5C_t *cache_ptr = NULL;            /* Pointer to the entry's associated metadata cache */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(parent_thing);
    HDassert(child_thing);

    entry_ptr = (H5AC_info_t *)parent_thing;
    cache_ptr = entry_ptr->cache_ptr;
    HDassert(cache_ptr);

    /* Create the flush dependency */
    if(H5C_create_flush_dependency(parent_thing, child_thing) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTDEPEND, FAIL, "H5C_create_flush_dependency() failed")

done:
    /* If currently logging, generate a message */
    if(cache_ptr->log_info->logging)
        if(H5C_log_write_create_fd_msg(cache_ptr, (H5AC_info_t *)parent_thing, (H5AC_info_t *)child_thing, ret_value) < 0)
            HDONE_ERROR(H5E_CACHE, H5E_LOGGING, FAIL, "unable to emit log message")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_create_flush_dependency() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_protect
 *
 * Purpose:     If the target entry is not in the cache, load it.  If
 *		necessary, attempt to evict one or more entries to keep
 *		the cache within its maximum size.
 *
 *		Mark the target entry as protected, and return its address
 *		to the caller.  The caller must call H5AC_unprotect() when
 *		finished with the entry.
 *
 *		While it is protected, the entry may not be either evicted
 *		or flushed -- nor may it be accessed by another call to
 *		H5AC_protect.  Any attempt to do so will result in a failure.
 *
 * Return:      Success:        Ptr to the object.
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Sep  2 1997
 *
 *-------------------------------------------------------------------------
 */
void *
H5AC_protect(H5F_t *f, const H5AC_class_t *type, haddr_t addr, void *udata,
    unsigned flags)
{
    void *              thing = NULL;           /* Pointer to native data structure for entry */
    void *              ret_value = NULL;       /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* Sanity check */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->cache);
    HDassert(type);
    HDassert(type->serialize);
    HDassert(H5F_addr_defined(addr));

    /* Check for unexpected flags -- H5C__FLUSH_COLLECTIVELY_FLAG
     * only permitted in the parallel case.
     */
#ifdef H5_HAVE_PARALLEL
    HDassert(0 == (flags & (unsigned)(~(H5C__READ_ONLY_FLAG | \
                                        H5C__FLUSH_LAST_FLAG | \
                                        H5C__FLUSH_COLLECTIVELY_FLAG))));
#else /* H5_HAVE_PARALLEL */
    HDassert(0 == (flags & (unsigned)(~(H5C__READ_ONLY_FLAG | \
                                        H5C__FLUSH_LAST_FLAG))));
#endif /* H5_HAVE_PARALLEL */

    /* Check for invalid access request */
    if((0 == (H5F_INTENT(f) & H5F_ACC_RDWR)) && (0 == (flags & H5C__READ_ONLY_FLAG)))
	HGOTO_ERROR(H5E_CACHE, H5E_BADVALUE, NULL, "no write intent on file")

#if H5AC_DO_TAGGING_SANITY_CHECKS
    if(!H5C_get_ignore_tags(f->shared->cache) && H5AC__verify_tag(type) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTTAG, NULL, "Bad tag value")
#endif /* H5AC_DO_TAGGING_SANITY_CHECKS */

    if(NULL == (thing = H5C_protect(f, type, addr, udata, flags)))
        HGOTO_ERROR(H5E_CACHE, H5E_CANTPROTECT, NULL, "H5C_protect() failed")

    /* Set return value */
    ret_value = thing;

done:
    /* If currently logging, generate a message */
    {
        herr_t fake_ret_value = (NULL == ret_value) ? FAIL : SUCCEED;

        if(f->shared->cache->log_info->logging)
            if(H5C_log_write_protect_entry_msg(f->shared->cache, (H5AC_info_t *)thing, type->id, flags, fake_ret_value) < 0)
                HDONE_ERROR(H5E_CACHE, H5E_LOGGING, NULL, "unable to emit log message")
    }

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_protect() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_resize_entry
 *
 * Purpose:	Resize a pinned or protected entry.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              7/5/06
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_resize_entry(void *thing, size_t new_size)
{
    H5AC_info_t *entry_ptr = NULL;    /* Pointer to the cache entry */
    H5C_t *cache_ptr = NULL;          /* Pointer to the entry's associated metadata cache */
    herr_t ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(thing);

    entry_ptr = (H5AC_info_t *)thing;
    cache_ptr = entry_ptr->cache_ptr;
    HDassert(cache_ptr);

    /* Resize the entry */
    if(H5C_resize_entry(thing, new_size) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTRESIZE, FAIL, "can't resize entry")

#ifdef H5_HAVE_PARALLEL
{
    H5AC_aux_t *aux_ptr;

    aux_ptr = (H5AC_aux_t *)H5C_get_aux_ptr(cache_ptr);
    if((!entry_ptr->is_dirty) && (NULL != aux_ptr))
        if(H5AC__log_dirtied_entry(entry_ptr) < 0)
            HGOTO_ERROR(H5E_CACHE, H5E_CANTMARKDIRTY, FAIL, "can't log dirtied entry")
}
#endif /* H5_HAVE_PARALLEL */

done:
    /* If currently logging, generate a message */
    if(cache_ptr->log_info->logging)
        if(H5C_log_write_resize_entry_msg(cache_ptr, entry_ptr, new_size, ret_value) < 0)
            HDONE_ERROR(H5E_CACHE, H5E_LOGGING, FAIL, "unable to emit log message")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_resize_entry() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_unpin_entry()
 *
 * Purpose:	Unpin a cache entry.  The entry must be unprotected at
 * 		the time of call, and must be pinned.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              4/11/06
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_unpin_entry(void *thing)
{
    H5AC_info_t *entry_ptr = NULL;    /* Pointer to the cache entry */
    H5C_t *cache_ptr = NULL;          /* Pointer to the entry's associated metadata cache */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(thing);

    entry_ptr = (H5AC_info_t *)thing;
    cache_ptr = entry_ptr->cache_ptr;
    HDassert(cache_ptr);

    /* Unpin the entry */
    if(H5C_unpin_entry(thing) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTUNPIN, FAIL, "can't unpin entry")

done:
    /* If currently logging, generate a message */
    if(cache_ptr->log_info->logging)
        if(H5C_log_write_unpin_entry_msg(cache_ptr, entry_ptr, ret_value) < 0)
            HDONE_ERROR(H5E_CACHE, H5E_LOGGING, FAIL, "unable to emit log message")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_unpin_entry() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_destroy_flush_dependency()
 *
 * Purpose:	Destroy a flush dependency between two entries.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              3/24/09
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_destroy_flush_dependency(void * parent_thing, void * child_thing)
{
    H5AC_info_t *entry_ptr = NULL;      /* Pointer to the cache entry */
    H5C_t *cache_ptr = NULL;            /* Pointer to the entry's associated metadata cache */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(parent_thing);
    HDassert(child_thing);

    entry_ptr = (H5AC_info_t *)parent_thing;
    cache_ptr = entry_ptr->cache_ptr;
    HDassert(cache_ptr);

    /* Destroy the flush dependency */
    if(H5C_destroy_flush_dependency(parent_thing, child_thing) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTUNDEPEND, FAIL, "H5C_destroy_flush_dependency() failed")

done:
    /* If currently logging, generate a message */
    if(cache_ptr->log_info->logging)
        if(H5C_log_write_destroy_fd_msg(cache_ptr, (H5AC_info_t *)parent_thing, (H5AC_info_t *)child_thing, ret_value) < 0)
            HDONE_ERROR(H5E_CACHE, H5E_LOGGING, FAIL, "unable to emit log message")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_destroy_flush_dependency() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_unprotect
 *
 * Purpose:	Undo an H5AC_protect() call -- specifically, mark the
 *		entry as unprotected, remove it from the protected list,
 *		and give it back to the replacement policy.
 *
 *		The TYPE and ADDR arguments must be the same as those in
 *		the corresponding call to H5AC_protect() and the THING
 *		argument must be the value returned by that call to
 *		H5AC_protect().
 *
 *		If the deleted flag is TRUE, simply remove the target entry
 *		from the cache, clear it, and free it without writing it to
 *		disk.
 *
 *		This version of the function is a complete re-write to
 *		use the new metadata cache.  While there isn't all that
 *		much difference between the old and new Purpose sections,
 *		the original version is given below.
 *
 *		Original purpose section:
 *
 *		This function should be called to undo the effect of
 *              H5AC_protect().  The TYPE and ADDR arguments should be the
 *              same as the corresponding call to H5AC_protect() and the
 *              THING argument should be the value returned by H5AC_protect().
 *              If the DELETED flag is set, then this object has been deleted
 *              from the file and should not be returned to the cache.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Sep  2 1997
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_unprotect(H5F_t *f, const H5AC_class_t *type, haddr_t addr, void *thing,
    unsigned flags)
{
    hbool_t		dirtied;
    hbool_t		deleted;
#ifdef H5_HAVE_PARALLEL
    H5AC_aux_t        * aux_ptr = NULL;
#endif /* H5_HAVE_PARALLEL */
    herr_t              ret_value=SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->cache);
    HDassert(type);
    HDassert(type->deserialize);
    HDassert(type->image_len);
    HDassert(H5F_addr_defined(addr));
    HDassert(thing);
    HDassert( ((H5AC_info_t *)thing)->addr == addr );
    HDassert( ((H5AC_info_t *)thing)->type == type );

    dirtied = (hbool_t)(((flags & H5AC__DIRTIED_FLAG) == H5AC__DIRTIED_FLAG) ||
		(((H5AC_info_t *)thing)->dirtied));
    deleted = (hbool_t)((flags & H5C__DELETED_FLAG) == H5C__DELETED_FLAG);

    /* Check if the size changed out from underneath us, if we're not deleting
     *  the entry.
     */
    if(dirtied && !deleted) {
        size_t		curr_size = 0;

        if((type->image_len)(thing, &curr_size) < 0)
            HGOTO_ERROR(H5E_CACHE, H5E_CANTGETSIZE, FAIL, "Can't get size of thing")

        if(((H5AC_info_t *)thing)->size != curr_size)
            HGOTO_ERROR(H5E_CACHE, H5E_BADSIZE, FAIL, "size of entry changed")
    } /* end if */

#ifdef H5_HAVE_PARALLEL
    if(NULL != (aux_ptr = (H5AC_aux_t *)H5C_get_aux_ptr(f->shared->cache))) {
        if(dirtied && ((H5AC_info_t *)thing)->is_dirty == FALSE)
            if(H5AC__log_dirtied_entry((H5AC_info_t *)thing) < 0)
                HGOTO_ERROR(H5E_CACHE, H5E_CANTUNPROTECT, FAIL, "can't log dirtied entry")

        if(deleted && aux_ptr->mpi_rank == 0)
            if(H5AC__log_deleted_entry((H5AC_info_t *)thing) < 0)
                HGOTO_ERROR(H5E_CACHE, H5E_CANTUNPROTECT, FAIL, "H5AC__log_deleted_entry() failed")
    } /* end if */
#endif /* H5_HAVE_PARALLEL */

    if(H5C_unprotect(f, addr, thing, flags) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTUNPROTECT, FAIL, "H5C_unprotect() failed")

#ifdef H5_HAVE_PARALLEL
    /* Check if we should try to flush */
    if((aux_ptr != NULL) && (aux_ptr->dirty_bytes >= aux_ptr->dirty_bytes_threshold))
        if(H5AC__run_sync_point(f, H5AC_SYNC_POINT_OP__FLUSH_TO_MIN_CLEAN) < 0)
            HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "Can't run sync point")
#endif /* H5_HAVE_PARALLEL */

done:
    /* If currently logging, generate a message */
    if(f->shared->cache->log_info->logging)
        if(H5C_log_write_unprotect_entry_msg(f->shared->cache, addr, type->id, flags, ret_value) < 0)
            HDONE_ERROR(H5E_CACHE, H5E_LOGGING, FAIL, "unable to emit log message")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_unprotect() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_get_cache_auto_resize_config
 *
 * Purpose:     Wrapper function for H5C_get_cache_auto_resize_config().
 *
 * Return:      SUCCEED on success, and FAIL on failure.
 *
 * Programmer:  John Mainzer
 *              3/10/05
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_get_cache_auto_resize_config(const H5AC_t *cache_ptr,
    H5AC_cache_config_t *config_ptr)
{
    H5C_auto_size_ctl_t internal_config;
    hbool_t evictions_enabled;
    herr_t ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    if((cache_ptr == NULL) || (config_ptr == NULL) ||
            (config_ptr->version != H5AC__CURR_CACHE_CONFIG_VERSION))
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Bad cache_ptr or config_ptr on entry")
#ifdef H5_HAVE_PARALLEL
{
    H5AC_aux_t *aux_ptr;

    aux_ptr = (H5AC_aux_t *)H5C_get_aux_ptr(cache_ptr);
    if((aux_ptr != NULL) && (aux_ptr->magic != H5AC__H5AC_AUX_T_MAGIC))
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Bad aux_ptr on entry")
}
#endif /* H5_HAVE_PARALLEL */

    /* Retrieve the configuration */
    if(H5C_get_cache_auto_resize_config((const H5C_t *)cache_ptr, &internal_config) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5C_get_cache_auto_resize_config() failed")
    if(H5C_get_evictions_enabled((const H5C_t *)cache_ptr, &evictions_enabled) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5C_get_resize_enabled() failed")

    /* Set the information to return */
    if(internal_config.rpt_fcn == NULL)
        config_ptr->rpt_fcn_enabled = FALSE;
    else
	config_ptr->rpt_fcn_enabled = TRUE;
    config_ptr->open_trace_file        = FALSE;
    config_ptr->close_trace_file       = FALSE;
    config_ptr->trace_file_name[0]     = '\0';
    config_ptr->evictions_enabled      = evictions_enabled;
    config_ptr->set_initial_size       = internal_config.set_initial_size;
    config_ptr->initial_size           = internal_config.initial_size;
    config_ptr->min_clean_fraction     = internal_config.min_clean_fraction;
    config_ptr->max_size               = internal_config.max_size;
    config_ptr->min_size               = internal_config.min_size;
    config_ptr->epoch_length           = (long)(internal_config.epoch_length);
    config_ptr->incr_mode              = internal_config.incr_mode;
    config_ptr->lower_hr_threshold     = internal_config.lower_hr_threshold;
    config_ptr->increment              = internal_config.increment;
    config_ptr->apply_max_increment    = internal_config.apply_max_increment;
    config_ptr->max_increment          = internal_config.max_increment;
    config_ptr->decr_mode              = internal_config.decr_mode;
    config_ptr->upper_hr_threshold     = internal_config.upper_hr_threshold;
    config_ptr->flash_incr_mode        = internal_config.flash_incr_mode;
    config_ptr->flash_multiple         = internal_config.flash_multiple;
    config_ptr->flash_threshold        = internal_config.flash_threshold;
    config_ptr->decrement              = internal_config.decrement;
    config_ptr->apply_max_decrement    = internal_config.apply_max_decrement;
    config_ptr->max_decrement          = internal_config.max_decrement;
    config_ptr->epochs_before_eviction = (int)(internal_config.epochs_before_eviction);
    config_ptr->apply_empty_reserve    = internal_config.apply_empty_reserve;
    config_ptr->empty_reserve          = internal_config.empty_reserve;
#ifdef H5_HAVE_PARALLEL
{
    H5AC_aux_t *aux_ptr;

    if(NULL != (aux_ptr = (H5AC_aux_t *)H5C_get_aux_ptr(cache_ptr))) {
        config_ptr->dirty_bytes_threshold = aux_ptr->dirty_bytes_threshold;
	config_ptr->metadata_write_strategy = aux_ptr->metadata_write_strategy;
    } /* end if */
    else {
#endif /* H5_HAVE_PARALLEL */
        config_ptr->dirty_bytes_threshold = H5AC__DEFAULT_DIRTY_BYTES_THRESHOLD;
	config_ptr->metadata_write_strategy = H5AC__DEFAULT_METADATA_WRITE_STRATEGY;
#ifdef H5_HAVE_PARALLEL
    } /* end else */
}
#endif /* H5_HAVE_PARALLEL */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_get_cache_auto_resize_config() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_get_cache_size
 *
 * Purpose:     Wrapper function for H5C_get_cache_size().
 *
 * Return:      SUCCEED on success, and FAIL on failure.
 *
 * Programmer:  John Mainzer
 *              3/11/05
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_get_cache_size(H5AC_t *cache_ptr, size_t *max_size_ptr, size_t *min_clean_size_ptr,
    size_t *cur_size_ptr, uint32_t *cur_num_entries_ptr)
{
    herr_t ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    if(H5C_get_cache_size((H5C_t *)cache_ptr, max_size_ptr, min_clean_size_ptr,
            cur_size_ptr, cur_num_entries_ptr) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5C_get_cache_size() failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_get_cache_size() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_get_cache_hit_rate
 *
 * Purpose:     Wrapper function for H5C_get_cache_hit_rate().
 *
 * Return:      SUCCEED on success, and FAIL on failure.
 *
 * Programmer:  John Mainzer
 *              3/10/05
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_get_cache_hit_rate(H5AC_t *cache_ptr, double *hit_rate_ptr)
{
    herr_t ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    if(H5C_get_cache_hit_rate((H5C_t *)cache_ptr, hit_rate_ptr) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5C_get_cache_hit_rate() failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_get_cache_hit_rate() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5AC_reset_cache_hit_rate_stats()
 *
 * Purpose:     Wrapper function for H5C_reset_cache_hit_rate_stats().
 *
 * Return:      SUCCEED on success, and FAIL on failure.
 *
 * Programmer:  John Mainzer, 3/10/05
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_reset_cache_hit_rate_stats(H5AC_t * cache_ptr)
{
    herr_t      ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    if(H5C_reset_cache_hit_rate_stats((H5C_t *)cache_ptr) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5C_reset_cache_hit_rate_stats() failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_reset_cache_hit_rate_stats() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_set_cache_auto_resize_config
 *
 * Purpose:     Wrapper function for H5C_set_cache_auto_resize_config().
 *
 * Return:      SUCCEED on success, and FAIL on failure.
 *
 * Programmer:  John Mainzer
 *              3/10/05
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_set_cache_auto_resize_config(H5AC_t *cache_ptr, H5AC_cache_config_t *config_ptr)
{
    H5C_auto_size_ctl_t internal_config;
    herr_t  ret_value = SUCCEED;      	/* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(cache_ptr);

    if(cache_ptr == NULL)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "bad cache_ptr on entry")
#ifdef H5_HAVE_PARALLEL
{
    H5AC_aux_t *aux_ptr;

    aux_ptr = (H5AC_aux_t *)H5C_get_aux_ptr(cache_ptr);
    if((aux_ptr != NULL) && (aux_ptr->magic != H5AC__H5AC_AUX_T_MAGIC))
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "bad aux_ptr on entry")
}
#endif /* H5_HAVE_PARALLEL */

    /* Validate external configuration */
    if(H5AC_validate_config(config_ptr) != SUCCEED)
        HGOTO_ERROR(H5E_CACHE, H5E_BADVALUE, FAIL, "Bad cache configuration")

    /* If the cache config struct is being used to control logging, perform
     * the open/close operations. Note that this is the only place where the
     * struct-based control opens and closes the log files so we also have
     * to write start/stop messages.
     */
    /* close */
    if(config_ptr->close_trace_file)
        if(H5C_log_tear_down((H5C_t *)cache_ptr) < 0)
            HGOTO_ERROR(H5E_CACHE, H5E_LOGGING, FAIL, "mdc logging tear-down failed")

    /* open */
    if(config_ptr->open_trace_file) {
        /* Turn on metadata cache logging.
         * This will be trace output until we create a special API call. JSON
         * output is generated when logging is controlled by the H5P calls.
         */
        if(H5C_log_set_up((H5C_t *)cache_ptr, config_ptr->trace_file_name, H5C_LOG_STYLE_TRACE, TRUE) < 0)
            HGOTO_ERROR(H5E_CACHE, H5E_LOGGING, FAIL, "mdc logging setup failed")
    }

    /* Convert external configuration to internal representation */
    if(H5AC__ext_config_2_int_config(config_ptr, &internal_config) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5AC__ext_config_2_int_config() failed")

    /* Set configuration */
    if(H5C_set_cache_auto_resize_config(cache_ptr, &internal_config) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5C_set_cache_auto_resize_config() failed")
    if(H5C_set_evictions_enabled(cache_ptr, config_ptr->evictions_enabled) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5C_set_evictions_enabled() failed")

#ifdef H5_HAVE_PARALLEL
{
    H5AC_aux_t *aux_ptr;

    /* Set parallel configuration values */
    /* (Which are only held in the H5AC layer -QAK) */
    if(NULL != (aux_ptr = (H5AC_aux_t *)H5C_get_aux_ptr(cache_ptr))) {
        aux_ptr->dirty_bytes_threshold = config_ptr->dirty_bytes_threshold;
        aux_ptr->metadata_write_strategy = config_ptr->metadata_write_strategy;
    } /* end if */
}
#endif /* H5_HAVE_PARALLEL */

done:
    /* If currently logging, generate a message */
    if(cache_ptr->log_info->logging)
        if(H5C_log_write_set_cache_config_msg(cache_ptr, config_ptr, ret_value) < 0)
            HDONE_ERROR(H5E_CACHE, H5E_LOGGING, FAIL, "unable to emit log message")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_set_cache_auto_resize_config() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_validate_config()
 *
 * Purpose:     Run a sanity check on the contents of the supplied
 *		instance of H5AC_cache_config_t.
 *
 *              Do nothing and return SUCCEED if no errors are detected,
 *              and flag an error and return FAIL otherwise.
 *
 *		At present, this function operates by packing the data
 *		from the instance of H5AC_cache_config_t into an instance
 *		of H5C_auto_size_ctl_t, and then calling
 *		H5C_validate_resize_config().  As H5AC_cache_config_t and
 *		H5C_auto_size_ctl_t diverge, we may have to change this.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              4/6/05
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_validate_config(H5AC_cache_config_t *config_ptr)
{
    H5C_auto_size_ctl_t internal_config;
    herr_t              ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    if(config_ptr == NULL)
        HGOTO_ERROR(H5E_CACHE, H5E_BADVALUE, FAIL, "NULL config_ptr on entry")
    if(config_ptr->version != H5AC__CURR_CACHE_CONFIG_VERSION)
        HGOTO_ERROR(H5E_CACHE, H5E_BADVALUE, FAIL, "Unknown config version")

    /* don't bother to test trace_file_name unless open_trace_file is TRUE */
    if(config_ptr->open_trace_file) {
        size_t	        name_len;

        /* Can't really test the trace_file_name field without trying to
         * open the file, so we will content ourselves with a couple of
         * sanity checks on the length of the file name.
         */
        name_len = HDstrlen(config_ptr->trace_file_name);
        if(name_len == 0)
            HGOTO_ERROR(H5E_CACHE, H5E_BADVALUE, FAIL, "config_ptr->trace_file_name is empty")
        else if(name_len > H5AC__MAX_TRACE_FILE_NAME_LEN)
            HGOTO_ERROR(H5E_CACHE, H5E_BADVALUE, FAIL, "config_ptr->trace_file_name too long")
    } /* end if */

    if((config_ptr->evictions_enabled == FALSE) &&
            ((config_ptr->incr_mode != H5C_incr__off) ||
                (config_ptr->flash_incr_mode != H5C_flash_incr__off) ||
                (config_ptr->decr_mode != H5C_decr__off)))
        HGOTO_ERROR(H5E_CACHE, H5E_BADVALUE, FAIL, "Can't disable evictions while auto-resize is enabled")

    if(config_ptr->dirty_bytes_threshold < H5AC__MIN_DIRTY_BYTES_THRESHOLD)
        HGOTO_ERROR(H5E_CACHE, H5E_BADVALUE, FAIL, "dirty_bytes_threshold too small")
    else if(config_ptr->dirty_bytes_threshold > H5AC__MAX_DIRTY_BYTES_THRESHOLD)
        HGOTO_ERROR(H5E_CACHE, H5E_BADVALUE, FAIL, "dirty_bytes_threshold too big")

    if((config_ptr->metadata_write_strategy != H5AC_METADATA_WRITE_STRATEGY__PROCESS_0_ONLY) &&
         (config_ptr->metadata_write_strategy != H5AC_METADATA_WRITE_STRATEGY__DISTRIBUTED))
        HGOTO_ERROR(H5E_CACHE, H5E_BADVALUE, FAIL, "config_ptr->metadata_write_strategy out of range")

    if(H5AC__ext_config_2_int_config(config_ptr, &internal_config) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5AC__ext_config_2_int_config() failed")

    if(H5C_validate_resize_config(&internal_config, H5C_RESIZE_CFG__VALIDATE_ALL) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_BADVALUE, FAIL, "error(s) in new config")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_validate_config() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_validate_cache_image_config()
 *
 * Purpose:     Run a sanity check on the contents of the supplied
 *		instance of H5AC_cache_image_config_t.
 *
 *              Do nothing and return SUCCEED if no errors are detected,
 *              and flag an error and return FAIL otherwise.
 *
 *		At present, this function operates by packing the data
 *		from the instance of H5AC_cache_image_config_t into an 
 *		instance of H5C_cache_image_ctl_t, and then calling
 *		H5C_validate_cache_image_config().  If and when 
 *              H5AC_cache_image_config_t and H5C_cache_image_ctl_t 
 *		diverge, we may have to change this.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              6/25/15
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_validate_cache_image_config(H5AC_cache_image_config_t *config_ptr)
{
    H5C_cache_image_ctl_t internal_config = H5C__DEFAULT_CACHE_IMAGE_CTL;
    herr_t              ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Check args */
    if(config_ptr == NULL)
        HGOTO_ERROR(H5E_CACHE, H5E_BADVALUE, FAIL, "NULL config_ptr on entry")

    if(config_ptr->version != H5AC__CURR_CACHE_IMAGE_CONFIG_VERSION)
        HGOTO_ERROR(H5E_CACHE, H5E_BADVALUE, FAIL, "Unknown image config version")

    /* don't need to get the current H5C image config here since the
     * default values of fields not in the H5AC config will always be 
     * valid.
     */
    internal_config.version            = config_ptr->version;
    internal_config.generate_image     = config_ptr->generate_image;
    internal_config.save_resize_status = config_ptr->save_resize_status;
    internal_config.entry_ageout       = config_ptr->entry_ageout;

    if(H5C_validate_cache_image_config(&internal_config) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_BADVALUE, FAIL, "error(s) in new cache image config")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_validate_cache_image_config() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5AC__check_if_write_permitted
 *
 * Purpose:     Determine if a write is permitted under the current
 *		circumstances, and set *write_permitted_ptr accordingly.
 *		As a general rule it is, but when we are running in parallel
 *		mode with collective I/O, we must ensure that a read cannot
 *		cause a write.
 *
 *		In the event of failure, the value of *write_permitted_ptr
 *		is undefined.
 *
 * Return:      Non-negative on success/Negative on failure.
 *
 * Programmer:  John Mainzer, 5/15/04
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5AC__check_if_write_permitted(const H5F_t
#ifndef H5_HAVE_PARALLEL
H5_ATTR_UNUSED
#endif /* H5_HAVE_PARALLEL */
    *f, hbool_t *write_permitted_ptr)
{
#ifdef H5_HAVE_PARALLEL
    H5AC_aux_t *	aux_ptr = NULL;
#endif /* H5_HAVE_PARALLEL */
    hbool_t		write_permitted = TRUE;

    FUNC_ENTER_STATIC_NOERR

#ifdef H5_HAVE_PARALLEL
    /* Sanity checks */
    HDassert(f != NULL);
    HDassert(f->shared != NULL);
    HDassert(f->shared->cache != NULL);
    aux_ptr = (H5AC_aux_t *)H5C_get_aux_ptr(f->shared->cache);
    if(aux_ptr != NULL) {
        HDassert(aux_ptr->magic == H5AC__H5AC_AUX_T_MAGIC);

        if((aux_ptr->mpi_rank == 0) || (aux_ptr->metadata_write_strategy == H5AC_METADATA_WRITE_STRATEGY__DISTRIBUTED))
	    write_permitted = aux_ptr->write_permitted;
        else
	    write_permitted = FALSE;
    } /* end if */
#endif /* H5_HAVE_PARALLEL */

    *write_permitted_ptr = write_permitted;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5AC__check_if_write_permitted() */


/*-------------------------------------------------------------------------
 * Function:    H5AC__ext_config_2_int_config()
 *
 * Purpose:     Utility function to translate an instance of
 *		H5AC_cache_config_t to an instance of H5C_auto_size_ctl_t.
 *
 *		Places translation in *int_conf_ptr and returns SUCCEED
 *		if successful.  Returns FAIL on failure.
 *
 *		Does only minimal sanity checking.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              1/26/06
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5AC__ext_config_2_int_config(H5AC_cache_config_t *ext_conf_ptr,
    H5C_auto_size_ctl_t *int_conf_ptr)
{
    herr_t               ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_STATIC

    if((ext_conf_ptr == NULL) || (ext_conf_ptr->version != H5AC__CURR_CACHE_CONFIG_VERSION) ||
            (int_conf_ptr == NULL))
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Bad ext_conf_ptr or inf_conf_ptr on entry")

    int_conf_ptr->version                = H5C__CURR_AUTO_SIZE_CTL_VER;
    if(ext_conf_ptr->rpt_fcn_enabled)
        int_conf_ptr->rpt_fcn            = H5C_def_auto_resize_rpt_fcn;
    else
        int_conf_ptr->rpt_fcn            = NULL;

    int_conf_ptr->set_initial_size       = ext_conf_ptr->set_initial_size;
    int_conf_ptr->initial_size           = ext_conf_ptr->initial_size;
    int_conf_ptr->min_clean_fraction     = ext_conf_ptr->min_clean_fraction;
    int_conf_ptr->max_size               = ext_conf_ptr->max_size;
    int_conf_ptr->min_size               = ext_conf_ptr->min_size;
    int_conf_ptr->epoch_length           = (int64_t)(ext_conf_ptr->epoch_length);

    int_conf_ptr->incr_mode              = ext_conf_ptr->incr_mode;
    int_conf_ptr->lower_hr_threshold     = ext_conf_ptr->lower_hr_threshold;
    int_conf_ptr->increment              = ext_conf_ptr->increment;
    int_conf_ptr->apply_max_increment    = ext_conf_ptr->apply_max_increment;
    int_conf_ptr->max_increment          = ext_conf_ptr->max_increment;
    int_conf_ptr->flash_incr_mode        = ext_conf_ptr->flash_incr_mode;
    int_conf_ptr->flash_multiple         = ext_conf_ptr->flash_multiple;
    int_conf_ptr->flash_threshold        = ext_conf_ptr->flash_threshold;

    int_conf_ptr->decr_mode              = ext_conf_ptr->decr_mode;
    int_conf_ptr->upper_hr_threshold     = ext_conf_ptr->upper_hr_threshold;
    int_conf_ptr->decrement              = ext_conf_ptr->decrement;
    int_conf_ptr->apply_max_decrement    = ext_conf_ptr->apply_max_decrement;
    int_conf_ptr->max_decrement          = ext_conf_ptr->max_decrement;
    int_conf_ptr->epochs_before_eviction = (int32_t)(ext_conf_ptr->epochs_before_eviction);
    int_conf_ptr->apply_empty_reserve    = ext_conf_ptr->apply_empty_reserve;
    int_conf_ptr->empty_reserve          = ext_conf_ptr->empty_reserve;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC__ext_config_2_int_config() */


/*------------------------------------------------------------------------------
 * Function:    H5AC_ignore_tags()
 *
 * Purpose:     Override all assertion frameworks and force application of 
 *              global tag everywhere. This should really only be used in the
 *              tests that need to access functions without going through 
 *              API paths.
 * 
 * Return:      SUCCEED on success, FAIL otherwise.
 *
 * Programmer:  Mike McGreevy
 *              December 1, 2009
 *
 *------------------------------------------------------------------------------
 */
herr_t
H5AC_ignore_tags(const H5F_t *f)
{
    herr_t      ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->cache);

    /* Set up a new metadata tag */
    if(H5C_ignore_tags(f->shared->cache) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTSET, FAIL, "H5C_ignore_tags() failed")
            
done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_ignore_tags() */


/*------------------------------------------------------------------------------
 * Function:    H5AC_tag()
 *
 * Purpose:     Sets the metadata tag property in the provided property list.
 * 
 * Return:      SUCCEED on success, FAIL otherwise.
 *
 * Programmer:  Mike McGreevy
 *              December 1, 2009
 *
 *------------------------------------------------------------------------------
 */
void
H5AC_tag(haddr_t metadata_tag, haddr_t *prev_tag)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Get the current tag value and return that (if prev_tag is NOT null) */
    if(prev_tag)
        *prev_tag = H5CX_get_tag();

    /* Set the provided tag */
    H5CX_set_tag(metadata_tag);

    FUNC_LEAVE_NOAPI_VOID
} /* H5AC_tag */


/*------------------------------------------------------------------------------
 * Function:    H5AC_retag_copied_metadata()
 *
 * Purpose:     Searches through cache index for all entries with the
 *              H5AC__COPIED_TAG, indicating that it was created as a 
 *              result of an object copy, and applies the provided tag.
 * 
 * Return:      SUCCEED on success, FAIL otherwise.
 *
 * Programmer:  Mike McGreevy
 *              March 17, 2010
 *
 *------------------------------------------------------------------------------
 */
herr_t
H5AC_retag_copied_metadata(const H5F_t *f, haddr_t metadata_tag) 
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(f);
    HDassert(f->shared);
     
    /* Call cache-level function to re-tag entries with the COPIED tag */
    if(H5C_retag_entries(f->shared->cache, H5AC__COPIED_TAG, metadata_tag) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTSET, FAIL, "Can't retag metadata")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_retag_copied_metadata() */


/*------------------------------------------------------------------------------
 * Function:    H5AC_flush_tagged_metadata()
 *
 * Purpose:     Wrapper for cache level function which flushes all metadata
 *              that contains the specific tag. 
 * 
 * Return:      SUCCEED on success, FAIL otherwise.
 *
 * Programmer:  Mike McGreevy
 *              May 19, 2010
 *
 *------------------------------------------------------------------------------
 */
herr_t
H5AC_flush_tagged_metadata(H5F_t *f, haddr_t metadata_tag)
{
    /* Variable Declarations */
    herr_t ret_value = SUCCEED;
 
    /* Function Enter Macro */   
    FUNC_ENTER_NOAPI(FAIL)

    /* Assertions */
    HDassert(f);
    HDassert(f->shared);

    /* Call cache level function to flush metadata entries with specified tag */
    if(H5C_flush_tagged_entries(f, metadata_tag) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "Cannot flush metadata")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_flush_tagged_metadata */


/*------------------------------------------------------------------------------
 * Function:    H5AC_evict_tagged_metadata()
 *
 * Purpose:     Wrapper for cache level function which flushes all metadata
 *              that contains the specific tag. 
 * 
 * Return:      SUCCEED on success, FAIL otherwise.
 *
 * Programmer:  Mike McGreevy
 *              May 19, 2010
 *
 *------------------------------------------------------------------------------
 */
herr_t
H5AC_evict_tagged_metadata(H5F_t *f, haddr_t metadata_tag, hbool_t match_global)
{
    /* Variable Declarations */
    herr_t ret_value = SUCCEED;
 
    /* Function Enter Macro */   
    FUNC_ENTER_NOAPI(FAIL)

    /* Assertions */
    HDassert(f);
    HDassert(f->shared);

    /* Call cache level function to evict metadata entries with specified tag */
    if(H5C_evict_tagged_entries(f, metadata_tag, match_global) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "Cannot evict metadata")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_evict_tagged_metadata() */


/*------------------------------------------------------------------------------
 * Function:    H5AC_expunge_tag_type_metadata()
 *
 * Purpose:     Wrapper for cache level function which expunge entries with
 *              a specific tag and type id.
 * 
 * Return:      SUCCEED on success, FAIL otherwise.
 *
 * Programmer:  Vailin Choi; May 2016
 *
 *------------------------------------------------------------------------------
 */
herr_t
H5AC_expunge_tag_type_metadata(H5F_t *f, haddr_t tag, int type_id, unsigned flags)
{
    /* Variable Declarations */
    herr_t ret_value = SUCCEED;
 
    /* Function Enter Macro */   
    FUNC_ENTER_NOAPI(FAIL)

    /* Assertions */
    HDassert(f);
    HDassert(f->shared);

    /* Call cache level function to expunge entries with specified tag and type id */
    if(H5C_expunge_tag_type_metadata(f, tag, type_id, flags)<0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "Cannot expunge tagged type entries")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_expunge_tag_type_metadata*/


/*------------------------------------------------------------------------------
 * Function:    H5AC_get_tag()
 *
 * Purpose:     Get the tag for a metadata cache entry.
 * 
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  Dana Robinson
 *              Fall 2016
 *
 *------------------------------------------------------------------------------
 */
herr_t
H5AC_get_tag(const void *thing, haddr_t *tag)
{
    /* Variable Declarations */
    herr_t ret_value = SUCCEED;
 
    /* Function Enter Macro */   
    FUNC_ENTER_NOAPI(FAIL)

    /* Assertions */
    HDassert(thing);
    HDassert(tag);

    /* Call cache level function to get the tag */
    if(H5C_get_tag(thing, tag) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTTAG, FAIL, "Cannot get tag for metadata cache entry")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5AC_get_tag() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_cork
 *
 * Purpose:     To cork/uncork/get cork status for an object
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Vailin Choi; Jan 2014
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_cork(H5F_t *f, haddr_t obj_addr, unsigned action, hbool_t *corked)
{
    herr_t ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->cache);
    HDassert(H5F_addr_defined(obj_addr));
    HDassert(action == H5AC__SET_CORK || action == H5AC__UNCORK || action == H5AC__GET_CORKED);

    /*  Skip the search on "tag_list" when there are no "corked" objects.
     *  This is done to mitigate the slow down when closing objects.
     *  Re-visit this optimization when we optimize tag info management
     *  in the future.
     */
    if(action == H5AC__GET_CORKED) {
        HDassert(corked);
        if(H5C_get_num_objs_corked(f->shared->cache) == 0) {
            *corked = FALSE;
            HGOTO_DONE(SUCCEED)
        }
    }

    if(H5C_cork(f->shared->cache, obj_addr, action, corked) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Cannot perform the cork action")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_cork() */

#if H5AC_DO_TAGGING_SANITY_CHECKS

/*-------------------------------------------------------------------------
 *
 * Function:    H5AC__verify_tag
 *
 * Purpose:     Performs sanity checking on an entry type and tag value
 *              stored in a supplied dxpl_id.
 *
 * Return:      SUCCEED or FAIL.
 *
 * Programmer:  Mike McGreevy
 *              October 20, 2010
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5AC__verify_tag(const H5AC_class_t *type)
{
    haddr_t tag;                        /* Entry tag to validate */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_STATIC

    /* Get the current tag */
    tag = H5CX_get_tag();

    /* Verify legal tag value */
    if(H5C_verify_tag(type->id, tag) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTGET, FAIL, "tag verification failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC__verify_tag */
#endif /* H5AC_DO_TAGGING_SANITY_CHECKS */


/*-------------------------------------------------------------------------
 * Function:    H5AC_get_entry_ring
 *
 * Purpose:     Given a file address, retrieve the ring for an entry at that
 *              address.
 *
 * 		On error, the value of *ring is not modified.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              9/8/15
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_get_entry_ring(const H5F_t *f, haddr_t addr, H5AC_ring_t *ring)
{
    herr_t      ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(f);
    HDassert(H5F_addr_defined(addr));
    HDassert(ring);

    /* Retrieve the ring value for the entry at address */
    if(H5C_get_entry_ring(f, addr, ring) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTGET, FAIL, "Can't retrieve ring for entry")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_get_entry_ring() */


/*-------------------------------------------------------------------------
 * Function:       H5AC_set_ring
 *
 * Purpose:        Routine to set the ring on a DXPL (for passing through
 *                 to the metadata cache).
 *
 * Return:	   Success:	Non-negative
 *		   Failure:	Negative
 *
 * Programmer:     Quincey Koziol
 *                 Tuesday, September 8, 2015
 *
 *-------------------------------------------------------------------------
 */
void
H5AC_set_ring(H5AC_ring_t ring, H5AC_ring_t *orig_ring)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Get the current ring value and return that (if orig_ring is NOT null) */
    if(orig_ring)
        *orig_ring = H5CX_get_ring();

    /* Set the provided ring */
    H5CX_set_ring(ring);

    FUNC_LEAVE_NOAPI_VOID
} /* end H5AC_set_ring() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_unsettle_entry_ring()
 *
 * Purpose:     Advise the metadata cache that the specified entry's metadata
 *              cache manager ring is no longer settled (if it was on entry).
 *
 *              If the target metadata cache manager ring is already
 *              unsettled, do nothing, and return SUCCEED.
 *
 *              If the target metadata cache manager ring is settled, and
 *              we are not in the process of a file shutdown, mark
 *              the ring as unsettled, and return SUCCEED.
 *
 *              If the target metadata cache  manager is settled, and we
 *              are in the process of a file shutdown, post an error
 *              message, and return FAIL.
 *
 *		Note that this function simply passes the call on to
 *		the metadata cache proper, and returns the result.
 *
 * Return:	Success:	Non-negative
 *		Failure:	Negative
 *
 * Programmer:  Quincey Koziol
 *              September 17, 2016
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_unsettle_entry_ring(void *_entry)
{
    H5AC_info_t *entry = (H5AC_info_t *)_entry; /* Entry to remove */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(entry);

    /* Unsettle the entry's ring */
    if(H5C_unsettle_entry_ring(entry) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTREMOVE, FAIL, "can't remove entry")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_unsettle_entry_ring() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_unsettle_ring()
 *
 * Purpose:     Advise the metadata cache that the specified free space
 *              manager ring is no longer settled (if it was on entry).
 *
 *              If the target free space manager ring is already
 *              unsettled, do nothing, and return SUCCEED.
 *
 *              If the target free space manager ring is settled, and
 *              we are not in the process of a file shutdown, mark
 *              the ring as unsettled, and return SUCCEED.
 *
 *              If the target free space manager is settled, and we
 *              are in the process of a file shutdown, post an error
 *              message, and return FAIL.
 *
 *              Note that this function simply passes the call on to
 *              the metadata cache proper, and returns the result.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              10/15/16
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_unsettle_ring(H5F_t * f, H5C_ring_t ring)
{
    herr_t              ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    if(FAIL == (ret_value = H5C_unsettle_ring(f, ring)))
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5C_unsettle_ring() failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_unsettle_ring() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_remove_entry()
 *
 * Purpose:     Remove an entry from the cache.  Must be not protected, pinned,
 *		dirty, involved in flush dependencies, etc.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              September 17, 2016
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_remove_entry(void *_entry)
{
    H5AC_info_t *entry = (H5AC_info_t *)_entry; /* Entry to remove */
    H5C_t *cache = NULL;                /* Pointer to the entry's associated metadata cache */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(entry);
    cache = entry->cache_ptr;
    HDassert(cache);

    /* Remove the entry from the cache*/
    if(H5C_remove_entry(entry) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTREMOVE, FAIL, "can't remove entry")

done:
    /* If currently logging, generate a message */
    if(cache->log_info->logging)
        if(H5C_log_write_remove_entry_msg(cache, entry, ret_value) < 0)
            HDONE_ERROR(H5E_CACHE, H5E_LOGGING, FAIL, "unable to emit log message")

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_remove_entry() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_get_mdc_image_info
 *
 * Purpose:     Wrapper function for H5C_get_mdc_image_info().
 *
 * Return:      SUCCEED on success, and FAIL on failure.
 *
 * Programmer:  Vailin Choi; March 2017
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_get_mdc_image_info(H5AC_t *cache_ptr, haddr_t *image_addr, hsize_t *image_len)
{
    herr_t ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    if(H5C_get_mdc_image_info((H5C_t *)cache_ptr, image_addr, image_len) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTGET, FAIL, "can't retrieve cache image info")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_get_mdc_image_info() */

