/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5. The full HDF5 copyright notice, including      *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Purpose: Code to implement a plugin cache which stores information
 *          about plugins which have already been loaded.
 *
 *          The plugin cache is implemented as a dynamic, global array which
 *          will grow as new plugins are added. The capacity of the cache
 *          never shrinks since plugins stay in memory once loaded.
 *
 *          Note that this functionality has absolutely nothing to do with
 *          the metadata or chunk caches.
 */

/****************/
/* Module Setup */
/****************/

#include "H5PLmodule.h"          /* This source code file is part of the H5PL module */


/***********/
/* Headers */
/***********/
#include "H5private.h"      /* Generic Functions            */
#include "H5Eprivate.h"     /* Error handling               */
#include "H5MMprivate.h"    /* Memory management            */
#include "H5PLpkg.h"        /* Plugin                       */
#include "H5Zprivate.h"     /* Filter pipeline              */


/****************/
/* Local Macros */
/****************/

/* Initial capacity of the plugin cache */
#define H5PL_INITIAL_CACHE_CAPACITY     16

/* The amount to add to the capacity when the cache is full */
#define H5PL_CACHE_CAPACITY_ADD         16


/******************/
/* Local Typedefs */
/******************/

/* Type for the list of info for opened plugin libraries */
typedef struct H5PL_plugin_t {
    H5PL_type_t     type;           /* Plugin type                          */
    H5PL_key_t      key;            /* Unique key to identify the plugin    */
    H5PL_HANDLE     handle;         /* Plugin handle                        */
} H5PL_plugin_t;


/********************/
/* Local Prototypes */
/********************/

static herr_t H5PL__expand_cache(void);


/*********************/
/* Package Variables */
/*********************/


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/

/* Cache for storing opened plugin libraries */
static H5PL_plugin_t   *H5PL_cache_g = NULL;

/* The number of stored plugins */
static unsigned int     H5PL_num_plugins_g = 0;

/* The capacity of the plugin cache */
static unsigned int     H5PL_cache_capacity_g = 0;



/*-------------------------------------------------------------------------
 * Function:    H5PL__create_plugin_cache
 *
 * Purpose:     Create the cache that will store plugins that have already
 *              been loaded.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PL__create_plugin_cache(void)
{
    herr_t      ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    /* Allocate memory for the plugin cache */
    H5PL_num_plugins_g = 0;

    H5PL_cache_capacity_g = H5PL_INITIAL_CACHE_CAPACITY;

    if (NULL == (H5PL_cache_g = (H5PL_plugin_t *)H5MM_calloc((size_t)H5PL_cache_capacity_g * sizeof(H5PL_plugin_t))))
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTALLOC, FAIL, "can't allocate memory for plugin cache")

done:
    /* Try to clean up on errors */
    if (FAIL == ret_value) {
        if (H5PL_cache_g)
            H5PL_cache_g = (H5PL_plugin_t *)H5MM_xfree(H5PL_cache_g);
        H5PL_cache_capacity_g = 0;
    }

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__create_plugin_cache() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__close_plugin_cache
 *
 * Purpose:     Close the cache of plugins that have already been loaded,
 *              closing all the plugins contained inside.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PL__close_plugin_cache(hbool_t *already_closed /*out*/)
{
    unsigned int    u;                      /* iterator */
    herr_t          ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE_NOERR

    /* Close opened dynamic libraries */
    if (H5PL_cache_g) {

        /* Close any cached plugins */
        for (u = 0; u < H5PL_num_plugins_g; u++)
            H5PL__close((H5PL_cache_g[u]).handle);

        /* Free the cache array */
        H5PL_cache_g = (H5PL_plugin_t *)H5MM_xfree(H5PL_cache_g);
        H5PL_num_plugins_g = 0;
        H5PL_cache_capacity_g = 0;

        /* Note that actually closed the table (needed by package close call) */
        *already_closed = FALSE;
    }
    else
        *already_closed = TRUE;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__close_plugin_cache() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__expand_cache
 *
 * Purpose:     Expand the plugin cache when it's full.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5PL__expand_cache(void)
{
    herr_t      ret_value = SUCCEED;

    FUNC_ENTER_STATIC

    /* Update the capacity */
    H5PL_cache_capacity_g += H5PL_CACHE_CAPACITY_ADD;

    /* Resize the array */
    if(NULL == (H5PL_cache_g = (H5PL_plugin_t *)H5MM_realloc(H5PL_cache_g, (size_t)H5PL_cache_capacity_g * sizeof(H5PL_plugin_t))))
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTALLOC, FAIL, "allocating additional memory for plugin cache failed")

    /* Initialize the new memory */
    HDmemset(H5PL_cache_g + H5PL_num_plugins_g, 0, (size_t)H5PL_CACHE_CAPACITY_ADD * sizeof(H5PL_plugin_t));

done:
    /* Set the cache capacity back if there were problems */
    if (FAIL == ret_value)
        H5PL_cache_capacity_g -= H5PL_CACHE_CAPACITY_ADD;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__expand_cache() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__add_plugin
 *
 * Purpose:     Add a plugin to the plugin cache.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PL__add_plugin(H5PL_type_t type, H5PL_key_t key, H5PL_HANDLE handle)
{
    herr_t      ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    /* Expand the cache if it is too small */
    if (H5PL_num_plugins_g >= H5PL_cache_capacity_g)
        if (H5PL__expand_cache() < 0)
            HGOTO_ERROR(H5E_PLUGIN, H5E_CANTALLOC, FAIL, "can't expand plugin cache")

    /* Store the plugin info and bump the # of plugins */
    H5PL_cache_g[H5PL_num_plugins_g].type       = type;
    H5PL_cache_g[H5PL_num_plugins_g].key        = key;
    H5PL_cache_g[H5PL_num_plugins_g].handle     = handle;

    H5PL_num_plugins_g++;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__add_plugin() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__find_plugin_in_cache
 *
 * Purpose:     Attempts to find a matching plugin from the cache.
 *
 *              The 'found' parameter will be set appropriately.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
/* See the other use of H5PL_GET_LIB_FUNC() for an explanation
 * for why we disable -Wpedantic here.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
herr_t
H5PL__find_plugin_in_cache(const H5PL_search_params_t *search_params, hbool_t *found, const void **plugin_info)
{
    unsigned int    u;                          /* iterator */
    herr_t          ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    /* Check args - Just assert on package functions */
    HDassert(search_params);
    HDassert(found);
    HDassert(plugin_info);

    /* Initialize output parameters */
    *found = FALSE;
    *plugin_info = NULL;

    /* Loop over all the plugins, looking for one that matches */
    for (u = 0; u < H5PL_num_plugins_g; u++) {

        /* If the plugin type (filter, etc.) and ID match, query the plugin for its info */
        if ((search_params->type == (H5PL_cache_g[u]).type) && (search_params->key.id == (H5PL_cache_g[u]).key.id)) {

            H5PL_get_plugin_info_t      get_plugin_info_function;
            const H5Z_class2_t          *filter_info;

            /* Get the "get plugin info" function from the plugin. */
            if (NULL == (get_plugin_info_function = (H5PL_get_plugin_info_t)H5PL_GET_LIB_FUNC((H5PL_cache_g[u]).handle, "H5PLget_plugin_info")))
                HGOTO_ERROR(H5E_PLUGIN, H5E_CANTGET, FAIL, "can't get function for H5PLget_plugin_info")

            /* Call the "get plugin info" function */
            if (NULL == (filter_info = (const H5Z_class2_t *)(*get_plugin_info_function)()))
                HGOTO_ERROR(H5E_PLUGIN, H5E_CANTGET, FAIL, "can't get plugin info")

            /* Set output parameters */
            *found = TRUE;
            *plugin_info = filter_info;

            /* No need to continue processing */
            break;

        } /* end if */

    } /* end for */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__find_plugin_in_cache() */
#pragma GCC diagnostic pop

