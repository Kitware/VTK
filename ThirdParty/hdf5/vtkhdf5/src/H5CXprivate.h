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
 *  Header file for API contexts, etc.
 */
#ifndef H5CXprivate_H
#define H5CXprivate_H

/* Private headers needed by this file */
#include "H5private.h"   /* Generic Functions                    */
#include "H5ACprivate.h" /* Metadata cache                       */
#ifdef H5_HAVE_PARALLEL
#include "H5FDprivate.h" /* File drivers                         */
#endif                   /* H5_HAVE_PARALLEL */
#include "H5Pprivate.h"  /* Property lists                       */
#include "H5Tprivate.h"  /* Datatypes                            */
#include "H5Tconv.h"     /* Datatype conversions                 */
#include "H5Zprivate.h"  /* Data filters                         */

/**************************/
/* Library Private Macros */
/**************************/

/****************************/
/* Library Private Typedefs */
/****************************/

/* API context state */
typedef struct H5CX_state_t {
    hid_t                 dcpl_id;            /* DCPL for operation */
    hid_t                 dxpl_id;            /* DXPL for operation */
    hid_t                 lapl_id;            /* LAPL for operation */
    hid_t                 lcpl_id;            /* LCPL for operation */
    void                 *vol_wrap_ctx;       /* VOL connector's "wrap context" for creating IDs */
    H5VL_connector_prop_t vol_connector_prop; /* VOL connector property */

#ifdef H5_HAVE_PARALLEL
    /* Internal: Parallel I/O settings */
    bool coll_metadata_read; /* Whether to use collective I/O for metadata read */
#endif                       /* H5_HAVE_PARALLEL */
} H5CX_state_t;

/* Typedef for context about each API call, as it proceeds */
/* Fields in this struct are of several types:
 * - The DXPL & LAPL ID are either library default ones (from the API context
 *      initialization) or passed in from the application via an API call
 *      parameter.  The corresponding H5P_genplist_t* is just the underlying
 *      property list struct for the ID, to optimize retrieving properties
 *      from the list multiple times.
 *
 * - Internal fields, used and set only within the library, for managing the
 *      operation under way.  These do not correspond to properties in the
 *      DXPL or LAPL and can have any name.
 *
 * - Cached fields, which are not returned to the application, for managing
 *      the operation under way.  These correspond to properties in the DXPL
 *      or LAPL, and are retrieved either from the (global) cache for a
 *      default property list, or from the corresponding property in the
 *      application's (non-default) property list.  Getting / setting these
 *      properties within the library does _not_ affect the application's
 *      property list.  Note that the naming of these fields, <foo> and
 *      <foo>_valid, is important for the H5CX_RETRIEVE_PROP_VALID
 *      macro to work properly.
 *
 * - "Return-only" properties that are returned to the application, mainly
 *      for sending out "introspection" information ("Why did collective I/O
 *      get broken for this operation?", "Which filters are set on the chunk I
 *      just directly read in?", etc) Setting these fields will cause the
 *      corresponding property in the property list to be set when the API
 *      context is popped, when returning from the API routine.  Note that the
 *      naming of these fields, <foo> and <foo>_set, is important for the
 *      H5CX_TEST_SET_PROP and H5CX_SET_PROP macros to work properly.
 *
 * - "Return-and-read" properties that are returned to the application to send out introspection information,
 *      but are also queried by the library internally. If the context value has been 'set' by an accessor,
 *      all future queries will return the stored value from the context, to avoid later queries overwriting
 *      that stored value with the value from the property list.
 *
 *      These properties have both a 'valid' and 'set' flag. <foo>_valid is true if the field has ever been
 *      populated from its underlying property list. <foo>_set flag is true if this field has ever been set on
 *      the context for application introspection. The naming of these fields is important for the
 *      H5CX_RETRIEVE_PROP_VALID_SET macro to work properly.
 *
 *      If a field has been set on the context but never read internally, <foo>_valid will be false
 *      despite the context containing a meaningful cached value.
 */
typedef struct H5CX_t {
    /* DXPL */
    hid_t           dxpl_id; /* DXPL ID for API operation */
    H5P_genplist_t *dxpl;    /* Dataset Transfer Property List */

    /* LCPL */
    hid_t           lcpl_id; /* LCPL ID for API operation */
    H5P_genplist_t *lcpl;    /* Link Creation Property List */

    /* LAPL */
    hid_t           lapl_id; /* LAPL ID for API operation */
    H5P_genplist_t *lapl;    /* Link Access Property List */

    /* DCPL */
    hid_t           dcpl_id; /* DCPL ID for API operation */
    H5P_genplist_t *dcpl;    /* Dataset Creation Property List */

    /* DAPL */
    hid_t           dapl_id; /* DAPL ID for API operation */
    H5P_genplist_t *dapl;    /* Dataset Access Property List */

    /* FAPL */
    hid_t           fapl_id; /* FAPL ID for API operation */
    H5P_genplist_t *fapl;    /* File Access Property List */

    /* Internal: Object tagging info */
    haddr_t tag; /* Current object's tag (ohdr chunk #0 address) */

    /* Internal: Metadata cache info */
    H5AC_ring_t ring; /* Current metadata cache ring for entries */

#ifdef H5_HAVE_PARALLEL
    /* Internal: Parallel I/O settings */
    bool         coll_metadata_read; /* Whether to use collective I/O for metadata read */
    MPI_Datatype btype;              /* MPI datatype for buffer, when using collective I/O */
    MPI_Datatype ftype;              /* MPI datatype for file, when using collective I/O */
    bool         mpi_file_flushing;  /* Whether an MPI-opened file is being flushed */
    bool         rank0_bcast;        /* Whether a dataset meets read-with-rank0-and-bcast requirements */
#endif                               /* H5_HAVE_PARALLEL */

    /* Cached DXPL properties */
    size_t    max_temp_buf;            /* Maximum temporary buffer size (H5D_XFER_MAX_TEMP_BUF_NAME) .*/
    bool      max_temp_buf_valid;      /* Whether maximum temporary buffer size is valid */
    void     *tconv_buf;               /* Temporary conversion buffer (H5D_XFER_TCONV_BUF_NAME) */
    bool      tconv_buf_valid;         /* Whether temporary conversion buffer is valid */
    void     *bkgr_buf;                /* Background conversion buffer (H5D_XFER_BKGR_BUF_NAME) */
    bool      bkgr_buf_valid;          /* Whether background conversion buffer is valid */
    H5T_bkg_t bkgr_buf_type;           /* Background buffer type (H5D_XFER_BKGR_BUF_TYPE_NAME) */
    bool      bkgr_buf_type_valid;     /* Whether background buffer type is valid */
    double    btree_split_ratio[3];    /* B-tree split ratios (H5D_XFER_BTREE_SPLIT_RATIO_NAME) */
    bool      btree_split_ratio_valid; /* Whether B-tree split ratios are valid */
    size_t    vec_size;                /* Size of hyperslab vector (H5D_XFER_HYPER_VECTOR_SIZE_NAME) */
    bool      vec_size_valid;          /* Whether hyperslab vector is valid */
#ifdef H5_HAVE_PARALLEL
    H5FD_mpio_xfer_t io_xfer_mode; /* Parallel transfer mode for this request (H5D_XFER_IO_XFER_MODE_NAME) */
    bool             io_xfer_mode_valid;      /* Whether parallel transfer mode is valid */
    H5FD_mpio_collective_opt_t mpio_coll_opt; /* Parallel transfer with independent IO or collective IO with
                                                 this mode (H5D_XFER_MPIO_COLLECTIVE_OPT_NAME) */
    bool mpio_coll_opt_valid;                 /* Whether parallel transfer option is valid */
    H5FD_mpio_chunk_opt_t
             mpio_chunk_opt_mode;        /* Collective chunk option (H5D_XFER_MPIO_CHUNK_OPT_HARD_NAME) */
    bool     mpio_chunk_opt_mode_valid;  /* Whether collective chunk option is valid */
    unsigned mpio_chunk_opt_num;         /* Collective chunk threshold (H5D_XFER_MPIO_CHUNK_OPT_NUM_NAME) */
    bool     mpio_chunk_opt_num_valid;   /* Whether collective chunk threshold is valid */
    unsigned mpio_chunk_opt_ratio;       /* Collective chunk ratio (H5D_XFER_MPIO_CHUNK_OPT_RATIO_NAME) */
    bool     mpio_chunk_opt_ratio_valid; /* Whether collective chunk ratio is valid */
#endif                                   /* H5_HAVE_PARALLEL */
    H5Z_EDC_t               err_detect;  /* Error detection info (H5D_XFER_EDC_NAME) */
    bool                    err_detect_valid;     /* Whether error detection info is valid */
    H5Z_cb_t                filter_cb;            /* Filter callback function (H5D_XFER_FILTER_CB_NAME) */
    bool                    filter_cb_valid;      /* Whether filter callback function is valid */
    H5Z_data_xform_t       *data_transform;       /* Data transform info (H5D_XFER_XFORM_NAME) */
    bool                    data_transform_valid; /* Whether data transform info is valid */
    H5T_vlen_alloc_info_t   vl_alloc_info;        /* VL datatype alloc info (H5D_XFER_VLEN_*_NAME) */
    bool                    vl_alloc_info_valid;  /* Whether VL datatype alloc info is valid */
    H5T_conv_cb_t           dt_conv_cb;           /* Datatype conversion struct (H5D_XFER_CONV_CB_NAME) */
    bool                    dt_conv_cb_valid;     /* Whether datatype conversion struct is valid */
    H5D_selection_io_mode_t selection_io_mode;    /* Selection I/O mode (H5D_XFER_SELECTION_IO_MODE_NAME) */
    bool                    selection_io_mode_valid; /* Whether selection I/O mode is valid */
    bool modify_write_buf; /* Whether the library can modify write buffers (H5D_XFER_MODIFY_WRITE_BUF_NAME)*/
    bool modify_write_buf_valid; /* Whether the modify_write_buf field is valid */

    /* Return-only DXPL properties to return to application */
#ifdef H5_HAVE_PARALLEL
    H5D_mpio_actual_chunk_opt_mode_t mpio_actual_chunk_opt; /* Chunk optimization mode used for parallel I/O
                                                               (H5D_MPIO_ACTUAL_CHUNK_OPT_MODE_NAME) */
    bool mpio_actual_chunk_opt_set; /* Whether chunk optimization mode used for parallel I/O is set */
    H5D_mpio_actual_io_mode_t
             mpio_actual_io_mode; /* Actual I/O mode used for parallel I/O (H5D_MPIO_ACTUAL_IO_MODE_NAME) */
    bool     mpio_actual_io_mode_set;        /* Whether actual I/O mode used for parallel I/O is set */
    uint32_t mpio_local_no_coll_cause;       /* Local reason for breaking collective I/O
                                                (H5D_MPIO_LOCAL_NO_COLLECTIVE_CAUSE_NAME) */
    bool     mpio_local_no_coll_cause_set;   /* Whether local reason for breaking collective I/O is set */
    bool     mpio_local_no_coll_cause_valid; /* Whether local reason for breaking collective I/O is valid */
    uint32_t mpio_global_no_coll_cause;      /* Global reason for breaking collective I/O
                                                (H5D_MPIO_GLOBAL_NO_COLLECTIVE_CAUSE_NAME) */
    bool mpio_global_no_coll_cause_set;      /* Whether global reason for breaking collective I/O is set */
    bool mpio_global_no_coll_cause_valid;    /* Whether global reason for breaking collective I/O is valid */
#ifdef H5_HAVE_INSTRUMENTED_LIBRARY
    int mpio_coll_chunk_link_hard;       /* Instrumented "collective chunk link hard" value
                                            (H5D_XFER_COLL_CHUNK_LINK_HARD_NAME) */
    bool mpio_coll_chunk_link_hard_set;  /* Whether instrumented "collective chunk link hard" value is set */
    int  mpio_coll_chunk_multi_hard;     /* Instrumented "collective chunk multi hard" value
                                            (H5D_XFER_COLL_CHUNK_MULTI_HARD_NAME) */
    bool mpio_coll_chunk_multi_hard_set; /* Whether instrumented "collective chunk multi hard" value is set */
    int  mpio_coll_chunk_link_num_true;  /* Instrumented "collective chunk link num true" value
                                            (H5D_XFER_COLL_CHUNK_LINK_NUM_TRUE_NAME) */
    bool mpio_coll_chunk_link_num_true_set;    /* Whether instrumented "collective chunk link num true" value
                                                     is set */
    int mpio_coll_chunk_link_num_false;        /* Instrumented "collective chunk link num false" value
                                                  (H5D_XFER_COLL_CHUNK_LINK_NUM_FALSE_NAME) */
    bool mpio_coll_chunk_link_num_false_set;   /* Whether instrumented "collective chunk link num false"
                                                     value is set */
    int mpio_coll_chunk_multi_ratio_coll;      /* Instrumented "collective chunk multi ratio coll" value
                                                  (H5D_XFER_COLL_CHUNK_MULTI_RATIO_COLL_NAME) */
    bool mpio_coll_chunk_multi_ratio_coll_set; /* Whether instrumented "collective chunk multi ratio coll"
                                                     value is set */
    int mpio_coll_chunk_multi_ratio_ind;       /* Instrumented "collective chunk multi ratio ind" value
                                                  (H5D_XFER_COLL_CHUNK_MULTI_RATIO_IND_NAME) */
    bool mpio_coll_chunk_multi_ratio_ind_set;  /* Whether instrumented "collective chunk multi ratio ind"
                                                     value is set */
    bool mpio_coll_rank0_bcast;                /* Instrumented "collective rank 0 broadcast" value
                                                     (H5D_XFER_COLL_RANK0_BCAST_NAME) */
    bool mpio_coll_rank0_bcast_set;   /* Whether instrumented "collective rank 0 broadcast" value is set */
#endif                                /* H5_HAVE_INSTRUMENTED_LIBRARY */
#endif                                /* H5_HAVE_PARALLEL */
    uint32_t no_selection_io_cause;   /* Reason for not performing selection I/O
                                            (H5D_XFER_NO_SELECTION_IO_CAUSE_NAME) */
    bool no_selection_io_cause_set;   /* Whether reason for not performing selection I/O is set */
    bool no_selection_io_cause_valid; /* Whether reason for not performing selection I/O is valid */

    uint32_t actual_selection_io_mode;   /* Actual selection I/O mode used
                                            (H5D_XFER_ACTUAL_SELECTION_IO_MODE_NAME) */
    bool actual_selection_io_mode_set;   /* Whether actual selection I/O mode is set */
    bool actual_selection_io_mode_valid; /* Whether actual selection I/O mode is valid */

    /* Cached LCPL properties */
    H5T_cset_t encoding;         /* Link name character encoding (H5P_STRCRT_CHAR_ENCODING_NAME) */
    bool       encoding_valid;   /* Whether link name character encoding is valid */
    unsigned intermediate_group; /* Whether to create intermediate groups (H5L_CRT_INTERMEDIATE_GROUP_NAME) */
    bool     intermediate_group_valid; /* Whether create intermediate group flag is valid */

    /* Cached LAPL properties */
    size_t nlinks;       /* Number of soft / UD links to traverse (H5L_ACS_NLINKS_NAME) */
    bool   nlinks_valid; /* Whether number of soft / UD links to traverse is valid */

    /* Cached DCPL properties */
    bool    do_min_dset_ohdr; /* Whether to minimize dataset object header (H5D_CRT_MIN_DSET_HDR_SIZE_NAME) */
    bool    do_min_dset_ohdr_valid; /* Whether minimize dataset object header flag is valid */
    uint8_t ohdr_flags;             /* Object header flags (H5O_CRT_OHDR_FLAGS_NAME) */
    bool    ohdr_flags_valid;       /* Whether the object headers flags are valid */

    /* Cached DAPL properties */
    const char *extfile_prefix;       /* Prefix for external file (H5D_ACS_EFILE_PREFIX_NAME) */
    bool        extfile_prefix_valid; /* Whether the prefix for external file is valid */
    const char *vds_prefix;           /* Prefix for VDS (H5D_ACS_VDS_PREFIX_NAME) */
    bool        vds_prefix_valid;     /* Whether the prefix for VDS is valid           */

    /* Cached FAPL properties */
    H5F_libver_t low_bound;       /* low_bound property for H5Pset_libver_bounds()
                                     (H5F_ACS_LIBVER_LOW_BOUND_NAME) */
    bool         low_bound_valid; /* Whether low_bound property is valid */
    H5F_libver_t high_bound;      /* high_bound property for H5Pset_libver_bounds
                                     (H5F_ACS_LIBVER_HIGH_BOUND_NAME) */
    bool high_bound_valid;        /* Whether high_bound property is valid */

    /* Cached VOL settings */
    H5VL_connector_prop_t vol_connector_prop; /* Property for VOL connector ID & info
                               This is treated as an independent field with
                               no relation to the property H5F_ACS_VOL_CONN_NAME stored on the FAPL */
    bool  vol_connector_prop_valid;           /* Whether property for VOL connector ID & info is valid */
    void *vol_wrap_ctx;                       /* VOL connector's "wrap context" for creating IDs */
    bool  vol_wrap_ctx_valid; /* Whether VOL connector's "wrap context" for creating IDs is valid */
} H5CX_t;

/* Typedef for nodes on the API context stack */
/* Each entry into the library through an API routine invokes H5CX_push()
 * in a FUNC_ENTER_API* macro, which pushes an H5CX_node_t on the API
 * context [thread-local] stack, after initializing it with default values.
 */
typedef struct H5CX_node_t {
    H5CX_t              ctx;  /* Context for current API call */
    struct H5CX_node_t *next; /* Pointer to previous context, on stack */
} H5CX_node_t;

/*****************************/
/* Library-private Variables */
/*****************************/

/***************************************/
/* Library-private Function Prototypes */
/***************************************/

/* Library private routines */
H5_DLL herr_t H5CX_push(H5CX_node_t *cnode);
H5_DLL herr_t H5CX_pop(bool update_dxpl_props);
H5_DLL bool   H5CX_pushed(void);
H5_DLL bool   H5CX_is_def_dxpl(void);

/* API context state routines */
H5_DLL herr_t H5CX_retrieve_state(H5CX_state_t **api_state);
H5_DLL herr_t H5CX_restore_state(const H5CX_state_t *api_state);
H5_DLL herr_t H5CX_free_state(H5CX_state_t *api_state);

/* "Setter" routines for API context info */
H5_DLL void   H5CX_set_dxpl(hid_t dxpl_id);
H5_DLL void   H5CX_set_lcpl(hid_t lcpl_id);
H5_DLL void   H5CX_set_lapl(hid_t lapl_id);
H5_DLL void   H5CX_set_dcpl(hid_t dcpl_id);
H5_DLL herr_t H5CX_set_libver_bounds(H5F_t *f);
H5_DLL herr_t H5CX_set_apl(hid_t *acspl_id, const H5P_libclass_t *libclass, hid_t loc_id, bool is_collective);
H5_DLL herr_t H5CX_set_loc(hid_t loc_id);
H5_DLL herr_t H5CX_set_vol_wrap_ctx(void *wrap_ctx);
H5_DLL herr_t H5CX_set_vol_connector_prop(const H5VL_connector_prop_t *vol_connector_prop);

/* "Getter" routines for API context info */
H5_DLL hid_t       H5CX_get_dxpl(void);
H5_DLL hid_t       H5CX_get_lapl(void);
H5_DLL herr_t      H5CX_get_vol_wrap_ctx(void **wrap_ctx);
H5_DLL herr_t      H5CX_get_vol_connector_prop(H5VL_connector_prop_t *vol_connector_prop);
H5_DLL haddr_t     H5CX_get_tag(void);
H5_DLL H5AC_ring_t H5CX_get_ring(void);
#ifdef H5_HAVE_PARALLEL
H5_DLL bool   H5CX_get_coll_metadata_read(void);
H5_DLL herr_t H5CX_get_mpi_coll_datatypes(MPI_Datatype *btype, MPI_Datatype *ftype);
H5_DLL bool   H5CX_get_mpi_file_flushing(void);
H5_DLL bool   H5CX_get_mpio_rank0_bcast(void);
#endif /* H5_HAVE_PARALLEL */

/* "Getter" routines for DXPL properties cached in API context */
H5_DLL herr_t H5CX_get_btree_split_ratios(double split_ratio[3]);
H5_DLL herr_t H5CX_get_max_temp_buf(size_t *max_temp_buf);
H5_DLL herr_t H5CX_get_tconv_buf(void **tconv_buf);
H5_DLL herr_t H5CX_get_bkgr_buf(void **bkgr_buf);
H5_DLL herr_t H5CX_get_bkgr_buf_type(H5T_bkg_t *bkgr_buf_type);
H5_DLL herr_t H5CX_get_vec_size(size_t *vec_size);
#ifdef H5_HAVE_PARALLEL
H5_DLL herr_t H5CX_get_io_xfer_mode(H5FD_mpio_xfer_t *io_xfer_mode);
H5_DLL herr_t H5CX_get_mpio_coll_opt(H5FD_mpio_collective_opt_t *mpio_coll_opt);
H5_DLL herr_t H5CX_get_mpio_local_no_coll_cause(uint32_t *mpio_local_no_coll_cause);
H5_DLL herr_t H5CX_get_mpio_global_no_coll_cause(uint32_t *mpio_global_no_coll_cause);
H5_DLL herr_t H5CX_get_mpio_chunk_opt_mode(H5FD_mpio_chunk_opt_t *mpio_chunk_opt_mode);
H5_DLL herr_t H5CX_get_mpio_chunk_opt_num(unsigned *mpio_chunk_opt_num);
H5_DLL herr_t H5CX_get_mpio_chunk_opt_ratio(unsigned *mpio_chunk_opt_ratio);
#endif /* H5_HAVE_PARALLEL */
H5_DLL herr_t H5CX_get_err_detect(H5Z_EDC_t *err_detect);
H5_DLL herr_t H5CX_get_filter_cb(H5Z_cb_t *filter_cb);
H5_DLL herr_t H5CX_get_data_transform(H5Z_data_xform_t **data_transform);
H5_DLL herr_t H5CX_get_vlen_alloc_info(H5T_vlen_alloc_info_t *vl_alloc_info);
H5_DLL herr_t H5CX_get_dt_conv_cb(H5T_conv_cb_t *cb_struct);
H5_DLL herr_t H5CX_get_selection_io_mode(H5D_selection_io_mode_t *selection_io_mode);
H5_DLL herr_t H5CX_get_no_selection_io_cause(uint32_t *no_selection_io_cause);
H5_DLL herr_t H5CX_get_actual_selection_io_mode(uint32_t *actual_selection_io_mode);
H5_DLL herr_t H5CX_get_modify_write_buf(bool *modify_write_buf);

/* "Getter" routines for LCPL properties cached in API context */
H5_DLL herr_t H5CX_get_encoding(H5T_cset_t *encoding);
H5_DLL herr_t H5CX_get_intermediate_group(unsigned *crt_intermed_group);

/* "Getter" routines for LAPL properties cached in API context */
H5_DLL herr_t H5CX_get_nlinks(size_t *nlinks);

/* "Getter" routines for DCPL properties cached in API context */
H5_DLL herr_t H5CX_get_dset_min_ohdr_flag(bool *dset_min_ohdr_flag);
H5_DLL herr_t H5CX_get_ohdr_flags(uint8_t *ohdr_flags);

/* "Getter" routines for DAPL properties cached in API context */
H5_DLL herr_t H5CX_get_ext_file_prefix(const char **prefix_extfile);
H5_DLL herr_t H5CX_get_vds_prefix(const char **prefix_vds);

/* "Getter" routines for FAPL properties cached in API context */
H5_DLL herr_t H5CX_get_libver_bounds(H5F_libver_t *low_bound, H5F_libver_t *high_bound);

/* "Setter" routines for API context info */
H5_DLL void H5CX_set_tag(haddr_t tag);
H5_DLL void H5CX_set_ring(H5AC_ring_t ring);
#ifdef H5_HAVE_PARALLEL
H5_DLL void   H5CX_set_coll_metadata_read(bool cmdr);
H5_DLL herr_t H5CX_set_mpi_coll_datatypes(MPI_Datatype btype, MPI_Datatype ftype);
H5_DLL herr_t H5CX_set_mpio_coll_opt(H5FD_mpio_collective_opt_t mpio_coll_opt);
H5_DLL void   H5CX_set_mpi_file_flushing(bool flushing);
H5_DLL void   H5CX_set_mpio_rank0_bcast(bool rank0_bcast);
#endif /* H5_HAVE_PARALLEL */

/* "Setter" routines for DXPL properties cached in API context */
#ifdef H5_HAVE_PARALLEL
H5_DLL herr_t H5CX_set_io_xfer_mode(H5FD_mpio_xfer_t io_xfer_mode);
#endif /* H5_HAVE_PARALLEL */
H5_DLL herr_t H5CX_set_vlen_alloc_info(H5MM_allocate_t alloc_func, void *alloc_info, H5MM_free_t free_func,
                                       void *free_info);

/* "Setter" routines for LAPL properties cached in API context */
H5_DLL herr_t H5CX_set_nlinks(size_t nlinks);

/* "Setter" routines for cached DXPL properties that must be returned to application */

H5_DLL void H5CX_set_no_selection_io_cause(uint32_t no_selection_io_cause);
H5_DLL void H5CX_set_actual_selection_io_mode(uint32_t actual_selection_io_mode);

#ifdef H5_HAVE_PARALLEL
H5_DLL void H5CX_set_mpio_actual_chunk_opt(H5D_mpio_actual_chunk_opt_mode_t chunk_opt);
H5_DLL void H5CX_set_mpio_actual_io_mode(H5D_mpio_actual_io_mode_t actual_io_mode);
H5_DLL void H5CX_set_mpio_local_no_coll_cause(uint32_t mpio_local_no_coll_cause);
H5_DLL void H5CX_set_mpio_global_no_coll_cause(uint32_t mpio_global_no_coll_cause);
#ifdef H5_HAVE_INSTRUMENTED_LIBRARY
H5_DLL herr_t H5CX_test_set_mpio_coll_chunk_link_hard(int mpio_coll_chunk_link_hard);
H5_DLL herr_t H5CX_test_set_mpio_coll_chunk_multi_hard(int mpio_coll_chunk_multi_hard);
H5_DLL herr_t H5CX_test_set_mpio_coll_chunk_link_num_true(int mpio_coll_chunk_link_num_true);
H5_DLL herr_t H5CX_test_set_mpio_coll_chunk_link_num_false(int mpio_coll_chunk_link_num_false);
H5_DLL herr_t H5CX_test_set_mpio_coll_chunk_multi_ratio_coll(int mpio_coll_chunk_multi_ratio_coll);
H5_DLL herr_t H5CX_test_set_mpio_coll_chunk_multi_ratio_ind(int mpio_coll_chunk_multi_ratio_ind);
H5_DLL herr_t H5CX_test_set_mpio_coll_rank0_bcast(bool rank0_bcast);
#endif /* H5_HAVE_INSTRUMENTED_LIBRARY */
#endif /* H5_HAVE_PARALLEL */

#endif /* H5CXprivate_H */
