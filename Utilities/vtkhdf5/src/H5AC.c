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
 * Created:             H5AC.c
 *                      Jul  9 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             Functions in this file implement a cache for
 *                      things which exist on disk.  All "things" associated
 *                      with a particular HDF file share the same cache; each
 *                      HDF file has it's own cache.
 *
 * Modifications:
 *
 *      Robb Matzke, 4 Aug 1997
 *      Added calls to H5E.
 *
 *      Quincey Koziol, 22 Apr 2000
 *      Turned on "H5AC_SORT_BY_ADDR"
 *
 *	John Mainzer, 5/19/04
 * 	Complete redesign and rewrite.  See the header comments for
 *      H5AC_t for an overview of what is going on.
 *
 *	John Mainzer, 6/4/04
 *	Factored the new cache code into a separate file (H5C.c) to
 *	facilitate re-use.  Re-worked this file again to use H5C.
 *
 *-------------------------------------------------------------------------
 */

#define H5AC_PACKAGE            /*suppress error about including H5ACpkg  */
#define H5C_PACKAGE             /*suppress error about including H5Cpkg   */
#define H5F_PACKAGE		/*suppress error about including H5Fpkg	  */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC	H5AC_init_interface

#ifdef H5_HAVE_PARALLEL
#include <mpi.h>
#endif /* H5_HAVE_PARALLEL */

#include "H5private.h"		/* Generic Functions			*/
#include "H5ACpkg.h"		/* Metadata cache			*/
#include "H5Cpkg.h"             /* Cache                                */
#include "H5Dprivate.h"		/* Dataset functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fpkg.h"		/* Files				*/
#include "H5FDprivate.h"	/* File drivers				*/
#include "H5FLprivate.h"        /* Free Lists                           */
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5MMprivate.h"        /* Memory management                    */
#include "H5Pprivate.h"         /* Property lists                       */


#ifdef H5_HAVE_PARALLEL

/* Declare a free list to manage the H5AC_aux_t struct */
H5FL_DEFINE_STATIC(H5AC_aux_t);

#endif /* H5_HAVE_PARALLEL */

/****************************************************************************
 *
 * structure H5AC_slist_entry_t
 *
 * The dirty entry list maintained via the d_slist_ptr field of H5AC_aux_t
 * and the cleaned entry list maintained via the c_slist_ptr field of
 * H5AC_aux_t are just lists of the file offsets of the dirty/cleaned
 * entries.  Unfortunately, the slist code makes us define a dynamically
 * allocated structure to store these offsets in.  This structure serves
 * that purpose.  Its fields are as follows:
 *
 * magic:       Unsigned 32 bit integer always set to
 *		H5AC__H5AC_SLIST_ENTRY_T_MAGIC.  This field is used to
 *		validate pointers to instances of H5AC_slist_entry_t.
 *
 * addr:	file offset of a metadata entry.  Entries are added to this
 *		list (if they aren't there already) when they are marked
 *		dirty in an unprotect, inserted, or moved.  They are
 *		removed when they appear in a clean entries broadcast.
 *
 ****************************************************************************/

#ifdef H5_HAVE_PARALLEL

#define H5AC__H5AC_SLIST_ENTRY_T_MAGIC        0x00D0A02

typedef struct H5AC_slist_entry_t
{
    uint32_t    magic;

    haddr_t     addr;
} H5AC_slist_entry_t;

/* Declare a free list to manage the H5AC_slist_entry_t struct */
H5FL_DEFINE_STATIC(H5AC_slist_entry_t);

#endif /* H5_HAVE_PARALLEL */


/*
 * Private file-scope variables.
 */

/* Default dataset transfer property list for metadata I/O calls */
/* (Collective set, "block before metadata write" set and "library internal" set) */
/* (Global variable definition, declaration is in H5ACprivate.h also) */
hid_t H5AC_dxpl_id=(-1);

/* Private dataset transfer property list for metadata I/O calls */
/* (Collective set and "library internal" set) */
/* (Static variable definition) */
static hid_t H5AC_noblock_dxpl_id=(-1);

/* Dataset transfer property list for independent metadata I/O calls */
/* (just "library internal" set - i.e. independent transfer mode) */
/* (Global variable definition, declaration is in H5ACprivate.h also) */
hid_t H5AC_ind_dxpl_id=(-1);


/*
 * Private file-scope function declarations:
 */

static herr_t H5AC_check_if_write_permitted(const H5F_t *f,
                                            hid_t dxpl_id,
                                            hbool_t * write_permitted_ptr);

#ifdef H5_HAVE_PARALLEL
static herr_t H5AC_broadcast_clean_list(H5AC_t * cache_ptr);
#endif /* JRM */

static herr_t H5AC_ext_config_2_int_config(H5AC_cache_config_t * ext_conf_ptr,
                                           H5C_auto_size_ctl_t * int_conf_ptr);

#ifdef H5_HAVE_PARALLEL
static herr_t H5AC_log_deleted_entry(H5AC_t * cache_ptr,
                                     H5AC_info_t * entry_ptr,
                                     haddr_t addr,
                                     unsigned int flags);

static herr_t H5AC_log_dirtied_entry(const H5AC_info_t * entry_ptr,
                                     haddr_t addr,
                                     hbool_t size_changed,
                                     size_t new_size);

static herr_t H5AC_log_flushed_entry(H5C_t * cache_ptr,
                                     haddr_t addr,
                                     hbool_t was_dirty,
                                     unsigned flags,
                                     int type_id);

#if 0 /* this is useful debugging code -- JRM */
static herr_t H5AC_log_flushed_entry_dummy(H5C_t * cache_ptr,
                                           haddr_t addr,
                                           hbool_t was_dirty,
                                           unsigned flags,
                                           int type_id);
#endif /* JRM */

static herr_t H5AC_log_inserted_entry(H5F_t * f,
                                      H5AC_t * cache_ptr,
                                      H5AC_info_t * entry_ptr,
                                      const H5AC_class_t * type,
                                      haddr_t addr);

static herr_t H5AC_propagate_flushed_and_still_clean_entries_list(H5F_t  * f,
                                                           hid_t dxpl_id,
                                                           H5AC_t * cache_ptr,
                                                           hbool_t  do_barrier);

static herr_t H5AC_receive_and_apply_clean_list(H5F_t  * f,
                                                hid_t    primary_dxpl_id,
                                                hid_t    secondary_dxpl_id,
                                                H5AC_t * cache_ptr);

static herr_t H5AC_log_moved_entry(const H5F_t * f,
                                     haddr_t old_addr,
                                     haddr_t new_addr);

static herr_t H5AC_flush_entries(H5F_t *f);
#endif /* H5_HAVE_PARALLEL */


/*-------------------------------------------------------------------------
 * Function:	H5AC_init
 *
 * Purpose:	Initialize the interface from some other layer.
 *
 * Return:	Success:	non-negative
 *
 *		Failure:	negative
 *
 * Programmer:	Quincey Koziol
 *              Saturday, January 18, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_init(void)
{
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5AC_init, FAIL)
    /* FUNC_ENTER() does all the work */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5AC_init_interface
 *
 * Purpose:	Initialize interface-specific information
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, July 18, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5AC_init_interface(void)
{
#ifdef H5_HAVE_PARALLEL
    H5P_genclass_t  *xfer_pclass;   /* Dataset transfer property list class object */
    H5P_genplist_t  *xfer_plist;    /* Dataset transfer property list object */
    unsigned block_before_meta_write; /* "block before meta write" property value */
    unsigned library_internal=1;    /* "library internal" property value */
    H5FD_mpio_xfer_t xfer_mode;     /* I/O transfer mode property value */
    herr_t ret_value=SUCCEED;           /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5AC_init_interface)

    /* Sanity check */
    HDassert(H5P_CLS_DATASET_XFER_g!=(-1));

    /* Get the dataset transfer property list class object */
    if (NULL == (xfer_pclass = H5I_object(H5P_CLS_DATASET_XFER_g)))
        HGOTO_ERROR(H5E_CACHE, H5E_BADATOM, FAIL, "can't get property list class")

    /* Get an ID for the blocking, collective H5AC dxpl */
    if ((H5AC_dxpl_id=H5P_create_id(xfer_pclass,FALSE)) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTCREATE, FAIL, "unable to register property list")

    /* Get the property list object */
    if (NULL == (xfer_plist = H5I_object(H5AC_dxpl_id)))
        HGOTO_ERROR(H5E_CACHE, H5E_BADATOM, FAIL, "can't get new property list object")

    /* Insert 'block before metadata write' property */
    block_before_meta_write=1;
    if(H5P_insert(xfer_plist,H5AC_BLOCK_BEFORE_META_WRITE_NAME,H5AC_BLOCK_BEFORE_META_WRITE_SIZE,&block_before_meta_write,NULL,NULL,NULL,NULL,NULL,NULL)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't insert metadata cache dxpl property")

    /* Insert 'library internal' property */
    if(H5P_insert(xfer_plist,H5AC_LIBRARY_INTERNAL_NAME,H5AC_LIBRARY_INTERNAL_SIZE,&library_internal,NULL,NULL,NULL,NULL,NULL,NULL)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't insert metadata cache dxpl property")

    /* Set the transfer mode */
    xfer_mode=H5FD_MPIO_COLLECTIVE;
    if (H5P_set(xfer_plist,H5D_XFER_IO_XFER_MODE_NAME,&xfer_mode)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value")

    /* Get an ID for the non-blocking, collective H5AC dxpl */
    if ((H5AC_noblock_dxpl_id=H5P_create_id(xfer_pclass,FALSE)) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTCREATE, FAIL, "unable to register property list")

    /* Get the property list object */
    if (NULL == (xfer_plist = H5I_object(H5AC_noblock_dxpl_id)))
        HGOTO_ERROR(H5E_CACHE, H5E_BADATOM, FAIL, "can't get new property list object")

    /* Insert 'block before metadata write' property */
    block_before_meta_write=0;
    if(H5P_insert(xfer_plist,H5AC_BLOCK_BEFORE_META_WRITE_NAME,H5AC_BLOCK_BEFORE_META_WRITE_SIZE,&block_before_meta_write,NULL,NULL,NULL,NULL,NULL,NULL)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't insert metadata cache dxpl property")

    /* Insert 'library internal' property */
    if(H5P_insert(xfer_plist,H5AC_LIBRARY_INTERNAL_NAME,H5AC_LIBRARY_INTERNAL_SIZE,&library_internal,NULL,NULL,NULL,NULL,NULL,NULL)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't insert metadata cache dxpl property")

    /* Set the transfer mode */
    xfer_mode=H5FD_MPIO_COLLECTIVE;
    if (H5P_set(xfer_plist,H5D_XFER_IO_XFER_MODE_NAME,&xfer_mode)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value")

    /* Get an ID for the non-blocking, independent H5AC dxpl */
    if ((H5AC_ind_dxpl_id=H5P_create_id(xfer_pclass,FALSE)) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTCREATE, FAIL, "unable to register property list")

    /* Get the property list object */
    if (NULL == (xfer_plist = H5I_object(H5AC_ind_dxpl_id)))
        HGOTO_ERROR(H5E_CACHE, H5E_BADATOM, FAIL, "can't get new property list object")

    /* Insert 'block before metadata write' property */
    block_before_meta_write=0;
    if(H5P_insert(xfer_plist,H5AC_BLOCK_BEFORE_META_WRITE_NAME,H5AC_BLOCK_BEFORE_META_WRITE_SIZE,&block_before_meta_write,NULL,NULL,NULL,NULL,NULL,NULL)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't insert metadata cache dxpl property")

    /* Insert 'library internal' property */
    if(H5P_insert(xfer_plist,H5AC_LIBRARY_INTERNAL_NAME,H5AC_LIBRARY_INTERNAL_SIZE,&library_internal,NULL,NULL,NULL,NULL,NULL,NULL)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't insert metadata cache dxpl property")

    /* Set the transfer mode */
    xfer_mode=H5FD_MPIO_INDEPENDENT;
    if (H5P_set(xfer_plist,H5D_XFER_IO_XFER_MODE_NAME,&xfer_mode)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value")

done:
    FUNC_LEAVE_NOAPI(ret_value)

#else /* H5_HAVE_PARALLEL */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5AC_init_interface)

    /* Sanity check */
    assert(H5P_LST_DATASET_XFER_g!=(-1));

    H5AC_dxpl_id=H5P_DATASET_XFER_DEFAULT;
    H5AC_noblock_dxpl_id=H5P_DATASET_XFER_DEFAULT;
    H5AC_ind_dxpl_id=H5P_DATASET_XFER_DEFAULT;

    FUNC_LEAVE_NOAPI(SUCCEED)
#endif /* H5_HAVE_PARALLEL */
} /* end H5AC_init_interface() */


/*-------------------------------------------------------------------------
 * Function:	H5AC_term_interface
 *
 * Purpose:	Terminate this interface.
 *
 * Return:	Success:	Positive if anything was done that might
 *				affect other interfaces; zero otherwise.
 *
 * 		Failure:	Negative.
 *
 * Programmer:	Quincey Koziol
 *              Thursday, July 18, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5AC_term_interface(void)
{
    int		n=0;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5AC_term_interface)

    if (H5_interface_initialize_g) {
#ifdef H5_HAVE_PARALLEL
        if(H5AC_dxpl_id>0 || H5AC_noblock_dxpl_id>0 || H5AC_ind_dxpl_id>0) {
            /* Indicate more work to do */
            n = 1; /* H5I */

            /* Close H5AC dxpl */
            if (H5I_dec_ref(H5AC_dxpl_id, FALSE) < 0 ||
                    H5I_dec_ref(H5AC_noblock_dxpl_id, FALSE) < 0 ||
                    H5I_dec_ref(H5AC_ind_dxpl_id, FALSE) < 0)
                H5E_clear_stack(NULL); /*ignore error*/
            else {
                /* Reset static IDs */
                H5AC_dxpl_id=(-1);
                H5AC_noblock_dxpl_id=(-1);
                H5AC_ind_dxpl_id=(-1);

                /* Reset interface initialization flag */
                H5_interface_initialize_g = 0;
            } /* end else */
        } /* end if */
        else
#else /* H5_HAVE_PARALLEL */
            /* Reset static IDs */
            H5AC_dxpl_id=(-1);
            H5AC_noblock_dxpl_id=(-1);
            H5AC_ind_dxpl_id=(-1);
#endif /* H5_HAVE_PARALLEL */
            /* Reset interface initialization flag */
            H5_interface_initialize_g = 0;
    } /* end if */

    FUNC_LEAVE_NOAPI(n)
} /* end H5AC_term_interface() */


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
 * Modifications:
 *
 *		Complete re-design and re-write to support the re-designed
 *		metadata cache.
 *
 *		At present, the size_hint is ignored, and the
 *		max_cache_size and min_clean_size fields are hard
 *		coded.  This should be fixed, but a parameter
 *		list change will be required, so I will leave it
 *		for now.
 *
 *		Since no-one seems to care, the function now returns
 *		one on success.
 *						JRM - 4/28/04
 *
 *		Reworked the function again after abstracting its guts to
 *		the similar function in H5C.c.  The function is now a
 *		wrapper for H5C_create().
 *						JRM - 6/4/04
 *
 *		Deleted the old size_hint parameter and added the
 *		max_cache_size, and min_clean_size parameters.
 *
 *                                              JRM - 3/10/05
 *
 *		Deleted the max_cache_size, and min_clean_size parameters,
 *		and added the config_ptr parameter.  Added code to
 *		validate the resize configuration before we do anything.
 *
 *						JRM - 3/24/05
 *
 *		Changed the type of config_ptr from H5AC_auto_size_ctl_t *
 *		to H5AC_cache_config_t *.  Propagated associated changes
 *		through the function.
 *						JRM - 4/7/05
 *
 *		Added code allocating and initializing the auxilary
 *		structure (an instance of H5AC_aux_t), and linking it
 *		to the instance of H5C_t created by H5C_create().  At
 *		present, the auxilary structure is only used in PHDF5.
 *
 *						JRM - 6/28/05
 *
 *		Added code to set the prefix if required.
 *
 *						JRM - 1/20/06
 *
 *		Added code to initialize the new write_done field.
 *
 *						JRM - 5/11/06
 *
 *-------------------------------------------------------------------------
 */

static const char * H5AC_entry_type_names[H5AC_NTYPES] =
{
    "B-tree nodes",
    "symbol table nodes",
    "local heap prefixes",
    "local heap data blocks",
    "global heaps",
    "object headers",
    "object header chunks",
    "v2 B-tree headers",
    "v2 B-tree internal nodes",
    "v2 B-tree leaf nodes",
    "fractal heap headers",
    "fractal heap direct blocks",
    "fractal heap indirect blocks",
    "free space headers",
    "free space sections",
    "shared OH message master table",
    "shared OH message index",
    "superblock",
    "test entry"	/* for testing only -- not used for actual files */
};

herr_t
H5AC_create(const H5F_t *f,
            H5AC_cache_config_t *config_ptr)
{
    herr_t ret_value = SUCCEED;      /* Return value */
    herr_t result;
#ifdef H5_HAVE_PARALLEL
    char 	 prefix[H5C__PREFIX_LEN] = "";
    MPI_Comm	 mpi_comm = MPI_COMM_NULL;
    int		 mpi_rank = -1;
    int	 	 mpi_size = -1;
    H5AC_aux_t * aux_ptr = NULL;
#endif /* H5_HAVE_PARALLEL */

    FUNC_ENTER_NOAPI(H5AC_create, FAIL)

    HDassert(f);
    HDassert(NULL == f->shared->cache);
    HDassert(config_ptr != NULL) ;
    HDcompile_assert(NELMTS(H5AC_entry_type_names) == H5AC_NTYPES);
    HDcompile_assert(H5C__MAX_NUM_TYPE_IDS == H5AC_NTYPES);

    result = H5AC_validate_config(config_ptr);

    if ( result != SUCCEED ) {

        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "Bad cache configuration");
    }

#ifdef H5_HAVE_PARALLEL
    if ( IS_H5FD_MPI(f) ) {

        if ( (mpi_comm = H5F_mpi_get_comm(f)) == MPI_COMM_NULL ) {

            HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, \
                        "can't get MPI communicator")
        }

        if ( (mpi_rank = H5F_mpi_get_rank(f)) < 0 ) {

            HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "can't get mpi rank")
        }

        if ( (mpi_size = H5F_mpi_get_size(f)) < 0 ) {

            HGOTO_ERROR(H5E_VFL, H5E_CANTGET, FAIL, "can't get mpi size")
        }

        /* There is no point in setting up the auxilary structure if size
         * is less than or equal to 1, as there will never be any processes
         * to broadcast the clean lists to.
         */
        if ( mpi_size > 1 ) {

            if ( NULL == (aux_ptr = H5FL_CALLOC(H5AC_aux_t)) ) {

                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, \
                            "Can't allocate H5AC auxilary structure.")

            } else {

                aux_ptr->magic = H5AC__H5AC_AUX_T_MAGIC;
                aux_ptr->mpi_comm = mpi_comm;
                aux_ptr->mpi_rank = mpi_rank;
                aux_ptr->mpi_size = mpi_size;
                aux_ptr->write_permitted = FALSE;
                aux_ptr->dirty_bytes_threshold =
			H5AC__DEFAULT_DIRTY_BYTES_THRESHOLD;
                aux_ptr->dirty_bytes = 0;
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
                aux_ptr->d_slist_len = 0;
                aux_ptr->c_slist_ptr = NULL;
                aux_ptr->c_slist_len = 0;
		aux_ptr->write_done = NULL;

		sprintf(prefix, "%d:", mpi_rank);
            }

            if ( mpi_rank == 0 ) {

                aux_ptr->d_slist_ptr =
                    H5SL_create(H5SL_TYPE_HADDR);

                if ( aux_ptr->d_slist_ptr == NULL ) {

                    HGOTO_ERROR(H5E_CACHE, H5E_CANTCREATE, FAIL,
                                "can't create dirtied entry list.")
                }

                aux_ptr->c_slist_ptr =
                    H5SL_create(H5SL_TYPE_HADDR);

                if ( aux_ptr->c_slist_ptr == NULL ) {

                    HGOTO_ERROR(H5E_CACHE, H5E_CANTCREATE, FAIL,
                                "can't create cleaned entry list.")
                }
            }
        }

        if ( aux_ptr != NULL ) {

            if ( aux_ptr->mpi_rank == 0 ) {

                f->shared->cache = H5C_create(H5AC__DEFAULT_MAX_CACHE_SIZE,
                                        H5AC__DEFAULT_MIN_CLEAN_SIZE,
                                        (H5AC_NTYPES - 1),
                                        (const char **)H5AC_entry_type_names,
                                        H5AC_check_if_write_permitted,
                                        TRUE,
                                        H5AC_log_flushed_entry,
                                        (void *)aux_ptr);

            } else {

                f->shared->cache = H5C_create(H5AC__DEFAULT_MAX_CACHE_SIZE,
                                        H5AC__DEFAULT_MIN_CLEAN_SIZE,
                                        (H5AC_NTYPES - 1),
                                        (const char **)H5AC_entry_type_names,
                                        NULL,
                                        FALSE,
#if 0 /* this is useful debugging code -- keep it for a while */ /* JRM */
                                        H5AC_log_flushed_entry_dummy,
#else /* JRM */
                                        NULL,
#endif /* JRM */
                                        (void *)aux_ptr);
            }

        } else {

            f->shared->cache = H5C_create(H5AC__DEFAULT_MAX_CACHE_SIZE,
                                        H5AC__DEFAULT_MIN_CLEAN_SIZE,
                                        (H5AC_NTYPES - 1),
                                        (const char **)H5AC_entry_type_names,
                                        H5AC_check_if_write_permitted,
                                        TRUE,
                                        NULL,
                                        NULL);
        }
    } else {
#endif /* H5_HAVE_PARALLEL */
        /* The default max cache size and min clean size will frequently be
         * overwritten shortly by the subsequent set resize config call.
         *                                             -- JRM
         */
        f->shared->cache = H5C_create(H5AC__DEFAULT_MAX_CACHE_SIZE,
                                        H5AC__DEFAULT_MIN_CLEAN_SIZE,
                                        (H5AC_NTYPES - 1),
                                        (const char **)H5AC_entry_type_names,
                                        H5AC_check_if_write_permitted,
                                        TRUE,
                                        NULL,
                                        NULL);
#ifdef H5_HAVE_PARALLEL
    }
#endif /* H5_HAVE_PARALLEL */

    if ( NULL == f->shared->cache ) {

	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

    }
#ifdef H5_HAVE_PARALLEL
    else if ( aux_ptr != NULL ) {

        result = H5C_set_prefix(f->shared->cache, prefix);

        if ( result != SUCCEED ) {

            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, \
                        "H5C_set_prefix() failed")
        }
    }
#endif /* H5_HAVE_PARALLEL */

    result = H5AC_set_cache_auto_resize_config(f->shared->cache, config_ptr);

    if ( result != SUCCEED ) {

        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, \
                    "auto resize configuration failed")
    }

done:

#ifdef H5_HAVE_PARALLEL

    /* if there is a failure, try to tidy up the auxilary structure */

    if ( ret_value != SUCCEED ) {

        if ( aux_ptr != NULL ) {

            if ( aux_ptr->d_slist_ptr != NULL ) {

                H5SL_close(aux_ptr->d_slist_ptr);
            }

            if ( aux_ptr->c_slist_ptr != NULL ) {

                H5SL_close(aux_ptr->c_slist_ptr);
            }

            aux_ptr->magic = 0;
            H5FL_FREE(H5AC_aux_t, aux_ptr);
            aux_ptr = NULL;
        }
    }
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
H5AC_dest(H5F_t *f, hid_t dxpl_id)
{
#ifdef H5_HAVE_PARALLEL
    H5AC_aux_t * aux_ptr = NULL;
#endif /* H5_HAVE_PARALLEL */
    herr_t ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(H5AC_dest, FAIL)

    /* Sanity check */
    HDassert(f);
    HDassert(f->shared->cache);

#if H5AC_DUMP_STATS_ON_CLOSE
    /* Dump debugging info */
    H5AC_stats(f);
#endif /* H5AC_DUMP_STATS_ON_CLOSE */

#if H5AC__TRACE_FILE_ENABLED
    if(H5AC_close_trace_file(f->shared->cache) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5AC_close_trace_file() failed.")
#endif /* H5AC__TRACE_FILE_ENABLED */

#ifdef H5_HAVE_PARALLEL
    aux_ptr = f->shared->cache->aux_ptr;
    if(aux_ptr)
        /* Sanity check */
        HDassert(aux_ptr->magic == H5AC__H5AC_AUX_T_MAGIC);

    /* Attempt to flush all entries from rank 0 & Bcast clean list to other ranks */
    if(H5AC_flush_entries(f) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "Can't flush.")
#endif /* H5_HAVE_PARALLEL */

    /* Destroy the cache */
    if(H5C_dest(f, dxpl_id, H5AC_noblock_dxpl_id) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTFREE, FAIL, "can't destroy cache")
    f->shared->cache = NULL;

#ifdef H5_HAVE_PARALLEL
    if(aux_ptr != NULL) {
        if(aux_ptr->d_slist_ptr != NULL)
            H5SL_close(aux_ptr->d_slist_ptr);
        if(aux_ptr->c_slist_ptr != NULL)
            H5SL_close(aux_ptr->c_slist_ptr);
        aux_ptr->magic = 0;
        H5FL_FREE(H5AC_aux_t, aux_ptr);
        aux_ptr = NULL;
    } /* end if */
#endif /* H5_HAVE_PARALLEL */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_dest() */


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
 * Modifications:
 *
 *		Added 'flags' paramater, to allow freeing file space
 *
 *						    QAK - 2/5/08
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_expunge_entry(H5F_t *f,
		   hid_t dxpl_id,
		   const H5AC_class_t *type,
		   haddr_t addr,
                   unsigned flags)
{
    herr_t  result;
#if H5AC__TRACE_FILE_ENABLED
    char                trace[128] = "";
    FILE *              trace_file_ptr = NULL;
#endif /* H5AC__TRACE_FILE_ENABLED */
    herr_t  ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(H5AC_expunge_entry, FAIL)

    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->cache);
    HDassert(type);
    HDassert(type->clear);
    HDassert(type->dest);
    HDassert(H5F_addr_defined(addr));

#if H5AC__TRACE_FILE_ENABLED
{
    H5AC_t * cache_ptr = f->shared->cache;

    /* For the expunge entry call, only the addr, and type id are really
     * necessary in the trace file.  Write the return value to catch occult
     * errors.
     */
    if ( ( cache_ptr != NULL ) &&
         ( H5C_get_trace_file_ptr(cache_ptr, &trace_file_ptr) >= 0 ) &&
         ( trace_file_ptr != NULL ) ) {

        sprintf(trace, "H5AC_expunge_entry 0x%lx %d",
	        (unsigned long)addr,
		(int)(type->id));
    }
}
#endif /* H5AC__TRACE_FILE_ENABLED */

    result = H5C_expunge_entry(f,
                               dxpl_id,
                               H5AC_noblock_dxpl_id,
                               type,
                               addr,
                               flags);

    if ( result < 0 ) {

        HGOTO_ERROR(H5E_CACHE, H5E_CANTEXPUNGE, FAIL, \
                    "H5C_expunge_entry() failed.")
    }

done:

#if H5AC__TRACE_FILE_ENABLED
    if ( trace_file_ptr != NULL ) {

	HDfprintf(trace_file_ptr, "%s %d\n", trace, (int)ret_value);
    }
#endif /* H5AC__TRACE_FILE_ENABLED */

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
H5AC_flush(H5F_t *f, hid_t dxpl_id)
{
#if H5AC__TRACE_FILE_ENABLED
    char 	  trace[128] = "";
    FILE *	  trace_file_ptr = NULL;
#endif /* H5AC__TRACE_FILE_ENABLED */
    herr_t	  ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(H5AC_flush, FAIL)

    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->cache);

#if H5AC__TRACE_FILE_ENABLED
    /* For the flush, only the flags are really necessary in the trace file.
     * Write the result to catch occult errors.
     */
    if((f != NULL) &&
            (f->shared != NULL) &&
            (f->shared->cache != NULL) &&
            (H5C_get_trace_file_ptr(f->shared->cache, &trace_file_ptr) >= 0) &&
            (trace_file_ptr != NULL))
	sprintf(trace, "H5AC_flush");
#endif /* H5AC__TRACE_FILE_ENABLED */

#ifdef H5_HAVE_PARALLEL
    /* Attempt to flush all entries from rank 0 & Bcast clean list to other ranks */
    if(H5AC_flush_entries(f) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "Can't flush.")
#endif /* H5_HAVE_PARALLEL */

    /* Flush the cache */
    if(H5C_flush_cache(f, dxpl_id, H5AC_noblock_dxpl_id, H5AC__NO_FLAGS_SET) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "Can't flush cache.")

done:
#if H5AC__TRACE_FILE_ENABLED
    if(trace_file_ptr != NULL)
        HDfprintf(trace_file_ptr, "%s %d\n", trace, (int)ret_value);
#endif /* H5AC__TRACE_FILE_ENABLED */

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_flush() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_get_entry_status
 *
 * Purpose:     Given a file address, determine whether the metadata
 * 		cache contains an entry at that location.  If it does,
 * 		also determine whether the entry is dirty, protected,
 * 		pinned, etc. and return that information to the caller
 * 		in *status_ptr.
 *
 * 		If the specified entry doesn't exist, set *status_ptr
 * 		to zero.
 *
 * 		On error, the value of *status_ptr is undefined.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              4/27/06
 *
 * Modifications:
 *
 *		None.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_get_entry_status(const H5F_t *f,
                      haddr_t    addr,
		      unsigned * status_ptr)
{
    H5C_t      *cache_ptr = f->shared->cache;
    herr_t	result;
    hbool_t	in_cache;
    hbool_t	is_dirty;
    hbool_t	is_protected;
    hbool_t	is_pinned;
    size_t	entry_size;
    unsigned	status = 0;
    herr_t      ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(H5AC_get_entry_status, FAIL)

    if ( ( cache_ptr == NULL ) ||
         ( cache_ptr->magic != H5C__H5C_T_MAGIC ) ||
	 ( ! H5F_addr_defined(addr) ) ||
	 ( status_ptr == NULL ) ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Bad param(s) on entry.")
    }

    result = H5C_get_entry_status(f, addr, &entry_size, &in_cache,
		                  &is_dirty, &is_protected, &is_pinned);

    if ( result < 0 ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                    "H5C_get_entry_status() failed.")
    }

    if ( in_cache ) {

	status |= H5AC_ES__IN_CACHE;

	if ( is_dirty )
	    status |= H5AC_ES__IS_DIRTY;

	if ( is_protected )
	    status |= H5AC_ES__IS_PROTECTED;

	if ( is_pinned )
	    status |= H5AC_ES__IS_PINNED;
    }

    *status_ptr = status;

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5AC_get_entry_status() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_set
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
 * Modifications:
 * 		Robb Matzke, 1999-07-27
 *		The ADDR argument is passed by value.
 *
 *		Bill Wendling, 2003-09-16
 *		Added automatic "flush" if the FPHDF5 driver is being
 *		used. This'll write the metadata to the SAP where other,
 *		lesser processes can grab it.
 *
 *		JRM - 5/13/04
 *		Complete re-write for the new metadata cache.  The new
 *		code is functionally almost identical to the old, although
 *		the sanity check for a protected entry is now an assert
 *		at the beginning of the function.
 *
 *		JRM - 6/7/04
 *		Abstracted the guts of the function to H5C_insert_entry()
 *		in H5C.c, and then re-wrote the function as a wrapper for
 *		H5C_insert_entry().
 *
 *		JRM - 1/6/05
 *		Added the flags parameter.  At present, this parameter is
 *		only used to set the new flush_marker field on the new
 *		entry.  Since this doesn't apply to the SAP code, no change
 *		is needed there.  Thus the only change to the body of the
 *		code is to pass the flags parameter through to
 *		H5C_insert_entry().
 *
 *		JRM - 6/6/05
 *		Added code to force newly inserted entries to be dirty
 *		in the flexible parallel case.  The normal case is handled
 *		in H5C.c.  This is part of a series of changes directed at
 *		moving management of the dirty flag on cache entries into
 *		the cache code.
 *
 *              JRM - 7/5/05
 *              Added code to track dirty byte generation, and to trigger
 *              clean entry list propagation when it exceeds a user
 *              specified threshold.  Note that this code only applies in
 *              the PHDF5 case.  It should have no effect on either the
 *              serial or FPHSD5 cases.
 *
 *              JRM - 6/6/06
 *              Added trace file support.
 *
 *-------------------------------------------------------------------------
 */

herr_t
H5AC_set(H5F_t *f, hid_t dxpl_id, const H5AC_class_t *type, haddr_t addr, void *thing, unsigned int flags)
{
    herr_t		result;
    H5AC_info_t        *info;
    herr_t ret_value=SUCCEED;      /* Return value */
#ifdef H5_HAVE_PARALLEL
    H5AC_aux_t        * aux_ptr = NULL;
#endif /* H5_HAVE_PARALLEL */
#if H5AC__TRACE_FILE_ENABLED
    char          	trace[128] = "";
    size_t              trace_entry_size = 0;
    FILE *        	trace_file_ptr = NULL;
#endif /* H5AC__TRACE_FILE_ENABLED */

    FUNC_ENTER_NOAPI(H5AC_set, FAIL)

    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->cache);
    HDassert(type);
    HDassert(type->flush);
    HDassert(type->size);
    HDassert(H5F_addr_defined(addr));
    HDassert(thing);

    /* Check for invalid access request */
    if(0 == (f->intent & H5F_ACC_RDWR))
	HGOTO_ERROR(H5E_CACHE, H5E_BADVALUE, FAIL, "no write intent on file")

#if H5AC__TRACE_FILE_ENABLED
    /* For the insert, only the addr, size, type id and flags are really
     * necessary in the trace file.  Write the result to catch occult
     * errors.
     *
     * Note that some data is not available right now -- put what we can
     * in the trace buffer now, and fill in the rest at the end.
     */
    if ( ( f != NULL ) &&
         ( f->shared != NULL ) &&
         ( f->shared->cache != NULL ) &&
         ( H5C_get_trace_file_ptr(f->shared->cache, &trace_file_ptr) >= 0) &&
         ( trace_file_ptr != NULL ) ) {

        sprintf(trace, "H5AC_set 0x%lx %d 0x%x",
	        (unsigned long)addr,
		type->id,
		flags);
    }
#endif /* H5AC__TRACE_FILE_ENABLED */

    /* Get local copy of this information */
    info = (H5AC_info_t *)thing;

    info->addr = addr;
    info->type = type;
    info->is_protected = FALSE;

#ifdef H5_HAVE_PARALLEL
    if ( NULL != (aux_ptr = f->shared->cache->aux_ptr) ) {

        result = H5AC_log_inserted_entry(f,
                                         f->shared->cache,
                                         (H5AC_info_t *)thing,
                                         type,
                                         addr);

        if ( result < 0 ) {

            HGOTO_ERROR(H5E_CACHE, H5E_CANTINS, FAIL, \
                    "H5AC_log_inserted_entry() failed.")
        }
    }
#endif /* H5_HAVE_PARALLEL */

    result = H5C_insert_entry(f,
                              dxpl_id,
                              H5AC_noblock_dxpl_id,
                              type,
                              addr,
                              thing,
                              flags);

    if ( result < 0 ) {

        HGOTO_ERROR(H5E_CACHE, H5E_CANTINS, FAIL, "H5C_insert_entry() failed")
    }

#if H5AC__TRACE_FILE_ENABLED
    if ( trace_file_ptr != NULL ) {

        /* make note of the entry size */
        trace_entry_size = ((H5C_cache_entry_t *)thing)->size;
    }
#endif /* H5AC__TRACE_FILE_ENABLED */

#ifdef H5_HAVE_PARALLEL
    /* Check if we should try to flush */
    if(aux_ptr && (aux_ptr->dirty_bytes >= aux_ptr->dirty_bytes_threshold)) {
        hbool_t evictions_enabled;

        /* Query if evictions are allowed */
        if(H5C_get_evictions_enabled((const H5C_t *)f->shared->cache, &evictions_enabled) < 0)
            HGOTO_ERROR(H5E_CACHE, H5E_CANTGET, FAIL, "H5C_get_evictions_enabled() failed.")

        /* Flush if evictions are allowed */
        if(evictions_enabled) {
            if(H5AC_propagate_flushed_and_still_clean_entries_list(f,
                    H5AC_noblock_dxpl_id, f->shared->cache, TRUE) < 0 )
                HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "Can't propagate clean entries list.")
        } /* end if */
    } /* end if */
#endif /* H5_HAVE_PARALLEL */

done:

#if H5AC__TRACE_FILE_ENABLED
    if ( trace_file_ptr != NULL ) {

	HDfprintf(trace_file_ptr, "%s %d %d\n", trace,
                  (int)trace_entry_size,
		  (int)ret_value);
    }
#endif /* H5AC__TRACE_FILE_ENABLED */

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5AC_set() */


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
#if H5AC__TRACE_FILE_ENABLED
    char          	trace[128] = "";
    FILE *        	trace_file_ptr = NULL;
#endif /* H5AC__TRACE_FILE_ENABLED */
    herr_t              ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5AC_mark_entry_dirty, FAIL)

    /* Sanity check */
    HDassert(thing);

#if H5AC__TRACE_FILE_ENABLED
    /* For the mark pinned or protected entry dirty call, only the addr
     * is really necessary in the trace file.  Write the result to catch
     * occult errors.
     */
    if((H5C_get_trace_file_ptr_from_entry(thing, &trace_file_ptr) >= 0) &&
         (NULL != trace_file_ptr))
        sprintf(trace, "%s 0x%lx", FUNC,
	        (unsigned long)(((H5C_cache_entry_t *)thing)->addr));
#endif /* H5AC__TRACE_FILE_ENABLED */

#ifdef H5_HAVE_PARALLEL
{
    H5AC_info_t *entry_ptr = (H5AC_info_t *)thing;
    H5C_t *cache_ptr = entry_ptr->cache_ptr;

    HDassert(cache_ptr);
    HDassert(cache_ptr->magic == H5C__H5C_T_MAGIC);

    if((!entry_ptr->is_dirty) && (!entry_ptr->is_protected) &&
             (entry_ptr->is_pinned) && (NULL != cache_ptr->aux_ptr)) {
        if(H5AC_log_dirtied_entry(entry_ptr, entry_ptr->addr, FALSE, 0) < 0)
            HGOTO_ERROR(H5E_CACHE, H5E_CANTMARKDIRTY, FAIL, "can't log dirtied entry")
    } /* end if */
}
#endif /* H5_HAVE_PARALLEL */

    if(H5C_mark_entry_dirty(thing) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTMARKDIRTY, FAIL, "can't mark pinned or protected entry dirty")

done:
#if H5AC__TRACE_FILE_ENABLED
    if(trace_file_ptr)
	HDfprintf(trace_file_ptr, "%s %d\n", trace, (int)ret_value);
#endif /* H5AC__TRACE_FILE_ENABLED */

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_mark_entry_dirty() */


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
    herr_t		result;
    herr_t ret_value=SUCCEED;      /* Return value */
#ifdef H5_HAVE_PARALLEL
    H5AC_aux_t        * aux_ptr = NULL;
#endif /* H5_HAVE_PARALLEL */
#if H5AC__TRACE_FILE_ENABLED
    char          	trace[128] = "";
    FILE *        	trace_file_ptr = NULL;
#endif /* H5AC__TRACE_FILE_ENABLED */

    FUNC_ENTER_NOAPI(H5AC_move_entry, FAIL)

    HDassert(f);
    HDassert(f->shared->cache);
    HDassert(type);
    HDassert(H5F_addr_defined(old_addr));
    HDassert(H5F_addr_defined(new_addr));
    HDassert(H5F_addr_ne(old_addr, new_addr));

#if H5AC__TRACE_FILE_ENABLED
    /* For the move call, only the old addr and new addr are really
     * necessary in the trace file.  Include the type id so we don't have to
     * look it up.  Also write the result to catch occult errors.
     */
    if ( ( f != NULL ) &&
         ( f->shared != NULL ) &&
         ( f->shared->cache != NULL ) &&
         ( H5C_get_trace_file_ptr(f->shared->cache, &trace_file_ptr) >= 0) &&
         ( trace_file_ptr != NULL ) ) {

        sprintf(trace, "H5AC_move_entry 0x%lx 0x%lx %d",
	        (unsigned long)old_addr,
		(unsigned long)new_addr,
		(int)(type->id));
    }
#endif /* H5AC__TRACE_FILE_ENABLED */

#ifdef H5_HAVE_PARALLEL
    if ( NULL != (aux_ptr = f->shared->cache->aux_ptr) ) {
        if(H5AC_log_moved_entry(f, old_addr, new_addr) < 0)
            HGOTO_ERROR(H5E_CACHE, H5E_CANTUNPROTECT, FAIL, "can't log moved entry")
    }
#endif /* H5_HAVE_PARALLEL */

    result = H5C_move_entry(f->shared->cache,
                              type,
                              old_addr,
                              new_addr);

    if ( result < 0 ) {

        HGOTO_ERROR(H5E_CACHE, H5E_CANTMOVE, FAIL, \
                    "H5C_move_entry() failed.")
    }

#ifdef H5_HAVE_PARALLEL
    /* Check if we should try to flush */
    if(aux_ptr && (aux_ptr->dirty_bytes >= aux_ptr->dirty_bytes_threshold)) {
        hbool_t evictions_enabled;

        /* Query if evictions are allowed */
        if(H5C_get_evictions_enabled((const H5C_t *)f->shared->cache, &evictions_enabled) < 0)
            HGOTO_ERROR(H5E_CACHE, H5E_CANTGET, FAIL, "H5C_get_evictions_enabled() failed.")

        /* Flush if evictions are allowed */
        if(evictions_enabled) {
            if(H5AC_propagate_flushed_and_still_clean_entries_list(f,
                    H5AC_noblock_dxpl_id, f->shared->cache, TRUE) < 0)
                HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "Can't propagate clean entries list.")
        } /* end if */
    } /* end if */
#endif /* H5_HAVE_PARALLEL */

done:

#if H5AC__TRACE_FILE_ENABLED
    if ( trace_file_ptr != NULL ) {

	HDfprintf(trace_file_ptr, "%s %d\n", trace, (int)ret_value);
    }
#endif /* H5AC__TRACE_FILE_ENABLED */

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
#if H5AC__TRACE_FILE_ENABLED
    char        trace[128] = "";
    FILE *      trace_file_ptr = NULL;
#endif /* H5AC__TRACE_FILE_ENABLED */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5AC_pin_protected_entry, FAIL)

    /* Sanity check */
    HDassert(thing);

#if H5AC__TRACE_FILE_ENABLED
    /* For the pin protected entry call, only the addr is really necessary
     * in the trace file.  Also write the result to catch occult errors.
     */
    if((H5C_get_trace_file_ptr_from_entry(thing, &trace_file_ptr) >= 0) &&
            (NULL != trace_file_ptr))
        sprintf(trace, "%s 0x%lx", FUNC,
	        (unsigned long)(((H5C_cache_entry_t *)thing)->addr));
#endif /* H5AC__TRACE_FILE_ENABLED */

    if(H5C_pin_protected_entry(thing) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTPIN, FAIL, "can't pin entry")

done:
#if H5AC__TRACE_FILE_ENABLED
    if(trace_file_ptr)
	HDfprintf(trace_file_ptr, "%s %d\n", trace, (int)ret_value);
#endif /* H5AC__TRACE_FILE_ENABLED */

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_pin_protected_entry() */


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
 *		This comment is a re-write of the original Purpose: section.
 *		For historical interest, the original version is reproduced
 *		below:
 *
 *		Original Purpose section:
 *
 *              Similar to H5AC_find() except the object is removed from
 *              the cache and given to the caller, preventing other parts
 *              of the program from modifying the protected object or
 *              preempting it from the cache.
 *
 *              The caller must call H5AC_unprotect() when finished with
 *              the pointer.
 *
 * Return:      Success:        Ptr to the object.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Sep  2 1997
 *
 * Modifications:
 *		Robb Matzke, 1999-07-27
 *		The ADDR argument is passed by value.
 *
 *              Bill Wendling, 2003-09-10
 *              Added parameter to indicate whether this is a READ or
 *              WRITE type of protect.
 *
 *		JRM -- 5/17/04
 *		Complete re-write for the new client cache.  See revised
 *		Purpose section above.
 *
 *		JRM - 6/7/04
 *		Abstracted the guts of the function to H5C_protect()
 *		in H5C.c, and then re-wrote the function as a wrapper for
 *		H5C_protect().
 *
 *		JRM - 6/6/06
 *		Added trace file support.
 *
 *		JRM - 3/18/07
 *		Modified code to support the new flags parameter for
 *		H5C_protect().  For now, that means passing in the
 *		H5C_READ_ONLY_FLAG if rw == H5AC_READ.
 *
 *		Also updated the trace file output to save the
 *		rw parameter, since we are now doing something with it.
 *
 *-------------------------------------------------------------------------
 */
void *
H5AC_protect(H5F_t *f,
             hid_t dxpl_id,
             const H5AC_class_t *type,
             haddr_t addr,
	     void *udata,
             H5AC_protect_t rw)
{
    /* char *		fcn_name = "H5AC_protect"; */
    unsigned		protect_flags = H5C__NO_FLAGS_SET;
    void *		thing = (void *)NULL;
    void *		ret_value;      /* Return value */
#if H5AC__TRACE_FILE_ENABLED
    char                trace[128] = "";
    size_t		trace_entry_size = 0;
    FILE *              trace_file_ptr = NULL;
#endif /* H5AC__TRACE_FILE_ENABLED */

    FUNC_ENTER_NOAPI(H5AC_protect, NULL)

    /* check args */
    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->cache);
    HDassert(type);
    HDassert(type->flush);
    HDassert(type->load);
    HDassert(H5F_addr_defined(addr));

    /* Check for invalid access request */
    if(0 == (f->intent & H5F_ACC_RDWR) && rw == H5AC_WRITE)
	HGOTO_ERROR(H5E_CACHE, H5E_BADVALUE, NULL, "no write intent on file")

#if H5AC__TRACE_FILE_ENABLED
    /* For the protect call, only the addr and type id is really necessary
     * in the trace file.  Include the size of the entry protected as a
     * sanity check.  Also indicate whether the call was successful to
     * catch occult errors.
     */
    if ( ( f != NULL ) &&
         ( f->shared != NULL ) &&
         ( f->shared->cache != NULL ) &&
         ( H5C_get_trace_file_ptr(f->shared->cache, &trace_file_ptr) >= 0) &&
         ( trace_file_ptr != NULL ) ) {

	char * rw_string;

        if ( rw == H5AC_WRITE ) {

	    rw_string = "H5AC_WRITE";

	} else if ( rw == H5AC_READ ) {

	    rw_string = "H5AC_READ";

	} else {

	    rw_string = "???";
	}

        sprintf(trace, "H5AC_protect 0x%lx %d %s",
	        (unsigned long)addr,
		(int)(type->id),
		rw_string);
    }
#endif /* H5AC__TRACE_FILE_ENABLED */

    if ( rw == H5AC_READ ) {

	protect_flags |= H5C__READ_ONLY_FLAG;
    }

    thing = H5C_protect(f,
                        dxpl_id,
                        H5AC_noblock_dxpl_id,
                        type,
                        addr,
                        udata,
			protect_flags);

    if ( thing == NULL ) {

        HGOTO_ERROR(H5E_CACHE, H5E_CANTPROTECT, NULL, "H5C_protect() failed.")
    }

#if H5AC__TRACE_FILE_ENABLED
    if ( trace_file_ptr != NULL ) {

        /* make note of the entry size */
        trace_entry_size = ((H5C_cache_entry_t *)thing)->size;
    }
#endif /* H5AC__TRACE_FILE_ENABLED */

    /* Set return value */
    ret_value = thing;

done:

#if H5AC__TRACE_FILE_ENABLED
    if ( trace_file_ptr != NULL ) {

	HDfprintf(trace_file_ptr, "%s %d %d\n", trace,
                  (int)trace_entry_size,
                  (int)(ret_value != NULL));
    }
#endif /* H5AC__TRACE_FILE_ENABLED */

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
#if H5AC__TRACE_FILE_ENABLED
    char          	trace[128] = "";
    FILE *        	trace_file_ptr = NULL;
#endif /* H5AC__TRACE_FILE_ENABLED */
    herr_t              ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5AC_resize_entry, FAIL)

    /* Sanity check */
    HDassert(thing);

#if H5AC__TRACE_FILE_ENABLED
    /* For the resize pinned entry call, only the addr, and new_size are
     * really necessary in the trace file. Write the result to catch
     * occult errors.
     */
    if((H5C_get_trace_file_ptr_from_entry(thing, &trace_file_ptr) >= 0) &&
            (NULL != trace_file_ptr))
        sprintf(trace, "%s 0x%lx %d", FUNC,
	        (unsigned long)(((H5C_cache_entry_t *)thing)->addr),
		(int)new_size);
#endif /* H5AC__TRACE_FILE_ENABLED */

#ifdef H5_HAVE_PARALLEL
{
    H5AC_info_t * entry_ptr = (H5AC_info_t *)thing;
    H5C_t *cache_ptr = entry_ptr->cache_ptr;

    HDassert(cache_ptr);
    HDassert(cache_ptr->magic == H5C__H5C_T_MAGIC);

    if((!entry_ptr->is_dirty) && (NULL != cache_ptr->aux_ptr)) {
        /* Check for usage errors */
        if(!(entry_ptr->is_pinned || entry_ptr->is_protected))
            HGOTO_ERROR(H5E_CACHE, H5E_CANTRESIZE, FAIL, "Entry isn't pinned or protected??")

        if(H5AC_log_dirtied_entry(entry_ptr, entry_ptr->addr, TRUE, new_size) < 0)
            HGOTO_ERROR(H5E_CACHE, H5E_CANTMARKDIRTY, FAIL, "can't log dirtied entry")
    } /* end if */
}
#endif /* H5_HAVE_PARALLEL */

    if(H5C_resize_entry(thing, new_size) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTRESIZE, FAIL, "can't resize entry")

done:
#if H5AC__TRACE_FILE_ENABLED
    if(trace_file_ptr)
	HDfprintf(trace_file_ptr, "%s %d\n", trace, (int)ret_value);
#endif /* H5AC__TRACE_FILE_ENABLED */

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
#if H5AC__TRACE_FILE_ENABLED
    char                trace[128] = "";
    FILE *              trace_file_ptr = NULL;
#endif /* H5AC__TRACE_FILE_ENABLED */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5AC_unpin_entry, FAIL)

    /* Sanity check */
    HDassert(thing);

#if H5AC__TRACE_FILE_ENABLED
    /* For the unpin entry call, only the addr is really necessary
     * in the trace file.  Also write the result to catch occult errors.
     */
    if((H5C_get_trace_file_ptr_from_entry(thing, &trace_file_ptr) >= 0) &&
            (NULL != trace_file_ptr))
        sprintf(trace, "%s 0x%lx", FUNC,
	        (unsigned long)(((H5C_cache_entry_t *)thing)->addr));
#endif /* H5AC__TRACE_FILE_ENABLED */

    if(H5C_unpin_entry(thing) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTUNPIN, FAIL, "can't unpin entry")

done:
#if H5AC__TRACE_FILE_ENABLED
    if(trace_file_ptr)
	HDfprintf(trace_file_ptr, "%s %d\n", trace, (int)ret_value);
#endif /* H5AC__TRACE_FILE_ENABLED */

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_unpin_entry() */


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
 *		This verion of the function is a complete re-write to
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
 * Modifications:
 *		Robb Matzke, 1999-07-27
 *		The ADDR argument is passed by value.
 *
 *		Quincey Koziol, 2003-03-19
 *		Added "deleted" argument
 *
 *              Bill Wendling, 2003-09-18
 *              If this is an FPHDF5 driver and the data is dirty,
 *              perform a "flush" that writes the data to the SAP.
 *
 *		John Mainzer 5/19/04
 *		Complete re-write for the new metadata cache.
 *
 *		JRM - 6/7/04
 *		Abstracted the guts of the function to H5C_unprotect()
 *		in H5C.c, and then re-wrote the function as a wrapper for
 *		H5C_unprotect().
 *
 *		JRM - 1/6/05
 *		Replaced the deleted parameter with the new flags parameter.
 *		Since the deleted parameter is not used by the FPHDF5 code,
 *		the only change in the body is to replace the deleted
 *		parameter with the flags parameter in the call to
 *		H5C_unprotect().
 *
 *		JRM - 6/6/05
 *		Added the dirtied flag and supporting code.  This is
 *		part of a collection of changes directed at moving
 *		management of cache entry dirty flags into the H5C code.
 *
 *		JRM - 7/5/05
 *		Added code to track dirty byte generation, and to trigger
 *		clean entry list propagation when it exceeds a user
 *		specified threshold.  Note that this code only applies in
 *		the PHDF5 case.  It should have no effect on either the
 *		serial or FPHSD5 cases.
 *
 *		JRM - 9/8/05
 *		Added code to track entry size changes.  This is necessary
 *		as it can effect dirty byte creation counts, thereby
 *		throwing the caches out of sync in the PHDF5 case.
 *
 *		JRM - 5/16/06
 *		Added code to use the new dirtied field in
 *		H5C_cache_entry_t in the test to see if the entry has
 *		been dirtied.
 *
 *		JRM - 6/7/06
 *		Added support for the trace file.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_unprotect(H5F_t *f, hid_t dxpl_id, const H5AC_class_t *type, haddr_t addr,
    void *thing, unsigned flags)
{
    herr_t		result;
    hbool_t		dirtied;
    size_t		new_size = 0;
#ifdef H5_HAVE_PARALLEL
    hbool_t		size_changed = FALSE;
    H5AC_aux_t        * aux_ptr = NULL;
#endif /* H5_HAVE_PARALLEL */
#if H5AC__TRACE_FILE_ENABLED
    char                trace[128] = "";
    size_t		trace_new_size = 0;
    unsigned		trace_flags = 0;
    FILE *              trace_file_ptr = NULL;
#endif /* H5AC__TRACE_FILE_ENABLED */
    herr_t              ret_value=SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(H5AC_unprotect, FAIL)

    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->cache);
    HDassert(type);
    HDassert(type->clear);
    HDassert(type->flush);
    HDassert(H5F_addr_defined(addr));
    HDassert(thing);
    HDassert( ((H5AC_info_t *)thing)->addr == addr );
    HDassert( ((H5AC_info_t *)thing)->type == type );

#if H5AC__TRACE_FILE_ENABLED
    /* For the unprotect call, only the addr, type id, flags, and possible
     * new size are really necessary in the trace file.  Write the return
     * value to catch occult errors.
     */
    if ( ( f != NULL ) &&
         ( f->shared != NULL ) &&
         ( f->shared->cache != NULL ) &&
         ( H5C_get_trace_file_ptr(f->shared->cache, &trace_file_ptr) >= 0) &&
         ( trace_file_ptr != NULL ) ) {

        sprintf(trace, "H5AC_unprotect 0x%lx %d",
	        (unsigned long)addr,
		(int)(type->id));

	trace_flags = flags;
    }
#endif /* H5AC__TRACE_FILE_ENABLED */

    dirtied = (hbool_t)( ( (flags & H5AC__DIRTIED_FLAG) == H5AC__DIRTIED_FLAG ) ||
		( ((H5AC_info_t *)thing)->dirtied ) );

    if ( dirtied ) {

        if ( (type->size)(f, thing, &new_size) < 0 ) {

            HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGETSIZE, FAIL, \
                        "Can't get size of thing")
        }

        if ( ((H5AC_info_t *)thing)->size != new_size ) {

#ifdef H5_HAVE_PARALLEL
            size_changed = TRUE;
#endif /* H5_HAVE_PARALLEL */
            flags = flags | H5AC__SIZE_CHANGED_FLAG;
#if H5AC__TRACE_FILE_ENABLED
	    trace_flags = flags;
	    trace_new_size = new_size;
#endif /* H5AC__TRACE_FILE_ENABLED */
        }
    }

#ifdef H5_HAVE_PARALLEL
    if ( ( dirtied ) && ( ((H5AC_info_t *)thing)->is_dirty == FALSE ) &&
         ( NULL != (aux_ptr = f->shared->cache->aux_ptr) ) ) {
        if(H5AC_log_dirtied_entry((H5AC_info_t *)thing, addr, size_changed, new_size) < 0)
            HGOTO_ERROR(H5E_CACHE, H5E_CANTUNPROTECT, FAIL, "can't log dirtied entry")
    }

    if ( ( (flags & H5C__DELETED_FLAG) != 0 ) &&
         ( NULL != (aux_ptr = f->shared->cache->aux_ptr) ) &&
         ( aux_ptr->mpi_rank == 0 ) ) {

        result = H5AC_log_deleted_entry(f->shared->cache,
                                        (H5AC_info_t *)thing,
                                        addr,
                                        flags);

        if ( result < 0 ) {

            HGOTO_ERROR(H5E_CACHE, H5E_CANTUNPROTECT, FAIL, \
                    "H5AC_log_deleted_entry() failed.")
        }
    }
#endif /* H5_HAVE_PARALLEL */

    result = H5C_unprotect(f,
                           dxpl_id,
                           H5AC_noblock_dxpl_id,
                           type,
                           addr,
                           thing,
                           flags,
                           new_size);

    if ( result < 0 ) {

        HGOTO_ERROR(H5E_CACHE, H5E_CANTUNPROTECT, FAIL, \
                    "H5C_unprotect() failed.")
    }

#ifdef H5_HAVE_PARALLEL
    /* Check if we should try to flush */
    if(aux_ptr && (aux_ptr->dirty_bytes >= aux_ptr->dirty_bytes_threshold)) {
        hbool_t evictions_enabled;

        /* Query if evictions are allowed */
        if(H5C_get_evictions_enabled((const H5C_t *)f->shared->cache, &evictions_enabled) < 0)
            HGOTO_ERROR(H5E_CACHE, H5E_CANTGET, FAIL, "H5C_get_evictions_enabled() failed.")

        /* Flush if evictions are allowed */
        if(evictions_enabled) {
            if(H5AC_propagate_flushed_and_still_clean_entries_list(f,
                    H5AC_noblock_dxpl_id, f->shared->cache, TRUE) < 0)
                HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "Can't propagate clean entries list.")
        } /* end if */
    } /* end if */
#endif /* H5_HAVE_PARALLEL */

done:

#if H5AC__TRACE_FILE_ENABLED
    if ( trace_file_ptr != NULL ) {

	HDfprintf(trace_file_ptr, "%s %d %x %d\n",
		  trace,
		  (int)trace_new_size,
		  (unsigned)trace_flags,
		  (int)ret_value);
    }
#endif /* H5AC__TRACE_FILE_ENABLED */

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5AC_unprotect() */


/*-------------------------------------------------------------------------
 * Function:    HA5C_set_write_done_callback
 *
 * Purpose:     Set the value of the write_done callback.  This callback
 *              is used to improve performance of the parallel test bed
 *              for the cache.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              5/11/06
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */

#ifdef H5_HAVE_PARALLEL
herr_t
H5AC_set_write_done_callback(H5C_t * cache_ptr,
                             void (* write_done)(void))
{
    herr_t       ret_value = SUCCEED;   /* Return value */
    H5AC_aux_t * aux_ptr = NULL;

    FUNC_ENTER_NOAPI(H5AC_set_write_done_callback, FAIL)

    /* This would normally be an assert, but we need to use an HGOTO_ERROR
     * call to shut up the compiler.
     */
    if ( ( ! cache_ptr ) || ( cache_ptr->magic != H5C__H5C_T_MAGIC ) ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Bad cache_ptr")
    }

    aux_ptr = cache_ptr->aux_ptr;

    HDassert( aux_ptr != NULL );
    HDassert( aux_ptr->magic == H5AC__H5AC_AUX_T_MAGIC );

    aux_ptr->write_done = write_done;

done:
    FUNC_LEAVE_NOAPI(ret_value)

} /* H5AC_set_write_done_callback() */
#endif /* H5_HAVE_PARALLEL */


/*-------------------------------------------------------------------------
 * Function:    H5AC_stats
 *
 * Purpose:     Prints statistics about the cache.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, October 30, 1997
 *
 * Modifications:
 *		John Mainzer 5/19/04
 *		Re-write to support the new metadata cache.
 *
 *		JRM - 6/7/04
 *		Abstracted the guts of the function to H5C_stats()
 *		in H5C.c, and then re-wrote the function as a wrapper for
 *		H5C_stats().
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_stats(const H5F_t *f)
{
    herr_t		ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5AC_stats, FAIL)

    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->cache);

    /* at present, this can't fail */
    (void)H5C_stats(f->shared->cache, H5F_OPEN_NAME(f), FALSE);

done:
    FUNC_LEAVE_NOAPI(ret_value)

} /* H5AC_stats() */


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
 * Modifications:
 *
 *		JRM - 4/6/05
 *              Reworked for the addition of struct H5AC_cache_config_t.
 *
 *		JRM - 10/25/05
 *		Added support for the new dirty_bytes_threshold field of
 *		both H5AC_cache_config_t and H5AC_aux_t.
 *
 *		JRM - 6/8/06
 *		Added support for the new trace file related fields.
 *
 *		JRM - 7/28/07
 *		Added support for the new evictions enabled related fields.
 *
 *		Observe that H5AC_get_cache_auto_resize_config() and
 *		H5AC_set_cache_auto_resize_config() are becoming generic
 *		metadata cache configuration routines as they gain
 *		switches for functions that are only tenuously related
 *		to auto resize configuration.
 *
 *		JRM - 1/2/08
 *		Added support for the new flash cache increment related
 *		fields.
 *
 *-------------------------------------------------------------------------
 */

herr_t
H5AC_get_cache_auto_resize_config(const H5AC_t * cache_ptr,
                                  H5AC_cache_config_t *config_ptr)
{
    herr_t result;
    herr_t ret_value = SUCCEED;      /* Return value */
    hbool_t evictions_enabled;
    H5C_auto_size_ctl_t internal_config;

    FUNC_ENTER_NOAPI(H5AC_get_cache_auto_resize_config, FAIL)

    if ( ( cache_ptr == NULL )
         ||
#ifdef H5_HAVE_PARALLEL
         ( ( cache_ptr->aux_ptr != NULL )
           &&
           ( ((H5AC_aux_t *)(cache_ptr->aux_ptr))->magic
             !=
             H5AC__H5AC_AUX_T_MAGIC
           )
         )
         ||
#endif /* H5_HAVE_PARALLEL */
         ( config_ptr == NULL )
         ||
         ( config_ptr->version != H5AC__CURR_CACHE_CONFIG_VERSION )
       )
    {
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                    "Bad cache_ptr or config_ptr on entry.")

    }

    result = H5C_get_cache_auto_resize_config((const H5C_t *)cache_ptr,
					      &internal_config);

    if ( result < 0 ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                    "H5C_get_cache_auto_resize_config() failed.")
    }

    result = H5C_get_evictions_enabled((const H5C_t *)cache_ptr, &evictions_enabled);

    if ( result < 0 ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
		    "H5C_get_resize_enabled() failed.")
    }

    if ( internal_config.rpt_fcn == NULL ) {

        config_ptr->rpt_fcn_enabled = FALSE;

    } else {

	config_ptr->rpt_fcn_enabled = TRUE;
    }

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
    config_ptr->epochs_before_eviction =
                                  (int)(internal_config.epochs_before_eviction);
    config_ptr->apply_empty_reserve    = internal_config.apply_empty_reserve;
    config_ptr->empty_reserve          = internal_config.empty_reserve;

#ifdef H5_HAVE_PARALLEL
    if ( cache_ptr->aux_ptr != NULL ) {

        config_ptr->dirty_bytes_threshold =
	    ((H5AC_aux_t *)(cache_ptr->aux_ptr))->dirty_bytes_threshold;

    } else {
#endif /* H5_HAVE_PARALLEL */

        config_ptr->dirty_bytes_threshold = H5AC__DEFAULT_DIRTY_BYTES_THRESHOLD;

#ifdef H5_HAVE_PARALLEL
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
 * Modifications:
 *
 *              None.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_get_cache_size(H5AC_t * cache_ptr,
                    size_t * max_size_ptr,
                    size_t * min_clean_size_ptr,
                    size_t * cur_size_ptr,
                    int32_t * cur_num_entries_ptr)
{
    herr_t result;
    herr_t ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(H5AC_get_cache_size, FAIL)

    result = H5C_get_cache_size((H5C_t *)cache_ptr,
				max_size_ptr,
				min_clean_size_ptr,
				cur_size_ptr,
				cur_num_entries_ptr);

    if ( result < 0 ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                    "H5C_get_cache_size() failed.")
    }

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
 * Modifications:
 *
 *              None.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_get_cache_hit_rate(H5AC_t * cache_ptr, double * hit_rate_ptr)
{
    herr_t ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(H5AC_get_cache_hit_rate, FAIL)

    if(H5C_get_cache_hit_rate((H5C_t *)cache_ptr, hit_rate_ptr) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5C_get_cache_hit_rate() failed.")

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
 * Modifications:
 *
 *              None.
 *
 *-------------------------------------------------------------------------
 */

herr_t
H5AC_reset_cache_hit_rate_stats(H5AC_t * cache_ptr)
{
    herr_t result;
    herr_t      ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(H5AC_reset_cache_hit_rate_stats, FAIL)

    result = H5C_reset_cache_hit_rate_stats((H5C_t *)cache_ptr);

    if ( result < 0 ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                    "H5C_reset_cache_hit_rate_stats() failed.")
    }

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
 * Modifications:
 *
 *              John Mainzer -- 4/6/05
 *              Updated for the addition of H5AC_cache_config_t.
 *
 *		John Mainzer -- 10/25/05
 *		Added support for the new dirty_bytes_threshold field of
 *		both H5AC_cache_config_t and H5AC_aux_t.
 *
 *		John Mainzer -- 6/7/06
 *		Added trace file support.
 *
 *		John Mainzer -- 7/28/07
 *		Added support for the new evictions enabled related fields.
 *
 *		Observe that H5AC_get_cache_auto_resize_config() and
 *		H5AC_set_cache_auto_resize_config() are becoming generic
 *		metadata cache configuration routines as they gain
 *		switches for functions that are only tenuously related
 *		to auto resize configuration.
 *
 *		John Mainzer -- 1/3/07
 *		Updated trace file code to record the new flash cache
 *		size increase related fields.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_set_cache_auto_resize_config(H5AC_t *cache_ptr,
                                  H5AC_cache_config_t *config_ptr)
{
    herr_t              result;
    herr_t              ret_value = SUCCEED;      /* Return value */
    H5C_auto_size_ctl_t internal_config;
#if H5AC__TRACE_FILE_ENABLED
    H5AC_cache_config_t trace_config = H5AC__DEFAULT_CACHE_CONFIG;
    FILE *              trace_file_ptr = NULL;
#endif /* H5AC__TRACE_FILE_ENABLED */

    FUNC_ENTER_NOAPI(H5AC_set_cache_auto_resize_config, FAIL)

    HDassert( cache_ptr );

#if H5AC__TRACE_FILE_ENABLED
    /* Make note of the new configuration.  Don't look up the trace file
     * pointer, as that may change before we use it.
     */
    if ( config_ptr != NULL ) {

        trace_config = *config_ptr;

    }
#endif /* H5AC__TRACE_FILE_ENABLED */

    if ( ( cache_ptr == NULL )
#ifdef H5_HAVE_PARALLEL
         ||
         ( ( cache_ptr->aux_ptr != NULL )
           &&
           (
             ((H5AC_aux_t *)(cache_ptr->aux_ptr))->magic
             !=
             H5AC__H5AC_AUX_T_MAGIC
           )
         )
#endif /* H5_HAVE_PARALLEL */
       ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "bad cache_ptr on entry.")
    }

    result = H5AC_validate_config(config_ptr);

    if ( result != SUCCEED ) {

        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "Bad cache configuration");
    }

    if ( config_ptr->open_trace_file ) {

	FILE * file_ptr = NULL;

	if ( H5C_get_trace_file_ptr(cache_ptr, &file_ptr) < 0 ) {

	    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
			"H5C_get_trace_file_ptr() failed.")
	}

	if ( ( ! ( config_ptr->close_trace_file ) ) &&
	     ( file_ptr != NULL ) ) {

            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, \
                        "Trace file already open.")
        }
    }

    if (
         (
           config_ptr->dirty_bytes_threshold
           <
           H5AC__MIN_DIRTY_BYTES_THRESHOLD
         )
         ||
         (
           config_ptr->dirty_bytes_threshold
           >
           H5AC__MAX_DIRTY_BYTES_THRESHOLD
         )
       ) {

        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "config_ptr->dirty_bytes_threshold out of range.")
    }

    if ( config_ptr->close_trace_file ) {

	if ( H5AC_close_trace_file(cache_ptr) < 0 ) {

            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                        "H5AC_close_trace_file() failed.")
	}
    }

    if ( config_ptr->open_trace_file ) {

        if ( H5AC_open_trace_file(cache_ptr, config_ptr->trace_file_name) < 0 )
	{

	    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, \
                        "H5AC_open_trace_file() failed.")
	}
    }

    if(H5AC_ext_config_2_int_config(config_ptr, &internal_config) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5AC_ext_config_2_int_config() failed.")

    if(H5C_set_cache_auto_resize_config(cache_ptr, &internal_config) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5C_set_cache_auto_resize_config() failed.")

    if(H5C_set_evictions_enabled(cache_ptr, config_ptr->evictions_enabled) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5C_set_evictions_enabled() failed.")

#ifdef H5_HAVE_PARALLEL
    if ( cache_ptr->aux_ptr != NULL ) {

        ((H5AC_aux_t *)(cache_ptr->aux_ptr))->dirty_bytes_threshold =
            config_ptr->dirty_bytes_threshold;
    }
#endif /* H5_HAVE_PARALLEL */

done:

#if H5AC__TRACE_FILE_ENABLED
    /* For the set cache auto resize config call, only the contents
     * of the config is necessary in the trace file. Write the return
     * value to catch occult errors.
     */
    if ( ( cache_ptr != NULL ) &&
         ( H5C_get_trace_file_ptr(cache_ptr, &trace_file_ptr) >= 0 ) &&
         ( trace_file_ptr != NULL ) ) {

	HDfprintf(trace_file_ptr,
                  "%s %d %d %d %d \"%s\" %d %d %d %f %d %d %ld %d %f %f %d %f %f %d %d %d %f %f %d %d %d %d %f %d %d\n",
		  "H5AC_set_cache_auto_resize_config",
		  trace_config.version,
		  (int)(trace_config.rpt_fcn_enabled),
		  (int)(trace_config.open_trace_file),
		  (int)(trace_config.close_trace_file),
		  trace_config.trace_file_name,
		  (int)(trace_config.evictions_enabled),
		  (int)(trace_config.set_initial_size),
		  (int)(trace_config.initial_size),
		  trace_config.min_clean_fraction,
		  (int)(trace_config.max_size),
		  (int)(trace_config.min_size),
		  trace_config.epoch_length,
		  (int)(trace_config.incr_mode),
		  trace_config.lower_hr_threshold,
		  trace_config.increment,
		  (int)(trace_config.flash_incr_mode),
		  trace_config.flash_multiple,
		  trace_config.flash_threshold,
		  (int)(trace_config.apply_max_increment),
		  (int)(trace_config.max_increment),
		  (int)(trace_config.decr_mode),
		  trace_config.upper_hr_threshold,
		  trace_config.decrement,
		  (int)(trace_config.apply_max_decrement),
		  (int)(trace_config.max_decrement),
		  trace_config.epochs_before_eviction,
		  (int)(trace_config.apply_empty_reserve),
		  trace_config.empty_reserve,
		  trace_config.dirty_bytes_threshold,
		  (int)ret_value);
    }
#endif /* H5AC__TRACE_FILE_ENABLED */

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
 * Modifications:
 *
 *            - Added code testing the trace file configuration fields.
 *              These tests are not comprehensive, as many errors cannot
 *              be caught until the directives contained in these fields
 *              are applied.
 *              					JRM - 5/15/06
 *
 *	      - Added code testing the evictions enabled field.  At
 *	        present this consists of verifying that if
 *	        evictions_enabled is FALSE, then automatic cache
 *		resizing in disabled.
 *
 *	        					JRM - 7/28/07
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_validate_config(H5AC_cache_config_t * config_ptr)
{
    herr_t              result;
    H5C_auto_size_ctl_t internal_config;
    herr_t              ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5AC_validate_config, FAIL)

    if ( config_ptr == NULL ) {

        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "NULL config_ptr on entry.")
    }

    if ( config_ptr->version != H5AC__CURR_CACHE_CONFIG_VERSION ) {

        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "Unknown config version.")
    }

    if ( ( config_ptr->rpt_fcn_enabled != TRUE ) &&
         ( config_ptr->rpt_fcn_enabled != FALSE ) ) {

        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, \
                    "config_ptr->rpt_fcn_enabled must be either TRUE or FALSE.")
    }

    if ( ( config_ptr->open_trace_file != TRUE ) &&
         ( config_ptr->open_trace_file != FALSE ) ) {

        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, \
                    "config_ptr->open_trace_file must be either TRUE or FALSE.")
    }

    if ( ( config_ptr->close_trace_file != TRUE ) &&
         ( config_ptr->close_trace_file != FALSE ) ) {

        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, \
                  "config_ptr->close_trace_file must be either TRUE or FALSE.")
    }

    /* don't bother to test trace_file_name unless open_trace_file is TRUE */
    if ( config_ptr->open_trace_file ) {
        size_t	        name_len;

	/* Can't really test the trace_file_name field without trying to
	 * open the file, so we will content ourselves with a couple of
	 * sanity checks on the length of the file name.
	 */
	name_len = HDstrlen(config_ptr->trace_file_name);

	if ( name_len == 0 ) {

            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, \
                        "config_ptr->trace_file_name is empty.")

        } else if ( name_len > H5AC__MAX_TRACE_FILE_NAME_LEN ) {

            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, \
                        "config_ptr->trace_file_name too long.")
	}
    }

    if ( ( config_ptr->evictions_enabled != TRUE ) &&
         ( config_ptr->evictions_enabled != FALSE ) ) {

        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, \
            "config_ptr->evictions_enabled must be either TRUE or FALSE.")
    }

    if ( ( config_ptr->evictions_enabled == FALSE ) &&
	 ( ( config_ptr->incr_mode != H5C_incr__off ) ||
	   ( config_ptr->flash_incr_mode != H5C_flash_incr__off ) ||
	   ( config_ptr->decr_mode != H5C_decr__off ) ) ) {

        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, \
                    "Can't disable evictions while auto-resize is enabled.")
    }

    if ( config_ptr->dirty_bytes_threshold < H5AC__MIN_DIRTY_BYTES_THRESHOLD ) {

        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "dirty_bytes_threshold too small.")
    } else
    if ( config_ptr->dirty_bytes_threshold > H5AC__MAX_DIRTY_BYTES_THRESHOLD ) {

        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "dirty_bytes_threshold too big.")
    }

    if ( H5AC_ext_config_2_int_config(config_ptr, &internal_config) !=
         SUCCEED ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                    "H5AC_ext_config_2_int_config() failed.")
    }

    result = H5C_validate_resize_config(&internal_config,
                                        H5C_RESIZE_CFG__VALIDATE_ALL);

    if ( result != SUCCEED ) {

        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "error(s) in new config.")
    }

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5AC_validate_config() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_close_trace_file()
 *
 * Purpose:     If a trace file is open, stop logging calls to the cache,
 *              and close the file.
 *
 *              Note that the function does nothing if there is no trace
 *              file.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              6/2/06
 *
 * Modifications:
 *
 *              None.
 *
 *-------------------------------------------------------------------------
 */

herr_t
H5AC_close_trace_file(H5AC_t * cache_ptr)

{
    herr_t   ret_value = SUCCEED;    /* Return value */
    FILE *   trace_file_ptr = NULL;

    FUNC_ENTER_NOAPI(H5AC_close_trace_file, FAIL)

    if ( cache_ptr == NULL ) {

        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "NULL cache_ptr on entry.")
    }

    if ( H5C_get_trace_file_ptr(cache_ptr, &trace_file_ptr) < 0 ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
		    "H5C_get_trace_file_ptr() failed.")
    }

    if ( trace_file_ptr != NULL ) {

        if ( H5C_set_trace_file_ptr(cache_ptr, NULL) < 0 ) {

            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                        "H5C_set_trace_file_ptr() failed.")
         }

        if ( HDfclose(trace_file_ptr) != 0 ) {

            HGOTO_ERROR(H5E_FILE, H5E_CANTCLOSEFILE, FAIL, \
                        "can't close metadata cache trace file")
        }
    }

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5AC_close_trace_file() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_open_trace_file()
 *
 * Purpose:     Open a trace file, and start logging calls to the cache.
 *
 * 		This logging is done at the H5C level, and will only take
 * 		place if H5C_TRACE_FILE_ENABLED (defined in H5Cprivate.h)
 * 		is TRUE.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              6/1/06
 *
 * Modifications:
 *
 *              None.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_open_trace_file(H5AC_t * cache_ptr,
		     const char * trace_file_name)
{
    herr_t   ret_value = SUCCEED;    /* Return value */
    char     file_name[H5AC__MAX_TRACE_FILE_NAME_LEN + H5C__PREFIX_LEN + 2];
    FILE *   file_ptr = NULL;
#ifdef H5_HAVE_PARALLEL
    H5AC_aux_t * aux_ptr = NULL;
#endif /* H5_HAVE_PARALLEL */

    FUNC_ENTER_NOAPI(H5AC_open_trace_file, FAIL)

    HDassert(cache_ptr);

    if ( cache_ptr == NULL ) {

        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "cache_ptr NULL on entry.")
    }

    if ( trace_file_name == NULL ) {

        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, \
		    "NULL trace_file_name on entry.")
    }

    if ( HDstrlen(trace_file_name) > H5AC__MAX_TRACE_FILE_NAME_LEN ) {

        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "trace file name too long.")
    }

    if ( H5C_get_trace_file_ptr(cache_ptr, &file_ptr) < 0 ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
		    "H5C_get_trace_file_ptr() failed.")
    }

    if ( file_ptr != NULL ) {

        HGOTO_ERROR(H5E_FILE, H5E_FILEOPEN, FAIL, "trace file already open.")
    }

#ifdef H5_HAVE_PARALLEL

    aux_ptr = (H5AC_aux_t *)(cache_ptr->aux_ptr);

    if ( cache_ptr->aux_ptr == NULL ) {

        sprintf(file_name, "%s", trace_file_name);

    } else {

	if ( aux_ptr->magic != H5AC__H5AC_AUX_T_MAGIC ) {

            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Bad aux_ptr->magic.")
	}

        sprintf(file_name, "%s.%d", trace_file_name, aux_ptr->mpi_rank);

    }

    if ( HDstrlen(file_name) >
         H5AC__MAX_TRACE_FILE_NAME_LEN + H5C__PREFIX_LEN + 1 ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
		    "cooked trace file name too long.")
    }

#else /* H5_HAVE_PARALLEL */

    sprintf(file_name, "%s", trace_file_name);

#endif /* H5_HAVE_PARALLEL */

    if ( (file_ptr = HDfopen(file_name, "w")) == NULL ) {

	/* trace file open failed */
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, FAIL, "trace file open failed.")
    }

    HDfprintf(file_ptr, "### HDF5 metadata cache trace file ###\n");

    if ( H5C_set_trace_file_ptr(cache_ptr, file_ptr) < 0 ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
		    "H5C_set_trace_file_ptr() failed.")
    }

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5AC_open_trace_file() */


/*************************************************************************/
/**************************** Private Functions: *************************/
/*************************************************************************/

/*-------------------------------------------------------------------------
 *
 * Function:    H5AC_broadcast_clean_list()
 *
 * Purpose:     Broadcast the contents of the process 0 cleaned entry
 *		slist.  In passing, also remove all entries from said
 *		list, and also remove any matching entries from the dirtied
 *		slist.
 *
 *		This function must only be called by the process with
 *		MPI_rank 0.
 *
 *		Return SUCCEED on success, and FAIL on failure.
 *
 * Return:      Non-negative on success/Negative on failure.
 *
 * Programmer:  John Mainzer, 7/1/05
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */

#ifdef H5_HAVE_PARALLEL
static herr_t
H5AC_broadcast_clean_list(H5AC_t * cache_ptr)
{
    herr_t               ret_value = SUCCEED;    /* Return value */
    haddr_t		 addr;
    H5AC_aux_t         * aux_ptr = NULL;
    H5SL_node_t        * slist_node_ptr = NULL;
    H5AC_slist_entry_t * slist_entry_ptr = NULL;
    MPI_Offset         * buf_ptr = NULL;
    size_t		 buf_size;
    int                  i = 0;
    int                  mpi_result;
    int			 num_entries;

    FUNC_ENTER_NOAPI(H5AC_broadcast_clean_list, FAIL)

    HDassert( cache_ptr != NULL );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );

    aux_ptr = (H5AC_aux_t *)(cache_ptr->aux_ptr);

    HDassert( aux_ptr != NULL );
    HDassert( aux_ptr->magic == H5AC__H5AC_AUX_T_MAGIC );
    HDassert( aux_ptr->mpi_rank == 0 );
    HDassert( aux_ptr->c_slist_ptr != NULL );
    HDassert( H5SL_count(aux_ptr->c_slist_ptr) ==
		    (size_t)(aux_ptr->c_slist_len) );


    /* First broadcast the number of entries in the list so that the
     * receives can set up a buffer to receive them.  If there aren't
     * any, we are done.
     */
    num_entries = aux_ptr->c_slist_len;

    mpi_result = MPI_Bcast(&num_entries, 1, MPI_INT, 0, aux_ptr->mpi_comm);

    if ( mpi_result != MPI_SUCCESS ) {

        HMPI_GOTO_ERROR(FAIL, "MPI_Bcast failed 1", mpi_result)

    }

    if ( num_entries > 0 )
    {
        /* allocate a buffer to store the list of entry base addresses in */

        buf_size = sizeof(MPI_Offset) * (size_t)num_entries;

        buf_ptr = (MPI_Offset *)H5MM_malloc(buf_size);

        if ( buf_ptr == NULL ) {

            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, \
                        "memory allocation failed for clean entry buffer")
        }

        /* now load the entry base addresses into the buffer, emptying the
         * cleaned entry list in passing
         */

        while ( NULL != (slist_node_ptr = H5SL_first(aux_ptr->c_slist_ptr) ) )
        {
            slist_entry_ptr = H5SL_item(slist_node_ptr);

            HDassert(slist_entry_ptr->magic == H5AC__H5AC_SLIST_ENTRY_T_MAGIC);

            HDassert( i < num_entries );

            addr = slist_entry_ptr->addr;

            if ( H5FD_mpi_haddr_to_MPIOff(addr, &(buf_ptr[i])) < 0 ) {

                HGOTO_ERROR(H5E_INTERNAL, H5E_BADRANGE, FAIL, \
                            "can't convert from haddr to MPI off")
            }

            i++;

            /* now remove the entry from the cleaned entry list */
            if ( H5SL_remove(aux_ptr->c_slist_ptr, (void *)(&addr))
                 != slist_entry_ptr ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTDELETE, FAIL, \
                            "Can't delete entry from cleaned entry slist.")
            }

            slist_entry_ptr->magic = 0;
            H5FL_FREE(H5AC_slist_entry_t, slist_entry_ptr);
            slist_entry_ptr = NULL;

            aux_ptr->c_slist_len -= 1;

            HDassert( aux_ptr->c_slist_len >= 0 );

            /* and also remove the matching entry from the dirtied list
             * if it exists.
             */
            if ( (slist_entry_ptr = H5SL_search(aux_ptr->d_slist_ptr,
                                                (void *)(&addr))) != NULL ) {

                HDassert( slist_entry_ptr->magic ==
                          H5AC__H5AC_SLIST_ENTRY_T_MAGIC );
                HDassert( slist_entry_ptr->addr == addr );

                if ( H5SL_remove(aux_ptr->d_slist_ptr, (void *)(&addr))
                         != slist_entry_ptr ) {

                    HGOTO_ERROR(H5E_CACHE, H5E_CANTDELETE, FAIL, \
                                "Can't delete entry from dirty entry slist.")
                }

                slist_entry_ptr->magic = 0;
                H5FL_FREE(H5AC_slist_entry_t, slist_entry_ptr);
                slist_entry_ptr = NULL;

                aux_ptr->d_slist_len -= 1;

                HDassert( aux_ptr->d_slist_len >= 0 );
            }

        } /* while */


        /* Now broadcast the list of cleaned entries -- if there is one.
         *
         * The peculiar structure of the following call to MPI_Bcast is
         * due to MPI's (?) failure to believe in the MPI_Offset type.
         * Thus the element type is MPI_BYTE, with size equal to the
         * buf_size computed above.
         */

        mpi_result = MPI_Bcast((void *)buf_ptr, (int)buf_size, MPI_BYTE, 0,
                               aux_ptr->mpi_comm);

        if ( mpi_result != MPI_SUCCESS ) {

            HMPI_GOTO_ERROR(FAIL, "MPI_Bcast failed 2", mpi_result)
        }
    }

done:

    if ( buf_ptr != NULL ) {

        buf_ptr = (MPI_Offset *)H5MM_xfree((void *)buf_ptr);
    }

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5AC_broadcast_clean_list() */
#endif /* H5_HAVE_PARALLEL */


/*-------------------------------------------------------------------------
 *
 * Function:    H5AC_check_if_write_permitted
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
 * Modifications:
 *
 *		John Mainzer, 9/23/05
 *		Rewrote function to return the value of the
 *		write_permitted field in aux structure if the structure
 *		exists and mpi_rank is 0.
 *
 *		If the aux structure exists, but mpi_rank isn't 0, the
 *		function now returns FALSE.
 *
 *		In all other cases, the function returns TRUE.
 *
 *-------------------------------------------------------------------------
 */

#ifdef H5_HAVE_PARALLEL
static herr_t
H5AC_check_if_write_permitted(const H5F_t *f,
                              hid_t UNUSED dxpl_id,
                              hbool_t * write_permitted_ptr)
#else /* H5_HAVE_PARALLEL */
static herr_t
H5AC_check_if_write_permitted(const H5F_t UNUSED * f,
                              hid_t UNUSED dxpl_id,
                              hbool_t * write_permitted_ptr)
#endif /* H5_HAVE_PARALLEL */
{
    hbool_t		write_permitted = TRUE;
    herr_t		ret_value = SUCCEED;      /* Return value */
#ifdef H5_HAVE_PARALLEL
    H5AC_aux_t *	aux_ptr = NULL;
#endif /* H5_HAVE_PARALLEL */


    FUNC_ENTER_NOAPI(H5AC_check_if_write_permitted, FAIL)

#ifdef H5_HAVE_PARALLEL
    HDassert( f != NULL );
    HDassert( f->shared != NULL );
    HDassert( f->shared->cache != NULL );

    aux_ptr = (H5AC_aux_t *)(f->shared->cache->aux_ptr);

    if ( aux_ptr != NULL ) {

        HDassert( aux_ptr->magic == H5AC__H5AC_AUX_T_MAGIC );

        if ( aux_ptr->mpi_rank == 0 ) {

	    write_permitted = aux_ptr->write_permitted;

        } else {

	    write_permitted = FALSE;
	}
    }
#endif /* H5_HAVE_PARALLEL */

    *write_permitted_ptr = write_permitted;

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5AC_check_if_write_permitted() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_ext_config_2_int_config()
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
 * Modifications:
 *
 *              Updated function for flash cache increment fields.
 *
 *                                              JRM -- 1/2/08
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_ext_config_2_int_config(H5AC_cache_config_t * ext_conf_ptr,
                             H5C_auto_size_ctl_t * int_conf_ptr)
{
    herr_t               ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5AC_ext_config_2_int_config, FAIL)

    if ( ( ext_conf_ptr == NULL ) ||
         ( ext_conf_ptr->version != H5AC__CURR_CACHE_CONFIG_VERSION ) ||
         ( int_conf_ptr == NULL ) ) {
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                    "Bad ext_conf_ptr or inf_conf_ptr on entry.")
    }

    int_conf_ptr->version                = H5C__CURR_AUTO_SIZE_CTL_VER;

    if ( ext_conf_ptr->rpt_fcn_enabled ) {

        int_conf_ptr->rpt_fcn            = H5C_def_auto_resize_rpt_fcn;

    } else {

        int_conf_ptr->rpt_fcn            = NULL;
    }

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
} /* H5AC_ext_config_2_int_config() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5AC_log_deleted_entry()
 *
 * Purpose:     Log an entry for which H5C__DELETED_FLAG has been set.
 *
 *		If mpi_rank is 0, we must make sure that the entry doesn't
 *		appear in the cleaned or dirty entry lists.  Otherwise,
 *		we have nothing to do.
 *
 *		Return SUCCEED on success, and FAIL on failure.
 *
 * Return:      Non-negative on success/Negative on failure.
 *
 * Programmer:  John Mainzer, 6/29/05
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */

#ifdef H5_HAVE_PARALLEL
static herr_t
H5AC_log_deleted_entry(H5AC_t * cache_ptr,
                       H5AC_info_t * entry_ptr,
                       haddr_t addr,
                       unsigned int flags)
{
    herr_t               ret_value = SUCCEED;    /* Return value */
    H5AC_aux_t         * aux_ptr = NULL;
    H5AC_slist_entry_t * slist_entry_ptr = NULL;

    FUNC_ENTER_NOAPI(H5AC_log_deleted_entry, FAIL)

    HDassert( cache_ptr != NULL );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );

    aux_ptr = cache_ptr->aux_ptr;

    HDassert( aux_ptr != NULL );
    HDassert( aux_ptr->magic == H5AC__H5AC_AUX_T_MAGIC );

    HDassert( entry_ptr != NULL );
    HDassert( entry_ptr->addr == addr );

    HDassert( (flags & H5C__DELETED_FLAG) != 0 );

    if ( aux_ptr->mpi_rank == 0 ) {

        HDassert( aux_ptr->d_slist_ptr != NULL );
        HDassert( aux_ptr->c_slist_ptr != NULL );

        /* if the entry appears in the dirtied entry slist, remove it. */
        if ( (slist_entry_ptr = H5SL_search(aux_ptr->d_slist_ptr,
                                            (void *)(&addr))) != NULL ) {

            HDassert( slist_entry_ptr->magic ==
                      H5AC__H5AC_SLIST_ENTRY_T_MAGIC );
            HDassert( slist_entry_ptr->addr == addr );

            if ( H5SL_remove(aux_ptr->d_slist_ptr, (void *)(&addr))
                     != slist_entry_ptr ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTDELETE, FAIL, \
                            "Can't delete entry from dirty entry slist.")
            }

            slist_entry_ptr->magic = 0;
            H5FL_FREE(H5AC_slist_entry_t, slist_entry_ptr);
            slist_entry_ptr = NULL;

            aux_ptr->d_slist_len -= 1;

            HDassert( aux_ptr->d_slist_len >= 0 );
        }

        /* if the entry appears in the cleaned entry slist, remove it. */
        if ( (slist_entry_ptr = H5SL_search(aux_ptr->c_slist_ptr,
                                            (void *)(&addr))) != NULL ) {

            HDassert( slist_entry_ptr->magic ==
                      H5AC__H5AC_SLIST_ENTRY_T_MAGIC );
            HDassert( slist_entry_ptr->addr == addr );

            if ( H5SL_remove(aux_ptr->c_slist_ptr, (void *)(&addr))
                     != slist_entry_ptr ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTDELETE, FAIL, \
                            "Can't delete entry from cleaned entry slist.")
            }

            slist_entry_ptr->magic = 0;
            H5FL_FREE(H5AC_slist_entry_t, slist_entry_ptr);
            slist_entry_ptr = NULL;

            aux_ptr->c_slist_len -= 1;

            HDassert( aux_ptr->c_slist_len >= 0 );
        }
    }

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5AC_log_deleted_entry() */
#endif /* H5_HAVE_PARALLEL */


/*-------------------------------------------------------------------------
 *
 * Function:    H5AC_log_dirtied_entry()
 *
 * Purpose:     Update the dirty_bytes count for a newly dirtied entry.
 *
 *		If mpi_rank isnt 0, this simply means adding the size
 *		of the entries to the dirty_bytes count.
 *
 *		If mpi_rank is 0, we must first check to see if the entry
 *		appears in the dirty entries slist.  If it is, do nothing.
 *		If it isn't, add the size to th dirty_bytes count, add the
 *		entry to the dirty entries slist, and remove it from the
 *		cleaned list (if it is present there).
 *
 *		Return SUCCEED on success, and FAIL on failure.
 *
 * Return:      Non-negative on success/Negative on failure.
 *
 * Programmer:  John Mainzer, 6/29/05
 *
 *-------------------------------------------------------------------------
 */
#ifdef H5_HAVE_PARALLEL
static herr_t
H5AC_log_dirtied_entry(const H5AC_info_t * entry_ptr,
                       haddr_t addr,
                       hbool_t size_changed,
                       size_t new_size)
{
    size_t		 entry_size;
    H5AC_t             * cache_ptr;
    H5AC_aux_t         * aux_ptr;
    herr_t               ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5AC_log_dirtied_entry, FAIL)

    HDassert( entry_ptr );
    HDassert( entry_ptr->addr == addr );
    HDassert( entry_ptr->is_dirty == FALSE );

    cache_ptr = entry_ptr->cache_ptr;

    HDassert( cache_ptr != NULL );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );

    aux_ptr = cache_ptr->aux_ptr;

    HDassert( aux_ptr != NULL );
    HDassert( aux_ptr->magic == H5AC__H5AC_AUX_T_MAGIC );

    if ( size_changed ) {

        entry_size = new_size;

    } else {

        entry_size = entry_ptr->size;
    }

    if ( aux_ptr->mpi_rank == 0 ) {
        H5AC_slist_entry_t * slist_entry_ptr;

        HDassert( aux_ptr->d_slist_ptr != NULL );
        HDassert( aux_ptr->c_slist_ptr != NULL );

        if ( H5SL_search(aux_ptr->d_slist_ptr, (void *)(&addr)) == NULL ) {

            /* insert the address of the entry in the dirty entry list, and
             * add its size to the dirty_bytes count.
             */
            if ( NULL == (slist_entry_ptr = H5FL_CALLOC(H5AC_slist_entry_t)) ) {

                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, \
                            "Can't allocate dirty slist entry .")
            }

            slist_entry_ptr->magic = H5AC__H5AC_SLIST_ENTRY_T_MAGIC;
            slist_entry_ptr->addr  = addr;

            if ( H5SL_insert(aux_ptr->d_slist_ptr, slist_entry_ptr,
                             &(slist_entry_ptr->addr)) < 0 ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTINSERT, FAIL, \
                            "can't insert entry into dirty entry slist.")
            }

            aux_ptr->d_slist_len += 1;
            aux_ptr->dirty_bytes += entry_size;
#if H5AC_DEBUG_DIRTY_BYTES_CREATION
	    aux_ptr->unprotect_dirty_bytes += entry_size;
	    aux_ptr->unprotect_dirty_bytes_updates += 1;
#endif /* H5AC_DEBUG_DIRTY_BYTES_CREATION */
        }

        if ( H5SL_search(aux_ptr->c_slist_ptr, (void *)(&addr)) != NULL ) {

            /* the entry is dirty.  If it exists on the cleaned entries list,
             * remove it.
             */
            if ( (slist_entry_ptr = H5SL_search(aux_ptr->c_slist_ptr,
                                                (void *)(&addr))) != NULL ) {

                HDassert( slist_entry_ptr->magic ==
                          H5AC__H5AC_SLIST_ENTRY_T_MAGIC );
                HDassert( slist_entry_ptr->addr == addr );

                if ( H5SL_remove(aux_ptr->c_slist_ptr, (void *)(&addr))
                     != slist_entry_ptr ) {

                    HGOTO_ERROR(H5E_CACHE, H5E_CANTDELETE, FAIL, \
                                "Can't delete entry from clean entry slist.")
                }

                slist_entry_ptr->magic = 0;
                H5FL_FREE(H5AC_slist_entry_t, slist_entry_ptr);
                slist_entry_ptr = NULL;

                aux_ptr->c_slist_len -= 1;

                HDassert( aux_ptr->c_slist_len >= 0 );
            }
        }
    } else {

        aux_ptr->dirty_bytes += entry_size;
#if H5AC_DEBUG_DIRTY_BYTES_CREATION
        aux_ptr->unprotect_dirty_bytes += entry_size;
        aux_ptr->unprotect_dirty_bytes_updates += 1;
#endif /* H5AC_DEBUG_DIRTY_BYTES_CREATION */
    }

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5AC_log_dirtied_entry() */
#endif /* H5_HAVE_PARALLEL */


/*-------------------------------------------------------------------------
 *
 * Function:    H5AC_log_flushed_entry()
 *
 * Purpose:     Update the clean entry slist for the flush of an entry --
 *		specifically, if the entry has been cleared, remove it
 * 		from both the cleaned and dirtied lists if it is present.
 *		Otherwise, if the entry was dirty, insert the indicated
 *		entry address in the clean slist if it isn't there already.
 *
 *		This function is only used in PHDF5, and should only
 *		be called for the process with mpi rank 0.
 *
 *		Return SUCCEED on success, and FAIL on failure.
 *
 * Return:      Non-negative on success/Negative on failure.
 *
 * Programmer:  John Mainzer, 6/29/05
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */

#ifdef H5_HAVE_PARALLEL
#if 0 /* This is useful debugging code. -- JRM */
static herr_t
H5AC_log_flushed_entry_dummy(H5C_t * cache_ptr,
                             haddr_t addr,
                             hbool_t was_dirty,
                             unsigned flags,
                             int type_id)
{
    herr_t               ret_value = SUCCEED;    /* Return value */
    H5AC_aux_t         * aux_ptr = NULL;

    FUNC_ENTER_NOAPI(H5AC_log_flushed_entry_dummy, FAIL)

    aux_ptr = cache_ptr->aux_ptr;

    if ( ( was_dirty ) && ( (flags & H5C__FLUSH_CLEAR_ONLY_FLAG) == 0 ) ) {

        HDfprintf(stdout,
         "%d:H5AC_log_flushed_entry(): addr = %d, flags = %x, was_dirty = %d, type_id = %d\n",
         (int)(aux_ptr->mpi_rank), (int)addr, flags, (int)was_dirty, type_id);
    }
done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5AC_log_flushed_entry_dummy() */
#endif /* JRM */

static herr_t
H5AC_log_flushed_entry(H5C_t * cache_ptr,
                       haddr_t addr,
                       hbool_t was_dirty,
                       unsigned flags,
                       int UNUSED type_id)
{
    herr_t               ret_value = SUCCEED;    /* Return value */
    hbool_t		 cleared;
    H5AC_aux_t         * aux_ptr;
    H5AC_slist_entry_t * slist_entry_ptr = NULL;


    FUNC_ENTER_NOAPI(H5AC_log_flushed_entry, FAIL)

    HDassert( cache_ptr != NULL );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );

    aux_ptr = cache_ptr->aux_ptr;

    HDassert( aux_ptr != NULL );
    HDassert( aux_ptr->magic == H5AC__H5AC_AUX_T_MAGIC );
    HDassert( aux_ptr->mpi_rank == 0 );
    HDassert( aux_ptr->c_slist_ptr != NULL );

    cleared = ( (flags & H5C__FLUSH_CLEAR_ONLY_FLAG) != 0 );

    if ( cleared ) {

        /* If the entry has been cleared, must remove it from both the
         * cleaned list and the dirtied list.
         */

        if ( (slist_entry_ptr = H5SL_search(aux_ptr->c_slist_ptr,
                                            (void *)(&addr))) != NULL ) {

            HDassert( slist_entry_ptr->magic == H5AC__H5AC_SLIST_ENTRY_T_MAGIC);
            HDassert( slist_entry_ptr->addr == addr );

            if ( H5SL_remove(aux_ptr->c_slist_ptr, (void *)(&addr))
                 != slist_entry_ptr ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTDELETE, FAIL, \
                            "Can't delete entry from clean entry slist.")
            }

            slist_entry_ptr->magic = 0;
            H5FL_FREE(H5AC_slist_entry_t, slist_entry_ptr);
            slist_entry_ptr = NULL;

            aux_ptr->c_slist_len -= 1;

            HDassert( aux_ptr->c_slist_len >= 0 );
        }

        if ( (slist_entry_ptr = H5SL_search(aux_ptr->d_slist_ptr,
                                            (void *)(&addr))) != NULL ) {

            HDassert( slist_entry_ptr->magic == H5AC__H5AC_SLIST_ENTRY_T_MAGIC);
            HDassert( slist_entry_ptr->addr == addr );

            if ( H5SL_remove(aux_ptr->d_slist_ptr, (void *)(&addr))
                 != slist_entry_ptr ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTDELETE, FAIL, \
                            "Can't delete entry from dirty entry slist.")
            }

            slist_entry_ptr->magic = 0;
            H5FL_FREE(H5AC_slist_entry_t, slist_entry_ptr);
            slist_entry_ptr = NULL;

            aux_ptr->d_slist_len -= 1;

            HDassert( aux_ptr->d_slist_len >= 0 );
        }
    } else if ( was_dirty ) {

        if ( H5SL_search(aux_ptr->c_slist_ptr, (void *)(&addr)) == NULL ) {

            /* insert the address of the entry in the clean entry list. */

            if ( NULL == (slist_entry_ptr = H5FL_CALLOC(H5AC_slist_entry_t)) ) {

                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, \
                            "Can't allocate clean slist entry .")
            }

            slist_entry_ptr->magic = H5AC__H5AC_SLIST_ENTRY_T_MAGIC;
            slist_entry_ptr->addr  = addr;

            if ( H5SL_insert(aux_ptr->c_slist_ptr, slist_entry_ptr,
                             &(slist_entry_ptr->addr)) < 0 ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTINSERT, FAIL, \
                            "can't insert entry into clean entry slist.")
            }

            aux_ptr->c_slist_len += 1;
        }
    }

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5AC_log_flushed_entry() */
#endif /* H5_HAVE_PARALLEL */


/*-------------------------------------------------------------------------
 *
 * Function:    H5AC_log_inserted_entry()
 *
 * Purpose:     Update the dirty_bytes count for a newly inserted entry.
 *
 *		If mpi_rank isnt 0, this simply means adding the size
 *		of the entry to the dirty_bytes count.
 *
 *		If mpi_rank is 0, we must also add the entry to the
 *		dirty entries slist.
 *
 *		Return SUCCEED on success, and FAIL on failure.
 *
 * Return:      Non-negative on success/Negative on failure.
 *
 * Programmer:  John Mainzer, 6/30/05
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */

#ifdef H5_HAVE_PARALLEL
static herr_t
H5AC_log_inserted_entry(H5F_t * f,
                        H5AC_t * cache_ptr,
                        H5AC_info_t * entry_ptr,
                        const H5AC_class_t * type,
                        haddr_t addr)
{
    herr_t               ret_value = SUCCEED;    /* Return value */
    size_t               size;
    H5AC_aux_t         * aux_ptr = NULL;
    H5AC_slist_entry_t * slist_entry_ptr = NULL;

    FUNC_ENTER_NOAPI(H5AC_log_inserted_entry, FAIL)

    HDassert( cache_ptr != NULL );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );

    aux_ptr = cache_ptr->aux_ptr;

    HDassert( aux_ptr != NULL );
    HDassert( aux_ptr->magic == H5AC__H5AC_AUX_T_MAGIC );

    HDassert( entry_ptr != NULL );
    HDassert( entry_ptr->addr == addr );
    HDassert( entry_ptr->type == type );

    /* the size field of the entry will not have been set yet, so we
     * have to obtain it directly.
     */
    if ( (type->size)(f, (void *)entry_ptr, &size) < 0 ) {

        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGETSIZE, FAIL, \
                    "Can't get size of entry to be inserted.")
    }

    if ( aux_ptr->mpi_rank == 0 ) {

        HDassert( aux_ptr->d_slist_ptr != NULL );
        HDassert( aux_ptr->c_slist_ptr != NULL );

        if ( H5SL_search(aux_ptr->d_slist_ptr, (void *)(&addr)) == NULL ) {

            /* insert the address of the entry in the dirty entry list, and
             * add its size to the dirty_bytes count.
             */
            if ( NULL == (slist_entry_ptr = H5FL_CALLOC(H5AC_slist_entry_t)) ) {

                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, \
                            "Can't allocate dirty slist entry .")
            }

            slist_entry_ptr->magic = H5AC__H5AC_SLIST_ENTRY_T_MAGIC;
            slist_entry_ptr->addr  = addr;

            if ( H5SL_insert(aux_ptr->d_slist_ptr, slist_entry_ptr,
                             &(slist_entry_ptr->addr)) < 0 ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTINSERT, FAIL, \
                            "can't insert entry into dirty entry slist.")
            }

            aux_ptr->d_slist_len += 1;

        } else {

            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                        "Inserted entry already in dirty slist.")
        }

        if ( H5SL_search(aux_ptr->c_slist_ptr, (void *)(&addr)) != NULL ) {

            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                        "Inserted entry in clean slist.")
        }
    }

    aux_ptr->dirty_bytes += size;

#if H5AC_DEBUG_DIRTY_BYTES_CREATION
    aux_ptr->insert_dirty_bytes += size;
    aux_ptr->insert_dirty_bytes_updates += 1;
#endif /* H5AC_DEBUG_DIRTY_BYTES_CREATION */

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5AC_log_inserted_entry() */
#endif /* H5_HAVE_PARALLEL */


/*-------------------------------------------------------------------------
 *
 * Function:    H5AC_log_moved_entry()
 *
 * Purpose:     Update the dirty_bytes count for a moved entry.
 *
 *		WARNING
 *
 *		At present, the way that the move call is used ensures
 *		that the moved entry is present in all caches by
 *		moving in a collective operation and immediately after
 *		unprotecting the target entry.
 *
 *		This function uses this invarient, and will cause arcane
 *		failures if it is not met.  If maintaining this invarient
 *		becomes impossible, we will have to rework this function
 *		extensively, and likely include a bit of IPC for
 *		synchronization.  A better option might be to subsume
 *		move in the unprotect operation.
 *
 *		Given that the target entry is in all caches, the function
 *		proceeds as follows:
 *
 *		For processes with mpi rank other 0, it simply checks to
 *		see if the entry was dirty prior to the move, and adds
 *		the entries size to the dirty bytes count.
 *
 *		In the process with mpi rank 0, the function first checks
 *		to see if the entry was dirty prior to the move.  If it
 *		was, and if the entry doesn't appear in the dirtied list
 *		under its old address, it adds the entry's size to the
 *		dirty bytes count.
 *
 *		The rank 0 process then removes any references to the
 *		entry under its old address from the cleands and dirtied
 *		lists, and inserts an entry in the dirtied list under the
 *		new address.
 *
 *		Return SUCCEED on success, and FAIL on failure.
 *
 * Return:      Non-negative on success/Negative on failure.
 *
 * Programmer:  John Mainzer, 6/30/05
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifdef H5_HAVE_PARALLEL
static herr_t
H5AC_log_moved_entry(const H5F_t *f,
                       haddr_t old_addr,
                       haddr_t new_addr)
{
    H5AC_t *             cache_ptr;
    hbool_t		 entry_in_cache;
    hbool_t		 entry_dirty;
    size_t               entry_size;
    H5AC_aux_t         * aux_ptr = NULL;
    H5AC_slist_entry_t * slist_entry_ptr = NULL;
    herr_t               ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(H5AC_log_moved_entry, FAIL)

    HDassert( f );
    HDassert( f->shared );

    cache_ptr = (H5AC_t *)f->shared->cache;

    HDassert( cache_ptr );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );

    aux_ptr = cache_ptr->aux_ptr;

    HDassert( aux_ptr != NULL );
    HDassert( aux_ptr->magic == H5AC__H5AC_AUX_T_MAGIC );

    /* get entry status, size, etc here */
    if ( H5C_get_entry_status(f, old_addr, &entry_size, &entry_in_cache,
                              &entry_dirty, NULL, NULL) < 0 ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Can't get entry status.")

    } else if ( ! entry_in_cache ) {

        HDassert( entry_in_cache );
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "entry not in cache.")
    }

    if ( aux_ptr->mpi_rank == 0 ) {

        HDassert( aux_ptr->d_slist_ptr != NULL );
        HDassert( aux_ptr->c_slist_ptr != NULL );

        /* if the entry appears in the cleaned entry slist, under its old
         * address, remove it.
         */
        if ( (slist_entry_ptr = H5SL_search(aux_ptr->c_slist_ptr,
                                            (void *)(&old_addr))) != NULL ) {

            HDassert( slist_entry_ptr->magic ==
                          H5AC__H5AC_SLIST_ENTRY_T_MAGIC );
            HDassert( slist_entry_ptr->addr == old_addr );

            if ( H5SL_remove(aux_ptr->c_slist_ptr, (void *)(&old_addr))
                               != slist_entry_ptr ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTDELETE, FAIL, \
                            "Can't delete entry from cleaned entry slist.")
            }

            slist_entry_ptr->magic = 0;
            H5FL_FREE(H5AC_slist_entry_t, slist_entry_ptr);
            slist_entry_ptr = NULL;

            aux_ptr->c_slist_len -= 1;

            HDassert( aux_ptr->c_slist_len >= 0 );
        }

        /* if the entry appears in the dirtied entry slist under its old
         * address, remove it, but don't free it. Set addr to new_addr.
         */
        if ( (slist_entry_ptr = H5SL_search(aux_ptr->d_slist_ptr,
                                            (void *)(&old_addr))) != NULL ) {

            HDassert( slist_entry_ptr->magic ==
                      H5AC__H5AC_SLIST_ENTRY_T_MAGIC );
            HDassert( slist_entry_ptr->addr == old_addr );

            if ( H5SL_remove(aux_ptr->d_slist_ptr, (void *)(&old_addr))
                != slist_entry_ptr ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTDELETE, FAIL, \
                            "Can't delete entry from dirty entry slist.")
            }

            slist_entry_ptr->addr = new_addr;

            aux_ptr->d_slist_len -= 1;

            HDassert( aux_ptr->d_slist_len >= 0 );

        } else {

             /* otherwise, allocate a new entry that is ready
              * for insertion, and increment dirty_bytes.
              *
              * Note that the fact that the entry wasn't in the dirtied
              * list under its old address implies that it must have
              * been clean to start with.
              */

            HDassert( !entry_dirty );

            if ( NULL == (slist_entry_ptr = H5FL_CALLOC(H5AC_slist_entry_t)) ) {

                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, \
                            "Can't allocate dirty slist entry .")
            }

            slist_entry_ptr->magic = H5AC__H5AC_SLIST_ENTRY_T_MAGIC;
            slist_entry_ptr->addr  = new_addr;

            aux_ptr->dirty_bytes += entry_size;

#if H5AC_DEBUG_DIRTY_BYTES_CREATION
            aux_ptr->move_dirty_bytes += entry_size;
            aux_ptr->move_dirty_bytes_updates += 1;
#endif /* H5AC_DEBUG_DIRTY_BYTES_CREATION */
        }

        /* verify that there is no entry at new_addr in the dirty slist */
        if ( H5SL_search(aux_ptr->d_slist_ptr, (void *)(&new_addr)) != NULL ) {

            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                        "dirty slist already contains entry at new_addr.")
        }

        /* insert / reinsert the entry in the dirty slist */
        if ( H5SL_insert(aux_ptr->d_slist_ptr, slist_entry_ptr,
                         &(slist_entry_ptr->addr)) < 0 ) {

            HGOTO_ERROR(H5E_CACHE, H5E_CANTINSERT, FAIL, \
                        "can't insert entry into dirty entry slist.")
        }

        aux_ptr->d_slist_len += 1;

    } else if ( ! entry_dirty ) {

        aux_ptr->dirty_bytes += entry_size;

#if H5AC_DEBUG_DIRTY_BYTES_CREATION
        aux_ptr->move_dirty_bytes += entry_size;
        aux_ptr->move_dirty_bytes_updates += 1;
#endif /* H5AC_DEBUG_DIRTY_BYTES_CREATION */
    }

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5AC_log_moved_entry() */
#endif /* H5_HAVE_PARALLEL */


/*-------------------------------------------------------------------------
 * Function:    H5AC_propagate_flushed_and_still_clean_entries_list
 *
 * Purpose:     In PHDF5, only the metadata cache with mpi rank 0 is allowed
 *		to write to file.  All other metadata caches on processes
 *		with rank greater than 0 must retain dirty entries until
 *		they are notified that the entry is now clean.
 *
 *		This function is the main routine for that proceedure.
 *  		It must be called simultaniously on all processes that
 *		have the relevant file open.  To this end, there must
 *		be a barrier immediately prior to this call.
 *
 *		Typicaly, this will be done one of two ways:
 *
 *		1) Dirty byte creation exceeds some user specified value.
 *
 *		   While metadata reads may occur independently, all
 *		   operations writing metadata must be collective.  Thus
 *		   all metadata caches see the same sequence of operations,
 *                 and therefore the same dirty data creation.
 *
 *		   This fact is used to synchronize the caches for purposes
 *                 of propagating the list of flushed and still clean
 *		   entries, by simply calling this function from all
 *		   caches whenever some user specified threshold on dirty
 *		   data is exceeded.
 *
 *		2) Under direct user control -- this operation must be
 *		   collective.
 *
 *              The operations to be managed by this function are as
 * 		follows:
 *
 *		For the process with mpi rank 0:
 *
 *		1) Enable writes, flush the cache to its min clean size,
 *		   and then disable writes again.
 *
 *		2) Load the contents of the flushed and still clean entries
 *		   list (c_slist_ptr) into a buffer, and broadcast that
 *		   buffer to all the other caches.
 *
 *		3) Clear the flushed and still clean entries list
 *                 (c_slist_ptr).
 *
 *
 *		For all processes with mpi rank greater than 0:
 *
 *		1) Receive the flushed and still clean entries list broadcast
 *
 *		2) Mark the specified entries as clean.
 *
 *
 *		For all processes:
 *
 *		1) Reset the dirtied bytes count to 0.
 *
 * Return:      Success:        non-negative
 *
 *              Failure:        negative
 *
 * Programmer:  John Mainzer
 *              July 5, 2005
 *
 * Modifications:
 *
 * 		JRM -- 5/11/06
 * 		Added code to call the write_done callback.
 *
 *-------------------------------------------------------------------------
 */

#ifdef H5_HAVE_PARALLEL
herr_t
H5AC_propagate_flushed_and_still_clean_entries_list(H5F_t  * f,
                                                    hid_t    dxpl_id,
                                                    H5AC_t * cache_ptr,
                                                    hbool_t  do_barrier)
{
    herr_t	 ret_value = SUCCEED;   /* Return value */
    herr_t	 result;
    int		 mpi_code;
    H5AC_aux_t * aux_ptr = NULL;

    FUNC_ENTER_NOAPI(H5AC_propagate_flushed_and_still_clean_entries_list, FAIL)

    HDassert( cache_ptr != NULL );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );

    aux_ptr = (H5AC_aux_t *)(cache_ptr->aux_ptr);

    HDassert( aux_ptr != NULL );
    HDassert( aux_ptr->magic == H5AC__H5AC_AUX_T_MAGIC );

#if H5AC_DEBUG_DIRTY_BYTES_CREATION
    HDfprintf(stdout,
              "%d:H5AC_propagate...:%d: (u/uu/i/iu/r/ru) = %d/%d/%d/%d/%d/%d\n",
              (int)(aux_ptr->mpi_rank),
              (int)(aux_ptr->dirty_bytes_propagations),
              (int)(aux_ptr->unprotect_dirty_bytes),
              (int)(aux_ptr->unprotect_dirty_bytes_updates),
              (int)(aux_ptr->insert_dirty_bytes),
              (int)(aux_ptr->insert_dirty_bytes_updates),
              (int)(aux_ptr->move_dirty_bytes),
              (int)(aux_ptr->move_dirty_bytes_updates));
#endif /* H5AC_DEBUG_DIRTY_BYTES_CREATION */

    if ( do_barrier ) {

        /* to prevent "messages from the future" we must synchronize all
         * processes before we start the flush.  This synchronization may
	 * already be done -- hence the do_barrier parameter.
         */

        if ( MPI_SUCCESS != (mpi_code = MPI_Barrier(aux_ptr->mpi_comm)) ) {

            HMPI_GOTO_ERROR(FAIL, "MPI_Barrier failed", mpi_code)
        }
    }

    if ( aux_ptr->mpi_rank == 0 ) {

	aux_ptr->write_permitted = TRUE;

	result = H5C_flush_to_min_clean(f, dxpl_id, H5AC_noblock_dxpl_id);

	aux_ptr->write_permitted = FALSE;

        if ( result < 0 ) {

            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                        "H5C_flush_to_min_clean() failed.")
        }

	if ( aux_ptr->write_done != NULL ) {

	    (aux_ptr->write_done)();
	}

        if ( H5AC_broadcast_clean_list(cache_ptr) < 0 ) {

            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                        "Can't broadcast clean slist.")
        }

        HDassert( aux_ptr->c_slist_len == 0 );

    } else {

        if ( H5AC_receive_and_apply_clean_list(f, dxpl_id,
                                               H5AC_noblock_dxpl_id,
                                               cache_ptr) < 0 ) {

            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                        "Can't receive and/or process clean slist broadcast.")
        }
    }

    aux_ptr->dirty_bytes = 0;
#if H5AC_DEBUG_DIRTY_BYTES_CREATION
    aux_ptr->dirty_bytes_propagations     += 1;
    aux_ptr->unprotect_dirty_bytes         = 0;
    aux_ptr->unprotect_dirty_bytes_updates = 0;
    aux_ptr->insert_dirty_bytes            = 0;
    aux_ptr->insert_dirty_bytes_updates    = 0;
    aux_ptr->move_dirty_bytes            = 0;
    aux_ptr->move_dirty_bytes_updates    = 0;
#endif /* H5AC_DEBUG_DIRTY_BYTES_CREATION */

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5AC_propagate_flushed_and_still_clean_entries_list() */
#endif /* H5_HAVE_PARALLEL */


/*-------------------------------------------------------------------------
 *
 * Function:    H5AC_receive_and_apply_clean_list()
 *
 * Purpose:     Receive the list of cleaned entries from process 0,
 *		and mark the specified entries as clean.
 *
 *		This function must only be called by the process with
 *		MPI_rank greater than 0.
 *
 *		Return SUCCEED on success, and FAIL on failure.
 *
 * Return:      Non-negative on success/Negative on failure.
 *
 * Programmer:  John Mainzer, 7/4/05
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */

#ifdef H5_HAVE_PARALLEL
static herr_t
H5AC_receive_and_apply_clean_list(H5F_t  * f,
                                  hid_t    primary_dxpl_id,
                                  hid_t    secondary_dxpl_id,
                                  H5AC_t * cache_ptr)
{
    herr_t               ret_value = SUCCEED;    /* Return value */
    H5AC_aux_t         * aux_ptr = NULL;
    haddr_t	       * haddr_buf_ptr = NULL;
    MPI_Offset         * MPI_Offset_buf_ptr = NULL;
    size_t		 buf_size;
    int                  i = 0;
    int                  mpi_result;
    int			 num_entries;

    FUNC_ENTER_NOAPI(H5AC_receive_and_apply_clean_list, FAIL)

    HDassert( cache_ptr != NULL );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );

    aux_ptr = (H5AC_aux_t *)(cache_ptr->aux_ptr);

    HDassert( aux_ptr != NULL );
    HDassert( aux_ptr->magic == H5AC__H5AC_AUX_T_MAGIC );
    HDassert( aux_ptr->mpi_rank != 0 );

    /* First receive the number of entries in the list so that we
     * can set up a buffer to receive them.  If there aren't
     * any, we are done.
     */
    mpi_result = MPI_Bcast(&num_entries, 1, MPI_INT, 0, aux_ptr->mpi_comm);

    if ( mpi_result != MPI_SUCCESS ) {

        HMPI_GOTO_ERROR(FAIL, "MPI_Bcast failed 1", mpi_result)
    }

    if ( num_entries > 0 )
    {
        /* allocate a buffers to store the list of entry base addresses in */

        buf_size = sizeof(MPI_Offset) * (size_t)num_entries;

        MPI_Offset_buf_ptr = (MPI_Offset *)H5MM_malloc(buf_size);

        if ( MPI_Offset_buf_ptr == NULL ) {

            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, \
                        "memory allocation failed for receive buffer")
        }

        haddr_buf_ptr = (haddr_t *)H5MM_malloc(sizeof(haddr_t) *
                                               (size_t)num_entries);

        if ( haddr_buf_ptr == NULL ) {

            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, \
                        "memory allocation failed for haddr buffer")
        }


        /* Now receive the list of cleaned entries
         *
         * The peculiar structure of the following call to MPI_Bcast is
         * due to MPI's (?) failure to believe in the MPI_Offset type.
         * Thus the element type is MPI_BYTE, with size equal to the
         * buf_size computed above.
         */

        mpi_result = MPI_Bcast((void *)MPI_Offset_buf_ptr, (int)buf_size,
                               MPI_BYTE, 0, aux_ptr->mpi_comm);

        if ( mpi_result != MPI_SUCCESS ) {

            HMPI_GOTO_ERROR(FAIL, "MPI_Bcast failed 2", mpi_result)
        }


        /* translate the MPI_Offsets to haddr_t */
        i = 0;
        while ( i < num_entries )
        {
            haddr_buf_ptr[i] = H5FD_mpi_MPIOff_to_haddr(MPI_Offset_buf_ptr[i]);

            if ( haddr_buf_ptr[i] == HADDR_UNDEF ) {

                HGOTO_ERROR(H5E_INTERNAL, H5E_BADRANGE, FAIL, \
                            "can't convert MPI off to haddr")
            }

            i++;
        }


        /* mark the indicated entries as clean */
        if ( H5C_mark_entries_as_clean(f, primary_dxpl_id, secondary_dxpl_id,
                           (int32_t)num_entries, &(haddr_buf_ptr[0])) < 0 ) {

            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                        "Can't mark entries clean.")

        }
    }

done:

    if ( MPI_Offset_buf_ptr != NULL ) {

        MPI_Offset_buf_ptr =
            (MPI_Offset *)H5MM_xfree((void *)MPI_Offset_buf_ptr);
    }

    if ( haddr_buf_ptr != NULL ) {

        haddr_buf_ptr = (haddr_t *)H5MM_xfree((void *)haddr_buf_ptr);
    }

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5AC_receive_and_apply_clean_list() */


/*-------------------------------------------------------------------------
 * Function:    H5AC_flush_entries
 *
 * Purpose:	Flush the metadata cache associated with the specified file,
 *		only writing from rank 0, but propagating the cleaned entries
 *		to all ranks.
 *
 * Return:      Non-negative on success/Negative on failure if there was a
 *              request to flush all items and something was protected.
 *
 * Programmer:  Quincey Koziol
 *              koziol@hdfgroup.org
 *              Aug 22 2009
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5AC_flush_entries(H5F_t *f)
{
    herr_t	  ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5AC_flush_entries)

    HDassert(f);
    HDassert(f->shared->cache);

    /* Check if we have >1 ranks */
    if(f->shared->cache->aux_ptr) {
        H5AC_aux_t	* aux_ptr = f->shared->cache->aux_ptr;
        int		  mpi_code;

#if H5AC_DEBUG_DIRTY_BYTES_CREATION
        HDfprintf(stdout,
                  "%d::H5AC_flush: (u/uu/i/iu/r/ru) = %d/%d/%d/%d/%d/%d\n",
                  (int)(aux_ptr->mpi_rank),
                  (int)(aux_ptr->unprotect_dirty_bytes),
                  (int)(aux_ptr->unprotect_dirty_bytes_updates),
                  (int)(aux_ptr->insert_dirty_bytes),
                  (int)(aux_ptr->insert_dirty_bytes_updates),
                  (int)(aux_ptr->move_dirty_bytes),
                  (int)(aux_ptr->move_dirty_bytes_updates));
#endif /* H5AC_DEBUG_DIRTY_BYTES_CREATION */

        /* to prevent "messages from the future" we must synchronize all
         * processes before we start the flush.  Hence the following
         * barrier.
         */
        if(MPI_SUCCESS != (mpi_code = MPI_Barrier(aux_ptr->mpi_comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Barrier failed", mpi_code)

        /* Flush data to disk, from rank 0 process */
        if(aux_ptr->mpi_rank == 0 ) {
            herr_t	  status;

	    aux_ptr->write_permitted = TRUE;

            status = H5C_flush_cache(f,
                                     H5AC_noblock_dxpl_id,
                                     H5AC_noblock_dxpl_id,
                                     H5AC__NO_FLAGS_SET);

	    aux_ptr->write_permitted = FALSE;

            if(status < 0)
                HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "Can't flush.")

            if(aux_ptr->write_done != NULL)
                (aux_ptr->write_done)();
        } /* end if ( aux_ptr->mpi_rank == 0 ) */

        /* Propagate cleaned entries to other ranks */
        if(H5AC_propagate_flushed_and_still_clean_entries_list(f,
                                                          H5AC_noblock_dxpl_id,
                                                          f->shared->cache,
                                                          FALSE) < 0 )
            HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "Can't propagate clean entries list.")
    } /* end if ( aux_ptr != NULL ) */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5AC_flush_entries() */
#endif /* H5_HAVE_PARALLEL */

