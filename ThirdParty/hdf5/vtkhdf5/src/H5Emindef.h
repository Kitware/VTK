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

#ifndef H5Emindef_H
#define H5Emindef_H

/***********************************/
/* Minor error message definitions */
/***********************************/

/* clang-format off */

/* ARGS: Argument errors */
static const H5E_msg_t H5E_BADRANGE_msg_s = {false, "Out of range", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_BADTYPE_msg_s = {false, "Inappropriate type", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_BADVALUE_msg_s = {false, "Bad value", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_UNINITIALIZED_msg_s = {false, "Information is uinitialized", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_UNSUPPORTED_msg_s = {false, "Feature is unsupported", H5E_MINOR, &H5E_err_cls_s};

/* ASYNC: Asynchronous operation errors */
static const H5E_msg_t H5E_CANTCANCEL_msg_s = {false, "Can't cancel operation", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTWAIT_msg_s = {false, "Can't wait on operation", H5E_MINOR, &H5E_err_cls_s};

/* BTREE: B-tree related errors */
static const H5E_msg_t H5E_CANTDECODE_msg_s = {false, "Unable to decode value", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTENCODE_msg_s = {false, "Unable to encode value", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTFIND_msg_s = {false, "Unable to check for record", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTINSERT_msg_s = {false, "Unable to insert object", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTLIST_msg_s = {false, "Unable to list node", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTMODIFY_msg_s = {false, "Unable to modify record", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTREDISTRIBUTE_msg_s = {false, "Unable to redistribute records", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTREMOVE_msg_s = {false, "Unable to remove object", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTSPLIT_msg_s = {false, "Unable to split node", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTSWAP_msg_s = {false, "Unable to swap records", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_EXISTS_msg_s = {false, "Object already exists", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_NOTFOUND_msg_s = {false, "Object not found", H5E_MINOR, &H5E_err_cls_s};

/* CACHE: Cache related errors */
static const H5E_msg_t H5E_CANTCLEAN_msg_s = {false, "Unable to mark metadata as clean", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTCORK_msg_s = {false, "Unable to cork an object", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTDEPEND_msg_s = {false, "Unable to create a flush dependency", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTDIRTY_msg_s = {false, "Unable to mark metadata as dirty", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTEXPUNGE_msg_s = {false, "Unable to expunge a metadata cache entry", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTFLUSH_msg_s = {false, "Unable to flush data from cache", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTINS_msg_s = {false, "Unable to insert metadata into cache", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTLOAD_msg_s = {false, "Unable to load metadata into cache", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTMARKCLEAN_msg_s = {false, "Unable to mark a pinned entry as clean", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTMARKDIRTY_msg_s = {false, "Unable to mark a pinned entry as dirty", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTMARKSERIALIZED_msg_s = {false, "Unable to mark an entry as serialized", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTMARKUNSERIALIZED_msg_s = {false, "Unable to mark an entry as unserialized", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTNOTIFY_msg_s = {false, "Unable to notify object about action", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTPIN_msg_s = {false, "Unable to pin cache entry", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTPROTECT_msg_s = {false, "Unable to protect metadata", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTRESIZE_msg_s = {false, "Unable to resize a metadata cache entry", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTSERIALIZE_msg_s = {false, "Unable to serialize data from cache", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTTAG_msg_s = {false, "Unable to tag metadata in the cache", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTUNCORK_msg_s = {false, "Unable to uncork an object", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTUNDEPEND_msg_s = {false, "Unable to destroy a flush dependency", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTUNPIN_msg_s = {false, "Unable to un-pin cache entry", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTUNPROTECT_msg_s = {false, "Unable to unprotect metadata", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTUNSERIALIZE_msg_s = {false, "Unable to mark metadata as unserialized", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_LOGGING_msg_s = {false, "Failure in the cache logging framework", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_NOTCACHED_msg_s = {false, "Metadata not currently cached", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_PROTECT_msg_s = {false, "Protected metadata error", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_SYSTEM_msg_s = {false, "Internal error detected", H5E_MINOR, &H5E_err_cls_s};

/* DSPACE: Dataspace errors */
static const H5E_msg_t H5E_BADSELECT_msg_s = {false, "Invalid selection", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTAPPEND_msg_s = {false, "Can't append object", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTCLIP_msg_s = {false, "Can't clip hyperslab region", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTCOMPARE_msg_s = {false, "Can't compare objects", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTCOUNT_msg_s = {false, "Can't count elements", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTNEXT_msg_s = {false, "Can't move to next iterator location", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTSELECT_msg_s = {false, "Can't select hyperslab", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_INCONSISTENTSTATE_msg_s = {false, "Internal states are inconsistent", H5E_MINOR, &H5E_err_cls_s};

/* FILE: Generic low-level file I/O errors */
static const H5E_msg_t H5E_CLOSEERROR_msg_s = {false, "Close failed", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_FCNTL_msg_s = {false, "File control (fcntl) failed", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_OVERFLOW_msg_s = {false, "Address overflowed", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_READERROR_msg_s = {false, "Read failed", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_SEEKERROR_msg_s = {false, "Seek failed", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_WRITEERROR_msg_s = {false, "Write failed", H5E_MINOR, &H5E_err_cls_s};

/* FILEACC: File accessibility errors */
static const H5E_msg_t H5E_BADFILE_msg_s = {false, "Bad file ID accessed", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTCLOSEFILE_msg_s = {false, "Unable to close file", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTCREATE_msg_s = {false, "Unable to create file", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTDELETEFILE_msg_s = {false, "Unable to delete file", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTLOCKFILE_msg_s = {false, "Unable to lock file", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTOPENFILE_msg_s = {false, "Unable to open file", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTUNLOCKFILE_msg_s = {false, "Unable to unlock file", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_FILEEXISTS_msg_s = {false, "File already exists", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_FILEOPEN_msg_s = {false, "File already open", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_MOUNT_msg_s = {false, "File mount error", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_NOTHDF5_msg_s = {false, "Not an HDF5 file", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_TRUNCATED_msg_s = {false, "File has been truncated", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_UNMOUNT_msg_s = {false, "File unmount error", H5E_MINOR, &H5E_err_cls_s};

/* FSPACE: Free space errors */
static const H5E_msg_t H5E_CANTMERGE_msg_s = {false, "Can't merge objects", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTREVIVE_msg_s = {false, "Can't revive object", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTSHRINK_msg_s = {false, "Can't shrink container", H5E_MINOR, &H5E_err_cls_s};

/* FUNC: Function entry/exit interface errors */
static const H5E_msg_t H5E_ALREADYINIT_msg_s = {false, "Object already initialized", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTINIT_msg_s = {false, "Unable to initialize object", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTRELEASE_msg_s = {false, "Unable to release object", H5E_MINOR, &H5E_err_cls_s};

/* GROUP: Group related errors */
static const H5E_msg_t H5E_CANTCLOSEOBJ_msg_s = {false, "Can't close object", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTOPENOBJ_msg_s = {false, "Can't open object", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_COMPLEN_msg_s = {false, "Name component is too long", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_PATH_msg_s = {false, "Problem with path to object", H5E_MINOR, &H5E_err_cls_s};

/* HEAP: Heap errors */
static const H5E_msg_t H5E_CANTATTACH_msg_s = {false, "Can't attach object", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTCOMPUTE_msg_s = {false, "Can't compute value", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTEXTEND_msg_s = {false, "Can't extend heap's space", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTOPERATE_msg_s = {false, "Can't operate on object", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTRESTORE_msg_s = {false, "Can't restore condition", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTUPDATE_msg_s = {false, "Can't update object", H5E_MINOR, &H5E_err_cls_s};

/* ID: Object ID related errors */
static const H5E_msg_t H5E_BADGROUP_msg_s = {false, "Unable to find ID group information", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_BADID_msg_s = {false, "Unable to find ID information (already closed?)", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTDEC_msg_s = {false, "Unable to decrement reference count", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTINC_msg_s = {false, "Unable to increment reference count", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTREGISTER_msg_s = {false, "Unable to register new ID", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_NOIDS_msg_s = {false, "Out of IDs for group", H5E_MINOR, &H5E_err_cls_s};

/* LINK: Link related errors */
static const H5E_msg_t H5E_CANTMOVE_msg_s = {false, "Can't move object", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTSORT_msg_s = {false, "Can't sort objects", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_NLINKS_msg_s = {false, "Too many soft links in path", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_NOTREGISTERED_msg_s = {false, "Link class not registered", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_TRAVERSE_msg_s = {false, "Link traversal failure", H5E_MINOR, &H5E_err_cls_s};

/* MAP: Map related errors */
static const H5E_msg_t H5E_CANTPUT_msg_s = {false, "Can't put value", H5E_MINOR, &H5E_err_cls_s};

/* MPI: Parallel MPI errors */
static const H5E_msg_t H5E_CANTGATHER_msg_s = {false, "Can't gather data", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTRECV_msg_s = {false, "Can't receive data", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_MPI_msg_s = {false, "Some MPI function failed", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_MPIERRSTR_msg_s = {false, "MPI Error String", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_NO_INDEPENDENT_msg_s = {false, "Can't perform independent IO", H5E_MINOR, &H5E_err_cls_s};

/* NONE: No error */
static const H5E_msg_t H5E_NONE_MINOR_msg_s = {false, "No error", H5E_MINOR, &H5E_err_cls_s};

/* OHDR: Object header related errors */
static const H5E_msg_t H5E_ALIGNMENT_msg_s = {false, "Alignment error", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_BADITER_msg_s = {false, "Iteration failed", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_BADMESG_msg_s = {false, "Unrecognized message", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTDELETE_msg_s = {false, "Can't delete message", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTPACK_msg_s = {false, "Can't pack messages", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTRENAME_msg_s = {false, "Unable to rename object", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTRESET_msg_s = {false, "Can't reset object", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_LINKCOUNT_msg_s = {false, "Bad object header link count", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_VERSION_msg_s = {false, "Wrong version number", H5E_MINOR, &H5E_err_cls_s};

/* PIPELINE: I/O pipeline errors */
static const H5E_msg_t H5E_CALLBACK_msg_s = {false, "Callback failed", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANAPPLY_msg_s = {false, "Error from filter 'can apply' callback", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTFILTER_msg_s = {false, "Filter operation failed", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_NOENCODER_msg_s = {false, "Filter present but encoding disabled", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_NOFILTER_msg_s = {false, "Requested filter is not available", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_SETLOCAL_msg_s = {false, "Error from filter 'set local' callback", H5E_MINOR, &H5E_err_cls_s};

/* PLIST: Property list errors */
static const H5E_msg_t H5E_CANTGET_msg_s = {false, "Can't get value", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTSET_msg_s = {false, "Can't set value", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_DUPCLASS_msg_s = {false, "Duplicate class name in parent class", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_SETDISALLOWED_msg_s = {false, "Disallowed operation", H5E_MINOR, &H5E_err_cls_s};

/* PLUGIN: Plugin errors */
static const H5E_msg_t H5E_OPENERROR_msg_s = {false, "Can't open directory or file", H5E_MINOR, &H5E_err_cls_s};

/* RESOURCE: Resource errors */
static const H5E_msg_t H5E_ALREADYEXISTS_msg_s = {false, "Object already exists", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTALLOC_msg_s = {false, "Can't allocate space", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTCOPY_msg_s = {false, "Unable to copy object", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTFREE_msg_s = {false, "Unable to free object", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTGC_msg_s = {false, "Unable to garbage collect", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTGETSIZE_msg_s = {false, "Unable to compute size", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTLOCK_msg_s = {false, "Unable to lock object", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTUNLOCK_msg_s = {false, "Unable to unlock object", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_NOSPACE_msg_s = {false, "No space available for allocation", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_OBJOPEN_msg_s = {false, "Object is already open", H5E_MINOR, &H5E_err_cls_s};

/* SYSTEM: System level errors */
static const H5E_msg_t H5E_SYSERRSTR_msg_s = {false, "System error message", H5E_MINOR, &H5E_err_cls_s};

/* TYPECONV: Datatype conversion errors */
static const H5E_msg_t H5E_BADSIZE_msg_s = {false, "Bad size for object", H5E_MINOR, &H5E_err_cls_s};
static const H5E_msg_t H5E_CANTCONVERT_msg_s = {false, "Can't convert datatypes", H5E_MINOR, &H5E_err_cls_s};
/* clang-format on */

#endif /* H5Emindef_H */
