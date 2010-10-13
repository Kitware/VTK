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
 * Created:             H5Gprivate.h
 *                      Jul 11 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             Library-visible declarations.
 *
 *-------------------------------------------------------------------------
 */

#ifndef _H5Gprivate_H
#define _H5Gprivate_H

/* Include package's public header */
#include "H5Gpublic.h"

/* Private headers needed by this file */
#include "H5private.h"		/* Generic Functions			*/
#include "H5Bprivate.h"		/* B-trees				*/
#include "H5Fprivate.h"		/* File access				*/
#include "H5RSprivate.h"        /* Reference-counted strings            */

/*
 * Define this to enable debugging.
 */
#ifdef NDEBUG
#  undef H5G_DEBUG
#endif

/*
 * The disk size for a symbol table entry...
 */
#define H5G_SIZEOF_SCRATCH      16
#define H5G_SIZEOF_ENTRY(F)                                                   \
   (H5F_SIZEOF_SIZE(F) +        /*offset of name into heap              */    \
    H5F_SIZEOF_ADDR(F) +        /*address of object header              */    \
    4 +                         /*entry type                            */    \
    4 +				/*reserved				*/    \
    H5G_SIZEOF_SCRATCH)         /*scratch pad space                     */

/* ========= Group Creation properties ============ */

/* Defaults for link info values */
#define H5G_CRT_LINFO_TRACK_CORDER              FALSE
#define H5G_CRT_LINFO_INDEX_CORDER              FALSE
#define H5G_CRT_LINFO_NLINKS                    0
#define H5G_CRT_LINFO_MAX_CORDER                0
#define H5G_CRT_LINFO_LINK_FHEAP_ADDR           HADDR_UNDEF
#define H5G_CRT_LINFO_NAME_BT2_ADDR             HADDR_UNDEF
#define H5G_CRT_LINFO_CORDER_BT2_ADDR           HADDR_UNDEF

/* Definitions for link info settings */
#define H5G_CRT_LINK_INFO_NAME                  "link info"
#define H5G_CRT_LINK_INFO_SIZE                  sizeof(H5O_linfo_t)
#define H5G_CRT_LINK_INFO_DEF                   {H5G_CRT_LINFO_TRACK_CORDER, \
                                                    H5G_CRT_LINFO_INDEX_CORDER, \
                                                    H5G_CRT_LINFO_MAX_CORDER, \
                                                    H5G_CRT_LINFO_CORDER_BT2_ADDR, \
                                                    H5G_CRT_LINFO_NLINKS, \
                                                    H5G_CRT_LINFO_LINK_FHEAP_ADDR, \
                                                    H5G_CRT_LINFO_NAME_BT2_ADDR \
                                                }

/* Defaults for group info values */
#define H5G_CRT_GINFO_LHEAP_SIZE_HINT           0
#define H5G_CRT_GINFO_STORE_LINK_PHASE_CHANGE   FALSE
#define H5G_CRT_GINFO_MAX_COMPACT               8
#define H5G_CRT_GINFO_MIN_DENSE                 6
#define H5G_CRT_GINFO_STORE_EST_ENTRY_INFO      FALSE
#define H5G_CRT_GINFO_EST_NUM_ENTRIES           4
#define H5G_CRT_GINFO_EST_NAME_LEN              8

/* Definitions for group info settings */
#define H5G_CRT_GROUP_INFO_NAME                 "group info"
#define H5G_CRT_GROUP_INFO_SIZE                 sizeof(H5O_ginfo_t)
#define H5G_CRT_GROUP_INFO_DEF                  {H5G_CRT_GINFO_LHEAP_SIZE_HINT, \
                                                    H5G_CRT_GINFO_STORE_LINK_PHASE_CHANGE, \
                                                    H5G_CRT_GINFO_MAX_COMPACT, \
                                                    H5G_CRT_GINFO_MIN_DENSE, \
                                                    H5G_CRT_GINFO_STORE_EST_ENTRY_INFO, \
                                                    H5G_CRT_GINFO_EST_NUM_ENTRIES, \
                                                    H5G_CRT_GINFO_EST_NAME_LEN \
                                                }

/* If the module using this macro is allowed access to the private variables, access them directly */
#ifdef H5G_PACKAGE
#define H5G_MOUNTED(G)              ((G)->shared->mounted)
#else /* H5G_PACKAGE */
#define H5G_MOUNTED(G)              (H5G_mounted(G))
#endif /* H5G_PACKAGE */

/* Type of operation being performed for call to H5G_name_replace() */
typedef enum {
    H5G_NAME_MOVE = 0,          /* H5*move call    */
    H5G_NAME_DELETE,            /* H5Ldelete call  */
    H5G_NAME_MOUNT,             /* H5Fmount call   */
    H5G_NAME_UNMOUNT            /* H5Funmount call */
} H5G_names_op_t;

/* Status returned from traversal callbacks; whether the object location
 * or group location need to be closed */
#define H5G_OWN_NONE 0
#define H5G_OWN_OBJ_LOC 1
#define H5G_OWN_GRP_LOC 2
#define H5G_OWN_BOTH 3
typedef int H5G_own_loc_t;

/* Structure to store information about the name an object was opened with */
typedef struct {
    H5RS_str_t  *full_path_r;           /* Path to object, as seen from root of current file mounting hierarchy */
    H5RS_str_t  *user_path_r;           /* Path to object, as opened by user */
    unsigned    obj_hidden;             /* Whether the object is visible in group hier. */
} H5G_name_t;

/* Forward declarations (for prototypes & struct definitions) */
struct H5O_loc_t;
struct H5O_link_t;

/*
 * The "location" of an object in a group hierarchy.  This points to an object
 * location and a group hierarchy path for the object.
 */
typedef struct {
    struct H5O_loc_t *oloc;             /* Object header location            */
    H5G_name_t *path;                   /* Group hierarchy path              */
} H5G_loc_t;

/* Callback information for copying groups */
typedef struct H5G_copy_file_ud_t {
    H5O_copy_file_ud_common_t common;   /* Shared information (must be first) */
} H5G_copy_file_ud_t;

typedef struct H5G_t H5G_t;
typedef struct H5G_shared_t H5G_shared_t;
typedef struct H5G_entry_t H5G_entry_t;

/*
 * Library prototypes...  These are the ones that other packages routinely
 * call.
 */
H5_DLL herr_t H5G_mkroot(H5F_t *f, hid_t dxpl_id, hbool_t create_root);
H5_DLL struct H5O_loc_t *H5G_oloc(H5G_t *grp);
H5_DLL H5G_t *H5G_rootof(H5F_t *f);
H5_DLL H5G_name_t * H5G_nameof(H5G_t *grp);
H5_DLL H5F_t *H5G_fileof(H5G_t *grp);
H5_DLL herr_t H5G_free(H5G_t *grp);
H5_DLL H5G_t *H5G_open(const H5G_loc_t *loc, hid_t dxpl_id);
H5_DLL herr_t H5G_close(H5G_t *grp);
H5_DLL herr_t H5G_free_grp_name(H5G_t *grp);
H5_DLL herr_t H5G_get_shared_count(H5G_t *grp);
H5_DLL herr_t H5G_mount(H5G_t *grp);
H5_DLL hbool_t H5G_mounted(H5G_t *grp);
H5_DLL herr_t H5G_unmount(H5G_t *grp);
#ifndef H5_NO_DEPRECATED_SYMBOLS
H5_DLL H5G_obj_t H5G_map_obj_type(H5O_type_t obj_type);
#endif /* H5_NO_DEPRECATED_SYMBOLS */
H5_DLL herr_t H5G_visit(hid_t loc_id, const char *group_name,
    H5_index_t idx_type, H5_iter_order_t order, H5L_iterate_t op, void *op_data,
    hid_t lapl_id, hid_t dxpl_id);

/*
 * These functions operate on symbol table nodes.
 */
H5_DLL herr_t H5G_node_close(const H5F_t *f);
H5_DLL herr_t H5G_node_debug(H5F_t *f, hid_t dxpl_id, haddr_t addr, FILE *stream,
			      int indent, int fwidth, haddr_t heap);

/*
 * These functions operate on group object locations.
 */
H5_DLL herr_t H5G_ent_encode(const H5F_t *f, uint8_t **pp, const H5G_entry_t *ent);
H5_DLL herr_t H5G_ent_decode(const H5F_t *f, const uint8_t **pp, H5G_entry_t *ent);

/*
 * These functions operate on group hierarchy names.
 */
H5_DLL herr_t H5G_name_replace(const struct H5O_link_t *lnk, H5G_names_op_t op,
    H5F_t *src_file, H5RS_str_t *src_full_path_r, H5F_t *dst_file,
    H5RS_str_t *dst_full_path_r, hid_t dxpl_id);
H5_DLL herr_t H5G_name_reset(H5G_name_t *name);
H5_DLL herr_t H5G_name_copy(H5G_name_t *dst, const H5G_name_t *src, H5_copy_depth_t depth);
H5_DLL herr_t H5G_name_free(H5G_name_t *name);
H5_DLL ssize_t H5G_get_name(hid_t id, char *name/*out*/, size_t size,
    hid_t lapl_id, hid_t dxpl_id);
H5_DLL ssize_t H5G_get_name_by_addr(hid_t fid, hid_t lapl_id, hid_t dxpl_id,
    const struct H5O_loc_t *loc, char* name, size_t size);

/*
 * These functions operate on group "locations"
 */
H5_DLL herr_t H5G_loc(hid_t loc_id, H5G_loc_t *loc);
H5_DLL herr_t H5G_loc_find(const H5G_loc_t *loc, const char *name,
    H5G_loc_t *obj_loc/*out*/, hid_t lapl_id, hid_t dxpl_id);
H5_DLL herr_t H5G_loc_find_by_idx(H5G_loc_t *loc, const char *group_name,
    H5_index_t idx_type, H5_iter_order_t order, hsize_t n,
    H5G_loc_t *obj_loc/*out*/, hid_t lapl_id, hid_t dxpl_id);
H5_DLL htri_t H5G_loc_exists(const H5G_loc_t *loc, const char *name,
    hid_t lapl_id, hid_t dxpl_id);
H5_DLL herr_t H5G_loc_info(H5G_loc_t *loc, const char *name,
    hbool_t want_ih_info, H5O_info_t *oinfo/*out*/, hid_t lapl_id,
    hid_t dxpl_id);
H5_DLL herr_t H5G_loc_set_comment(H5G_loc_t *loc, const char *name,
    const char *comment, hid_t lapl_id, hid_t dxpl_id);
H5_DLL ssize_t H5G_loc_get_comment(H5G_loc_t *loc, const char *name,
    char *comment/*out*/, size_t bufsize, hid_t lapl_id, hid_t dxpl_id);
H5_DLL herr_t H5G_loc_reset(H5G_loc_t *loc);
H5_DLL herr_t H5G_loc_free(H5G_loc_t *loc);

#endif /* _H5Gprivate_H */

