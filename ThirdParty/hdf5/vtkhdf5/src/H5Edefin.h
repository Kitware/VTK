/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Generated automatically by bin/make_err -- do not edit */
/* Add new errors to H5err.txt file */

#ifndef H5Edefin_H
#define H5Edefin_H

/* Major error IDs */
hid_t H5E_ARGS_g           = H5I_INVALID_HID;      /* Invalid arguments to routine */
hid_t H5E_ATTR_g           = H5I_INVALID_HID;      /* Attribute */
hid_t H5E_BTREE_g          = H5I_INVALID_HID;      /* B-Tree node */
hid_t H5E_CACHE_g          = H5I_INVALID_HID;      /* Object cache */
hid_t H5E_CONTEXT_g        = H5I_INVALID_HID;      /* API Context */
hid_t H5E_DATASET_g        = H5I_INVALID_HID;      /* Dataset */
hid_t H5E_DATASPACE_g      = H5I_INVALID_HID;      /* Dataspace */
hid_t H5E_DATATYPE_g       = H5I_INVALID_HID;      /* Datatype */
hid_t H5E_EARRAY_g         = H5I_INVALID_HID;      /* Extensible Array */
hid_t H5E_EFL_g            = H5I_INVALID_HID;      /* External file list */
hid_t H5E_ERROR_g          = H5I_INVALID_HID;      /* Error API */
hid_t H5E_EVENTSET_g       = H5I_INVALID_HID;      /* Event Set */
hid_t H5E_FARRAY_g         = H5I_INVALID_HID;      /* Fixed Array */
hid_t H5E_FILE_g           = H5I_INVALID_HID;      /* File accessibility */
hid_t H5E_FSPACE_g         = H5I_INVALID_HID;      /* Free Space Manager */
hid_t H5E_FUNC_g           = H5I_INVALID_HID;      /* Function entry/exit */
hid_t H5E_HEAP_g           = H5I_INVALID_HID;      /* Heap */
hid_t H5E_ID_g             = H5I_INVALID_HID;      /* Object ID */
hid_t H5E_INTERNAL_g       = H5I_INVALID_HID;      /* Internal error (too specific to document in detail) */
hid_t H5E_IO_g             = H5I_INVALID_HID;      /* Low-level I/O */
hid_t H5E_LIB_g            = H5I_INVALID_HID;      /* General library infrastructure */
hid_t H5E_LINK_g           = H5I_INVALID_HID;      /* Links */
hid_t H5E_MAP_g            = H5I_INVALID_HID;      /* Map */
hid_t H5E_NONE_MAJOR_g     = H5I_INVALID_HID;      /* No error */
hid_t H5E_OHDR_g           = H5I_INVALID_HID;      /* Object header */
hid_t H5E_PAGEBUF_g        = H5I_INVALID_HID;      /* Page Buffering */
hid_t H5E_PLINE_g          = H5I_INVALID_HID;      /* Data filters */
hid_t H5E_PLIST_g          = H5I_INVALID_HID;      /* Property lists */
hid_t H5E_PLUGIN_g         = H5I_INVALID_HID;      /* Plugin for dynamically loaded library */
hid_t H5E_REFERENCE_g      = H5I_INVALID_HID;      /* References */
hid_t H5E_RESOURCE_g       = H5I_INVALID_HID;      /* Resource unavailable */
hid_t H5E_RS_g             = H5I_INVALID_HID;      /* Reference Counted Strings */
hid_t H5E_SLIST_g          = H5I_INVALID_HID;      /* Skip Lists */
hid_t H5E_SOHM_g           = H5I_INVALID_HID;      /* Shared Object Header Messages */
hid_t H5E_STORAGE_g        = H5I_INVALID_HID;      /* Data storage */
hid_t H5E_SYM_g            = H5I_INVALID_HID;      /* Symbol table */
hid_t H5E_TST_g            = H5I_INVALID_HID;      /* Ternary Search Trees */
hid_t H5E_VFL_g            = H5I_INVALID_HID;      /* Virtual File Layer */
hid_t H5E_VOL_g            = H5I_INVALID_HID;      /* Virtual Object Layer */

/* Number of major error messages */
#define H5E_NUM_MAJ_ERRORS 39

/* Minor error IDs */

/* ARGS: Argument errors */
hid_t H5E_BADRANGE_g       = H5I_INVALID_HID;      /* Out of range */
hid_t H5E_BADTYPE_g        = H5I_INVALID_HID;      /* Inappropriate type */
hid_t H5E_BADVALUE_g       = H5I_INVALID_HID;      /* Bad value */
hid_t H5E_UNINITIALIZED_g  = H5I_INVALID_HID;      /* Information is uinitialized */
hid_t H5E_UNSUPPORTED_g    = H5I_INVALID_HID;      /* Feature is unsupported */

/* ASYNC: Asynchronous operation errors */
hid_t H5E_CANTCANCEL_g     = H5I_INVALID_HID;      /* Can't cancel operation */
hid_t H5E_CANTWAIT_g       = H5I_INVALID_HID;      /* Can't wait on operation */

/* BTREE: B-tree related errors */
hid_t H5E_CANTDECODE_g     = H5I_INVALID_HID;      /* Unable to decode value */
hid_t H5E_CANTENCODE_g     = H5I_INVALID_HID;      /* Unable to encode value */
hid_t H5E_CANTFIND_g       = H5I_INVALID_HID;      /* Unable to check for record */
hid_t H5E_CANTINSERT_g     = H5I_INVALID_HID;      /* Unable to insert object */
hid_t H5E_CANTLIST_g       = H5I_INVALID_HID;      /* Unable to list node */
hid_t H5E_CANTMODIFY_g     = H5I_INVALID_HID;      /* Unable to modify record */
hid_t H5E_CANTREDISTRIBUTE_g = H5I_INVALID_HID;      /* Unable to redistribute records */
hid_t H5E_CANTREMOVE_g     = H5I_INVALID_HID;      /* Unable to remove object */
hid_t H5E_CANTSPLIT_g      = H5I_INVALID_HID;      /* Unable to split node */
hid_t H5E_CANTSWAP_g       = H5I_INVALID_HID;      /* Unable to swap records */
hid_t H5E_EXISTS_g         = H5I_INVALID_HID;      /* Object already exists */
hid_t H5E_NOTFOUND_g       = H5I_INVALID_HID;      /* Object not found */

/* CACHE: Cache related errors */
hid_t H5E_CANTCLEAN_g      = H5I_INVALID_HID;      /* Unable to mark metadata as clean */
hid_t H5E_CANTCORK_g       = H5I_INVALID_HID;      /* Unable to cork an object */
hid_t H5E_CANTDEPEND_g     = H5I_INVALID_HID;      /* Unable to create a flush dependency */
hid_t H5E_CANTDIRTY_g      = H5I_INVALID_HID;      /* Unable to mark metadata as dirty */
hid_t H5E_CANTEXPUNGE_g    = H5I_INVALID_HID;      /* Unable to expunge a metadata cache entry */
hid_t H5E_CANTFLUSH_g      = H5I_INVALID_HID;      /* Unable to flush data from cache */
hid_t H5E_CANTINS_g        = H5I_INVALID_HID;      /* Unable to insert metadata into cache */
hid_t H5E_CANTLOAD_g       = H5I_INVALID_HID;      /* Unable to load metadata into cache */
hid_t H5E_CANTMARKCLEAN_g  = H5I_INVALID_HID;      /* Unable to mark a pinned entry as clean */
hid_t H5E_CANTMARKDIRTY_g  = H5I_INVALID_HID;      /* Unable to mark a pinned entry as dirty */
hid_t H5E_CANTMARKSERIALIZED_g = H5I_INVALID_HID;      /* Unable to mark an entry as serialized */
hid_t H5E_CANTMARKUNSERIALIZED_g = H5I_INVALID_HID;      /* Unable to mark an entry as unserialized */
hid_t H5E_CANTNOTIFY_g     = H5I_INVALID_HID;      /* Unable to notify object about action */
hid_t H5E_CANTPIN_g        = H5I_INVALID_HID;      /* Unable to pin cache entry */
hid_t H5E_CANTPROTECT_g    = H5I_INVALID_HID;      /* Unable to protect metadata */
hid_t H5E_CANTRESIZE_g     = H5I_INVALID_HID;      /* Unable to resize a metadata cache entry */
hid_t H5E_CANTSERIALIZE_g  = H5I_INVALID_HID;      /* Unable to serialize data from cache */
hid_t H5E_CANTTAG_g        = H5I_INVALID_HID;      /* Unable to tag metadata in the cache */
hid_t H5E_CANTUNCORK_g     = H5I_INVALID_HID;      /* Unable to uncork an object */
hid_t H5E_CANTUNDEPEND_g   = H5I_INVALID_HID;      /* Unable to destroy a flush dependency */
hid_t H5E_CANTUNPIN_g      = H5I_INVALID_HID;      /* Unable to un-pin cache entry */
hid_t H5E_CANTUNPROTECT_g  = H5I_INVALID_HID;      /* Unable to unprotect metadata */
hid_t H5E_CANTUNSERIALIZE_g = H5I_INVALID_HID;      /* Unable to mark metadata as unserialized */
hid_t H5E_LOGGING_g        = H5I_INVALID_HID;      /* Failure in the cache logging framework */
hid_t H5E_NOTCACHED_g      = H5I_INVALID_HID;      /* Metadata not currently cached */
hid_t H5E_PROTECT_g        = H5I_INVALID_HID;      /* Protected metadata error */
hid_t H5E_SYSTEM_g         = H5I_INVALID_HID;      /* Internal error detected */

/* DSPACE: Dataspace errors */
hid_t H5E_BADSELECT_g      = H5I_INVALID_HID;      /* Invalid selection */
hid_t H5E_CANTAPPEND_g     = H5I_INVALID_HID;      /* Can't append object */
hid_t H5E_CANTCLIP_g       = H5I_INVALID_HID;      /* Can't clip hyperslab region */
hid_t H5E_CANTCOMPARE_g    = H5I_INVALID_HID;      /* Can't compare objects */
hid_t H5E_CANTCOUNT_g      = H5I_INVALID_HID;      /* Can't count elements */
hid_t H5E_CANTNEXT_g       = H5I_INVALID_HID;      /* Can't move to next iterator location */
hid_t H5E_CANTSELECT_g     = H5I_INVALID_HID;      /* Can't select hyperslab */
hid_t H5E_INCONSISTENTSTATE_g = H5I_INVALID_HID;      /* Internal states are inconsistent */

/* FILE: Generic low-level file I/O errors */
hid_t H5E_CLOSEERROR_g     = H5I_INVALID_HID;      /* Close failed */
hid_t H5E_FCNTL_g          = H5I_INVALID_HID;      /* File control (fcntl) failed */
hid_t H5E_OVERFLOW_g       = H5I_INVALID_HID;      /* Address overflowed */
hid_t H5E_READERROR_g      = H5I_INVALID_HID;      /* Read failed */
hid_t H5E_SEEKERROR_g      = H5I_INVALID_HID;      /* Seek failed */
hid_t H5E_WRITEERROR_g     = H5I_INVALID_HID;      /* Write failed */

/* FILEACC: File accessibility errors */
hid_t H5E_BADFILE_g        = H5I_INVALID_HID;      /* Bad file ID accessed */
hid_t H5E_CANTCLOSEFILE_g  = H5I_INVALID_HID;      /* Unable to close file */
hid_t H5E_CANTCREATE_g     = H5I_INVALID_HID;      /* Unable to create file */
hid_t H5E_CANTDELETEFILE_g = H5I_INVALID_HID;      /* Unable to delete file */
hid_t H5E_CANTLOCKFILE_g   = H5I_INVALID_HID;      /* Unable to lock file */
hid_t H5E_CANTOPENFILE_g   = H5I_INVALID_HID;      /* Unable to open file */
hid_t H5E_CANTUNLOCKFILE_g = H5I_INVALID_HID;      /* Unable to unlock file */
hid_t H5E_FILEEXISTS_g     = H5I_INVALID_HID;      /* File already exists */
hid_t H5E_FILEOPEN_g       = H5I_INVALID_HID;      /* File already open */
hid_t H5E_MOUNT_g          = H5I_INVALID_HID;      /* File mount error */
hid_t H5E_NOTHDF5_g        = H5I_INVALID_HID;      /* Not an HDF5 file */
hid_t H5E_TRUNCATED_g      = H5I_INVALID_HID;      /* File has been truncated */
hid_t H5E_UNMOUNT_g        = H5I_INVALID_HID;      /* File unmount error */

/* FSPACE: Free space errors */
hid_t H5E_CANTMERGE_g      = H5I_INVALID_HID;      /* Can't merge objects */
hid_t H5E_CANTREVIVE_g     = H5I_INVALID_HID;      /* Can't revive object */
hid_t H5E_CANTSHRINK_g     = H5I_INVALID_HID;      /* Can't shrink container */

/* FUNC: Function entry/exit interface errors */
hid_t H5E_ALREADYINIT_g    = H5I_INVALID_HID;      /* Object already initialized */
hid_t H5E_CANTINIT_g       = H5I_INVALID_HID;      /* Unable to initialize object */
hid_t H5E_CANTRELEASE_g    = H5I_INVALID_HID;      /* Unable to release object */

/* GROUP: Group related errors */
hid_t H5E_CANTCLOSEOBJ_g   = H5I_INVALID_HID;      /* Can't close object */
hid_t H5E_CANTOPENOBJ_g    = H5I_INVALID_HID;      /* Can't open object */
hid_t H5E_COMPLEN_g        = H5I_INVALID_HID;      /* Name component is too long */
hid_t H5E_PATH_g           = H5I_INVALID_HID;      /* Problem with path to object */

/* HEAP: Heap errors */
hid_t H5E_CANTATTACH_g     = H5I_INVALID_HID;      /* Can't attach object */
hid_t H5E_CANTCOMPUTE_g    = H5I_INVALID_HID;      /* Can't compute value */
hid_t H5E_CANTEXTEND_g     = H5I_INVALID_HID;      /* Can't extend heap's space */
hid_t H5E_CANTOPERATE_g    = H5I_INVALID_HID;      /* Can't operate on object */
hid_t H5E_CANTRESTORE_g    = H5I_INVALID_HID;      /* Can't restore condition */
hid_t H5E_CANTUPDATE_g     = H5I_INVALID_HID;      /* Can't update object */

/* ID: Object ID related errors */
hid_t H5E_BADGROUP_g       = H5I_INVALID_HID;      /* Unable to find ID group information */
hid_t H5E_BADID_g          = H5I_INVALID_HID;      /* Unable to find ID information (already closed?) */
hid_t H5E_CANTDEC_g        = H5I_INVALID_HID;      /* Unable to decrement reference count */
hid_t H5E_CANTINC_g        = H5I_INVALID_HID;      /* Unable to increment reference count */
hid_t H5E_CANTREGISTER_g   = H5I_INVALID_HID;      /* Unable to register new ID */
hid_t H5E_NOIDS_g          = H5I_INVALID_HID;      /* Out of IDs for group */

/* LINK: Link related errors */
hid_t H5E_CANTMOVE_g       = H5I_INVALID_HID;      /* Can't move object */
hid_t H5E_CANTSORT_g       = H5I_INVALID_HID;      /* Can't sort objects */
hid_t H5E_NLINKS_g         = H5I_INVALID_HID;      /* Too many soft links in path */
hid_t H5E_NOTREGISTERED_g  = H5I_INVALID_HID;      /* Link class not registered */
hid_t H5E_TRAVERSE_g       = H5I_INVALID_HID;      /* Link traversal failure */

/* MAP: Map related errors */
hid_t H5E_CANTPUT_g        = H5I_INVALID_HID;      /* Can't put value */

/* MPI: Parallel MPI errors */
hid_t H5E_CANTGATHER_g     = H5I_INVALID_HID;      /* Can't gather data */
hid_t H5E_CANTRECV_g       = H5I_INVALID_HID;      /* Can't receive data */
hid_t H5E_MPI_g            = H5I_INVALID_HID;      /* Some MPI function failed */
hid_t H5E_MPIERRSTR_g      = H5I_INVALID_HID;      /* MPI Error String */
hid_t H5E_NO_INDEPENDENT_g = H5I_INVALID_HID;      /* Can't perform independent IO */

/* NONE: No error */
hid_t H5E_NONE_MINOR_g     = H5I_INVALID_HID;      /* No error */

/* OHDR: Object header related errors */
hid_t H5E_ALIGNMENT_g      = H5I_INVALID_HID;      /* Alignment error */
hid_t H5E_BADITER_g        = H5I_INVALID_HID;      /* Iteration failed */
hid_t H5E_BADMESG_g        = H5I_INVALID_HID;      /* Unrecognized message */
hid_t H5E_CANTDELETE_g     = H5I_INVALID_HID;      /* Can't delete message */
hid_t H5E_CANTPACK_g       = H5I_INVALID_HID;      /* Can't pack messages */
hid_t H5E_CANTRENAME_g     = H5I_INVALID_HID;      /* Unable to rename object */
hid_t H5E_CANTRESET_g      = H5I_INVALID_HID;      /* Can't reset object */
hid_t H5E_LINKCOUNT_g      = H5I_INVALID_HID;      /* Bad object header link count */
hid_t H5E_VERSION_g        = H5I_INVALID_HID;      /* Wrong version number */

/* PIPELINE: I/O pipeline errors */
hid_t H5E_CALLBACK_g       = H5I_INVALID_HID;      /* Callback failed */
hid_t H5E_CANAPPLY_g       = H5I_INVALID_HID;      /* Error from filter 'can apply' callback */
hid_t H5E_CANTFILTER_g     = H5I_INVALID_HID;      /* Filter operation failed */
hid_t H5E_NOENCODER_g      = H5I_INVALID_HID;      /* Filter present but encoding disabled */
hid_t H5E_NOFILTER_g       = H5I_INVALID_HID;      /* Requested filter is not available */
hid_t H5E_SETLOCAL_g       = H5I_INVALID_HID;      /* Error from filter 'set local' callback */

/* PLIST: Property list errors */
hid_t H5E_CANTGET_g        = H5I_INVALID_HID;      /* Can't get value */
hid_t H5E_CANTSET_g        = H5I_INVALID_HID;      /* Can't set value */
hid_t H5E_DUPCLASS_g       = H5I_INVALID_HID;      /* Duplicate class name in parent class */
hid_t H5E_SETDISALLOWED_g  = H5I_INVALID_HID;      /* Disallowed operation */

/* PLUGIN: Plugin errors */
hid_t H5E_OPENERROR_g      = H5I_INVALID_HID;      /* Can't open directory or file */

/* RESOURCE: Resource errors */
hid_t H5E_ALREADYEXISTS_g  = H5I_INVALID_HID;      /* Object already exists */
hid_t H5E_CANTALLOC_g      = H5I_INVALID_HID;      /* Can't allocate space */
hid_t H5E_CANTCOPY_g       = H5I_INVALID_HID;      /* Unable to copy object */
hid_t H5E_CANTFREE_g       = H5I_INVALID_HID;      /* Unable to free object */
hid_t H5E_CANTGC_g         = H5I_INVALID_HID;      /* Unable to garbage collect */
hid_t H5E_CANTGETSIZE_g    = H5I_INVALID_HID;      /* Unable to compute size */
hid_t H5E_CANTLOCK_g       = H5I_INVALID_HID;      /* Unable to lock object */
hid_t H5E_CANTUNLOCK_g     = H5I_INVALID_HID;      /* Unable to unlock object */
hid_t H5E_NOSPACE_g        = H5I_INVALID_HID;      /* No space available for allocation */
hid_t H5E_OBJOPEN_g        = H5I_INVALID_HID;      /* Object is already open */

/* SYSTEM: System level errors */
hid_t H5E_SYSERRSTR_g      = H5I_INVALID_HID;      /* System error message */

/* TYPECONV: Datatype conversion errors */
hid_t H5E_BADSIZE_g        = H5I_INVALID_HID;      /* Bad size for object */
hid_t H5E_CANTCONVERT_g    = H5I_INVALID_HID;      /* Can't convert datatypes */

/* Number of minor error messages */
#define H5E_NUM_MIN_ERRORS 140

#endif /* H5Edefin_H */
