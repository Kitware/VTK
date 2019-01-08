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
 * Purpose: Code to implement a path table which stores plugin search paths.
 *
 *          The path table is implemented as a dynamic, global array which
 *          will grow as new paths are inserted. The capacity of the path
 *          table never shrinks (though given the low number of paths
 *          expected and the low likelihood of paths being removed, this
 *          seems unlikely to be a problem). Inserts and removals rework
 *          the array so that there are no 'holes' in the in-use part
 *          of the array.
 *
 *          Note that it's basically up to the user to manage the indexes
 *          when a complicated series of insert, overwrite, and, remove
 *          operations take place.
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


/****************/
/* Local Macros */
/****************/

/* Initial capacity of the path table */
#define H5PL_INITIAL_PATH_CAPACITY      16

/* The amount to add to the capacity when the table is full */
#define H5PL_PATH_CAPACITY_ADD          16


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Local Prototypes */
/********************/

static herr_t H5PL__insert_at(const char *path, unsigned int index);
static herr_t H5PL__make_space_at(unsigned int index);
static herr_t H5PL__replace_at(const char *path, unsigned int index);
static herr_t H5PL__expand_path_table(void);
static herr_t H5PL__find_plugin_in_path(const H5PL_search_params_t *search_params, hbool_t *found, const char *dir, const void **plugin_info);

/*********************/
/* Package Variables */
/*********************/


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/

/* Stored plugin paths to search */
static char       **H5PL_paths_g = NULL;

/* The number of stored paths */
static unsigned     H5PL_num_paths_g = 0;

/* The capacity of the path table */
static unsigned     H5PL_path_capacity_g = H5PL_INITIAL_PATH_CAPACITY;



/*-------------------------------------------------------------------------
 * Function:    H5PL__insert_at()
 *
 * Purpose:     Insert a path at a particular index in the path table.
 *              Does not clobber! Will move existing paths up to make
 *              room. Use H5PL__replace_at(index) if you want to clobber.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5PL__insert_at(const char *path, unsigned int index)
{
    char    *path_copy = NULL;      /* copy of path string (for storing) */
    herr_t  ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_STATIC

    /* Check args - Just assert on package functions */
    HDassert(path);
    HDassert(HDstrlen(path));

    /* Expand the table if it is full */
    if (H5PL_num_paths_g == H5PL_path_capacity_g)
        if (H5PL__expand_path_table() < 0)
            HGOTO_ERROR(H5E_PLUGIN, H5E_CANTALLOC, FAIL, "can't expand path table")

    /* Copy the path for storage so the caller can dispose of theirs */
    if (NULL == (path_copy = H5MM_strdup(path)))
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTALLOC, FAIL, "can't make internal copy of path")

#ifdef H5_HAVE_WIN32_API
    /* Clean up Microsoft Windows environment variables in the path string */
    if(H5_expand_windows_env_vars(&path_copy))
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTCONVERT, FAIL, "can't expand environment variable string")
#endif /* H5_HAVE_WIN32_API */

    /* If the table entry is in use, make some space */
    if (H5PL_paths_g[index])
        if (H5PL__make_space_at(index) < 0)
            HGOTO_ERROR(H5E_PLUGIN, H5E_NOSPACE, FAIL, "unable to make space in the table for the new entry")

    /* Insert the copy of the search path into the table at the specified index */
    H5PL_paths_g[index] = path_copy;
    H5PL_num_paths_g++;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__insert_at() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__make_space_at()
 *
 * Purpose:     Free up a slot in the path table, moving existing path
 *              entries as necessary.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5PL__make_space_at(unsigned int index)
{
    unsigned    u;                      /* iterator */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_STATIC_NOERR

    /* Check args - Just assert on package functions */
    HDassert(index < H5PL_path_capacity_g);

    /* Copy the paths back to make a space  */
    for (u = H5PL_num_paths_g; u > index; u--)
        H5PL_paths_g[u] = H5PL_paths_g[u-1];

    H5PL_paths_g[index] = NULL;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__make_space_at() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__replace_at()
 *
 * Purpose:     Replace a path at a particular index in the path table.
 *              The path in the table must exist and will be freed by this
 *              function.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5PL__replace_at(const char *path, unsigned int index)
{
    char    *path_copy = NULL;      /* copy of path string (for storing) */
    herr_t  ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_STATIC

    /* Check args - Just assert on package functions */
    HDassert(path);
    HDassert(HDstrlen(path));

    /* Check that the table entry is in use */
    if (!H5PL_paths_g[index])
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTFREE, FAIL, "path entry at index %u in the table is NULL", index)

    /* Copy the path for storage so the caller can dispose of theirs */
    if (NULL == (path_copy = H5MM_strdup(path)))
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTALLOC, FAIL, "can't make internal copy of path")

#ifdef H5_HAVE_WIN32_API
        /* Clean up Microsoft Windows environment variables in the path string */
        if (H5_expand_windows_env_vars(&path_copy))
            HGOTO_ERROR(H5E_PLUGIN, H5E_CANTCONVERT, FAIL, "can't expand environment variable string")
#endif /* H5_HAVE_WIN32_API */

    /* Free the existing path entry */
    H5PL_paths_g[index] = (char *)H5MM_xfree(H5PL_paths_g[index]);

    /* Copy the search path into the table at the specified index */
    H5PL_paths_g[index] = path_copy;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__replace_at() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__create_path_table
 *
 * Purpose:     Create the collection of paths that will be searched
 *              when loading plugins.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PL__create_path_table(void)
{
    char        *env_var= NULL;         /* Path string from environment variable */
    char        *paths = NULL;          /* Delimited paths string. Either from the
                                         * environment variable or the default.
                                         */
    char        *next_path = NULL;      /* A path tokenized from the paths string */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE

    /* Allocate memory for the path table */
    H5PL_num_paths_g = 0;
    H5PL_path_capacity_g = H5PL_INITIAL_PATH_CAPACITY;
    if (NULL == (H5PL_paths_g = (char **)H5MM_calloc((size_t)H5PL_path_capacity_g * sizeof(char *))))
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTALLOC, FAIL, "can't allocate memory for path table")

    /* Retrieve paths from HDF5_PLUGIN_PATH if the user sets it
     * or from the default paths if it isn't set.
     */
    env_var = HDgetenv("HDF5_PLUGIN_PATH");
    if (NULL == env_var)
        paths = H5MM_strdup(H5PL_DEFAULT_PATH);
    else
        paths = H5MM_strdup(env_var);

    if (NULL == paths)
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTALLOC, FAIL, "can't allocate memory for path copy")

    /* Separate the paths and store them */
    /* XXX: strtok() is not thread-safe */
    next_path = HDstrtok(paths, H5PL_PATH_SEPARATOR);
    while (next_path) {

        /* Insert the path into the table */
        if (H5PL__append_path(next_path) < 0)
            HGOTO_ERROR(H5E_PLUGIN, H5E_CANTALLOC, FAIL, "can't insert path: %s", next_path)

        /* Get the next path from the environment string */
        next_path = HDstrtok(NULL, H5PL_PATH_SEPARATOR);
    } /* end while */

done:
    if (paths)
        paths = (char *)H5MM_xfree(paths);

    /* Try to clean up on errors */
    if (FAIL == ret_value) {
        if (H5PL_paths_g)
            H5PL_paths_g = (char **)H5MM_xfree(H5PL_paths_g);
        H5PL_path_capacity_g = 0;
    }

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__create_path_table() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__close_path_table
 *
 * Purpose:     Close the collection of paths that will be searched
 *              when loading plugins.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PL__close_path_table(void)
{
    unsigned    u;                      /* iterator */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE_NOERR

    /* Free paths */
    for (u = 0; u < H5PL_num_paths_g; u++)
        if (H5PL_paths_g[u])
            H5PL_paths_g[u] = (char *)H5MM_xfree(H5PL_paths_g[u]);

    /* Free path table */
    H5PL_paths_g = (char **)H5MM_xfree(H5PL_paths_g);

    /* Reset values */
    H5PL_num_paths_g = 0;

    FUNC_LEAVE_NOAPI(ret_value)

} /* end H5PL__close_path_table() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__get_num_paths
 *
 * Purpose:     Gets the number of plugin paths that have been stored.
 *
 * Return:      Success:    The number of paths
 *              Failture:   Can't fail
 *-------------------------------------------------------------------------
 */
unsigned
H5PL__get_num_paths(void)
{
    FUNC_ENTER_PACKAGE_NOERR

    FUNC_LEAVE_NOAPI(H5PL_num_paths_g)

} /* end H5PL__get_num_paths() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__expand_path_table
 *
 * Purpose:     Expand the path table when it's full.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5PL__expand_path_table(void)
{
    herr_t      ret_value = SUCCEED;

    FUNC_ENTER_STATIC

    /* Update the capacity */
    H5PL_path_capacity_g += H5PL_PATH_CAPACITY_ADD;

    /* Resize the array */
    if(NULL == (H5PL_paths_g = (char **)H5MM_realloc(H5PL_paths_g, (size_t)H5PL_path_capacity_g * sizeof(char *))))
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTALLOC, FAIL, "allocating additional memory for path table failed")

    /* Initialize the new memory */
    HDmemset(H5PL_paths_g + H5PL_num_paths_g, 0, (size_t)H5PL_PATH_CAPACITY_ADD * sizeof(char *));

done:
    /* Set the path capacity back if there were problems */
    if (FAIL == ret_value)
        H5PL_path_capacity_g -= H5PL_PATH_CAPACITY_ADD;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__expand_path_table() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__append_path
 *
 * Purpose:     Insert a path at the end of the table.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PL__append_path(const char *path)
{
    herr_t  ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args - Just assert on package functions */
    HDassert(path);
    HDassert(HDstrlen(path));

    /* Insert the path at the end of the table */
    if (H5PL__insert_at(path, H5PL_num_paths_g) < 0)
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTINSERT, FAIL, "unable to append search path")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__append_path() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__prepend_path
 *
 * Purpose:     Insert a path at the beginning of the table.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PL__prepend_path(const char *path)
{
    herr_t  ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args - Just assert on package functions */
    HDassert(path);
    HDassert(HDstrlen(path));

    /* Insert the path at the beginning of the table */
    if (H5PL__insert_at(path, 0) < 0)
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTINSERT, FAIL, "unable to prepend search path")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__prepend_path() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__replace_path
 *
 * Purpose:     Replace a path at particular index in the table.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PL__replace_path(const char *path, unsigned int index)
{
    herr_t  ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args - Just assert on package functions */
    HDassert(path);
    HDassert(HDstrlen(path));
    HDassert(index < H5PL_path_capacity_g);

    /* Insert the path at the requested index */
    if (H5PL__replace_at(path, index) < 0)
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTINSERT, FAIL, "unable to replace search path")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__replace_path() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__insert_path
 *
 * Purpose:     Insert a path at particular index in the table, moving
 *              any existing paths back to make space.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PL__insert_path(const char *path, unsigned int index)
{
    herr_t  ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args - Just assert on package functions */
    HDassert(path);
    HDassert(HDstrlen(path));
    HDassert(index < H5PL_path_capacity_g);

    /* Insert the path at the requested index */
    if (H5PL__insert_at(path, index) < 0)
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTINSERT, FAIL, "unable to insert search path")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__insert_path() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__remove_path
 *
 * Purpose:     Remove a path at particular index in the table, freeing
 *              the path string and moving the paths down to close the gap.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PL__remove_path(unsigned int index)
{
    unsigned    u;                      /* iterator */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args - Just assert on package functions */
    HDassert(index < H5PL_path_capacity_g);

    /* Check if the path at that index is set */
    if (!H5PL_paths_g[index])
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTDELETE, FAIL, "search path at index %u is NULL", index)

    /* Delete the path */
    H5PL_num_paths_g--;
    H5PL_paths_g[index] = (char *)H5MM_xfree(H5PL_paths_g[index]);

    /* Shift the paths down to close the gap */
    for (u = index; u < H5PL_num_paths_g; u++)
        H5PL_paths_g[u] = H5PL_paths_g[u+1];

    /* Set the (former) last path to NULL */
    H5PL_paths_g[H5PL_num_paths_g] = NULL;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__remove_path() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__get_path
 *
 * Purpose:     Get a pointer to a path at particular index in the table.
 *
 * Return:      Success:    A pointer to a path string stored in the table
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
const char *
H5PL__get_path(unsigned int index)
{
    char    *ret_value = NULL;    /* Return value */

    FUNC_ENTER_PACKAGE

    /* Get the path at the requested index */
    if (index >= H5PL_num_paths_g)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, NULL, "path index %u is out of range in table", index)

    return H5PL_paths_g[index];
done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__replace_path() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__find_plugin_in_path_table
 *
 * Purpose:     Attempts to find a matching plugin in the file system
 *              using the paths stored in the path table.
 *.
 *              The 'found' parameter will be set appropriately.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PL__find_plugin_in_path_table(const H5PL_search_params_t *search_params, hbool_t *found, const void **plugin_info)
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

    /* Loop over the paths in the table, checking for an appropriate plugin */
    for (u = 0; u < H5PL_num_paths_g; u++) {

        /* Search for the plugin in this path */
        if (H5PL__find_plugin_in_path(search_params, found, H5PL_paths_g[u], plugin_info) < 0)
            HGOTO_ERROR(H5E_PLUGIN, H5E_CANTGET, FAIL, "search in path %s encountered an error", H5PL_paths_g[u])

        /* Break out if found */
        if (*found) {
            if (!plugin_info)
                HGOTO_ERROR(H5E_PLUGIN, H5E_BADVALUE, FAIL, "plugin info should not be NULL")
            break;
        }
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__find_plugin_in_path_table() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__find_plugin_in_path
 *
 * Purpose:     Given a path, this function opens the directory and envokes
 *              another function to go through all files to find the right
 *              plugin library. Two function definitions are for Unix and
 *              Windows.
 *
 *              The found parameter will be set to TRUE and the info
 *              parameter will be filled in on success.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
#ifndef H5_HAVE_WIN32_API
static herr_t
H5PL__find_plugin_in_path(const H5PL_search_params_t *search_params, hbool_t *found, const char *dir, const void **plugin_info)
{
    char           *path = NULL;
    DIR            *dirp = NULL;            /* Directory stream */
    struct dirent  *dp = NULL;              /* Directory entry */
    herr_t         ret_value = SUCCEED;

    FUNC_ENTER_STATIC

    /* Check args - Just assert on package functions */
    HDassert(search_params);
    HDassert(found);
    HDassert(dir);
    HDassert(plugin_info);

    /* Initialize the found parameter */
    *found = FALSE;

    /* Open the directory */
    if (!(dirp = HDopendir(dir)))
        HGOTO_ERROR(H5E_PLUGIN, H5E_OPENERROR, FAIL, "can't open directory: %s", dir)

    /* Iterate through all entries in the directory */
    while (NULL != (dp = HDreaddir(dirp))) {

        /* The library we are looking for should be called libxxx.so... on Unix
         * or libxxx.xxx.dylib on Mac.
         */
#ifndef __CYGWIN__
        if (!HDstrncmp(dp->d_name, "lib", (size_t)3) &&
                (HDstrstr(dp->d_name, ".so") || HDstrstr(dp->d_name, ".dylib"))) {
#else
        if (!HDstrncmp(dp->d_name, "cyg", (size_t)3) &&
                HDstrstr(dp->d_name, ".dll") ) {
#endif

            h5_stat_t   my_stat;
            size_t      len;

            /* Allocate & initialize the path name */
            len = HDstrlen(dir) + HDstrlen(H5PL_PATH_SEPARATOR) + HDstrlen(dp->d_name) + 1 /*\0*/;

            if (NULL == (path = (char *)H5MM_calloc(len)))
                HGOTO_ERROR(H5E_PLUGIN, H5E_CANTALLOC, FAIL, "can't allocate memory for path")

            HDsnprintf(path, len, "%s/%s", dir, dp->d_name);

            /* Get info for directory entry */
            if (HDstat(path, &my_stat) == -1)
                HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "can't stat file %s -- error was: %s", path, HDstrerror(errno))

            /* If it is a directory, skip it */
            if (S_ISDIR(my_stat.st_mode))
                continue;

            /* attempt to open the dynamic library as a filter library */
            if (H5PL__open(path, search_params->type, search_params->key, found, plugin_info) < 0)
                HGOTO_ERROR(H5E_PLUGIN, H5E_CANTGET, FAIL, "search in directory failed")
            if (*found)
                HGOTO_DONE(SUCCEED)

            path = (char *)H5MM_xfree(path);
        } /* end if */
    } /* end while */

done:
    if (dirp)
        if (HDclosedir(dirp) < 0)
            HDONE_ERROR(H5E_FILE, H5E_CLOSEERROR, FAIL, "can't close directory: %s", HDstrerror(errno))

    path = (char *)H5MM_xfree(path);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__find_plugin_in_path() */
#else /* H5_HAVE_WIN32_API */
static herr_t
H5PL__find_plugin_in_path(const H5PL_search_params_t *search_params, hbool_t *found, const char *dir, const void **plugin_info)
{
    WIN32_FIND_DATAA    fdFile;
    HANDLE              hFind = INVALID_HANDLE_VALUE;
    char                *path = NULL;
    char                service[2048];
    herr_t              ret_value = SUCCEED;

    FUNC_ENTER_STATIC

    /* Check args - Just assert on package functions */
    HDassert(search_params);
    HDassert(found);
    HDassert(dir);
    HDassert(plugin_info);

    /* Initialize the found parameter */
    *found = FALSE;

    /* Specify a file mask. *.* = We want everything! */
    HDsprintf(service, "%s\\*.dll", dir);
    if ((hFind = FindFirstFileA(service, &fdFile)) == INVALID_HANDLE_VALUE)
        HGOTO_ERROR(H5E_PLUGIN, H5E_OPENERROR, FAIL, "can't open directory")

    /* Loop over all the files */
    do {
        /* Ignore '.' and '..' */
        if (HDstrcmp(fdFile.cFileName, ".") != 0 && HDstrcmp(fdFile.cFileName, "..") != 0) {

            /* XXX: Probably just continue here and move the code below over one tab */

            size_t      len;

            /* Allocate & initialize the path name */
            len = HDstrlen(dir) + HDstrlen(H5PL_PATH_SEPARATOR) + HDstrlen(fdFile.cFileName) + 1;

            if (NULL == (path = (char *)H5MM_calloc(len)))
                HGOTO_ERROR(H5E_PLUGIN, H5E_CANTALLOC, FAIL, "can't allocate memory for path")

            HDsnprintf(path, len, "%s\\%s", dir, fdFile.cFileName);

            /* Ignore directories */
            if (fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;

            /* attempt to open the dynamic library as a filter library */
            if (H5PL__open(path, search_params->type, search_params->key, found, plugin_info) < 0)
                HGOTO_ERROR(H5E_PLUGIN, H5E_CANTGET, FAIL, "search in directory failed")
            if (*found)
                HGOTO_DONE(SUCCEED)

            path = (char *)H5MM_xfree(path);
        }
    } while (FindNextFileA(hFind, &fdFile));

done:
    if (hFind != INVALID_HANDLE_VALUE)
        FindClose(hFind);
    if (path)
        path = (char *)H5MM_xfree(path);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__find_plugin_in_path() */
#endif /* H5_HAVE_WIN32_API */

