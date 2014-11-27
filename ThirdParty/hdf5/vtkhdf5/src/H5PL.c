/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5. The full HDF5 copyright notice, including      *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic document set and is     *
 * linked from the top-level documents page.  It can also be found at        *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have access   *
 * to either file, you may request a copy from help@hdfgroup.org.            *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/****************/
/* Module Setup */
/****************/

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC	H5PL__init_interface

/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5PLprivate.h"	/* Plugin       			*/
#include "H5Zprivate.h"		/* Filter pipeline			*/

#ifndef H5_VMS

/****************/
/* Local Macros */
/****************/

#define H5PL_MAX_PATH_NUM       16

/****************************/
/* Macros for supporting 
 * both Windows and Unix */
/****************************/
/* Windows support */
#ifdef H5_HAVE_WIN32_API

#define H5PL_PATH_SEPARATOR     ";"

/* Handle for dynamic library */
#define H5PL_HANDLE HINSTANCE

/* Get a handle to a plugin library.  Windows: TEXT macro handles Unicode strings */
#define H5PL_OPEN_DLIB(S) LoadLibraryEx(TEXT(S), NULL, LOAD_WITH_ALTERED_SEARCH_PATH)

/* Get the address of a symbol in dynamic library */
#define H5PL_GET_LIB_FUNC(H,N) GetProcAddress(H,N)

/* Close dynamic library */
#define H5PL_CLOSE_LIB(H) FreeLibrary(H)

/* Clear error - nothing to do */
#define H5PL_CLR_ERROR

typedef const void *(__cdecl *H5PL_get_plugin_info_t)(void);

/* Unix support */
#else /* H5_HAVE_WIN32_API */

#define H5PL_PATH_SEPARATOR     ":"

/* Handle for dynamic library */
#define H5PL_HANDLE void *

/* Get a handle to a plugin library.  Windows: TEXT macro handles Unicode strings */
#define H5PL_OPEN_DLIB(S) dlopen(S, RTLD_NOW)

/* Get the address of a symbol in dynamic library */
#define H5PL_GET_LIB_FUNC(H,N) dlsym(H,N)

/* Close dynamic library */
#define H5PL_CLOSE_LIB(H) dlclose(H)

/* Clear error */
#define H5PL_CLR_ERROR dlerror()

typedef const void *(*H5PL_get_plugin_info_t)(void);
#endif /* H5_HAVE_WIN32_API */

#define H5PL_DEFAULT_PATH       H5_DEFAULT_PLUGINDIR

/* Special symbol to indicate no plugin loading */
#define H5PL_NO_PLUGIN          "::"

/******************/
/* Local Typedefs */
/******************/

/* Type for the list of info for opened plugin libraries */
typedef struct H5PL_table_t {
    H5PL_type_t pl_type;			/* plugin type	     */
    int         pl_id;                          /* ID for the plugin */
    H5PL_HANDLE handle;			/* plugin handle     */
} H5PL_table_t;


/********************/
/* Local Prototypes */
/********************/

static herr_t H5PL__init_path_table(void);
static htri_t H5PL__find(H5PL_type_t plugin_type, int type_id, char *dir, const void **info);
static htri_t H5PL__open(H5PL_type_t pl_type, char *libname, int plugin_id, const void **pl_info);
static htri_t H5PL__search_table(H5PL_type_t plugin_type, int type_id, const void **info);
static herr_t H5PL__close(H5PL_HANDLE handle);


/*********************/
/* Package Variables */
/*********************/


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/

/* Table for opened plugin libraries */
static size_t		H5PL_table_alloc_g = 0;
static size_t		H5PL_table_used_g = 0;
static H5PL_table_t     *H5PL_table_g = NULL;

/* Table of location paths for plugin libraries */
static char             *H5PL_path_table_g[H5PL_MAX_PATH_NUM];
static size_t           H5PL_num_paths_g = 0;
static hbool_t          H5PL_path_found_g = FALSE;

/* Whether to preload pathnames for plugin libraries */
static hbool_t          H5PL_no_plugin_g = FALSE;


/*--------------------------------------------------------------------------
NAME
   H5PL__init_interface -- Initialize interface-specific information
USAGE
    herr_t H5PL__init_interface()
RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.

--------------------------------------------------------------------------*/
static herr_t
H5PL__init_interface(void)
{
    char        *preload_path;

    FUNC_ENTER_STATIC_NOERR

    /* Retrieve pathnames from HDF5_PLUGIN_PRELOAD if the user sets it
     * to tell the library to load plugin libraries without search.
     */
    if(NULL != (preload_path = HDgetenv("HDF5_PLUGIN_PRELOAD"))) {
        /* Special symbal "::" means no plugin during data reading. */
        if(!HDstrcmp(preload_path, H5PL_NO_PLUGIN))
            H5PL_no_plugin_g = TRUE;
    } /* end if */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5PL__init_interface() */


/*-------------------------------------------------------------------------
 * Function:	H5PL_no_plugin
 *
 * Purpose:	Quick way for filter module to query whether to load plugin 
 *
 * Return:	TRUE:	No plugin loading during data reading
 *
 * 		FALSE:	Load plugin during data reading
 *
 * Programmer:	Raymond Lu
 *              20 February 2013
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5PL_no_plugin(void)
{
    htri_t ret_value;

    FUNC_ENTER_NOAPI(FAIL)

    ret_value = (htri_t)H5PL_no_plugin_g;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL_no_plugin() */


/*-------------------------------------------------------------------------
 * Function:	H5PL_term_interface
 *
 * Purpose:	Terminate the H5PL interface: release all memory, reset all
 *		global variables to initial values. This only happens if all
 *		types have been destroyed from other interfaces.
 *
 * Return:	Success:	Positive if any action was taken that might
 *				affect some other interface; zero otherwise.
 *
 * 		Failure:	Negative.
 *
 * Programmer:	Raymond Lu
 *              20 February 2013
 *
 *-------------------------------------------------------------------------
 */
int
H5PL_term_interface(void)
{
    int  i = 0;
    
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if(H5_interface_initialize_g) {
        size_t u;       /* Local index variable */

	/* Close opened dynamic libraries */
        for(u = 0; u < H5PL_table_used_g; u++)
            H5PL__close((H5PL_table_g[u]).handle);

	/* Free the table of dynamic libraries */
	H5PL_table_g = (H5PL_table_t *)H5MM_xfree(H5PL_table_g);
	H5PL_table_used_g = H5PL_table_alloc_g = 0;

        /* Free the table of search paths */
        for(u = 0; u < H5PL_num_paths_g; u++)
            if(H5PL_path_table_g[u])
                H5PL_path_table_g[u] = (char *)H5MM_xfree(H5PL_path_table_g[u]);
        H5PL_num_paths_g = 0;
        H5PL_path_found_g = FALSE;

	H5_interface_initialize_g = 0;
        i = 1;
    } /* end if */

    FUNC_LEAVE_NOAPI(i)
} /* end H5PL_term_interface() */


/*-------------------------------------------------------------------------
 * Function:	H5PL_load
 *
 * Purpose:	Given the plugin type and identifier, this function searches
 *              and/or loads a dynamic plugin library first among the already
 *              opened libraries then in the designated location paths.
 *
 * Return:	Non-NULL on success/NULL on failure
 *
 * Programmer:	Raymond Lu
 *              13 February 2013
 *
 *-------------------------------------------------------------------------
 */
const void *
H5PL_load(H5PL_type_t type, int id)
{
    htri_t      found;               /* Whether the plugin was found */
    const void  *plugin_info = NULL;
    const void  *ret_value = NULL;

    FUNC_ENTER_NOAPI(NULL)

    /* Check for "no plugins" indicated" */
    if(H5PL_no_plugin_g)
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTLOAD, NULL, "required dynamically loaded plugin filter '%d' is not available", id)

    /* Initialize the location paths for dynamic libraries, if they aren't
     * already set up.
     */
    if(FALSE == H5PL_path_found_g)
        if(H5PL__init_path_table() < 0)
            HGOTO_ERROR(H5E_PLUGIN, H5E_CANTINIT, NULL, "can't initialize search path table")

    /* Search in the table of already loaded plugin libraries */
    if((found = H5PL__search_table(type, id, &plugin_info)) < 0)
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTGET, NULL, "search in table failed")

    /* If not found, iterate through the path table to find the right dynamic library */
    if(!found) {
        size_t	   i;                   /* Local index variable */

        for(i = 0; i < H5PL_num_paths_g; i++) {
            if((found = H5PL__find(type, id, H5PL_path_table_g[i], &plugin_info)) < 0)
                HGOTO_ERROR(H5E_PLUGIN, H5E_CANTGET, NULL, "search in paths failed")
     
            /* Break out if found */
            if(found) {
                HDassert(plugin_info);
                break;
            } /* end if */
        } /* end for */
    } /* end if */

    /* Check if we found the plugin */
    if(found)
        ret_value = plugin_info;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL_load() */


/*-------------------------------------------------------------------------
 * Function:	H5PL__init_path_table
 *
 * Purpose:	Initialize the path table.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              18 March 2013
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5PL__init_path_table(void)
{
    char        *dl_path = NULL;
    char        *origin_dl_path;
    char        *dir;
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_STATIC

    /* Retrieve paths from HDF5_PLUGIN_PATH if the user sets it
     * or from the default paths if it isn't set.
     */
    origin_dl_path = HDgetenv("HDF5_PLUGIN_PATH");
    if(NULL == origin_dl_path)
        dl_path = HDstrdup(H5PL_DEFAULT_PATH);
    else
        dl_path = HDstrdup(origin_dl_path);
    if(NULL == dl_path)
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTALLOC, FAIL, "can't allocate memory for path")

    /* Put paths in the path table.  They are separated by ":" */
    dir = HDstrtok(dl_path, H5PL_PATH_SEPARATOR);
    while(dir) {
        /* Check for too many directories in path */
        if(H5PL_num_paths_g == H5PL_MAX_PATH_NUM)
            HGOTO_ERROR(H5E_PLUGIN, H5E_NOSPACE, FAIL, "too many directories in path for table")
        if(NULL == (H5PL_path_table_g[H5PL_num_paths_g] = HDstrdup(dir)))
            HGOTO_ERROR(H5E_PLUGIN, H5E_CANTALLOC, FAIL, "can't allocate memory for path")
        H5PL_num_paths_g++;
        dir = HDstrtok(NULL, H5PL_PATH_SEPARATOR);
    } /* end while */

    H5PL_path_found_g = TRUE;

done:
    if(dl_path)
        dl_path = (char *)H5MM_xfree(dl_path);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__init_path_table() */


/*-------------------------------------------------------------------------
 * Function:	H5PL__find
 *
 * Purpose:     Given a path, this function opens the directory and envokes
 *              another function to go through all files to find the right 
 *              plugin library. Two function definitions are for Unix and 
 *              Windows.
 *
 * Return:	TRUE on success, 
 *              FALSE on not found,
 *              negative on failure
 *
 * Programmer:	Raymond Lu
 *              13 February 2013
 *
 *-------------------------------------------------------------------------
 */
#ifndef H5_HAVE_WIN32_API
static htri_t
H5PL__find(H5PL_type_t plugin_type, int type_id, char *dir, const void **info)
{
    char           *pathname = NULL;
    DIR            *dirp = NULL;
    struct dirent  *dp;
    htri_t         ret_value = FALSE;

    FUNC_ENTER_STATIC

    /* Open the directory */  
    if(!(dirp = HDopendir(dir)))
        HGOTO_ERROR(H5E_PLUGIN, H5E_OPENERROR, FAIL, "can't open directory")

    /* Iterates through all entries in the directory to find the right plugin library */
    while(NULL != (dp = HDreaddir(dirp))) {
        /* The library we are looking for should be called libxxx.so... on Unix 
         * or libxxx.xxx.dylib on Mac.
         */ 
        if(!HDstrncmp(dp->d_name, "lib", (size_t)3) && 
                (HDstrstr(dp->d_name, ".so") || HDstrstr(dp->d_name, ".dylib"))) {
            h5_stat_t   my_stat;
            size_t      pathname_len;
            htri_t      found_in_dir;

            /* Allocate & initialize the path name */
            pathname_len = HDstrlen(dir) + HDstrlen(dp->d_name) + 2;
            if(NULL == (pathname = (char *)H5MM_malloc(pathname_len)))
                HGOTO_ERROR(H5E_PLUGIN, H5E_CANTALLOC, FAIL, "can't allocate memory for path")
            HDsnprintf(pathname, pathname_len, "%s/%s", dir, dp->d_name);

            /* Get info for directory entry */
            if(HDstat(pathname, &my_stat) == -1)
                HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "can't stat file: %s", HDstrerror(errno))

            /* If it is a directory, skip it */ 
            if(S_ISDIR(my_stat.st_mode))
                continue;

            /* Attempt to open the dynamic library as a filter library */
            if((found_in_dir = H5PL__open(plugin_type, pathname, type_id, info)) < 0)
                HGOTO_ERROR(H5E_PLUGIN, H5E_CANTGET, FAIL, "search in directory failed")
            if(found_in_dir) {
                /* Indicate success */
                HGOTO_DONE(TRUE)
            } /* end if */
            else
                HDassert(pathname);
                pathname = (char *)H5MM_xfree(pathname);
        } /* end if */
    } /* end while */

done:
    if(dirp) 
        if(HDclosedir(dirp) < 0)
            HDONE_ERROR(H5E_FILE, H5E_CLOSEERROR, FAIL, "can't close directory: %s", HDstrerror(errno))
    if(pathname) 
        pathname = (char *)H5MM_xfree(pathname);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__find() */
#else /* H5_HAVE_WIN32_API */
static htri_t
H5PL__find(H5PL_type_t plugin_type, int type_id, char *dir, const void **info)
{
    WIN32_FIND_DATA fdFile;
    HANDLE          hFind;
    char           *pathname = NULL;
    char            service[2048];
    htri_t          ret_value = FALSE;

    FUNC_ENTER_STATIC

    /* Specify a file mask. *.* = We want everything! */
    sprintf(service, "%s\\*.dll", dir);
    if((hFind = FindFirstFile(service, &fdFile)) == INVALID_HANDLE_VALUE)
        HGOTO_ERROR(H5E_PLUGIN, H5E_OPENERROR, FAIL, "can't open directory")

    do {
        /* Find first file will always return "."
         * and ".." as the first two directories.
         */
        if(HDstrcmp(fdFile.cFileName, ".") != 0 && HDstrcmp(fdFile.cFileName, "..") != 0) {
            size_t      pathname_len;
            htri_t      found_in_dir;

            /* Allocate & initialize the path name */
            pathname_len = HDstrlen(dir) + HDstrlen(fdFile.cFileName) + 2;
	    if(NULL == (pathname = (char *)H5MM_malloc(pathname_len)))
                HGOTO_ERROR(H5E_PLUGIN, H5E_CANTALLOC, FAIL, "can't allocate memory for path")
            HDsnprintf(pathname, pathname_len, "%s\\%s", dir, fdFile.cFileName);

            /* Is the entity a File or Folder? */
            if(fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;

            if((found_in_dir = H5PL__open(plugin_type, pathname, type_id, info)) < 0)
                HGOTO_ERROR(H5E_PLUGIN, H5E_CANTGET, FAIL, "search in directory failed")
            if(found_in_dir) {
                /* Indicate success */
                HGOTO_DONE(TRUE)
            } /* end if */
            else
	        HDassert(pathname);
                pathname = (char *)H5MM_xfree(pathname);
        } /* end if */
    } while(FindNextFile(hFind, &fdFile)); /* Find the next file. */

done:
    if(hFind) 
        FindClose(hFind);
    if(pathname) 
        pathname = (char *)H5MM_xfree(pathname);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__find() */
#endif /* H5_HAVE_WIN32_API */


/*-------------------------------------------------------------------------
 * Function:	H5PL__open
 *
 * Purpose:     Iterates through all files to find the right plugin library.
 *              It loads the dynamic plugin library and keeps it on the list 
 *              of loaded libraries. 	
 *
 * Return:	TRUE on success, 
 *              FALSE on not found,
 *              negative on failure
 *
 * Programmer:	Raymond Lu
 *              13 February 2013
 *
 *-------------------------------------------------------------------------
 */
static htri_t
H5PL__open(H5PL_type_t pl_type, char *libname, int pl_id, const void **pl_info)
{
    H5PL_HANDLE    handle = NULL;
    htri_t         ret_value = FALSE;

    FUNC_ENTER_STATIC

    /* There are different reasons why a library can't be open, e.g. wrong architecture.
     * simply continue if we can't open it.
     */
    if(NULL == (handle = H5PL_OPEN_DLIB(libname))) {
        H5PL_CLR_ERROR; /* clear error */
    } /* end if */
    else {
        H5PL_get_plugin_info_t get_plugin_info = NULL;

        /* Return a handle for the function H5PLget_plugin_info in the dynamic library.
         * The plugin library is suppose to define this function.
         */
        if(NULL == (get_plugin_info = (H5PL_get_plugin_info_t)H5PL_GET_LIB_FUNC(handle, "H5PLget_plugin_info"))) {
            if(H5PL__close(handle) < 0)
                HGOTO_ERROR(H5E_PLUGIN, H5E_CLOSEERROR, FAIL, "can't close dynamic library")
        } /* end if */
        else {
            const H5Z_class2_t *plugin_info;

            /* Invoke H5PLget_plugin_info to verify this is the right library we are looking for.
             * Move on if it isn't.
             */
            if(NULL == (plugin_info = (const H5Z_class2_t *)(*get_plugin_info)())) {
                if(H5PL__close(handle) < 0)
                    HGOTO_ERROR(H5E_PLUGIN, H5E_CLOSEERROR, FAIL, "can't close dynamic library")
                HGOTO_ERROR(H5E_PLUGIN, H5E_CANTGET, FAIL, "can't get plugin info")
            } /* end if */

            /* Successfully found plugin library, check if it's the right one */
            if(plugin_info->id == pl_id) {
                /* Expand the table if it is too small */
                if(H5PL_table_used_g >= H5PL_table_alloc_g) {
                    size_t n = MAX(H5Z_MAX_NFILTERS, 2 * H5PL_table_alloc_g);
                    H5PL_table_t *table = (H5PL_table_t *)H5MM_realloc(H5PL_table_g, n * sizeof(H5PL_table_t));

                    if(!table)
                        HGOTO_ERROR(H5E_RESOURCE, H5E_CANTALLOC, FAIL, "unable to extend dynamic library table")

                    H5PL_table_g = table;
                    H5PL_table_alloc_g = n;
                } /* end if */

                (H5PL_table_g[H5PL_table_used_g]).handle = handle;
                (H5PL_table_g[H5PL_table_used_g]).pl_type = pl_type;
                (H5PL_table_g[H5PL_table_used_g]).pl_id = plugin_info->id;
                H5PL_table_used_g++;

                /* Set the plugin info to return */
                *pl_info = (const void *)plugin_info;
     
                /* Indicate success */
                ret_value = TRUE;
            } /* end if */
            else
                if(H5PL__close(handle) < 0)
                    HGOTO_ERROR(H5E_PLUGIN, H5E_CLOSEERROR, FAIL, "can't close dynamic library")
        } /* end if */
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__open() */


/*-------------------------------------------------------------------------
 * Function:	H5PL__search_table
 *
 * Purpose:     Search in the list of already opened dynamic libraries
 *              to see if the one we are looking for is already opened.
 *
 * Return:	TRUE on success, 
 *              FALSE on not found,
 *              Negative on failure
 *
 * Programmer:	Raymond Lu
 *              13 February 2013
 *
 *-------------------------------------------------------------------------
 */
static htri_t
H5PL__search_table(H5PL_type_t plugin_type, int type_id, const void **info)
{
    htri_t         ret_value = FALSE;

    FUNC_ENTER_STATIC

    /* Search in the table of already opened dynamic libraries */
    if(H5PL_table_used_g > 0) {
        size_t         i;

        for(i = 0; i < H5PL_table_used_g; i++) {
            if((plugin_type == (H5PL_table_g[i]).pl_type) && (type_id == (H5PL_table_g[i]).pl_id)) {
                H5PL_get_plugin_info_t get_plugin_info;
                const H5Z_class2_t   *plugin_info;

                if(NULL == (get_plugin_info = (H5PL_get_plugin_info_t)H5PL_GET_LIB_FUNC((H5PL_table_g[i]).handle, "H5PLget_plugin_info")))
		    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTGET, FAIL, "can't get function for H5PLget_plugin_info")

	        if(NULL == (plugin_info = (const H5Z_class2_t *)(*get_plugin_info)()))
		    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTGET, FAIL, "can't get plugin info")

	        *info = plugin_info;
                HGOTO_DONE(TRUE)
            } /* end if */
        } /* end for */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__search_table() */


/*-------------------------------------------------------------------------
 * Function:	H5PL__close
 *
 * Purpose:     Closes the handle for dynamic library	
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 *              13 February 2013
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5PL__close(H5PL_HANDLE handle)
{
    FUNC_ENTER_STATIC_NOERR

    H5PL_CLOSE_LIB(handle);
   
    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5PL__close() */
#endif /*H5_VMS*/
