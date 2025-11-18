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

/*
 * This file contains private information about the H5T module
 */
#ifndef H5Tprivate_H
#define H5Tprivate_H

/* Early typedefs to avoid circular dependencies */
typedef struct H5T_t H5T_t;

/* Include package's public headers */
#include "H5Tpublic.h"
#include "H5Tdevelop.h"

/* Other public headers needed by this file */
#include "H5MMpublic.h" /* Memory management                        */

/* Private headers needed by this file */
#include "H5private.h"   /* Generic Functions                        */
#include "H5Gprivate.h"  /* Groups                                   */
#include "H5Rprivate.h"  /* References                               */
#include "H5Tconv.h"     /* Datatype conversions                     */
#include "H5VLprivate.h" /* VOL Drivers                              */

/* Macro for size of temporary buffers to contain a single element */
#define H5T_ELEM_BUF_SIZE 256

/* If the module using this macro is allowed access to the private variables, access them directly */
#ifdef H5T_MODULE
#define H5T_GET_SIZE(T)             ((T)->shared->size)
#define H5T_GET_SHARED(T)           ((T)->shared)
#define H5T_GET_MEMBER_OFFSET(T, I) ((T)->u.compnd.memb[I].offset)
#define H5T_GET_MEMBER_SIZE(T, I)   ((T)->u.compnd.memb[I].shared->size)
#define H5T_GET_FORCE_CONV(T)       ((T)->shared->force_conv)
#else /* H5T_MODULE */
#define H5T_GET_SIZE(T)             (H5T_get_size(T))
#define H5T_GET_SHARED(T)           (H5T_get_shared(T))
#define H5T_GET_MEMBER_OFFSET(T, I) (H5T_get_member_offset((T), (I)))
#define H5T_GET_MEMBER_SIZE(T, I)   (H5T_get_member_size((T), (I)))
#define H5T_GET_FORCE_CONV(T)       (H5T_get_force_conv(T))
#endif /* H5T_MODULE */

/* Forward reference of H5S_t */
struct H5S_t;

/* How to copy a datatype */
typedef enum H5T_copy_t { H5T_COPY_TRANSIENT, H5T_COPY_ALL } H5T_copy_t;

/* Location of datatype information */
typedef enum {
    H5T_LOC_BADLOC = 0, /* invalid datatype location */
    H5T_LOC_MEMORY,     /* data stored in memory */
    H5T_LOC_DISK,       /* data stored on disk */
    H5T_LOC_MAXLOC      /* highest value (Invalid as true value) */
} H5T_loc_t;

/* VL allocation information */
typedef struct {
    H5MM_allocate_t alloc_func; /* Allocation function */
    void           *alloc_info; /* Allocation information */
    H5MM_free_t     free_func;  /* Free function */
    void           *free_info;  /* Free information */
} H5T_vlen_alloc_info_t;

/* Forward declarations for prototype arguments */
struct H5G_loc_t;
struct H5G_name_t;
struct H5O_shared_t;

/* The native endianness of the platform */
H5_DLLVAR H5T_order_t H5T_native_order_g;

/* Private functions */
H5_DLL herr_t             H5T_init(void);
H5_DLL H5T_t             *H5T_copy(const H5T_t *old_dt, H5T_copy_t method);
H5_DLL H5T_t             *H5T_copy_reopen(H5T_t *old_dt);
H5_DLL herr_t             H5T_lock(H5T_t *dt, bool immutable);
H5_DLL herr_t             H5T_close(H5T_t *dt);
H5_DLL herr_t             H5T_close_real(H5T_t *dt);
H5_DLL H5T_t             *H5T_get_super(const H5T_t *dt);
H5_DLL H5T_class_t        H5T_get_class(const H5T_t *dt, htri_t internal);
H5_DLL htri_t             H5T_detect_class(const H5T_t *dt, H5T_class_t cls, bool from_api);
H5_DLL size_t             H5T_get_size(const H5T_t *dt);
H5_DLL int                H5T_cmp(const H5T_t *dt1, const H5T_t *dt2, bool superset);
H5_DLL herr_t             H5T_encode(H5T_t *obj, unsigned char *buf, size_t *nalloc);
H5_DLL H5T_t             *H5T_decode(size_t buf_size, const unsigned char *buf);
H5_DLL herr_t             H5T_debug(const H5T_t *dt, FILE *stream);
H5_DLL struct H5O_loc_t  *H5T_oloc(H5T_t *dt);
H5_DLL struct H5G_name_t *H5T_nameof(H5T_t *dt);
H5_DLL htri_t             H5T_is_immutable(const H5T_t *dt);
H5_DLL htri_t             H5T_is_named(const H5T_t *dt);
H5_DLL herr_t             H5T_convert_committed_datatype(H5T_t *dt, H5F_t *f);
H5_DLL htri_t             H5T_is_relocatable(const H5T_t *dt);
H5_DLL herr_t             H5T_unregister(H5T_pers_t pers, const char *name, H5T_t *src, H5T_t *dst,
                                         H5VL_object_t *owned_vol_obj, H5T_conv_t func);
H5_DLL herr_t             H5T_vlen_reclaim_elmt(void *elem, const H5T_t *dt);
H5_DLL htri_t             H5T_set_loc(H5T_t *dt, H5VL_object_t *file, H5T_loc_t loc);
H5_DLL htri_t             H5T_is_sensible(const H5T_t *dt);
H5_DLL herr_t             H5T_set_version(H5F_t *f, H5T_t *dt);
H5_DLL herr_t             H5T_patch_file(H5T_t *dt, H5F_t *f);
H5_DLL herr_t             H5T_patch_vlen_file(H5T_t *dt, H5VL_object_t *file);
H5_DLL herr_t             H5T_own_vol_obj(H5T_t *dt, H5VL_object_t *vol_obj);
H5_DLL htri_t             H5T_is_variable_str(const H5T_t *dt);
H5_DLL H5T_t             *H5T_construct_datatype(H5VL_object_t *dt_obj);
H5_DLL H5VL_object_t     *H5T_get_named_type(const H5T_t *dt);
H5_DLL H5T_t             *H5T_get_actual_type(H5T_t *dt);
H5_DLL herr_t             H5T_save_refresh_state(hid_t tid, struct H5O_shared_t *cached_H5O_shared);
H5_DLL herr_t             H5T_restore_refresh_state(hid_t tid, struct H5O_shared_t *cached_H5O_shared);
H5_DLL bool               H5T_already_vol_managed(const H5T_t *dt);
H5_DLL htri_t             H5T_is_vl_storage(const H5T_t *dt);
H5_DLL herr_t H5T_invoke_vol_optional(H5T_t *dt, H5VL_optional_args_t *args, hid_t dxpl_id, void **req,
                                      H5VL_object_t **vol_obj_ptr);
H5_DLL bool   H5T_is_numeric_with_unusual_unused_bits(const H5T_t *dt);

/* Reference specific functions */
H5_DLL H5R_type_t H5T_get_ref_type(const H5T_t *dt);

/* Operations on named datatypes */
H5_DLL H5T_t *H5T_open(const struct H5G_loc_t *loc);
H5_DLL int    H5T_link(const H5T_t *type, int adjust);
H5_DLL herr_t H5T_update_shared(H5T_t *type);

/* Field functions (for both compound & enumerated types) */
H5_DLL int    H5T_get_nmembers(const H5T_t *dt);
H5_DLL H5T_t *H5T_get_member_type(const H5T_t *dt, unsigned membno);
H5_DLL size_t H5T_get_member_offset(const H5T_t *dt, unsigned membno);

/* Atomic functions */
H5_DLL H5T_order_t H5T_get_order(const H5T_t *dt);
H5_DLL size_t      H5T_get_precision(const H5T_t *dt);
H5_DLL int         H5T_get_offset(const H5T_t *dt);

/* Fixed-point functions */
H5_DLL H5T_sign_t H5T_get_sign(H5T_t const *dt);

#endif /* H5Tprivate_H */
