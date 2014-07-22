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
 * Created:     H5C.c
 *              June 1 2004
 *              John Mainzer
 *
 * Purpose:     Functions in this file implement a generic cache for
 *              things which exist on disk, and which may be
 *	 	unambiguously referenced by their disk addresses.
 *
 *              The code in this module was initially written in
 *		support of a complete re-write of the metadata cache
 *		in H5AC.c  However, other uses for the cache code
 *		suggested themselves, and thus this file was created
 *		in an attempt to support re-use.
 *
 *		For a detailed overview of the cache, please see the
 *		header comment for H5C_t in H5Cpkg.h.
 *
 *-------------------------------------------------------------------------
 */

/**************************************************************************
 *
 *				To Do:
 *
 *	Code Changes:
 *
 *	 - Remove extra functionality in H5C_flush_single_entry()?
 *
 *	 - Change protect/unprotect to lock/unlock.
 *
 *	 - Flush entries in increasing address order in
 *	   H5C_make_space_in_cache().
 *
 *	 - Also in H5C_make_space_in_cache(), use high and low water marks
 *	   to reduce the number of I/O calls.
 *
 *	 - When flushing, attempt to combine contiguous entries to reduce
 *	   I/O overhead.  Can't do this just yet as some entries are not
 *	   contiguous.  Do this in parallel only or in serial as well?
 *
 *	 - Create MPI type for dirty objects when flushing in parallel.
 *
 *	 - Now that TBBT routines aren't used, fix nodes in memory to
 *         point directly to the skip list node from the LRU list, eliminating
 *         skip list lookups when evicting objects from the cache.
 *
 *	Tests:
 *
 *	 - Trim execution time.  (This is no longer a major issue with the
 *	   shift from the TBBT to a hash table for indexing.)
 *
 *	 - Add random tests.
 *
 **************************************************************************/

#define H5C_PACKAGE		/*suppress error about including H5Cpkg   */
#define H5F_PACKAGE		/*suppress error about including H5Fpkg	  */


#include "H5private.h"		/* Generic Functions			*/
#ifdef H5_HAVE_PARALLEL
#include "H5ACprivate.h"        /* Metadata cache                       */
#endif /* H5_HAVE_PARALLEL */
#include "H5Cpkg.h"		/* Cache				*/
#include "H5Dprivate.h"		/* Dataset functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fpkg.h"		/* Files				*/
#include "H5FDprivate.h"	/* File drivers				*/
#include "H5FLprivate.h"	/* Free Lists                           */
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Pprivate.h"         /* Property lists                       */
#include "H5SLprivate.h"	/* Skip lists				*/


/*
 * Private file-scope variables.
 */

/* Declare a free list to manage the H5C_t struct */
H5FL_DEFINE_STATIC(H5C_t);


/*
 * Private file-scope function declarations:
 */

static herr_t H5C__auto_adjust_cache_size(H5F_t * f,
                                          hid_t primary_dxpl_id,
                                          hid_t secondary_dxpl_id,
                                          hbool_t write_permitted,
                                          hbool_t * first_flush_ptr);

static herr_t H5C__autoadjust__ageout(H5F_t * f,
                                      double hit_rate,
                                      enum H5C_resize_status * status_ptr,
                                      size_t * new_max_cache_size_ptr,
                                      hid_t primary_dxpl_id,
                                      hid_t secondary_dxpl_id,
                                      hbool_t write_permitted,
                                      hbool_t * first_flush_ptr);

static herr_t H5C__autoadjust__ageout__cycle_epoch_marker(H5C_t * cache_ptr);

static herr_t H5C__autoadjust__ageout__evict_aged_out_entries(H5F_t * f,
                                                    hid_t primary_dxpl_id,
                                                    hid_t secondary_dxpl_id,
                                                    hbool_t write_permitted,
                                                    hbool_t * first_flush_ptr);

static herr_t H5C__autoadjust__ageout__insert_new_marker(H5C_t * cache_ptr);

static herr_t H5C__autoadjust__ageout__remove_all_markers(H5C_t * cache_ptr);

static herr_t H5C__autoadjust__ageout__remove_excess_markers(H5C_t * cache_ptr);

static herr_t H5C__flash_increase_cache_size(H5C_t * cache_ptr,
                                             size_t old_entry_size,
                                             size_t new_entry_size);

static herr_t H5C_flush_single_entry(H5F_t *       	 f,
                                     hid_t               primary_dxpl_id,
                                     hid_t               secondary_dxpl_id,
                                     const H5C_class_t * type_ptr,
                                     haddr_t             addr,
                                     unsigned            flags,
                                     hbool_t *           first_flush_ptr,
                                     hbool_t    del_entry_from_slist_on_destroy);

static herr_t H5C_flush_invalidate_cache(H5F_t *  f,
	                                 hid_t    primary_dxpl_id,
				         hid_t    secondary_dxpl_id,
					 unsigned flags);

static void * H5C_load_entry(H5F_t *             f,
                             hid_t               dxpl_id,
                             const H5C_class_t * type,
                             haddr_t             addr,
                             void *              udata);

static herr_t H5C_make_space_in_cache(H5F_t * f,
                                      hid_t   primary_dxpl_id,
                                      hid_t   secondary_dxpl_id,
                                      size_t  space_needed,
                                      hbool_t write_permitted,
                                      hbool_t * first_flush_ptr);
#if H5C_DO_EXTREME_SANITY_CHECKS
static herr_t H5C_validate_lru_list(H5C_t * cache_ptr);
static herr_t H5C_verify_not_in_index(H5C_t * cache_ptr,
                                      H5C_cache_entry_t * entry_ptr);
#endif /* H5C_DO_EXTREME_SANITY_CHECKS */


/****************************************************************************
 *
 * #defines and declarations for epoch marker cache entries.
 *
 * As a strategy for automatic cache size reduction, the cache may insert
 * marker entries in the LRU list at the end of each epoch.  These markers
 * are then used to identify entries that have not been accessed for n
 * epochs so that they can be evicted from the cache.
 *
 ****************************************************************************/

/* Note that H5C__MAX_EPOCH_MARKERS is defined in H5Cpkg.h, not here because
 * it is needed to dimension arrays in H5C_t.
 */

#define H5C__EPOCH_MARKER_TYPE	H5C__MAX_NUM_TYPE_IDS

static void *H5C_epoch_marker_load(H5F_t *f, hid_t dxpl_id, haddr_t addr,
                                   void *udata);
static herr_t H5C_epoch_marker_flush(H5F_t *f, hid_t dxpl_id, hbool_t dest,
                                     haddr_t addr, void *thing,
				     unsigned *flags_ptr);
static herr_t H5C_epoch_marker_dest(H5F_t *f, void *thing);
static herr_t H5C_epoch_marker_clear(H5F_t *f, void *thing, hbool_t dest);
static herr_t H5C_epoch_marker_notify(H5C_notify_action_t action, void *thing);
static herr_t H5C_epoch_marker_size(const H5F_t *f, const void *thing, size_t *size_ptr);

const H5C_class_t epoch_marker_class =
{
    /* id    = */ H5C__EPOCH_MARKER_TYPE,
    /* load  = */ &H5C_epoch_marker_load,
    /* flush = */ &H5C_epoch_marker_flush,
    /* dest  = */ &H5C_epoch_marker_dest,
    /* clear = */ &H5C_epoch_marker_clear,
    /* size  = */ &H5C_epoch_marker_size
};

/***************************************************************************
 * Class functions for H5C__EPOCH_MAKER_TYPE:
 *
 * None of these functions should ever be called, so there is no point in
 * documenting them separately.
 *                                                     JRM - 11/16/04
 *
 ***************************************************************************/

static void *
H5C_epoch_marker_load(H5F_t UNUSED * f,
                      hid_t UNUSED dxpl_id,
                      haddr_t UNUSED addr,
                      void UNUSED * udata)
{
    void * ret_value = NULL;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, NULL, "called unreachable fcn.")

done:

    FUNC_LEAVE_NOAPI(ret_value)
}

static herr_t
H5C_epoch_marker_flush(H5F_t UNUSED *f,
                       hid_t UNUSED dxpl_id,
                       hbool_t UNUSED dest,
                       haddr_t UNUSED addr,
                       void UNUSED *thing,
		       unsigned UNUSED * flags_ptr)
{
    herr_t ret_value = FAIL;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "called unreachable fcn.")

done:

    FUNC_LEAVE_NOAPI(ret_value)
}

static herr_t
H5C_epoch_marker_dest(H5F_t UNUSED * f,
                      void UNUSED * thing)
{
    herr_t ret_value = FAIL;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "called unreachable fcn.")

done:

    FUNC_LEAVE_NOAPI(ret_value)
}

static herr_t
H5C_epoch_marker_clear(H5F_t UNUSED * f,
                       void UNUSED * thing,
                       hbool_t UNUSED dest)
{
    herr_t ret_value = FAIL;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "called unreachable fcn.")

done:

    FUNC_LEAVE_NOAPI(ret_value)
}

static herr_t
H5C_epoch_marker_notify(H5C_notify_action_t UNUSED action,
                       void UNUSED * thing)
{
    herr_t ret_value = FAIL;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "called unreachable fcn.")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}

static herr_t
H5C_epoch_marker_size(const H5F_t UNUSED * f,
                      const void UNUSED * thing,
                      size_t UNUSED * size_ptr)
{
    herr_t ret_value = FAIL;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "called unreachable fcn.")

done:

    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:    H5C_apply_candidate_list
 *
 * Purpose:     Apply the supplied candidate list.
 *
 *		We used to do this by simply having each process write 
 *		every mpi_size-th entry in the candidate list, starting 
 *		at index mpi_rank, and mark all the others clean.  
 *
 *		However, this can cause unnecessary contention in a file 
 *		system by increasing the number of processes writing to 
 *		adjacent locations in the HDF5 file.
 *
 *		To attempt to minimize this, we now arange matters such 
 *		that each process writes n adjacent entries in the 
 *		candidate list, and marks all others clean.  We must do
 *		this in such a fashion as to guarantee that each entry 
 *		on the candidate list is written by exactly one process, 
 *		and marked clean by all others.  
 *
 *		To do this, first construct a table mapping mpi_rank
 *		to the index of the first entry in the candidate list to
 *		be written by the process of that mpi_rank, and then use
 *		the table to control which entries are written and which
 *		are marked as clean as a function of the mpi_rank.
 *
 *		Note that the table must be identical on all processes, as
 *		all see the same candidate list, mpi_size, and mpi_rank --
 *		the inputs used to construct the table.  
 *
 *		We construct the table as follows.  Let:
 *
 *			n = num_candidates / mpi_size;
 *
 *			m = num_candidates % mpi_size;
 *
 *		Now allocate an array of integers of length mpi_size + 1, 
 *		and call this array candidate_assignment_table. 
 *
 *		Conceptually, if the number of candidates is a multiple
 *		of the mpi_size, we simply pass through the candidate list
 *		and assign n entries to each process to flush, with the 
 *		index of the first entry to flush in the location in 
 *		the candidate_assignment_table indicated by the mpi_rank
 *		of the process.  
 *
 *		In the more common case in which the candidate list isn't 
 *		isn't a multiple of the mpi_size, we pretend it is, and 
 *		give num_candidates % mpi_size processes one extra entry
 *		each to make things work out.
 *
 *		Once the table is constructed, we determine the first and
 *		last entry this process is to flush as follows:
 *
 *	 	first_entry_to_flush = candidate_assignment_table[mpi_rank]
 *
 *		last_entry_to_flush = 
 *			candidate_assignment_table[mpi_rank + 1] - 1;
 *		
 *		With these values determined, we simply scan through the 
 *		candidate list, marking all entries in the range 
 *		[first_entry_to_flush, last_entry_to_flush] for flush,
 *		and all others to be cleaned.
 *
 *		Finally, we scan the LRU from tail to head, flushing 
 *		or marking clean the candidate entries as indicated.
 *		If necessary, we scan the pinned list as well.
 *
 *		Note that this function will fail if any protected or 
 *		clean entries appear on the candidate list.
 *
 *		This function is used in managing sync points, and 
 *		shouldn't be used elsewhere.
 *
 * Return:      Success:        SUCCEED
 *
 *              Failure:        FAIL
 *
 * Programmer:  John Mainzer
 *              3/17/10
 *
 * Modifications:
 *
 *		Heavily reworked to have each process flush a group of 
 *		adjacent entries.
 *						JRM -- 4/15/10
 *
 *-------------------------------------------------------------------------
 */
#ifdef H5_HAVE_PARALLEL
#define H5C_APPLY_CANDIDATE_LIST__DEBUG 0
herr_t
H5C_apply_candidate_list(H5F_t * f,
                         hid_t primary_dxpl_id,
                         hid_t secondary_dxpl_id,
                         H5C_t * cache_ptr,
                         int num_candidates,
                         haddr_t * candidates_list_ptr,
                         int mpi_rank,
                         int mpi_size)
{
    hbool_t             first_flush = FALSE;
    int                 i;
    int			m;
    int			n;
    int			first_entry_to_flush;
    int			last_entry_to_flush;
    int			entries_to_clear = 0;
    int			entries_to_flush = 0;
    int			entries_cleared = 0;
    int			entries_flushed = 0;
    int			entries_examined = 0;
    int			initial_list_len;
    int               * candidate_assignment_table = NULL;
    haddr_t		addr;
    H5C_cache_entry_t *	clear_ptr = NULL;
    H5C_cache_entry_t *	entry_ptr = NULL;
    H5C_cache_entry_t *	flush_ptr = NULL;
#if H5C_DO_SANITY_CHECKS
    haddr_t		last_addr;
#endif /* H5C_DO_SANITY_CHECKS */
#if H5C_APPLY_CANDIDATE_LIST__DEBUG
    char		tbl_buf[1024];
#endif /* H5C_APPLY_CANDIDATE_LIST__DEBUG */
    herr_t              ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert( cache_ptr != NULL );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );
    HDassert( num_candidates > 0 );
    HDassert( num_candidates <= cache_ptr->slist_len );
    HDassert( candidates_list_ptr != NULL );
    HDassert( 0 <= mpi_rank );
    HDassert( mpi_rank < mpi_size );

#if H5C_APPLY_CANDIDATE_LIST__DEBUG
    HDfprintf(stdout, "%s:%d: setting up candidate assignment table.\n", 
              FUNC, mpi_rank);
    for ( i = 0; i < 1024; i++ ) tbl_buf[i] = '\0';
    sprintf(&(tbl_buf[0]), "candidate list = ");
    for ( i = 0; i < num_candidates; i++ )
    {
        sprintf(&(tbl_buf[HDstrlen(tbl_buf)]), " 0x%llx", 
                (long long)(*(candidates_list_ptr + i)));
    }
    sprintf(&(tbl_buf[HDstrlen(tbl_buf)]), "\n");
    HDfprintf(stdout, "%s", tbl_buf);
#endif /* H5C_APPLY_CANDIDATE_LIST__DEBUG */

    n = num_candidates / mpi_size;
    m = num_candidates % mpi_size;
    HDassert(n >= 0);

    if(NULL == (candidate_assignment_table = (int *)H5MM_malloc(sizeof(int) * (size_t)(mpi_size + 1))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed for candidate assignment table")

    candidate_assignment_table[0] = 0;
    candidate_assignment_table[mpi_size] = num_candidates;

    if(m == 0) { /* mpi_size is an even divisor of num_candidates */
        HDassert(n > 0);
        for(i = 1; i < mpi_size; i++)
            candidate_assignment_table[i] = candidate_assignment_table[i - 1] + n;
    } /* end if */
    else { 
        for(i = 1; i <= m; i++)
            candidate_assignment_table[i] = candidate_assignment_table[i - 1] + n + 1;

        if(num_candidates < mpi_size) {
            for(i = m + 1; i < mpi_size; i++)
                candidate_assignment_table[i] = num_candidates;
        } /* end if */
        else {
            for(i = m + 1; i < mpi_size; i++)
                candidate_assignment_table[i] = candidate_assignment_table[i - 1] + n;
        } /* end else */
    } /* end else */
    HDassert((candidate_assignment_table[mpi_size - 1] + n) == num_candidates);

#if H5C_DO_SANITY_CHECKS
    /* verify that the candidate assignment table has the expected form */
    for ( i = 1; i < mpi_size - 1; i++ ) 
    {
        int a, b;

        a = candidate_assignment_table[i] - candidate_assignment_table[i - 1];
        b = candidate_assignment_table[i + 1] - candidate_assignment_table[i];

        HDassert( n + 1 >= a );
        HDassert( a >= b );
        HDassert( b >= n );
    }
#endif /* H5C_DO_SANITY_CHECKS */

    first_entry_to_flush = candidate_assignment_table[mpi_rank];
    last_entry_to_flush = candidate_assignment_table[mpi_rank + 1] - 1;

#if H5C_APPLY_CANDIDATE_LIST__DEBUG
    for ( i = 0; i < 1024; i++ )
        tbl_buf[i] = '\0';
    sprintf(&(tbl_buf[0]), "candidate assignment table = ");
    for(i = 0; i <= mpi_size; i++)
        sprintf(&(tbl_buf[HDstrlen(tbl_buf)]), " %d", candidate_assignment_table[i]);
    sprintf(&(tbl_buf[HDstrlen(tbl_buf)]), "\n");
    HDfprintf(stdout, "%s", tbl_buf);

    HDfprintf(stdout, "%s:%d: flush entries [%d, %d].\n", 
              FUNC, mpi_rank, first_entry_to_flush, last_entry_to_flush);

    HDfprintf(stdout, "%s:%d: marking entries.\n", FUNC, mpi_rank);
#endif /* H5C_APPLY_CANDIDATE_LIST__DEBUG */

    for(i = 0; i < num_candidates; i++) {
        addr = candidates_list_ptr[i];
        HDassert( H5F_addr_defined(addr) );

#if H5C_DO_SANITY_CHECKS
        if ( i > 0 ) {
            if ( last_addr == addr ) {
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Duplicate entry in cleaned list.\n")
            } else if ( last_addr > addr ) {
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "candidate list not sorted.\n")
            }
        }

        last_addr = addr;
#endif /* H5C_DO_SANITY_CHECKS */

        H5C__SEARCH_INDEX(cache_ptr, addr, entry_ptr, FAIL)
        if(entry_ptr == NULL) {
            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Listed candidate entry not in cache?!?!?.")
        } else if(!entry_ptr->is_dirty) {
            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Listed entry not dirty?!?!?.")
        } else if ( entry_ptr->is_protected ) {
            /* For now at least, we can't deal with protected entries.
             * If we encounter one, scream and die.  If it becomes an
             * issue, we should be able to work around this. 
             */
            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Listed entry is protected?!?!?.")
        } else {
            /* determine whether the entry is to be cleared or flushed,
             * and mark it accordingly.  We will scan the protected and 
             * pinned list shortly, and clear or flush according to these
             * markings.  
             */
            if((i >= first_entry_to_flush) && (i <= last_entry_to_flush)) {
                entries_to_flush++;
                entry_ptr->flush_immediately = TRUE;
            } /* end if */
            else {
                entries_to_clear++;
                entry_ptr->clear_on_unprotect = TRUE;
            } /* end else */
        } /* end else */
    } /* end for */

#if H5C_APPLY_CANDIDATE_LIST__DEBUG
    HDfprintf(stdout, "%s:%d: num candidates/to clear/to flush = %d/%d/%d.\n", 
              FUNC, mpi_rank, (int)num_candidates, (int)entries_to_clear,
              (int)entries_to_flush);
#endif /* H5C_APPLY_CANDIDATE_LIST__DEBUG */


    /* We have now marked all the entries on the candidate list for 
     * either flush or clear -- now scan the LRU and the pinned list
     * for these entries and do the deed.
     *
     * Note that we are doing things in this round about manner so as
     * to preserve the order of the LRU list to the best of our ability.
     * If we don't do this, my experiments indicate that we will have a
     * noticably poorer hit ratio as a result.
     */

#if H5C_APPLY_CANDIDATE_LIST__DEBUG
    HDfprintf(stdout, "%s:%d: scanning LRU list. len = %d.\n", FUNC, mpi_rank,
              (int)(cache_ptr->LRU_list_len));
#endif /* H5C_APPLY_CANDIDATE_LIST__DEBUG */

    entries_examined = 0;
    initial_list_len = cache_ptr->LRU_list_len;
    entry_ptr = cache_ptr->LRU_tail_ptr;

    while((entry_ptr != NULL) && (entries_examined <= initial_list_len) &&
            ((entries_cleared + entries_flushed) < num_candidates)) {
        if(entry_ptr->clear_on_unprotect) {
            entry_ptr->clear_on_unprotect = FALSE;
            clear_ptr = entry_ptr;
            entry_ptr = entry_ptr->prev;
            entries_cleared++;

#if ( H5C_APPLY_CANDIDATE_LIST__DEBUG > 1 )
    HDfprintf(stdout, "%s:%d: clearing 0x%llx.\n", FUNC, mpi_rank, 
              (long long)clear_ptr->addr);
#endif /* H5C_APPLY_CANDIDATE_LIST__DEBUG */

            if(H5C_flush_single_entry(f,
                                      primary_dxpl_id,
                                      secondary_dxpl_id,
                                      clear_ptr->type,
                                      clear_ptr->addr,
                                      H5C__FLUSH_CLEAR_ONLY_FLAG,
                                      &first_flush,
                                      TRUE) < 0)
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Can't clear entry.")
        } else if(entry_ptr->flush_immediately) {
            entry_ptr->flush_immediately = FALSE;
            flush_ptr = entry_ptr;
            entry_ptr = entry_ptr->prev;
            entries_flushed++;

#if ( H5C_APPLY_CANDIDATE_LIST__DEBUG > 1 )
    HDfprintf(stdout, "%s:%d: flushing 0x%llx.\n", FUNC, mpi_rank, 
              (long long)flush_ptr->addr);
#endif /* H5C_APPLY_CANDIDATE_LIST__DEBUG */

            if(H5C_flush_single_entry(f,
                                      primary_dxpl_id,
                                      secondary_dxpl_id,
                                      flush_ptr->type,
                                      flush_ptr->addr,
                                      H5C__NO_FLAGS_SET,
                                      &first_flush,
                                      TRUE) < 0)
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Can't clear entry.")
        } else {
            entry_ptr = entry_ptr->prev;
        }

        entries_examined++;
    } /* end while */

#if H5C_APPLY_CANDIDATE_LIST__DEBUG
    HDfprintf(stdout, "%s:%d: entries examined/cleared/flushed = %d/%d/%d.\n", 
              FUNC, mpi_rank, entries_examined, 
              entries_cleared, entries_flushed);
#endif /* H5C_APPLY_CANDIDATE_LIST__DEBUG */

    /* It is also possible that some of the cleared entries are on the
     * pinned list.  Must scan that also.
     */

#if H5C_APPLY_CANDIDATE_LIST__DEBUG
    HDfprintf(stdout, "%s:%d: scanning pinned entry list. len = %d\n", 
             FUNC, mpi_rank, (int)(cache_ptr->pel_len));
#endif /* H5C_APPLY_CANDIDATE_LIST__DEBUG */

    entry_ptr = cache_ptr->pel_head_ptr;
    while((entry_ptr != NULL) &&
            ((entries_cleared + entries_flushed) < num_candidates)) {
        if(entry_ptr->clear_on_unprotect) {
            entry_ptr->clear_on_unprotect = FALSE;
            clear_ptr = entry_ptr;
            entry_ptr = entry_ptr->next;
            entries_cleared++;

#if ( H5C_APPLY_CANDIDATE_LIST__DEBUG > 1 )
            HDfprintf(stdout, "%s:%d: clearing 0x%llx.\n", FUNC, mpi_rank, 
                      (long long)clear_ptr->addr);
#endif /* H5C_APPLY_CANDIDATE_LIST__DEBUG */

            if(H5C_flush_single_entry(f,
                                      primary_dxpl_id,
                                      secondary_dxpl_id,
                                      clear_ptr->type,
                                      clear_ptr->addr,
                                      H5C__FLUSH_CLEAR_ONLY_FLAG,
                                      &first_flush,
                                      TRUE) < 0)
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Can't clear entry.")
        } else if(entry_ptr->flush_immediately) {
            entry_ptr->flush_immediately = FALSE;
            flush_ptr = entry_ptr;
            entry_ptr = entry_ptr->next;
            entries_flushed++;

#if ( H5C_APPLY_CANDIDATE_LIST__DEBUG > 1 )
            HDfprintf(stdout, "%s:%d: flushing 0x%llx.\n", FUNC, mpi_rank, 
                      (long long)flush_ptr->addr);
#endif /* H5C_APPLY_CANDIDATE_LIST__DEBUG */

            if(H5C_flush_single_entry(f,
                                      primary_dxpl_id,
                                      secondary_dxpl_id,
                                      flush_ptr->type,
                                      flush_ptr->addr,
                                      H5C__NO_FLAGS_SET,
                                      &first_flush,
                                      TRUE) < 0)
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Can't clear entry.")
        } else {
            entry_ptr = entry_ptr->next;
        }
    } /* end while */

#if H5C_APPLY_CANDIDATE_LIST__DEBUG
    HDfprintf(stdout, 
              "%s:%d: pel entries examined/cleared/flushed = %d/%d/%d.\n", 
              FUNC, mpi_rank, entries_examined, 
              entries_cleared, entries_flushed);
    HDfprintf(stdout, "%s:%d: done.\n", FUNC, mpi_rank);

    HDfsync(stdout);
#endif /* H5C_APPLY_CANDIDATE_LIST__DEBUG */

    if((entries_flushed != entries_to_flush) || (entries_cleared != entries_to_clear))
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "entry count mismatch.")

done:
    if(candidate_assignment_table != NULL)
        candidate_assignment_table = (int *)H5MM_xfree((void *)candidate_assignment_table);

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5C_apply_candidate_list() */
#endif /* H5_HAVE_PARALLEL */


/*-------------------------------------------------------------------------
 * Function:    H5C_construct_candidate_list__clean_cache
 *
 * Purpose:     Construct the list of entries that should be flushed to 
 *		clean all entries in the cache.
 *
 *		This function is used in managing sync points, and 
 *		shouldn't be used elsewhere.
 *
 * Return:      Success:        SUCCEED
 *
 *              Failure:        FAIL
 *
 * Programmer:  John Mainzer
 *              3/17/10
 *
 *-------------------------------------------------------------------------
 */
#ifdef H5_HAVE_PARALLEL
herr_t
H5C_construct_candidate_list__clean_cache(H5C_t * cache_ptr)
{
    size_t              space_needed;
    herr_t              ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert( cache_ptr != NULL );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );

    /* As a sanity check, set space needed to the size of the skip list.
     * This should be the sum total of the sizes of all the dirty entries
     * in the metadata cache.
     */
    space_needed = cache_ptr->slist_size;

    /* Recall that while we shouldn't have any protected entries at this
     * point, it is possible that some dirty entries may reside on the
     * pinned list at this point.
     */
    HDassert( cache_ptr->slist_size <= 
              (cache_ptr->dLRU_list_size + cache_ptr->pel_size) );
    HDassert( cache_ptr->slist_len  <= 
              (cache_ptr->dLRU_list_len + cache_ptr->pel_len) );

    if(space_needed > 0) { /* we have work to do */
        H5C_cache_entry_t *entry_ptr;
        int     nominated_entries_count = 0;
        size_t  nominated_entries_size = 0;
        haddr_t	nominated_addr;

        HDassert( cache_ptr->slist_len > 0 );

        /* Scan the dirty LRU list from tail forward and nominate sufficient
         * entries to free up the necessary space. 
         */
        entry_ptr = cache_ptr->dLRU_tail_ptr;
        while((nominated_entries_size < space_needed) &&
                (nominated_entries_count < cache_ptr->slist_len) &&
                (entry_ptr != NULL)) {
            HDassert( ! (entry_ptr->is_protected) );
            HDassert( ! (entry_ptr->is_read_only) );
            HDassert( entry_ptr->ro_ref_count == 0 );
            HDassert( entry_ptr->is_dirty );
            HDassert( entry_ptr->in_slist );

            nominated_addr = entry_ptr->addr;
            if(H5AC_add_candidate((H5AC_t *)cache_ptr, nominated_addr) < 0)
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5AC_add_candidate() failed(1).")

            nominated_entries_size += entry_ptr->size;
            nominated_entries_count++;
            entry_ptr = entry_ptr->aux_prev;
        } /* end while */
        HDassert( entry_ptr == NULL );

        /* it is possible that there are some dirty entries on the 
         * protected entry list as well -- scan it too if necessary
         */
        entry_ptr = cache_ptr->pel_head_ptr;
        while((nominated_entries_size < space_needed) &&
                (nominated_entries_count < cache_ptr->slist_len) &&
                (entry_ptr != NULL)) {
            if(entry_ptr->is_dirty) {
                HDassert( ! (entry_ptr->is_protected) );
                HDassert( ! (entry_ptr->is_read_only) );
                HDassert( entry_ptr->ro_ref_count == 0 );
                HDassert( entry_ptr->is_dirty );
                HDassert( entry_ptr->in_slist );

                nominated_addr = entry_ptr->addr;
                if(H5AC_add_candidate((H5AC_t *)cache_ptr, nominated_addr) < 0)
                    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5AC_add_candidate() failed(2).")

                nominated_entries_size += entry_ptr->size;
                nominated_entries_count++;
            } /* end if */

            entry_ptr = entry_ptr->next;
        } /* end while */

        HDassert( nominated_entries_count == cache_ptr->slist_len );
        HDassert( nominated_entries_size == space_needed );
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5C_construct_candidate_list__clean_cache() */
#endif /* H5_HAVE_PARALLEL */


/*-------------------------------------------------------------------------
 * Function:    H5C_construct_candidate_list__min_clean
 *
 * Purpose:     Construct the list of entries that should be flushed to 
 *		get the cache back within its min clean constraints.
 *
 *		This function is used in managing sync points, and 
 *		shouldn't be used elsewhere.
 *
 * Return:      Success:        SUCCEED
 *
 *              Failure:        FAIL
 *
 * Programmer:  John Mainzer
 *              3/17/10
 *
 *-------------------------------------------------------------------------
 */
#ifdef H5_HAVE_PARALLEL
herr_t
H5C_construct_candidate_list__min_clean(H5C_t * cache_ptr)
{
    size_t              space_needed = 0;
    herr_t              ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert( cache_ptr != NULL );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );

    /* compute the number of bytes (if any) that must be flushed to get the 
     * cache back within its min clean constraints.
     */
    if(cache_ptr->max_cache_size > cache_ptr->index_size) {
        if(((cache_ptr->max_cache_size - cache_ptr->index_size) +
               cache_ptr->cLRU_list_size) >= cache_ptr->min_clean_size)
            space_needed = 0;
        else
            space_needed = cache_ptr->min_clean_size -
                ((cache_ptr->max_cache_size - cache_ptr->index_size) +
                 cache_ptr->cLRU_list_size);
    } /* end if */
    else {
        if(cache_ptr->min_clean_size <= cache_ptr->cLRU_list_size)
           space_needed = 0;
        else
            space_needed = cache_ptr->min_clean_size -
                           cache_ptr->cLRU_list_size;
    } /* end else */

    if(space_needed > 0) { /* we have work to do */
        H5C_cache_entry_t *entry_ptr;
        int    nominated_entries_count = 0;
        size_t nominated_entries_size = 0;

        HDassert( cache_ptr->slist_len > 0 );

        /* Scan the dirty LRU list from tail forward and nominate sufficient
         * entries to free up the necessary space. 
         */
        entry_ptr = cache_ptr->dLRU_tail_ptr;
        while((nominated_entries_size < space_needed) &&
                (nominated_entries_count < cache_ptr->slist_len) &&
                (entry_ptr != NULL)) {
            haddr_t		nominated_addr;

            HDassert( ! (entry_ptr->is_protected) );
            HDassert( ! (entry_ptr->is_read_only) );
            HDassert( entry_ptr->ro_ref_count == 0 );
            HDassert( entry_ptr->is_dirty );
            HDassert( entry_ptr->in_slist );

            nominated_addr = entry_ptr->addr;
            if(H5AC_add_candidate((H5AC_t *)cache_ptr, nominated_addr) < 0)
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5AC_add_candidate() failed.")

            nominated_entries_size += entry_ptr->size;
            nominated_entries_count++;
            entry_ptr = entry_ptr->aux_prev;
        } /* end while */
        HDassert( nominated_entries_count <= cache_ptr->slist_len );
        HDassert( nominated_entries_size >= space_needed );
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5C_construct_candidate_list__min_clean() */
#endif /* H5_HAVE_PARALLEL */


/*-------------------------------------------------------------------------
 * Function:    H5C_create
 *
 * Purpose:     Allocate, initialize, and return the address of a new
 *		instance of H5C_t.
 *
 *		In general, the max_cache_size parameter must be positive,
 *		and the min_clean_size parameter must lie in the closed
 *		interval [0, max_cache_size].
 *
 *		The check_write_permitted parameter must either be NULL,
 *		or point to a function of type H5C_write_permitted_func_t.
 *		If it is NULL, the cache will use the write_permitted
 *		flag to determine whether writes are permitted.
 *
 * Return:      Success:        Pointer to the new instance.
 *
 *              Failure:        NULL
 *
 * Programmer:  John Mainzer
 *              6/2/04
 *
 *-------------------------------------------------------------------------
 */
H5C_t *
H5C_create(size_t		      max_cache_size,
           size_t		      min_clean_size,
           int			      max_type_id,
           const char *		      (* type_name_table_ptr),
           H5C_write_permitted_func_t check_write_permitted,
           hbool_t		      write_permitted,
           H5C_log_flush_func_t       log_flush,
           void *                     aux_ptr)
{
    int i;
    H5C_t * cache_ptr = NULL;
    H5C_t * ret_value = NULL;      /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    HDassert( max_cache_size >= H5C__MIN_MAX_CACHE_SIZE );
    HDassert( max_cache_size <= H5C__MAX_MAX_CACHE_SIZE );
    HDassert( min_clean_size <= max_cache_size );

    HDassert( max_type_id >= 0 );
    HDassert( max_type_id < H5C__MAX_NUM_TYPE_IDS );
    HDassert( type_name_table_ptr );

    HDassert( ( write_permitted == TRUE ) || ( write_permitted == FALSE ) );

    for ( i = 0; i <= max_type_id; i++ ) {

        HDassert( (type_name_table_ptr)[i] );
        HDassert( HDstrlen(( type_name_table_ptr)[i]) > 0 );
    }

    if ( NULL == (cache_ptr = H5FL_CALLOC(H5C_t)) ) {

	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, \
                    "memory allocation failed")
    }

    if ( (cache_ptr->slist_ptr = H5SL_create(H5SL_TYPE_HADDR, NULL)) == NULL ) {

        HGOTO_ERROR(H5E_CACHE, H5E_CANTCREATE, NULL, "can't create skip list.")
    }

    /* If we get this far, we should succeed.  Go ahead and initialize all
     * the fields.
     */

    cache_ptr->magic 				= H5C__H5C_T_MAGIC;

    cache_ptr->flush_in_progress		= FALSE;

    cache_ptr->trace_file_ptr			= NULL;

    cache_ptr->aux_ptr				= aux_ptr;

    cache_ptr->max_type_id			= max_type_id;

    cache_ptr->type_name_table_ptr		= type_name_table_ptr;

    cache_ptr->max_cache_size			= max_cache_size;
    cache_ptr->min_clean_size			= min_clean_size;

    cache_ptr->check_write_permitted		= check_write_permitted;
    cache_ptr->write_permitted			= write_permitted;

    cache_ptr->log_flush			= log_flush;

    cache_ptr->evictions_enabled		= TRUE;

    cache_ptr->index_len			= 0;
    cache_ptr->index_size			= (size_t)0;
    cache_ptr->clean_index_size			= (size_t)0;
    cache_ptr->dirty_index_size			= (size_t)0;

    cache_ptr->slist_len			= 0;
    cache_ptr->slist_size			= (size_t)0;

#if H5C_DO_SANITY_CHECKS
    cache_ptr->slist_len_increase		= 0;
    cache_ptr->slist_size_increase		= 0;
#endif /* H5C_DO_SANITY_CHECKS */

    for ( i = 0; i < H5C__HASH_TABLE_LEN; i++ )
    {
        (cache_ptr->index)[i] = NULL;
    }

    cache_ptr->pl_len				= 0;
    cache_ptr->pl_size				= (size_t)0;
    cache_ptr->pl_head_ptr			= NULL;
    cache_ptr->pl_tail_ptr			= NULL;

    cache_ptr->pel_len				= 0;
    cache_ptr->pel_size				= (size_t)0;
    cache_ptr->pel_head_ptr			= NULL;
    cache_ptr->pel_tail_ptr			= NULL;

    cache_ptr->LRU_list_len			= 0;
    cache_ptr->LRU_list_size			= (size_t)0;
    cache_ptr->LRU_head_ptr			= NULL;
    cache_ptr->LRU_tail_ptr			= NULL;

    cache_ptr->cLRU_list_len			= 0;
    cache_ptr->cLRU_list_size			= (size_t)0;
    cache_ptr->cLRU_head_ptr			= NULL;
    cache_ptr->cLRU_tail_ptr			= NULL;

    cache_ptr->dLRU_list_len			= 0;
    cache_ptr->dLRU_list_size			= (size_t)0;
    cache_ptr->dLRU_head_ptr			= NULL;
    cache_ptr->dLRU_tail_ptr			= NULL;

    cache_ptr->size_increase_possible		= FALSE;
    cache_ptr->flash_size_increase_possible     = FALSE;
    cache_ptr->flash_size_increase_threshold    = 0;
    cache_ptr->size_decrease_possible		= FALSE;
    cache_ptr->resize_enabled			= FALSE;
    cache_ptr->cache_full			= FALSE;
    cache_ptr->size_decreased			= FALSE;

    (cache_ptr->resize_ctl).version		= H5C__CURR_AUTO_SIZE_CTL_VER;
    (cache_ptr->resize_ctl).rpt_fcn		= NULL;
    (cache_ptr->resize_ctl).set_initial_size	= FALSE;
    (cache_ptr->resize_ctl).initial_size	= H5C__DEF_AR_INIT_SIZE;
    (cache_ptr->resize_ctl).min_clean_fraction	= H5C__DEF_AR_MIN_CLEAN_FRAC;
    (cache_ptr->resize_ctl).max_size		= H5C__DEF_AR_MAX_SIZE;
    (cache_ptr->resize_ctl).min_size		= H5C__DEF_AR_MIN_SIZE;
    (cache_ptr->resize_ctl).epoch_length	= H5C__DEF_AR_EPOCH_LENGTH;

    (cache_ptr->resize_ctl).incr_mode		= H5C_incr__off;
    (cache_ptr->resize_ctl).lower_hr_threshold	= H5C__DEF_AR_LOWER_THRESHHOLD;
    (cache_ptr->resize_ctl).increment	        = H5C__DEF_AR_INCREMENT;
    (cache_ptr->resize_ctl).apply_max_increment	= TRUE;
    (cache_ptr->resize_ctl).max_increment	= H5C__DEF_AR_MAX_INCREMENT;

    (cache_ptr->resize_ctl).flash_incr_mode     = H5C_flash_incr__off;
    (cache_ptr->resize_ctl).flash_multiple      = 1.0;
    (cache_ptr->resize_ctl).flash_threshold     = 0.25;

    (cache_ptr->resize_ctl).decr_mode		= H5C_decr__off;
    (cache_ptr->resize_ctl).upper_hr_threshold	= H5C__DEF_AR_UPPER_THRESHHOLD;
    (cache_ptr->resize_ctl).decrement	        = H5C__DEF_AR_DECREMENT;
    (cache_ptr->resize_ctl).apply_max_decrement	= TRUE;
    (cache_ptr->resize_ctl).max_decrement	= H5C__DEF_AR_MAX_DECREMENT;
    (cache_ptr->resize_ctl).epochs_before_eviction = H5C__DEF_AR_EPCHS_B4_EVICT;
    (cache_ptr->resize_ctl).apply_empty_reserve = TRUE;
    (cache_ptr->resize_ctl).empty_reserve	= H5C__DEF_AR_EMPTY_RESERVE;

    cache_ptr->epoch_markers_active		= 0;

    /* no need to initialize the ring buffer itself */
    cache_ptr->epoch_marker_ringbuf_first	= 1;
    cache_ptr->epoch_marker_ringbuf_last	= 0;
    cache_ptr->epoch_marker_ringbuf_size	= 0;

    for ( i = 0; i < H5C__MAX_EPOCH_MARKERS; i++ )
    {
        (cache_ptr->epoch_marker_active)[i]		 = FALSE;
#ifndef NDEBUG
        ((cache_ptr->epoch_markers)[i]).magic		 =
					       H5C__H5C_CACHE_ENTRY_T_MAGIC;
#endif /* NDEBUG */
        ((cache_ptr->epoch_markers)[i]).addr		 = (haddr_t)i;
        ((cache_ptr->epoch_markers)[i]).size		 = (size_t)0;
        ((cache_ptr->epoch_markers)[i]).type		 = &epoch_marker_class;
        ((cache_ptr->epoch_markers)[i]).is_dirty	 = FALSE;
        ((cache_ptr->epoch_markers)[i]).dirtied		 = FALSE;
        ((cache_ptr->epoch_markers)[i]).is_protected	 = FALSE;
	((cache_ptr->epoch_markers)[i]).is_read_only	 = FALSE;
	((cache_ptr->epoch_markers)[i]).ro_ref_count	 = 0;
        ((cache_ptr->epoch_markers)[i]).is_pinned	 = FALSE;
        ((cache_ptr->epoch_markers)[i]).in_slist	 = FALSE;
        ((cache_ptr->epoch_markers)[i]).ht_next		 = NULL;
        ((cache_ptr->epoch_markers)[i]).ht_prev		 = NULL;
        ((cache_ptr->epoch_markers)[i]).next		 = NULL;
        ((cache_ptr->epoch_markers)[i]).prev		 = NULL;
        ((cache_ptr->epoch_markers)[i]).aux_next	 = NULL;
        ((cache_ptr->epoch_markers)[i]).aux_prev	 = NULL;
#if H5C_COLLECT_CACHE_ENTRY_STATS
        ((cache_ptr->epoch_markers)[i]).accesses	 = 0;
        ((cache_ptr->epoch_markers)[i]).clears		 = 0;
        ((cache_ptr->epoch_markers)[i]).flushes		 = 0;
        ((cache_ptr->epoch_markers)[i]).pins		 = 0;
#endif /* H5C_COLLECT_CACHE_ENTRY_STATS */
    }

    if ( H5C_reset_cache_hit_rate_stats(cache_ptr) != SUCCEED ) {

        /* this should be impossible... */
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, NULL, \
                    "H5C_reset_cache_hit_rate_stats failed.")
    }

    H5C_stats__reset(cache_ptr);

    cache_ptr->prefix[0]			= '\0';  /* empty string */

    /* Set return value */
    ret_value = cache_ptr;

done:

    if ( ret_value == 0 ) {

        if ( cache_ptr != NULL ) {

            if ( cache_ptr->slist_ptr != NULL )
                H5SL_close(cache_ptr->slist_ptr);

            cache_ptr->magic = 0;
            cache_ptr = H5FL_FREE(H5C_t, cache_ptr);

        } /* end if */

    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C_create() */


/*-------------------------------------------------------------------------
 * Function:    H5C_def_auto_resize_rpt_fcn
 *
 * Purpose:     Print results of a automatic cache resize.
 *
 *		This function should only be used where HDprintf() behaves
 *		well -- i.e. not on Windows.
 *
 * Return:      void
 *
 * Programmer:  John Mainzer
 *		10/27/04
 *
 *-------------------------------------------------------------------------
 */
void
H5C_def_auto_resize_rpt_fcn(H5C_t * cache_ptr,
#ifndef NDEBUG
                            int32_t version,
#else /* NDEBUG */
                            int32_t UNUSED version,
#endif /* NDEBUG */
                            double hit_rate,
                            enum H5C_resize_status status,
                            size_t old_max_cache_size,
                            size_t new_max_cache_size,
                            size_t old_min_clean_size,
                            size_t new_min_clean_size)
{
    HDassert( cache_ptr != NULL );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );
    HDassert( version == H5C__CURR_AUTO_RESIZE_RPT_FCN_VER );

    switch ( status )
    {
        case in_spec:
            HDfprintf(stdout,
                      "%sAuto cache resize -- no change. (hit rate = %lf)\n",
                      cache_ptr->prefix, hit_rate);
            break;

        case increase:
            HDassert( hit_rate < (cache_ptr->resize_ctl).lower_hr_threshold );
            HDassert( old_max_cache_size < new_max_cache_size );

            HDfprintf(stdout,
                      "%sAuto cache resize -- hit rate (%lf) out of bounds low (%6.5lf).\n",
                      cache_ptr->prefix, hit_rate,
                      (cache_ptr->resize_ctl).lower_hr_threshold);

            HDfprintf(stdout,
                    "%s	cache size increased from (%Zu/%Zu) to (%Zu/%Zu).\n",
                    cache_ptr->prefix,
                    old_max_cache_size,
                    old_min_clean_size,
                    new_max_cache_size,
                    new_min_clean_size);
            break;

        case flash_increase:
            HDassert( old_max_cache_size < new_max_cache_size );

            HDfprintf(stdout,
                    "%sflash cache resize(%d) -- size threshold = %Zu.\n",
                    cache_ptr->prefix,
                    (int)((cache_ptr->resize_ctl).flash_incr_mode),
                    cache_ptr->flash_size_increase_threshold);

            HDfprintf(stdout,
                  "%s cache size increased from (%Zu/%Zu) to (%Zu/%Zu).\n",
                   cache_ptr->prefix,
                   old_max_cache_size,
                   old_min_clean_size,
                   new_max_cache_size,
                   new_min_clean_size);
                break;

        case decrease:
            HDassert( old_max_cache_size > new_max_cache_size );

            switch ( (cache_ptr->resize_ctl).decr_mode )
            {
                case H5C_decr__off:
                    HDfprintf(stdout,
                              "%sAuto cache resize -- decrease off.  HR = %lf\n",
                              cache_ptr->prefix, hit_rate);
                    break;

                case H5C_decr__threshold:
                    HDassert( hit_rate >
                              (cache_ptr->resize_ctl).upper_hr_threshold );

                    HDfprintf(stdout,
                              "%sAuto cache resize -- decrease by threshold.  HR = %lf > %6.5lf\n",
                              cache_ptr->prefix, hit_rate,
                              (cache_ptr->resize_ctl).upper_hr_threshold);

                    HDfprintf(stdout, "%sout of bounds high (%6.5lf).\n",
                              cache_ptr->prefix,
                              (cache_ptr->resize_ctl).upper_hr_threshold);
                    break;

                case H5C_decr__age_out:
                    HDfprintf(stdout,
                              "%sAuto cache resize -- decrease by ageout.  HR = %lf\n",
                              cache_ptr->prefix, hit_rate);
                    break;

                case H5C_decr__age_out_with_threshold:
                    HDassert( hit_rate >
                              (cache_ptr->resize_ctl).upper_hr_threshold );

                    HDfprintf(stdout,
                              "%sAuto cache resize -- decrease by ageout with threshold. HR = %lf > %6.5lf\n",
                              cache_ptr->prefix, hit_rate,
                              (cache_ptr->resize_ctl).upper_hr_threshold);
                    break;

                default:
                    HDfprintf(stdout,
                              "%sAuto cache resize -- decrease by unknown mode.  HR = %lf\n",
                              cache_ptr->prefix, hit_rate);
            }

            HDfprintf(stdout,
                      "%s	cache size decreased from (%Zu/%Zu) to (%Zu/%Zu).\n",
                      cache_ptr->prefix,
                      old_max_cache_size,
                      old_min_clean_size,
                      new_max_cache_size,
                      new_min_clean_size);
            break;

        case at_max_size:
            HDfprintf(stdout,
                      "%sAuto cache resize -- hit rate (%lf) out of bounds low (%6.5lf).\n",
                      cache_ptr->prefix, hit_rate,
                      (cache_ptr->resize_ctl).lower_hr_threshold);
            HDfprintf(stdout,
                      "%s	cache already at maximum size so no change.\n",
                      cache_ptr->prefix);
            break;

        case at_min_size:
            HDfprintf(stdout,
                      "%sAuto cache resize -- hit rate (%lf) -- can't decrease.\n",
                      cache_ptr->prefix, hit_rate);
            HDfprintf(stdout, "%s	cache already at minimum size.\n",
                      cache_ptr->prefix);
            break;

        case increase_disabled:
            HDfprintf(stdout,
                      "%sAuto cache resize -- increase disabled -- HR = %lf.",
                      cache_ptr->prefix, hit_rate);
            break;

        case decrease_disabled:
            HDfprintf(stdout,
                      "%sAuto cache resize -- decrease disabled -- HR = %lf.\n",
                      cache_ptr->prefix, hit_rate);
            break;

        case not_full:
            HDassert( hit_rate < (cache_ptr->resize_ctl).lower_hr_threshold );

            HDfprintf(stdout,
                      "%sAuto cache resize -- hit rate (%lf) out of bounds low (%6.5lf).\n",
                      cache_ptr->prefix, hit_rate,
                      (cache_ptr->resize_ctl).lower_hr_threshold);
            HDfprintf(stdout,
                      "%s	cache not full so no increase in size.\n",
                      cache_ptr->prefix);
            break;

        default:
            HDfprintf(stdout, "%sAuto cache resize -- unknown status code.\n",
                      cache_ptr->prefix);
            break;
    }

    return;

} /* H5C_def_auto_resize_rpt_fcn() */


/*-------------------------------------------------------------------------
 * Function:    H5C_dest
 *
 * Purpose:     Flush all data to disk and destroy the cache.
 *
 *              This function fails if any object are protected since the
 *              resulting file might not be consistent.
 *
 *		The primary_dxpl_id and secondary_dxpl_id parameters
 *		specify the dxpl_ids used on the first write occasioned
 *		by the destroy (primary_dxpl_id), and on all subsequent
 *		writes (secondary_dxpl_id).  This is useful in the metadata
 *		cache, but may not be needed elsewhere.  If so, just use the
 *		same dxpl_id for both parameters.
 *
 *		Note that *cache_ptr has been freed upon successful return.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *		6/2/04
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_dest(H5F_t * f,
         hid_t	 primary_dxpl_id,
         hid_t	 secondary_dxpl_id)
{
    H5C_t * cache_ptr = f->shared->cache;
    herr_t ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(cache_ptr);
    HDassert(cache_ptr->magic == H5C__H5C_T_MAGIC);

    /* Flush and invalidate all cache entries */
    if(H5C_flush_invalidate_cache(f, primary_dxpl_id, secondary_dxpl_id,
                H5C__NO_FLAGS_SET) < 0 )
        HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "unable to flush cache")

    if(cache_ptr->slist_ptr != NULL) {
        H5SL_close(cache_ptr->slist_ptr);
        cache_ptr->slist_ptr = NULL;
    } /* end if */

    cache_ptr->magic = 0;

    cache_ptr = H5FL_FREE(H5C_t, cache_ptr);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5C_dest() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5C_expunge_entry
 *
 * Purpose:     Use this function to tell the cache to expunge an entry
 * 		from the cache without writing it to disk even if it is
 * 		dirty.  The entry may not be either pinned or protected.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              6/29/06
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_expunge_entry(H5F_t *             f,
                  hid_t               primary_dxpl_id,
                  hid_t               secondary_dxpl_id,
                  const H5C_class_t * type,
                  haddr_t 	      addr,
                  unsigned 	      flags)
{
    H5C_t *		cache_ptr;
    herr_t		result;
    hbool_t		first_flush = TRUE;
    H5C_cache_entry_t *	entry_ptr = NULL;
    herr_t		ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert( f );
    HDassert( f->shared );

    cache_ptr = f->shared->cache;

    HDassert( cache_ptr );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );
    HDassert( type );
    HDassert( type->clear );
    HDassert( type->dest );
    HDassert( H5F_addr_defined(addr) );

#if H5C_DO_EXTREME_SANITY_CHECKS
        if ( H5C_validate_lru_list(cache_ptr) < 0 ) {

                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                            "LRU sanity check failed.\n");
        }
#endif /* H5C_DO_EXTREME_SANITY_CHECKS */

    H5C__SEARCH_INDEX(cache_ptr, addr, entry_ptr, FAIL)

    if ( ( entry_ptr == NULL ) || ( entry_ptr->type != type ) ) {

        /* the target doesn't exist in the cache, so we are done. */
        HGOTO_DONE(SUCCEED)
    }

    HDassert( entry_ptr->addr == addr );
    HDassert( entry_ptr->type == type );

    if ( entry_ptr->is_protected ) {

        HGOTO_ERROR(H5E_CACHE, H5E_CANTEXPUNGE, FAIL, \
		    "Target entry is protected.")
    }

    if ( entry_ptr->is_pinned ) {

        HGOTO_ERROR(H5E_CACHE, H5E_CANTEXPUNGE, FAIL, \
		    "Target entry is pinned.")
    }

    /* Pass along 'free file space' flag to cache client */
    entry_ptr->free_file_space_on_destroy = ( (flags & H5C__FREE_FILE_SPACE_FLAG) != 0 );

    /* If we get this far, call H5C_flush_single_entry() with the
     * H5C__FLUSH_INVALIDATE_FLAG and the H5C__FLUSH_CLEAR_ONLY_FLAG.
     * This will clear the entry, and then delete it from the cache.
     */
    result = H5C_flush_single_entry(f,
                                    primary_dxpl_id,
                                    secondary_dxpl_id,
                                    entry_ptr->type,
                                    entry_ptr->addr,
                                    H5C__FLUSH_INVALIDATE_FLAG | H5C__FLUSH_CLEAR_ONLY_FLAG,
                                    &first_flush,
                                    TRUE);

    if ( result < 0 ) {

        HGOTO_ERROR(H5E_CACHE, H5E_CANTEXPUNGE, FAIL, \
                    "H5C_flush_single_entry() failed.")
    }

done:

#if H5C_DO_EXTREME_SANITY_CHECKS
        if ( H5C_validate_lru_list(cache_ptr) < 0 ) {

                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                            "LRU sanity check failed.\n");
        }
#endif /* H5C_DO_EXTREME_SANITY_CHECKS */

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C_expunge_entry() */


/*-------------------------------------------------------------------------
 * Function:    H5C_flush_cache
 *
 * Purpose:	Flush (and possibly destroy) the entries contained in the
 *		specified cache.
 *
 *		If the cache contains protected entries, the function will
 *		fail, as protected entries cannot be flushed.  However
 *		all unprotected entries should be flushed before the
 *		function returns failure.
 *
 *		The primary_dxpl_id and secondary_dxpl_id parameters
 *		specify the dxpl_ids used on the first write occasioned
 *		by the flush (primary_dxpl_id), and on all subsequent
 *		writes (secondary_dxpl_id).  This is useful in the metadata
 *		cache, but may not be needed elsewhere.  If so, just use the
 *		same dxpl_id for both parameters.
 *
 * Return:      Non-negative on success/Negative on failure or if there was
 *		a request to flush all items and something was protected.
 *
 * Programmer:  John Mainzer
 *		6/2/04
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_flush_cache(H5F_t *f, hid_t primary_dxpl_id, hid_t secondary_dxpl_id, unsigned flags)
{
    H5C_t * cache_ptr = f->shared->cache;
    herr_t              status;
    herr_t		ret_value = SUCCEED;
    hbool_t             destroy;
    hbool_t		flushed_entries_last_pass;
    hbool_t		flush_marked_entries;
    hbool_t		first_flush = TRUE;
    hbool_t		ignore_protected;
    hbool_t		tried_to_flush_protected_entry = FALSE;
    int32_t		passes = 0;
    int32_t		protected_entries = 0;
    H5SL_node_t * 	node_ptr = NULL;
    H5C_cache_entry_t *	entry_ptr = NULL;
    H5C_cache_entry_t *	next_entry_ptr = NULL;
#if H5C_DO_SANITY_CHECKS
    int64_t		flushed_entries_count;
    size_t		flushed_entries_size;
    int64_t		initial_slist_len;
    size_t              initial_slist_size;
#endif /* H5C_DO_SANITY_CHECKS */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert( cache_ptr );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );
    HDassert( cache_ptr->slist_ptr );

    ignore_protected = ( (flags & H5C__FLUSH_IGNORE_PROTECTED_FLAG) != 0 );

    destroy = ( (flags & H5C__FLUSH_INVALIDATE_FLAG) != 0 );

    /* note that flush_marked_entries is set to FALSE if destroy is TRUE */
    flush_marked_entries = ( ( (flags & H5C__FLUSH_MARKED_ENTRIES_FLAG) != 0 )
                             &&
                             ( ! destroy )
                           );

    HDassert( ! ( destroy && ignore_protected ) );

    HDassert( ! ( cache_ptr->flush_in_progress ) );

    cache_ptr->flush_in_progress = TRUE;

    if ( destroy ) {

        status = H5C_flush_invalidate_cache(f,
			                    primary_dxpl_id,
					    secondary_dxpl_id,
					    flags);

        if ( status < 0 ) {

            /* This shouldn't happen -- if it does, we are toast so
             * just scream and die.
             */
            HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, \
			"flush invalidate failed.")
        }
    } else {
	/* When we are only flushing marked entries, the slist will usually
	 * still contain entries when we have flushed everything we should.
	 * Thus we track whether we have flushed any entries in the last
	 * pass, and terminate if we haven't.
	 */

	flushed_entries_last_pass = TRUE;

        while ( ( passes < H5C__MAX_PASSES_ON_FLUSH ) &&
		( cache_ptr->slist_len != 0 ) &&
		( protected_entries == 0 ) &&
		( flushed_entries_last_pass ) )
	{
	    flushed_entries_last_pass = FALSE;
            node_ptr = H5SL_first(cache_ptr->slist_ptr);

            if ( node_ptr != NULL ) {

                next_entry_ptr = (H5C_cache_entry_t *)H5SL_item(node_ptr);

                if ( next_entry_ptr == NULL ) {

                    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
		                "next_entry_ptr == NULL 1 ?!?!");
                }
#ifndef NDEBUG
		HDassert( next_entry_ptr->magic ==
                          H5C__H5C_CACHE_ENTRY_T_MAGIC );
#endif /* NDEBUG */
	        HDassert( next_entry_ptr->is_dirty );
                HDassert( next_entry_ptr->in_slist );

            } else {

                next_entry_ptr = NULL;

            }

	    HDassert( node_ptr != NULL );

#if H5C_DO_SANITY_CHECKS
	    /* For sanity checking, try to verify that the skip list has
	     * the expected size and number of entries at the end of each
	     * internal while loop (see below).
	     *
	     * Doing this get a bit tricky, as depending on flags, we may
	     * or may not flush all the entries in the slist.
	     *
	     * To make things more entertaining, with the advent of the
	     * fractal heap, the entry flush callback can cause entries
	     * to be dirtied, resized, and/or moved.
	     *
	     * To deal with this, we first make note of the initial
	     * skip list length and size:
	     */
            initial_slist_len = cache_ptr->slist_len;
            initial_slist_size = cache_ptr->slist_size;

	    /* We then zero counters that we use to track the number
	     * and total size of entries flushed:
	     */
            flushed_entries_count = 0;
            flushed_entries_size = 0;

	    /* As mentioned above, there is the possibility that
	     * entries will be dirtied, resized, and/or flushed during
	     * our pass through the skip list.  To capture the number
	     * of entries added, and the skip list size delta,
	     * zero the slist_len_increase and slist_size_increase of
	     * the cache's instance of H5C_t.  These fields will be
	     * updated elsewhere to account for slist insertions and/or
	     * dirty entry size changes.
	     */
	    cache_ptr->slist_len_increase = 0;
	    cache_ptr->slist_size_increase = 0;

	    /* at the end of the loop, use these values to compute the
	     * expected slist length and size and compare this with the
	     * value recorded in the cache's instance of H5C_t.
	     */
#endif /* H5C_DO_SANITY_CHECKS */

	    while ( node_ptr != NULL )
	    {
                entry_ptr = next_entry_ptr;

                /* With the advent of the fractal heap, it is possible
		 * that the flush callback will dirty and/or resize
		 * other entries in the cache.  In particular, while
		 * Quincey has promised me that this will never happen,
		 * it is possible that the flush callback for an
		 * entry may protect an entry that is not in the cache,
		 * perhaps causing the cache to flush and possibly
		 * evict the entry associated with node_ptr to make
		 * space for the new entry.
		 *
		 * Thus we do a bit of extra sanity checking on entry_ptr,
		 * and break out of this scan of the skip list if we
		 * detect minor problems.  We have a bit of leaway on the
		 * number of passes though the skip list, so this shouldn't
		 * be an issue in the flush in and of itself, as it should
		 * be all but impossible for this to happen more than once
		 * in any flush.
		 *
		 * Observe that that breaking out of the scan early
		 * shouldn't break the sanity checks just after the end
		 * of this while loop.
		 *
		 * If an entry has merely been marked clean and removed from
		 * the s-list, we simply break out of the scan.
		 *
		 * If the entry has been evicted, we flag an error and
		 * exit.
		 */
#ifndef NDEBUG
		if ( entry_ptr->magic != H5C__H5C_CACHE_ENTRY_T_MAGIC ) {

                    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                                "entry_ptr->magic invalid ?!?!");

	        } else
#endif /* NDEBUG */
		if ( ( ! entry_ptr->is_dirty ) ||
                     ( ! entry_ptr->in_slist ) ) {

                    /* the s-list has been modified out from under us.
                     * set node_ptr to NULL and break out of the loop.
                     */
                    node_ptr = NULL;
                    break;
                }

                /* increment node pointer now, before we delete its target
                 * from the slist.
                 */
                node_ptr = H5SL_next(node_ptr);

                if ( node_ptr != NULL ) {
                    next_entry_ptr = (H5C_cache_entry_t *)H5SL_item(node_ptr);

                    if ( next_entry_ptr == NULL ) {
                        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                                    "next_entry_ptr == NULL 2 ?!?!");
		    }
#ifndef NDEBUG
		    HDassert( next_entry_ptr->magic ==
                              H5C__H5C_CACHE_ENTRY_T_MAGIC );
#endif /* NDEBUG */
                    HDassert( next_entry_ptr->is_dirty );
                    HDassert( next_entry_ptr->in_slist );
                } else {
                    next_entry_ptr = NULL;
                }

                HDassert( entry_ptr != NULL );
                HDassert( entry_ptr->in_slist );

                if ( ( ! flush_marked_entries ) ||
                     ( entry_ptr->flush_marker ) ) {

                    if ( entry_ptr->is_protected ) {

                        /* we probably have major problems -- but lets flush
                         * everything we can before we decide whether to flag
                         * an error.
                         */
                        tried_to_flush_protected_entry = TRUE;
	                protected_entries++;

                    } else if ( entry_ptr->is_pinned ) {
			/* Test to see if we are can flush the entry now.
			 * If we can, go ahead and flush.  Note that we
			 * aren't trying to do a destroy here, so that
			 * is not an issue.
			 */
                        if ( TRUE ) { /* When we get to multithreaded cache,
                                       * we will need either locking code,
                                       * and/or a test to see if the entry
                                       * is in flushable condition here.
                                       */
#if H5C_DO_SANITY_CHECKS
                            flushed_entries_count++;
                            flushed_entries_size += entry_ptr->size;
#endif /* H5C_DO_SANITY_CHECKS */
                            status = H5C_flush_single_entry(f,
                                                            primary_dxpl_id,
                                                            secondary_dxpl_id,
                                                            NULL,
                                                            entry_ptr->addr,
                                                            flags,
                                                            &first_flush,
                                                            FALSE);
                            if ( status < 0 ) {

                                /* This shouldn't happen -- if it does, we are
			         * toast so just scream and die.
                                 */
                                HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, \
                                            "dirty pinned entry flush failed.")
                            }
			    flushed_entries_last_pass = TRUE;
			}
                    } else {
#if H5C_DO_SANITY_CHECKS
                        flushed_entries_count++;
			flushed_entries_size += entry_ptr->size;
#endif /* H5C_DO_SANITY_CHECKS */
                        status = H5C_flush_single_entry(f,
                                                        primary_dxpl_id,
                                                        secondary_dxpl_id,
                                                        NULL,
                                                        entry_ptr->addr,
                                                        flags,
                                                        &first_flush,
                                                        FALSE);
                        if ( status < 0 ) {

                            /* This shouldn't happen -- if it does, we are
			     * toast so just scream and die.
                             */
                            HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, \
                                        "Can't flush entry.")
                        }
			flushed_entries_last_pass = TRUE;
                    }
                }
            } /* while ( node_ptr != NULL ) */

#if H5C_DO_SANITY_CHECKS
            /* Verify that the slist size and length are as expected. */

	    HDassert( (initial_slist_len + cache_ptr->slist_len_increase -
                       flushed_entries_count) == cache_ptr->slist_len );
	    HDassert( (initial_slist_size + cache_ptr->slist_size_increase -
		       flushed_entries_size) == cache_ptr->slist_size );
#endif /* H5C_DO_SANITY_CHECKS */

	    passes++;

	} /* while */

        HDassert( protected_entries <= cache_ptr->pl_len );

        if ( ( ( cache_ptr->pl_len > 0 ) && ( !ignore_protected ) )
             ||
             ( tried_to_flush_protected_entry ) ) {

            HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, \
		        "cache has protected items")
        }

	if ( ( cache_ptr->slist_len != 0 ) &&
	     ( passes >= H5C__MAX_PASSES_ON_FLUSH ) ) {

            HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, \
		        "flush pass limit exceeded.")
	}

#if H5C_DO_SANITY_CHECKS
        if ( ! flush_marked_entries ) {

            HDassert( cache_ptr->slist_len == 0 );
            HDassert( cache_ptr->slist_size == 0 );
        }
#endif /* H5C_DO_SANITY_CHECKS */

    }

done:

    cache_ptr->flush_in_progress = FALSE;

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C_flush_cache() */


/*-------------------------------------------------------------------------
 * Function:    H5C_flush_to_min_clean
 *
 * Purpose:	Flush dirty entries until the caches min clean size is
 *		attained.
 *
 *		This function is used in the implementation of the
 *		metadata cache in PHDF5.  To avoid "messages from the
 *		future", the cache on process 0 can't be allowed to
 *		flush entries until the other processes have reached
 *		the same point in the calculation.  If this constraint
 *		is not met, it is possible that the other processes will
 *		read metadata generated at a future point in the
 *		computation.
 *
 *
 * Return:      Non-negative on success/Negative on failure or if
 *		write is not permitted.
 *
 * Programmer:  John Mainzer
 *		9/16/05
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_flush_to_min_clean(H5F_t * f,
                       hid_t   primary_dxpl_id,
                       hid_t   secondary_dxpl_id)
{
    H5C_t *            cache_ptr;
    herr_t      	result;
    hbool_t		first_flush = TRUE;
    hbool_t		write_permitted;
#if 0 /* modified code -- commented out for now */
    int			i;
    int			flushed_entries_count = 0;
    size_t		flushed_entries_size = 0;
    size_t		space_needed = 0;
    haddr_t	      * flushed_entries_list = NULL;
    H5C_cache_entry_t *	entry_ptr = NULL;
#endif /* JRM */
    herr_t		ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(FAIL)

    HDassert( f );
    HDassert( f->shared );

    cache_ptr = f->shared->cache;

    HDassert( cache_ptr );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );

    if ( cache_ptr->check_write_permitted != NULL ) {

        result = (cache_ptr->check_write_permitted)(f,
                                                    primary_dxpl_id,
                                                    &write_permitted);

        if ( result < 0 ) {

            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                        "Can't get write_permitted")
        }
    } else {

        write_permitted = cache_ptr->write_permitted;
    }

    if ( ! write_permitted ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                    "cache write is not permitted!?!\n");
    }
#if 1 /* original code */
    result = H5C_make_space_in_cache(f,
                                     primary_dxpl_id,
                                     secondary_dxpl_id,
                                     (size_t)0,
                                     write_permitted,
                                     &first_flush);

    if ( result < 0 ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                    "H5C_make_space_in_cache failed.")
    }
#else /* modified code -- commented out for now */
    if ( cache_ptr->max_cache_size > cache_ptr->index_size ) {

        if ( ((cache_ptr->max_cache_size - cache_ptr->index_size) +
               cache_ptr->cLRU_list_size) >= cache_ptr->min_clean_size ) {

            space_needed = 0;

        } else {

            space_needed = cache_ptr->min_clean_size -
                ((cache_ptr->max_cache_size - cache_ptr->index_size) +
                 cache_ptr->cLRU_list_size);
        }
    } else {

        if ( cache_ptr->min_clean_size <= cache_ptr->cLRU_list_size ) {

           space_needed = 0;

        } else {

            space_needed = cache_ptr->min_clean_size -
                           cache_ptr->cLRU_list_size;
        }
    }

    if ( space_needed > 0 ) { /* we have work to do */

        HDassert( cache_ptr->slist_len > 0 );

        /* allocate an array to keep a list of the entries that we
         * mark for flush.  We need this list to touch up the LRU
         * list after the flush.
         */
        flushed_entries_list = (haddr_t *)H5MM_malloc(sizeof(haddr_t) *
                                              (size_t)(cache_ptr->slist_len));

        if ( flushed_entries_list == NULL ) {

            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, \
                        "memory allocation failed for flushed entries list")
        }

        /* Scan the dirty LRU list from tail forward and mark sufficient
         * entries to free up the necessary space.  Keep a list of the
         * entries marked in the order in which they are encountered.
         */
        entry_ptr = cache_ptr->dLRU_tail_ptr;

        while ( ( flushed_entries_size < space_needed ) &&
                ( flushed_entries_count < cache_ptr->slist_len ) &&
                ( entry_ptr != NULL ) )
        {
            HDassert( ! (entry_ptr->is_protected) );
            HDassert( ! (entry_ptr->is_read_only) );
            HDassert( entry_ptr->ro_ref_count == 0 );
            HDassert( entry_ptr->is_dirty );
            HDassert( entry_ptr->in_slist );

            entry_ptr->flush_marker = TRUE;
            flushed_entries_size += entry_ptr->size;
            flushed_entries_list[flushed_entries_count] = entry_ptr->addr;
            flushed_entries_count++;
            entry_ptr = entry_ptr->aux_prev;
        }

        HDassert( flushed_entries_count <= cache_ptr->slist_len );
        HDassert( flushed_entries_size >= space_needed );


        /* Flush the marked entries */
	result = H5C_flush_cache(f, primary_dxpl_id, secondary_dxpl_id,
                                 H5C__FLUSH_MARKED_ENTRIES_FLAG | H5C__FLUSH_IGNORE_PROTECTED_FLAG);

        if ( result < 0 ) {

            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5C_flush_cache failed.")
        }

        /* Now touch up the LRU list so as to place the flushed entries in
         * the order they they would be in if we had flushed them in the
         * order we encountered them in.
         */

        i = 0;
        while ( i < flushed_entries_count )
        {
            H5C__SEARCH_INDEX_NO_STATS(cache_ptr, flushed_entries_list[i], \
                                       entry_ptr, FAIL)

	    /* At present, the above search must always succeed.  However,
             * that may change.  Write the code so we need only remove the
             * following assert in that event.
             */
            HDassert( entry_ptr != NULL );
            H5C__FAKE_RP_FOR_MOST_RECENT_ACCESS(cache_ptr, entry_ptr, FAIL)
            i++;
        }
    } /* if ( space_needed > 0 ) */
#endif /* end modified code -- commented out for now */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5C_flush_to_min_clean() */


/*-------------------------------------------------------------------------
 * Function:    H5C_get_cache_auto_resize_config
 *
 * Purpose:	Copy the current configuration of the cache automatic
 *		re-sizing function into the instance of H5C_auto_size_ctl_t
 *		pointed to by config_ptr.
 *
 * Return:      SUCCEED on success, and FAIL on failure.
 *
 * Programmer:  John Mainzer
 *		10/8/04
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_get_cache_auto_resize_config(const H5C_t * cache_ptr,
                                 H5C_auto_size_ctl_t *config_ptr)
{
    herr_t ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    if ( ( cache_ptr == NULL ) || ( cache_ptr->magic != H5C__H5C_T_MAGIC ) ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Bad cache_ptr on entry.")
    }

    if ( config_ptr == NULL ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Bad config_ptr on entry.")
    }

    *config_ptr = cache_ptr->resize_ctl;

    config_ptr->set_initial_size = FALSE;
    config_ptr->initial_size = cache_ptr->max_cache_size;

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C_get_cache_auto_resize_config() */


/*-------------------------------------------------------------------------
 * Function:    H5C_get_cache_size
 *
 * Purpose:	Return the cache maximum size, the minimum clean size, the
 *		current size, and the current number of entries in
 *              *max_size_ptr, *min_clean_size_ptr, *cur_size_ptr, and
 *		*cur_num_entries_ptr respectively.  If any of these
 *		parameters are NULL, skip that value.
 *
 * Return:      SUCCEED on success, and FAIL on failure.
 *
 * Programmer:  John Mainzer
 *		10/8/04
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_get_cache_size(H5C_t * cache_ptr,
                   size_t * max_size_ptr,
                   size_t * min_clean_size_ptr,
                   size_t * cur_size_ptr,
                   int32_t * cur_num_entries_ptr)
{
    herr_t ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    if ( ( cache_ptr == NULL ) || ( cache_ptr->magic != H5C__H5C_T_MAGIC ) ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Bad cache_ptr on entry.")
    }

    if ( max_size_ptr != NULL ) {

        *max_size_ptr = cache_ptr->max_cache_size;
    }

    if ( min_clean_size_ptr != NULL ) {

        *min_clean_size_ptr = cache_ptr->min_clean_size;
    }

    if ( cur_size_ptr != NULL ) {

        *cur_size_ptr = cache_ptr->index_size;
    }

    if ( cur_num_entries_ptr != NULL ) {

        *cur_num_entries_ptr = cache_ptr->index_len;
    }

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C_get_cache_size() */


/*-------------------------------------------------------------------------
 * Function:    H5C_get_cache_hit_rate
 *
 * Purpose:	Compute and return the current cache hit rate in
 *              *hit_rate_ptr.  If there have been no accesses since the
 *              last time the cache hit rate stats were reset, set
 *		*hit_rate_ptr to 0.0.  On error, *hit_rate_ptr is
 *		undefined.
 *
 * Return:      SUCCEED on success, and FAIL on failure.
 *
 * Programmer:  John Mainzer
 *		10/7/04
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_get_cache_hit_rate(H5C_t * cache_ptr,
                       double * hit_rate_ptr)

{
    herr_t ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    if ( ( cache_ptr == NULL ) || ( cache_ptr->magic != H5C__H5C_T_MAGIC ) ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Bad cache_ptr on entry.")
    }

    if ( hit_rate_ptr == NULL ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Bad hit_rate_ptr on entry.")
    }

    HDassert( cache_ptr->cache_hits >= 0 );
    HDassert( cache_ptr->cache_accesses >= cache_ptr->cache_hits );

    if ( cache_ptr->cache_accesses > 0 ) {

        *hit_rate_ptr = ((double)(cache_ptr->cache_hits)) /
                         ((double)(cache_ptr->cache_accesses));

    } else {

        *hit_rate_ptr = 0.0;
    }

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C_get_cache_hit_rate() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5C_get_entry_status
 *
 * Purpose:     This function is used to determine whether the cache
 *		contains an entry with the specified base address.  If
 *		the entry exists, it also reports some status information
 *		on the entry.
 *
 *		Status information is reported in the locations pointed
 *		to by the size_ptr, in_cache_ptr, is_dirty_ptr, and
 *		is_protected_ptr.  While in_cache_ptr must be defined,
 *		the remaining pointers may be NULL, in which case the
 *		associated data is not reported.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              7/1/05
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_get_entry_status(const H5F_t *f,
                     haddr_t   addr,
                     size_t *  size_ptr,
                     hbool_t * in_cache_ptr,
                     hbool_t * is_dirty_ptr,
                     hbool_t * is_protected_ptr,
		     hbool_t * is_pinned_ptr)
{
    H5C_t             * cache_ptr;
    H5C_cache_entry_t *	entry_ptr = NULL;
    herr_t		ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert( f );
    HDassert( f->shared );

    cache_ptr = f->shared->cache;

    HDassert( cache_ptr != NULL );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );
    HDassert( H5F_addr_defined(addr) );
    HDassert( in_cache_ptr != NULL );

    /* this test duplicates two of the above asserts, but we need an
     * invocation of HGOTO_ERROR to keep the compiler happy.
     */
    if ( ( cache_ptr == NULL ) || ( cache_ptr->magic != H5C__H5C_T_MAGIC ) ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Bad cache_ptr on entry.")
    }

    H5C__SEARCH_INDEX(cache_ptr, addr, entry_ptr, FAIL)

    if ( entry_ptr == NULL ) {

        /* the entry doesn't exist in the cache -- report this
         * and quit.
         */
        *in_cache_ptr = FALSE;

    } else {

        *in_cache_ptr = TRUE;

        if ( size_ptr != NULL ) {

            *size_ptr = entry_ptr->size;
        }

        if ( is_dirty_ptr != NULL ) {

            *is_dirty_ptr = entry_ptr->is_dirty;
        }

        if ( is_protected_ptr != NULL ) {

            *is_protected_ptr = entry_ptr->is_protected;
        }

        if ( is_pinned_ptr != NULL ) {

            *is_pinned_ptr = entry_ptr->is_pinned;
        }
    }

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C_get_entry_status() */


/*-------------------------------------------------------------------------
 * Function:    H5C_get_evictions_enabled()
 *
 * Purpose:     Copy the current value of cache_ptr->evictions_enabled into
 *              *evictions_enabled_ptr.
 *
 * Return:      SUCCEED on success, and FAIL on failure.
 *
 * Programmer:  John Mainzer
 *              7/27/07
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_get_evictions_enabled(const H5C_t *cache_ptr,
                          hbool_t * evictions_enabled_ptr)
{
    herr_t ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    if ( ( cache_ptr == NULL ) || ( cache_ptr->magic != H5C__H5C_T_MAGIC ) ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Bad cache_ptr on entry.")
    }

    if ( evictions_enabled_ptr == NULL ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                    "Bad evictions_enabled_ptr on entry.")
    }

    *evictions_enabled_ptr = cache_ptr->evictions_enabled;

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C_get_evictions_enabled() */


/*-------------------------------------------------------------------------
 * Function:    H5C_get_trace_file_ptr
 *
 * Purpose:     Get the trace_file_ptr field from the cache.
 *
 *              This field will either be NULL (which indicates that trace
 *              file logging is turned off), or contain a pointer to the
 *              open file to which trace file data is to be written.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              1/20/06
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_get_trace_file_ptr(const H5C_t *cache_ptr, FILE **trace_file_ptr_ptr)
{
    FUNC_ENTER_NOAPI_NOERR

    HDassert(cache_ptr);
    HDassert(cache_ptr->magic == H5C__H5C_T_MAGIC);
    HDassert(trace_file_ptr_ptr);

    *trace_file_ptr_ptr = cache_ptr->trace_file_ptr;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5C_get_trace_file_ptr() */


/*-------------------------------------------------------------------------
 * Function:    H5C_get_trace_file_ptr_from_entry
 *
 * Purpose:     Get the trace_file_ptr field from the cache, via an entry.
 *
 *              This field will either be NULL (which indicates that trace
 *              file logging is turned off), or contain a pointer to the
 *              open file to which trace file data is to be written.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              6/9/08
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_get_trace_file_ptr_from_entry(const H5C_cache_entry_t *entry_ptr,
    FILE **trace_file_ptr_ptr)
{
    FUNC_ENTER_NOAPI_NOERR

    /* Sanity checks */
    HDassert(entry_ptr);
    HDassert(entry_ptr->cache_ptr);

    H5C_get_trace_file_ptr(entry_ptr->cache_ptr, trace_file_ptr_ptr);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5C_get_trace_file_ptr_from_entry() */


/*-------------------------------------------------------------------------
 * Function:    H5C_insert_entry
 *
 * Purpose:     Adds the specified thing to the cache.  The thing need not
 *              exist on disk yet, but it must have an address and disk
 *              space reserved.
 *
 *		The primary_dxpl_id and secondary_dxpl_id parameters
 *		specify the dxpl_ids used on the first write occasioned
 *		by the insertion (primary_dxpl_id), and on all subsequent
 *		writes (secondary_dxpl_id).  This is useful in the
 *		metadata cache, but may not be needed elsewhere.  If so,
 *		just use the same dxpl_id for both parameters.
 *
 *		The primary_dxpl_id is the dxpl_id passed to the
 *		check_write_permitted function if such a function has been
 *		provided.
 *
 *		Observe that this function cannot occasion a read.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *		6/2/04
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_insert_entry(H5F_t *             f,
                 hid_t		     primary_dxpl_id,
                 hid_t		     secondary_dxpl_id,
                 const H5C_class_t * type,
                 haddr_t 	     addr,
                 void *		     thing,
                 unsigned int        flags)
{
    H5C_t *             cache_ptr;
    herr_t		result;
    herr_t		ret_value = SUCCEED;    /* Return value */
    hbool_t		first_flush = TRUE;
    hbool_t		insert_pinned;
    hbool_t             set_flush_marker;
    hbool_t		write_permitted = TRUE;
    size_t		empty_space;
    H5C_cache_entry_t *	entry_ptr;
    H5C_cache_entry_t *	test_entry_ptr;

    FUNC_ENTER_NOAPI(FAIL)

    HDassert( f );
    HDassert( f->shared );

    cache_ptr = f->shared->cache;

    HDassert( cache_ptr );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );
    HDassert( type );
    HDassert( type->flush );
    HDassert( type->size );
    HDassert( H5F_addr_defined(addr) );
    HDassert( thing );

#if H5C_DO_EXTREME_SANITY_CHECKS
    if ( H5C_verify_not_in_index(cache_ptr, (H5C_cache_entry_t *)thing) < 0 ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "thing already in index.\n");
    }
#endif /* H5C_DO_SANITY_CHECKS */

#if H5C_DO_EXTREME_SANITY_CHECKS
    if ( H5C_validate_lru_list(cache_ptr) < 0 ) {

            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                        "LRU sanity check failed.\n");
    }
#endif /* H5C_DO_EXTREME_SANITY_CHECKS */

    set_flush_marker = ( (flags & H5C__SET_FLUSH_MARKER_FLAG) != 0 );
    insert_pinned    = ( (flags & H5C__PIN_ENTRY_FLAG) != 0 );

    entry_ptr = (H5C_cache_entry_t *)thing;

    /* verify that the new entry isn't already in the hash table -- scream
     * and die if it is.
     */

    H5C__SEARCH_INDEX(cache_ptr, addr, test_entry_ptr, FAIL)

    if ( test_entry_ptr != NULL ) {

        if ( test_entry_ptr == entry_ptr ) {

            HGOTO_ERROR(H5E_CACHE, H5E_CANTINS, FAIL, \
                        "entry already in cache.")

        } else {

            HGOTO_ERROR(H5E_CACHE, H5E_CANTINS, FAIL, \
                        "duplicate entry in cache.")
        }
    }

#ifndef NDEBUG
    entry_ptr->magic = H5C__H5C_CACHE_ENTRY_T_MAGIC;
#endif /* NDEBUG */
    entry_ptr->cache_ptr = cache_ptr;
    entry_ptr->addr  = addr;
    entry_ptr->type  = type;

    entry_ptr->is_protected = FALSE;
    entry_ptr->is_read_only = FALSE;
    entry_ptr->ro_ref_count = 0;

    entry_ptr->is_pinned = insert_pinned;

    /* newly inserted entries are assumed to be dirty */
    entry_ptr->is_dirty = TRUE;

    /* not protected, so can't be dirtied */
    entry_ptr->dirtied  = FALSE;

    /* Retrieve the size of the thing */
    if((type->size)(f, thing, &(entry_ptr->size)) < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGETSIZE, FAIL, "Can't get size of thing")
    HDassert(entry_ptr->size > 0 &&  entry_ptr->size < H5C_MAX_ENTRY_SIZE);

    entry_ptr->in_slist = FALSE;

#ifdef H5_HAVE_PARALLEL
    entry_ptr->clear_on_unprotect = FALSE;
    entry_ptr->flush_immediately = FALSE;
#endif /* H5_HAVE_PARALLEL */

    entry_ptr->flush_in_progress = FALSE;
    entry_ptr->destroy_in_progress = FALSE;
    entry_ptr->free_file_space_on_destroy = FALSE;

    entry_ptr->ht_next = NULL;
    entry_ptr->ht_prev = NULL;

    entry_ptr->next = NULL;
    entry_ptr->prev = NULL;

    entry_ptr->aux_next = NULL;
    entry_ptr->aux_prev = NULL;

    H5C__RESET_CACHE_ENTRY_STATS(entry_ptr)

    if ( ( cache_ptr->flash_size_increase_possible ) &&
         ( entry_ptr->size > cache_ptr->flash_size_increase_threshold ) ) {

        result = H5C__flash_increase_cache_size(cache_ptr, 0, entry_ptr->size);

        if ( result < 0 ) {

            HGOTO_ERROR(H5E_CACHE, H5E_CANTINS, FAIL, \
                        "H5C__flash_increase_cache_size failed.")
        }
    }

    if ( cache_ptr->index_size >= cache_ptr->max_cache_size ) {

       empty_space = 0;

    } else {

       empty_space = cache_ptr->max_cache_size - cache_ptr->index_size;

    }

    if ( ( cache_ptr->evictions_enabled )
	 &&
         ( ( (cache_ptr->index_size + entry_ptr->size) >
	     cache_ptr->max_cache_size
	   )
	   ||
	   (
	     ( ( empty_space + cache_ptr->clean_index_size ) <
	       cache_ptr->min_clean_size )
	   )
	 )
       ) {

        size_t space_needed;

	if ( empty_space <= entry_ptr->size ) {

            cache_ptr->cache_full = TRUE;
	}

        if ( cache_ptr->check_write_permitted != NULL ) {

            result = (cache_ptr->check_write_permitted)(f,
                                                        primary_dxpl_id,
                                                        &write_permitted);

            if ( result < 0 ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTINS, FAIL, \
                            "Can't get write_permitted")
            }
        } else {

            write_permitted = cache_ptr->write_permitted;
        }

        HDassert( entry_ptr->size <= H5C_MAX_ENTRY_SIZE );

        space_needed = entry_ptr->size;

        if ( space_needed > cache_ptr->max_cache_size ) {

            space_needed = cache_ptr->max_cache_size;
        }

        /* Note that space_needed is just the amount of space that
         * needed to insert the new entry without exceeding the cache
         * size limit.  The subsequent call to H5C_make_space_in_cache()
         * may evict the entries required to free more or less space
         * depending on conditions.  It MAY be less if the cache is
         * currently undersized, or more if the cache is oversized.
         *
         * The cache can exceed its maximum size limit via the following
         * mechanisms:
         *
         * First, it is possible for the cache to grow without
         * bound as long as entries are protected and not unprotected.
         *
         * Second, when writes are not permitted it is also possible
         * for the cache to grow without bound.
         *
         * Finally, we usually don't check to see if the cache is
         * oversized at the end of an unprotect.  As a result, it is
         * possible to have a vastly oversized cache with no protected
         * entries as long as all the protects preceed the unprotects.
         *
         * Since items 1 and 2 are not changing any time soon, I see
         * no point in worrying about the third.
         */

        result = H5C_make_space_in_cache(f,
                                         primary_dxpl_id,
                                         secondary_dxpl_id,
                                         space_needed,
                                         write_permitted,
                                         &first_flush);

        if ( result < 0 ) {

            HGOTO_ERROR(H5E_CACHE, H5E_CANTINS, FAIL, \
                        "H5C_make_space_in_cache failed.")
        }
    }

    H5C__INSERT_IN_INDEX(cache_ptr, entry_ptr, FAIL)

    /* New entries are presumed to be dirty, so this if statement is
     * unnecessary.  Rework it once the rest of the code changes are
     * in and tested.   -- JRM
     */
    if ( entry_ptr->is_dirty ) {

        entry_ptr->flush_marker = set_flush_marker;
        H5C__INSERT_ENTRY_IN_SLIST(cache_ptr, entry_ptr, FAIL)

    } else {

        entry_ptr->flush_marker = FALSE;
    }

    H5C__UPDATE_RP_FOR_INSERTION(cache_ptr, entry_ptr, FAIL)

#if H5C_DO_EXTREME_SANITY_CHECKS
    if ( H5C_validate_lru_list(cache_ptr) < 0 ) {

            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                        "LRU sanity check failed.\n");
    }
#endif /* H5C_DO_EXTREME_SANITY_CHECKS */

    H5C__UPDATE_STATS_FOR_INSERTION(cache_ptr, entry_ptr)

done:

#if H5C_DO_EXTREME_SANITY_CHECKS
    if ( H5C_validate_lru_list(cache_ptr) < 0 ) {

            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                        "LRU sanity check failed.\n");
    }
#endif /* H5C_DO_EXTREME_SANITY_CHECKS */

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C_insert_entry() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5C_mark_entries_as_clean
 *
 * Purpose:     When the H5C code is used to implement the metadata caches
 *		in PHDF5, only the cache with MPI_rank 0 is allowed to
 *		actually write entries to disk -- all other caches must
 *		retain dirty entries until they are advised that the
 *		entries are clean.
 *
 *		This function exists to allow the H5C code to receive these
 *		notifications.
 *
 *		The function receives a list of entry base addresses
 *		which must refer to dirty entries in the cache.  If any
 *		of the entries are either clean or don't exist, the
 *		function flags an error.
 *
 *		The function scans the list of entries and flushes all
 *		those that are currently unprotected with the
 *		H5C__FLUSH_CLEAR_ONLY_FLAG.  Those that are currently
 *		protected are flagged for clearing when they are
 *		unprotected.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              7/5/05
 *
 *-------------------------------------------------------------------------
 */
#ifdef H5_HAVE_PARALLEL
herr_t
H5C_mark_entries_as_clean(H5F_t *  f,
                          hid_t     primary_dxpl_id,
                          hid_t     secondary_dxpl_id,
                          int32_t   ce_array_len,
                          haddr_t * ce_array_ptr)
{
    H5C_t *             cache_ptr;
    hbool_t		first_flush = TRUE;
    int			entries_cleared;
    int			entries_examined;
    int                 i;
    int			initial_list_len;
    haddr_t		addr;
#if H5C_DO_SANITY_CHECKS
    int			pinned_entries_marked = 0;
    int			protected_entries_marked = 0;
    int			other_entries_marked = 0;
    haddr_t		last_addr;
#endif /* H5C_DO_SANITY_CHECKS */
    H5C_cache_entry_t *	clear_ptr = NULL;
    H5C_cache_entry_t *	entry_ptr = NULL;
    herr_t		ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert( f );
    HDassert( f->shared );
    cache_ptr = f->shared->cache;
    HDassert( cache_ptr );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );

    HDassert( ce_array_len > 0 );
    HDassert( ce_array_ptr != NULL );

#if H5C_DO_EXTREME_SANITY_CHECKS
        if ( H5C_validate_lru_list(cache_ptr) < 0 ) {

                HDassert(0);
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                            "LRU sanity check failed.\n");
        }
#endif /* H5C_DO_EXTREME_SANITY_CHECKS */

    for ( i = 0; i < ce_array_len; i++ )
    {
        addr = ce_array_ptr[i];

#if H5C_DO_SANITY_CHECKS
        if ( i == 0 ) {

            last_addr = addr;

        } else {

            if ( last_addr == addr ) {

                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                            "Duplicate entry in cleaned list.\n");

            } else if ( last_addr > addr ) {

                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                            "cleaned list not sorted.\n");
            }
        }

#if H5C_DO_EXTREME_SANITY_CHECKS
        if ( H5C_validate_lru_list(cache_ptr) < 0 ) {

                HDassert(0);
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                            "LRU sanity check failed.\n");
        }
#endif /* H5C_DO_EXTREME_SANITY_CHECKS */
#endif /* H5C_DO_SANITY_CHECKS */

        HDassert( H5F_addr_defined(addr) );

        H5C__SEARCH_INDEX(cache_ptr, addr, entry_ptr, FAIL)

        if ( entry_ptr == NULL ) {
#if H5C_DO_SANITY_CHECKS
	    HDfprintf(stdout,
                  "H5C_mark_entries_as_clean: entry[%d] = %ld not in cache.\n",
                      (int)i,
                      (long)addr);
#endif /* H5C_DO_SANITY_CHECKS */
            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                        "Listed entry not in cache?!?!?.")

        } else if ( ! entry_ptr->is_dirty ) {

#if H5C_DO_SANITY_CHECKS
	    HDfprintf(stdout,
                      "H5C_mark_entries_as_clean: entry %ld is not dirty!?!\n",
                      (long)addr);
#endif /* H5C_DO_SANITY_CHECKS */
            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                        "Listed entry not dirty?!?!?.")
#if 0 /* original code */
        } else if ( entry_ptr->is_protected ) {

            entry_ptr->clear_on_unprotect = TRUE;

        } else {

            if ( H5C_flush_single_entry(f,
                                        primary_dxpl_id,
                                        secondary_dxpl_id,
                                        entry_ptr->type,
                                        addr,
                                        H5C__FLUSH_CLEAR_ONLY_FLAG,
                                        &first_flush,
                                        TRUE) < 0 ) {

                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Can't clear entry.")
            }
        }
#else /* modified code */
        } else {
            /* Mark the entry to be cleared on unprotect.  We will
             * scan the LRU list shortly, and clear all those entries
             * not currently protected.
             */
            entry_ptr->clear_on_unprotect = TRUE;
#if H5C_DO_SANITY_CHECKS
	    if ( entry_ptr->is_protected ) {

		protected_entries_marked++;

	    } else if ( entry_ptr->is_pinned ) {

		pinned_entries_marked++;

	    } else {

		other_entries_marked++;
	    }
#endif /* H5C_DO_SANITY_CHECKS */
        }
#endif /* end modified code */
    }
#if 1 /* modified code */
    /* Scan through the LRU list from back to front, and flush the
     * entries whose clear_on_unprotect flags are set.  Observe that
     * any protected entries will not be on the LRU, and therefore
     * will not be flushed at this time.
     */

    entries_cleared = 0;
    entries_examined = 0;
    initial_list_len = cache_ptr->LRU_list_len;
    entry_ptr = cache_ptr->LRU_tail_ptr;

    while ( ( entry_ptr != NULL ) &&
            ( entries_examined <= initial_list_len ) &&
            ( entries_cleared < ce_array_len ) )
    {
        if ( entry_ptr->clear_on_unprotect ) {

            entry_ptr->clear_on_unprotect = FALSE;
            clear_ptr = entry_ptr;
            entry_ptr = entry_ptr->prev;
            entries_cleared++;

            if ( H5C_flush_single_entry(f,
                                        primary_dxpl_id,
                                        secondary_dxpl_id,
                                        clear_ptr->type,
                                        clear_ptr->addr,
                                        H5C__FLUSH_CLEAR_ONLY_FLAG,
                                        &first_flush,
                                        TRUE) < 0 ) {

                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Can't clear entry.")
            }
        } else {

            entry_ptr = entry_ptr->prev;
        }
        entries_examined++;
    }

#if H5C_DO_SANITY_CHECKS
    HDassert( entries_cleared == other_entries_marked );
#endif /* H5C_DO_SANITY_CHECKS */

    /* It is also possible that some of the cleared entries are on the
     * pinned list.  Must scan that also.
     */

    entry_ptr = cache_ptr->pel_head_ptr;

    while ( entry_ptr != NULL )
    {
        if ( entry_ptr->clear_on_unprotect ) {

            entry_ptr->clear_on_unprotect = FALSE;
            clear_ptr = entry_ptr;
            entry_ptr = entry_ptr->next;
            entries_cleared++;

            if ( H5C_flush_single_entry(f,
                                        primary_dxpl_id,
                                        secondary_dxpl_id,
                                        clear_ptr->type,
                                        clear_ptr->addr,
                                        H5C__FLUSH_CLEAR_ONLY_FLAG,
                                        &first_flush,
                                        TRUE) < 0 ) {

                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Can't clear entry.")
            }
        } else {

            entry_ptr = entry_ptr->next;
        }
    }

#if H5C_DO_SANITY_CHECKS
    HDassert( entries_cleared == pinned_entries_marked + other_entries_marked );
    HDassert( entries_cleared + protected_entries_marked == ce_array_len );
#endif /* H5C_DO_SANITY_CHECKS */

    HDassert( ( entries_cleared == ce_array_len ) ||
              ( (ce_array_len - entries_cleared) <= cache_ptr->pl_len ) );

#if H5C_DO_SANITY_CHECKS
    i = 0;
    entry_ptr = cache_ptr->pl_head_ptr;
    while ( entry_ptr != NULL )
    {
        if ( entry_ptr->clear_on_unprotect ) {

            i++;
        }
        entry_ptr = entry_ptr->next;
    }
    HDassert( (entries_cleared + i) == ce_array_len );
#endif /* H5C_DO_SANITY_CHECKS */
#endif /* modified code */

done:

#if H5C_DO_EXTREME_SANITY_CHECKS
        if ( H5C_validate_lru_list(cache_ptr) < 0 ) {

                HDassert(0);
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                            "LRU sanity check failed.\n");
        }
#endif /* H5C_DO_EXTREME_SANITY_CHECKS */

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C_mark_entries_as_clean() */
#endif /* H5_HAVE_PARALLEL */


/*-------------------------------------------------------------------------
 * Function:    H5C_mark_entry_dirty
 *
 * Purpose:	Mark a pinned or protected entry as dirty.  The target entry
 * 		MUST be either pinned or protected, and MAY be both.
 *
 * 		In the protected case, this call is the functional
 * 		equivalent of setting the H5C__DIRTIED_FLAG on an unprotect
 * 		call.
 *
 * 		In the pinned but not protected case, if the entry is not
 * 		already dirty, the function places function marks the entry
 * 		dirty and places it on the skip list.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              5/15/06
 *
 * 		JRM -- 11/5/08
 * 		Added call to H5C__UPDATE_INDEX_FOR_ENTRY_DIRTY() to
 * 		update the new clean_index_size and dirty_index_size
 * 		fields of H5C_t in the case that the entry was clean
 * 		prior to this call, and is pinned and not protected.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_mark_entry_dirty(void *thing)
{
    H5C_t *             cache_ptr;
    H5C_cache_entry_t * entry_ptr = (H5C_cache_entry_t *)thing;
    herr_t              ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(entry_ptr);
    HDassert(H5F_addr_defined(entry_ptr->addr));
    cache_ptr = entry_ptr->cache_ptr;
    HDassert(cache_ptr);
    HDassert(cache_ptr->magic == H5C__H5C_T_MAGIC);

    if ( entry_ptr->is_protected ) {
	HDassert( ! ((entry_ptr)->is_read_only) );

        /* set the dirtied flag */
        entry_ptr->dirtied = TRUE;

    } else if ( entry_ptr->is_pinned ) {
        hbool_t		was_pinned_unprotected_and_clean;

	was_pinned_unprotected_and_clean = ! ( entry_ptr->is_dirty );

        /* mark the entry as dirty if it isn't already */
        entry_ptr->is_dirty = TRUE;

	if ( was_pinned_unprotected_and_clean ) {

	    H5C__UPDATE_INDEX_FOR_ENTRY_DIRTY(cache_ptr, entry_ptr);
	}

        if ( ! (entry_ptr->in_slist) ) {

            H5C__INSERT_ENTRY_IN_SLIST(cache_ptr, entry_ptr, FAIL)
        }

        H5C__UPDATE_STATS_FOR_DIRTY_PIN(cache_ptr, entry_ptr)

    } else {

        HGOTO_ERROR(H5E_CACHE, H5E_CANTMARKDIRTY, FAIL, \
                    "Entry is neither pinned nor protected??")
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5C_mark_entry_dirty() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5C_move_entry
 *
 * Purpose:     Use this function to notify the cache that an entry's
 *              file address changed.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              6/2/04
 *
 *              JRM -- 11/5/08
 *              On review this function looks like no change is needed to
 *              support the new clean_index_size and dirty_index_size
 *              fields of H5C_t.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_move_entry(H5C_t *	     cache_ptr,
                 const H5C_class_t * type,
                 haddr_t 	     old_addr,
	         haddr_t 	     new_addr)
{
#if H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS
    hbool_t			was_dirty;
#endif /* H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS */
    H5C_cache_entry_t *	entry_ptr = NULL;
    H5C_cache_entry_t *	test_entry_ptr = NULL;
#if H5C_DO_SANITY_CHECKS
    hbool_t			removed_entry_from_slist = FALSE;
#endif /* H5C_DO_SANITY_CHECKS */
    herr_t			ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert( cache_ptr );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );
    HDassert( type );
    HDassert( H5F_addr_defined(old_addr) );
    HDassert( H5F_addr_defined(new_addr) );
    HDassert( H5F_addr_ne(old_addr, new_addr) );

#if H5C_DO_EXTREME_SANITY_CHECKS
        if ( H5C_validate_lru_list(cache_ptr) < 0 ) {

                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                            "LRU sanity check failed.\n");
        }
#endif /* H5C_DO_EXTREME_SANITY_CHECKS */

    H5C__SEARCH_INDEX(cache_ptr, old_addr, entry_ptr, FAIL)

    if ( ( entry_ptr == NULL ) || ( entry_ptr->type != type ) ) {

        /* the old item doesn't exist in the cache, so we are done. */
        HGOTO_DONE(SUCCEED)
    }

    HDassert( entry_ptr->addr == old_addr );
    HDassert( entry_ptr->type == type );

    if ( entry_ptr->is_protected ) {

        HGOTO_ERROR(H5E_CACHE, H5E_CANTMOVE, FAIL, \
		    "Target entry is protected.")
    }

    H5C__SEARCH_INDEX(cache_ptr, new_addr, test_entry_ptr, FAIL)

    if ( test_entry_ptr != NULL ) { /* we are hosed */

        if ( test_entry_ptr->type == type ) {

            HGOTO_ERROR(H5E_CACHE, H5E_CANTMOVE, FAIL, \
                        "Target already moved & reinserted???.")

        } else {

            HGOTO_ERROR(H5E_CACHE, H5E_CANTMOVE, FAIL, \
                        "New address already in use?.")

        }
    }

    /* If we get this far we have work to do.  Remove *entry_ptr from
     * the hash table (and skip list if necessary), change its address to the
     * new address, mark it as dirty (if it isn't already) and then re-insert.
     *
     * Update the replacement policy for a hit to avoid an eviction before
     * the moved entry is touched.  Update stats for a move.
     *
     * Note that we do not check the size of the cache, or evict anything.
     * Since this is a simple re-name, cache size should be unaffected.
     *
     * Check to see if the target entry is in the process of being destroyed
     * before we delete from the index, etc.  If it is, all we do is
     * change the addr.  If the entry is only in the process of being flushed,
     * don't mark it as dirty either, lest we confuse the flush call back.
     */

    if ( ! ( entry_ptr->destroy_in_progress ) ) {

        H5C__DELETE_FROM_INDEX(cache_ptr, entry_ptr)

        if ( entry_ptr->in_slist ) {

            HDassert( cache_ptr->slist_ptr );

            H5C__REMOVE_ENTRY_FROM_SLIST(cache_ptr, entry_ptr)

#if H5C_DO_SANITY_CHECKS

            removed_entry_from_slist = TRUE;

#endif /* H5C_DO_SANITY_CHECKS */
        }
    }

    entry_ptr->addr = new_addr;

    if ( ! ( entry_ptr->destroy_in_progress ) ) {

#if H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS
        was_dirty = entry_ptr->is_dirty;
#endif /* H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS */

	if ( ! ( entry_ptr->flush_in_progress ) ) {

            entry_ptr->is_dirty = TRUE;
	}

        H5C__INSERT_IN_INDEX(cache_ptr, entry_ptr, FAIL)

	if ( ! ( entry_ptr->flush_in_progress ) ) {

            H5C__INSERT_ENTRY_IN_SLIST(cache_ptr, entry_ptr, FAIL)

#if H5C_DO_SANITY_CHECKS

            if ( removed_entry_from_slist ) {

		/* we just removed the entry from the slist.  Thus we
		 * must touch up cache_ptr->slist_len_increase and
		 * cache_ptr->slist_size_increase to keep from skewing
		 * the sanity checks.
		 */
		HDassert( cache_ptr->slist_len_increase > 1 );
		HDassert( cache_ptr->slist_size_increase > entry_ptr->size );

		cache_ptr->slist_len_increase -= 1;
		cache_ptr->slist_size_increase -= entry_ptr->size;
	    }

#endif /* H5C_DO_SANITY_CHECKS */

            H5C__UPDATE_RP_FOR_MOVE(cache_ptr, entry_ptr, was_dirty, FAIL)
	}
    }

    H5C__UPDATE_STATS_FOR_MOVE(cache_ptr, entry_ptr)

done:

#if H5C_DO_EXTREME_SANITY_CHECKS
        if ( H5C_validate_lru_list(cache_ptr) < 0 ) {

                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                            "LRU sanity check failed.\n");
        }
#endif /* H5C_DO_EXTREME_SANITY_CHECKS */

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C_move_entry() */


/*-------------------------------------------------------------------------
 * Function:    H5C_resize_entry
 *
 * Purpose:	Resize a pinned or protected entry.
 *
 * 		Resizing an entry dirties it, so if the entry is not
 * 		already dirty, the function places the entry on the
 * 		skip list.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              7/5/06
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_resize_entry(void *thing, size_t new_size)
{
    H5C_t             * cache_ptr;
    H5C_cache_entry_t * entry_ptr = (H5C_cache_entry_t *)thing;
    herr_t              ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(entry_ptr);
    HDassert(H5F_addr_defined(entry_ptr->addr));
    cache_ptr = entry_ptr->cache_ptr;
    HDassert(cache_ptr);
    HDassert(cache_ptr->magic == H5C__H5C_T_MAGIC);

    /* Check for usage errors */
    if(new_size <= 0)
        HGOTO_ERROR(H5E_CACHE, H5E_BADVALUE, FAIL, "New size is non-positive.")
    if(!(entry_ptr->is_pinned || entry_ptr->is_protected))
        HGOTO_ERROR(H5E_CACHE, H5E_BADTYPE, FAIL, "Entry isn't pinned or protected??")

    /* update for change in entry size if necessary */
    if ( entry_ptr->size != new_size ) {
        hbool_t		was_clean;

        /* make note of whether the entry was clean to begin with */
        was_clean = ! ( entry_ptr->is_dirty );

        /* mark the entry as dirty if it isn't already */
        entry_ptr->is_dirty = TRUE;

        /* do a flash cache size increase if appropriate */
        if ( cache_ptr->flash_size_increase_possible ) {

            if ( new_size > entry_ptr->size ) {
                size_t             	size_increase;

                size_increase = new_size - entry_ptr->size;

                if(size_increase >= cache_ptr->flash_size_increase_threshold) {
                    if(H5C__flash_increase_cache_size(cache_ptr, entry_ptr->size, new_size) < 0)
                        HGOTO_ERROR(H5E_CACHE, H5E_CANTRESIZE, FAIL, "flash cache increase failed")
                }
            }
        }

        /* update the pinned and/or protected entry list */
        if(entry_ptr->is_pinned) {
            H5C__DLL_UPDATE_FOR_SIZE_CHANGE((cache_ptr->pel_len), \
                                            (cache_ptr->pel_size), \
                                            (entry_ptr->size), (new_size))
        } /* end if */
        if(entry_ptr->is_protected) {
            H5C__DLL_UPDATE_FOR_SIZE_CHANGE((cache_ptr->pl_len), \
                                            (cache_ptr->pl_size), \
                                            (entry_ptr->size), (new_size))
        } /* end if */

        /* update the hash table */
	H5C__UPDATE_INDEX_FOR_SIZE_CHANGE((cache_ptr), (entry_ptr->size),\
                                          (new_size), (entry_ptr), (was_clean));

        /* if the entry is in the skip list, update that too */
        if ( entry_ptr->in_slist ) {
	    H5C__UPDATE_SLIST_FOR_SIZE_CHANGE((cache_ptr), (entry_ptr->size),\
                                              (new_size));
        } /* end if */

        /* update statistics just before changing the entry size */
	H5C__UPDATE_STATS_FOR_ENTRY_SIZE_CHANGE((cache_ptr), (entry_ptr), \
                                                (new_size));

	/* finally, update the entry size proper */
	entry_ptr->size = new_size;

        if(!entry_ptr->in_slist) {
            H5C__INSERT_ENTRY_IN_SLIST(cache_ptr, entry_ptr, FAIL)
        } /* end if */

        if(entry_ptr->is_pinned) {
            H5C__UPDATE_STATS_FOR_DIRTY_PIN(cache_ptr, entry_ptr)
        } /* end if */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5C_resize_entry() */


/*-------------------------------------------------------------------------
 * Function:    H5C_pin_protected_entry()
 *
 * Purpose:	Pin a protected cache entry.  The entry must be protected
 * 		at the time of call, and must be unpinned.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              4/26/06
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_pin_protected_entry(void *thing)
{
    H5C_t             * cache_ptr;
    H5C_cache_entry_t * entry_ptr = (H5C_cache_entry_t *)thing; /* Pointer to entry to pin */
    herr_t              ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(entry_ptr);
    HDassert(H5F_addr_defined(entry_ptr->addr));
    cache_ptr = entry_ptr->cache_ptr;
    HDassert(cache_ptr);
    HDassert(cache_ptr->magic == H5C__H5C_T_MAGIC);

    if ( ! ( entry_ptr->is_protected ) ) {

        HGOTO_ERROR(H5E_CACHE, H5E_CANTPIN, FAIL, "Entry isn't protected")
    }

    if ( entry_ptr->is_pinned ) {

        HGOTO_ERROR(H5E_CACHE, H5E_CANTPIN, FAIL, "Entry is already pinned")
    }

    entry_ptr->is_pinned = TRUE;

    H5C__UPDATE_STATS_FOR_PIN(cache_ptr, entry_ptr)

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C_pin_protected_entry() */


/*-------------------------------------------------------------------------
 * Function:    H5C_protect
 *
 * Purpose:     If the target entry is not in the cache, load it.  If
 *		necessary, attempt to evict one or more entries to keep
 *		the cache within its maximum size.
 *
 *		Mark the target entry as protected, and return its address
 *		to the caller.  The caller must call H5C_unprotect() when
 *		finished with the entry.
 *
 *		While it is protected, the entry may not be either evicted
 *		or flushed -- nor may it be accessed by another call to
 *		H5C_protect.  Any attempt to do so will result in a failure.
 *
 *		The primary_dxpl_id and secondary_dxpl_id parameters
 *		specify the dxpl_ids used on the first write occasioned
 *		by the insertion (primary_dxpl_id), and on all subsequent
 *		writes (secondary_dxpl_id).  This is useful in the
 *		metadata cache, but may not be needed elsewhere.  If so,
 *		just use the same dxpl_id for both parameters.
 *
 *		All reads are performed with the primary_dxpl_id.
 *
 *		Similarly, the primary_dxpl_id is passed to the
 *		check_write_permitted function if it is called.
 *
 * Return:      Success:        Ptr to the desired entry
 *              Failure:        NULL
 *
 * Programmer:  John Mainzer -  6/2/04
 *
 * 		JRM -- 11/13/08
 * 		Modified function to call H5C_make_space_in_cache() when
 * 		the min_clean_size is violated, not just when there isn't
 * 		enough space for and entry that has just been loaded.
 *
 *              The purpose of this modification is to avoid "metadata
 *              blizzards" in the write only case.  In such instances,
 *              the cache was allowed to fill with dirty metadata.  When
 *              we finally needed to evict an entry to make space, we had
 *              to flush out a whole cache full of metadata -- which has
 *              interesting performance effects.  We hope to avoid (or
 *              perhaps more accurately hide) this effect by maintaining
 *              the min_clean_size, which should force us to start flushing
 *              entries long before we actually have to evict something
 *              to make space.
 *
 *-------------------------------------------------------------------------
 */
void *
H5C_protect(H5F_t *		f,
            hid_t	        primary_dxpl_id,
            hid_t	        secondary_dxpl_id,
            const H5C_class_t * type,
            haddr_t 	        addr,
            void *              udata,
	    unsigned		flags)
{
    H5C_t *		cache_ptr;
    hbool_t		hit;
    hbool_t		first_flush;
    hbool_t		have_write_permitted = FALSE;
    hbool_t		read_only = FALSE;
    hbool_t		write_permitted;
    herr_t		result;
    size_t		empty_space;
    void *		thing;
    H5C_cache_entry_t *	entry_ptr;
    void *		ret_value;      /* Return value */

    FUNC_ENTER_NOAPI(NULL)

    /* check args */
    HDassert( f );
    HDassert( f->shared );

    cache_ptr = f->shared->cache;

    HDassert( cache_ptr );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );
    HDassert( type );
    HDassert( type->flush );
    HDassert( type->load );
    HDassert( H5F_addr_defined(addr) );

#if H5C_DO_EXTREME_SANITY_CHECKS
    if ( H5C_validate_lru_list(cache_ptr) < 0 ) {

	HDassert(0);
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, NULL, \
                    "LRU sanity check failed.\n");
    }
#endif /* H5C_DO_EXTREME_SANITY_CHECKS */

    if ( (flags & H5C__READ_ONLY_FLAG) != 0 )
    {
	read_only = TRUE;
    }

    /* first check to see if the target is in cache */
    H5C__SEARCH_INDEX(cache_ptr, addr, entry_ptr, NULL)

    if ( entry_ptr != NULL ) {

        /* Check for trying to load the wrong type of entry from an address */
        if(entry_ptr->type != type)
            HGOTO_ERROR(H5E_CACHE, H5E_BADTYPE, NULL, "incorrect cache entry type")

        hit = TRUE;
	thing = (void *)entry_ptr;

    } else {

        /* must try to load the entry from disk. */

        hit = FALSE;

        thing = H5C_load_entry(f, primary_dxpl_id, type, addr, udata);

        if ( thing == NULL ) {

            HGOTO_ERROR(H5E_CACHE, H5E_CANTLOAD, NULL, "can't load entry")
        }

        entry_ptr = (H5C_cache_entry_t *)thing;

        /* If the entry is very large, and we are configured to allow it,
         * we may wish to perform a flash cache size increase.
         */
        if ( ( cache_ptr->flash_size_increase_possible ) &&
             ( entry_ptr->size > cache_ptr->flash_size_increase_threshold ) ) {

            result = H5C__flash_increase_cache_size(cache_ptr, 0,
                                                     entry_ptr->size);

            if ( result < 0 ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTPROTECT, NULL, \
                            "H5C__flash_increase_cache_size failed.")
             }
        }

        if ( cache_ptr->index_size >= cache_ptr->max_cache_size ) {

           empty_space = 0;

        } else {

           empty_space = cache_ptr->max_cache_size - cache_ptr->index_size;

        }

	/* try to free up if necceary and if evictions are permitted.  Note
	 * that if evictions are enabled, we will call H5C_make_space_in_cache()
	 * regardless if the min_free_space requirement is not met.
	 */

        if ( ( cache_ptr->evictions_enabled ) &&
             ( ( (cache_ptr->index_size + entry_ptr->size) >
	         cache_ptr->max_cache_size)
	       ||
	       ( ( empty_space + cache_ptr->clean_index_size ) <
	         cache_ptr->min_clean_size )
	     )
           ) {

            size_t space_needed;

	    if ( empty_space <= entry_ptr->size ) {

                cache_ptr->cache_full = TRUE;

	    }

            if ( cache_ptr->check_write_permitted != NULL ) {

                result = (cache_ptr->check_write_permitted)(f,
                                                            primary_dxpl_id,
                                                            &write_permitted);

                if ( result < 0 ) {

                    HGOTO_ERROR(H5E_CACHE, H5E_CANTPROTECT, NULL, \
                               "Can't get write_permitted 1")

                } else {

                    have_write_permitted = TRUE;

                    first_flush = TRUE;
                }
            } else {

                write_permitted = cache_ptr->write_permitted;

                have_write_permitted = TRUE;

                first_flush = TRUE;
            }

            HDassert( entry_ptr->size <= H5C_MAX_ENTRY_SIZE );

            space_needed = entry_ptr->size;

            if ( space_needed > cache_ptr->max_cache_size ) {

                space_needed = cache_ptr->max_cache_size;
            }

            /* Note that space_needed is just the amount of space that
             * needed to insert the new entry without exceeding the cache
             * size limit.  The subsequent call to H5C_make_space_in_cache()
             * may evict the entries required to free more or less space
             * depending on conditions.  It MAY be less if the cache is
             * currently undersized, or more if the cache is oversized.
             *
             * The cache can exceed its maximum size limit via the following
             * mechanisms:
             *
             * First, it is possible for the cache to grow without
             * bound as long as entries are protected and not unprotected.
             *
             * Second, when writes are not permitted it is also possible
             * for the cache to grow without bound.
	     *
	     * Third, the user may choose to disable evictions -- causing
	     * the cache to grow without bound until evictions are
	     * re-enabled.
             *
             * Finally, we usually don't check to see if the cache is
             * oversized at the end of an unprotect.  As a result, it is
             * possible to have a vastly oversized cache with no protected
             * entries as long as all the protects preceed the unprotects.
             *
             * Since items 1, 2, and 3 are not changing any time soon, I
             * see no point in worrying about the fourth.
             */

            result = H5C_make_space_in_cache(f, primary_dxpl_id,
                                             secondary_dxpl_id,
                                             space_needed, write_permitted,
                                             &first_flush);

            if ( result < 0 ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTPROTECT, NULL, \
                            "H5C_make_space_in_cache failed 1.")
            }
        }

        /* Insert the entry in the hash table.  It can't be dirty yet, so
         * we don't even check to see if it should go in the skip list.
         *
         * This is no longer true -- due to a bug fix, we may modify
         * data on load to repair a file.
         */
        H5C__INSERT_IN_INDEX(cache_ptr, entry_ptr, NULL)

        if ( ( entry_ptr->is_dirty ) && ( ! (entry_ptr->in_slist) ) ) {

            H5C__INSERT_ENTRY_IN_SLIST(cache_ptr, entry_ptr, NULL)
        }

        /* insert the entry in the data structures used by the replacement
         * policy.  We are just going to take it out again when we update
         * the replacement policy for a protect, but this simplifies the
         * code.  If we do this often enough, we may want to optimize this.
         */
        H5C__UPDATE_RP_FOR_INSERTION(cache_ptr, entry_ptr, NULL)
    }

    HDassert( entry_ptr->addr == addr );
    HDassert( entry_ptr->type == type );

    if ( entry_ptr->is_protected ) {

	if ( ( read_only ) && ( entry_ptr->is_read_only ) ) {

	    HDassert( entry_ptr->ro_ref_count > 0 );

	    (entry_ptr->ro_ref_count)++;

	} else {

            HGOTO_ERROR(H5E_CACHE, H5E_CANTPROTECT, NULL, \
                        "Target already protected & not read only?!?.")
	}
    } else {

    	H5C__UPDATE_RP_FOR_PROTECT(cache_ptr, entry_ptr, NULL)

    	entry_ptr->is_protected = TRUE;

	if ( read_only ) {

	    entry_ptr->is_read_only = TRUE;
	    entry_ptr->ro_ref_count = 1;
	}

    	entry_ptr->dirtied = FALSE;
    }

    H5C__UPDATE_CACHE_HIT_RATE_STATS(cache_ptr, hit)

    H5C__UPDATE_STATS_FOR_PROTECT(cache_ptr, entry_ptr, hit)

    ret_value = thing;

    if ( ( cache_ptr->evictions_enabled ) &&
         ( ( cache_ptr->size_decreased ) ||
           ( ( cache_ptr->resize_enabled ) &&
             ( cache_ptr->cache_accesses >=
               (cache_ptr->resize_ctl).epoch_length ) ) ) ) {

        if ( ! have_write_permitted ) {

            if ( cache_ptr->check_write_permitted != NULL ) {

                result = (cache_ptr->check_write_permitted)(f,
                                                            primary_dxpl_id,
                                                            &write_permitted);

                if ( result < 0 ) {

                    HGOTO_ERROR(H5E_CACHE, H5E_CANTPROTECT, NULL, \
                               "Can't get write_permitted 2")

                } else {

                    have_write_permitted = TRUE;

                    first_flush = TRUE;
                }
            } else {

                write_permitted = cache_ptr->write_permitted;

                have_write_permitted = TRUE;

                first_flush = TRUE;
            }
        }

        if ( ( cache_ptr->resize_enabled ) &&
             ( cache_ptr->cache_accesses >=
               (cache_ptr->resize_ctl).epoch_length ) ) {

            result = H5C__auto_adjust_cache_size(f,
                                                 primary_dxpl_id,
                                                 secondary_dxpl_id,
                                                 write_permitted,
                                                 &first_flush);
            if ( result != SUCCEED ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTPROTECT, NULL, \
                            "Cache auto-resize failed.")
            }
        }

        if ( cache_ptr->size_decreased  ) {

            cache_ptr->size_decreased = FALSE;

            /* check to see if the cache is now oversized due to the cache
             * size reduction.  If it is, try to evict enough entries to
             * bring the cache size down to the current maximum cache size.
	     *
	     * Also, if the min_clean_size requirement is not met, we
	     * should also call H5C_make_space_in_cache() to bring us
	     * into complience.
             */

            if ( cache_ptr->index_size >= cache_ptr->max_cache_size ) {

               empty_space = 0;

            } else {

               empty_space = cache_ptr->max_cache_size - cache_ptr->index_size;

            }

            if ( ( cache_ptr->index_size > cache_ptr->max_cache_size )
	         ||
	         ( ( empty_space + cache_ptr->clean_index_size ) <
	           cache_ptr->min_clean_size) ) {

		if ( cache_ptr->index_size > cache_ptr->max_cache_size ) {

                    cache_ptr->cache_full = TRUE;
		}

                result = H5C_make_space_in_cache(f, primary_dxpl_id,
                                                 secondary_dxpl_id,
                                                 (size_t)0, write_permitted,
                                                 &first_flush);

                if ( result < 0 ) {

                    HGOTO_ERROR(H5E_CACHE, H5E_CANTPROTECT, NULL, \
                                "H5C_make_space_in_cache failed 2.")
                }
            }
        }
    }

done:

#if H5C_DO_EXTREME_SANITY_CHECKS
        if ( H5C_validate_lru_list(cache_ptr) < 0 ) {

                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, NULL, \
                            "LRU sanity check failed.\n");
        }
#endif /* H5C_DO_EXTREME_SANITY_CHECKS */

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C_protect() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5C_reset_cache_hit_rate_stats()
 *
 * Purpose:     Reset the cache hit rate computation fields.
 *
 * Return:      SUCCEED on success, and FAIL on failure.
 *
 * Programmer:  John Mainzer, 10/5/04
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_reset_cache_hit_rate_stats(H5C_t * cache_ptr)
{
    herr_t	ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    if ( ( cache_ptr == NULL ) || ( cache_ptr->magic != H5C__H5C_T_MAGIC ) ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Bad cache_ptr on entry.")
    }

    cache_ptr->cache_hits		= 0;
    cache_ptr->cache_accesses		= 0;

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C_reset_cache_hit_rate_stats() */


/*-------------------------------------------------------------------------
 * Function:    H5C_set_cache_auto_resize_config
 *
 * Purpose:	Set the cache automatic resize configuration to the
 *		provided values if they are in range, and fail if they
 *		are not.
 *
 *		If the new configuration enables automatic cache resizing,
 *		coerce the cache max size and min clean size into agreement
 *		with the new policy and re-set the full cache hit rate
 *		stats.
 *
 * Return:      SUCCEED on success, and FAIL on failure.
 *
 * Programmer:  John Mainzer
 *		10/8/04
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_set_cache_auto_resize_config(H5C_t *cache_ptr,
                                 H5C_auto_size_ctl_t *config_ptr)
{
    herr_t	result;
    size_t      new_max_cache_size;
    size_t      new_min_clean_size;
    herr_t	ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    if ( ( cache_ptr == NULL ) || ( cache_ptr->magic != H5C__H5C_T_MAGIC ) ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Bad cache_ptr on entry.")
    }

    if ( config_ptr == NULL ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "NULL config_ptr on entry.")
    }

    if ( config_ptr->version != H5C__CURR_AUTO_SIZE_CTL_VER ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Unknown config version.")
    }

    /* check general configuration section of the config: */
    if ( SUCCEED != H5C_validate_resize_config(config_ptr,
                                   H5C_RESIZE_CFG__VALIDATE_GENERAL) ) {

        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, \
                    "error in general configuration fields of new config.")
    }

    /* check size increase control fields of the config: */
    if ( SUCCEED != H5C_validate_resize_config(config_ptr,
                                   H5C_RESIZE_CFG__VALIDATE_INCREMENT) ) {

        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, \
                    "error in the size increase control fields of new config.")
    }

    /* check size decrease control fields of the config: */
    if ( SUCCEED != H5C_validate_resize_config(config_ptr,
                                   H5C_RESIZE_CFG__VALIDATE_DECREMENT) ) {

        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, \
                    "error in the size decrease control fields of new config.")
    }

    /* check for conflicts between size increase and size decrease controls: */
    if ( SUCCEED != H5C_validate_resize_config(config_ptr,
                                   H5C_RESIZE_CFG__VALIDATE_INTERACTIONS) ) {

        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, \
                    "conflicting threshold fields in new config.")
    }

    /* will set the increase possible fields to FALSE later if needed */
    cache_ptr->size_increase_possible       = TRUE;
    cache_ptr->flash_size_increase_possible = TRUE;
    cache_ptr->size_decrease_possible       = TRUE;

    switch ( config_ptr->incr_mode )
    {
        case H5C_incr__off:
            cache_ptr->size_increase_possible = FALSE;
            break;

        case H5C_incr__threshold:
            if ( ( config_ptr->lower_hr_threshold <= 0.0 ) ||
                 ( config_ptr->increment <= 1.0 ) ||
                 ( ( config_ptr->apply_max_increment ) &&
                   ( config_ptr->max_increment <= 0 ) ) ) {

                 cache_ptr->size_increase_possible = FALSE;
            }
            break;

        default: /* should be unreachable */
            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Unknown incr_mode?!?!?.")
    }

    /* logically, this is were configuration for flash cache size increases
     * should go.  However, this configuration depends on max_cache_size, so
     * we wait until the end of the function, when this field is set.
     */

    switch ( config_ptr->decr_mode )
    {
        case H5C_decr__off:
            cache_ptr->size_decrease_possible = FALSE;
            break;

        case H5C_decr__threshold:
            if ( ( config_ptr->upper_hr_threshold >= 1.0 ) ||
                 ( config_ptr->decrement >= 1.0 ) ||
                 ( ( config_ptr->apply_max_decrement ) &&
                   ( config_ptr->max_decrement <= 0 ) ) ) {

                cache_ptr->size_decrease_possible = FALSE;
            }
            break;

        case H5C_decr__age_out:
            if ( ( ( config_ptr->apply_empty_reserve ) &&
                   ( config_ptr->empty_reserve >= 1.0 ) ) ||
                 ( ( config_ptr->apply_max_decrement ) &&
                   ( config_ptr->max_decrement <= 0 ) ) ) {

                cache_ptr->size_decrease_possible = FALSE;
            }
            break;

        case H5C_decr__age_out_with_threshold:
            if ( ( ( config_ptr->apply_empty_reserve ) &&
                   ( config_ptr->empty_reserve >= 1.0 ) ) ||
                 ( ( config_ptr->apply_max_decrement ) &&
                   ( config_ptr->max_decrement <= 0 ) ) ||
                 ( config_ptr->upper_hr_threshold >= 1.0 ) ) {

                cache_ptr->size_decrease_possible = FALSE;
            }
            break;

        default: /* should be unreachable */
            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Unknown decr_mode?!?!?.")
    }

    if ( config_ptr->max_size == config_ptr->min_size ) {

        cache_ptr->size_increase_possible = FALSE;
	cache_ptr->flash_size_increase_possible = FALSE;
        cache_ptr->size_decrease_possible = FALSE;
    }

    /* flash_size_increase_possible is intentionally omitted from the
     * following:
     */
    cache_ptr->resize_enabled = cache_ptr->size_increase_possible ||
                                cache_ptr->size_decrease_possible;

    cache_ptr->resize_ctl = *config_ptr;

    /* Resize the cache to the supplied initial value if requested, or as
     * necessary to force it within the bounds of the current automatic
     * cache resizing configuration.
     *
     * Note that the min_clean_fraction may have changed, so we
     * go through the exercise even if the current size is within
     * range and an initial size has not been provided.
     */
    if ( (cache_ptr->resize_ctl).set_initial_size ) {

        new_max_cache_size = (cache_ptr->resize_ctl).initial_size;
    }
    else if ( cache_ptr->max_cache_size > (cache_ptr->resize_ctl).max_size ) {

        new_max_cache_size = (cache_ptr->resize_ctl).max_size;
    }
    else if ( cache_ptr->max_cache_size < (cache_ptr->resize_ctl).min_size ) {

        new_max_cache_size = (cache_ptr->resize_ctl).min_size;

    } else {

        new_max_cache_size = cache_ptr->max_cache_size;
    }

    new_min_clean_size = (size_t)
                         ((double)new_max_cache_size *
                          ((cache_ptr->resize_ctl).min_clean_fraction));


    /* since new_min_clean_size is of type size_t, we have
     *
     * 	( 0 <= new_min_clean_size )
     *
     * by definition.
     */
    HDassert( new_min_clean_size <= new_max_cache_size );
    HDassert( (cache_ptr->resize_ctl).min_size <= new_max_cache_size );
    HDassert( new_max_cache_size <= (cache_ptr->resize_ctl).max_size );

    if ( new_max_cache_size < cache_ptr->max_cache_size ) {

        cache_ptr->size_decreased = TRUE;
    }

    cache_ptr->max_cache_size = new_max_cache_size;
    cache_ptr->min_clean_size = new_min_clean_size;

    if ( H5C_reset_cache_hit_rate_stats(cache_ptr) != SUCCEED ) {

        /* this should be impossible... */
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                    "H5C_reset_cache_hit_rate_stats failed.")
    }

    /* remove excess epoch markers if any */
    if ( ( config_ptr->decr_mode == H5C_decr__age_out_with_threshold ) ||
         ( config_ptr->decr_mode == H5C_decr__age_out ) ) {

        if ( cache_ptr->epoch_markers_active >
             (cache_ptr->resize_ctl).epochs_before_eviction ) {

            result =
                H5C__autoadjust__ageout__remove_excess_markers(cache_ptr);

            if ( result != SUCCEED ) {

                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                            "can't remove excess epoch markers.")
            }
        }
    } else if ( cache_ptr->epoch_markers_active > 0 ) {

        result = H5C__autoadjust__ageout__remove_all_markers(cache_ptr);

        if ( result != SUCCEED ) {

            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                        "error removing all epoch markers.")
        }
    }

    /* configure flash size increase facility.  We wait until the
     * end of the function, as we need the max_cache_size set before
     * we start to keep things simple.
     *
     * If we haven't already ruled out flash cache size increases above,
     * go ahead and configure it.
     */

    if ( cache_ptr->flash_size_increase_possible ) {

        switch ( config_ptr->flash_incr_mode )
        {
            case H5C_flash_incr__off:
                cache_ptr->flash_size_increase_possible = FALSE;
                break;

            case H5C_flash_incr__add_space:
                cache_ptr->flash_size_increase_possible = TRUE;
                cache_ptr->flash_size_increase_threshold =
                    (size_t)
                    (((double)(cache_ptr->max_cache_size)) *
                     ((cache_ptr->resize_ctl).flash_threshold));
                break;

            default: /* should be unreachable */
                 HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                             "Unknown flash_incr_mode?!?!?.")
                 break;
        }
    }

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C_set_cache_auto_resize_config() */


/*-------------------------------------------------------------------------
 * Function:    H5C_set_evictions_enabled()
 *
 * Purpose:     Set cache_ptr->evictions_enabled to the value of the
 *              evictions enabled parameter.
 *
 * Return:      SUCCEED on success, and FAIL on failure.
 *
 * Programmer:  John Mainzer
 *              7/27/07
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_set_evictions_enabled(H5C_t *cache_ptr,
                          hbool_t evictions_enabled)
{
    herr_t ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    if ( ( cache_ptr == NULL ) || ( cache_ptr->magic != H5C__H5C_T_MAGIC ) ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Bad cache_ptr on entry.")
    }

    if ( ( evictions_enabled != TRUE ) && ( evictions_enabled != FALSE ) ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                    "Bad evictions_enabled on entry.")
    }

    /* There is no fundamental reason why we should not permit
     * evictions to be disabled while automatic resize is enabled.
     * However, I can't think of any good reason why one would
     * want to, and allowing it would greatly complicate testing
     * the feature.  Hence the following:
     */
    if ( ( evictions_enabled != TRUE ) &&
         ( ( cache_ptr->resize_ctl.incr_mode != H5C_incr__off ) ||
	   ( cache_ptr->resize_ctl.decr_mode != H5C_decr__off ) ) ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                    "Can't disable evictions when auto resize enabled.")
    }

    cache_ptr->evictions_enabled = evictions_enabled;

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C_set_evictions_enabled() */


/*-------------------------------------------------------------------------
 * Function:    H5C_set_prefix
 *
 * Purpose:     Set the values of the prefix field of H5C_t.  This
 *		filed is used to label some debugging output.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              1/20/06
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_set_prefix(H5C_t * cache_ptr, char * prefix)
{
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    if ( ( cache_ptr == NULL ) ||
         ( cache_ptr->magic != H5C__H5C_T_MAGIC ) ||
         ( prefix == NULL ) ||
         ( HDstrlen(prefix) >= H5C__PREFIX_LEN ) ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Bad param(s) on entry.")
    }

    HDstrncpy(&(cache_ptr->prefix[0]), prefix, (size_t)(H5C__PREFIX_LEN));

    cache_ptr->prefix[H5C__PREFIX_LEN - 1] = '\0';

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C_set_prefix() */


/*-------------------------------------------------------------------------
 * Function:    H5C_set_trace_file_ptr
 *
 * Purpose:     Set the trace_file_ptr field for the cache.
 *
 *              This field must either be NULL (which turns of trace
 *              file logging), or be a pointer to an open file to which
 *              trace file data is to be written.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              1/20/06
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_set_trace_file_ptr(H5C_t * cache_ptr,
                       FILE * trace_file_ptr)
{
    herr_t		ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* This would normally be an assert, but we need to use an HGOTO_ERROR
     * call to shut up the compiler.
     */
    if ( ( ! cache_ptr ) || ( cache_ptr->magic != H5C__H5C_T_MAGIC ) ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Bad cache_ptr")
    }

    cache_ptr->trace_file_ptr = trace_file_ptr;

done:
    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C_set_trace_file_ptr() */


/*-------------------------------------------------------------------------
 * Function:    H5C_stats
 *
 * Purpose:     Prints statistics about the cache.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              6/2/04
 *
 *		JRM -- 11/13/08
 *		Added code displaying the max_clean_index_size and
 *		max_dirty_index_size.
 *
 *              MAM -- 01/06/09
 *              Added code displaying the calls_to_msic,
 *              total_entries_skipped_in_msic, total_entries_scanned_in_msic,
 *              and max_entries_skipped_in_msic fields.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_stats(H5C_t * cache_ptr,
          const char *  cache_name,
          hbool_t
#if !H5C_COLLECT_CACHE_STATS
          UNUSED
#endif /* H5C_COLLECT_CACHE_STATS */
          display_detailed_stats)
{
    herr_t	ret_value = SUCCEED;   /* Return value */

#if H5C_COLLECT_CACHE_STATS
    int		i;
    int64_t     total_hits = 0;
    int64_t     total_misses = 0;
    int64_t	total_write_protects = 0;
    int64_t	total_read_protects = 0;
    int64_t	max_read_protects = 0;
    int64_t     total_insertions = 0;
    int64_t     total_pinned_insertions = 0;
    int64_t     total_clears = 0;
    int64_t     total_flushes = 0;
    int64_t     total_evictions = 0;
    int64_t     total_moves = 0;
    int64_t     total_entry_flush_moves = 0;
    int64_t     total_cache_flush_moves = 0;
    int64_t	total_size_increases = 0;
    int64_t	total_size_decreases = 0;
    int64_t	total_entry_flush_size_changes = 0;
    int64_t	total_cache_flush_size_changes = 0;
    int64_t	total_pins = 0;
    int64_t	total_unpins = 0;
    int64_t	total_dirty_pins = 0;
    int64_t	total_pinned_flushes = 0;
    int64_t	total_pinned_clears = 0;
    int32_t     aggregate_max_accesses = 0;
    int32_t     aggregate_min_accesses = 1000000;
    int32_t     aggregate_max_clears = 0;
    int32_t     aggregate_max_flushes = 0;
    size_t      aggregate_max_size = 0;
    int32_t	aggregate_max_pins = 0;
    double      hit_rate;
    double	average_successful_search_depth = 0.0;
    double	average_failed_search_depth = 0.0;
    double      average_entries_skipped_per_calls_to_msic = 0.0;
    double      average_entries_scanned_per_calls_to_msic = 0.0;
#endif /* H5C_COLLECT_CACHE_STATS */

    FUNC_ENTER_NOAPI(FAIL)

    /* This would normally be an assert, but we need to use an HGOTO_ERROR
     * call to shut up the compiler.
     */
    if ( ( ! cache_ptr ) ||
         ( cache_ptr->magic != H5C__H5C_T_MAGIC ) ||
         ( !cache_name ) ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Bad cache_ptr or cache_name")
    }

#if H5C_COLLECT_CACHE_STATS

    for ( i = 0; i <= cache_ptr->max_type_id; i++ ) {

        total_hits              += cache_ptr->hits[i];
        total_misses            += cache_ptr->misses[i];
	total_write_protects	+= cache_ptr->write_protects[i];
	total_read_protects	+= cache_ptr->read_protects[i];
	if ( max_read_protects < cache_ptr->max_read_protects[i] ) {
	    max_read_protects = cache_ptr->max_read_protects[i];
	}
        total_insertions        += cache_ptr->insertions[i];
        total_pinned_insertions += cache_ptr->pinned_insertions[i];
        total_clears            += cache_ptr->clears[i];
        total_flushes           += cache_ptr->flushes[i];
        total_evictions         += cache_ptr->evictions[i];
        total_moves           += cache_ptr->moves[i];
	total_entry_flush_moves
				+= cache_ptr->entry_flush_moves[i];
	total_cache_flush_moves
				+= cache_ptr->cache_flush_moves[i];
        total_size_increases    += cache_ptr->size_increases[i];
        total_size_decreases    += cache_ptr->size_decreases[i];
    	total_entry_flush_size_changes
				+= cache_ptr->entry_flush_size_changes[i];
    	total_cache_flush_size_changes
				+= cache_ptr->cache_flush_size_changes[i];
	total_pins              += cache_ptr->pins[i];
	total_unpins            += cache_ptr->unpins[i];
	total_dirty_pins        += cache_ptr->dirty_pins[i];
	total_pinned_flushes    += cache_ptr->pinned_flushes[i];
	total_pinned_clears     += cache_ptr->pinned_clears[i];
#if H5C_COLLECT_CACHE_ENTRY_STATS
    if ( aggregate_max_accesses < cache_ptr->max_accesses[i] )
        aggregate_max_accesses = cache_ptr->max_accesses[i];
    if ( aggregate_min_accesses > aggregate_max_accesses )
        aggregate_min_accesses = aggregate_max_accesses;
    if ( aggregate_min_accesses > cache_ptr->min_accesses[i] )
        aggregate_min_accesses = cache_ptr->min_accesses[i];
    if ( aggregate_max_clears < cache_ptr->max_clears[i] )
        aggregate_max_clears = cache_ptr->max_clears[i];
    if ( aggregate_max_flushes < cache_ptr->max_flushes[i] )
        aggregate_max_flushes = cache_ptr->max_flushes[i];
    if ( aggregate_max_size < cache_ptr->max_size[i] )
        aggregate_max_size = cache_ptr->max_size[i];
    if ( aggregate_max_pins < cache_ptr->max_pins[i] )
        aggregate_max_pins = cache_ptr->max_pins[i];
#endif /* H5C_COLLECT_CACHE_ENTRY_STATS */
    }

    if ( ( total_hits > 0 ) || ( total_misses > 0 ) ) {

        hit_rate = 100.0 * ((double)(total_hits)) /
                   ((double)(total_hits + total_misses));
    } else {
        hit_rate = 0.0;
    }

    if ( cache_ptr->successful_ht_searches > 0 ) {

        average_successful_search_depth =
            ((double)(cache_ptr->total_successful_ht_search_depth)) /
            ((double)(cache_ptr->successful_ht_searches));
    }

    if ( cache_ptr->failed_ht_searches > 0 ) {

        average_failed_search_depth =
            ((double)(cache_ptr->total_failed_ht_search_depth)) /
            ((double)(cache_ptr->failed_ht_searches));
    }


    HDfprintf(stdout, "\n%sH5C: cache statistics for %s\n",
              cache_ptr->prefix, cache_name);

    HDfprintf(stdout, "\n");

    HDfprintf(stdout,
              "%s  hash table insertion / deletions   = %ld / %ld\n",
              cache_ptr->prefix,
              (long)(cache_ptr->total_ht_insertions),
              (long)(cache_ptr->total_ht_deletions));

    HDfprintf(stdout,
              "%s  HT successful / failed searches    = %ld / %ld\n",
              cache_ptr->prefix,
              (long)(cache_ptr->successful_ht_searches),
              (long)(cache_ptr->failed_ht_searches));

    HDfprintf(stdout,
              "%s  Av. HT suc / failed search depth   = %f / %f\n",
              cache_ptr->prefix,
              average_successful_search_depth,
              average_failed_search_depth);

    HDfprintf(stdout,
             "%s  current (max) index size / length  = %ld (%ld) / %ld (%ld)\n",
              cache_ptr->prefix,
              (long)(cache_ptr->index_size),
              (long)(cache_ptr->max_index_size),
              (long)(cache_ptr->index_len),
              (long)(cache_ptr->max_index_len));

    HDfprintf(stdout,
             "%s  current (max) clean/dirty idx size = %ld (%ld) / %ld (%ld)\n",
              cache_ptr->prefix,
              (long)(cache_ptr->clean_index_size),
              (long)(cache_ptr->max_clean_index_size),
              (long)(cache_ptr->dirty_index_size),
              (long)(cache_ptr->max_dirty_index_size));

    HDfprintf(stdout,
             "%s  current (max) slist size / length  = %ld (%ld) / %ld (%ld)\n",
              cache_ptr->prefix,
              (long)(cache_ptr->slist_size),
              (long)(cache_ptr->max_slist_size),
              (long)(cache_ptr->slist_len),
              (long)(cache_ptr->max_slist_len));

    HDfprintf(stdout,
             "%s  current (max) PL size / length     = %ld (%ld) / %ld (%ld)\n",
              cache_ptr->prefix,
              (long)(cache_ptr->pl_size),
              (long)(cache_ptr->max_pl_size),
              (long)(cache_ptr->pl_len),
              (long)(cache_ptr->max_pl_len));

    HDfprintf(stdout,
             "%s  current (max) PEL size / length    = %ld (%ld) / %ld (%ld)\n",
              cache_ptr->prefix,
              (long)(cache_ptr->pel_size),
              (long)(cache_ptr->max_pel_size),
              (long)(cache_ptr->pel_len),
              (long)(cache_ptr->max_pel_len));

    HDfprintf(stdout,
              "%s  current LRU list size / length     = %ld / %ld\n",
              cache_ptr->prefix,
              (long)(cache_ptr->LRU_list_size),
              (long)(cache_ptr->LRU_list_len));

    HDfprintf(stdout,
              "%s  current clean LRU size / length    = %ld / %ld\n",
              cache_ptr->prefix,
              (long)(cache_ptr->cLRU_list_size),
              (long)(cache_ptr->cLRU_list_len));

    HDfprintf(stdout,
              "%s  current dirty LRU size / length    = %ld / %ld\n",
              cache_ptr->prefix,
              (long)(cache_ptr->dLRU_list_size),
              (long)(cache_ptr->dLRU_list_len));

    HDfprintf(stdout,
              "%s  Total hits / misses / hit_rate     = %ld / %ld / %f\n",
              cache_ptr->prefix,
              (long)total_hits,
              (long)total_misses,
              hit_rate);

    HDfprintf(stdout,
              "%s  Total write / read (max) protects  = %ld / %ld (%ld)\n",
              cache_ptr->prefix,
              (long)total_write_protects,
              (long)total_read_protects,
              (long)max_read_protects);

    HDfprintf(stdout,
              "%s  Total clears / flushes / evictions = %ld / %ld / %ld\n",
              cache_ptr->prefix,
              (long)total_clears,
              (long)total_flushes,
              (long)total_evictions);

    HDfprintf(stdout,
	      "%s  Total insertions(pinned) / moves = %ld(%ld) / %ld\n",
              cache_ptr->prefix,
              (long)total_insertions,
              (long)total_pinned_insertions,
              (long)total_moves);

    HDfprintf(stdout,
	      "%s  Total entry / cache flush moves  = %ld / %ld\n",
              cache_ptr->prefix,
              (long)total_entry_flush_moves,
              (long)total_cache_flush_moves);

    HDfprintf(stdout, "%s  Total entry size incrs / decrs     = %ld / %ld\n",
              cache_ptr->prefix,
              (long)total_size_increases,
              (long)total_size_decreases);

    HDfprintf(stdout, "%s  Ttl entry/cache flush size changes = %ld / %ld\n",
              cache_ptr->prefix,
              (long)total_entry_flush_size_changes,
              (long)total_cache_flush_size_changes);

    HDfprintf(stdout,
	      "%s  Total entry pins (dirty) / unpins  = %ld (%ld) / %ld\n",
              cache_ptr->prefix,
              (long)total_pins,
	      (long)total_dirty_pins,
              (long)total_unpins);

    HDfprintf(stdout, "%s  Total pinned flushes / clears      = %ld / %ld\n",
              cache_ptr->prefix,
              (long)total_pinned_flushes,
              (long)total_pinned_clears);

    HDfprintf(stdout, "%s  MSIC: (make space in cache) calls  = %lld\n",
              cache_ptr->prefix,
              (long long)(cache_ptr->calls_to_msic));

    if (cache_ptr->calls_to_msic > 0) {
        average_entries_skipped_per_calls_to_msic =
            (((double)(cache_ptr->total_entries_skipped_in_msic)) /
            ((double)(cache_ptr->calls_to_msic)));
    }

    HDfprintf(stdout, "%s  MSIC: Average/max entries skipped  = %lf / %ld\n",
              cache_ptr->prefix,
              (float)average_entries_skipped_per_calls_to_msic,
              (long)(cache_ptr->max_entries_skipped_in_msic));

    if (cache_ptr->calls_to_msic > 0) {
        average_entries_scanned_per_calls_to_msic =
            (((double)(cache_ptr->total_entries_scanned_in_msic)) /
            ((double)(cache_ptr->calls_to_msic)));
    }

    HDfprintf(stdout, "%s  MSIC: Average/max entries scanned  = %lf / %ld\n",
              cache_ptr->prefix,
              (float)average_entries_scanned_per_calls_to_msic,
              (long)(cache_ptr->max_entries_scanned_in_msic));

    HDfprintf(stdout, "%s  MSIC: Scanned to make space(evict) = %lld\n",
              cache_ptr->prefix,
              (long long)(cache_ptr->entries_scanned_to_make_space));

    HDfprintf(stdout, "%s  MSIC: Scanned to satisfy min_clean = %lld\n",
              cache_ptr->prefix,
              (long long)(cache_ptr->total_entries_scanned_in_msic -
                            cache_ptr->entries_scanned_to_make_space));

#if H5C_COLLECT_CACHE_ENTRY_STATS

    HDfprintf(stdout, "%s  aggregate max / min accesses       = %d / %d\n",
              cache_ptr->prefix,
              (int)aggregate_max_accesses,
              (int)aggregate_min_accesses);

    HDfprintf(stdout, "%s  aggregate max_clears / max_flushes = %d / %d\n",
              cache_ptr->prefix,
              (int)aggregate_max_clears,
              (int)aggregate_max_flushes);

    HDfprintf(stdout, "%s  aggregate max_size / max_pins      = %d / %d\n",
              cache_ptr->prefix,
              (int)aggregate_max_size,
	      (int)aggregate_max_pins);

#endif /* H5C_COLLECT_CACHE_ENTRY_STATS */

    if ( display_detailed_stats )
    {

        for ( i = 0; i <= cache_ptr->max_type_id; i++ ) {

            HDfprintf(stdout, "\n");

            HDfprintf(stdout, "%s  Stats on %s:\n",
                      cache_ptr->prefix,
                      ((cache_ptr->type_name_table_ptr))[i]);

            if ( ( cache_ptr->hits[i] > 0 ) || ( cache_ptr->misses[i] > 0 ) ) {

                hit_rate = 100.0 * ((double)(cache_ptr->hits[i])) /
                          ((double)(cache_ptr->hits[i] + cache_ptr->misses[i]));
            } else {
                hit_rate = 0.0;
            }

            HDfprintf(stdout,
                      "%s    hits / misses / hit_rate       = %ld / %ld / %f\n",
                      cache_ptr->prefix,
                      (long)(cache_ptr->hits[i]),
                      (long)(cache_ptr->misses[i]),
                      hit_rate);

            HDfprintf(stdout,
                      "%s    write / read (max) protects    = %ld / %ld (%d)\n",
                      cache_ptr->prefix,
                      (long)(cache_ptr->write_protects[i]),
                      (long)(cache_ptr->read_protects[i]),
                      (int)(cache_ptr->max_read_protects[i]));

            HDfprintf(stdout,
                     "%s    clears / flushes / evictions   = %ld / %ld / %ld\n",
                      cache_ptr->prefix,
                      (long)(cache_ptr->clears[i]),
                      (long)(cache_ptr->flushes[i]),
                      (long)(cache_ptr->evictions[i]));

            HDfprintf(stdout,
                      "%s    insertions(pinned) / moves   = %ld(%ld) / %ld\n",
                      cache_ptr->prefix,
                      (long)(cache_ptr->insertions[i]),
                      (long)(cache_ptr->pinned_insertions[i]),
                      (long)(cache_ptr->moves[i]));

            HDfprintf(stdout,
                      "%s    entry / cache flush moves    = %ld / %ld\n",
                      cache_ptr->prefix,
                      (long)(cache_ptr->entry_flush_moves[i]),
                      (long)(cache_ptr->cache_flush_moves[i]));

            HDfprintf(stdout,
                      "%s    size increases / decreases     = %ld / %ld\n",
                      cache_ptr->prefix,
                      (long)(cache_ptr->size_increases[i]),
                      (long)(cache_ptr->size_decreases[i]));

            HDfprintf(stdout,
                      "%s    entry/cache flush size changes = %ld / %ld\n",
                      cache_ptr->prefix,
                      (long)(cache_ptr->entry_flush_size_changes[i]),
                      (long)(cache_ptr->cache_flush_size_changes[i]));


            HDfprintf(stdout,
                      "%s    entry pins / unpins            = %ld / %ld\n",
                      cache_ptr->prefix,
                      (long)(cache_ptr->pins[i]),
                      (long)(cache_ptr->unpins[i]));

            HDfprintf(stdout,
                      "%s    entry dirty pins/pin'd flushes  = %ld / %ld\n",
                      cache_ptr->prefix,
                      (long)(cache_ptr->dirty_pins[i]),
                      (long)(cache_ptr->pinned_flushes[i]));

#if H5C_COLLECT_CACHE_ENTRY_STATS

            HDfprintf(stdout,
                      "%s    entry max / min accesses       = %d / %d\n",
                      cache_ptr->prefix,
                      cache_ptr->max_accesses[i],
                      cache_ptr->min_accesses[i]);

            HDfprintf(stdout,
                      "%s    entry max_clears / max_flushes = %d / %d\n",
                      cache_ptr->prefix,
                      cache_ptr->max_clears[i],
                      cache_ptr->max_flushes[i]);

            HDfprintf(stdout,
                      "%s    entry max_size / max_pins      = %d / %d\n",
                      cache_ptr->prefix,
                      (int)(cache_ptr->max_size[i]),
		      (int)(cache_ptr->max_pins[i]));


#endif /* H5C_COLLECT_CACHE_ENTRY_STATS */

        }
    }

    HDfprintf(stdout, "\n");

#endif /* H5C_COLLECT_CACHE_STATS */

done:
    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C_stats() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5C_stats__reset
 *
 * Purpose:     Reset the stats fields to their initial values.
 *
 * Return:      void
 *
 * Programmer:  John Mainzer, 4/28/04
 *
 *		JRM 11/13/08
 *		Added initialization for the new max_clean_index_size and
 *		max_dirty_index_size fields.
 *
 *              MAM -- 01/06/09
 *              Added code to initalize the calls_to_msic,
 *              total_entries_skipped_in_msic, total_entries_scanned_in_msic,
 *              and max_entries_skipped_in_msic fields.
 *
 *-------------------------------------------------------------------------
 */
void
#ifndef NDEBUG
H5C_stats__reset(H5C_t * cache_ptr)
#else /* NDEBUG */
#if H5C_COLLECT_CACHE_STATS
H5C_stats__reset(H5C_t * cache_ptr)
#else /* H5C_COLLECT_CACHE_STATS */
H5C_stats__reset(H5C_t UNUSED * cache_ptr)
#endif /* H5C_COLLECT_CACHE_STATS */
#endif /* NDEBUG */
{
#if H5C_COLLECT_CACHE_STATS
    int i;
#endif /* H5C_COLLECT_CACHE_STATS */

    HDassert( cache_ptr );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );

#if H5C_COLLECT_CACHE_STATS
    for ( i = 0; i <= cache_ptr->max_type_id; i++ )
    {
        cache_ptr->hits[i]			= 0;
        cache_ptr->misses[i]			= 0;
        cache_ptr->write_protects[i]		= 0;
        cache_ptr->read_protects[i]		= 0;
        cache_ptr->max_read_protects[i]		= 0;
        cache_ptr->insertions[i]		= 0;
        cache_ptr->pinned_insertions[i]		= 0;
        cache_ptr->clears[i]			= 0;
        cache_ptr->flushes[i]			= 0;
        cache_ptr->evictions[i]	 		= 0;
        cache_ptr->moves[i]	 		= 0;
        cache_ptr->entry_flush_moves[i]	= 0;
        cache_ptr->cache_flush_moves[i]	= 0;
        cache_ptr->pins[i]	 		= 0;
        cache_ptr->unpins[i]	 		= 0;
        cache_ptr->dirty_pins[i]	 	= 0;
        cache_ptr->pinned_flushes[i]	 	= 0;
        cache_ptr->pinned_clears[i]	 	= 0;
        cache_ptr->size_increases[i] 		= 0;
        cache_ptr->size_decreases[i] 		= 0;
	cache_ptr->entry_flush_size_changes[i]	= 0;
	cache_ptr->cache_flush_size_changes[i]	= 0;
    }

    cache_ptr->total_ht_insertions		= 0;
    cache_ptr->total_ht_deletions		= 0;
    cache_ptr->successful_ht_searches		= 0;
    cache_ptr->total_successful_ht_search_depth	= 0;
    cache_ptr->failed_ht_searches		= 0;
    cache_ptr->total_failed_ht_search_depth	= 0;

    cache_ptr->max_index_len			= 0;
    cache_ptr->max_index_size			= (size_t)0;
    cache_ptr->max_clean_index_size		= (size_t)0;
    cache_ptr->max_dirty_index_size		= (size_t)0;

    cache_ptr->max_slist_len			= 0;
    cache_ptr->max_slist_size			= (size_t)0;

    cache_ptr->max_pl_len			= 0;
    cache_ptr->max_pl_size			= (size_t)0;

    cache_ptr->max_pel_len			= 0;
    cache_ptr->max_pel_size			= (size_t)0;

    cache_ptr->calls_to_msic                    = 0;
    cache_ptr->total_entries_skipped_in_msic    = 0;
    cache_ptr->total_entries_scanned_in_msic    = 0;
    cache_ptr->max_entries_skipped_in_msic      = 0;
    cache_ptr->max_entries_scanned_in_msic      = 0;
    cache_ptr->entries_scanned_to_make_space    = 0;

#if H5C_COLLECT_CACHE_ENTRY_STATS

    for ( i = 0; i <= cache_ptr->max_type_id; i++ )
    {
        cache_ptr->max_accesses[i]		= 0;
        cache_ptr->min_accesses[i]		= 1000000;
        cache_ptr->max_clears[i]		= 0;
        cache_ptr->max_flushes[i]		= 0;
        cache_ptr->max_size[i]			= (size_t)0;
        cache_ptr->max_pins[i]			= 0;
    }

#endif /* H5C_COLLECT_CACHE_ENTRY_STATS */
#endif /* H5C_COLLECT_CACHE_STATS */

    return;

} /* H5C_stats__reset() */


/*-------------------------------------------------------------------------
 * Function:    H5C_dump_cache
 *
 * Purpose:     Print a summary of the contents of the metadata cache for
 *              debugging purposes.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              10/10/10
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_dump_cache(H5C_t * cache_ptr,
               const char *  cache_name)
{
    herr_t              ret_value = SUCCEED;   /* Return value */
    int                 i;
    H5C_cache_entry_t * entry_ptr = NULL;
    H5SL_t *            slist_ptr = NULL;
    H5SL_node_t *       node_ptr = NULL;

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(cache_ptr != NULL);
    HDassert(cache_ptr->magic == H5C__H5C_T_MAGIC);
    HDassert(cache_name != NULL );

    /* First, create a skip list */
    slist_ptr = H5SL_create(H5SL_TYPE_HADDR, NULL);

    if ( slist_ptr == NULL ) {

        HGOTO_ERROR(H5E_CACHE, H5E_CANTCREATE, FAIL, "can't create skip list.")
    }

    /* Next, scan the index, and insert all entries in the skip list.
     * Do this, as we want to display cache entries in increasing address
     * order.
     */
    for ( i = 0; i < H5C__HASH_TABLE_LEN; i++ ) {

        entry_ptr = cache_ptr->index[i];

        while ( entry_ptr != NULL ) {

            HDassert( entry_ptr->magic == H5C__H5C_CACHE_ENTRY_T_MAGIC );

            if ( H5SL_insert(slist_ptr, entry_ptr, &(entry_ptr->addr)) < 0 ) {

                HGOTO_ERROR(H5E_CACHE, H5E_BADVALUE, FAIL, \
                            "Can't insert entry in skip list")
            }

            entry_ptr = entry_ptr->ht_next;
        }
    }

    /* If we get this far, all entries in the cache are listed in the
     * skip list -- scan the skip list generating the desired output.
     */

    HDfprintf(stdout, "\n\nDump of metadata cache \"%s\".\n", cache_name);
    HDfprintf(stdout,
        "Num:   Addr:           Len:    Type:   Prot:   Pinned: Dirty:\n");

    i = 0;

    node_ptr = H5SL_first(slist_ptr);

    if ( node_ptr != NULL ) {

        entry_ptr = (H5C_cache_entry_t *)H5SL_item(node_ptr);

    } else {

        entry_ptr = NULL;
    }

    while ( entry_ptr != NULL ) {

        HDassert( entry_ptr->magic == H5C__H5C_CACHE_ENTRY_T_MAGIC );

        HDfprintf(stdout,
            "%s%d       0x%08llx        0x%3llx %2d     %d      %d      %d\n",
             cache_ptr->prefix, i,
             (long long)(entry_ptr->addr),
             (long long)(entry_ptr->size),
             (int)(entry_ptr->type->id),
             (int)(entry_ptr->is_protected),
             (int)(entry_ptr->is_pinned),
             (int)(entry_ptr->is_dirty));

        /* increment node_ptr before we delete its target */
        node_ptr = H5SL_next(node_ptr);

        /* remove the first item in the skip list */
        if ( H5SL_remove(slist_ptr, &(entry_ptr->addr)) != entry_ptr ) {

            HGOTO_ERROR(H5E_CACHE, H5E_BADVALUE, FAIL, \
                        "Can't delete entry from skip list.")
        }

        if ( node_ptr != NULL ) {

            entry_ptr = (H5C_cache_entry_t *)H5SL_item(node_ptr);

        } else {

            entry_ptr = NULL;
        }

        i++;
    }

    HDfprintf(stdout, "\n\n");

    /* Finally, discard the skip list */

    HDassert( H5SL_count(slist_ptr) == 0 );

    H5SL_close(slist_ptr);

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C_dump_cache() */


/*-------------------------------------------------------------------------
 * Function:    H5C_unpin_entry()
 *
 * Purpose:	Unpin a cache entry.  The entry must be unprotected at
 * 		the time of call, and must be pinned.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              3/22/06
 *
 * Modifications:
 *
 * 		JRM -- 4/26/06
 *		Modified routine to allow it to operate on protected
 *		entries.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_unpin_entry(void *_entry_ptr)
{
    H5C_t             * cache_ptr;
    H5C_cache_entry_t * entry_ptr = (H5C_cache_entry_t *)_entry_ptr; /* Pointer to entry to unpin */
    herr_t              ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(entry_ptr);
    cache_ptr = entry_ptr->cache_ptr;
    HDassert(cache_ptr);
    HDassert(cache_ptr->magic == H5C__H5C_T_MAGIC);

    if ( ! ( entry_ptr->is_pinned ) ) {

        HGOTO_ERROR(H5E_CACHE, H5E_CANTUNPIN, FAIL, "Entry isn't pinned")
    }

    if ( ! ( entry_ptr->is_protected ) ) {

        H5C__UPDATE_RP_FOR_UNPIN(cache_ptr, entry_ptr, FAIL)
    }

    entry_ptr->is_pinned = FALSE;

    H5C__UPDATE_STATS_FOR_UNPIN(cache_ptr, entry_ptr)

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C_unpin_entry() */


/*-------------------------------------------------------------------------
 * Function:    H5C_unprotect
 *
 * Purpose:	Undo an H5C_protect() call -- specifically, mark the
 *		entry as unprotected, remove it from the protected list,
 *		and give it back to the replacement policy.
 *
 *		The TYPE and ADDR arguments must be the same as those in
 *		the corresponding call to H5C_protect() and the THING
 *		argument must be the value returned by that call to
 *		H5C_protect().
 *
 *		The primary_dxpl_id and secondary_dxpl_id parameters
 *		specify the dxpl_ids used on the first write occasioned
 *		by the unprotect (primary_dxpl_id), and on all subsequent
 *		writes (secondary_dxpl_id).  Since an uprotect cannot
 *		occasion a write at present, all this is moot for now.
 *		However, things change, and in any case,
 *		H5C_flush_single_entry() needs primary_dxpl_id and
 *		secondary_dxpl_id in its parameter list.
 *
 *		The function can't cause a read either, so the dxpl_id
 *		parameters are moot in this case as well.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 *		If the deleted flag is TRUE, simply remove the target entry
 *		from the cache, clear it, and free it without writing it to
 *		disk.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              6/2/04
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_unprotect(H5F_t *		  f,
              hid_t		  primary_dxpl_id,
              hid_t		  secondary_dxpl_id,
              const H5C_class_t * type,
              haddr_t		  addr,
              void *		  thing,
              unsigned int        flags)
{
    H5C_t *		cache_ptr;
    hbool_t		deleted;
    hbool_t		dirtied;
    hbool_t             set_flush_marker;
    hbool_t		pin_entry;
    hbool_t		unpin_entry;
    hbool_t		free_file_space;
    hbool_t		take_ownership;
    hbool_t 		was_clean;
#ifdef H5_HAVE_PARALLEL
    hbool_t		clear_entry = FALSE;
#endif /* H5_HAVE_PARALLEL */
    H5C_cache_entry_t *	entry_ptr;
    H5C_cache_entry_t *	test_entry_ptr;
    herr_t              ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    deleted          = ( (flags & H5C__DELETED_FLAG) != 0 );
    dirtied          = ( (flags & H5C__DIRTIED_FLAG) != 0 );
    set_flush_marker = ( (flags & H5C__SET_FLUSH_MARKER_FLAG) != 0 );
    pin_entry        = ( (flags & H5C__PIN_ENTRY_FLAG) != 0 );
    unpin_entry      = ( (flags & H5C__UNPIN_ENTRY_FLAG) != 0 );
    free_file_space  = ( (flags & H5C__FREE_FILE_SPACE_FLAG) != 0 );
    take_ownership   = ( (flags & H5C__TAKE_OWNERSHIP_FLAG) != 0 );

    HDassert( f );
    HDassert( f->shared );

    cache_ptr = f->shared->cache;

    HDassert( cache_ptr );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );
    HDassert( type );
    HDassert( type->clear );
    HDassert( type->flush );
    HDassert( H5F_addr_defined(addr) );
    HDassert( thing );
    HDassert( ! ( pin_entry && unpin_entry ) );
    HDassert( ( ! free_file_space ) || ( deleted ) );   /* deleted flag must accompany free_file_space */
    HDassert( ( ! take_ownership ) || ( deleted ) );    /* deleted flag must accompany take_ownership */
    HDassert( ! ( free_file_space && take_ownership ) );    /* can't have both free_file_space & take_ownership */

    entry_ptr = (H5C_cache_entry_t *)thing;

    HDassert( entry_ptr->addr == addr );
    HDassert( entry_ptr->type == type );

    /* also set the dirtied variable if the dirtied field is set in
     * the entry.
     */
    dirtied |= entry_ptr->dirtied;
    was_clean = ! ( entry_ptr->is_dirty );

#if H5C_DO_EXTREME_SANITY_CHECKS
        if ( H5C_validate_lru_list(cache_ptr) < 0 ) {

                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                            "LRU sanity check failed.\n");
        }
#endif /* H5C_DO_EXTREME_SANITY_CHECKS */


    /* if the entry has multiple read only protects, just decrement
     * the ro_ref_counter.  Don't actually unprotect until the ref count
     * drops to zero.
     */
    if ( entry_ptr->ro_ref_count > 1 ) {

	HDassert( entry_ptr->is_protected );
        HDassert( entry_ptr->is_read_only );

	if ( dirtied ) {

            HGOTO_ERROR(H5E_CACHE, H5E_CANTUNPROTECT, FAIL, \
                        "Read only entry modified(1)??")
	}

	(entry_ptr->ro_ref_count)--;

        /* Pin or unpin the entry as requested. */
        if ( pin_entry ) {

            if ( entry_ptr->is_pinned ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTPIN, FAIL, \
                            "Entry already pinned???")
            }
	    entry_ptr->is_pinned = TRUE;
	    H5C__UPDATE_STATS_FOR_PIN(cache_ptr, entry_ptr)

        } else if ( unpin_entry ) {

            if ( ! ( entry_ptr->is_pinned ) ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTUNPIN, FAIL, \
			    "Entry already unpinned???")
            }
	    entry_ptr->is_pinned = FALSE;
	    H5C__UPDATE_STATS_FOR_UNPIN(cache_ptr, entry_ptr)

        }

    } else {

	if ( entry_ptr->is_read_only ) {

	    HDassert( entry_ptr->ro_ref_count == 1 );

	    if ( dirtied ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTUNPROTECT, FAIL, \
                            "Read only entry modified(2)??")
	    }

	    entry_ptr->is_read_only = FALSE;
	    entry_ptr->ro_ref_count = 0;
	}

#ifdef H5_HAVE_PARALLEL
        /* When the H5C code is used to implement the metadata cache in the
         * PHDF5 case, only the cache on process 0 is allowed to write to file.
         * All the other metadata caches must hold dirty entries until they
         * are told that the entries are clean.
         *
         * The clear_on_unprotect flag in the H5C_cache_entry_t structure
         * exists to deal with the case in which an entry is protected when
         * its cache receives word that the entry is now clean.  In this case,
         * the clear_on_unprotect flag is set, and the entry is flushed with
         * the H5C__FLUSH_CLEAR_ONLY_FLAG.
         *
         * All this is a bit awkward, but until the metadata cache entries
         * are contiguous, with only one dirty flag, we have to let the supplied
         * functions deal with the reseting the is_dirty flag.
         */
        if ( entry_ptr->clear_on_unprotect ) {

            HDassert( entry_ptr->is_dirty );

            entry_ptr->clear_on_unprotect = FALSE;

            if ( ! dirtied ) {

                clear_entry = TRUE;
            }
        }
#endif /* H5_HAVE_PARALLEL */

        if ( ! (entry_ptr->is_protected) ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTUNPROTECT, FAIL, \
                            "Entry already unprotected??")
        }

        /* mark the entry as dirty if appropriate */
        entry_ptr->is_dirty = ( (entry_ptr->is_dirty) || dirtied );

        if ( ( was_clean ) && ( entry_ptr->is_dirty ) ) {

	    H5C__UPDATE_INDEX_FOR_ENTRY_DIRTY(cache_ptr, entry_ptr)
	}

        /* Pin or unpin the entry as requested. */
        if ( pin_entry ) {

            if ( entry_ptr->is_pinned ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTPIN, FAIL, \
                            "Entry already pinned???")
            }
	    entry_ptr->is_pinned = TRUE;
	    H5C__UPDATE_STATS_FOR_PIN(cache_ptr, entry_ptr)

        } else if ( unpin_entry ) {

            if ( ! ( entry_ptr->is_pinned ) ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTUNPIN, FAIL, \
			    "Entry already unpinned???")
            }
	    entry_ptr->is_pinned = FALSE;
	    H5C__UPDATE_STATS_FOR_UNPIN(cache_ptr, entry_ptr)

        }

        /* H5C__UPDATE_RP_FOR_UNPROTECT will place the unprotected entry on
         * the pinned entry list if entry_ptr->is_pinned is TRUE.
         */
        H5C__UPDATE_RP_FOR_UNPROTECT(cache_ptr, entry_ptr, FAIL)

        entry_ptr->is_protected = FALSE;

        /* if the entry is dirty, 'or' its flush_marker with the set flush flag,
         * and then add it to the skip list if it isn't there already.
         */

        if ( entry_ptr->is_dirty ) {

            entry_ptr->flush_marker |= set_flush_marker;

            if ( ! (entry_ptr->in_slist) ) {

                H5C__INSERT_ENTRY_IN_SLIST(cache_ptr, entry_ptr, FAIL)
            }
        }

        /* this implementation of the "deleted" option is a bit inefficient, as
         * we re-insert the entry to be deleted into the replacement policy
         * data structures, only to remove them again.  Depending on how often
         * we do this, we may want to optimize a bit.
         *
         * On the other hand, this implementation is reasonably clean, and
         * makes good use of existing code.
         *                                             JRM - 5/19/04
         */
        if ( deleted ) {

            /* the following first flush flag will never be used as we are
             * calling H5C_flush_single_entry with both the
             * H5C__FLUSH_CLEAR_ONLY_FLAG and H5C__FLUSH_INVALIDATE_FLAG flags.
	     * However, it is needed for the function call.
             */
            hbool_t	dummy_first_flush = TRUE;
            unsigned    flush_flags = (H5C__FLUSH_CLEAR_ONLY_FLAG |
                                         H5C__FLUSH_INVALIDATE_FLAG);

	    /* we can't delete a pinned entry */
	    HDassert ( ! (entry_ptr->is_pinned ) );

            /* verify that the target entry is in the cache. */

            H5C__SEARCH_INDEX(cache_ptr, addr, test_entry_ptr, FAIL)

            if ( test_entry_ptr == NULL ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTUNPROTECT, FAIL, \
                            "entry not in hash table?!?.")
            }
            else if ( test_entry_ptr != entry_ptr ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTUNPROTECT, FAIL, \
                            "hash table contains multiple entries for addr?!?.")
            }

            /* Pass along 'free file space' flag to cache client */

            entry_ptr->free_file_space_on_destroy = free_file_space;

            /* Set the "take ownership" flag for the flush, if needed */
            if ( take_ownership) {

                flush_flags |= H5C__TAKE_OWNERSHIP_FLAG;
            }

            if ( H5C_flush_single_entry(f,
                                        primary_dxpl_id,
                                        secondary_dxpl_id,
                                        type,
                                        addr,
                                        flush_flags,
                                        &dummy_first_flush,
                                        TRUE) < 0 ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTUNPROTECT, FAIL, "Can't flush.")
            }
        }
#ifdef H5_HAVE_PARALLEL
        else if ( clear_entry ) {

            /* the following first flush flag will never be used as we are
             * calling H5C_flush_single_entry with the
	     * H5C__FLUSH_CLEAR_ONLY_FLAG flag.  However, it is needed for
	     * the function call.
             */
            hbool_t		dummy_first_flush = TRUE;

            /* verify that the target entry is in the cache. */

            H5C__SEARCH_INDEX(cache_ptr, addr, test_entry_ptr, FAIL)

            if ( test_entry_ptr == NULL ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTUNPROTECT, FAIL, \
                            "entry not in hash table?!?.")
            }
            else if ( test_entry_ptr != entry_ptr ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTUNPROTECT, FAIL, \
                            "hash table contains multiple entries for addr?!?.")
            }

            if ( H5C_flush_single_entry(f,
                                        primary_dxpl_id,
                                        secondary_dxpl_id,
                                        type,
                                        addr,
                                        H5C__FLUSH_CLEAR_ONLY_FLAG,
                                        &dummy_first_flush,
                                        TRUE) < 0 ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTUNPROTECT, FAIL, "Can't clear.")
            }
        }
#endif /* H5_HAVE_PARALLEL */
    }

    H5C__UPDATE_STATS_FOR_UNPROTECT(cache_ptr)

done:

#if H5C_DO_EXTREME_SANITY_CHECKS
        if ( H5C_validate_lru_list(cache_ptr) < 0 ) {

                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                            "LRU sanity check failed.\n");
        }
#endif /* H5C_DO_EXTREME_SANITY_CHECKS */

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C_unprotect() */


/*-------------------------------------------------------------------------
 * Function:    H5C_validate_resize_config()
 *
 * Purpose:	Run a sanity check on the specified sections of the
 *		provided instance of struct H5C_auto_size_ctl_t.
 *
 *		Do nothing and return SUCCEED if no errors are detected,
 *		and flag an error and return FAIL otherwise.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              3/23/05
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_validate_resize_config(H5C_auto_size_ctl_t * config_ptr,
                           unsigned int tests)
{
    herr_t              ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    if ( config_ptr == NULL ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "NULL config_ptr on entry.")
    }

    if ( config_ptr->version != H5C__CURR_AUTO_SIZE_CTL_VER ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Unknown config version.")
    }


    if ( (tests & H5C_RESIZE_CFG__VALIDATE_GENERAL) != 0 ) {

        if ( ( config_ptr->set_initial_size != TRUE ) &&
             ( config_ptr->set_initial_size != FALSE ) ) {

            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, \
                        "set_initial_size must be either TRUE or FALSE");
        }

        if ( config_ptr->max_size > H5C__MAX_MAX_CACHE_SIZE ) {

            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "max_size too big");
        }

        if ( config_ptr->min_size < H5C__MIN_MAX_CACHE_SIZE ) {

            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "min_size too small");
        }

        if ( config_ptr->min_size > config_ptr->max_size ) {

            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "min_size > max_size");
        }

        if ( ( config_ptr->set_initial_size ) &&
             ( ( config_ptr->initial_size < config_ptr->min_size ) ||
               ( config_ptr->initial_size > config_ptr->max_size ) ) ) {

            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, \
                  "initial_size must be in the interval [min_size, max_size]");
        }

        if ( ( config_ptr->min_clean_fraction < 0.0 ) ||
             ( config_ptr->min_clean_fraction > 1.0 ) ) {

            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, \
                  "min_clean_fraction must be in the interval [0.0, 1.0]");
        }

        if ( config_ptr->epoch_length < H5C__MIN_AR_EPOCH_LENGTH ) {

            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "epoch_length too small");
        }

        if ( config_ptr->epoch_length > H5C__MAX_AR_EPOCH_LENGTH ) {

            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "epoch_length too big");
        }
    } /* H5C_RESIZE_CFG__VALIDATE_GENERAL */


    if ( (tests & H5C_RESIZE_CFG__VALIDATE_INCREMENT) != 0 ) {

        if ( ( config_ptr->incr_mode != H5C_incr__off ) &&
             ( config_ptr->incr_mode != H5C_incr__threshold ) ) {

            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "Invalid incr_mode");
        }

        if ( config_ptr->incr_mode == H5C_incr__threshold ) {

            if ( ( config_ptr->lower_hr_threshold < 0.0 ) ||
                 ( config_ptr->lower_hr_threshold > 1.0 ) ) {

                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, \
                    "lower_hr_threshold must be in the range [0.0, 1.0]");
            }

            if ( config_ptr->increment < 1.0 ) {

                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, \
                            "increment must be greater than or equal to 1.0");
            }

            if ( ( config_ptr->apply_max_increment != TRUE ) &&
                 ( config_ptr->apply_max_increment != FALSE ) ) {

                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, \
                            "apply_max_increment must be either TRUE or FALSE");
            }

            /* no need to check max_increment, as it is a size_t,
             * and thus must be non-negative.
             */
        } /* H5C_incr__threshold */

        switch ( config_ptr->flash_incr_mode )
        {
            case H5C_flash_incr__off:
                /* nothing to do here */
                break;

            case H5C_flash_incr__add_space:
                if ( ( config_ptr->flash_multiple < 0.1 ) ||
                     ( config_ptr->flash_multiple > 10.0 ) ) {

                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, \
                        "flash_multiple must be in the range [0.1, 10.0]");
                }

                if ( ( config_ptr->flash_threshold < 0.1 ) ||
                     ( config_ptr->flash_threshold > 1.0 ) ) {

                    HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, \
                           "flash_threshold must be in the range [0.1, 1.0]");
                }
                break;

            default:
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, \
                            "Invalid flash_incr_mode");
                break;
        }
    } /* H5C_RESIZE_CFG__VALIDATE_INCREMENT */


    if ( (tests & H5C_RESIZE_CFG__VALIDATE_DECREMENT) != 0 ) {

        if ( ( config_ptr->decr_mode != H5C_decr__off ) &&
             ( config_ptr->decr_mode != H5C_decr__threshold ) &&
             ( config_ptr->decr_mode != H5C_decr__age_out ) &&
             ( config_ptr->decr_mode != H5C_decr__age_out_with_threshold )
           ) {

            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "Invalid decr_mode");
        }

        if ( config_ptr->decr_mode == H5C_decr__threshold ) {

            if ( config_ptr->upper_hr_threshold > 1.0 ) {

                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, \
                            "upper_hr_threshold must be <= 1.0");
            }

            if ( ( config_ptr->decrement > 1.0 ) ||
                 ( config_ptr->decrement < 0.0 ) ) {

                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, \
                            "decrement must be in the interval [0.0, 1.0]");
            }

            /* no need to check max_decrement as it is a size_t
             * and thus must be non-negative.
             */
        } /* H5C_decr__threshold */

        if ( ( config_ptr->decr_mode == H5C_decr__age_out ) ||
             ( config_ptr->decr_mode == H5C_decr__age_out_with_threshold )
           ) {

            if ( config_ptr->epochs_before_eviction < 1 ) {

                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, \
                            "epochs_before_eviction must be positive");
            }

            if ( config_ptr->epochs_before_eviction > H5C__MAX_EPOCH_MARKERS ) {

                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, \
                            "epochs_before_eviction too big");
            }

            if ( ( config_ptr->apply_empty_reserve != TRUE ) &&
                 ( config_ptr->apply_empty_reserve != FALSE ) ) {

                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, \
                            "apply_empty_reserve must be either TRUE or FALSE");
            }

            if ( ( config_ptr->apply_empty_reserve ) &&
                 ( ( config_ptr->empty_reserve > 1.0 ) ||
                   ( config_ptr->empty_reserve < 0.0 ) ) ) {

                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, \
                            "empty_reserve must be in the interval [0.0, 1.0]");
            }

            /* no need to check max_decrement as it is a size_t
             * and thus must be non-negative.
             */
        } /* H5C_decr__age_out || H5C_decr__age_out_with_threshold */

        if ( config_ptr->decr_mode == H5C_decr__age_out_with_threshold ) {

            if ( ( config_ptr->upper_hr_threshold > 1.0 ) ||
                 ( config_ptr->upper_hr_threshold < 0.0 ) ) {

                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, \
                       "upper_hr_threshold must be in the interval [0.0, 1.0]");
            }
        } /* H5C_decr__age_out_with_threshold */

    } /* H5C_RESIZE_CFG__VALIDATE_DECREMENT */


    if ( (tests & H5C_RESIZE_CFG__VALIDATE_INTERACTIONS) != 0 ) {

        if ( ( config_ptr->incr_mode == H5C_incr__threshold )
             &&
             ( ( config_ptr->decr_mode == H5C_decr__threshold )
               ||
               ( config_ptr->decr_mode == H5C_decr__age_out_with_threshold )
             )
             &&
             ( config_ptr->lower_hr_threshold
               >=
               config_ptr->upper_hr_threshold
             )
           ) {

            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, \
                        "conflicting threshold fields in config.")
        }
    } /* H5C_RESIZE_CFG__VALIDATE_INTERACTIONS */

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C_validate_resize_config() */


/*************************************************************************/
/**************************** Private Functions: *************************/
/*************************************************************************/

/*-------------------------------------------------------------------------
 *
 * Function:	H5C__auto_adjust_cache_size
 *
 * Purpose:    	Obtain the current full cache hit rate, and compare it
 *		with the hit rate thresholds for modifying cache size.
 *		If one of the thresholds has been crossed, adjusts the
 *		size of the cache accordingly.
 *
 *		The function then resets the full cache hit rate
 *		statistics, and exits.
 *
 * Return:      Non-negative on success/Negative on failure or if there was
 *		an attempt to flush a protected item.
 *
 *
 * Programmer:  John Mainzer, 10/7/04
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5C__auto_adjust_cache_size(H5F_t * f,
                            hid_t primary_dxpl_id,
                            hid_t secondary_dxpl_id,
                            hbool_t write_permitted,
                            hbool_t * first_flush_ptr)
{
    H5C_t *			cache_ptr = f->shared->cache;
    herr_t			result;
    hbool_t			inserted_epoch_marker = FALSE;
    size_t			new_max_cache_size = 0;
    size_t			old_max_cache_size = 0;
    size_t			new_min_clean_size = 0;
    size_t			old_min_clean_size = 0;
    double			hit_rate;
    enum H5C_resize_status	status = in_spec; /* will change if needed */
    herr_t			ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HDassert( f );
    HDassert( cache_ptr );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );
    HDassert( cache_ptr->cache_accesses >=
              (cache_ptr->resize_ctl).epoch_length );
    HDassert( 0.0 <= (cache_ptr->resize_ctl).min_clean_fraction );
    HDassert( (cache_ptr->resize_ctl).min_clean_fraction <= 100.0 );

    if ( !cache_ptr->resize_enabled ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Auto cache resize disabled.")
    }

    HDassert( ( (cache_ptr->resize_ctl).incr_mode != H5C_incr__off ) || \
              ( (cache_ptr->resize_ctl).decr_mode != H5C_decr__off ) );

    if ( H5C_get_cache_hit_rate(cache_ptr, &hit_rate) != SUCCEED ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Can't get hit rate.")
    }

    HDassert( ( 0.0 <= hit_rate ) && ( hit_rate <= 1.0 ) );

    switch ( (cache_ptr->resize_ctl).incr_mode )
    {
        case H5C_incr__off:
            if ( cache_ptr->size_increase_possible ) {

                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                           "size_increase_possible but H5C_incr__off?!?!?")
            }
            break;

        case H5C_incr__threshold:
            if ( hit_rate < (cache_ptr->resize_ctl).lower_hr_threshold ) {

                if ( ! cache_ptr->size_increase_possible ) {

                    status = increase_disabled;

                } else if ( cache_ptr->max_cache_size >=
                            (cache_ptr->resize_ctl).max_size ) {

                    HDassert( cache_ptr->max_cache_size == \
                              (cache_ptr->resize_ctl).max_size );
                    status = at_max_size;

                } else if ( ! cache_ptr->cache_full ) {

                    status = not_full;

                } else {

                    new_max_cache_size = (size_t)
                                     (((double)(cache_ptr->max_cache_size)) *
                                      (cache_ptr->resize_ctl).increment);

                    /* clip to max size if necessary */
                    if ( new_max_cache_size >
                         (cache_ptr->resize_ctl).max_size ) {

                        new_max_cache_size = (cache_ptr->resize_ctl).max_size;
                    }

                    /* clip to max increment if necessary */
                    if ( ( (cache_ptr->resize_ctl).apply_max_increment ) &&
                         ( (cache_ptr->max_cache_size +
                            (cache_ptr->resize_ctl).max_increment) <
                           new_max_cache_size ) ) {

                        new_max_cache_size = cache_ptr->max_cache_size +
                                         (cache_ptr->resize_ctl).max_increment;
                    }

                    status = increase;
                }
            }
            break;

        default:
            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "unknown incr_mode.")
    }

    /* If the decr_mode is either age out or age out with threshold, we
     * must run the marker maintenance code, whether we run the size
     * reduction code or not.  We do this in two places -- here we
     * insert a new marker if the number of active epoch markers is
     * is less than the the current epochs before eviction, and after
     * the ageout call, we cycle the markers.
     *
     * However, we can't call the ageout code or cycle the markers
     * unless there was a full complement of markers in place on
     * entry.  The inserted_epoch_marker flag is used to track this.
     */

    if ( ( ( (cache_ptr->resize_ctl).decr_mode == H5C_decr__age_out )
           ||
           ( (cache_ptr->resize_ctl).decr_mode ==
              H5C_decr__age_out_with_threshold
           )
         )
         &&
         ( cache_ptr->epoch_markers_active <
           (cache_ptr->resize_ctl).epochs_before_eviction
         )
       ) {

        result = H5C__autoadjust__ageout__insert_new_marker(cache_ptr);

        if ( result != SUCCEED ) {

            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                        "can't insert new epoch marker.")

        } else {

            inserted_epoch_marker = TRUE;
        }
    }

    /* don't run the cache size decrease code unless the cache size
     * increase code is disabled, or the size increase code sees no need
     * for action.  In either case, status == in_spec at this point.
     */

    if ( status == in_spec ) {

        switch ( (cache_ptr->resize_ctl).decr_mode )
        {
            case H5C_decr__off:
                break;

            case H5C_decr__threshold:
                if ( hit_rate > (cache_ptr->resize_ctl).upper_hr_threshold ) {

                    if ( ! cache_ptr->size_decrease_possible ) {

                        status = decrease_disabled;

                    } else if ( cache_ptr->max_cache_size <=
                                (cache_ptr->resize_ctl).min_size ) {

                        HDassert( cache_ptr->max_cache_size ==
                                  (cache_ptr->resize_ctl).min_size );
                        status = at_min_size;

                    } else {

                        new_max_cache_size = (size_t)
                                 (((double)(cache_ptr->max_cache_size)) *
                                  (cache_ptr->resize_ctl).decrement);

                        /* clip to min size if necessary */
                        if ( new_max_cache_size <
                             (cache_ptr->resize_ctl).min_size ) {

                            new_max_cache_size =
                                (cache_ptr->resize_ctl).min_size;
                        }

                        /* clip to max decrement if necessary */
                        if ( ( (cache_ptr->resize_ctl).apply_max_decrement ) &&
                             ( ((cache_ptr->resize_ctl).max_decrement +
                                new_max_cache_size) <
                               cache_ptr->max_cache_size ) ) {

                            new_max_cache_size = cache_ptr->max_cache_size -
                                         (cache_ptr->resize_ctl).max_decrement;
                        }

                        status = decrease;
                    }
                }
                break;

            case H5C_decr__age_out_with_threshold:
            case H5C_decr__age_out:
                if ( ! inserted_epoch_marker ) {

                    if ( ! cache_ptr->size_decrease_possible ) {

                        status = decrease_disabled;

                    } else {

                        result = H5C__autoadjust__ageout(f,
                                                         hit_rate,
                                                         &status,
                                                         &new_max_cache_size,
                                                         primary_dxpl_id,
                                                         secondary_dxpl_id,
                                                         write_permitted,
                                                         first_flush_ptr);

                        if ( result != SUCCEED ) {

                            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                                        "ageout code failed.")
                        }
                    }
                }
                break;

            default:
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "unknown incr_mode.")
        }
    }

    /* cycle the epoch markers here if appropriate */
    if ( ( ( (cache_ptr->resize_ctl).decr_mode == H5C_decr__age_out )
           ||
           ( (cache_ptr->resize_ctl).decr_mode ==
              H5C_decr__age_out_with_threshold
           )
         )
         &&
         ( ! inserted_epoch_marker )
       ) {

        /* move last epoch marker to the head of the LRU list */
        result = H5C__autoadjust__ageout__cycle_epoch_marker(cache_ptr);

        if ( result != SUCCEED ) {

            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                        "error cycling epoch marker.")
        }
    }

    if ( ( status == increase ) || ( status == decrease ) ) {

        old_max_cache_size = cache_ptr->max_cache_size;
        old_min_clean_size = cache_ptr->min_clean_size;

        new_min_clean_size = (size_t)
                             ((double)new_max_cache_size *
                              ((cache_ptr->resize_ctl).min_clean_fraction));

        /* new_min_clean_size is of size_t, and thus must be non-negative.
         * Hence we have
         *
         * 	( 0 <= new_min_clean_size ).
         *
 	 * by definition.
         */
        HDassert( new_min_clean_size <= new_max_cache_size );
        HDassert( (cache_ptr->resize_ctl).min_size <= new_max_cache_size );
        HDassert( new_max_cache_size <= (cache_ptr->resize_ctl).max_size );

        cache_ptr->max_cache_size = new_max_cache_size;
        cache_ptr->min_clean_size = new_min_clean_size;

        if ( status == increase ) {

            cache_ptr->cache_full = FALSE;

        } else if ( status == decrease ) {

            cache_ptr->size_decreased = TRUE;
        }

	/* update flash cache size increase fields as appropriate */
	if ( cache_ptr->flash_size_increase_possible ) {

            switch ( (cache_ptr->resize_ctl).flash_incr_mode )
            {
                case H5C_flash_incr__off:

                    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                     "flash_size_increase_possible but H5C_flash_incr__off?!")
                    break;

                case H5C_flash_incr__add_space:
                    cache_ptr->flash_size_increase_threshold =
                        (size_t)
                        (((double)(cache_ptr->max_cache_size)) *
                         ((cache_ptr->resize_ctl).flash_threshold));
                     break;

                default: /* should be unreachable */
                    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                                "Unknown flash_incr_mode?!?!?.")
                    break;
            }
        }
    }

    if ( (cache_ptr->resize_ctl).rpt_fcn != NULL ) {

        (*((cache_ptr->resize_ctl).rpt_fcn))
            (cache_ptr,
             H5C__CURR_AUTO_RESIZE_RPT_FCN_VER,
             hit_rate,
             status,
             old_max_cache_size,
             new_max_cache_size,
             old_min_clean_size,
             new_min_clean_size);
    }

    if ( H5C_reset_cache_hit_rate_stats(cache_ptr) != SUCCEED ) {

        /* this should be impossible... */
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                    "H5C_reset_cache_hit_rate_stats failed.")
    }

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C__auto_adjust_cache_size() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5C__autoadjust__ageout
 *
 * Purpose:     Implement the ageout automatic cache size decrement
 *		algorithm.  Note that while this code evicts aged out
 *		entries, the code does not change the maximum cache size.
 *		Instead, the function simply computes the new value (if
 *		any change is indicated) and reports this value in
 *		*new_max_cache_size_ptr.
 *
 * Return:      Non-negative on success/Negative on failure or if there was
 *              an attempt to flush a protected item.
 *
 *
 * Programmer:  John Mainzer, 11/18/04
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5C__autoadjust__ageout(H5F_t * f,
                        double hit_rate,
                        enum H5C_resize_status * status_ptr,
                        size_t * new_max_cache_size_ptr,
                        hid_t primary_dxpl_id,
                        hid_t secondary_dxpl_id,
                        hbool_t write_permitted,
                        hbool_t * first_flush_ptr)
{
    H5C_t *     cache_ptr = f->shared->cache;
    herr_t	result;
    size_t	test_size;
    herr_t	ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HDassert( f );
    HDassert( cache_ptr );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );
    HDassert( ( status_ptr ) && ( *status_ptr == in_spec ) );
    HDassert( ( new_max_cache_size_ptr ) && ( *new_max_cache_size_ptr == 0 ) );

    /* remove excess epoch markers if any */
    if ( cache_ptr->epoch_markers_active >
         (cache_ptr->resize_ctl).epochs_before_eviction ) {

        result = H5C__autoadjust__ageout__remove_excess_markers(cache_ptr);

        if ( result != SUCCEED ) {

            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                        "can't remove excess epoch markers.")
        }
    }

    if ( ( (cache_ptr->resize_ctl).decr_mode == H5C_decr__age_out )
         ||
         ( ( (cache_ptr->resize_ctl).decr_mode ==
              H5C_decr__age_out_with_threshold
               )
           &&
           ( hit_rate >= (cache_ptr->resize_ctl).upper_hr_threshold )
         )
       ) {

        if ( cache_ptr->max_cache_size > (cache_ptr->resize_ctl).min_size ){

            /* evict aged out cache entries if appropriate... */
            if(H5C__autoadjust__ageout__evict_aged_out_entries(f, primary_dxpl_id,
                    secondary_dxpl_id, write_permitted, first_flush_ptr) < 0)
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "error flushing aged out entries.")

            /* ... and then reduce cache size if appropriate */
            if ( cache_ptr->index_size < cache_ptr->max_cache_size ) {

                if ( (cache_ptr->resize_ctl).apply_empty_reserve ) {

                    test_size = (size_t)(((double)cache_ptr->index_size) /
                                (1 - (cache_ptr->resize_ctl).empty_reserve));

                    if ( test_size < cache_ptr->max_cache_size ) {

                        *status_ptr = decrease;
                        *new_max_cache_size_ptr = test_size;
                    }
                } else {

                    *status_ptr = decrease;
                    *new_max_cache_size_ptr = cache_ptr->index_size;
                }

                if ( *status_ptr == decrease ) {

                    /* clip to min size if necessary */
                    if ( *new_max_cache_size_ptr <
                         (cache_ptr->resize_ctl).min_size ) {

                        *new_max_cache_size_ptr =
                                (cache_ptr->resize_ctl).min_size;
                    }

                    /* clip to max decrement if necessary */
                    if ( ( (cache_ptr->resize_ctl).apply_max_decrement ) &&
                         ( ((cache_ptr->resize_ctl).max_decrement +
                            *new_max_cache_size_ptr) <
                           cache_ptr->max_cache_size ) ) {

                        *new_max_cache_size_ptr = cache_ptr->max_cache_size -
                                         (cache_ptr->resize_ctl).max_decrement;
                    }
                }
            }
        } else {

            *status_ptr = at_min_size;
        }
    }

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C__autoadjust__ageout() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5C__autoadjust__ageout__cycle_epoch_marker
 *
 * Purpose:     Remove the oldest epoch marker from the LRU list,
 *		and reinsert it at the head of the LRU list.  Also
 *		remove the epoch marker's index from the head of the
 *		ring buffer, and re-insert it at the tail of the ring
 *		buffer.
 *
 * Return:      SUCCEED on success/FAIL on failure.
 *
 * Programmer:  John Mainzer, 11/22/04
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5C__autoadjust__ageout__cycle_epoch_marker(H5C_t * cache_ptr)
{
    herr_t                      ret_value = SUCCEED;      /* Return value */
    int i;

    FUNC_ENTER_NOAPI_NOINIT

    HDassert( cache_ptr );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );

    if ( cache_ptr->epoch_markers_active <= 0 ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                    "No active epoch markers on entry?!?!?.")
    }

    /* remove the last marker from both the ring buffer and the LRU list */

    i = cache_ptr->epoch_marker_ringbuf[cache_ptr->epoch_marker_ringbuf_first];

    cache_ptr->epoch_marker_ringbuf_first =
            (cache_ptr->epoch_marker_ringbuf_first + 1) %
            (H5C__MAX_EPOCH_MARKERS + 1);

    cache_ptr->epoch_marker_ringbuf_size -= 1;

    if ( cache_ptr->epoch_marker_ringbuf_size < 0 ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "ring buffer underflow.")
    }

    if ( (cache_ptr->epoch_marker_active)[i] != TRUE ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "unused marker in LRU?!?")
    }

    H5C__DLL_REMOVE((&((cache_ptr->epoch_markers)[i])), \
                    (cache_ptr)->LRU_head_ptr, \
                    (cache_ptr)->LRU_tail_ptr, \
                    (cache_ptr)->LRU_list_len, \
                    (cache_ptr)->LRU_list_size, \
                    (FAIL))

    /* now, re-insert it at the head of the LRU list, and at the tail of
     * the ring buffer.
     */

    HDassert( ((cache_ptr->epoch_markers)[i]).addr == (haddr_t)i );
    HDassert( ((cache_ptr->epoch_markers)[i]).next == NULL );
    HDassert( ((cache_ptr->epoch_markers)[i]).prev == NULL );

    cache_ptr->epoch_marker_ringbuf_last =
        (cache_ptr->epoch_marker_ringbuf_last + 1) %
        (H5C__MAX_EPOCH_MARKERS + 1);

    (cache_ptr->epoch_marker_ringbuf)[cache_ptr->epoch_marker_ringbuf_last] = i;

    cache_ptr->epoch_marker_ringbuf_size += 1;

    if ( cache_ptr->epoch_marker_ringbuf_size > H5C__MAX_EPOCH_MARKERS ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "ring buffer overflow.")
    }

    H5C__DLL_PREPEND((&((cache_ptr->epoch_markers)[i])), \
                     (cache_ptr)->LRU_head_ptr, \
                     (cache_ptr)->LRU_tail_ptr, \
                     (cache_ptr)->LRU_list_len, \
                     (cache_ptr)->LRU_list_size, \
                     (FAIL))
done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C__autoadjust__ageout__cycle_epoch_marker() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5C__autoadjust__ageout__evict_aged_out_entries
 *
 * Purpose:     Evict clean entries in the cache that haven't
 *		been accessed for at least
 *              (cache_ptr->resize_ctl).epochs_before_eviction epochs,
 *      	and flush dirty entries that haven't been accessed for
 *		that amount of time.
 *
 *		Depending on configuration, the function will either
 *		flush or evict all such entries, or all such entries it
 *		encounters until it has freed the maximum amount of space
 *		allowed under the maximum decrement.
 *
 *		If we are running in parallel mode, writes may not be
 *		permitted.  If so, the function simply skips any dirty
 *		entries it may encounter.
 *
 *		The function makes no attempt to maintain the minimum
 *		clean size, as there is no guarantee that the cache size
 *		will be changed.
 *
 *		If there is no cache size change, the minimum clean size
 *		constraint will be met through a combination of clean
 *		entries and free space in the cache.
 *
 *		If there is a cache size reduction, the minimum clean size
 *		will be re-calculated, and will be enforced the next time
 *		we have to make space in the cache.
 *
 *              The primary_dxpl_id and secondary_dxpl_id parameters
 *              specify the dxpl_ids used depending on the value of
 *		*first_flush_ptr.  The idea is to use the primary_dxpl_id
 *		on the first write in a sequence of writes, and to use
 *		the secondary_dxpl_id on all subsequent writes.
 *
 *              This is useful in the metadata cache, but may not be
 *		needed elsewhere.  If so, just use the same dxpl_id for
 *		both parameters.
 *
 *              Observe that this function cannot occasion a read.
 *
 * Return:      Non-negative on success/Negative on failure.
 *
 * Programmer:  John Mainzer, 11/22/04
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5C__autoadjust__ageout__evict_aged_out_entries(H5F_t * f,
                                                hid_t   primary_dxpl_id,
                                                hid_t   secondary_dxpl_id,
                                                hbool_t write_permitted,
                                                hbool_t * first_flush_ptr)
{
    H5C_t *		cache_ptr = f->shared->cache;
    herr_t              result;
    size_t		eviction_size_limit;
    size_t		bytes_evicted = 0;
    hbool_t		prev_is_dirty = FALSE;
    H5C_cache_entry_t * entry_ptr;
    H5C_cache_entry_t * next_ptr;
    H5C_cache_entry_t * prev_ptr;
    herr_t              ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HDassert( f );
    HDassert( cache_ptr );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );

    /* if there is a limit on the amount that the cache size can be decrease
     * in any one round of the cache size reduction algorithm, load that
     * limit into eviction_size_limit.  Otherwise, set eviction_size_limit
     * to the equivalent of infinity.  The current size of the index will
     * do nicely.
     */
    if ( (cache_ptr->resize_ctl).apply_max_decrement ) {

        eviction_size_limit = (cache_ptr->resize_ctl).max_decrement;

    } else {

        eviction_size_limit = cache_ptr->index_size; /* i.e. infinity */
    }

    if ( write_permitted ) {

        entry_ptr = cache_ptr->LRU_tail_ptr;

        while ( ( entry_ptr != NULL ) &&
                ( (entry_ptr->type)->id != H5C__EPOCH_MARKER_TYPE ) &&
                ( bytes_evicted < eviction_size_limit ) )
        {
            HDassert( ! (entry_ptr->is_protected) );

	    next_ptr = entry_ptr->next;
            prev_ptr = entry_ptr->prev;

	    if ( prev_ptr != NULL ) {

                prev_is_dirty = prev_ptr->is_dirty;
            }

            if ( entry_ptr->is_dirty ) {

                result = H5C_flush_single_entry(f,
                                                primary_dxpl_id,
                                                secondary_dxpl_id,
                                                entry_ptr->type,
                                                entry_ptr->addr,
                                                H5C__NO_FLAGS_SET,
                                                first_flush_ptr,
                                                FALSE);
            } else {

                bytes_evicted += entry_ptr->size;

                result = H5C_flush_single_entry(f,
                                                primary_dxpl_id,
                                                secondary_dxpl_id,
                                                entry_ptr->type,
                                                entry_ptr->addr,
                                                H5C__FLUSH_INVALIDATE_FLAG,
                                                first_flush_ptr,
                                                TRUE);
            }

            if ( result < 0 ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, \
                            "unable to flush entry")
            }

            if ( prev_ptr != NULL ) {
#ifndef NDEBUG
                if ( prev_ptr->magic != H5C__H5C_CACHE_ENTRY_T_MAGIC ) {

                    /* something horrible has happened to *prev_ptr --
                     * scream and die.
                     */
                    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                                "*prev_ptr corrupt")

                } else
#endif /* NDEBUG */
		if ( ( prev_ptr->is_dirty != prev_is_dirty )
                         ||
                         ( prev_ptr->next != next_ptr )
                         ||
                         ( prev_ptr->is_protected )
                         ||
                         ( prev_ptr->is_pinned ) ) {

                    /* something has happened to the LRU -- start over
		     * from the tail.
                     */
                    entry_ptr = cache_ptr->LRU_tail_ptr;

                } else {

                    entry_ptr = prev_ptr;

                }
	    } else {

		entry_ptr = NULL;

	    }
        } /* end while */

        /* for now at least, don't bother to maintain the minimum clean size,
         * as the cache should now be less than its maximum size.  Due to
         * the vaguries of the cache size reduction algorthim, we may not
         * reduce the size of the cache.
         *
         * If we do, we will calculate a new minimum clean size, which will
         * be enforced the next time we try to make space in the cache.
         *
         * If we don't, no action is necessary, as we have just evicted and/or
         * or flushed a bunch of entries and therefore the sum of the clean
         * and free space in the cache must be greater than or equal to the
         * min clean space requirement (assuming that requirement was met on
         * entry).
         */

    } else /* ! write_permitted */  {

        /* since we are not allowed to write, all we can do is evict
         * any clean entries that we may encounter before we either
         * hit the eviction size limit, or encounter the epoch marker.
         *
         * If we are operating read only, this isn't an issue, as there
         * will not be any dirty entries.
         *
         * If we are operating in R/W mode, all the dirty entries we
         * skip will be flushed the next time we attempt to make space
         * when writes are permitted.  This may have some local
         * performance implications, but it shouldn't cause any net
         * slowdown.
         */

        HDassert( H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS );

        entry_ptr = cache_ptr->LRU_tail_ptr;

        while ( ( entry_ptr != NULL ) &&
                ( (entry_ptr->type)->id != H5C__EPOCH_MARKER_TYPE ) &&
                ( bytes_evicted < eviction_size_limit ) )
        {
            HDassert( ! (entry_ptr->is_protected) );

            prev_ptr = entry_ptr->prev;

            if ( ! (entry_ptr->is_dirty) ) {

                result = H5C_flush_single_entry(f,
                                                primary_dxpl_id,
                                                secondary_dxpl_id,
                                                entry_ptr->type,
                                                entry_ptr->addr,
                                                H5C__FLUSH_INVALIDATE_FLAG,
                                                first_flush_ptr,
                                                TRUE);

                if ( result < 0 ) {

                    HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, \
                                "unable to flush clean entry")
                }
            }
            /* just skip the entry if it is dirty, as we can't do
             * anything with it now since we can't write.
             */

            entry_ptr = prev_ptr;

        } /* end while */
    }

    if ( cache_ptr->index_size < cache_ptr->max_cache_size ) {

        cache_ptr->cache_full = FALSE;
    }

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C__autoadjust__ageout__evict_aged_out_entries() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5C__autoadjust__ageout__insert_new_marker
 *
 * Purpose:     Find an unused marker cache entry, mark it as used, and
 *		insert it at the head of the LRU list.  Also add the
 *		marker's index in the epoch_markers array.
 *
 * Return:      SUCCEED on success/FAIL on failure.
 *
 * Programmer:  John Mainzer, 11/19/04
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5C__autoadjust__ageout__insert_new_marker(H5C_t * cache_ptr)
{
    herr_t                      ret_value = SUCCEED;      /* Return value */
    int i;

    FUNC_ENTER_NOAPI_NOINIT

    HDassert( cache_ptr );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );

    if ( cache_ptr->epoch_markers_active >=
         (cache_ptr->resize_ctl).epochs_before_eviction ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                    "Already have a full complement of markers.")
    }

    /* find an unused marker */
    i = 0;
    while ( ( (cache_ptr->epoch_marker_active)[i] ) &&
            ( i < H5C__MAX_EPOCH_MARKERS ) )
    {
        i++;
    }

    if(i >= H5C__MAX_EPOCH_MARKERS)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Can't find unused marker.")

    HDassert( ((cache_ptr->epoch_markers)[i]).addr == (haddr_t)i );
    HDassert( ((cache_ptr->epoch_markers)[i]).next == NULL );
    HDassert( ((cache_ptr->epoch_markers)[i]).prev == NULL );

    (cache_ptr->epoch_marker_active)[i] = TRUE;

    cache_ptr->epoch_marker_ringbuf_last =
        (cache_ptr->epoch_marker_ringbuf_last + 1) %
        (H5C__MAX_EPOCH_MARKERS + 1);

    (cache_ptr->epoch_marker_ringbuf)[cache_ptr->epoch_marker_ringbuf_last] = i;

    cache_ptr->epoch_marker_ringbuf_size += 1;

    if ( cache_ptr->epoch_marker_ringbuf_size > H5C__MAX_EPOCH_MARKERS ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "ring buffer overflow.")
    }

    H5C__DLL_PREPEND((&((cache_ptr->epoch_markers)[i])), \
                     (cache_ptr)->LRU_head_ptr, \
                     (cache_ptr)->LRU_tail_ptr, \
                     (cache_ptr)->LRU_list_len, \
                     (cache_ptr)->LRU_list_size, \
                     (FAIL))

    cache_ptr->epoch_markers_active += 1;

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C__autoadjust__ageout__insert_new_marker() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5C__autoadjust__ageout__remove_all_markers
 *
 * Purpose:     Remove all epoch markers from the LRU list and mark them
 *		as inactive.
 *
 * Return:      SUCCEED on success/FAIL on failure.
 *
 * Programmer:  John Mainzer, 11/22/04
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5C__autoadjust__ageout__remove_all_markers(H5C_t * cache_ptr)
{
    herr_t                      ret_value = SUCCEED;      /* Return value */
    int i;
    int ring_buf_index;

    FUNC_ENTER_NOAPI_NOINIT

    HDassert( cache_ptr );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );

    while ( cache_ptr->epoch_markers_active > 0 )
    {
        /* get the index of the last epoch marker in the LRU list
         * and remove it from the ring buffer.
         */

        ring_buf_index = cache_ptr->epoch_marker_ringbuf_first;
        i = (cache_ptr->epoch_marker_ringbuf)[ring_buf_index];

        cache_ptr->epoch_marker_ringbuf_first =
            (cache_ptr->epoch_marker_ringbuf_first + 1) %
            (H5C__MAX_EPOCH_MARKERS + 1);

        cache_ptr->epoch_marker_ringbuf_size -= 1;

        if ( cache_ptr->epoch_marker_ringbuf_size < 0 ) {

            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "ring buffer underflow.")
        }

        if ( (cache_ptr->epoch_marker_active)[i] != TRUE ) {

            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "unused marker in LRU?!?")
        }

        /* remove the epoch marker from the LRU list */
        H5C__DLL_REMOVE((&((cache_ptr->epoch_markers)[i])), \
                        (cache_ptr)->LRU_head_ptr, \
                        (cache_ptr)->LRU_tail_ptr, \
                        (cache_ptr)->LRU_list_len, \
                        (cache_ptr)->LRU_list_size, \
                        (FAIL))

        /* mark the epoch marker as unused. */
        (cache_ptr->epoch_marker_active)[i] = FALSE;

        HDassert( ((cache_ptr->epoch_markers)[i]).addr == (haddr_t)i );
        HDassert( ((cache_ptr->epoch_markers)[i]).next == NULL );
        HDassert( ((cache_ptr->epoch_markers)[i]).prev == NULL );

        /* decrement the number of active epoch markers */
        cache_ptr->epoch_markers_active -= 1;

        HDassert( cache_ptr->epoch_markers_active == \
                  cache_ptr->epoch_marker_ringbuf_size );
    }

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C__autoadjust__ageout__remove_all_markers() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5C__autoadjust__ageout__remove_excess_markers
 *
 * Purpose:     Remove epoch markers from the end of the LRU list and
 *		mark them as inactive until the number of active markers
 *		equals the the current value of
 *		(cache_ptr->resize_ctl).epochs_before_eviction.
 *
 * Return:      SUCCEED on success/FAIL on failure.
 *
 * Programmer:  John Mainzer, 11/19/04
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5C__autoadjust__ageout__remove_excess_markers(H5C_t * cache_ptr)
{
    herr_t	ret_value = SUCCEED;      /* Return value */
    int		i;
    int		ring_buf_index;

    FUNC_ENTER_NOAPI_NOINIT

    HDassert( cache_ptr );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );

    if ( cache_ptr->epoch_markers_active <=
         (cache_ptr->resize_ctl).epochs_before_eviction ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "no excess markers on entry.")
    }

    while ( cache_ptr->epoch_markers_active >
            (cache_ptr->resize_ctl).epochs_before_eviction )
    {
        /* get the index of the last epoch marker in the LRU list
         * and remove it from the ring buffer.
         */

        ring_buf_index = cache_ptr->epoch_marker_ringbuf_first;
        i = (cache_ptr->epoch_marker_ringbuf)[ring_buf_index];

        cache_ptr->epoch_marker_ringbuf_first =
            (cache_ptr->epoch_marker_ringbuf_first + 1) %
            (H5C__MAX_EPOCH_MARKERS + 1);

        cache_ptr->epoch_marker_ringbuf_size -= 1;

        if ( cache_ptr->epoch_marker_ringbuf_size < 0 ) {

            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "ring buffer underflow.")
        }

        if ( (cache_ptr->epoch_marker_active)[i] != TRUE ) {

            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "unused marker in LRU?!?")
        }

        /* remove the epoch marker from the LRU list */
        H5C__DLL_REMOVE((&((cache_ptr->epoch_markers)[i])), \
                        (cache_ptr)->LRU_head_ptr, \
                        (cache_ptr)->LRU_tail_ptr, \
                        (cache_ptr)->LRU_list_len, \
                        (cache_ptr)->LRU_list_size, \
                        (FAIL))

        /* mark the epoch marker as unused. */
        (cache_ptr->epoch_marker_active)[i] = FALSE;

        HDassert( ((cache_ptr->epoch_markers)[i]).addr == (haddr_t)i );
        HDassert( ((cache_ptr->epoch_markers)[i]).next == NULL );
        HDassert( ((cache_ptr->epoch_markers)[i]).prev == NULL );

        /* decrement the number of active epoch markers */
        cache_ptr->epoch_markers_active -= 1;

        HDassert( cache_ptr->epoch_markers_active == \
                  cache_ptr->epoch_marker_ringbuf_size );
    }

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C__autoadjust__ageout__remove_excess_markers() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5C__flash_increase_cache_size
 *
 * Purpose:     If there is not at least new_entry_size - old_entry_size
 *              bytes of free space in the cache and the current
 *              max_cache_size is less than (cache_ptr->resize_ctl).max_size,
 *              perform a flash increase in the cache size and then reset
 *              the full cache hit rate statistics, and exit.
 *
 * Return:      Non-negative on success/Negative on failure.
 *
 * Programmer:  John Mainzer, 12/31/07
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5C__flash_increase_cache_size(H5C_t * cache_ptr,
                               size_t old_entry_size,
                               size_t new_entry_size)
{
    size_t                     new_max_cache_size = 0;
    size_t                     old_max_cache_size = 0;
    size_t                     new_min_clean_size = 0;
    size_t                     old_min_clean_size = 0;
    size_t                     space_needed;
    enum H5C_resize_status     status = flash_increase;  /* may change */
    double                     hit_rate;
    herr_t                     ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HDassert( cache_ptr );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );
    HDassert( cache_ptr->flash_size_increase_possible );
    HDassert( new_entry_size > cache_ptr->flash_size_increase_threshold );
    HDassert( old_entry_size < new_entry_size );

    if ( old_entry_size >= new_entry_size ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                    "old_entry_size >= new_entry_size")
    }

    space_needed = new_entry_size - old_entry_size;

    if ( ( (cache_ptr->index_size + space_needed) >
                            cache_ptr->max_cache_size ) &&
         ( cache_ptr->max_cache_size < (cache_ptr->resize_ctl).max_size ) ) {

        /* we have work to do */

        switch ( (cache_ptr->resize_ctl).flash_incr_mode )
        {
            case H5C_flash_incr__off:
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                   "flash_size_increase_possible but H5C_flash_incr__off?!")
                break;

            case H5C_flash_incr__add_space:
                if ( cache_ptr->index_size < cache_ptr->max_cache_size ) {

                    HDassert( (cache_ptr->max_cache_size - cache_ptr->index_size)
                               < space_needed );
                    space_needed -= cache_ptr->max_cache_size -
			            cache_ptr->index_size;
                }
                space_needed =
                    (size_t)(((double)space_needed) *
                             (cache_ptr->resize_ctl).flash_multiple);

                new_max_cache_size = cache_ptr->max_cache_size + space_needed;

                break;

            default: /* should be unreachable */
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                            "Unknown flash_incr_mode?!?!?.")
                break;
        }

        if ( new_max_cache_size > (cache_ptr->resize_ctl).max_size ) {

            new_max_cache_size = (cache_ptr->resize_ctl).max_size;
        }

        HDassert( new_max_cache_size > cache_ptr->max_cache_size );

        new_min_clean_size = (size_t)
                             ((double)new_max_cache_size *
                              ((cache_ptr->resize_ctl).min_clean_fraction));

        HDassert( new_min_clean_size <= new_max_cache_size );

        old_max_cache_size = cache_ptr->max_cache_size;
        old_min_clean_size = cache_ptr->min_clean_size;

        cache_ptr->max_cache_size = new_max_cache_size;
        cache_ptr->min_clean_size = new_min_clean_size;

        /* update flash cache size increase fields as appropriate */
        HDassert ( cache_ptr->flash_size_increase_possible );

        switch ( (cache_ptr->resize_ctl).flash_incr_mode )
        {
            case H5C_flash_incr__off:
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                    "flash_size_increase_possible but H5C_flash_incr__off?!")
                break;

            case H5C_flash_incr__add_space:
                cache_ptr->flash_size_increase_threshold =
                    (size_t)
                    (((double)(cache_ptr->max_cache_size)) *
                     ((cache_ptr->resize_ctl).flash_threshold));
                break;

            default: /* should be unreachable */
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                            "Unknown flash_incr_mode?!?!?.")
                break;
        }

        /* note that we don't cycle the epoch markers.  We can
	 * argue either way as to whether we should, but for now
	 * we don't.
	 */

        if ( (cache_ptr->resize_ctl).rpt_fcn != NULL ) {

            /* get the hit rate for the reporting function.  Should still
             * be good as we havent reset the hit rate statistics.
             */
            if ( H5C_get_cache_hit_rate(cache_ptr, &hit_rate) != SUCCEED ) {

                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Can't get hit rate.")
            }

            (*((cache_ptr->resize_ctl).rpt_fcn))
                (cache_ptr,
                 H5C__CURR_AUTO_RESIZE_RPT_FCN_VER,
                 hit_rate,
                 status,
                 old_max_cache_size,
                 new_max_cache_size,
                 old_min_clean_size,
                 new_min_clean_size);
        }

        if ( H5C_reset_cache_hit_rate_stats(cache_ptr) != SUCCEED ) {

            /* this should be impossible... */
            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                        "H5C_reset_cache_hit_rate_stats failed.")
        }
    }

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C__flash_increase_cache_size() */


/*-------------------------------------------------------------------------
 * Function:    H5C_flush_invalidate_cache
 *
 * Purpose:	Flush and destroy the entries contained in the target
 *		cache.
 *
 *		If the cache contains protected entries, the function will
 *		fail, as protected entries cannot be either flushed or
 *		destroyed.  However all unprotected entries should be
 *		flushed and destroyed before the function returns failure.
 *
 *		While pinned entries can usually be flushed, they cannot
 *		be destroyed.  However, they should be unpinned when all
 *		the entries that reference them have been destroyed (thus
 *		reduding the pinned entry's reference count to 0, allowing
 *		it to be unpinned).
 *
 *		If pinned entries are present, the function makes repeated
 *		passes through the cache, flushing all dirty entries
 *		(including the pinned dirty entries where permitted) and
 *		destroying all unpinned entries.  This process is repeated
 *		until either the cache is empty, or the number of pinned
 *		entries stops decreasing on each pass.
 *
 *		The primary_dxpl_id and secondary_dxpl_id parameters
 *		specify the dxpl_ids used on the first write occasioned
 *		by the flush (primary_dxpl_id), and on all subsequent
 *		writes (secondary_dxpl_id).
 *
 * Return:      Non-negative on success/Negative on failure or if there was
 *		a request to flush all items and something was protected.
 *
 * Programmer:  John Mainzer
 *		3/24/065
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5C_flush_invalidate_cache(H5F_t * f,
                           hid_t    primary_dxpl_id,
                           hid_t    secondary_dxpl_id,
			   unsigned flags)
{
    H5C_t *		cache_ptr = f->shared->cache;
    herr_t              status;
    hbool_t		first_flush = TRUE;
    int32_t		protected_entries = 0;
    int32_t		i;
    int32_t		cur_pel_len;
    int32_t		old_pel_len;
    int32_t		passes = 0;
    unsigned		cooked_flags;
    H5SL_node_t * 	node_ptr = NULL;
    H5C_cache_entry_t *	entry_ptr = NULL;
    H5C_cache_entry_t *	next_entry_ptr = NULL;
#if H5C_DO_SANITY_CHECKS
    int64_t		actual_slist_len = 0;
    int64_t		initial_slist_len = 0;
    size_t              actual_slist_size = 0;
    size_t              initial_slist_size = 0;
#endif /* H5C_DO_SANITY_CHECKS */
    herr_t		ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(FAIL)

    HDassert( f );
    HDassert( cache_ptr );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );
    HDassert( cache_ptr->slist_ptr );

    /* Filter out the flags that are not relevant to the flush/invalidate.
     * At present, only the H5C__FLUSH_CLEAR_ONLY_FLAG is kept.
     */
    cooked_flags = flags & H5C__FLUSH_CLEAR_ONLY_FLAG;

    /* remove ageout markers if present */
    if ( cache_ptr->epoch_markers_active > 0 ) {

        status = H5C__autoadjust__ageout__remove_all_markers(cache_ptr);

        if ( status != SUCCEED ) {

            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                        "error removing all epoch markers.")
        }
    }

    /* The flush proceedure here is a bit strange.
     *
     * In the outer while loop we make at least one pass through the
     * cache, and then repeat until either all the pinned entries
     * unpin themselves, or until the number of pinned entries stops
     * declining.  In this later case, we scream and die.
     *
     * Since the fractal heap can dirty, resize, and/or move entries
     * in is flush callback, it is possible that the cache will still
     * contain dirty entries at this point.  If so, we must make up to
     * H5C__MAX_PASSES_ON_FLUSH more passes through the skip list
     * to allow it to empty.  If is is not empty at this point, we again
     * scream and die.
     *
     * Further, since clean entries can be dirtied, resized, and/or moved
     * as the result of a flush call back (either the entries own, or that
     * for some other cache entry), we can no longer promise to flush
     * the cache entries in increasing address order.
     *
     * Instead, we just do the best we can -- making a pass through
     * the skip list, and then a pass through the "clean" entries, and
     * then repeating as needed.  Thus it is quite possible that an
     * entry will be evicted from the cache only to be re-loaded later
     * in the flush process (From what Quincey tells me, the pin
     * mechanism makes this impossible, but even it it is true now,
     * we shouldn't count on it in the future.)
     *
     * The bottom line is that entries will probably be flushed in close
     * to increasing address order, but there are no guarantees.
     */

    cur_pel_len = cache_ptr->pel_len;
    old_pel_len = cache_ptr->pel_len;

    while ( cache_ptr->index_len > 0 )
    {
	/* first, try to flush-destroy any dirty entries.   Do this by
	 * making a scan through the slist.  Note that new dirty entries
	 * may be created by the flush call backs.  Thus it is possible
	 * that the slist will not be empty after we finish the scan.
	 */

        if ( cache_ptr->slist_len == 0 ) {

            node_ptr = NULL;
            HDassert( cache_ptr->slist_size == 0 );

        } else {

            node_ptr = H5SL_first(cache_ptr->slist_ptr);

            if ( node_ptr == NULL ) {
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                            "slist_len != 0 && node_ptr == NULL");
            }

            next_entry_ptr = (H5C_cache_entry_t *)H5SL_item(node_ptr);

            if ( next_entry_ptr == NULL ) {
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                            "next_entry_ptr == NULL 1 ?!?!");
            }
#ifndef NDEBUG
	    HDassert( next_entry_ptr->magic == H5C__H5C_CACHE_ENTRY_T_MAGIC );
#endif /* NDEBUG */
            HDassert( next_entry_ptr->is_dirty );
            HDassert( next_entry_ptr->in_slist );

        }

#if H5C_DO_SANITY_CHECKS
        /* Depending on circumstances, H5C_flush_single_entry() will
         * remove dirty entries from the slist as it flushes them.
         * Thus for sanity checks we must make note of the initial
         * slist length and size before we do any flushes.
         */
        initial_slist_len = cache_ptr->slist_len;
        initial_slist_size = cache_ptr->slist_size;

        /* There is also the possibility that entries will be
         * dirtied, resized, and/or moved as the result of
         * calls to the flush callbacks.  We use the slist_len_increase
         * and slist_size_increase increase fields in struct H5C_t
         * to track these changes for purpose of sanity checking.
         * To this end, we must zero these fields before we start
         * the pass through the slist.
         */
        cache_ptr->slist_len_increase = 0;
        cache_ptr->slist_size_increase = 0;

	/* Finally, reset the actual_slist_len and actual_slist_size
	 * fields to zero, as these fields are used to accumulate
	 * the slist lenght and size that we see as we scan through
	 * the slist.
	 */
	actual_slist_len = 0;
	actual_slist_size = 0;
#endif /* H5C_DO_SANITY_CHECKS */

        while ( node_ptr != NULL )
        {
            entry_ptr = next_entry_ptr;

            /* With the advent of the fractal heap, it is possible
             * that the flush callback will dirty and/or resize
             * other entries in the cache.  In particular, while
             * Quincey has promised me that this will never happen,
             * it is possible that the flush callback for an
             * entry may protect an entry that is not in the cache,
             * perhaps causing the cache to flush and possibly
             * evict the entry associated with node_ptr to make
             * space for the new entry.
             *
             * Thus we do a bit of extra sanity checking on entry_ptr,
             * and break out of this scan of the skip list if we
             * detect major problems.  We have a bit of leaway on the
             * number of passes though the skip list, so this shouldn't
             * be an issue in the flush in and of itself, as it should
             * be all but impossible for this to happen more than once
             * in any flush.
             *
             * Observe that that breaking out of the scan early
             * shouldn't break the sanity checks just after the end
	     * of this while loop.
	     *
	     * If an entry has merely been marked clean and removed from
	     * the s-list, we simply break out of the scan.
	     *
	     * If the entry has been evicted, we flag an error and
	     * exit.
             */
#ifndef NDEBUG
	    if ( entry_ptr->magic != H5C__H5C_CACHE_ENTRY_T_MAGIC ) {

                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                            "entry_ptr->magic is invalid ?!?!");

	    } else
#endif /* NDEBUG */
	    if ( ( ! entry_ptr->is_dirty ) ||
                 ( ! entry_ptr->in_slist ) ) {

                /* the s-list has been modified out from under us.
	         * break out of the loop.
                 */
                break;
            }

            /* increment node pointer now, before we delete its target
             * from the slist.
             */
            node_ptr = H5SL_next(node_ptr);
            if ( node_ptr != NULL ) {

                next_entry_ptr = (H5C_cache_entry_t *)H5SL_item(node_ptr);
                if ( next_entry_ptr == NULL ) {
                    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                                "next_entry_ptr == NULL 2 ?!?!");
	        }
#ifndef NDEBUG
		HDassert( next_entry_ptr->magic ==
                          H5C__H5C_CACHE_ENTRY_T_MAGIC );
#endif /* NDEBUG */
                HDassert( next_entry_ptr->is_dirty );
                HDassert( next_entry_ptr->in_slist );

            } else {

                next_entry_ptr = NULL;
	    }

            /* Note that we now remove nodes from the slist as we flush
	     * the associated entries, instead of leaving them there
	     * until we are done, and then destroying all nodes in
             * the slist.
             *
             * While this optimization used to be easy, with the possibility
             * of new entries being added to the slist in the midst of the
             * flush, we must keep the slist in cannonical form at all
             * times.
             */

            HDassert( entry_ptr != NULL );
            HDassert( entry_ptr->in_slist );

#if H5C_DO_SANITY_CHECKS
            /* update actual_slist_len & actual_slist_size before
	     * the flush.  Note that the entry will be removed
	     * from the slist after the flush, and thus may be
	     * resized by the flush callback.  This is OK, as
	     * we will catch the size delta in
	     * cache_ptr->slist_size_increase.
	     *
	     * Note that we include pinned entries in this count, even
	     * though we will not actually flush them.
	     */
            actual_slist_len++;
            actual_slist_size += entry_ptr->size;
#endif /* H5C_DO_SANITY_CHECKS */

            if ( entry_ptr->is_protected ) {

                /* we have major problems -- but lets flush
                 * everything we can before we flag an error.
                 */
	        protected_entries++;

            } else if ( entry_ptr->is_pinned ) {

		/* Test to see if we are can flush the entry now.
                 * If we can, go ahead and flush, but don't tell
                 * H5C_flush_single_entry() to destroy the entry
                 * as pinned entries can't be evicted.
                 */
		if ( TRUE ) { /* When we get to multithreaded cache,
			       * we will need either locking code, and/or
			       * a test to see if the entry is in flushable
			       * condition here.
			       */

                    status = H5C_flush_single_entry(f,
                                                    primary_dxpl_id,
                                                    secondary_dxpl_id,
                                                    NULL,
                                                    entry_ptr->addr,
                                                    H5C__NO_FLAGS_SET,
                                                    &first_flush,
                                                    FALSE);
                    if ( status < 0 ) {

                        /* This shouldn't happen -- if it does, we are toast
                         * so just scream and die.
                         */

                        HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, \
                                    "dirty pinned entry flush failed.")
                    }
                }
            } else {

                status = H5C_flush_single_entry(f,
                                                primary_dxpl_id,
                                                secondary_dxpl_id,
                                                NULL,
                                                entry_ptr->addr,
                                                (cooked_flags | H5C__FLUSH_INVALIDATE_FLAG),
                                                &first_flush,
                                                TRUE);
                if ( status < 0 ) {

                    /* This shouldn't happen -- if it does, we are toast so
                     * just scream and die.
                     */

                    HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, \
                                "dirty entry flush destroy failed.")
                }
            }
        } /* end while loop scanning skip list */

#if H5C_DO_SANITY_CHECKS
	/* It is possible that entries were added to the slist during
	 * the scan, either before or after scan pointer.  The following
	 * asserts take this into account.
         *
         * Don't bother with the sanity checks if node_ptr != NULL, as
         * in this case we broke out of the loop because it got changed
         * out from under us.
         */

	if ( node_ptr == NULL ) {

            HDassert( (actual_slist_len + cache_ptr->slist_len) ==
		      (initial_slist_len + cache_ptr->slist_len_increase) );
            HDassert( (actual_slist_size + cache_ptr->slist_size) ==
		      (initial_slist_size + cache_ptr->slist_size_increase) );
	}
#endif /* H5C_DO_SANITY_CHECKS */

        /* Since we are doing a destroy, we must make a pass through
         * the hash table and try to flush - destroy all entries that
         * remain.
	 *
	 * It used to be that all entries remaining in the cache at
	 * this point had to be clean, but with the fractal heap mods
	 * this may not be the case.  If so, we will flush entries out
	 * of increasing address order.
	 *
	 * Writes to disk are possible here.
         */
        for ( i = 0; i < H5C__HASH_TABLE_LEN; i++ )
        {
	    next_entry_ptr = cache_ptr->index[i];

            while ( next_entry_ptr != NULL )
            {
                entry_ptr = next_entry_ptr;

                next_entry_ptr = entry_ptr->ht_next;
#ifndef NDEBUG
		HDassert ( ( next_entry_ptr == NULL ) ||
                           ( next_entry_ptr->magic ==
                             H5C__H5C_CACHE_ENTRY_T_MAGIC ) );
#endif /* NDEBUG */
                if ( entry_ptr->is_protected ) {
#ifndef NDEBUG
                    HDassert( entry_ptr->magic == H5C__H5C_CACHE_ENTRY_T_MAGIC );
#endif /* NDEBUG */

                    /* we have major problems -- but lets flush and destroy
                     * everything we can before we flag an error.
                     */
	            protected_entries++;

                    if ( ! entry_ptr->in_slist ) {

                        HDassert( !(entry_ptr->is_dirty) );
                    }
                } else if ( ! ( entry_ptr->is_pinned ) ) {

                    status = H5C_flush_single_entry(f,
                                                    primary_dxpl_id,
                                                    secondary_dxpl_id,
                                                    NULL,
                                                    entry_ptr->addr,
                                                    (cooked_flags | H5C__FLUSH_INVALIDATE_FLAG),
                                                    &first_flush,
                                                    TRUE);
                    if ( status < 0 ) {

                        /* This shouldn't happen -- if it does, we are toast so
                         * just scream and die.
                         */

                        HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, \
                                    "Entry flush destroy failed.")
                    }
                }
	        /* We can't do anything if the entry is pinned.  The
		 * hope is that the entry will be unpinned as the
		 * result of destroys of entries that reference it.
		 *
		 * We detect this by noting the change in the number
		 * of pinned entries from pass to pass.  If it stops
		 * shrinking before it hits zero, we scream and die.
		 */
                /* if the flush function on the entry we last evicted
                 * loaded an entry into cache (as Quincey has promised me
                 * it never will), and if the cache was full, it is
                 * possible that *next_entry_ptr was flushed or evicted.
                 *
                 * Test to see if this happened here.  Note that if this
		 * test is triggred, we are accessing a deallocated piece
		 * of dynamically allocated memory, so we just scream and
		 * die.
                 */
#ifndef NDEBUG
                if ( ( next_entry_ptr != NULL ) &&
                     ( next_entry_ptr->magic !=
                       H5C__H5C_CACHE_ENTRY_T_MAGIC ) ) {

                    /* Something horrible has happened to
                     * *next_entry_ptr -- scream and die.
                     */
                    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                                "next_entry_ptr->magic is invalid?!?!?.")
                }
#endif /* NDEBUG */
            } /* end while loop scanning hash table bin */
        } /* end for loop scanning hash table */

	old_pel_len = cur_pel_len;
	cur_pel_len = cache_ptr->pel_len;

	if ( ( cur_pel_len > 0 ) && ( cur_pel_len >= old_pel_len ) ) {

	   /* The number of pinned entries is positive, and it is not
	    * declining.  Scream and die.
	    */

            HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, \
                        "Pinned entry count not decreasing, cur_pel_len = %d, old_pel_len = %d", (int)cur_pel_len, (int)old_pel_len)

        } else if ( ( cur_pel_len == 0 ) && ( old_pel_len == 0 ) ) {

	    /* increment the pass count */
	    passes++;
	}

	if ( passes >= H5C__MAX_PASSES_ON_FLUSH ) {

	    /* we have exceeded the maximum number of passes through the
	     * cache to flush and destroy all entries.  Scream and die.
	     */

            HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, \
	                "Maximum passes on flush exceeded.")
	}
    } /* main while loop */

    /* Invariants, after destroying all entries in the hash table */
    HDassert( cache_ptr->index_size == 0 );
    HDassert( cache_ptr->clean_index_size == 0 );
    HDassert( cache_ptr->dirty_index_size == 0 );
    HDassert( cache_ptr->slist_len == 0 );
    HDassert( cache_ptr->slist_size == 0 );
    HDassert( cache_ptr->pel_len == 0 );
    HDassert( cache_ptr->pel_size == 0 );
    HDassert( cache_ptr->pl_len == 0 );
    HDassert( cache_ptr->pl_size == 0 );
    HDassert( cache_ptr->LRU_list_len == 0 );
    HDassert( cache_ptr->LRU_list_size == 0 );


    HDassert( protected_entries <= cache_ptr->pl_len );

    if ( protected_entries > 0 ) {

        HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, \
	            "Cache has protected entries.")

    } else if ( cur_pel_len > 0 ) {

        HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, \
	            "Can't unpin all pinned entries.")

    }

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C_flush_invalidate_cache() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5C_flush_single_entry
 *
 * Purpose:     Flush or clear (and evict if requested) the cache entry
 *		with the specified address and type.  If the type is NULL,
 *		any unprotected entry at the specified address will be
 *		flushed (and possibly evicted).
 *
 *		Attempts to flush a protected entry will result in an
 *		error.
 *
 *		*first_flush_ptr should be true if only one
 *		flush is contemplated before the next load, or if this
 *		is the first of a sequence of flushes that will be
 *		completed before the next load.  *first_flush_ptr is set
 *		to false if a flush actually takes place, and should be
 *		left false until the end of the sequence.
 *
 *		The primary_dxpl_id is used if *first_flush_ptr is TRUE
 *		on entry, and a flush actually takes place.  The
 *		secondary_dxpl_id is used in any subsequent flush where
 *		*first_flush_ptr is FALSE on entry.
 *
 *		If the H5C__FLUSH_INVALIDATE_FLAG flag is set, the entry will
 *		be cleared and not flushed -- in the case *first_flush_ptr,
 *		primary_dxpl_id, and secondary_dxpl_id are all irrelevent,
 *		and the call can't be part of a sequence of flushes.
 *
 *		If the caller knows the address of the TBBT node at
 *		which the target entry resides, it can avoid a lookup
 *		by supplying that address in the tgt_node_ptr parameter.
 *		If this parameter is NULL, the function will do a TBBT
 *		search for the entry instead.
 *
 *		The function does nothing silently if there is no entry
 *		at the supplied address, or if the entry found has the
 *		wrong type.
 *
 * Return:      Non-negative on success/Negative on failure or if there was
 *		an attempt to flush a protected item.
 *
 * Programmer:  John Mainzer, 5/5/04
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5C_flush_single_entry(H5F_t *	   	   f,
                       hid_t 		   primary_dxpl_id,
                       hid_t 		   secondary_dxpl_id,
                       const H5C_class_t * type_ptr,
                       haddr_t		   addr,
                       unsigned	     	   flags,
                       hbool_t *	   first_flush_ptr,
                       hbool_t		   del_entry_from_slist_on_destroy)
{
    H5C_t *	     	cache_ptr = f->shared->cache;
    hbool_t		destroy;
    hbool_t		clear_only;
    hbool_t		take_ownership;
    hbool_t		was_dirty;
    hbool_t		destroy_entry;
    herr_t		status;
    int			type_id;
    unsigned		flush_flags = H5C_CALLBACK__NO_FLAGS_SET;
    H5C_cache_entry_t * entry_ptr = NULL;
    herr_t		ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HDassert( f );
    HDassert( cache_ptr );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );
    HDassert( H5F_addr_defined(addr) );
    HDassert( first_flush_ptr );

    destroy = ( (flags & H5C__FLUSH_INVALIDATE_FLAG) != 0 );
    clear_only = ( (flags & H5C__FLUSH_CLEAR_ONLY_FLAG) != 0);
    take_ownership = ( (flags & H5C__TAKE_OWNERSHIP_FLAG) != 0);

    /* Set the flag for destroying the entry, based on the 'take ownership'
     *  and 'destroy' flags
     */
    if(take_ownership)
        destroy_entry = FALSE;
    else
        destroy_entry = destroy;

    /* attempt to find the target entry in the hash table */
    H5C__SEARCH_INDEX(cache_ptr, addr, entry_ptr, FAIL)

#if H5C_DO_SANITY_CHECKS
    if ( entry_ptr != NULL ) {

        HDassert( ! ( ( destroy ) && ( entry_ptr->is_pinned ) ) );

        if ( entry_ptr->in_slist ) {

            if ( ( ( entry_ptr->flush_marker ) && ( ! entry_ptr->is_dirty ) ) ||
                 ( entry_ptr->addr != addr ) ) {

                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                            "entry in slist failed sanity checks.")
            }
        } else {

            if ( ( entry_ptr->is_dirty ) ||
                 ( entry_ptr->flush_marker ) ||
                 ( entry_ptr->addr != addr ) ) {

                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                            "entry failed sanity checks.")
            }
        }
    }
#endif /* H5C_DO_SANITY_CHECKS */

    if ( ( entry_ptr != NULL ) && ( entry_ptr->is_protected ) )
    {

        /* Attempt to flush a protected entry -- scream and die. */
        HGOTO_ERROR(H5E_CACHE, H5E_PROTECT, FAIL, \
                    "Attempt to flush a protected entry.")
    }

    if ( ( entry_ptr != NULL ) &&
         ( ( type_ptr == NULL ) || ( type_ptr->id == entry_ptr->type->id ) ) )
    {
        /* we have work to do */

	/* We will set flush_in_progress back to FALSE at the end if the
	 * entry still exists at that point.
	 */
	entry_ptr->flush_in_progress = TRUE;

#ifdef H5_HAVE_PARALLEL
#ifndef NDEBUG
        /* If MPI based VFD is used, do special parallel I/O sanity checks.
         * Note that we only do these sanity checks when the clear_only flag
         * is not set, and the entry to be flushed is dirty.  Don't bother
         * otherwise as no file I/O can result.
         */
        if(!clear_only && entry_ptr->is_dirty &&
                IS_H5FD_MPI(f)) {
            H5P_genplist_t *dxpl;       /* Dataset transfer property list */
            unsigned coll_meta;         /* Collective metadata write flag */

            /* Get the dataset transfer property list */
            if(NULL == (dxpl = H5I_object(primary_dxpl_id)))
                HGOTO_ERROR(H5E_CACHE, H5E_BADTYPE, FAIL, "not a dataset transfer property list")

            /* Get the collective metadata write property */
            if(H5P_get(dxpl, H5AC_COLLECTIVE_META_WRITE_NAME, &coll_meta) < 0)
                HGOTO_ERROR(H5E_CACHE, H5E_CANTGET, FAIL, "can't retrieve xfer mode")

            /* Sanity check collective metadata write flag */
            HDassert(coll_meta);
        } /* end if */
#endif /* NDEBUG */
#endif /* H5_HAVE_PARALLEL */

        was_dirty = entry_ptr->is_dirty;
        type_id = entry_ptr->type->id;

        entry_ptr->flush_marker = FALSE;

        if ( clear_only ) {
            H5C__UPDATE_STATS_FOR_CLEAR(cache_ptr, entry_ptr)
        } else {
            H5C__UPDATE_STATS_FOR_FLUSH(cache_ptr, entry_ptr)
        }

        if ( destroy ) {
            H5C__UPDATE_STATS_FOR_EVICTION(cache_ptr, entry_ptr)
        }

        /* Always remove the entry from the hash table on a destroy.  On a
         * flush with destroy, it is cheaper to discard the skip list all at
         * once rather than remove the entries one by one, so we only delete
         * from the slist only if requested.
         *
         * We must do deletions now as the callback routines will free the
         * entry if destroy is true.
	 *
	 * Note that it is possible that the entry will be moved during
	 * its call to flush.  This will upset H5C_move_entry() if we
	 * don't tell it that it doesn't have to worry about updating the
	 * index and SLIST.  Use the destroy_in_progress field for this
	 * purpose.
         */
        if ( destroy ) {

            entry_ptr->destroy_in_progress = TRUE;

            H5C__DELETE_FROM_INDEX(cache_ptr, entry_ptr)

            if ( ( entry_ptr->in_slist ) &&
                 ( del_entry_from_slist_on_destroy ) ) {

                H5C__REMOVE_ENTRY_FROM_SLIST(cache_ptr, entry_ptr)
            }
        }

        /* Update the replacement policy for the flush or eviction.
         * Again, do this now so we don't have to reference freed
         * memory in the destroy case.
         */
        if ( destroy ) { /* AKA eviction */

            H5C__UPDATE_RP_FOR_EVICTION(cache_ptr, entry_ptr, FAIL)

        } else {

            H5C__UPDATE_RP_FOR_FLUSH(cache_ptr, entry_ptr, FAIL)
        }

        /* Clear the dirty flag only, if requested */
        if ( clear_only ) {

	    if ( destroy ) {
#ifndef NDEBUG
		/* we are about to call the clear callback with the
		 * destroy flag set -- this will result in *entry_ptr
		 * being freed.  Set the magic field to bad magic
		 * so we can detect a freed cache entry if we see
		 * one.
		 */
		entry_ptr->magic = H5C__H5C_CACHE_ENTRY_T_BAD_MAGIC;
#endif /* NDEBUG */
                entry_ptr->cache_ptr = NULL;
	    }
            /* Call the callback routine to clear all dirty flags for object */
            if ( (entry_ptr->type->clear)(f, entry_ptr, destroy_entry) < 0 ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "can't clear entry")
            }
        } else {

#if H5C_DO_SANITY_CHECKS
            if ( ( entry_ptr->is_dirty ) &&
                 ( cache_ptr->check_write_permitted == NULL ) &&
                 ( ! (cache_ptr->write_permitted) ) ) {

                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                            "Write when writes are always forbidden!?!?!")
            }
#endif /* H5C_DO_SANITY_CHECKS */

	    if ( destroy ) {
#ifndef NDEBUG
	        /* we are about to call the flush callback with the
	         * destroy flag set -- this will result in *entry_ptr
	         * being freed.  Set the magic field to bad magic
	         * so we can detect a freed cache entry if we see
	         * one.
	         */
	        entry_ptr->magic = H5C__H5C_CACHE_ENTRY_T_BAD_MAGIC;
#endif /* NDEBUG */
                entry_ptr->cache_ptr = NULL;
	    }

            /* Only block for all the processes on the first piece of metadata
             */

            if ( *first_flush_ptr && entry_ptr->is_dirty ) {

                status = (entry_ptr->type->flush)(f, primary_dxpl_id, destroy_entry,
                                                 entry_ptr->addr, entry_ptr,
						 &flush_flags);
                *first_flush_ptr = FALSE;

            } else {

                status = (entry_ptr->type->flush)(f, secondary_dxpl_id,
                                                 destroy_entry, entry_ptr->addr,
                                                 entry_ptr, &flush_flags);
            }

            if ( status < 0 ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, \
                            "unable to flush entry")
            }

#ifdef H5_HAVE_PARALLEL
            if ( flush_flags != H5C_CALLBACK__NO_FLAGS_SET ) {

                /* In the parallel case, flush operations can
		 * cause problems.  If they occur, scream and
		 * die.
		 *
		 * At present, in the parallel case, the aux_ptr
		 * will only be set if there is more than one
		 * process.  Thus we can use this to detect
		 * the parallel case.
		 *
		 * This works for now, but if we start using the
		 * aux_ptr for other purposes, we will have to
		 * change this test accordingly.
		 *
		 * NB: While this test detects entryies that attempt
		 *     to resize or move themselves during a flush
		 *     in the parallel case, it will not detect an
		 *     entry that dirties, resizes, and/or moves
		 *     other entries during its flush.
		 *
		 *     From what Quincey tells me, this test is
		 *     sufficient for now, as any flush routine that
		 *     does the latter will also do the former.
		 *
		 *     If that ceases to be the case, further
		 *     tests will be necessary.
		 */
		if ( cache_ptr->aux_ptr != NULL ) {

                    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
		        "resize/move in serialize occured in parallel case.")

		}
	    }
#endif /* H5_HAVE_PARALLEL */
        }

        if ( ( ! destroy ) && ( entry_ptr->in_slist ) ) {

            H5C__REMOVE_ENTRY_FROM_SLIST(cache_ptr, entry_ptr)

        }

	if ( ( ! destroy ) && ( was_dirty ) ) {

            H5C__UPDATE_INDEX_FOR_ENTRY_CLEAN(cache_ptr, entry_ptr);
        }

        if ( ! destroy ) { /* i.e. if the entry still exists */

            HDassert( !(entry_ptr->is_dirty) );
            HDassert( !(entry_ptr->flush_marker) );
            HDassert( !(entry_ptr->in_slist) );
            HDassert( !(entry_ptr->is_protected) );
            HDassert( !(entry_ptr->is_read_only) );
            HDassert( (entry_ptr->ro_ref_count) == 0 );

	    if ( (flush_flags & H5C_CALLBACK__SIZE_CHANGED_FLAG) != 0 ) {

		/* The entry size changed as a result of the flush.
		 *
		 * Most likely, the entry was compressed, and the
		 * new version is of a different size than the old.
		 *
		 * In any case, we must update entry and cache size
		 * accordingly.
		 */
		size_t new_size;

                if ( (entry_ptr->type->size)(f, (void *)entry_ptr, &new_size)
                     < 0 ) {

                    HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGETSIZE, FAIL, \
                                "Can't get entry size after flush")
                }

		if ( new_size != entry_ptr->size ) {

                    HDassert( entry_ptr->size < H5C_MAX_ENTRY_SIZE );

                    /* update the hash table for the size change
		     * We pass TRUE as the was_clean parameter, as we
		     * have already updated the clean and dirty index
		     * size fields for the fact that the entry has
		     * been flushed. (See above call to
		     * H5C__UPDATE_INDEX_FOR_ENTRY_CLEAN()).
		     */
	            H5C__UPDATE_INDEX_FOR_SIZE_CHANGE((cache_ptr), \
				                      (entry_ptr->size), \
                                                      (new_size), \
						      (entry_ptr), \
						      (TRUE))

		    /* The entry can't be protected since we just flushed it.
		     * Thus we must update the replacement policy data
		     * structures for the size change.  The macro deals
		     * with the pinned case.
		     */
		    H5C__UPDATE_RP_FOR_SIZE_CHANGE(cache_ptr, entry_ptr, \
				                   new_size)

		    /* The entry can't be in the slist, so no need to update
		     * the slist for the size change.
		     */

		    /* update stats for the size change */
		    H5C__UPDATE_STATS_FOR_ENTRY_SIZE_CHANGE(cache_ptr, \
				                            entry_ptr, \
							    new_size)

		    /* finally, update the entry size proper */
		    entry_ptr->size = new_size;
		}
	    }

	    if ( (flush_flags & H5C_CALLBACK__MOVED_FLAG) != 0 ) {

		/* The entry was moved as the result of the flush.
		 *
		 * Most likely, the entry was compressed, and the
		 * new version is larger than the old and thus had
		 * to be relocated.
		 *
		 * At preset, all processing for this case is
		 * handled elsewhere.  But lets keep the if statement
		 * around just in case.
		 */

	    }

	    entry_ptr->flush_in_progress = FALSE;
        }

        if ( cache_ptr->log_flush ) {

            status = (cache_ptr->log_flush)(cache_ptr, addr, was_dirty,
                                            flags, type_id);

            if ( status < 0 ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, \
                            "log_flush callback failed.")
            }
        }
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5C_flush_single_entry() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5C_load_entry
 *
 * Purpose:     Attempt to load the entry at the specified disk address
 *		and with the specified type into memory.  If successful.
 *		return the in memory address of the entry.  Return NULL
 *		on failure.
 *
 *		Note that this function simply loads the entry into
 *		core.  It does not insert it into the cache.
 *
 * Return:      Non-NULL on success / NULL on failure.
 *
 * Programmer:  John Mainzer, 5/18/04
 *
 *		QAK -- 1/31/08
 *		Added initialization for the new free_file_space_on_destroy
 *		field.
 *
 *-------------------------------------------------------------------------
 */
static void *
H5C_load_entry(H5F_t *             f,
               hid_t               dxpl_id,
               const H5C_class_t * type,
               haddr_t             addr,
               void *              udata)
{
    void *		thing = NULL;   /* Pointer to thing loaded */
    H5C_cache_entry_t *	entry;          /* Alias for thing loaded, as cache entry */
    void *		ret_value;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(f);
    HDassert(f->shared);
    HDassert(f->shared->cache);
    HDassert(type);
    HDassert(type->load);
    HDassert(type->size);
    HDassert(H5F_addr_defined(addr));

    if(NULL == (thing = (type->load)(f, dxpl_id, addr, udata)))

        HGOTO_ERROR(H5E_CACHE, H5E_CANTLOAD, NULL, "unable to load entry")


    entry = (H5C_cache_entry_t *)thing;

    /* In general, an entry should be clean just after it is loaded.
     *
     * However, when this code is used in the metadata cache, it is
     * possible that object headers will be dirty at this point, as
     * the load function will alter object headers if necessary to
     * fix an old bug.
     *
     * To support this bug fix, I have replace the old assert:
     *
     * 	HDassert( entry->is_dirty == FALSE );
     *
     * with:
     *
     * 	HDassert( ( entry->is_dirty == FALSE ) || ( type->id == 5 ) );
     *
     * Note that type id 5 is associated with object headers in the metadata
     * cache.
     *
     * When we get to using H5C for other purposes, we may wish to
     * tighten up the assert so that the loophole only applies to the
     * metadata cache.
     */

    HDassert( ( entry->is_dirty == FALSE ) || ( type->id == 5 ) );
#ifndef NDEBUG
    entry->magic                = H5C__H5C_CACHE_ENTRY_T_MAGIC;
#endif /* NDEBUG */
    entry->cache_ptr            = f->shared->cache;
    entry->addr                 = addr;
    entry->type                 = type;
    entry->is_protected         = FALSE;
    entry->is_read_only         = FALSE;
    entry->ro_ref_count         = 0;
    entry->in_slist             = FALSE;
    entry->flush_marker         = FALSE;
#ifdef H5_HAVE_PARALLEL
    entry->clear_on_unprotect   = FALSE;
    entry->flush_immediately    = FALSE;
#endif /* H5_HAVE_PARALLEL */
    entry->flush_in_progress    = FALSE;
    entry->destroy_in_progress  = FALSE;
    entry->free_file_space_on_destroy = FALSE;

    if((type->size)(f, thing, &(entry->size)) < 0)

        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGETSIZE, NULL, "Can't get size of thing")

    HDassert( entry->size < H5C_MAX_ENTRY_SIZE );

    entry->next                 = NULL;
    entry->prev                 = NULL;

    entry->aux_next             = NULL;
    entry->aux_prev             = NULL;

    H5C__RESET_CACHE_ENTRY_STATS(entry);

    ret_value = thing;

done:

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5C_load_entry() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5C_make_space_in_cache
 *
 * Purpose:     Attempt to evict cache entries until the index_size
 *		is at least needed_space below max_cache_size.
 *
 *		In passing, also attempt to bring cLRU_list_size to a
 *		value greater than min_clean_size.
 *
 *		Depending on circumstances, both of these goals may
 *		be impossible, as in parallel mode, we must avoid generating
 *		a write as part of a read (to avoid deadlock in collective
 *		I/O), and in all cases, it is possible (though hopefully
 *		highly unlikely) that the protected list may exceed the
 *		maximum size of the cache.
 *
 *		Thus the function simply does its best, returning success
 *		unless an error is encountered.
 *
 *		The primary_dxpl_id and secondary_dxpl_id parameters
 *		specify the dxpl_ids used on the first write occasioned
 *		by the call (primary_dxpl_id), and on all subsequent
 *		writes (secondary_dxpl_id).  This is useful in the metadata
 *		cache, but may not be needed elsewhere.  If so, just use the
 *		same dxpl_id for both parameters.
 *
 *		Observe that this function cannot occasion a read.
 *
 * Return:      Non-negative on success/Negative on failure.
 *
 * Programmer:  John Mainzer, 5/14/04
 *
 *              JRM -- 11/13/08
 *              Modified function to always observe the min_clean_size
 *              whether we are maintaining the clean and dirt LRU lists
 *              or not.  To do this, we had to add the new clean_index_size
 *              and dirty_index_size fields to H5C_t, and supporting code
 *              as needed throughout the cache.
 *
 *              The purpose of this modification is to avoid "metadata
 *              blizzards" in the write only case.  In such instances,
 *              the cache was allowed to fill with dirty metadata.  When
 *              we finally needed to evict an entry to make space, we had
 *              to flush out a whole cache full of metadata -- which has
 *              interesting performance effects.  We hope to avoid (or
 *              perhaps more accurately hide) this effect by maintaining
 *              the min_clean_size, which should force us to start flushing
 *              entries long before we actually have to evict something
 *              to make space.
 *
 *              MAM -- 01/06/09
 *              Added code to maintain clean_entries_skipped and total_entries
 *              scanned statistics.
 *-------------------------------------------------------------------------
 */
static herr_t
H5C_make_space_in_cache(H5F_t *	f,
                        hid_t	primary_dxpl_id,
                        hid_t	secondary_dxpl_id,
		        size_t	space_needed,
                        hbool_t	write_permitted,
                        hbool_t * first_flush_ptr)
{
    H5C_t *		cache_ptr = f->shared->cache;
    herr_t		result;
#if H5C_COLLECT_CACHE_STATS
    int32_t             clean_entries_skipped = 0;
    int32_t             total_entries_scanned = 0;
#endif /* H5C_COLLECT_CACHE_STATS */
    int32_t		entries_examined = 0;
    int32_t		initial_list_len;
    size_t		empty_space;
    hbool_t		prev_is_dirty = FALSE;
    hbool_t             didnt_flush_entry = FALSE;
    H5C_cache_entry_t *	entry_ptr;
    H5C_cache_entry_t *	prev_ptr;
    H5C_cache_entry_t *	next_ptr;
    herr_t		ret_value = SUCCEED;      /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    HDassert( f );
    HDassert( cache_ptr );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );
    HDassert( first_flush_ptr != NULL );
    HDassert( ( *first_flush_ptr == TRUE ) || ( *first_flush_ptr == FALSE ) );
    HDassert( cache_ptr->index_size ==
	      (cache_ptr->clean_index_size + cache_ptr->dirty_index_size) );

    if ( write_permitted ) {

        initial_list_len = cache_ptr->LRU_list_len;

        entry_ptr = cache_ptr->LRU_tail_ptr;

	if ( cache_ptr->index_size >= cache_ptr->max_cache_size ) {

	   empty_space = 0;

	} else {

	   empty_space = cache_ptr->max_cache_size - cache_ptr->index_size;

	}

        while ( ( ( (cache_ptr->index_size + space_needed)
                    >
                    cache_ptr->max_cache_size
                  )
		  ||
		  (
		    ( empty_space + cache_ptr->clean_index_size )
		    <
		    ( cache_ptr->min_clean_size )
                  )
		)
                &&
                ( entries_examined <= (2 * initial_list_len) )
                &&
                ( entry_ptr != NULL )
              )
        {
            HDassert( ! (entry_ptr->is_protected) );
            HDassert( ! (entry_ptr->is_read_only) );
            HDassert( (entry_ptr->ro_ref_count) == 0 );

	    next_ptr = entry_ptr->next;
            prev_ptr = entry_ptr->prev;

	    if ( prev_ptr != NULL ) {

		prev_is_dirty = prev_ptr->is_dirty;
	    }

            if ( (entry_ptr->type)->id != H5C__EPOCH_MARKER_TYPE ) {

                didnt_flush_entry = FALSE;

                if ( entry_ptr->is_dirty ) {

#if H5C_COLLECT_CACHE_STATS
                    if ( (cache_ptr->index_size + space_needed)
                           >
                          cache_ptr->max_cache_size ) {

                        cache_ptr->entries_scanned_to_make_space++;
                    }
#endif /* H5C_COLLECT_CACHE_STATS */
                    result = H5C_flush_single_entry(f,
                                                    primary_dxpl_id,
                                                    secondary_dxpl_id,
                                                    entry_ptr->type,
                                                    entry_ptr->addr,
                                                    H5C__NO_FLAGS_SET,
                                                    first_flush_ptr,
                                                    FALSE);
                } else if ( (cache_ptr->index_size + space_needed)
                              >
                             cache_ptr->max_cache_size ) {
#if H5C_COLLECT_CACHE_STATS
                    cache_ptr->entries_scanned_to_make_space++;
#endif /* H5C_COLLECT_CACHE_STATS */

                    result = H5C_flush_single_entry(f,
                                                    primary_dxpl_id,
                                                    secondary_dxpl_id,
                                                    entry_ptr->type,
                                                    entry_ptr->addr,
                                                    H5C__FLUSH_INVALIDATE_FLAG,
                                                    first_flush_ptr,
                                                    TRUE);
                } else {

                    /* We have enough space so don't flush clean entry.
                     * Set result to SUCCEED to avoid triggering the error
                     * code below.
                     */
#if H5C_COLLECT_CACHE_STATS
                    clean_entries_skipped++;
#endif /* H5C_COLLECT_CACHE_STATS */
                    didnt_flush_entry = TRUE;
                    result = SUCCEED;

                }

#if H5C_COLLECT_CACHE_STATS
                total_entries_scanned++;
#endif /* H5C_COLLECT_CACHE_STATS */


            } else {

                /* Skip epoch markers.  Set result to SUCCEED to avoid
                 * triggering the error code below.
                 */
                didnt_flush_entry = TRUE;
                result = SUCCEED;
            }

            if ( result < 0 ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, \
                            "unable to flush entry")
            }

	    if ( prev_ptr != NULL ) {
#ifndef NDEBUG
		if ( prev_ptr->magic != H5C__H5C_CACHE_ENTRY_T_MAGIC ) {

		    /* something horrible has happened to *prev_ptr --
		     * scream and die.
		     */
                    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
				"*prev_ptr corrupt 1")

                }
#endif /* NDEBUG */
		if ( didnt_flush_entry ) {

		    /* epoch markers don't get flushed, so the sanity checks
		     * on normal entries will fail -- thus just set entry_ptr
		     * to prev_ptr and go on.
		     */
                    entry_ptr = prev_ptr;

		} else if ( ( prev_ptr->is_dirty != prev_is_dirty )
		            ||
		            ( prev_ptr->next != next_ptr )
		            ||
		            ( prev_ptr->is_protected )
		            ||
		            ( prev_ptr->is_pinned ) ) {

		    /* something has happened to the LRU -- start over
		     * from the tail.
		     */
	            entry_ptr = cache_ptr->LRU_tail_ptr;

		} else {

		    entry_ptr = prev_ptr;

		}
	    } else {

		entry_ptr = NULL;

	    }

	    entries_examined++;

	    if ( cache_ptr->index_size >= cache_ptr->max_cache_size ) {

	       empty_space = 0;

	    } else {

	       empty_space = cache_ptr->max_cache_size - cache_ptr->index_size;

	    }

	    HDassert( cache_ptr->index_size ==
	              (cache_ptr->clean_index_size +
		       cache_ptr->dirty_index_size) );

	}

#if H5C_COLLECT_CACHE_STATS
        cache_ptr->calls_to_msic++;

        cache_ptr->total_entries_skipped_in_msic += clean_entries_skipped;
        cache_ptr->total_entries_scanned_in_msic += total_entries_scanned;

        if ( clean_entries_skipped > cache_ptr->max_entries_skipped_in_msic ) {

            cache_ptr->max_entries_skipped_in_msic = clean_entries_skipped;
        }

        if ( total_entries_scanned > cache_ptr->max_entries_scanned_in_msic ) {

            cache_ptr->max_entries_scanned_in_msic = total_entries_scanned;
        }
#endif /* H5C_COLLECT_CACHE_STATS */

	HDassert( ( entries_examined > (2 * initial_list_len) ) ||
		  ( (cache_ptr->pl_size + cache_ptr->pel_size + cache_ptr->min_clean_size) >
		    cache_ptr->max_cache_size ) ||
		  ( ( cache_ptr->clean_index_size + empty_space )
		    >= cache_ptr->min_clean_size ) );

#if H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS

        HDassert( ( entries_examined > (2 * initial_list_len) ) ||
		  ( cache_ptr->cLRU_list_size <= cache_ptr->clean_index_size ) );
        HDassert( ( entries_examined > (2 * initial_list_len) ) ||
		  ( cache_ptr->dLRU_list_size <= cache_ptr->dirty_index_size ) );

#endif /* H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS */

    } else {

        HDassert( H5C_MAINTAIN_CLEAN_AND_DIRTY_LRU_LISTS );

        initial_list_len = cache_ptr->cLRU_list_len;
        entry_ptr = cache_ptr->cLRU_tail_ptr;

        while ( ( (cache_ptr->index_size + space_needed)
                  >
                  cache_ptr->max_cache_size
                )
                &&
                ( entries_examined <= initial_list_len )
                &&
                ( entry_ptr != NULL )
              )
        {
            HDassert( ! (entry_ptr->is_protected) );
            HDassert( ! (entry_ptr->is_read_only) );
            HDassert( (entry_ptr->ro_ref_count) == 0 );
            HDassert( ! (entry_ptr->is_dirty) );

            prev_ptr = entry_ptr->aux_prev;

            result = H5C_flush_single_entry(f,
                                            primary_dxpl_id,
                                            secondary_dxpl_id,
                                            entry_ptr->type,
                                            entry_ptr->addr,
                                            H5C__FLUSH_INVALIDATE_FLAG,
                                            first_flush_ptr,
                                            TRUE);

            if ( result < 0 ) {

                HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, \
                            "unable to flush entry")
            }

            entry_ptr = prev_ptr;
	    entries_examined++;
        }
    }

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C_make_space_in_cache() */


/*-------------------------------------------------------------------------
 *
 * Function:    H5C_validate_lru_list
 *
 * Purpose:     Debugging function that scans the LRU list for errors.
 *
 *		If an error is detected, the function generates a
 *		diagnostic and returns FAIL.  If no error is detected,
 *		the function returns SUCCEED.
 *
 * Return:      FAIL if error is detected, SUCCEED otherwise.
 *
 * Programmer:  John Mainzer, 7/14/05
 *
 *-------------------------------------------------------------------------
 */
#if H5C_DO_EXTREME_SANITY_CHECKS

static herr_t
H5C_validate_lru_list(H5C_t * cache_ptr)
{
    herr_t		ret_value = SUCCEED;      /* Return value */
    int32_t             len = 0;
    size_t              size = 0;
    H5C_cache_entry_t *	entry_ptr = NULL;

    FUNC_ENTER_NOAPI_NOINIT

    HDassert( cache_ptr );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );

    if ( ( ( cache_ptr->LRU_head_ptr == NULL )
           ||
           ( cache_ptr->LRU_tail_ptr == NULL )
         )
         &&
         ( cache_ptr->LRU_head_ptr != cache_ptr->LRU_tail_ptr )
       ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Check 1 failed")
    }

    if ( ( cache_ptr->LRU_list_len < 0 ) || ( cache_ptr->LRU_list_size < 0 ) ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Check 2 failed")
    }

    if ( ( cache_ptr->LRU_list_len == 1 )
         &&
         ( ( cache_ptr->LRU_head_ptr != cache_ptr->LRU_tail_ptr )
           ||
           ( cache_ptr->LRU_head_ptr == NULL )
           ||
           ( cache_ptr->LRU_head_ptr->size != cache_ptr->LRU_list_size )
         )
       ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Check 3 failed")
    }

    if ( ( cache_ptr->LRU_list_len >= 1 )
         &&
         ( ( cache_ptr->LRU_head_ptr == NULL )
           ||
           ( cache_ptr->LRU_head_ptr->prev != NULL )
           ||
           ( cache_ptr->LRU_tail_ptr == NULL )
           ||
           ( cache_ptr->LRU_tail_ptr->next != NULL )
         )
       ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Check 4 failed")
    }

    entry_ptr = cache_ptr->LRU_head_ptr;
    while ( entry_ptr != NULL )
    {

        if ( ( entry_ptr != cache_ptr->LRU_head_ptr ) &&
             ( ( entry_ptr->prev == NULL ) ||
               ( entry_ptr->prev->next != entry_ptr ) ) ) {

            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Check 5 failed")
        }

        if ( ( entry_ptr != cache_ptr->LRU_tail_ptr ) &&
             ( ( entry_ptr->next == NULL ) ||
               ( entry_ptr->next->prev != entry_ptr ) ) ) {

            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Check 6 failed")
        }

        len++;
        size += entry_ptr->size;
        entry_ptr = entry_ptr->next;
    }

    if ( ( cache_ptr->LRU_list_len != len ) ||
         ( cache_ptr->LRU_list_size != size ) ) {

        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Check 7 failed")
    }

done:

    if ( ret_value != SUCCEED ) {

        HDassert(0);
    }

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C_validate_lru_list() */

#endif /* H5C_DO_EXTREME_SANITY_CHECKS */


/*-------------------------------------------------------------------------
 *
 * Function:    H5C_verify_not_in_index
 *
 * Purpose:     Debugging function that scans the hash table to verify
 *		that the specified instance of H5C_cache_entry_t is not
 *		present.
 *
 *		If an error is detected, the function generates a
 *		diagnostic and returns FAIL.  If no error is detected,
 *		the function returns SUCCEED.
 *
 * Return:      FAIL if error is detected, SUCCEED otherwise.
 *
 * Programmer:  John Mainzer, 7/14/05
 *
 *-------------------------------------------------------------------------
 */
#if H5C_DO_EXTREME_SANITY_CHECKS

static herr_t
H5C_verify_not_in_index(H5C_t * cache_ptr,
                        H5C_cache_entry_t * entry_ptr)
{
    herr_t		ret_value = SUCCEED;      /* Return value */
    int32_t             i;
    int32_t             depth;
    H5C_cache_entry_t *	scan_ptr = NULL;

    FUNC_ENTER_NOAPI_NOINIT

    HDassert( cache_ptr != NULL );
    HDassert( cache_ptr->magic == H5C__H5C_T_MAGIC );
    HDassert( entry_ptr != NULL );

    for ( i = 0; i < H5C__HASH_TABLE_LEN; i++ )
    {
        depth = 0;
        scan_ptr = cache_ptr->index[i];

        while ( scan_ptr != NULL )
        {
            if ( scan_ptr == entry_ptr ) {

                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, \
                            "Entry already in index.")
            }
            depth++;
            scan_ptr = scan_ptr->ht_next;
        }
    }

done:

    if ( ret_value != SUCCEED ) {

        HDassert(0);
    }

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C_verify_not_in_index() */

#endif /* H5C_DO_EXTREME_SANITY_CHECKS */

