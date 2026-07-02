/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the LICENSE file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/****************/
/* Module Setup */
/****************/
#include "H5module.h" /* This source code file is part of the H5 module */

/***********/
/* Headers */
/***********/
#include "H5private.h"   /* Generic Functions                        */
#include "H5ACprivate.h" /* Metadata cache                           */
#include "H5CXprivate.h" /* API Contexts                             */
#include "H5Dprivate.h"  /* Datasets                                 */
#include "H5Eprivate.h"  /* Error handling                           */
#include "H5FLprivate.h" /* Free lists                               */
#include "H5FSprivate.h" /* File free space                          */
#include "H5Lprivate.h"  /* Links                                    */
#include "H5MMprivate.h" /* Memory management                        */
#include "H5Pprivate.h"  /* Property lists                           */
#include "H5PLprivate.h" /* Plugins                                  */
#include "H5SLprivate.h" /* Skip lists                               */
#include "H5Tprivate.h"  /* Datatypes                                */
#include "H5TSprivate.h" /* Threadsafety                             */

/****************/
/* Local Macros */
/****************/

/******************/
/* Local Typedefs */
/******************/

/********************/
/* Package Typedefs */
/********************/

/* Node for list of 'atclose' routines to invoke at library shutdown */
typedef struct H5_atclose_node_t {
    H5_atclose_func_t         func; /* Function to invoke */
    void                     *ctx;  /* Context to pass to function */
    struct H5_atclose_node_t *next; /* Pointer to next node in list */
} H5_atclose_node_t;

/********************/
/* Local Prototypes */
/********************/
static void H5__debug_mask(const char *);
#ifdef H5_HAVE_PARALLEL
static int H5__mpi_delete_cb(MPI_Comm comm, int keyval, void *attr_val, int *flag);
#endif /*H5_HAVE_PARALLEL*/
static herr_t H5_check_version(unsigned majnum, unsigned minnum, unsigned relnum);

/*********************/
/* Package Variables */
/*********************/

/* Package initialization variable */
bool H5_PKG_INIT_VAR = false;

/*****************************/
/* Library Private Variables */
/*****************************/

/* Library known incompatible minor versions; develop releases are incompatible
 * by design. 999 is entered for testing an exception as a minor version that
 * will never occur.  Any released minor version found to be truly incompatible
 * (this should never happen) should be added to the list with 999.  999 alone
 * in the list indicates that there are no incompatible minor versions. */
static const unsigned VERS_MINOR_EXCEPTIONS[] = {999};
/* The size should be set to the number of minor version exceptions in the list. */
static const unsigned VERS_MINOR_EXCEPTIONS_SIZE = 1;

/* Library init / term status (global) */
bool H5_libinit_g = false; /* Library hasn't been initialized */
bool H5_libterm_g = false; /* Library isn't being shutdown */

char        H5_lib_vers_info_g[] = H5_VERS_INFO;
static bool H5_dont_atexit_g     = false;
H5_debug_t  H5_debug_g; /* debugging info */

/*******************/
/* Local Variables */
/*******************/

/* Linked list of registered 'atclose' functions to invoke at library shutdown */
static H5_atclose_node_t *H5_atclose_head = NULL;

/* Declare a free list to manage the H5_atclose_node_t struct */
H5FL_DEFINE_STATIC(H5_atclose_node_t);

/*--------------------------------------------------------------------------
NAME
    H5__init_package -- Initialize interface-specific information
USAGE
    herr_t H5__init_package()
RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.
--------------------------------------------------------------------------*/
herr_t
H5__init_package(void)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    /* Run the library initialization routine, if it hasn't already ran */
    if (!H5_INIT_GLOBAL && !H5_TERM_GLOBAL)
        if (H5_init_library() < 0)
            HGOTO_ERROR(H5E_LIB, H5E_CANTINIT, FAIL, "unable to initialize library");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5__init_package() */

/*--------------------------------------------------------------------------
 * NAME
 *   H5_init_library -- Initialize library-global information
 * USAGE
 *    herr_t H5_init_library()
 *
 * RETURNS
 *    Non-negative on success/Negative on failure
 *
 * DESCRIPTION
 *    Initializes any library-global data or routines.
 *
 *--------------------------------------------------------------------------
 */
herr_t
H5_init_library(void)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(FAIL)

    /* Run the library initialization routine, if it hasn't already run */
    if (H5_INIT_GLOBAL || H5_TERM_GLOBAL)
        HGOTO_DONE(SUCCEED);

    /* Check library version */
    /* (Will abort() on failure) */
    H5_check_version(H5_VERS_MAJOR, H5_VERS_MINOR, H5_VERS_RELEASE);

    /* Set the 'library initialized' flag as early as possible, to avoid
     * possible re-entrancy.
     */
    H5_INIT_GLOBAL = true;

    /* Make sure we picked a good type for ssize_t if it wasn't present */
    HDcompile_assert(sizeof(size_t) == sizeof(ssize_t));

#ifdef H5_HAVE_PARALLEL
    {
        int mpi_initialized;
        int mpi_finalized;
        int mpi_code;

        MPI_Initialized(&mpi_initialized);
        MPI_Finalized(&mpi_finalized);

        /* add an attribute on MPI_COMM_SELF to call H5_term_library
           when it is destroyed, i.e. on MPI_Finalize */
        if (mpi_initialized && !mpi_finalized) {
            int key_val;

            if (MPI_SUCCESS != (mpi_code = MPI_Comm_create_keyval(
                                    MPI_COMM_NULL_COPY_FN, (MPI_Comm_delete_attr_function *)H5__mpi_delete_cb,
                                    &key_val, NULL)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Comm_create_keyval failed", mpi_code)

            if (MPI_SUCCESS != (mpi_code = MPI_Comm_set_attr(MPI_COMM_SELF, key_val, NULL)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Comm_set_attr failed", mpi_code)

            if (MPI_SUCCESS != (mpi_code = MPI_Comm_free_keyval(&key_val)))
                HMPI_GOTO_ERROR(FAIL, "MPI_Comm_free_keyval failed", mpi_code)
        }
    }
#endif /*H5_HAVE_PARALLEL*/

    /*
     * Make sure the package information is updated.
     */
    memset(&H5_debug_g, 0, sizeof H5_debug_g);
    H5_debug_g.pkg[H5_PKG_A].name  = "a";
    H5_debug_g.pkg[H5_PKG_AC].name = "ac";
    H5_debug_g.pkg[H5_PKG_B].name  = "b";
    H5_debug_g.pkg[H5_PKG_D].name  = "d";
    H5_debug_g.pkg[H5_PKG_E].name  = "e";
    H5_debug_g.pkg[H5_PKG_F].name  = "f";
    H5_debug_g.pkg[H5_PKG_G].name  = "g";
    H5_debug_g.pkg[H5_PKG_HG].name = "hg";
    H5_debug_g.pkg[H5_PKG_HL].name = "hl";
    H5_debug_g.pkg[H5_PKG_I].name  = "i";
    H5_debug_g.pkg[H5_PKG_M].name  = "m";
    H5_debug_g.pkg[H5_PKG_MF].name = "mf";
    H5_debug_g.pkg[H5_PKG_MM].name = "mm";
    H5_debug_g.pkg[H5_PKG_O].name  = "o";
    H5_debug_g.pkg[H5_PKG_P].name  = "p";
    H5_debug_g.pkg[H5_PKG_S].name  = "s";
    H5_debug_g.pkg[H5_PKG_T].name  = "t";
    H5_debug_g.pkg[H5_PKG_V].name  = "v";
    H5_debug_g.pkg[H5_PKG_VL].name = "vl";
    H5_debug_g.pkg[H5_PKG_Z].name  = "z";

    /*
     * Install atexit() library cleanup routines unless the H5dont_atexit()
     * has been called.  Once we add something to the atexit() list it stays
     * there permanently, so we set H5_dont_atexit_g after we add it to prevent
     * adding it again later if the library is closed and reopened.
     */
    if (!H5_dont_atexit_g) {

#ifdef H5_HAVE_THREADSAFE_API
        /* Clean up thread resources.
         *
         * This must be pushed before the library cleanup code so it's
         * executed in LIFO order (i.e., last).
         */
        (void)atexit(H5TS_term_package);
#endif /* H5_HAVE_THREADSAFE_API */

        /* Normal library termination code */
        (void)atexit(H5_term_library);

        H5_dont_atexit_g = true;
    } /* end if */

    /*
     * Initialize interfaces that use macros of the form "(H5OPEN <var>)", so
     * that the variable returned through the macros has been initialized.
     * Also initialize some interfaces that might not be able to initialize
     * themselves soon enough.
     *
     * Interfaces returning variables through a macro: H5E, H5FD, H5O, H5P, H5T
     *
     * The link interface needs to be initialized so that the external link
     *   class is registered.
     *
     * The FS module needs to be initialized as a result of the fix for HDFFV-10160:
     *   It might not be initialized during normal file open.
     *   When the application does not close the file, routines in the module might
     *   be called via H5_term_library() when shutting down the file.
     *
     * The dataspace interface needs to be initialized so that future IDs for
     *   dataspaces work.
     *
     * The VFD & VOL interfaces need to be initialized before the H5P interface
     *   so that the default VFD and default VOL connector are ready for the
     *   default FAPL.
     *
     */
    if (H5E_init() < 0)
        HGOTO_ERROR(H5E_FUNC, H5E_CANTINIT, FAIL, "unable to initialize error interface");
    if (H5FD_init() < 0)
        HGOTO_ERROR(H5E_FUNC, H5E_CANTINIT, FAIL, "unable to initialize VFL interface");
    if (H5VL_init_phase1() < 0)
        HGOTO_ERROR(H5E_FUNC, H5E_CANTINIT, FAIL, "unable to initialize vol interface");
    if (H5P_init_phase1() < 0)
        HGOTO_ERROR(H5E_FUNC, H5E_CANTINIT, FAIL, "unable to initialize property list interface");
    if (H5L_init() < 0)
        HGOTO_ERROR(H5E_FUNC, H5E_CANTINIT, FAIL, "unable to initialize link interface");
    if (H5O_init() < 0)
        HGOTO_ERROR(H5E_FUNC, H5E_CANTINIT, FAIL, "unable to initialize object interface");
    if (H5FS_init() < 0)
        HGOTO_ERROR(H5E_FUNC, H5E_CANTINIT, FAIL, "unable to initialize FS interface");
    if (H5S_init() < 0)
        HGOTO_ERROR(H5E_FUNC, H5E_CANTINIT, FAIL, "unable to initialize dataspace interface");
    if (H5T_init() < 0)
        HGOTO_ERROR(H5E_FUNC, H5E_CANTINIT, FAIL, "unable to initialize datatype interface");

    /* Finish initializing interfaces that depend on the interfaces above */
    if (H5P_init_phase2() < 0)
        HGOTO_ERROR(H5E_FUNC, H5E_CANTINIT, FAIL, "unable to initialize property list interface");
    if (H5VL_init_phase2() < 0)
        HGOTO_ERROR(H5E_FUNC, H5E_CANTINIT, FAIL, "unable to initialize vol interface");

    /* Debugging? */
    H5__debug_mask("-all");
    H5__debug_mask(getenv("HDF5_DEBUG"));

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5_init_library() */

/*-------------------------------------------------------------------------
 * Function:    H5_term_library
 *
 * Purpose:    Terminate interfaces in a well-defined order due to
 *        dependencies among the interfaces, then terminate
 *        library-specific data.
 *
 * Return:    void
 *
 *-------------------------------------------------------------------------
 */
void
H5_term_library(void)
{
    int         pending, ntries = 0, n;
    size_t      at = 0;
    char        loop[1024];
    H5E_auto2_t func;
    H5CX_node_t api_ctx = {{0}, NULL}; /* API context node to push */

    /* Acquire the API lock */
    H5_API_SETUP_PUBLIC_API_VARS
    H5_API_LOCK

    /* Don't do anything if the library is already closed */
    if (!H5_INIT_GLOBAL)
        goto done;

    /* Indicate that the library is being shut down */
    H5_TERM_GLOBAL = true;

    /* Push the API context without checking for errors */
    H5CX_push(&api_ctx);

    /* Check if we should display error output */
    (void)H5E_get_default_auto_func(&func);

    /* Iterate over the list of 'atclose' callbacks that have been registered */
    if (H5_atclose_head) {
        H5_atclose_node_t *curr_atclose; /* Current 'atclose' node */

        /* Iterate over all 'atclose' nodes, making callbacks */
        curr_atclose = H5_atclose_head;
        while (curr_atclose) {
            H5_atclose_node_t *tmp_atclose; /* Temporary pointer to 'atclose' node */

            /* Prepare & restore library for user callback */
            H5_BEFORE_USER_CB_NOCHECK
                {
                    /* Invoke callback, providing context */
                    (*curr_atclose->func)(curr_atclose->ctx);
                }
            H5_AFTER_USER_CB_NOCHECK

            /* Advance to next node and free this one */
            tmp_atclose  = curr_atclose;
            curr_atclose = curr_atclose->next;
            H5FL_FREE(H5_atclose_node_t, tmp_atclose);
        } /* end while */

        /* Reset list head, in case library is re-initialized */
        H5_atclose_head = NULL;
    } /* end if */

    /*
     * Terminate each interface. The termination functions return a positive
     * value if they do something that might affect some other interface in a
     * way that would necessitate some cleanup work in the other interface.
     */
#define DOWN(F)                                                                                              \
    (((n = H5##F##_term_package()) && (at + 8) < sizeof loop)                                                \
         ? (sprintf(loop + at, "%s%s", (at ? "," : ""), #F), at += strlen(loop + at), n)                     \
         : ((n > 0 && (at + 5) < sizeof loop) ? (sprintf(loop + at, "..."), at += strlen(loop + at), n)      \
                                              : n))

    do {
        pending = 0;

        /* Try to organize these so the "higher" level components get shut
         * down before "lower" level components that they might rely on. -QAK
         */

        /* Close the event sets first, so that all asynchronous operations
         * complete before anything else attempts to shut down.
         */
        pending += DOWN(ES);

        /* Close down the user-facing interfaces, after the event sets */
        if (pending == 0) {
            /* Close the interfaces dependent on others */
            pending += DOWN(L);

            /* Close the "top" of various interfaces (IDs, etc) but don't shut
             * down the whole interface yet, so that the object header messages
             * get serialized correctly for entries in the metadata cache and the
             * symbol table entry in the superblock gets serialized correctly, etc.
             * all of which is performed in the 'F' shutdown.
             */
            pending += DOWN(A_top);
            pending += DOWN(D_top);
            pending += DOWN(G_top);
            pending += DOWN(M_top);
            pending += DOWN(S_top);
            pending += DOWN(T_top);
        } /* end if */

        /* Don't shut down the file code until objects in files are shut down */
        if (pending == 0)
            pending += DOWN(F);

        /* Don't shut down the property list code until all objects that might
         * use property lists are shut down */
        if (pending == 0)
            pending += DOWN(P);

        /* Wait to shut down the "bottom" of various interfaces until the
         * files are closed, so pieces of the file can be serialized
         * correctly.
         */
        if (pending == 0) {
            /* Shut down the "bottom" of the attribute, dataset, group,
             * dataspace, and datatype interfaces, fully closing
             * out the interfaces now.
             */
            pending += DOWN(A);
            pending += DOWN(D);
            pending += DOWN(G);
            pending += DOWN(M);
            pending += DOWN(S);
            pending += DOWN(T);
        } /* end if */

        /* Don't shut down "low-level" components until "high-level" components
         * have successfully shut down.  This prevents property lists and IDs
         * from being closed "out from underneath" of the high-level objects
         * that depend on them. -QAK
         */
        if (pending == 0) {
            pending += DOWN(AC);
            /* Shut down the "pluggable" interfaces, before the plugin framework */
            pending += DOWN(Z);
            pending += DOWN(FD);
            pending += DOWN(VL);
            /* Don't shut down the plugin code until all "pluggable" interfaces (Z, FD, PL) are shut down */
            if (pending == 0)
                pending += DOWN(PL);
            /* Don't shut down the error code until other APIs which use it are shut down */
            if (pending == 0)
                pending += DOWN(E);
            /* Don't shut down the ID code until other APIs which use them are shut down */
            if (pending == 0)
                pending += DOWN(I);
            /* Don't shut down the skip list code until everything that uses it is down */
            if (pending == 0)
                pending += DOWN(SL);
            /* Don't shut down the free list code until everything that uses it is down */
            if (pending == 0)
                pending += DOWN(FL);
            /* Don't shut down the API context code until _everything_ else is down */
            if (pending == 0)
                pending += DOWN(CX);
        } /* end if */
    } while (pending && ntries++ < 100);

    if (pending) {
        /* Only display the error message if the user is interested in them. */
        if (func) {
            fprintf(stderr, "HDF5: infinite loop closing library\n");
            fprintf(stderr, "      %s\n", loop);
#ifndef NDEBUG
            abort();
#endif
        }
    }

    /* Free open debugging streams */
    while (H5_debug_g.open_stream) {
        H5_debug_open_stream_t *tmp_open_stream;

        tmp_open_stream = H5_debug_g.open_stream;
        (void)fclose(H5_debug_g.open_stream->stream);
        H5_debug_g.open_stream = H5_debug_g.open_stream->next;
        (void)H5MM_free(tmp_open_stream);
    } /* end while */

    /* Reset flag indicating that the library is being shut down */
    H5_TERM_GLOBAL = false;

    /* Mark library as closed */
    H5_INIT_GLOBAL = false;

    /* Don't pop the API context (i.e. H5CX_pop), since it's been shut down already */

done:
    /* Release API lock */
    H5_API_UNLOCK

    return;
} /* end H5_term_library() */

/*-------------------------------------------------------------------------
 * Function:    H5dont_atexit
 *
 * Purpose:    Indicates that the library is not to clean up after itself
 *        when the application exits by calling exit() or returning
 *        from main().  This function must be called before any other
 *        HDF5 function or constant is used or it will have no effect.
 *
 *        If this function is used then certain memory buffers will not
 *        be de-allocated nor will open files be flushed automatically.
 *        The application may still call H5close() explicitly to
 *        accomplish these things.
 *
 * Return:    Success:    non-negative
 *
 *        Failure:    negative if this function is called more than
 *                once or if it is called too late.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5dont_atexit(void)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT_NOERR

    if (H5_dont_atexit_g)
        ret_value = FAIL;
    else
        H5_dont_atexit_g = true;

    FUNC_LEAVE_API_NOERR(ret_value)
} /* end H5dont_atexit() */

/*-------------------------------------------------------------------------
 * Function:    H5garbage_collect
 *
 * Purpose:    Walks through all the garbage collection routines for the
 *        library, which are supposed to free any unused memory they have
 *        allocated.
 *
 *      These should probably be registered dynamically in a linked list of
 *          functions to call, but there aren't that many right now, so we
 *          hard-wire them...
 *
 * Return:    Success:    non-negative
 *
 *        Failure:    negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5garbage_collect(void)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_API(FAIL)

    /* Call the garbage collection routines in the library */
    if (H5FL_garbage_coll() < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGC, FAIL, "can't garbage collect objects");

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5garbage_collect() */

/*-------------------------------------------------------------------------
 * Function:    H5set_free_list_limits
 *
 * Purpose:    Sets limits on the different kinds of free lists.  Setting a value
 *      of -1 for a limit means no limit of that type.  These limits are global
 *      for the entire library.  Each "global" limit only applies to free lists
 *      of that type, so if an application sets a limit of 1 MB on each of the
 *      global lists, up to 3 MB of total storage might be allocated (1MB on
 *      each of regular, array and block type lists).
 *
 *      The settings for block free lists are duplicated to factory free lists.
 *      Factory free list limits cannot be set independently currently.
 *
 * Parameters:
 *  int reg_global_lim;  IN: The limit on all "regular" free list memory used
 *  int reg_list_lim;    IN: The limit on memory used in each "regular" free list
 *  int arr_global_lim;  IN: The limit on all "array" free list memory used
 *  int arr_list_lim;    IN: The limit on memory used in each "array" free list
 *  int blk_global_lim;  IN: The limit on all "block" free list memory used
 *  int blk_list_lim;    IN: The limit on memory used in each "block" free list
 *
 * Return:    Success:    non-negative
 *
 *        Failure:    negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5set_free_list_limits(int reg_global_lim, int reg_list_lim, int arr_global_lim, int arr_list_lim,
                       int blk_global_lim, int blk_list_lim)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_API(FAIL)

    /* Call the free list function to actually set the limits */
    if (H5FL_set_free_list_limits(reg_global_lim, reg_list_lim, arr_global_lim, arr_list_lim, blk_global_lim,
                                  blk_list_lim, blk_global_lim, blk_list_lim) < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTSET, FAIL, "can't set garbage collection limits");

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5set_free_list_limits() */

/*-------------------------------------------------------------------------
 * Function:    H5get_free_list_sizes
 *
 * Purpose:    Gets the current size of the different kinds of free lists that
 *    the library uses to manage memory.  The free list sizes can be set with
 *    H5set_free_list_limits and garbage collected with H5garbage_collect.
 *      These lists are global for the entire library.
 *
 * Parameters:
 *  size_t *reg_size;    OUT: The current size of all "regular" free list memory used
 *  size_t *arr_size;    OUT: The current size of all "array" free list memory used
 *  size_t *blk_size;    OUT: The current size of all "block" free list memory used
 *  size_t *fac_size;    OUT: The current size of all "factory" free list memory used
 *
 * Return:    Success:    non-negative
 *        Failure:    negative
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5get_free_list_sizes(size_t *reg_size /*out*/, size_t *arr_size /*out*/, size_t *blk_size /*out*/,
                      size_t *fac_size /*out*/)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)

    /* Call the free list function to actually get the sizes */
    if (H5FL_get_free_list_sizes(reg_size, arr_size, blk_size, fac_size) < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGET, FAIL, "can't get garbage collection sizes");

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5get_free_list_sizes() */

/*-------------------------------------------------------------------------
 * Function:    H5__debug_mask
 *
 * Purpose:     Set runtime debugging flags according to the string S.  The
 *              string should contain file numbers and package names
 *              separated by other characters. A file number applies to all
 *              following package names up to the next file number. The
 *              initial file number is `2' (the standard error stream). Each
 *              package name can be preceded by a `+' or `-' to add or remove
 *              the package from the debugging list (`+' is the default). The
 *              special name `all' means all packages.
 *
 *              The name `trace' indicates that API tracing is to be turned
 *              on or off.
 *
 *              The name 'ttop' indicates that only top-level API calls
 *              should be shown. This also turns on tracing as if the
 *              'trace' word was shown.
 *
 * Return:      void
 *
 *-------------------------------------------------------------------------
 */
static void
H5__debug_mask(const char *s)
{
    FILE  *stream = stderr;
    char   pkg_name[32], *rest;
    size_t i;
    bool   clear;

    while (s && *s) {

        if (isalpha(*s) || '-' == *s || '+' == *s) {

            /* Enable or Disable debugging? */
            if ('-' == *s) {
                clear = true;
                s++;
            }
            else if ('+' == *s) {
                clear = false;
                s++;
            }
            else {
                clear = false;
            } /* end if */

            /* Get the name */
            for (i = 0; isalpha(*s); i++, s++)
                if (i < sizeof pkg_name)
                    pkg_name[i] = *s;
            pkg_name[MIN(sizeof(pkg_name) - 1, i)] = '\0';

            /* Trace, all, or one? */
            if (!strcmp(pkg_name, "trace")) {
                H5_debug_g.trace = clear ? NULL : stream;
            }
            else if (!strcmp(pkg_name, "ttop")) {
                H5_debug_g.trace = stream;
                H5_debug_g.ttop  = (bool)!clear;
            }
            else if (!strcmp(pkg_name, "ttimes")) {
                H5_debug_g.trace  = stream;
                H5_debug_g.ttimes = (bool)!clear;
            }
            else if (!strcmp(pkg_name, "all")) {
                for (i = 0; i < (size_t)H5_NPKGS; i++)
                    H5_debug_g.pkg[i].stream = clear ? NULL : stream;
            }
            else {
                for (i = 0; i < (size_t)H5_NPKGS; i++) {
                    if (!strcmp(H5_debug_g.pkg[i].name, pkg_name)) {
                        H5_debug_g.pkg[i].stream = clear ? NULL : stream;
                        break;
                    } /* end if */
                }     /* end for */
                if (i >= (size_t)H5_NPKGS)
                    fprintf(stderr, "HDF5_DEBUG: ignored %s\n", pkg_name);
            } /* end if-else */
        }
        else if (isdigit(*s)) {
            int                     fd = (int)strtol(s, &rest, 0);
            H5_debug_open_stream_t *open_stream;

            if ((stream = HDfdopen(fd, "w")) != NULL) {
                (void)HDsetvbuf(stream, NULL, _IOLBF, (size_t)0);

                if (NULL ==
                    (open_stream = (H5_debug_open_stream_t *)H5MM_malloc(sizeof(H5_debug_open_stream_t)))) {
                    (void)fclose(stream);
                    return;
                } /* end if */

                open_stream->stream    = stream;
                open_stream->next      = H5_debug_g.open_stream;
                H5_debug_g.open_stream = open_stream;
            } /* end if */

            s = rest;
        }
        else {
            s++;
        } /* end if-else */
    }     /* end while */
} /* end H5__debug_mask() */

#ifdef H5_HAVE_PARALLEL

/*-------------------------------------------------------------------------
 * Function:    H5__mpi_delete_cb
 *
 * Purpose:    Callback attribute on MPI_COMM_SELF to terminate the HDF5
 *              library when the communicator is destroyed, i.e. on MPI_Finalize.
 *
 * Return:    MPI_SUCCESS
 *
 *-------------------------------------------------------------------------
 */
static int
H5__mpi_delete_cb(MPI_Comm H5_ATTR_UNUSED comm, int H5_ATTR_UNUSED keyval, void H5_ATTR_UNUSED *attr_val,
                  int H5_ATTR_UNUSED *flag)
{
    H5_term_library();
    return MPI_SUCCESS;
}
#endif /*H5_HAVE_PARALLEL*/

/*-------------------------------------------------------------------------
 * Function:    H5get_libversion
 *
 * Purpose:    Returns the library version numbers through arguments. MAJNUM
 *        will be the major revision number of the library, MINNUM the
 *        minor revision number, and RELNUM the release revision number.
 *
 * Note:    When printing an HDF5 version number it should be printed as
 *
 *         printf("%u.%u.%u", maj, min, rel)        or
 *        printf("version %u.%u release %u", maj, min, rel)
 *
 * Return:    Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5get_libversion(unsigned *majnum /*out*/, unsigned *minnum /*out*/, unsigned *relnum /*out*/)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_API(FAIL)

    /* Set the version information */
    if (majnum)
        *majnum = H5_VERS_MAJOR;
    if (minnum)
        *minnum = H5_VERS_MINOR;
    if (relnum)
        *relnum = H5_VERS_RELEASE;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5get_libversion() */

/*-------------------------------------------------------------------------
 * Function:    H5__check_version
 *
 * Purpose:     Internal routine which Verifies that the arguments match the
 *              version numbers compiled into the library.
 *
 *              Within major.minor.release version, the expectation
 *              is that all minor versions are compatible, exceptions to
 *              this rule must be added to the VERS_MINOR_EXCEPTIONS list.
 *
 * Return:      Success:    SUCCEED
 *              Failure:    abort()
 *
 *-------------------------------------------------------------------------
 */
#define VERSION_MISMATCH_WARNING                                                                             \
    "Warning! ***HDF5 library version mismatched error***\n"                                                 \
    "The HDF5 header files used to compile this application do not match\n"                                  \
    "the version used by the HDF5 library to which this application is linked.\n"                            \
    "Data corruption or segmentation faults may occur if the application continues.\n"                       \
    "This can happen when an application was compiled by one version of HDF5 but\n"                          \
    "linked with a different version of static or shared HDF5 library.\n"                                    \
    "You should recompile the application or check your shared library related\n"                            \
    "settings such as 'LD_LIBRARY_PATH'.\n"
#define MINOR_VERSION_MISMATCH_WARNING                                                                       \
    "Warning! ***HDF5 library minor version mismatched error***\n"                                           \
    "The HDF5 header files used to compile this application are not compatible with\n"                       \
    "the version used by the HDF5 library to which this application is linked.\n"                            \
    "Data corruption or segmentation faults may occur if the application continues.\n"                       \
    "This can happen when an application was compiled by one version of HDF5 but\n"                          \
    "linked with an incompatible version of static or shared HDF5 library.\n"                                \
    "You should recompile the application or check your shared library related\n"                            \
    "settings such as 'LD_LIBRARY_PATH'.\n"
#define MINOR_VERSION_FORWARD_COMPATIBLE_WARNING                                                             \
    "Warning! ***HDF5 library minor version forward compatibility error***\n"                                \
    "The HDF5 header files used to compile this application are from a newer\n"                              \
    "version of the HDF5 library than the one to which this application is linked.\n"                        \
    "Data corruption or segmentation faults may occur if the application continues.\n"                       \
    "This can happen when an application was compiled by a newer version of HDF5 but\n"                      \
    "linked with an older version of static or shared HDF5 library.\n"                                       \
    "You should recompile the application or check your shared library related\n"                            \
    "settings such as 'LD_LIBRARY_PATH'.\n"

static herr_t
H5_check_version(unsigned majnum, unsigned minnum, unsigned relnum)
{
    char                lib_str[256];
    char                substr[]                 = H5_VERS_SUBRELEASE;
    static bool         checked                  = false; /* If we've already checked the version info */
    static unsigned int disable_version_check    = 0;     /* Set if the version check should be disabled */
    static const char  *version_mismatch_warning = VERSION_MISMATCH_WARNING;
    static const char  *minor_version_mismatch_warning           = MINOR_VERSION_MISMATCH_WARNING;
    static const char  *minor_version_forward_compatible_warning = MINOR_VERSION_FORWARD_COMPATIBLE_WARNING;
    herr_t              ret_value                                = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Don't check again, if we already have */
    if (checked)
        HGOTO_DONE(SUCCEED);

    {
        const char *s; /* Environment string for disabling version check */

        /* Allow different versions of the header files and library? */
        s = getenv("HDF5_DISABLE_VERSION_CHECK");

        if (s && isdigit(*s))
            disable_version_check = (unsigned int)strtol(s, NULL, 0);
    }

    /* H5_VERS_MAJOR must match */
    if (H5_VERS_MAJOR != majnum) {
        switch (disable_version_check) {
            case 0:
                fprintf(stderr, "%s%s", version_mismatch_warning,
                        "You can, at your own risk, disable this warning by setting the environment\n"
                        "variable 'HDF5_DISABLE_VERSION_CHECK' to a value of '1'.\n"
                        "Setting it to 2 or higher will suppress the warning messages totally.\n");
                /* Mention the versions we are referring to */
                fprintf(stderr, "Headers are %u.%u.%u, library is %u.%u.%u\n", majnum, minnum, relnum,
                        (unsigned)H5_VERS_MAJOR, (unsigned)H5_VERS_MINOR, (unsigned)H5_VERS_RELEASE);
                /* Show library build settings if available */
                fprintf(stderr, "%s", H5build_settings);

                /* Bail out now. */
                fputs("Bye...\n", stderr);
                abort();
            case 1:
                /* continue with a warning */
                /* Note that the warning message is embedded in the format string.*/
                fprintf(stderr,
                        "%s'HDF5_DISABLE_VERSION_CHECK' "
                        "environment variable is set to %d, application will\n"
                        "continue at your own risk.\n",
                        version_mismatch_warning, disable_version_check);
                /* Mention the versions we are referring to */
                fprintf(stderr, "Headers are %u.%u.%u, library is %u.%u.%u\n", majnum, minnum, relnum,
                        (unsigned)H5_VERS_MAJOR, (unsigned)H5_VERS_MINOR, (unsigned)H5_VERS_RELEASE);
                /* Show library build settings if available */
                fprintf(stderr, "%s", H5build_settings);
                break;
            default:
                /* 2 or higher: continue silently */
                break;
        } /* end switch */

    } /* end if (H5_VERS_MAJOR != majnum) */

    /* H5_VERS_MINOR should be compatible, we will only add checks for exceptions */
    /* Library develop minor versions are incompatible by design */
    if (H5_VERS_MINOR != minnum) {
        for (unsigned i = 0; i < VERS_MINOR_EXCEPTIONS_SIZE; i++) {
            /* Check for incompatible headers or incompatible library */
            if (VERS_MINOR_EXCEPTIONS[i] == minnum || VERS_MINOR_EXCEPTIONS[i] == H5_VERS_MINOR) {
                switch (disable_version_check) {
                    case 0:
                        fprintf(stderr, "%s%s", minor_version_mismatch_warning,
                                "You can, at your own risk, disable this warning by setting the environment\n"
                                "variable 'HDF5_DISABLE_VERSION_CHECK' to a value of '1'.\n"
                                "Setting it to 2 or higher will suppress the warning messages totally.\n");
                        /* Mention the versions we are referring to */
                        fprintf(stderr, "Headers are %u.%u.%u, library is %u.%u.%u\n", majnum, minnum, relnum,
                                (unsigned)H5_VERS_MAJOR, (unsigned)H5_VERS_MINOR, (unsigned)H5_VERS_RELEASE);

                        /* Bail out now. */
                        fputs("Bye...\n", stderr);
                        abort();
                    case 1:
                        /* continue with a warning */
                        /* Note that the warning message is embedded in the format string.*/
                        fprintf(stderr,
                                "%s'HDF5_DISABLE_VERSION_CHECK' "
                                "environment variable is set to %d, application will\n"
                                "continue at your own risk.\n",
                                minor_version_mismatch_warning, disable_version_check);
                        /* Mention the versions we are referring to */
                        fprintf(stderr, "Headers are %u.%u.%u, library is %u.%u.%u\n", majnum, minnum, relnum,
                                (unsigned)H5_VERS_MAJOR, (unsigned)H5_VERS_MINOR, (unsigned)H5_VERS_RELEASE);
                        break;
                    default:
                        /* 2 or higher: continue silently */
                        break;
                } /* end switch */

            } /* end if */

        } /* end for */

        /* Check for forward compatibility usage. */
        if (minnum > H5_VERS_MINOR) {
            switch (disable_version_check) {
                case 0:
                    fprintf(stderr, "%s%s", minor_version_forward_compatible_warning,
                            "You can, at your own risk, disable this warning by setting the environment\n"
                            "variable 'HDF5_DISABLE_VERSION_CHECK' to a value of '1'.\n"
                            "Setting it to 2 or higher will suppress the warning messages totally.\n");
                    /* Mention the versions we are referring to */
                    fprintf(stderr, "Headers are %u.%u.%u, library is %u.%u.%u\n", majnum, minnum, relnum,
                            (unsigned)H5_VERS_MAJOR, (unsigned)H5_VERS_MINOR, (unsigned)H5_VERS_RELEASE);

                    /* Bail out now. */
                    fputs("Bye...\n", stderr);
                    abort();
                case 1:
                    /* continue with a warning */
                    /* Note that the warning message is embedded in the format string.*/
                    fprintf(stderr,
                            "%s'HDF5_DISABLE_VERSION_CHECK' "
                            "environment variable is set to %d, application will\n"
                            "continue at your own risk.\n",
                            minor_version_forward_compatible_warning, disable_version_check);
                    /* Mention the versions we are referring to */
                    fprintf(stderr, "Headers are %u.%u.%u, library is %u.%u.%u\n", majnum, minnum, relnum,
                            (unsigned)H5_VERS_MAJOR, (unsigned)H5_VERS_MINOR, (unsigned)H5_VERS_RELEASE);
                    break;
                default:
                    /* 2 or higher: continue silently */
                    break;
            } /* end switch */
        }

    } /* end if (H5_VERS_MINOR != minnum) */

    /* Indicate that the version check has been performed */
    checked = true;

    if (!disable_version_check) {
        /*
         * Verify if H5_VERS_INFO is consistent with the other version information.
         * Check only the first sizeof(lib_str) char.  Assume the information
         * will fit within this size or enough significance.
         */
        snprintf(lib_str, sizeof(lib_str), "HDF5 library version: %d.%d.%d%s%s", H5_VERS_MAJOR, H5_VERS_MINOR,
                 H5_VERS_RELEASE, (*substr ? "-" : ""), substr);

        if (strcmp(lib_str, H5_lib_vers_info_g) != 0) {
            fputs("Warning!  Library version information error.\n"
                  "The HDF5 library version information are not "
                  "consistent in its source code.\nThis is NOT a fatal error "
                  "but should be corrected.  Setting the environment\n"
                  "variable 'HDF5_DISABLE_VERSION_CHECK' to a value of 1 "
                  "will suppress\nthis warning.\n",
                  stderr);
            fprintf(stderr,
                    "Library version information are:\n"
                    "H5_VERS_MAJOR=%d, H5_VERS_MINOR=%d, H5_VERS_RELEASE=%d, "
                    "H5_VERS_SUBRELEASE=%s,\nH5_VERS_INFO=%s\n",
                    H5_VERS_MAJOR, H5_VERS_MINOR, H5_VERS_RELEASE, H5_VERS_SUBRELEASE, H5_VERS_INFO);
        } /* end if */
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5__check_version() */

herr_t
H5check_version(unsigned majnum, unsigned minnum, unsigned relnum)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT_NOERR

    /* Call internal routine */
    /* (Will abort() on failure) */
    H5_check_version(majnum, minnum, relnum);

    FUNC_LEAVE_API_NOERR(ret_value)
} /* end H5check_version() */

/*-------------------------------------------------------------------------
 * Function:    H5open
 *
 * Purpose:     Initialize the library.  This is normally called
 *              automatically, but if you find that an HDF5 library function
 *              is failing inexplicably, then try calling this function
 *              first.
 *
 * Return:    Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5open(void)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOPUSH(FAIL)

    /* all work is done by FUNC_ENTER() */

done:
    FUNC_LEAVE_API_NOPUSH(ret_value)
} /* end H5open() */

/*-------------------------------------------------------------------------
 * Function:    H5atclose
 *
 * Purpose:    Register a callback for the library to invoke when it's
 *        closing.  Callbacks are invoked in LIFO order.
 *
 * Return:    Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5atclose(H5_atclose_func_t func, void *ctx)
{
    H5_atclose_node_t *new_atclose;         /* New 'atclose' node */
    herr_t             ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)

    /* Check arguments */
    if (NULL == func)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "NULL func pointer");

    /* Allocate space for the 'atclose' node */
    if (NULL == (new_atclose = H5FL_MALLOC(H5_atclose_node_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL, "can't allocate 'atclose' node");

    /* Set up 'atclose' node */
    new_atclose->func = func;
    new_atclose->ctx  = ctx;

    /* Connector to linked-list of 'atclose' nodes */
    new_atclose->next = H5_atclose_head;
    H5_atclose_head   = new_atclose;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5atclose() */

/*-------------------------------------------------------------------------
 * Function:    H5close
 *
 * Purpose:    Terminate the library and release all resources.
 *
 * Return:    Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5close(void)
{
    /*
     * Don't call normal FUNC_ENTER() since we don't want to initialize the
     * whole library just to release it all right away.  It is safe to call
     * this function for an uninitialized library.
     */
    FUNC_ENTER_API_NAMECHECK_ONLY

    H5_term_library();

    FUNC_LEAVE_API_NAMECHECK_ONLY(SUCCEED)
} /* end H5close() */

/*-------------------------------------------------------------------------
 * Function:    H5allocate_memory
 *
 * Purpose:        Allocate a memory buffer with the semantics of malloc().
 *
 *              NOTE: This function is intended for use with filter
 *              plugins so that all allocation and free operations
 *              use the same memory allocator. It is not intended for
 *              use as a general memory allocator in applications.
 *
 * Parameters:
 *
 *      size:   The size of the buffer.
 *
 *      clear:  Whether or not to memset the buffer to 0.
 *
 * Return:
 *
 *      Success:    A pointer to the allocated buffer or NULL if the size
 *                  parameter is zero.
 *
 *      Failure:    NULL (but may also be NULL w/ size 0!)
 *
 *-------------------------------------------------------------------------
 */
void *H5_ATTR_MALLOC
H5allocate_memory(size_t size, bool clear)
{
    void *ret_value = NULL;

    FUNC_ENTER_API_NOINIT

    if (0 == size)
        HGOTO_DONE(NULL);

    if (clear)
        ret_value = H5MM_calloc(size);
    else
        ret_value = H5MM_malloc(size);

done:
    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5allocate_memory() */

/*-------------------------------------------------------------------------
 * Function:    H5resize_memory
 *
 * Purpose:        Resize a memory buffer with the semantics of realloc().
 *
 *              NOTE: This function is intended for use with filter
 *              plugins so that all allocation and free operations
 *              use the same memory allocator. It is not intended for
 *              use as a general memory allocator in applications.
 *
 * Parameters:
 *
 *      mem:    The buffer to be resized.
 *
 *      size:   The size of the buffer.
 *
 * Return:
 *
 *      Success:    A pointer to the resized buffer.
 *
 *      Failure:    NULL (the input buffer will be unchanged)
 *
 *-------------------------------------------------------------------------
 */
void *
H5resize_memory(void *mem, size_t size)
{
    void *ret_value = NULL;

    FUNC_ENTER_API_NOINIT

    ret_value = H5MM_realloc(mem, size);

    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5resize_memory() */

/*-------------------------------------------------------------------------
 * Function:    H5free_memory
 *
 * Purpose:        Frees memory allocated by the library that it is the user's
 *              responsibility to free.  Ensures that the same library
 *              that was used to allocate the memory frees it.  Passing
 *              NULL pointers is allowed.
 *
 * Return:        SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5free_memory(void *mem)
{
    FUNC_ENTER_API_NOINIT

    /* At this time, it is impossible for this to fail. */
    H5MM_xfree(mem);

    FUNC_LEAVE_API_NOINIT(SUCCEED)
} /* end H5free_memory() */

/*-------------------------------------------------------------------------
 * Function:    H5is_library_threadsafe
 *
 * Purpose:        Checks to see if the library was built with thread-safety
 *              enabled.
 *
 * Return:        SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5is_library_threadsafe(bool *is_ts /*out*/)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT

    if (is_ts) {
#ifdef H5_HAVE_THREADSAFE_API
        *is_ts = true;
#else  /* H5_HAVE_THREADSAFE_API */
        *is_ts = false;
#endif /* H5_HAVE_THREADSAFE_API */
    }
    else
        ret_value = FAIL;

    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5is_library_threadsafe() */

/*-------------------------------------------------------------------------
 * Function:    H5is_library_terminating
 *
 * Purpose:    Checks to see if the library is shutting down.
 *
 * Note:    Useful for plugins to detect when the library is terminating.
 *        For example, a VOL connector could check if a "file close"
 *        callback was the result of the library shutdown process, or
 *        an API action from the application.
 *
 * Return:    SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5is_library_terminating(bool *is_terminating /*out*/)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API_NOINIT

    assert(is_terminating);

    if (is_terminating)
        *is_terminating = H5_TERM_GLOBAL;
    else
        ret_value = FAIL;

    FUNC_LEAVE_API_NOINIT(ret_value)
} /* end H5is_library_terminating() */

/*-------------------------------------------------------------------------
 * Function:    H5_user_cb_prepare
 *
 * Purpose:     Prepares library before a user callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5_user_cb_prepare(H5_user_cb_state_t *state)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Prepare H5E package for user callback */
    if (H5E_user_cb_prepare(&state->h5e_state) < 0)
        HGOTO_ERROR(H5E_LIB, H5E_CANTSET, FAIL, "unable to prepare H5E package for user callback");

#ifdef H5_HAVE_CONCURRENCY
    /* Prepare H5TS package for user callback */
    if (H5TS_user_cb_prepare() < 0)
        HGOTO_ERROR(H5E_LIB, H5E_CANTSET, FAIL, "unable to prepare H5TS package for user callback");
#endif /* H5_HAVE_THREADSAFE_API */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5_user_cb_prepare() */

/*-------------------------------------------------------------------------
 * Function:    H5_user_cb_restore
 *
 * Purpose:     Restores library after a user callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5_user_cb_restore(const H5_user_cb_state_t *state)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Restore H5E package after user callback */
    if (H5E_user_cb_restore(&state->h5e_state) < 0)
        HGOTO_ERROR(H5E_LIB, H5E_CANTRESTORE, FAIL, "unable to restore H5E package after user callback");

#ifdef H5_HAVE_CONCURRENCY
    /* Restore H5TS package after user callback */
    if (H5TS_user_cb_restore() < 0)
        HGOTO_ERROR(H5E_LIB, H5E_CANTRESTORE, FAIL, "unable to restore H5TS package after user callback");
#endif /* H5_HAVE_THREADSAFE_API */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5_user_cb_restore() */
