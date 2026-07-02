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

/*
 * Purpose: Windows threadsafety routines
 *
 * Note:  Because this threadsafety framework operates outside the library,
 *        it does not use the error stack (although it does use error macros
 *        that don't push errors on a stack) and only uses the "namecheck only"
 *        FUNC_ENTER_* / FUNC_LEAVE_* macros.
 */

/****************/
/* Module Setup */
/****************/

#include "H5TSmodule.h" /* This source code file is part of the H5TS module */

/***********/
/* Headers */
/***********/
#include "H5private.h"  /* Generic Functions                   */
#include "H5Eprivate.h" /* Error handling                      */
#include "H5TSpkg.h"    /* Threadsafety                        */

#ifdef H5_HAVE_WIN_THREADS

/****************/
/* Local Macros */
/****************/

/******************/
/* Local Typedefs */
/******************/

/********************/
/* Local Prototypes */
/********************/
#ifdef H5_HAVE_THREADSAFE
#if defined(H5_BUILT_AS_DYNAMIC_LIB) && defined(H5_HAVE_WIN32_API)
static herr_t H5TS__win32_thread_enter(void);
static herr_t H5TS__win32_thread_exit(void);
#endif
#endif

/*********************/
/* Package Variables */
/*********************/

/*****************************/
/* Library Private Variables */
/*****************************/

/*******************/
/* Local Variables */
/*******************/

#ifdef H5_HAVE_THREADSAFE_API
/*--------------------------------------------------------------------------
 * Function:    H5TS__win32_process_enter
 *
 * Purpose:     Per-process setup on Windows when using Win32 threads.
 *
 * Returns:     TRUE on success, FALSE on failure
 *
 *--------------------------------------------------------------------------
 */
H5_DLL BOOL CALLBACK
H5TS__win32_process_enter(PINIT_ONCE InitOnce, PVOID Parameter, PVOID *lpContex)
{
    BOOL ret_value = TRUE;

    FUNC_ENTER_PACKAGE_NAMECHECK_ONLY

    /* Initialize H5TS package */
    if (H5_UNLIKELY(H5TS__init_package() < 0))
        HGOTO_DONE(FALSE);

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* H5TS__win32_process_enter() */

#if defined(H5_BUILT_AS_DYNAMIC_LIB) && defined(H5_HAVE_WIN32_API)
/*--------------------------------------------------------------------------
 * Function:    H5TS__win32_thread_enter
 *
 * Purpose:     Per-thread setup on Windows when using Win32 threads.
 *
 * Return:      Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
static herr_t
H5TS__win32_thread_enter(void)
{
    FUNC_ENTER_PACKAGE_NAMECHECK_ONLY

    /* Currently a placeholder function.  TLS setup is performed
     * elsewhere in the library.
     *
     * WARNING: Do NOT use C standard library functions here.
     * CRT functions are not allowed in DllMain, which is where this code
     * is used.
     */

    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(SUCCEED)
} /* H5TS__win32_thread_enter() */

/*--------------------------------------------------------------------------
 * Function:    H5TS__win32_thread_exit
 *
 * Purpose:     Per-thread cleanup on Windows when using Win32 threads.
 *
 * Return:      Non-negative on success / Negative on failure
 *
 *--------------------------------------------------------------------------
 */
static herr_t
H5TS__win32_thread_exit(void)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE_NAMECHECK_ONLY

    /* Windows uses a different thread local storage mechanism which does
     * not support auto-freeing like pthreads' keys.
     *
     * WARNING: Do NOT use C standard library functions here.
     * CRT functions are not allowed in DllMain, which is where this code
     * is used.
     */

    /* Clean up per-thread thread local storage */
    if (H5TS_thrd_info_key_g != TLS_OUT_OF_INDEXES) {
        LPVOID lpvData;

        if (H5_UNLIKELY(H5TS_key_get_value(H5TS_thrd_info_key_g, &lpvData) < 0))
            HGOTO_DONE(FAIL);
        if (lpvData)
            H5TS__tinfo_destroy(lpvData);
    }

done:
    FUNC_LEAVE_NOAPI_NAMECHECK_ONLY(ret_value)
} /* H5TS__win32_thread_exit() */

/*-------------------------------------------------------------------------
 * Function:    DllMain
 *
 * Purpose:     Handles various conditions in the library on Windows.
 *
 *    NOTE:     The main purpose of this is for handling Win32 thread cleanup
 *              on thread/process detach.
 *
 *              Only enabled when the shared Windows library is built with
 *              thread safety enabled.
 *
 * Return:      true on success, false on failure
 *
 *-------------------------------------------------------------------------
 */
BOOL WINAPI
DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD fdwReason, _In_ LPVOID lpvReserved)
{
    /* Don't add our function enter/leave macros since this function will be
     * called before the library is initialized.
     *
     * NOTE: Do NOT call any CRT functions in DllMain!
     * This includes any functions that are called by from here!
     */

    BOOL fOkay = true;

    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_THREAD_ATTACH:
            if (H5TS__win32_thread_enter() < 0)
                fOkay = false;
            break;

        case DLL_THREAD_DETACH:
            if (H5TS__win32_thread_exit() < 0)
                fOkay = false;
            break;

        default:
            /* Shouldn't get here */
            fOkay = false;
            break;
    }

    return fOkay;
}
#endif /* H5_HAVE_WIN32_API && H5_BUILT_AS_DYNAMIC_LIB */
#endif /* H5_HAVE_THREADSAFE_API */

#endif /* H5_HAVE_WIN_THREADS */
