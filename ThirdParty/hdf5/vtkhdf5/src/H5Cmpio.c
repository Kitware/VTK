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
 * Created:     H5Cmpio.c
 *              June 20 2015
 *              Quincey Koziol
 *
 * Purpose:     Functions in this file implement support for parallel I/O for
 *              generic cache code.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#include "H5Cmodule.h" /* This source code file is part of the H5C module */
#define H5F_FRIEND     /*suppress error about including H5Fpkg      */

/***********/
/* Headers */
/***********/
#include "H5private.h"   /* Generic Functions                    */
#include "H5ACprivate.h" /* Metadata cache                       */
#include "H5Cpkg.h"      /* Cache                                */
#include "H5CXprivate.h" /* API Contexts                         */
#include "H5Eprivate.h"  /* Error handling                       */
#include "H5Fpkg.h"      /* Files                                */
#include "H5FDprivate.h" /* File drivers                         */
#include "H5MMprivate.h" /* Memory management                    */

#ifdef H5_HAVE_PARALLEL
/****************/
/* Local Macros */
/****************/
#define H5C_APPLY_CANDIDATE_LIST__DEBUG 0

/******************/
/* Local Typedefs */
/******************/

/********************/
/* Local Prototypes */
/********************/
static herr_t H5C__collective_write(H5F_t *f);
static herr_t H5C__flush_candidate_entries(H5F_t *f, unsigned entries_to_flush[H5C_RING_NTYPES],
                                           unsigned entries_to_clear[H5C_RING_NTYPES]);
static herr_t H5C__flush_candidates_in_ring(H5F_t *f, H5C_ring_t ring, unsigned entries_to_flush,
                                            unsigned entries_to_clear);

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
 * Function:    H5C_apply_candidate_list
 *
 * Purpose:     Apply the supplied candidate list.
 *
 *              We used to do this by simply having each process write
 *              every mpi_size-th entry in the candidate list, starting
 *              at index mpi_rank, and mark all the others clean.
 *
 *              However, this can cause unnecessary contention in a file
 *              system by increasing the number of processes writing to
 *              adjacent locations in the HDF5 file.
 *
 *              To attempt to minimize this, we now arange matters such
 *              that each process writes n adjacent entries in the
 *              candidate list, and marks all others clean.  We must do
 *              this in such a fashion as to guarantee that each entry
 *              on the candidate list is written by exactly one process,
 *              and marked clean by all others.
 *
 *              To do this, first construct a table mapping mpi_rank
 *              to the index of the first entry in the candidate list to
 *              be written by the process of that mpi_rank, and then use
 *              the table to control which entries are written and which
 *              are marked as clean as a function of the mpi_rank.
 *
 *              Note that the table must be identical on all processes, as
 *              all see the same candidate list, mpi_size, and mpi_rank --
 *              the inputs used to construct the table.
 *
 *              We construct the table as follows.  Let:
 *
 *                      n = num_candidates / mpi_size;
 *
 *                      m = num_candidates % mpi_size;
 *
 *              Now allocate an array of integers of length mpi_size + 1,
 *              and call this array candidate_assignment_table.
 *
 *              Conceptually, if the number of candidates is a multiple
 *              of the mpi_size, we simply pass through the candidate list
 *              and assign n entries to each process to flush, with the
 *              index of the first entry to flush in the location in
 *              the candidate_assignment_table indicated by the mpi_rank
 *              of the process.
 *
 *              In the more common case in which the candidate list isn't
 *              isn't a multiple of the mpi_size, we pretend it is, and
 *              give num_candidates % mpi_size processes one extra entry
 *              each to make things work out.
 *
 *              Once the table is constructed, we determine the first and
 *              last entry this process is to flush as follows:
 *
 *              first_entry_to_flush = candidate_assignment_table[mpi_rank]
 *
 *              last_entry_to_flush =
 *                      candidate_assignment_table[mpi_rank + 1] - 1;
 *
 *              With these values determined, we simply scan through the
 *              candidate list, marking all entries in the range
 *              [first_entry_to_flush, last_entry_to_flush] for flush,
 *              and all others to be cleaned.
 *
 *              Finally, we scan the LRU from tail to head, flushing
 *              or marking clean the candidate entries as indicated.
 *              If necessary, we scan the pinned list as well.
 *
 *              Note that this function will fail if any protected or
 *              clean entries appear on the candidate list.
 *
 *              This function is used in managing sync points, and
 *              shouldn't be used elsewhere.
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
herr_t
H5C_apply_candidate_list(H5F_t *f, H5C_t *cache_ptr, unsigned num_candidates, haddr_t *candidates_list_ptr,
                         int mpi_rank, int mpi_size)
{
    unsigned           first_entry_to_flush;
    unsigned           last_entry_to_flush;
    unsigned           total_entries_to_clear     = 0;
    unsigned           total_entries_to_flush     = 0;
    unsigned *         candidate_assignment_table = NULL;
    unsigned           entries_to_flush[H5C_RING_NTYPES];
    unsigned           entries_to_clear[H5C_RING_NTYPES];
    haddr_t            addr;
    H5C_cache_entry_t *entry_ptr = NULL;
#if H5C_DO_SANITY_CHECKS
    haddr_t last_addr;
#endif /* H5C_DO_SANITY_CHECKS */
#if H5C_APPLY_CANDIDATE_LIST__DEBUG
    char tbl_buf[1024];
#endif /* H5C_APPLY_CANDIDATE_LIST__DEBUG */
    unsigned m, n;
    unsigned u;                   /* Local index variable */
    herr_t   ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(cache_ptr != NULL);
    HDassert(cache_ptr->magic == H5C__H5C_T_MAGIC);
    HDassert(num_candidates > 0);
    HDassert((!cache_ptr->slist_enabled) || (num_candidates <= cache_ptr->slist_len));
    HDassert(candidates_list_ptr != NULL);
    HDassert(0 <= mpi_rank);
    HDassert(mpi_rank < mpi_size);

    /* Initialize the entries_to_flush and entries_to_clear arrays */
    HDmemset(entries_to_flush, 0, sizeof(entries_to_flush));
    HDmemset(entries_to_clear, 0, sizeof(entries_to_clear));

#if H5C_APPLY_CANDIDATE_LIST__DEBUG
    HDfprintf(stdout, "%s:%d: setting up candidate assignment table.\n", FUNC, mpi_rank);

    HDmemset(tbl_buf, 0, sizeof(tbl_buf));

    HDsprintf(&(tbl_buf[0]), "candidate list = ");
    for (u = 0; u < num_candidates; u++)
        HDsprintf(&(tbl_buf[HDstrlen(tbl_buf)]), " 0x%llx", (long long)(*(candidates_list_ptr + u)));
    HDsprintf(&(tbl_buf[HDstrlen(tbl_buf)]), "\n");

    HDfprintf(stdout, "%s", tbl_buf);
#endif /* H5C_APPLY_CANDIDATE_LIST__DEBUG */

    if (f->shared->coll_md_write) {
        /* Sanity check */
        HDassert(NULL == cache_ptr->coll_write_list);

        /* Create skip list of entries for collective write */
        if (NULL == (cache_ptr->coll_write_list = H5SL_create(H5SL_TYPE_HADDR, NULL)))
            HGOTO_ERROR(H5E_DATASET, H5E_CANTCREATE, FAIL, "can't create skip list for entries")
    } /* end if */

    n = num_candidates / (unsigned)mpi_size;
    m = num_candidates % (unsigned)mpi_size;

    if (NULL ==
        (candidate_assignment_table = (unsigned *)H5MM_malloc(sizeof(unsigned) * (size_t)(mpi_size + 1))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                    "memory allocation failed for candidate assignment table")

    candidate_assignment_table[0]        = 0;
    candidate_assignment_table[mpi_size] = num_candidates;

    if (m == 0) { /* mpi_size is an even divisor of num_candidates */
        for (u = 1; u < (unsigned)mpi_size; u++)
            candidate_assignment_table[u] = candidate_assignment_table[u - 1] + n;
    } /* end if */
    else {
        for (u = 1; u <= m; u++)
            candidate_assignment_table[u] = candidate_assignment_table[u - 1] + n + 1;

        if (num_candidates < (unsigned)mpi_size) {
            for (u = m + 1; u < (unsigned)mpi_size; u++)
                candidate_assignment_table[u] = num_candidates;
        } /* end if */
        else {
            for (u = m + 1; u < (unsigned)mpi_size; u++)
                candidate_assignment_table[u] = candidate_assignment_table[u - 1] + n;
        } /* end else */
    }     /* end else */
    HDassert((candidate_assignment_table[mpi_size - 1] + n) == num_candidates);

#if H5C_DO_SANITY_CHECKS
    /* Verify that the candidate assignment table has the expected form */
    for (u = 1; u < (unsigned)(mpi_size - 1); u++) {
        unsigned a, b;

        a = candidate_assignment_table[u] - candidate_assignment_table[u - 1];
        b = candidate_assignment_table[u + 1] - candidate_assignment_table[u];

        HDassert(n + 1 >= a);
        HDassert(a >= b);
        HDassert(b >= n);
    }
#endif /* H5C_DO_SANITY_CHECKS */

    first_entry_to_flush = candidate_assignment_table[mpi_rank];
    last_entry_to_flush  = candidate_assignment_table[mpi_rank + 1] - 1;

#if H5C_APPLY_CANDIDATE_LIST__DEBUG
    for (u = 0; u < 1024; u++)
        tbl_buf[u] = '\0';
    HDsprintf(&(tbl_buf[0]), "candidate assignment table = ");
    for (u = 0; u <= (unsigned)mpi_size; u++)
        HDsprintf(&(tbl_buf[HDstrlen(tbl_buf)]), " %u", candidate_assignment_table[u]);
    HDsprintf(&(tbl_buf[HDstrlen(tbl_buf)]), "\n");
    HDfprintf(stdout, "%s", tbl_buf);

    HDfprintf(stdout, "%s:%d: flush entries [%u, %u].\n", FUNC, mpi_rank, first_entry_to_flush,
              last_entry_to_flush);

    HDfprintf(stdout, "%s:%d: marking entries.\n", FUNC, mpi_rank);
#endif /* H5C_APPLY_CANDIDATE_LIST__DEBUG */

    for (u = 0; u < num_candidates; u++) {
        addr = candidates_list_ptr[u];
        HDassert(H5F_addr_defined(addr));

#if H5C_DO_SANITY_CHECKS
        if (u > 0) {
            if (last_addr == addr)
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "duplicate entry in cleaned list")
            else if (last_addr > addr)
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "candidate list not sorted")
        } /* end if */

        last_addr = addr;
#endif /* H5C_DO_SANITY_CHECKS */

        H5C__SEARCH_INDEX(cache_ptr, addr, entry_ptr, FAIL)
        if (entry_ptr == NULL)
            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "listed candidate entry not in cache?!?!?")
        if (!entry_ptr->is_dirty)
            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Listed entry not dirty?!?!?")
        if (entry_ptr->is_protected)
            /* For now at least, we can't deal with protected entries.
             * If we encounter one, scream and die.  If it becomes an
             * issue, we should be able to work around this.
             */
            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Listed entry is protected?!?!?")

        /* Sanity checks */
        HDassert(entry_ptr->magic == H5C__H5C_CACHE_ENTRY_T_MAGIC);
        HDassert(entry_ptr->ring >= H5C_RING_USER);
        HDassert(entry_ptr->ring <= H5C_RING_SB);
        HDassert(!entry_ptr->flush_immediately);
        HDassert(!entry_ptr->clear_on_unprotect);

        /* Determine whether the entry is to be cleared or flushed,
         * and mark it accordingly.  We will scan the protected and
         * pinned list shortly, and clear or flush according to these
         * markings.
         */
        if (u >= first_entry_to_flush && u <= last_entry_to_flush) {
            total_entries_to_flush++;
            entries_to_flush[entry_ptr->ring]++;
            entry_ptr->flush_immediately = TRUE;
        } /* end if */
        else {
            total_entries_to_clear++;
            entries_to_clear[entry_ptr->ring]++;
            entry_ptr->clear_on_unprotect = TRUE;
        } /* end else */

        /* Entries marked as collectively accessed and are in the
         * candidate list to clear from the cache have to be
         * removed from the coll list. This is OK since the
         * candidate list is collective and uniform across all
         * ranks.
         */
        if (entry_ptr->coll_access) {
            entry_ptr->coll_access = FALSE;
            H5C__REMOVE_FROM_COLL_LIST(cache_ptr, entry_ptr, FAIL)
        } /* end if */
    }     /* end for */

#if H5C_DO_SANITY_CHECKS
    m = 0;
    n = 0;
    for (u = 0; u < H5C_RING_NTYPES; u++) {
        m += entries_to_flush[u];
        n += entries_to_clear[u];
    } /* end if */

    HDassert((unsigned)m == total_entries_to_flush);
    HDassert(n == total_entries_to_clear);
#endif /* H5C_DO_SANITY_CHECKS */

#if H5C_APPLY_CANDIDATE_LIST__DEBUG
    HDfprintf(stdout, "%s:%d: num candidates/to clear/to flush = %u/%u/%u.\n", FUNC, mpi_rank, num_candidates,
              total_entries_to_clear, total_entries_to_flush);
#endif /* H5C_APPLY_CANDIDATE_LIST__DEBUG */

    /* We have now marked all the entries on the candidate list for
     * either flush or clear -- now scan the LRU and the pinned list
     * for these entries and do the deed.  Do this via a call to
     * H5C__flush_candidate_entries().
     *
     * Note that we are doing things in this round about manner so as
     * to preserve the order of the LRU list to the best of our ability.
     * If we don't do this, my experiments indicate that we will have a
     * noticeably poorer hit ratio as a result.
     */
    if (H5C__flush_candidate_entries(f, entries_to_flush, entries_to_clear) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "flush candidates failed")

    /* If we've deferred writing to do it collectively, take care of that now */
    if (f->shared->coll_md_write) {
        /* Sanity check */
        HDassert(cache_ptr->coll_write_list);

        /* Write collective list */
        if (H5C__collective_write(f) < 0)
            HGOTO_ERROR(H5E_CACHE, H5E_WRITEERROR, FAIL, "can't write metadata collectively")
    } /* end if */

done:
    if (candidate_assignment_table != NULL)
        candidate_assignment_table = (unsigned *)H5MM_xfree((void *)candidate_assignment_table);
    if (cache_ptr->coll_write_list) {
        if (H5SL_close(cache_ptr->coll_write_list) < 0)
            HDONE_ERROR(H5E_CACHE, H5E_CANTFREE, FAIL, "failed to destroy skip list")
        cache_ptr->coll_write_list = NULL;
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5C_apply_candidate_list() */

/*-------------------------------------------------------------------------
 *
 * Function:    H5C_construct_candidate_list__clean_cache
 *
 * Purpose:     Construct the list of entries that should be flushed to
 *              clean all entries in the cache.
 *
 *              This function is used in managing sync points, and
 *              shouldn't be used elsewhere.
 *
 * Return:      Success:        SUCCEED
 *
 *              Failure:        FAIL
 *
 * Programmer:  John Mainzer
 *              3/17/10
 *
 * Changes:     With the slist optimization, the slist is not maintained
 *              unless a flush is in progress.  Thus we can not longer use
 *              cache_ptr->slist_size to determine the total size of
 *              the entries we must insert in the candidate list.
 *
 *              To address this, we now use cache_ptr->dirty_index_size
 *              instead.
 *
 *                                              JRM -- 7/27/20
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_construct_candidate_list__clean_cache(H5C_t *cache_ptr)
{
    size_t space_needed;
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(cache_ptr != NULL);
    HDassert(cache_ptr->magic == H5C__H5C_T_MAGIC);

    /* As a sanity check, set space needed to the dirty_index_size.  This
     * should be the sum total of the sizes of all the dirty entries
     * in the metadata cache.  Note that if the slist is enabled,
     * cache_ptr->slist_size should equal cache_ptr->dirty_index_size.
     */
    space_needed = cache_ptr->dirty_index_size;

    HDassert((!cache_ptr->slist_enabled) || (space_needed == cache_ptr->slist_size));

    /* Recall that while we shouldn't have any protected entries at this
     * point, it is possible that some dirty entries may reside on the
     * pinned list at this point.
     */
    HDassert(cache_ptr->dirty_index_size <= (cache_ptr->dLRU_list_size + cache_ptr->pel_size));
    HDassert((!cache_ptr->slist_enabled) ||
             (cache_ptr->slist_len <= (cache_ptr->dLRU_list_len + cache_ptr->pel_len)));

    if (space_needed > 0) { /* we have work to do */

        H5C_cache_entry_t *entry_ptr;
        unsigned           nominated_entries_count = 0;
        size_t             nominated_entries_size  = 0;
        haddr_t            nominated_addr;

        HDassert((!cache_ptr->slist_enabled) || (cache_ptr->slist_len > 0));

        /* Scan the dirty LRU list from tail forward and nominate sufficient
         * entries to free up the necessary space.
         */
        entry_ptr = cache_ptr->dLRU_tail_ptr;

        while ((nominated_entries_size < space_needed) &&
               ((!cache_ptr->slist_enabled) || (nominated_entries_count < cache_ptr->slist_len)) &&
               (entry_ptr != NULL)) {

            HDassert(!(entry_ptr->is_protected));
            HDassert(!(entry_ptr->is_read_only));
            HDassert(entry_ptr->ro_ref_count == 0);
            HDassert(entry_ptr->is_dirty);
            HDassert((!cache_ptr->slist_enabled) || (entry_ptr->in_slist));

            nominated_addr = entry_ptr->addr;

            if (H5AC_add_candidate((H5AC_t *)cache_ptr, nominated_addr) < 0)

                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5AC_add_candidate() failed")

            nominated_entries_size += entry_ptr->size;
            nominated_entries_count++;
            entry_ptr = entry_ptr->aux_prev;

        } /* end while */

        HDassert(entry_ptr == NULL);

        /* it is possible that there are some dirty entries on the
         * protected entry list as well -- scan it too if necessary
         */
        entry_ptr = cache_ptr->pel_head_ptr;

        while ((nominated_entries_size < space_needed) &&
               ((!cache_ptr->slist_enabled) || (nominated_entries_count < cache_ptr->slist_len)) &&
               (entry_ptr != NULL)) {

            if (entry_ptr->is_dirty) {

                HDassert(!(entry_ptr->is_protected));
                HDassert(!(entry_ptr->is_read_only));
                HDassert(entry_ptr->ro_ref_count == 0);
                HDassert(entry_ptr->is_dirty);
                HDassert(entry_ptr->in_slist);

                nominated_addr = entry_ptr->addr;

                if (H5AC_add_candidate((H5AC_t *)cache_ptr, nominated_addr) < 0)

                    HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5AC_add_candidate() failed")

                nominated_entries_size += entry_ptr->size;
                nominated_entries_count++;

            } /* end if */

            entry_ptr = entry_ptr->next;

        } /* end while */

        HDassert((!cache_ptr->slist_enabled) || (nominated_entries_count == cache_ptr->slist_len));
        HDassert(nominated_entries_size == space_needed);

    } /* end if */

done:

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5C_construct_candidate_list__clean_cache() */

/*-------------------------------------------------------------------------
 * Function:    H5C_construct_candidate_list__min_clean
 *
 * Purpose:     Construct the list of entries that should be flushed to
 *              get the cache back within its min clean constraints.
 *
 *              This function is used in managing sync points, and
 *              shouldn't be used elsewhere.
 *
 * Return:      Success:        SUCCEED
 *
 *              Failure:        FAIL
 *
 * Programmer:  John Mainzer
 *              3/17/10
 *
 * Changes:     With the slist optimization, the slist is not maintained
 *              unless a flush is in progress.  Updated sanity checks to
 *              reflect this.
 *
 *                                              JRM -- 7/27/20
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_construct_candidate_list__min_clean(H5C_t *cache_ptr)
{
    size_t space_needed = 0;
    herr_t ret_value    = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(cache_ptr != NULL);
    HDassert(cache_ptr->magic == H5C__H5C_T_MAGIC);

    /* compute the number of bytes (if any) that must be flushed to get the
     * cache back within its min clean constraints.
     */
    if (cache_ptr->max_cache_size > cache_ptr->index_size) {

        if (((cache_ptr->max_cache_size - cache_ptr->index_size) + cache_ptr->cLRU_list_size) >=
            cache_ptr->min_clean_size) {

            space_needed = 0;
        }
        else {

            space_needed = cache_ptr->min_clean_size -
                           ((cache_ptr->max_cache_size - cache_ptr->index_size) + cache_ptr->cLRU_list_size);
        }
    } /* end if */
    else {

        if (cache_ptr->min_clean_size <= cache_ptr->cLRU_list_size) {

            space_needed = 0;
        }
        else {

            space_needed = cache_ptr->min_clean_size - cache_ptr->cLRU_list_size;
        }
    } /* end else */

    if (space_needed > 0) { /* we have work to do */

        H5C_cache_entry_t *entry_ptr;
        unsigned           nominated_entries_count = 0;
        size_t             nominated_entries_size  = 0;

        HDassert((!cache_ptr->slist_enabled) || (cache_ptr->slist_len > 0));

        /* Scan the dirty LRU list from tail forward and nominate sufficient
         * entries to free up the necessary space.
         */
        entry_ptr = cache_ptr->dLRU_tail_ptr;

        while ((nominated_entries_size < space_needed) &&
               ((!cache_ptr->slist_enabled) || (nominated_entries_count < cache_ptr->slist_len)) &&
               (entry_ptr != NULL) && (!entry_ptr->flush_me_last)) {

            haddr_t nominated_addr;

            HDassert(!(entry_ptr->is_protected));
            HDassert(!(entry_ptr->is_read_only));
            HDassert(entry_ptr->ro_ref_count == 0);
            HDassert(entry_ptr->is_dirty);
            HDassert((!cache_ptr->slist_enabled) || (entry_ptr->in_slist));

            nominated_addr = entry_ptr->addr;

            if (H5AC_add_candidate((H5AC_t *)cache_ptr, nominated_addr) < 0)

                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "H5AC_add_candidate() failed")

            nominated_entries_size += entry_ptr->size;
            nominated_entries_count++;
            entry_ptr = entry_ptr->aux_prev;

        } /* end while */

        HDassert((!cache_ptr->slist_enabled) || (nominated_entries_count <= cache_ptr->slist_len));
        HDassert(nominated_entries_size <= cache_ptr->dirty_index_size);
        HDassert(nominated_entries_size >= space_needed);
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5C_construct_candidate_list__min_clean() */

/*-------------------------------------------------------------------------
 *
 * Function:    H5C_mark_entries_as_clean
 *
 * Purpose:     When the H5C code is used to implement the metadata caches
 *              in PHDF5, only the cache with MPI_rank 0 is allowed to
 *              actually write entries to disk -- all other caches must
 *              retain dirty entries until they are advised that the
 *              entries are clean.
 *
 *              This function exists to allow the H5C code to receive these
 *              notifications.
 *
 *              The function receives a list of entry base addresses
 *              which must refer to dirty entries in the cache.  If any
 *              of the entries are either clean or don't exist, the
 *              function flags an error.
 *
 *              The function scans the list of entries and flushes all
 *              those that are currently unprotected with the
 *              H5C__FLUSH_CLEAR_ONLY_FLAG.  Those that are currently
 *              protected are flagged for clearing when they are
 *              unprotected.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  John Mainzer
 *              7/5/05
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_mark_entries_as_clean(H5F_t *f, unsigned ce_array_len, haddr_t *ce_array_ptr)
{
    H5C_t *  cache_ptr;
    unsigned entries_cleared;
    unsigned pinned_entries_cleared;
    hbool_t  progress;
    unsigned entries_examined;
    unsigned initial_list_len;
    haddr_t  addr;
    unsigned pinned_entries_marked = 0;
#if H5C_DO_SANITY_CHECKS
    unsigned protected_entries_marked = 0;
    unsigned other_entries_marked     = 0;
    haddr_t  last_addr;
#endif /* H5C_DO_SANITY_CHECKS */
    H5C_cache_entry_t *clear_ptr = NULL;
    H5C_cache_entry_t *entry_ptr = NULL;
    unsigned           u;
    herr_t             ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(f);
    HDassert(f->shared);
    cache_ptr = f->shared->cache;
    HDassert(cache_ptr);
    HDassert(cache_ptr->magic == H5C__H5C_T_MAGIC);

    HDassert(ce_array_len > 0);
    HDassert(ce_array_ptr != NULL);

#if H5C_DO_EXTREME_SANITY_CHECKS
    if (H5C_validate_protected_entry_list(cache_ptr) < 0 || H5C_validate_pinned_entry_list(cache_ptr) < 0 ||
        H5C_validate_lru_list(cache_ptr) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "an extreme sanity check failed on entry")
#endif /* H5C_DO_EXTREME_SANITY_CHECKS */

    for (u = 0; u < ce_array_len; u++) {
        addr = ce_array_ptr[u];

#if H5C_DO_SANITY_CHECKS
        if (u == 0)
            last_addr = addr;
        else {
            if (last_addr == addr)
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Duplicate entry in cleaned list")
            if (last_addr > addr)
                HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "cleaned list not sorted")
        } /* end else */

#if H5C_DO_EXTREME_SANITY_CHECKS
        if (H5C_validate_protected_entry_list(cache_ptr) < 0 ||
            H5C_validate_pinned_entry_list(cache_ptr) < 0 || H5C_validate_lru_list(cache_ptr) < 0)
            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "an extreme sanity check failed in for loop")
#endif /* H5C_DO_EXTREME_SANITY_CHECKS */
#endif /* H5C_DO_SANITY_CHECKS */

        HDassert(H5F_addr_defined(addr));

        H5C__SEARCH_INDEX(cache_ptr, addr, entry_ptr, FAIL)

        if (entry_ptr == NULL) {
#if H5C_DO_SANITY_CHECKS
            HDfprintf(stdout, "H5C_mark_entries_as_clean: entry[%u] = %" PRIuHADDR " not in cache.\n", u,
                      addr);
#endif /* H5C_DO_SANITY_CHECKS */
            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Listed entry not in cache?!?!?")
        } /* end if */
        else if (!entry_ptr->is_dirty) {
#if H5C_DO_SANITY_CHECKS
            HDfprintf(stdout, "H5C_mark_entries_as_clean: entry %" PRIuHADDR " is not dirty!?!\n", addr);
#endif /* H5C_DO_SANITY_CHECKS */
            HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "Listed entry not dirty?!?!?")
        } /* end else-if */
        else {
            /* Mark the entry to be cleared on unprotect.  We will
             * scan the LRU list shortly, and clear all those entries
             * not currently protected.
             */

            /* Make sure first that we clear the collective flag from
               it so it can be cleared */
            if (TRUE == entry_ptr->coll_access) {
                entry_ptr->coll_access = FALSE;
                H5C__REMOVE_FROM_COLL_LIST(cache_ptr, entry_ptr, FAIL)
            } /* end if */

            entry_ptr->clear_on_unprotect = TRUE;
            if (entry_ptr->is_pinned)
                pinned_entries_marked++;
#if H5C_DO_SANITY_CHECKS
            else if (entry_ptr->is_protected)
                protected_entries_marked++;
            else
                other_entries_marked++;
#endif /* H5C_DO_SANITY_CHECKS */
        }
    }

    /* Scan through the LRU list from back to front, and flush the
     * entries whose clear_on_unprotect flags are set.  Observe that
     * any protected entries will not be on the LRU, and therefore
     * will not be flushed at this time.
     *
     * Note that unlike H5C_apply_candidate_list(),
     * H5C_mark_entries_as_clean() makes all its calls to
     * H5C__flush_single_entry() with the H5C__FLUSH_CLEAR_ONLY_FLAG
     * set.  As a result, the pre_serialize() and serialize calls are
     * not made.
     *
     * This then implies that (assuming such actions were
     * permitted in the parallel case) no loads, dirties,
     * resizes, or removals of other entries can occur as
     * a side effect of the flush.  Hence, there is no need
     * for the checks for entry removal / status change
     * that I ported to H5C_apply_candidate_list().
     *
     * However, if (in addition to allowing such operations
     * in the parallel case), we allow such operations outside
     * of the pre_serialize / serialize routines, this may
     * cease to be the case -- requiring a review of this
     * point.
     *                                  JRM -- 4/7/15
     */
    entries_cleared  = 0;
    entries_examined = 0;
    initial_list_len = cache_ptr->LRU_list_len;
    entry_ptr        = cache_ptr->LRU_tail_ptr;
    while (entry_ptr != NULL && entries_examined <= initial_list_len && entries_cleared < ce_array_len) {
        if (entry_ptr->clear_on_unprotect) {
            entry_ptr->clear_on_unprotect = FALSE;
            clear_ptr                     = entry_ptr;
            entry_ptr                     = entry_ptr->prev;
            entries_cleared++;

            if (H5C__flush_single_entry(f, clear_ptr,
                                        (H5C__FLUSH_CLEAR_ONLY_FLAG | H5C__GENERATE_IMAGE_FLAG |
                                         H5C__UPDATE_PAGE_BUFFER_FLAG)) < 0)
                HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "can't clear entry")
        } /* end if */
        else
            entry_ptr = entry_ptr->prev;
        entries_examined++;
    } /* end while */

#if H5C_DO_SANITY_CHECKS
    HDassert(entries_cleared == other_entries_marked);
#endif /* H5C_DO_SANITY_CHECKS */

    /* It is also possible that some of the cleared entries are on the
     * pinned list.  Must scan that also.
     */
    pinned_entries_cleared = 0;
    progress               = TRUE;
    while ((pinned_entries_cleared < pinned_entries_marked) && progress) {
        progress  = FALSE;
        entry_ptr = cache_ptr->pel_head_ptr;
        while (entry_ptr != NULL) {
            if (entry_ptr->clear_on_unprotect && entry_ptr->flush_dep_ndirty_children == 0) {
                entry_ptr->clear_on_unprotect = FALSE;
                clear_ptr                     = entry_ptr;
                entry_ptr                     = entry_ptr->next;
                entries_cleared++;
                pinned_entries_cleared++;
                progress = TRUE;

                if (H5C__flush_single_entry(f, clear_ptr,
                                            (H5C__FLUSH_CLEAR_ONLY_FLAG | H5C__GENERATE_IMAGE_FLAG |
                                             H5C__UPDATE_PAGE_BUFFER_FLAG)) < 0)
                    HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "can't clear entry")
            } /* end if */
            else
                entry_ptr = entry_ptr->next;
        } /* end while */
    }     /* end while */

#if H5C_DO_SANITY_CHECKS
    HDassert(entries_cleared == pinned_entries_marked + other_entries_marked);
    HDassert(entries_cleared + protected_entries_marked == ce_array_len);
#endif /* H5C_DO_SANITY_CHECKS */

    HDassert((entries_cleared == ce_array_len) || ((ce_array_len - entries_cleared) <= cache_ptr->pl_len));

#if H5C_DO_SANITY_CHECKS
    u         = 0;
    entry_ptr = cache_ptr->pl_head_ptr;
    while (entry_ptr != NULL) {
        if (entry_ptr->clear_on_unprotect) {

            u++;
        }
        entry_ptr = entry_ptr->next;
    }
    HDassert((entries_cleared + u) == ce_array_len);
#endif /* H5C_DO_SANITY_CHECKS */

done:
#if H5C_DO_EXTREME_SANITY_CHECKS
    if (H5C_validate_protected_entry_list(cache_ptr) < 0 || H5C_validate_pinned_entry_list(cache_ptr) < 0 ||
        H5C_validate_lru_list(cache_ptr) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "an extreme sanity check failed on exit")
#endif /* H5C_DO_EXTREME_SANITY_CHECKS */

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5C_mark_entries_as_clean() */

/*-------------------------------------------------------------------------
 *
 * Function:    H5C_clear_coll_entries
 *
 * Purpose:     Clear half or the entire list of collective entries and
 *              mark them as independent.
 *
 * Return:      FAIL if error is detected, SUCCEED otherwise.
 *
 * Programmer:  Mohamad Chaarawi
 *              April, 2015
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5C_clear_coll_entries(H5C_t *cache_ptr, hbool_t partial)
{
    uint32_t           clear_cnt;
    H5C_cache_entry_t *entry_ptr = NULL;
    herr_t             ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT

    entry_ptr = cache_ptr->coll_tail_ptr;
    clear_cnt = (partial ? cache_ptr->coll_list_len / 2 : cache_ptr->coll_list_len);
    while (entry_ptr && clear_cnt > 0) {
        H5C_cache_entry_t *prev_ptr = entry_ptr->coll_prev;

        /* Sanity check */
        HDassert(entry_ptr->coll_access);

        /* Mark entry as independent */
        entry_ptr->coll_access = FALSE;
        H5C__REMOVE_FROM_COLL_LIST(cache_ptr, entry_ptr, FAIL)

        /* Decrement entry count */
        clear_cnt--;

        /* Advance to next entry */
        entry_ptr = prev_ptr;
    } /* end while */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5C_clear_coll_entries */

/*-------------------------------------------------------------------------
 *
 * Function:    H5C__collective_write
 *
 * Purpose:     Perform a collective write of a list of metadata entries.
 *
 * Return:      FAIL if error is detected, SUCCEED otherwise.
 *
 * Programmer:  Mohamad Chaarawi
 *              February, 2016
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5C__collective_write(H5F_t *f)
{
    H5AC_t *         cache_ptr;
    H5FD_mpio_xfer_t orig_xfer_mode = H5FD_MPIO_COLLECTIVE;
    void *           base_buf;
    int              count;
    int *            length_array = NULL;
    MPI_Aint *       buf_array    = NULL;
    MPI_Aint *       offset_array = NULL;
    MPI_Datatype     btype        = MPI_BYTE;
    MPI_Datatype     ftype        = MPI_BYTE;
    int              mpi_code;
    char             unused = 0; /* Unused, except for non-NULL pointer value */
    size_t           buf_count;
    herr_t           ret_value = SUCCEED;

    FUNC_ENTER_STATIC

    /* Sanity checks */
    HDassert(f != NULL);
    cache_ptr = f->shared->cache;
    HDassert(cache_ptr != NULL);
    HDassert(cache_ptr->coll_write_list != NULL);

    /* Get original transfer mode */
    if (H5CX_get_io_xfer_mode(&orig_xfer_mode) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTGET, FAIL, "can't get MPI-I/O transfer mode")

    /* Set transfer mode */
    if (H5CX_set_io_xfer_mode(H5FD_MPIO_COLLECTIVE) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTSET, FAIL, "can't set MPI-I/O transfer mode")

    /* Get number of entries in collective write list */
    count = (int)H5SL_count(cache_ptr->coll_write_list);
    if (count > 0) {
        H5SL_node_t *      node;
        H5C_cache_entry_t *entry_ptr;
        int                i;

        /* Allocate arrays */
        if (NULL == (length_array = (int *)H5MM_malloc((size_t)count * sizeof(int))))
            HGOTO_ERROR(H5E_CACHE, H5E_CANTALLOC, FAIL,
                        "memory allocation failed for collective write table length array")
        if (NULL == (buf_array = (MPI_Aint *)H5MM_malloc((size_t)count * sizeof(MPI_Aint))))
            HGOTO_ERROR(H5E_CACHE, H5E_CANTALLOC, FAIL,
                        "memory allocation failed for collective buf table length array")
        if (NULL == (offset_array = (MPI_Aint *)H5MM_malloc((size_t)count * sizeof(MPI_Aint))))
            HGOTO_ERROR(H5E_CACHE, H5E_CANTALLOC, FAIL,
                        "memory allocation failed for collective offset table length array")

        /* Fill arrays */
        node = H5SL_first(cache_ptr->coll_write_list);
        HDassert(node);
        if (NULL == (entry_ptr = (H5C_cache_entry_t *)H5SL_item(node)))
            HGOTO_ERROR(H5E_CACHE, H5E_NOTFOUND, FAIL, "can't retrieve skip list item")

        /* Set up initial array position & buffer base address */
        length_array[0] = (int)entry_ptr->size;
        base_buf        = entry_ptr->image_ptr;
        buf_array[0]    = (MPI_Aint)0;
        offset_array[0] = (MPI_Aint)entry_ptr->addr;

        node = H5SL_next(node);
        i    = 1;
        while (node) {
            if (NULL == (entry_ptr = (H5C_cache_entry_t *)H5SL_item(node)))
                HGOTO_ERROR(H5E_CACHE, H5E_NOTFOUND, FAIL, "can't retrieve skip list item")

            /* Set up array position */
            length_array[i] = (int)entry_ptr->size;
            buf_array[i]    = (MPI_Aint)entry_ptr->image_ptr - (MPI_Aint)base_buf;
            offset_array[i] = (MPI_Aint)entry_ptr->addr;

            /* Advance to next node & array location */
            node = H5SL_next(node);
            i++;
        } /* end while */

        /* Create memory MPI type */
        if (MPI_SUCCESS !=
            (mpi_code = MPI_Type_create_hindexed(count, length_array, buf_array, MPI_BYTE, &btype)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Type_create_hindexed failed", mpi_code)
        if (MPI_SUCCESS != (mpi_code = MPI_Type_commit(&btype)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Type_commit failed", mpi_code)

        /* Create file MPI type */
        if (MPI_SUCCESS !=
            (mpi_code = MPI_Type_create_hindexed(count, length_array, offset_array, MPI_BYTE, &ftype)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Type_create_hindexed failed", mpi_code)
        if (MPI_SUCCESS != (mpi_code = MPI_Type_commit(&ftype)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Type_commit failed", mpi_code)

        /* MPI count to write */
        buf_count = 1;
    } /* end if */
    else {
        /* Set non-NULL pointer for I/O operation */
        base_buf = &unused;

        /* MPI count to write */
        buf_count = 0;
    } /* end else */

    /* Pass buf type, file type to the file driver */
    if (H5CX_set_mpi_coll_datatypes(btype, ftype) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_CANTSET, FAIL, "can't set MPI-I/O properties")

    /* Write data */
    if (H5F_block_write(f, H5FD_MEM_DEFAULT, (haddr_t)0, buf_count, base_buf) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_WRITEERROR, FAIL, "unable to write entries collectively")

done:
    /* Free arrays */
    length_array = (int *)H5MM_xfree(length_array);
    buf_array    = (MPI_Aint *)H5MM_xfree(buf_array);
    offset_array = (MPI_Aint *)H5MM_xfree(offset_array);

    /* Free MPI Types */
    if (MPI_BYTE != btype && MPI_SUCCESS != (mpi_code = MPI_Type_free(&btype)))
        HMPI_DONE_ERROR(FAIL, "MPI_Type_free failed", mpi_code)
    if (MPI_BYTE != ftype && MPI_SUCCESS != (mpi_code = MPI_Type_free(&ftype)))
        HMPI_DONE_ERROR(FAIL, "MPI_Type_free failed", mpi_code)

    /* Reset transfer mode in API context, if changed */
    if (orig_xfer_mode != H5FD_MPIO_COLLECTIVE)
        if (H5CX_set_io_xfer_mode(orig_xfer_mode) < 0)
            HDONE_ERROR(H5E_CACHE, H5E_CANTSET, FAIL, "can't set MPI-I/O transfer mode")

    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5C__collective_write() */

/*-------------------------------------------------------------------------
 * Function:    H5C__flush_candidate_entries
 *
 * Purpose:     Flush or clear (as indicated) the candidate entries that
 *              have been marked in the metadata cache.  In so doing,
 *              observe rings and flush dependencies.
 *
 *              Note that this function presumes that:
 *
 *              1) no candidate entries are protected,
 *
 *              2) all candidate entries are dirty, and
 *
 *              3) if a candidate entry has a dirty flush dependency
 *                 child, that child is also a candidate entry.
 *
 *              The function will fail if any of these preconditions are
 *              not met.
 *
 *              Candidate entries are marked by setting either the
 *              flush_immediately or the clear_on_unprotect flags in the
 *              cache entry (but not both).  Entries marked flush_immediately
 *              will be flushed, those marked clear_on_unprotect will be
 *              cleared.
 *
 *              Note that this function is a modified version of
 *              H5C_flush_cache() -- any changes there may need to be
 *              reflected here and vise versa.
 *
 * Return:      Non-negative on success/Negative on failure.
 *
 * Programmer:  John Mainzer
 *              2/10/17
 *
 * Changes:     None.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5C__flush_candidate_entries(H5F_t *f, unsigned entries_to_flush[H5C_RING_NTYPES],
                             unsigned entries_to_clear[H5C_RING_NTYPES])
{
#if H5C_DO_SANITY_CHECKS
    int      i;
    uint32_t index_len        = 0;
    size_t   index_size       = (size_t)0;
    size_t   clean_index_size = (size_t)0;
    size_t   dirty_index_size = (size_t)0;
    size_t   slist_size       = (size_t)0;
    uint32_t slist_len        = 0;
#endif /* H5C_DO_SANITY_CHECKS */
    H5C_ring_t ring;
    H5C_t *    cache_ptr;
    herr_t     ret_value = SUCCEED;

    FUNC_ENTER_STATIC

    HDassert(f);
    HDassert(f->shared);

    cache_ptr = f->shared->cache;

    HDassert(cache_ptr);
    HDassert(cache_ptr->magic == H5C__H5C_T_MAGIC);
    HDassert(cache_ptr->slist_ptr);

    HDassert(entries_to_flush[H5C_RING_UNDEFINED] == 0);
    HDassert(entries_to_clear[H5C_RING_UNDEFINED] == 0);

#if H5C_DO_SANITY_CHECKS
    HDassert(cache_ptr->index_ring_len[H5C_RING_UNDEFINED] == 0);
    HDassert(cache_ptr->index_ring_size[H5C_RING_UNDEFINED] == (size_t)0);
    HDassert(cache_ptr->clean_index_ring_size[H5C_RING_UNDEFINED] == (size_t)0);
    HDassert(cache_ptr->dirty_index_ring_size[H5C_RING_UNDEFINED] == (size_t)0);
    HDassert(cache_ptr->slist_ring_len[H5C_RING_UNDEFINED] == 0);
    HDassert(cache_ptr->slist_ring_size[H5C_RING_UNDEFINED] == (size_t)0);

    for (i = H5C_RING_USER; i < H5C_RING_NTYPES; i++) {
        index_len += cache_ptr->index_ring_len[i];
        index_size += cache_ptr->index_ring_size[i];
        clean_index_size += cache_ptr->clean_index_ring_size[i];
        dirty_index_size += cache_ptr->dirty_index_ring_size[i];

        slist_len += cache_ptr->slist_ring_len[i];
        slist_size += cache_ptr->slist_ring_size[i];
    } /* end for */

    HDassert(cache_ptr->index_len == index_len);
    HDassert(cache_ptr->index_size == index_size);
    HDassert(cache_ptr->clean_index_size == clean_index_size);
    HDassert(cache_ptr->dirty_index_size == dirty_index_size);
    HDassert(cache_ptr->slist_len == slist_len);
    HDassert(cache_ptr->slist_size == slist_size);
#endif /* H5C_DO_SANITY_CHECKS */

#if H5C_DO_EXTREME_SANITY_CHECKS
    if (H5C_validate_protected_entry_list(cache_ptr) < 0 || H5C_validate_pinned_entry_list(cache_ptr) < 0 ||
        H5C_validate_lru_list(cache_ptr) < 0)
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "an extreme sanity check failed on entry")
#endif /* H5C_DO_EXTREME_SANITY_CHECKS */

    cache_ptr->flush_in_progress = TRUE;

    /* flush each ring, starting from the outermost ring and
     * working inward.
     */
    ring = H5C_RING_USER;
    while (ring < H5C_RING_NTYPES) {
        if (H5C__flush_candidates_in_ring(f, ring, entries_to_flush[ring], entries_to_clear[ring]) < 0)
            HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "flush candidates in ring failed")

        ring++;
    } /* end while */

done:
    cache_ptr->flush_in_progress = FALSE;

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5C__flush_candidate_entries() */

/*-------------------------------------------------------------------------
 * Function:    H5C__flush_candidates_in_ring
 *
 * Purpose:     Flush or clear (as indicated) the candidate entries
 *              contained in the specified cache and ring.  All candidate
 *              entries in rings outside the specified ring must have been
 *              flushed (or cleared) on entry.
 *
 *              Note that this function presumes that:
 *
 *              1) no candidate entries are protected,
 *
 *              2) all candidate entries are dirty, and
 *
 *              3) if a candidate entry has a dirty flush dependency
 *                 child, that child is also a candidate entry.
 *
 *              The function will fail if any of these preconditions are
 *              not met.
 *
 *              Candidate entries are marked by setting either the
 *              flush_immediately or the clear_on_unprotect flags in the
 *              cache entry (but not both).  Entries marked flush_immediately
 *              will be flushed, those marked clear_on_unprotect will be
 *              cleared.
 *
 *              Candidate entries residing in the LRU must be flushed
 *              (or cleared) in LRU order to avoid performance issues.
 *
 * Return:      Non-negative on success/Negative on failure.
 *
 * Programmer:  John Mainzer
 *              2/10/17
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5C__flush_candidates_in_ring(H5F_t *f, H5C_ring_t ring, unsigned entries_to_flush, unsigned entries_to_clear)
{
    H5C_t *  cache_ptr;
    hbool_t  progress;
    hbool_t  restart_scan    = FALSE;
    unsigned entries_flushed = 0;
    unsigned entries_cleared = 0;
#if H5C_DO_SANITY_CHECKS
    unsigned init_index_len;
#endif /* H5C_DO_SANITY_CHECKS */
    unsigned clear_flags =
        H5C__FLUSH_CLEAR_ONLY_FLAG | H5C__GENERATE_IMAGE_FLAG | H5C__UPDATE_PAGE_BUFFER_FLAG;
    unsigned           flush_flags = H5C__NO_FLAGS_SET;
    unsigned           op_flags;
    H5C_cache_entry_t *op_ptr;
    H5C_cache_entry_t *entry_ptr;
    herr_t             ret_value = SUCCEED;

    FUNC_ENTER_STATIC

    /* Sanity checks */
    HDassert(f);
    HDassert(f->shared);
    cache_ptr = f->shared->cache;
    HDassert(cache_ptr);
    HDassert(cache_ptr->magic == H5C__H5C_T_MAGIC);
    HDassert(cache_ptr->slist_ptr);
    HDassert(ring > H5C_RING_UNDEFINED);
    HDassert(ring < H5C_RING_NTYPES);

#if H5C_DO_EXTREME_SANITY_CHECKS
    if ((H5C_validate_protected_entry_list(cache_ptr) < 0) ||
        (H5C_validate_pinned_entry_list(cache_ptr) < 0) || (H5C_validate_lru_list(cache_ptr) < 0))
        HGOTO_ERROR(H5E_CACHE, H5E_SYSTEM, FAIL, "an extreme sanity check failed on entry")
#endif /* H5C_DO_EXTREME_SANITY_CHECKS */

#if H5C_DO_SANITY_CHECKS
    /* index len should not change */
    init_index_len = cache_ptr->index_len;
#endif /* H5C_DO_SANITY_CHECKS */

    /* Examine entries in the LRU list, and flush or clear all entries
     * so marked in the target ring.
     *
     * With the current implementation of flush dependencies, no entry
     * in the LRU can have flush dependency children -- thus one pass
     * through the LRU will be sufficient.
     *
     * It is possible that this will change -- hence the assertion.
     */
    restart_scan = FALSE;
    entry_ptr    = cache_ptr->LRU_tail_ptr;
    while (((entries_flushed < entries_to_flush) || (entries_cleared < entries_to_clear)) &&
           (entry_ptr != NULL)) {
        hbool_t            prev_is_dirty = FALSE;
        H5C_cache_entry_t *next_ptr;

        /* Entries in the LRU must not have flush dependency children */
        HDassert(entry_ptr->flush_dep_nchildren == 0);

        /* Remember dirty state of entry to advance to */
        if (entry_ptr->prev != NULL)
            prev_is_dirty = entry_ptr->prev->is_dirty;

        /* If the entry is in the ring */
        if (entry_ptr->ring == ring) {
            /* If this process needs to clear this entry. */
            if (entry_ptr->clear_on_unprotect) {
                HDassert(entry_ptr->is_dirty);

                /* Set entry and flags for operation */
                op_ptr   = entry_ptr;
                op_flags = clear_flags;

                /* Set next entry appropriately */
                next_ptr = entry_ptr->next;

                /* Reset entry flag */
                entry_ptr->clear_on_unprotect = FALSE;
                entries_cleared++;
            } /* end if */
            else if (entry_ptr->flush_immediately) {
                HDassert(entry_ptr->is_dirty);

                /* Set entry and flags for operation */
                op_ptr   = entry_ptr;
                op_flags = flush_flags;

                /* Set next entry appropriately */
                next_ptr = entry_ptr->next;

                /* Reset entry flag */
                entry_ptr->flush_immediately = FALSE;
                entries_flushed++;
            } /* end else-if */
            else {
                /* No operation for this entry */
                op_ptr = NULL;

                /* Set next entry appropriately */
                next_ptr = entry_ptr;
            } /* end else */

            /* Advance to next entry */
            entry_ptr = entry_ptr->prev;

            /* Check for operation */
            if (op_ptr) {
                /* reset entries_removed_counter and
                 * last_entry_removed_ptr prior to the call to
                 * H5C__flush_single_entry() so that we can spot
                 * unexpected removals of entries from the cache,
                 * and set the restart_scan flag if proceeding
                 * would be likely to cause us to scan an entry
                 * that is no longer in the cache.
                 *
                 * Note that as of this writing, this
                 * case cannot occur in the parallel case.
                 *
                 * Note also that there is no test code to verify
                 * that this code actually works (although similar code
                 * in the serial version exists and is tested).
                 */
                cache_ptr->entries_removed_counter = 0;
                cache_ptr->last_entry_removed_ptr  = NULL;

                if (H5C__flush_single_entry(f, op_ptr, op_flags) < 0)
                    HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "can't flush entry")

                if (cache_ptr->entries_removed_counter != 0 || cache_ptr->last_entry_removed_ptr != NULL)
                    restart_scan = TRUE;
            } /* end if */
        }     /* end if */
        else {
            /* Remember "next" pointer (after advancing entries) */
            next_ptr = entry_ptr;

            /* Advance to next entry */
            entry_ptr = entry_ptr->prev;
        } /* end else */

        /* Check for restarts, etc. */
        if ((entry_ptr != NULL) &&
            (restart_scan || (entry_ptr->is_dirty != prev_is_dirty) || (entry_ptr->next != next_ptr) ||
             entry_ptr->is_protected || entry_ptr->is_pinned)) {

            /* Something has happened to the LRU -- start over
             * from the tail.
             *
             * Recall that this code should be un-reachable at present,
             * as all the operations by entries on flush that could cause
             * it to be reachable are disallowed in the parallel case at
             * present.  Hence the following assertion which should be
             * removed if the above changes.
             */
            HDassert(!restart_scan);
            HDassert(entry_ptr->is_dirty == prev_is_dirty);
            HDassert(entry_ptr->next == next_ptr);
            HDassert(!entry_ptr->is_protected);
            HDassert(!entry_ptr->is_pinned);

            HDassert(FALSE); /* see comment above */

            restart_scan = FALSE;
            entry_ptr    = cache_ptr->LRU_tail_ptr;

            H5C__UPDATE_STATS_FOR_LRU_SCAN_RESTART(cache_ptr)
        } /* end if */
    }     /* end while */

    /* It is also possible that some of the cleared entries are on the
     * pinned list.  Must scan that also.
     *
     * Observe that in the case of the pinned entry list, most of the
     * entries will have flush dependency children.  As entries with
     * flush dependency children may not be flushed until all of their
     * children are clean, multiple passes throguh the pinned entry list
     * may be required.
     *
     * WARNING:
     *
     *  As we now allow unpinning, and removal of other entries as a side
     *  effect of flushing an entry, it is possible that the next entry
     *  in a PEL scan could either be no longer pinned, or no longer in
     *  the cache by the time we get to it.
     *
     *  At present, this should not be possible in this case, as we disallow
     *  such operations in the parallel version of the library.  However,
     *  this may change, and to that end, I have included code to detect
     *  such changes and cause this function to fail if they are detected.
     */
    progress = TRUE;
    while (progress && ((entries_flushed < entries_to_flush) || (entries_cleared < entries_to_clear))) {
        progress  = FALSE;
        entry_ptr = cache_ptr->pel_head_ptr;
        while ((entry_ptr != NULL) &&
               ((entries_flushed < entries_to_flush) || (entries_cleared < entries_to_clear))) {
            H5C_cache_entry_t *prev_ptr;
            hbool_t            next_is_dirty = FALSE;

            HDassert(entry_ptr->is_pinned);

            /* Remember dirty state of entry to advance to */
            if (entry_ptr->next != NULL)
                next_is_dirty = entry_ptr->next->is_dirty;

            if (entry_ptr->ring == ring && entry_ptr->flush_dep_ndirty_children == 0) {
                if (entry_ptr->clear_on_unprotect) {
                    HDassert(entry_ptr->is_dirty);

                    /* Set entry and flags for operation */
                    op_ptr   = entry_ptr;
                    op_flags = clear_flags;

                    /* Reset entry flag */
                    entry_ptr->clear_on_unprotect = FALSE;
                    entries_cleared++;
                    progress = TRUE;
                } /* end if */
                else if (entry_ptr->flush_immediately) {
                    HDassert(entry_ptr->is_dirty);

                    /* Set entry and flags for operation */
                    op_ptr   = entry_ptr;
                    op_flags = flush_flags;

                    /* Reset entry flag */
                    entry_ptr->flush_immediately = FALSE;
                    entries_flushed++;
                    progress = TRUE;
                } /* end else-if */
                else
                    /* No operation for this entry */
                    op_ptr = NULL;

                /* Check for operation */
                if (op_ptr) {
                    /* reset entries_removed_counter and
                     * last_entry_removed_ptr prior to the call to
                     * H5C__flush_single_entry() so that we can spot
                     * unexpected removals of entries from the cache,
                     * and set the restart_scan flag if proceeding
                     * would be likely to cause us to scan an entry
                     * that is no longer in the cache.
                     *
                     * Note that as of this writing,  this
                     * case cannot occur in the parallel case.
                     *
                     * Note also that there is no test code to verify
                     * that this code actually works (although similar code
                     * in the serial version exists and is tested).
                     */
                    cache_ptr->entries_removed_counter = 0;
                    cache_ptr->last_entry_removed_ptr  = NULL;

                    /* Add this entry to the list of entries to collectively write
                     *
                     * This comment is misleading -- the entry will be added to the
                     * collective write list only if said list exists.
                     *
                     *                                    JRM -- 2/9/17
                     */
                    if (H5C__flush_single_entry(f, op_ptr, op_flags) < 0)
                        HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "can't flush entry")

                    if (cache_ptr->entries_removed_counter != 0 || cache_ptr->last_entry_removed_ptr != NULL)
                        restart_scan = TRUE;
                } /* end if */
            }     /* end if */

            /* Remember "previous" pointer (after advancing entries) */
            prev_ptr = entry_ptr;

            /* Advance to next entry */
            entry_ptr = entry_ptr->next;

            /* Check for restarts, etc. */
            if ((entry_ptr != NULL) &&
                (restart_scan || (entry_ptr->is_dirty != next_is_dirty) || (entry_ptr->prev != prev_ptr) ||
                 entry_ptr->is_protected || !entry_ptr->is_pinned)) {
                /* Something has happened to the pinned entry list -- start
                 * over from the head.
                 *
                 * Recall that this code should be un-reachable at present,
                 * as all the operations by entries on flush that could cause
                 * it to be reachable are disallowed in the parallel case at
                 * present.  Hence the following assertion which should be
                 * removed if the above changes.
                 */

                HDassert(!restart_scan);
                HDassert(entry_ptr->is_dirty == next_is_dirty);
                HDassert(entry_ptr->prev == prev_ptr);
                HDassert(!entry_ptr->is_protected);
                HDassert(entry_ptr->is_pinned);

                HDassert(FALSE); /* see comment above */

                restart_scan = FALSE;

                entry_ptr = cache_ptr->pel_head_ptr;

                /* we don't keeps stats for pinned entry list scan
                 * restarts.  If this code ever becomes reachable,
                 * define the necessary field, and implement the
                 * the following macro:
                 *
                 * H5C__UPDATE_STATS_FOR_PEL_SCAN_RESTART(cache_ptr)
                 */
            } /* end if */
        }     /* end while ( ( entry_ptr != NULL ) &&
               *             ( ( entries_flushed > entries_to_flush ) ||
               *               ( entries_cleared > entries_to_clear ) ) )
               */
    }         /* end while ( ( ( entries_flushed > entries_to_flush ) ||
               *               ( entries_cleared > entries_to_clear ) ) &&
               *             ( progress ) )
               */

#if H5C_DO_SANITY_CHECKS
    HDassert(init_index_len == cache_ptr->index_len);
#endif /* H5C_DO_SANITY_CHECKS */

    if (entries_flushed != entries_to_flush || entries_cleared != entries_to_clear) {
        entry_ptr = cache_ptr->il_head;
        while (entry_ptr != NULL) {
            HDassert(!entry_ptr->clear_on_unprotect || (entry_ptr->ring > ring));
            HDassert(!entry_ptr->flush_immediately || (entry_ptr->ring > ring));
            entry_ptr = entry_ptr->il_next;
        } /* end while */

        HGOTO_ERROR(H5E_CACHE, H5E_CANTFLUSH, FAIL, "can't flush/clear all entries")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5C__flush_candidates_in_ring() */
#endif /* H5_HAVE_PARALLEL */
