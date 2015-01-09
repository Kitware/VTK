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
 * Created:		H5Pdxpl.c
 *			March 16 1998
 *			Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:		Data transfer property list class routines
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/
#define H5P_PACKAGE		/*suppress error about including H5Ppkg	  */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Dprivate.h"		/* Datasets				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5Ppkg.h"		/* Property lists		  	*/


/****************/
/* Local Macros */
/****************/

/* ======== Data transfer properties ======== */
/* Definitions for maximum temp buffer size property */
#define H5D_XFER_MAX_TEMP_BUF_SIZE      sizeof(size_t)
#define H5D_XFER_MAX_TEMP_BUF_DEF       H5D_TEMP_BUF_SIZE
/* Definitions for type conversion buffer property */
#define H5D_XFER_TCONV_BUF_SIZE         sizeof(void *)
#define H5D_XFER_TCONV_BUF_DEF          NULL
/* Definitions for background buffer property */
#define H5D_XFER_BKGR_BUF_SIZE          sizeof(void *)
#define H5D_XFER_BKGR_BUF_DEF           NULL
/* Definitions for background buffer type property */
#define H5D_XFER_BKGR_BUF_TYPE_SIZE     sizeof(H5T_bkg_t)
#define H5D_XFER_BKGR_BUF_TYPE_DEF      H5T_BKG_NO
/* Definitions for B-tree node splitting ratio property */
/* (These default B-tree node splitting ratios are also used for splitting
 * group's B-trees as well as chunked dataset's B-trees - QAK)
 */
#define H5D_XFER_BTREE_SPLIT_RATIO_SIZE sizeof(double[3])
#define H5D_XFER_BTREE_SPLIT_RATIO_DEF  {0.1, 0.5, 0.9}
/* Definitions for vlen allocation function property */
#define H5D_XFER_VLEN_ALLOC_SIZE        sizeof(H5MM_allocate_t)
#define H5D_XFER_VLEN_ALLOC_DEF         H5D_VLEN_ALLOC
/* Definitions for vlen allocation info property */
#define H5D_XFER_VLEN_ALLOC_INFO_SIZE   sizeof(void *)
#define H5D_XFER_VLEN_ALLOC_INFO_DEF    H5D_VLEN_ALLOC_INFO
/* Definitions for vlen free function property */
#define H5D_XFER_VLEN_FREE_SIZE         sizeof(H5MM_free_t)
#define H5D_XFER_VLEN_FREE_DEF          H5D_VLEN_FREE
/* Definitions for vlen free info property */
#define H5D_XFER_VLEN_FREE_INFO_SIZE    sizeof(void *)
#define H5D_XFER_VLEN_FREE_INFO_DEF     H5D_VLEN_FREE_INFO
/* Definitions for hyperslab vector size property */
/* (Be cautious about increasing the default size, there are arrays allocated
 *      on the stack which depend on it - QAK)
 */
#define H5D_XFER_HYPER_VECTOR_SIZE_SIZE sizeof(size_t)
#define H5D_XFER_HYPER_VECTOR_SIZE_DEF  H5D_IO_VECTOR_SIZE

#ifdef H5_HAVE_PARALLEL
/* Definitions for I/O transfer mode property */
#define H5D_XFER_IO_XFER_MODE_SIZE      sizeof(H5FD_mpio_xfer_t)
#define H5D_XFER_IO_XFER_MODE_DEF       H5FD_MPIO_INDEPENDENT
/* Definitions for optimization of MPI-IO transfer mode property */
#define H5D_XFER_MPIO_COLLECTIVE_OPT_SIZE       sizeof(H5FD_mpio_collective_opt_t)
#define H5D_XFER_MPIO_COLLECTIVE_OPT_DEF        H5FD_MPIO_COLLECTIVE_IO
#define H5D_XFER_MPIO_CHUNK_OPT_HARD_SIZE       sizeof(H5FD_mpio_chunk_opt_t)
#define H5D_XFER_MPIO_CHUNK_OPT_HARD_DEF        H5FD_MPIO_CHUNK_DEFAULT
#define H5D_XFER_MPIO_CHUNK_OPT_NUM_SIZE        sizeof(unsigned)
#define H5D_XFER_MPIO_CHUNK_OPT_NUM_DEF         H5D_ONE_LINK_CHUNK_IO_THRESHOLD
#define H5D_XFER_MPIO_CHUNK_OPT_RATIO_SIZE      sizeof(unsigned)
#define H5D_XFER_MPIO_CHUNK_OPT_RATIO_DEF       H5D_MULTI_CHUNK_IO_COL_THRESHOLD
/* Definitions for chunk opt mode property. */
#define H5D_MPIO_ACTUAL_CHUNK_OPT_MODE_SIZE     sizeof(H5D_mpio_actual_chunk_opt_mode_t)
#define H5D_MPIO_ACTUAL_CHUNK_OPT_MODE_DEF      H5D_MPIO_NO_CHUNK_OPTIMIZATION
/* Definitions for chunk io mode property. */
#define H5D_MPIO_ACTUAL_IO_MODE_SIZE    sizeof(H5D_mpio_actual_io_mode_t)
#define H5D_MPIO_ACTUAL_IO_MODE_DEF     H5D_MPIO_NO_COLLECTIVE
/* Definitions for cause of broken collective io property */
#define H5D_MPIO_NO_COLLECTIVE_CAUSE_SIZE   sizeof(uint32_t)
#define H5D_MPIO_NO_COLLECTIVE_CAUSE_DEF   H5D_MPIO_COLLECTIVE 
/* Definitions for memory MPI type property */
#define H5FD_MPI_XFER_MEM_MPI_TYPE_SIZE        sizeof(MPI_Datatype)
#define H5FD_MPI_XFER_MEM_MPI_TYPE_DEF         MPI_DATATYPE_NULL
/* Definitions for file MPI type property */
#define H5FD_MPI_XFER_FILE_MPI_TYPE_SIZE       sizeof(MPI_Datatype)
#define H5FD_MPI_XFER_FILE_MPI_TYPE_DEF        MPI_DATATYPE_NULL
#endif /* H5_HAVE_PARALLEL */

/* Definitions for EDC property */
#define H5D_XFER_EDC_SIZE           sizeof(H5Z_EDC_t)
#define H5D_XFER_EDC_DEF            H5Z_ENABLE_EDC
/* Definitions for filter callback function property */
#define H5D_XFER_FILTER_CB_SIZE     sizeof(H5Z_cb_t)
#define H5D_XFER_FILTER_CB_DEF      {NULL,NULL}
/* Definitions for type conversion callback function property */
#define H5D_XFER_CONV_CB_SIZE       sizeof(H5T_conv_cb_t)
#define H5D_XFER_CONV_CB_DEF        {NULL,NULL}
/* Definitions for data transform property */
#define H5D_XFER_XFORM_SIZE         sizeof(void *)
#define H5D_XFER_XFORM_DEF          NULL
#define H5D_XFER_XFORM_DEL          H5P__dxfr_xform_del
#define H5D_XFER_XFORM_COPY         H5P__dxfr_xform_copy
#define H5D_XFER_XFORM_CMP          H5P__dxfr_xform_cmp
#define H5D_XFER_XFORM_CLOSE        H5P__dxfr_xform_close
/* Definitions for properties of direct chunk write */
#define H5D_XFER_DIRECT_CHUNK_WRITE_FLAG_SIZE		sizeof(hbool_t)
#define H5D_XFER_DIRECT_CHUNK_WRITE_FLAG_DEF		FALSE
#define H5D_XFER_DIRECT_CHUNK_WRITE_FILTERS_SIZE	sizeof(uint32_t)
#define H5D_XFER_DIRECT_CHUNK_WRITE_FILTERS_DEF		0
#define H5D_XFER_DIRECT_CHUNK_WRITE_OFFSET_SIZE		sizeof(hsize_t *)
#define H5D_XFER_DIRECT_CHUNK_WRITE_OFFSET_DEF		NULL
#define H5D_XFER_DIRECT_CHUNK_WRITE_DATASIZE_SIZE	sizeof(uint32_t)
#define H5D_XFER_DIRECT_CHUNK_WRITE_DATASIZE_DEF	0

/******************/
/* Local Typedefs */
/******************/


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/

/* Property class callbacks */
static herr_t H5P__dxfr_reg_prop(H5P_genclass_t *pclass);

/* Property list callbacks */
static herr_t H5P__dxfr_xform_del(hid_t prop_id, const char* name, size_t size, void* value);
static herr_t H5P__dxfr_xform_copy(const char* name, size_t size, void* value);
static int H5P__dxfr_xform_cmp(const void *value1, const void *value2, size_t size);
static herr_t H5P__dxfr_xform_close(const char* name, size_t size, void* value);


/*********************/
/* Package Variables */
/*********************/

/* Data transfer property list class library initialization object */
const H5P_libclass_t H5P_CLS_DXFR[1] = {{
    "data transfer",		/* Class name for debugging     */
    H5P_TYPE_DATASET_XFER,      /* Class type                   */
    &H5P_CLS_ROOT_g,		/* Parent class ID              */
    &H5P_CLS_DATASET_XFER_g,	/* Pointer to class ID          */
    &H5P_LST_DATASET_XFER_g,	/* Pointer to default property list ID */
    H5P__dxfr_reg_prop,		/* Default property registration routine */
    NULL,		        /* Class creation callback      */
    NULL,		        /* Class creation callback info */
    NULL,			/* Class copy callback          */
    NULL,		        /* Class copy callback info     */
    NULL,			/* Class close callback         */
    NULL 		        /* Class close callback info    */
}};


/*****************************/
/* Library Private Variables */
/*****************************/


/***************************/
/* Local Private Variables */
/***************************/



/*-------------------------------------------------------------------------
 * Function:    H5P__dxfr_reg_prop
 *
 * Purpose:     Register the data transfer property list class's properties
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              October 31, 2006
 *-------------------------------------------------------------------------
 */
static herr_t
H5P__dxfr_reg_prop(H5P_genclass_t *pclass)
{
    size_t def_max_temp_buf = H5D_XFER_MAX_TEMP_BUF_DEF;        /* Default value for maximum temp buffer size */
    void *def_tconv_buf = H5D_XFER_TCONV_BUF_DEF;               /* Default value for type conversion buffer */
    void *def_bkgr_buf = H5D_XFER_BKGR_BUF_DEF;                 /* Default value for background buffer */
    H5T_bkg_t def_bkgr_buf_type = H5D_XFER_BKGR_BUF_TYPE_DEF;
    double def_btree_split_ratio[3] = H5D_XFER_BTREE_SPLIT_RATIO_DEF;   /* Default value for B-tree node split ratios */
    H5MM_allocate_t def_vlen_alloc = H5D_XFER_VLEN_ALLOC_DEF;   /* Default value for vlen allocation function */
    void *def_vlen_alloc_info = H5D_XFER_VLEN_ALLOC_INFO_DEF;   /* Default value for vlen allocation information */
    H5MM_free_t def_vlen_free = H5D_XFER_VLEN_FREE_DEF;         /* Default value for vlen free function */
    void *def_vlen_free_info = H5D_XFER_VLEN_FREE_INFO_DEF;     /* Default value for vlen free information */
    size_t def_hyp_vec_size = H5D_XFER_HYPER_VECTOR_SIZE_DEF;   /* Default value for vector size */
#ifdef H5_HAVE_PARALLEL
    H5FD_mpio_xfer_t def_io_xfer_mode = H5D_XFER_IO_XFER_MODE_DEF;      /* Default value for I/O transfer mode */
    H5FD_mpio_chunk_opt_t def_mpio_chunk_opt_mode = H5D_XFER_MPIO_CHUNK_OPT_HARD_DEF;
    H5FD_mpio_collective_opt_t def_mpio_collective_opt_mode = H5D_XFER_MPIO_COLLECTIVE_OPT_DEF;
    unsigned def_mpio_chunk_opt_num = H5D_XFER_MPIO_CHUNK_OPT_NUM_DEF;
    unsigned def_mpio_chunk_opt_ratio = H5D_XFER_MPIO_CHUNK_OPT_RATIO_DEF;
    H5D_mpio_actual_chunk_opt_mode_t def_mpio_actual_chunk_opt_mode = H5D_MPIO_ACTUAL_CHUNK_OPT_MODE_DEF;
    H5D_mpio_actual_io_mode_t def_mpio_actual_io_mode = H5D_MPIO_ACTUAL_IO_MODE_DEF;
    H5D_mpio_no_collective_cause_t def_mpio_no_collective_cause = H5D_MPIO_NO_COLLECTIVE_CAUSE_DEF; 
    MPI_Datatype btype = H5FD_MPI_XFER_MEM_MPI_TYPE_DEF;  /* Default value for MPI buffer type */
    MPI_Datatype ftype = H5FD_MPI_XFER_FILE_MPI_TYPE_DEF; /* Default value for MPI file type */
#endif /* H5_HAVE_PARALLEL */
    H5Z_EDC_t enable_edc = H5D_XFER_EDC_DEF;            /* Default value for EDC property */
    H5Z_cb_t filter_cb = H5D_XFER_FILTER_CB_DEF;        /* Default value for filter callback */
    H5T_conv_cb_t conv_cb = H5D_XFER_CONV_CB_DEF;       /* Default value for datatype conversion callback */
    void *def_xfer_xform = H5D_XFER_XFORM_DEF;          /* Default value for data transform */
    hbool_t direct_chunk_flag = H5D_XFER_DIRECT_CHUNK_WRITE_FLAG_DEF; 	        /* Default value for the flag of direct chunk write */
    uint32_t direct_chunk_filters = H5D_XFER_DIRECT_CHUNK_WRITE_FILTERS_DEF;	/* Default value for the filters of direct chunk write */
    hsize_t *direct_chunk_offset = H5D_XFER_DIRECT_CHUNK_WRITE_OFFSET_DEF; 	/* Default value for the offset of direct chunk write */
    uint32_t direct_chunk_datasize = H5D_XFER_DIRECT_CHUNK_WRITE_DATASIZE_DEF;    /* Default value for the datasize of direct chunk write */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_STATIC

    /* Register the max. temp buffer size property */
    if(H5P_register_real(pclass, H5D_XFER_MAX_TEMP_BUF_NAME, H5D_XFER_MAX_TEMP_BUF_SIZE, &def_max_temp_buf, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the type conversion buffer property */
    if(H5P_register_real(pclass, H5D_XFER_TCONV_BUF_NAME, H5D_XFER_TCONV_BUF_SIZE, &def_tconv_buf, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the background buffer property */
    if(H5P_register_real(pclass, H5D_XFER_BKGR_BUF_NAME, H5D_XFER_BKGR_BUF_SIZE, &def_bkgr_buf, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the background buffer type property */
    if(H5P_register_real(pclass, H5D_XFER_BKGR_BUF_TYPE_NAME, H5D_XFER_BKGR_BUF_TYPE_SIZE, &def_bkgr_buf_type, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the B-Tree node splitting ratios property */
    if(H5P_register_real(pclass, H5D_XFER_BTREE_SPLIT_RATIO_NAME, H5D_XFER_BTREE_SPLIT_RATIO_SIZE, def_btree_split_ratio, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the vlen allocation function property */
    if(H5P_register_real(pclass, H5D_XFER_VLEN_ALLOC_NAME, H5D_XFER_VLEN_ALLOC_SIZE, &def_vlen_alloc, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the vlen allocation information property */
    if(H5P_register_real(pclass, H5D_XFER_VLEN_ALLOC_INFO_NAME, H5D_XFER_VLEN_ALLOC_INFO_SIZE, &def_vlen_alloc_info, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the vlen free function property */
    if(H5P_register_real(pclass, H5D_XFER_VLEN_FREE_NAME, H5D_XFER_VLEN_FREE_SIZE, &def_vlen_free, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the vlen free information property */
    if(H5P_register_real(pclass, H5D_XFER_VLEN_FREE_INFO_NAME, H5D_XFER_VLEN_FREE_INFO_SIZE, &def_vlen_free_info, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the vector size property */
    if(H5P_register_real(pclass, H5D_XFER_HYPER_VECTOR_SIZE_NAME, H5D_XFER_HYPER_VECTOR_SIZE_SIZE, &def_hyp_vec_size, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

#ifdef H5_HAVE_PARALLEL
    /* Register the I/O transfer mode properties */
    if(H5P_register_real(pclass, H5D_XFER_IO_XFER_MODE_NAME, H5D_XFER_IO_XFER_MODE_SIZE, &def_io_xfer_mode, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")
    if(H5P_register_real(pclass, H5D_XFER_MPIO_COLLECTIVE_OPT_NAME, H5D_XFER_MPIO_COLLECTIVE_OPT_SIZE, &def_mpio_collective_opt_mode, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")
    if(H5P_register_real(pclass, H5D_XFER_MPIO_CHUNK_OPT_HARD_NAME, H5D_XFER_MPIO_CHUNK_OPT_HARD_SIZE, &def_mpio_chunk_opt_mode, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")
    if(H5P_register_real(pclass, H5D_XFER_MPIO_CHUNK_OPT_NUM_NAME, H5D_XFER_MPIO_CHUNK_OPT_NUM_SIZE, &def_mpio_chunk_opt_num, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")
    if(H5P_register_real(pclass, H5D_XFER_MPIO_CHUNK_OPT_RATIO_NAME, H5D_XFER_MPIO_CHUNK_OPT_RATIO_SIZE, &def_mpio_chunk_opt_ratio, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the chunk optimization mode property. */
    if(H5P_register_real(pclass, H5D_MPIO_ACTUAL_CHUNK_OPT_MODE_NAME, H5D_MPIO_ACTUAL_CHUNK_OPT_MODE_SIZE, &def_mpio_actual_chunk_opt_mode, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the actual I/O mode property. */
    if(H5P_register_real(pclass, H5D_MPIO_ACTUAL_IO_MODE_NAME, H5D_MPIO_ACTUAL_IO_MODE_SIZE, &def_mpio_actual_io_mode, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the local cause of broken collective I/O */
    if(H5P_register_real(pclass, H5D_MPIO_LOCAL_NO_COLLECTIVE_CAUSE_NAME, H5D_MPIO_NO_COLLECTIVE_CAUSE_SIZE, &def_mpio_no_collective_cause, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the global cause of broken collective I/O */
    if(H5P_register_real(pclass, H5D_MPIO_GLOBAL_NO_COLLECTIVE_CAUSE_NAME, H5D_MPIO_NO_COLLECTIVE_CAUSE_SIZE, &def_mpio_no_collective_cause, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the MPI memory type property */
    if(H5P_register_real(pclass, H5FD_MPI_XFER_MEM_MPI_TYPE_NAME, H5FD_MPI_XFER_MEM_MPI_TYPE_SIZE, 
                         &btype, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the MPI file type property */
    if(H5P_register_real(pclass, H5FD_MPI_XFER_FILE_MPI_TYPE_NAME, H5FD_MPI_XFER_FILE_MPI_TYPE_SIZE, 
                         &ftype, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")
#endif /* H5_HAVE_PARALLEL */

    /* Register the EDC property */
    if(H5P_register_real(pclass, H5D_XFER_EDC_NAME, H5D_XFER_EDC_SIZE, &enable_edc, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the filter callback property */
    if(H5P_register_real(pclass, H5D_XFER_FILTER_CB_NAME, H5D_XFER_FILTER_CB_SIZE, &filter_cb, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the type conversion callback property */
    if(H5P_register_real(pclass, H5D_XFER_CONV_CB_NAME, H5D_XFER_CONV_CB_SIZE, &conv_cb, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the data transform property */
    if(H5P_register_real(pclass, H5D_XFER_XFORM_NAME, H5D_XFER_XFORM_SIZE, &def_xfer_xform, NULL, NULL, NULL, H5D_XFER_XFORM_DEL, H5D_XFER_XFORM_COPY, H5D_XFER_XFORM_CMP, H5D_XFER_XFORM_CLOSE) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the property of flag for direct chunk write */
    /* (Note: this property should not have an encode/decode callback -QAK) */
    if(H5P_register_real(pclass, H5D_XFER_DIRECT_CHUNK_WRITE_FLAG_NAME, H5D_XFER_DIRECT_CHUNK_WRITE_FLAG_SIZE, &direct_chunk_flag,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the property of filter for direct chunk write */
    /* (Note: this property should not have an encode/decode callback -QAK) */
    if(H5P_register_real(pclass, H5D_XFER_DIRECT_CHUNK_WRITE_FILTERS_NAME, H5D_XFER_DIRECT_CHUNK_WRITE_FILTERS_SIZE, &direct_chunk_filters,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the property of offset for direct chunk write */
    /* (Note: this property should not have an encode/decode callback -QAK) */
    if(H5P_register_real(pclass, H5D_XFER_DIRECT_CHUNK_WRITE_OFFSET_NAME, H5D_XFER_DIRECT_CHUNK_WRITE_OFFSET_SIZE, &direct_chunk_offset,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the property of datasize for direct chunk write */
    /* (Note: this property should not have an encode/decode callback -QAK) */
    if(H5P_register_real(pclass, H5D_XFER_DIRECT_CHUNK_WRITE_DATASIZE_NAME, H5D_XFER_DIRECT_CHUNK_WRITE_DATASIZE_SIZE, &direct_chunk_datasize,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5P__dxfr_reg_prop() */


/*-------------------------------------------------------------------------
 * Function: H5P_dxfr_xform_del
 *
 * Purpose: Frees memory allocated by H5P_dxfr_xform_set
 *
 * Return: Success: SUCCEED, Failure: FAIL
 *
 * Programmer: Leon Arber larber@uiuc.edu
 *
 * Date: April 9, 2004
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5P__dxfr_xform_del(hid_t UNUSED prop_id, const char UNUSED *name, size_t UNUSED size, void *value)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_STATIC

    HDassert(value);

    if(H5Z_xform_destroy(*(H5Z_data_xform_t **)value) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTCLOSEOBJ, FAIL, "error closing the parse tree")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5P__dxfr_xform_del() */


/*-------------------------------------------------------------------------
 * Function: H5P__dxfr_xform_copy
 *
 * Purpose: Creates a copy of the user's data transform string and its
 *              associated parse tree.
 *
 * Return: Success: SUCCEED, Failure: FAIL
 *
 * Programmer: Leon Arber larber@uiuc.edu
 *
 * Date: April 9, 2004
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5P__dxfr_xform_copy(const char UNUSED *name, size_t UNUSED size, void *value)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_STATIC

    HDassert(value);

    if(H5Z_xform_copy((H5Z_data_xform_t **)value) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTCLOSEOBJ, FAIL, "error copying the data transform info")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5P__dxfr_xform_copy() */


/*-------------------------------------------------------------------------
 * Function: H5P__dxfr_xform_cmp
 *
 * Purpose: Compare two data transforms.
 *
 * Return: positive if VALUE1 is greater than VALUE2, negative if VALUE2 is
 *		greater than VALUE1 and zero if VALUE1 and VALUE2 are equal.
 *
 * Programmer:     Quincey Koziol
 *                 Wednesday, August 15, 2012
 *
 *-------------------------------------------------------------------------
 */
static int
H5P__dxfr_xform_cmp(const void *_xform1, const void *_xform2, size_t UNUSED size)
{
    const H5Z_data_xform_t * const *xform1 = (const H5Z_data_xform_t * const *)_xform1; /* Create local aliases for values */
    const H5Z_data_xform_t * const *xform2 = (const H5Z_data_xform_t * const *)_xform2; /* Create local aliases for values */
    const char *pexp1, *pexp2;          /* Pointers to transform expressions */
    herr_t ret_value = 0;               /* Return value */

    FUNC_ENTER_STATIC_NOERR

    /* Sanity check */
    HDassert(xform1);
    HDassert(xform2);
    HDassert(size == sizeof(H5Z_data_xform_t *));

    /* Check for a property being set */
    if(*xform1 == NULL && *xform2 != NULL) HGOTO_DONE(-1);
    if(*xform1 != NULL && *xform2 == NULL) HGOTO_DONE(1);

    if(*xform1) {
        HDassert(*xform2);
    
        /* Get the transform expressions */
        pexp1 = H5Z_xform_extract_xform_str(*xform1);
        pexp2 = H5Z_xform_extract_xform_str(*xform2);

        /* Check for property expressions */
        if(pexp1 == NULL && pexp2 != NULL) HGOTO_DONE(-1);
        if(pexp1 != NULL && pexp2 == NULL) HGOTO_DONE(1);

        if(pexp1) {
            HDassert(pexp2);
            ret_value = HDstrcmp(pexp1, pexp2);
        } /* end if */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5P__dxfr_xform_copy() */


/*-------------------------------------------------------------------------
 * Function: H5P__dxfr_xform_close
 *
 * Purpose: Frees memory allocated by H5P_dxfr_xform_set
 *
 * Return: Success: SUCCEED, Failure: FAIL
 *
 * Programmer: Leon Arber larber@uiuc.edu
 *
 * Date: April 9, 2004
 *
 *-------------------------------------------------------------------------
 */
/* ARGSUSED */
static herr_t
H5P__dxfr_xform_close(const char UNUSED *name, size_t UNUSED size, void *value)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_STATIC

    HDassert(value);

    if(H5Z_xform_destroy(*(H5Z_data_xform_t **)value) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTCLOSEOBJ, FAIL, "error closing the parse tree")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5P__dxfr_xform_close() */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_data_transform
 *
 * Purpose:	Sets data transform expression.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Leon Arber
 *              Monday, March 07, 2004
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_data_transform(hid_t plist_id, const char *expression)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    H5Z_data_xform_t *data_xform_prop = NULL;    /* New data xform property */
    herr_t ret_value = SUCCEED;   /* return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*s", plist_id, expression);

    /* Check arguments */
    if(expression == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "expression cannot be NULL")

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* See if a data transform is already set, and free it if it is */
    if(H5P_get(plist, H5D_XFER_XFORM_NAME, &data_xform_prop) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "error getting data transform expression")

    /* Destroy previous data transform property */
    if(H5Z_xform_destroy(data_xform_prop) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CLOSEERROR, FAIL, "unable to release data transform expression")

    /* Create data transform info from expression */
    if(NULL == (data_xform_prop = H5Z_xform_create(expression)))
        HGOTO_ERROR(H5E_PLINE, H5E_NOSPACE, FAIL, "unable to create data transform info")

    /* Update property list */
    if(H5P_set(plist, H5D_XFER_XFORM_NAME, &data_xform_prop) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "Error setting data transform expression")

done:
    if(ret_value <  0) {
        if(data_xform_prop && H5Z_xform_destroy(data_xform_prop) <  0)
            HDONE_ERROR(H5E_PLINE, H5E_CLOSEERROR, FAIL, "unable to release data transform expression")
    } /* end if */

    FUNC_LEAVE_API(ret_value)
} /* end H5Pset_data_transform() */


/*-------------------------------------------------------------------------
 * Function:	H5Pget_data_transform
 *
 * Purpose:	Gets data transform expression.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Comments:
 *  If `expression' is non-NULL then write up to `size' bytes into that
 *  buffer and always return the length of the transform name.
 *  Otherwise `size' is ignored and the function does not store the expression,
 *  just returning the number of characters required to store the expression.
 *  If an error occurs then the buffer pointed to by `expression' (NULL or non-NULL)
 *  is unchanged and the function returns a negative value.
 *  If a zero is returned for the name's length, then there is no name
 *  associated with the ID.
 *
 * Programmer:	Leon Arber
 *              August 27, 2004
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5Pget_data_transform(hid_t plist_id, char *expression /*out*/, size_t size)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    H5Z_data_xform_t *data_xform_prop = NULL;    /* New data xform property */
    size_t	len;
    const char*	pexp;
    ssize_t 	ret_value;   /* return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("Zs", "ixz", plist_id, expression, size);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    if(H5P_get(plist, H5D_XFER_XFORM_NAME, &data_xform_prop) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "error getting data transform expression")

    if(NULL == data_xform_prop)
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "data transform has not been set")

    /* Get the data transform string */
    if(NULL == (pexp = H5Z_xform_extract_xform_str(data_xform_prop)))
	HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "failed to retrieve transform expression")

    len = HDstrlen(pexp);
    if(expression) {
	HDstrncpy(expression, pexp, MIN(len + 1, size));
        if(len >= size)
            expression[size - 1] = '\0';
    } /* end if */

    ret_value = (ssize_t)len;

done:
    if(ret_value < 0) {
	if(data_xform_prop && H5Z_xform_destroy(data_xform_prop) < 0)
            HDONE_ERROR(H5E_PLINE, H5E_CLOSEERROR, FAIL, "unable to release data transform expression")
    } /* end if */

    FUNC_LEAVE_API(ret_value)
} /* end H5Pget_data_transform() */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_buffer
 *
 * Purpose:	Given a dataset transfer property list, set the maximum size
 *		for the type conversion buffer and background buffer and
 *		optionally supply pointers to application-allocated buffers.
 *		If the buffer size is smaller than the entire amount of data
 *		being transfered between application and file, and a type
 *		conversion buffer or background buffer is required then
 *		strip mining will be used.
 *
 *		If TCONV and/or BKG are null pointers then buffers will be
 *		allocated and freed during the data transfer.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Monday, March 16, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_buffer(hid_t plist_id, size_t size, void *tconv, void *bkg)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value = SUCCEED; /* return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE4("e", "iz*x*x", plist_id, size, tconv, bkg);

    /* Check arguments */
    if(size == 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "buffer size must not be zero")

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Update property list */
    if(H5P_set(plist, H5D_XFER_MAX_TEMP_BUF_NAME, &size) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "Can't set transfer buffer size")
    if(H5P_set(plist, H5D_XFER_TCONV_BUF_NAME, &tconv) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "Can't set transfer type conversion buffer")
    if(H5P_set(plist, H5D_XFER_BKGR_BUF_NAME, &bkg) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "Can't set background type conversion buffer")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pset_buffer() */


/*-------------------------------------------------------------------------
 * Function:	H5Pget_buffer
 *
 * Purpose:	Reads values previously set with H5Pset_buffer().
 *
 * Return:	Success:	Buffer size.
 *
 *		Failure:	0
 *
 * Programmer:	Robb Matzke
 *              Monday, March 16, 1998
 *
 *-------------------------------------------------------------------------
 */
size_t
H5Pget_buffer(hid_t plist_id, void **tconv/*out*/, void **bkg/*out*/)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    size_t size;                /* Type conversion buffer size */
    size_t ret_value;           /* Return value */

    FUNC_ENTER_API(0)
    H5TRACE3("z", "ixx", plist_id, tconv, bkg);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, 0, "can't find object for ID")

    /* Return values */
    if(tconv)
        if(H5P_get(plist, H5D_XFER_TCONV_BUF_NAME, tconv) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, 0, "Can't get transfer type conversion buffer")
    if(bkg)
        if(H5P_get(plist, H5D_XFER_BKGR_BUF_NAME, bkg) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, 0, "Can't get background type conversion buffer")

    /* Get the size */
    if(H5P_get(plist, H5D_XFER_MAX_TEMP_BUF_NAME, &size) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, 0, "Can't set transfer buffer size")

    /* Set the return value */
    ret_value = size;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pget_buffer() */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_preserve
 *
 * Purpose:	When reading or writing compound data types and the
 *		destination is partially initialized and the read/write is
 *		intended to initialize the other members, one must set this
 *		property to TRUE.  Otherwise the I/O pipeline treats the
 *		destination datapoints as completely uninitialized.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Tuesday, March 17, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_preserve(hid_t plist_id, hbool_t status)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    H5T_bkg_t need_bkg;         /* Value for background buffer type */
    herr_t ret_value = SUCCEED; /* return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "ib", plist_id, status);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Update property list */
    need_bkg = status ? H5T_BKG_YES : H5T_BKG_NO;
    if(H5P_set(plist, H5D_XFER_BKGR_BUF_TYPE_NAME, &need_bkg) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pset_preserve() */


/*-------------------------------------------------------------------------
 * Function:	H5Pget_preserve
 *
 * Purpose:	The inverse of H5Pset_preserve()
 *
 * Return:	Success:	TRUE or FALSE
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Tuesday, March 17, 1998
 *
 *-------------------------------------------------------------------------
 */
int
H5Pget_preserve(hid_t plist_id)
{
    H5T_bkg_t need_bkg;         /* Background value */
    H5P_genplist_t *plist;      /* Property list pointer */
    int ret_value;              /* return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE1("Is", "i", plist_id);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Get value */
    if(H5P_get(plist, H5D_XFER_BKGR_BUF_TYPE_NAME, &need_bkg) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get value")

    /* Set return value */
    ret_value = need_bkg ? TRUE : FALSE;

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pget_preserve() */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_edc_check
 *
 * Purpose:     Enable or disable error-detecting for a dataset reading
 *              process.  This error-detecting algorithm is whichever
 *              user chooses earlier.  This function cannot control
 *              writing process.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 *              Jan 3, 2003
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_edc_check(hid_t plist_id, H5Z_EDC_t check)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value = SUCCEED; /* return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "iZe", plist_id, check);

    /* Check argument */
    if(check != H5Z_ENABLE_EDC && check != H5Z_DISABLE_EDC)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a valid value")

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Update property list */
    if(H5P_set(plist, H5D_XFER_EDC_NAME, &check) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pset_edc_check() */


/*-------------------------------------------------------------------------
 * Function:	H5Pget_edc_check
 *
 * Purpose:     Enable or disable error-detecting for a dataset reading
 *              process.  This error-detecting algorithm is whichever
 *              user chooses earlier.  This function cannot control
 *              writing process.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 *              Jan 3, 2003
 *
 *-------------------------------------------------------------------------
 */
H5Z_EDC_t
H5Pget_edc_check(hid_t plist_id)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    H5Z_EDC_t      ret_value;   /* Return value */

    FUNC_ENTER_API(H5Z_ERROR_EDC)
    H5TRACE1("Ze", "i", plist_id);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, H5Z_ERROR_EDC, "can't find object for ID")

    /* Update property list */
    if(H5P_get(plist, H5D_XFER_EDC_NAME, &ret_value) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, H5Z_ERROR_EDC, "unable to set value")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pget_edc_check() */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_filter_callback
 *
 * Purpose:     Sets user's callback function for dataset transfer property
 *              list.  This callback function defines what user wants to do
 *              if certain filter fails.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 *              Jan 14, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_filter_callback(hid_t plist_id, H5Z_filter_func_t func, void *op_data)
{
    H5P_genplist_t      *plist;      /* Property list pointer */
    herr_t              ret_value=SUCCEED;   /* return value */
    H5Z_cb_t            cb_struct;

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "ix*x", plist_id, func, op_data);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Update property list */
    cb_struct.func = func;
    cb_struct.op_data = op_data;

    if (H5P_set(plist,H5D_XFER_FILTER_CB_NAME,&cb_struct)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value")

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5Pset_type_conv_cb
 *
 * Purpose:     Sets user's callback function for dataset transfer property
 *              list.  This callback function defines what user wants to do
 *              if there's exception during datatype conversion.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 *              April 15, 2004
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_type_conv_cb(hid_t plist_id, H5T_conv_except_func_t op, void *operate_data)
{
    H5P_genplist_t      *plist;      /* Property list pointer */
    herr_t              ret_value=SUCCEED;   /* return value */
    H5T_conv_cb_t       cb_struct;

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "ix*x", plist_id, op, operate_data);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Update property list */
    cb_struct.func = op;
    cb_struct.user_data = operate_data;

    if (H5P_set(plist,H5D_XFER_CONV_CB_NAME,&cb_struct)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value")

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5Pget_type_conv_cb
 *
 * Purpose:     Gets callback function for dataset transfer property
 *              list.  This callback function defines what user wants to do
 *              if there's exception during datatype conversion.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Raymond Lu
 *              April 15, 2004
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_type_conv_cb(hid_t plist_id, H5T_conv_except_func_t *op, void **operate_data)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    H5T_conv_cb_t       cb_struct;
    herr_t              ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "i*x**x", plist_id, op, operate_data);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Get property */
    if (H5P_get(plist,H5D_XFER_CONV_CB_NAME,&cb_struct)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value")

    /* Assign return value */
    *op = cb_struct.func;
    *operate_data = cb_struct.user_data;

done:
    FUNC_LEAVE_API(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:	H5Pget_btree_ratios
 *
 * Purpose:	Queries B-tree split ratios.  See H5Pset_btree_ratios().
 *
 * Return:	Success:	Non-negative with split ratios returned through
 *				the non-null arguments.
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *              Monday, September 28, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_btree_ratios(hid_t plist_id, double *left/*out*/, double *middle/*out*/,
    double *right/*out*/)
{
    H5P_genplist_t *plist;              /* Property list pointer */
    double btree_split_ratio[3];        /* B-tree node split ratios */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE4("e", "ixxx", plist_id, left, middle, right);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Get the split ratios */
    if(H5P_get(plist, H5D_XFER_BTREE_SPLIT_RATIO_NAME, &btree_split_ratio) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get value")

    /* Get values */
    if(left)
        *left = btree_split_ratio[0];
    if(middle)
        *middle = btree_split_ratio[1];
    if(right)
        *right = btree_split_ratio[2];

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pget_btree_ratios() */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_btree_ratios
 *
 * Purpose:	Sets B-tree split ratios for a dataset transfer property
 *		list. The split ratios determine what percent of children go
 *		in the first node when a node splits.  The LEFT ratio is
 *		used when the splitting node is the left-most node at its
 *		level in the tree; the RIGHT ratio is when the splitting node
 *		is the right-most node at its level; and the MIDDLE ratio for
 *		all other cases.  A node which is the only node at its level
 *		in the tree uses the RIGHT ratio when it splits.  All ratios
 *		are real numbers between 0 and 1, inclusive.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *              Monday, September 28, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_btree_ratios(hid_t plist_id, double left, double middle,
    double right)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    double split_ratio[3];      /* B-tree node split ratios */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE4("e", "iddd", plist_id, left, middle, right);

    /* Check arguments */
    if(left < (double)0.0f || left > (double)1.0f
            || middle < (double)0.0f || middle > (double)1.0f
            || right < (double)0.0f || right > (double)1.0f)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "split ratio must satisfy 0.0<=X<=1.0")

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Set values */
    split_ratio[0] = left;
    split_ratio[1] = middle;
    split_ratio[2] = right;

    /* Set the split ratios */
    if(H5P_set(plist, H5D_XFER_BTREE_SPLIT_RATIO_NAME, &split_ratio) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pset_btree_ratios() */


/*-------------------------------------------------------------------------
 * Function:	H5P_set_vlen_mem_manager
 *
 * Purpose:	Sets the memory allocate/free pair for VL datatypes.  The
 *		allocation routine is called when data is read into a new
 *		array and the free routine is called when H5Dvlen_reclaim is
 *		called.  The alloc_info and free_info are user parameters
 *		which are passed to the allocation and freeing functions
 *		respectively.  To reset the allocate/free functions to the
 *		default setting of using the system's malloc/free functions,
 *		call this routine with alloc_func and free_func set to NULL.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, July 1, 1999
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5P_set_vlen_mem_manager(H5P_genplist_t *plist, H5MM_allocate_t alloc_func,
    void *alloc_info, H5MM_free_t free_func, void *free_info)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(plist);

    /* Update property list */
    if(H5P_set(plist, H5D_XFER_VLEN_ALLOC_NAME, &alloc_func) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value")
    if(H5P_set(plist, H5D_XFER_VLEN_ALLOC_INFO_NAME, &alloc_info) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value")
    if(H5P_set(plist, H5D_XFER_VLEN_FREE_NAME, &free_func) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value")
    if(H5P_set(plist, H5D_XFER_VLEN_FREE_INFO_NAME, &free_info) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5P_set_vlen_mem_manager() */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_vlen_mem_manager
 *
 * Purpose:	Sets the memory allocate/free pair for VL datatypes.  The
 *		allocation routine is called when data is read into a new
 *		array and the free routine is called when H5Dvlen_reclaim is
 *		called.  The alloc_info and free_info are user parameters
 *		which are passed to the allocation and freeing functions
 *		respectively.  To reset the allocate/free functions to the
 *		default setting of using the system's malloc/free functions,
 *		call this routine with alloc_func and free_func set to NULL.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, July 1, 1999
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_vlen_mem_manager(hid_t plist_id, H5MM_allocate_t alloc_func,
    void *alloc_info, H5MM_free_t free_func, void *free_info)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE5("e", "ix*xx*x", plist_id, alloc_func, alloc_info, free_func,
             free_info);

    /* Check arguments */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset transfer property list")

    /* Update property list */
    if(H5P_set_vlen_mem_manager(plist, alloc_func, alloc_info, free_func, free_info) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set values")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pset_vlen_mem_manager() */


/*-------------------------------------------------------------------------
 * Function:	H5Pget_vlen_mem_manager
 *
 * Purpose:	The inverse of H5Pset_vlen_mem_manager()
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, July 1, 1999
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_vlen_mem_manager(hid_t plist_id, H5MM_allocate_t *alloc_func/*out*/,
    void **alloc_info/*out*/, H5MM_free_t *free_func/*out*/, void **free_info/*out*/)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE5("e", "ixxxx", plist_id, alloc_func, alloc_info, free_func, free_info);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    if(alloc_func)
        if(H5P_get(plist, H5D_XFER_VLEN_ALLOC_NAME, alloc_func) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get value")
    if(alloc_info)
        if(H5P_get(plist, H5D_XFER_VLEN_ALLOC_INFO_NAME, alloc_info) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get value")
    if(free_func)
        if(H5P_get(plist, H5D_XFER_VLEN_FREE_NAME, free_func) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get value")
    if(free_info)
        if(H5P_get(plist, H5D_XFER_VLEN_FREE_INFO_NAME, free_info) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get value")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pget_vlen_mem_manager() */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_hyper_vector_size
 *
 * Purpose:	Given a dataset transfer property list, set the number of
 *              "I/O vectors" (offset and length pairs) which are to be
 *              accumulated in memory before being issued to the lower levels
 *              of the library for reading or writing the actual data.
 *              Increasing the number should give better performance, but use
 *              more memory during hyperslab I/O.  The vector size must be
 *              greater than 1.
 *
 *		The default is to use 1024 vectors for I/O during hyperslab
 *              reading/writing.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Monday, July 9, 2001
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_hyper_vector_size(hid_t plist_id, size_t vector_size)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "iz", plist_id, vector_size);

    /* Check arguments */
    if(vector_size < 1)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "vector size too small")

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Update property list */
    if(H5P_set(plist, H5D_XFER_HYPER_VECTOR_SIZE_NAME, &vector_size) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set value")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pset_hyper_vector_size() */


/*-------------------------------------------------------------------------
 * Function:	H5Pget_hyper_vector_size
 *
 * Purpose:	Reads values previously set with H5Pset_hyper_vector_size().
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Monday, July 9, 2001
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_hyper_vector_size(hid_t plist_id, size_t *vector_size/*out*/)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "ix", plist_id, vector_size);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Return values */
    if(vector_size)
        if(H5P_get(plist, H5D_XFER_HYPER_VECTOR_SIZE_NAME, vector_size) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get value")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pget_hyper_vector_size() */

#ifdef H5_HAVE_PARALLEL

/*-------------------------------------------------------------------------
 * Function:	H5Pget_mpio_actual_chunk_opt_mode
 *
 * Purpose:	Retrieves the chunked io optimization scheme that library chose
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Jacob Gruber
 *              Wednesday, May 4, 2011
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_mpio_actual_chunk_opt_mode(hid_t plist_id, H5D_mpio_actual_chunk_opt_mode_t *actual_chunk_opt_mode)
{
    H5P_genplist_t     *plist;
    herr_t ret_value = SUCCEED;   /* return value */
    
    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*Do", plist_id, actual_chunk_opt_mode);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Return values */
    if(actual_chunk_opt_mode)
        if(H5P_get(plist, H5D_MPIO_ACTUAL_CHUNK_OPT_MODE_NAME, actual_chunk_opt_mode) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get value")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pget_mpio_actual_chunk_opt_mode() */


/*-------------------------------------------------------------------------
 * Function:	H5Pget_mpio_actual_io_mode
 *
 * Purpose:	Retrieves the type of I/O actually preformed when collective I/O
 *		is requested.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Jacob Gruber
 *              Wednesday, May 4, 2011
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_mpio_actual_io_mode(hid_t plist_id, H5D_mpio_actual_io_mode_t *actual_io_mode)
{
    H5P_genplist_t     *plist;
    herr_t ret_value = SUCCEED;   /* return value */
    
    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*Di", plist_id, actual_io_mode);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Return values */
    if(actual_io_mode)
        if(H5P_get(plist, H5D_MPIO_ACTUAL_IO_MODE_NAME, actual_io_mode) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get value")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pget_mpio_actual_io_mode() */

/*-------------------------------------------------------------------------
 * Function:	H5Pget_mpio_no_collective_cause
 *
 * Purpose:	Retrieves cause for the broke collective I/O
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Jonathan Kim
 *              Aug 3, 2012
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_mpio_no_collective_cause(hid_t plist_id, uint32_t *local_no_collective_cause, uint32_t *global_no_collective_cause)
{
    H5P_genplist_t     *plist;
    herr_t ret_value = SUCCEED;   /* return value */
    
    FUNC_ENTER_API(FAIL)
    H5TRACE3("e", "i*Iu*Iu", plist_id, local_no_collective_cause,
             global_no_collective_cause);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_DATASET_XFER)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Return values */
    if(local_no_collective_cause)
        if(H5P_get(plist, H5D_MPIO_LOCAL_NO_COLLECTIVE_CAUSE_NAME, local_no_collective_cause) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get local value")
    if(global_no_collective_cause)
        if(H5P_get(plist, H5D_MPIO_GLOBAL_NO_COLLECTIVE_CAUSE_NAME, global_no_collective_cause) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get global value")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pget_mpio_no_collective_cause() */


#endif /* H5_HAVE_PARALLEL */

