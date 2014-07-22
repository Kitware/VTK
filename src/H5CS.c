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

/*
 * Purpose:	Provides internal function tracing in the form of a stack.
 *		The FUNC_ENTER() macro adds the function name to the function
 *              stack whenever a function is entered.
 *		As the functions return with FUNC_LEAVE,
 *		entries are removed from the stack.
 *
 *		A function stack has a fixed maximum size.  If this size is
 *		exceeded then the stack will be truncated and only the
 *		first called functions will have entries on the stack. This is
 *		expected to be a rare condition.
 *
 */


#include "H5private.h"		/* Generic Functions			*/
#include "H5CSprivate.h"	/* Function stack			*/
#include "H5MMprivate.h"	/* Memory management			*/

#ifdef H5_HAVE_CODESTACK

#ifdef H5_HAVE_THREADSAFE
/*
 * The per-thread function stack. pthread_once() initializes a special
 * key that will be used by all threads to create a stack specific to
 * each thread individually. The association of stacks to threads will
 * be handled by the pthread library.
 *
 * In order for this macro to work, H5CS_get_my_stack() must be preceeded
 * by "H5CS_t *fstack =".
 */
static H5CS_t *H5CS_get_stack(void);
#define H5CS_get_my_stack()  H5CS_get_stack()
#else /* H5_HAVE_THREADSAFE */
/*
 * The function stack.  Eventually we'll have some sort of global table so each
 * thread has it's own stack.  The stacks will be created on demand when the
 * thread first calls H5CS_push().  */
H5CS_t		H5CS_stack_g[1];
#define H5CS_get_my_stack()	(H5CS_stack_g+0)
#endif /* H5_HAVE_THREADSAFE */


#ifdef H5_HAVE_THREADSAFE
/*-------------------------------------------------------------------------
 * Function:	H5CS_get_stack
 *
 * Purpose:	Support function for H5CS_get_my_stack() to initialize and
 *              acquire per-thread function stack.
 *
 * Return:	Success:	function stack (H5CS_t *)
 *
 *		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              February 6, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static H5CS_t *
H5CS_get_stack(void)
{
    H5CS_t *fstack;

    FUNC_ENTER_NOAPI_NOERR_NOFS

    fstack = H5TS_get_thread_local_value(H5TS_funcstk_key_g);
    if (!fstack) {
        /* No associated value with current thread - create one */
#ifdef H5_HAVE_WIN_THREADS
        fstack = (H5CS_t *)LocalAlloc(LPTR, sizeof(H5CS_t)); /* Win32 has to use LocalAlloc to match the LocalFree in DllMain */
#else
        fstack = (H5CS_t *)HDmalloc(sizeof(H5CS_t)); /* Don't use H5MM_malloc() here, it causes infinite recursion */
#endif /* H5_HAVE_WIN_THREADS */
        HDassert(fstack);

        /* Set the thread-specific info */
        fstack->nused=0;

        /* (It's not necessary to release this in this API, it is
         *      released by the "key destructor" set up in the H5TS
         *      routines.  See calls to pthread_key_create() in H5TS.c -QAK)
         */
        H5TS_set_thread_local_value(H5TS_funcstk_key_g, (void *)fstack);
    }

    FUNC_LEAVE_NOAPI_NOFS(fstack);
} /* end H5CS_get_stack() */
#endif  /* H5_HAVE_THREADSAFE */


/*-------------------------------------------------------------------------
 * Function:	H5CS_print_stack
 *
 * Purpose:	Prints a function stack.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, February 6, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5CS_print_stack(const H5CS_t *fstack, FILE *stream)
{
    const int	indent = 2;             /* Indention level */
    int         i;                      /* Local index ariable */

    /* Don't push this function on the function stack... :-) */
    FUNC_ENTER_NOAPI_NOERR_NOFS

    /* Sanity check */
    HDassert(fstack);

    /* Default to outputting information to stderr */
    if (!stream)
        stream = stderr;

    HDfprintf (stream, "HDF5-DIAG: Function stack from %s ", H5_lib_vers_info_g);
    /* try show the process or thread id in multiple processes cases*/
#ifdef H5_HAVE_THREADSAFE
    HDfprintf (stream, "thread %lu.", HDpthread_self_ulong());
#else  /* H5_HAVE_THREADSAFE */
    HDfprintf (stream, "thread 0.");
#endif  /* H5_HAVE_THREADSAFE */
    if (fstack && fstack->nused>0)
        HDfprintf (stream, "  Back trace follows.");
    HDfputc ('\n', stream);

    for (i=fstack->nused-1; i>=0; --i)
        HDfprintf(stream, "%*s#%03d: Routine: %s\n", indent, "", i, fstack->slot[i]);

    FUNC_LEAVE_NOAPI_NOFS(SUCCEED);
} /* end H5CS_print_stack() */


/*-------------------------------------------------------------------------
 * Function:	H5CS_print
 *
 * Purpose:	Prints the default function stack in some default way.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, February 6, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5CS_print(FILE *stream)
{
    H5CS_t	*fstack = H5CS_get_my_stack (); /* Get the correct function stack */

    /* Don't push this function on the function stack... :-) */
    FUNC_ENTER_NOAPI_NOERR_NOFS

    /* Sanity check */
    HDassert(fstack);

    H5CS_print_stack(fstack, stream);

    FUNC_LEAVE_NOAPI_NOFS(SUCCEED);
} /* end H5CS_print() */


/*-------------------------------------------------------------------------
 * Function:	H5CS_push
 *
 * Purpose:	Pushes a new record onto function stack for the current
 *		thread.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Thursday, February 6, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5CS_push(const char *func_name)
{
    H5CS_t	*fstack = H5CS_get_my_stack ();

    /* Don't push this function on the function stack... :-) */
    FUNC_ENTER_NOAPI_NOERR_NOFS

    /* Sanity check */
    HDassert(fstack);
    HDassert(func_name);

    /*
     * Push the function if there's room.  Otherwise just increment count
     */
    if (fstack->nused<H5CS_NSLOTS)
	fstack->slot[fstack->nused] = func_name;
    fstack->nused++;

    FUNC_LEAVE_NOAPI_NOFS(SUCCEED);
} /* end H5CS_push() */


/*-------------------------------------------------------------------------
 * Function:	H5CS_pop
 *
 * Purpose:	Pops a record off function stack for the current thread.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Thursday, February 6, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5CS_pop(void)
{
    H5CS_t	*fstack = H5CS_get_my_stack ();

    /* Don't push this function on the function stack... :-) */
    FUNC_ENTER_NOAPI_NOERR_NOFS

    /* Sanity check */
    HDassert(fstack);
    HDassert(fstack->nused>0);

    /* Pop the function. */
    fstack->nused--;

    FUNC_LEAVE_NOAPI_NOFS(SUCCEED);
} /* end H5CS_pop() */


/*-------------------------------------------------------------------------
 * Function:	H5CS_copy_stack
 *
 * Purpose:	Makes a copy of the current stack
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Tuesday, August 9, 2005
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5CS_copy_stack(H5CS_t *new_stack)
{
    H5CS_t	*old_stack = H5CS_get_my_stack ();
    unsigned    u;                      /* Local index variable */

    /* Don't push this function on the function stack... :-) */
    FUNC_ENTER_NOAPI_NOERR_NOFS

    /* Sanity check */
    HDassert(old_stack);

    /* Copy old stack to new one, duplicating the strings */
    for(u = 0; u < old_stack->nused; u++)
        new_stack->slot[u] = H5MM_strdup(old_stack->slot[u]);
    new_stack->nused = old_stack->nused;

    FUNC_LEAVE_NOAPI_NOFS(SUCCEED);
} /* end H5CS_copy_stack() */


/*-------------------------------------------------------------------------
 * Function:	H5CS_close_stack
 *
 * Purpose:	Closes a copy of a stack
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		Tuesday, August 9, 2005
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5CS_close_stack(H5CS_t *stack)
{
    unsigned    u;                      /* Local index variable */

    /* Don't push this function on the function stack... :-) */
    FUNC_ENTER_NOAPI_NOERR_NOFS

    /* Sanity check */
    HDassert(stack);

    /* Free strings on stack */
    for(u = 0; u < stack->nused; u++)
        stack->slot[u] = H5MM_xfree((void *)stack->slot[u]);

    FUNC_LEAVE_NOAPI_NOFS(SUCCEED);
} /* end H5CS_close_stack() */

#endif /* H5_HAVE_CODESTACK */

