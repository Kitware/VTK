/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Wednesday, October  8, 1997
 *
 * Purpose:     Messages related to data layout.
 */

#define H5D_FRIEND              /*suppress error about including H5Dpkg	  */
#include "H5Omodule.h"          /* This source code file is part of the H5O module */


#include "H5private.h"          /* Generic Functions                        */
#include "H5Dpkg.h"             /* Dataset functions                        */
#include "H5Eprivate.h"         /* Error handling                           */
#include "H5FLprivate.h"        /* Free Lists                               */
#include "H5MFprivate.h"        /* File space management                    */
#include "H5MMprivate.h"        /* Memory management                        */
#include "H5Opkg.h"             /* Object headers                           */
#include "H5Pprivate.h"         /* Property lists                           */


/* Local macros */


/* PRIVATE PROTOTYPES */
static void *H5O__layout_decode(H5F_t *f, H5O_t *open_oh, unsigned mesg_flags,
    unsigned *ioflags, size_t p_size, const uint8_t *p);
static herr_t H5O__layout_encode(H5F_t *f, hbool_t disable_shared, uint8_t *p, const void *_mesg);
static void *H5O__layout_copy(const void *_mesg, void *_dest);
static size_t H5O__layout_size(const H5F_t *f, hbool_t disable_shared, const void *_mesg);
static herr_t H5O__layout_reset(void *_mesg);
static herr_t H5O__layout_free(void *_mesg);
static herr_t H5O__layout_delete(H5F_t *f, H5O_t *open_oh, void *_mesg);
static herr_t H5O__layout_pre_copy_file(H5F_t *file_src, const void *mesg_src,
    hbool_t *deleted, const H5O_copy_t *cpy_info, void *udata);
static void *H5O__layout_copy_file(H5F_t *file_src, void *mesg_src,
    H5F_t *file_dst, hbool_t *recompute_size, unsigned *mesg_flags,
    H5O_copy_t *cpy_info, void *udata);
static herr_t H5O__layout_debug(H5F_t *f, const void *_mesg, FILE *stream,
    int indent, int fwidth);

/* This message derives from H5O message class */
const H5O_msg_class_t H5O_MSG_LAYOUT[1] = {{
    H5O_LAYOUT_ID,              /* message id number                    */
    "layout",                   /* message name for debugging           */
    sizeof(H5O_layout_t),       /* native message size                  */
    0,                          /* messages are sharable?               */
    H5O__layout_decode,         /* decode message                       */
    H5O__layout_encode,         /* encode message                       */
    H5O__layout_copy,           /* copy the native value                */
    H5O__layout_size,           /* size of message on disk              */
    H5O__layout_reset,          /* reset method                         */
    H5O__layout_free,           /* free the struct                      */
    H5O__layout_delete,	        /* file delete method                   */
    NULL,                       /* link method                          */
    NULL,                       /* set share method                     */
    NULL,                       /* can share method                     */
    H5O__layout_pre_copy_file,  /* pre copy native value to file        */
    H5O__layout_copy_file,      /* copy native value to file            */
    NULL,                       /* post copy native value to file       */
    NULL,                       /* get creation index                   */
    NULL,                       /* set creation index                   */
    H5O__layout_debug           /* debug the message                    */
}};


/* Declare a free list to manage the H5O_layout_t struct */
H5FL_DEFINE(H5O_layout_t);


/*-------------------------------------------------------------------------
 * Function:    H5O__layout_decode
 *
 * Purpose:     Decode an data layout message and return a pointer to a
 *              new one created with malloc().
 *
 * Return:      Success:        Ptr to new message in native order.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Wednesday, October  8, 1997
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O__layout_decode(H5F_t *f, H5O_t H5_ATTR_UNUSED *open_oh,
    unsigned H5_ATTR_UNUSED mesg_flags, unsigned H5_ATTR_UNUSED *ioflags,
    size_t H5_ATTR_UNUSED p_size, const uint8_t *p)
{
    H5O_layout_t           *mesg = NULL;
    uint8_t                *heap_block = NULL;
    unsigned               u;
    void                   *ret_value = NULL;   /* Return value */

    FUNC_ENTER_STATIC

    /* check args */
    HDassert(f);
    HDassert(p);

    /* decode */
    if(NULL == (mesg = H5FL_CALLOC(H5O_layout_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")
    mesg->storage.type = H5D_LAYOUT_ERROR;

    mesg->version = *p++;
    if(mesg->version < H5O_LAYOUT_VERSION_1 || mesg->version > H5O_LAYOUT_VERSION_4)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "bad version number for layout message")

    if(mesg->version < H5O_LAYOUT_VERSION_3) {
        unsigned        ndims;          /* Num dimensions in chunk */

        /* Dimensionality */
        ndims = *p++;
        if(!ndims || ndims > H5O_LAYOUT_NDIMS)
            HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "dimensionality is out of range")

        /* Layout class */
        mesg->type = (H5D_layout_t)*p++;
        HDassert(H5D_CONTIGUOUS == mesg->type || H5D_CHUNKED == mesg->type || H5D_COMPACT == mesg->type);

        /* Set the storage type */
        mesg->storage.type = mesg->type;

        /* Reserved bytes */
        p += 5;

        /* Address */
        if(mesg->type == H5D_CONTIGUOUS) {
            H5F_addr_decode(f, &p, &(mesg->storage.u.contig.addr));

            /* Set the layout operations */
            mesg->ops = H5D_LOPS_CONTIG;
        } /* end if */
        else if(mesg->type == H5D_CHUNKED) {
            H5F_addr_decode(f, &p, &(mesg->storage.u.chunk.idx_addr));

            /* Set the layout operations */
            mesg->ops = H5D_LOPS_CHUNK;

            /* Set the chunk operations */
            /* (Only "btree" indexing type currently supported in this version) */
            mesg->storage.u.chunk.idx_type = H5D_CHUNK_IDX_BTREE;
            mesg->storage.u.chunk.ops = H5D_COPS_BTREE;
        } /* end if */
        else {
            /* Sanity check */
            HDassert(mesg->type == H5D_COMPACT);

            /* Set the layout operations */
            mesg->ops = H5D_LOPS_COMPACT;
        } /* end else */

        /* Read the size */
        if(mesg->type != H5D_CHUNKED) {
            /* Don't compute size of contiguous storage here, due to possible
             * truncation of the dimension sizes when they were stored in this
             * version of the layout message.  Compute the contiguous storage
             * size in the dataset code, where we've got the dataspace
             * information available also.  - QAK 5/26/04
             */
            p += ndims * 4;     /* Skip over dimension sizes (32-bit quantities) */
        } /* end if */
        else {
            mesg->u.chunk.ndims=ndims;
            for(u = 0; u < ndims; u++)
                UINT32DECODE(p, mesg->u.chunk.dim[u]);

            /* Compute chunk size */
            for(u = 1, mesg->u.chunk.size = mesg->u.chunk.dim[0]; u < ndims; u++)
                mesg->u.chunk.size *= mesg->u.chunk.dim[u];
        } /* end if */

        if(mesg->type == H5D_COMPACT) {
            UINT32DECODE(p, mesg->storage.u.compact.size);
            if(mesg->storage.u.compact.size > 0) {
                if(NULL == (mesg->storage.u.compact.buf = H5MM_malloc(mesg->storage.u.compact.size)))
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for compact data buffer")
                HDmemcpy(mesg->storage.u.compact.buf, p, mesg->storage.u.compact.size);
                p += mesg->storage.u.compact.size;
            } /* end if */
        } /* end if */
    } /* end if */
    else {
        /* Layout & storage class */
        mesg->type = mesg->storage.type = (H5D_layout_t)*p++;

        /* Interpret the rest of the message according to the layout class */
        switch(mesg->type) {
            case H5D_COMPACT:
                /* Compact data size */
                UINT16DECODE(p, mesg->storage.u.compact.size);

                if(mesg->storage.u.compact.size > 0) {
                    /* Allocate space for compact data */
                    if(NULL == (mesg->storage.u.compact.buf = H5MM_malloc(mesg->storage.u.compact.size)))
                        HGOTO_ERROR(H5E_OHDR, H5E_CANTALLOC, NULL, "memory allocation failed for compact data buffer")

                    /* Compact data */
                    HDmemcpy(mesg->storage.u.compact.buf, p, mesg->storage.u.compact.size);
                    p += mesg->storage.u.compact.size;
                } /* end if */

                /* Set the layout operations */
                mesg->ops = H5D_LOPS_COMPACT;
                break;

            case H5D_CONTIGUOUS:
                /* Contiguous storage address */
                H5F_addr_decode(f, &p, &(mesg->storage.u.contig.addr));

                /* Contiguous storage size */
                H5F_DECODE_LENGTH(f, p, mesg->storage.u.contig.size);

                /* Set the layout operations */
                mesg->ops = H5D_LOPS_CONTIG;
                break;

            case H5D_CHUNKED:
                if(mesg->version < H5O_LAYOUT_VERSION_4) {
                    /* Set the chunked layout flags */
                    mesg->u.chunk.flags = (uint8_t)0;

                    /* Dimensionality */
                    mesg->u.chunk.ndims = *p++;
                    if(mesg->u.chunk.ndims > H5O_LAYOUT_NDIMS)
                        HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "dimensionality is too large")

                    /* B-tree address */
                    H5F_addr_decode(f, &p, &(mesg->storage.u.chunk.idx_addr));

                    /* Chunk dimensions */
                    for(u = 0; u < mesg->u.chunk.ndims; u++) {
                        UINT32DECODE(p, mesg->u.chunk.dim[u]);

                        /* Just in case that something goes very wrong, such as file corruption. */
                        if(mesg->u.chunk.dim[u] == 0)
                            HGOTO_ERROR(H5E_DATASET, H5E_BADVALUE, NULL, "chunk dimension must be positive: mesg->u.chunk.dim[%u] = %u",
                                        u, mesg->u.chunk.dim[u])
                    } /* end for */

                    /* Compute chunk size */
                    for(u = 1, mesg->u.chunk.size = mesg->u.chunk.dim[0]; u < mesg->u.chunk.ndims; u++)
                        mesg->u.chunk.size *= mesg->u.chunk.dim[u];

                    /* Set the chunk operations */
                    /* (Only "btree" indexing type supported with v3 of message format) */
                    mesg->storage.u.chunk.idx_type = H5D_CHUNK_IDX_BTREE;
                    mesg->storage.u.chunk.ops = H5D_COPS_BTREE;
                } /* end if */
                else {
                    /* Get the chunked layout flags */
                    mesg->u.chunk.flags = *p++;

                    /* Check for valid flags */
                    /* (Currently issues an error for all non-zero values,
                     *      until features are added for the flags)
                     */
                    if(mesg->u.chunk.flags & ~H5O_LAYOUT_ALL_CHUNK_FLAGS)
                        HGOTO_ERROR(H5E_OHDR, H5E_BADVALUE, NULL, "bad flag value for message")

                    /* Dimensionality */
                    mesg->u.chunk.ndims = *p++;
                    if(mesg->u.chunk.ndims > H5O_LAYOUT_NDIMS)
                        HGOTO_ERROR(H5E_OHDR, H5E_BADVALUE, NULL, "dimensionality is too large")

                    /* Encoded # of bytes for each chunk dimension */
                    mesg->u.chunk.enc_bytes_per_dim = *p++;
                    if(mesg->u.chunk.enc_bytes_per_dim == 0 || mesg->u.chunk.enc_bytes_per_dim > 8)
                        HGOTO_ERROR(H5E_OHDR, H5E_BADVALUE, NULL, "encoded chunk dimension size is too large")

                    /* Chunk dimensions */
                    for(u = 0; u < mesg->u.chunk.ndims; u++)
                        UINT64DECODE_VAR(p, mesg->u.chunk.dim[u], mesg->u.chunk.enc_bytes_per_dim);

                    /* Compute chunk size */
                    for(u = 1, mesg->u.chunk.size = mesg->u.chunk.dim[0]; u < mesg->u.chunk.ndims; u++)
                        mesg->u.chunk.size *= mesg->u.chunk.dim[u];

                    /* Chunk index type */
                    mesg->u.chunk.idx_type = (H5D_chunk_index_t)*p++;
                    if(mesg->u.chunk.idx_type >= H5D_CHUNK_IDX_NTYPES)
                        HGOTO_ERROR(H5E_OHDR, H5E_BADVALUE, NULL, "unknown chunk index type")
                    mesg->storage.u.chunk.idx_type = mesg->u.chunk.idx_type;

                    switch(mesg->u.chunk.idx_type) {
                        case H5D_CHUNK_IDX_BTREE:
                            HGOTO_ERROR(H5E_OHDR, H5E_BADVALUE, NULL, "v1 B-tree index type should never be in a v4 layout message")
                            break;

                        case H5D_CHUNK_IDX_NONE:       /* Implicit Index */
                            mesg->storage.u.chunk.ops = H5D_COPS_NONE;
                            break;

                        case H5D_CHUNK_IDX_SINGLE:     /* Single Chunk Index */
                            if(mesg->u.chunk.flags & H5O_LAYOUT_CHUNK_SINGLE_INDEX_WITH_FILTER) {
                                H5F_DECODE_LENGTH(f, p, mesg->storage.u.chunk.u.single.nbytes);
                                UINT32DECODE(p, mesg->storage.u.chunk.u.single.filter_mask);
                            } /* end if */

                            /* Set the chunk operations */
                            mesg->storage.u.chunk.ops = H5D_COPS_SINGLE;
                            break;

                        case H5D_CHUNK_IDX_FARRAY:
                            /* Fixed array creation parameters */
                            mesg->u.chunk.u.farray.cparam.max_dblk_page_nelmts_bits = *p++;
                            if(0 == mesg->u.chunk.u.farray.cparam.max_dblk_page_nelmts_bits)
                                HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "invalid fixed array creation parameter")

                            /* Set the chunk operations */
                            mesg->storage.u.chunk.ops = H5D_COPS_FARRAY;
                            break;

                        case H5D_CHUNK_IDX_EARRAY:
                            /* Extensible array creation parameters */
                            mesg->u.chunk.u.earray.cparam.max_nelmts_bits = *p++;
                            if(0 == mesg->u.chunk.u.earray.cparam.max_nelmts_bits)
                                HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "invalid extensible array creation parameter")
                            mesg->u.chunk.u.earray.cparam.idx_blk_elmts = *p++;
                            if(0 == mesg->u.chunk.u.earray.cparam.idx_blk_elmts)
                                HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "invalid extensible array creation parameter")
                            mesg->u.chunk.u.earray.cparam.sup_blk_min_data_ptrs = *p++;
                            if(0 == mesg->u.chunk.u.earray.cparam.sup_blk_min_data_ptrs)
                                HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "invalid extensible array creation parameter")
                            mesg->u.chunk.u.earray.cparam.data_blk_min_elmts = *p++;
                            if(0 == mesg->u.chunk.u.earray.cparam.data_blk_min_elmts)
                                HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "invalid extensible array creation parameter")
                            mesg->u.chunk.u.earray.cparam.max_dblk_page_nelmts_bits = *p++;
                            if(0 == mesg->u.chunk.u.earray.cparam.max_dblk_page_nelmts_bits)
                                HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "invalid extensible array creation parameter")

                            /* Set the chunk operations */
                            mesg->storage.u.chunk.ops = H5D_COPS_EARRAY;
                            break;

                        case H5D_CHUNK_IDX_BT2:       /* v2 B-tree index */
                            UINT32DECODE(p, mesg->u.chunk.u.btree2.cparam.node_size);
                            mesg->u.chunk.u.btree2.cparam.split_percent = *p++;
                            mesg->u.chunk.u.btree2.cparam.merge_percent = *p++;

                            /* Set the chunk operations */
                            mesg->storage.u.chunk.ops = H5D_COPS_BT2;
                            break;

                        case H5D_CHUNK_IDX_NTYPES:
                        default:
                            HGOTO_ERROR(H5E_OHDR, H5E_BADVALUE, NULL, "Invalid chunk index type")
                    } /* end switch */

                    /* Chunk index address */
                    H5F_addr_decode(f, &p, &(mesg->storage.u.chunk.idx_addr));
                } /* end else */

                /* Set the layout operations */
                mesg->ops = H5D_LOPS_CHUNK;
                break;

            case H5D_VIRTUAL:
                /* Check version */
                if(mesg->version < H5O_LAYOUT_VERSION_4)
                    HGOTO_ERROR(H5E_OHDR, H5E_VERSION, NULL, "invalid layout version with virtual layout")
                
                /* Heap information */
                H5F_addr_decode(f, &p, &(mesg->storage.u.virt.serial_list_hobjid.addr));
                UINT32DECODE(p, mesg->storage.u.virt.serial_list_hobjid.idx);

                /* Initialize other fields */
                mesg->storage.u.virt.list_nused = 0;
                mesg->storage.u.virt.list = NULL;
                mesg->storage.u.virt.list_nalloc = 0;
                mesg->storage.u.virt.view = H5D_VDS_ERROR;
                mesg->storage.u.virt.printf_gap = HSIZE_UNDEF;
                mesg->storage.u.virt.source_fapl = -1;
                mesg->storage.u.virt.source_dapl = -1;
                mesg->storage.u.virt.init = FALSE;

                /* Decode heap block if it exists */
                if(mesg->storage.u.virt.serial_list_hobjid.addr != HADDR_UNDEF) {
                    const uint8_t *heap_block_p;
                    uint8_t heap_vers;
                    size_t block_size = 0;
                    size_t tmp_size;
                    hsize_t tmp_hsize;
                    uint32_t stored_chksum;
                    uint32_t computed_chksum;
                    size_t i;

                    /* Read heap */
                    if(NULL == (heap_block = (uint8_t *)H5HG_read(f, &(mesg->storage.u.virt.serial_list_hobjid), NULL, &block_size)))
                        HGOTO_ERROR(H5E_OHDR, H5E_READERROR, NULL, "Unable to read global heap block")

                    heap_block_p = (const uint8_t *)heap_block;

                    /* Decode the version number of the heap block encoding */
                    heap_vers = (uint8_t)*heap_block_p++;
                    if((uint8_t)H5O_LAYOUT_VDS_GH_ENC_VERS != heap_vers)
                        HGOTO_ERROR(H5E_OHDR, H5E_VERSION, NULL, "bad version # of encoded VDS heap information, expected %u, got %u", (unsigned)H5O_LAYOUT_VDS_GH_ENC_VERS, (unsigned)heap_vers)

                    /* Number of entries */
                    H5F_DECODE_LENGTH(f, heap_block_p, tmp_hsize)

                    /* Allocate entry list */
                    if(NULL == (mesg->storage.u.virt.list = (H5O_storage_virtual_ent_t *)H5MM_calloc((size_t)tmp_hsize * sizeof(H5O_storage_virtual_ent_t))))
                        HGOTO_ERROR(H5E_OHDR, H5E_RESOURCE, NULL, "unable to allocate heap block")
                    mesg->storage.u.virt.list_nalloc = (size_t)tmp_hsize;
                    mesg->storage.u.virt.list_nused = (size_t)tmp_hsize;

                    /* Decode each entry */
                    for(i = 0; i < mesg->storage.u.virt.list_nused; i++) {
                        /* Source file name */
                        tmp_size = HDstrlen((const char *)heap_block_p) + 1;
                        if(NULL == (mesg->storage.u.virt.list[i].source_file_name = (char *)H5MM_malloc(tmp_size)))
                            HGOTO_ERROR(H5E_OHDR, H5E_RESOURCE, NULL, "unable to allocate memory for source file name")
                        (void)HDmemcpy(mesg->storage.u.virt.list[i].source_file_name, heap_block_p, tmp_size);
                        heap_block_p += tmp_size;

                        /* Source dataset name */
                        tmp_size = HDstrlen((const char *)heap_block_p) + 1;
                        if(NULL == (mesg->storage.u.virt.list[i].source_dset_name = (char *)H5MM_malloc(tmp_size)))
                            HGOTO_ERROR(H5E_OHDR, H5E_RESOURCE, NULL, "unable to allocate memory for source dataset name")
                        (void)HDmemcpy(mesg->storage.u.virt.list[i].source_dset_name, heap_block_p, tmp_size);
                        heap_block_p += tmp_size;

                        /* Source selection */
                        if(H5S_SELECT_DESERIALIZE(&mesg->storage.u.virt.list[i].source_select, &heap_block_p) < 0)
                            HGOTO_ERROR(H5E_OHDR, H5E_CANTDECODE, NULL, "can't decode source space selection")

                        /* Virtual selection */
                        if(H5S_SELECT_DESERIALIZE(&mesg->storage.u.virt.list[i].source_dset.virtual_select, &heap_block_p) < 0)
                            HGOTO_ERROR(H5E_OHDR, H5E_CANTDECODE, NULL, "can't decode virtual space selection")

                        /* Parse source file and dataset names for "printf"
                         * style format specifiers */
                        if(H5D_virtual_parse_source_name(mesg->storage.u.virt.list[i].source_file_name, &mesg->storage.u.virt.list[i].parsed_source_file_name, &mesg->storage.u.virt.list[i].psfn_static_strlen, &mesg->storage.u.virt.list[i].psfn_nsubs) < 0)
                            HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, NULL, "can't parse source file name")
                        if(H5D_virtual_parse_source_name(mesg->storage.u.virt.list[i].source_dset_name, &mesg->storage.u.virt.list[i].parsed_source_dset_name, &mesg->storage.u.virt.list[i].psdn_static_strlen, &mesg->storage.u.virt.list[i].psdn_nsubs) < 0)
                            HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, NULL, "can't parse source dataset name")

                        /* Set source names in source_dset struct */
                        if((mesg->storage.u.virt.list[i].psfn_nsubs == 0)
                                && (mesg->storage.u.virt.list[i].psdn_nsubs == 0)) {
                            if(mesg->storage.u.virt.list[i].parsed_source_file_name)
                                mesg->storage.u.virt.list[i].source_dset.file_name = mesg->storage.u.virt.list[i].parsed_source_file_name->name_segment;
                            else
                                mesg->storage.u.virt.list[i].source_dset.file_name = mesg->storage.u.virt.list[i].source_file_name;
                            if(mesg->storage.u.virt.list[i].parsed_source_dset_name)
                                mesg->storage.u.virt.list[i].source_dset.dset_name = mesg->storage.u.virt.list[i].parsed_source_dset_name->name_segment;
                            else
                                mesg->storage.u.virt.list[i].source_dset.dset_name = mesg->storage.u.virt.list[i].source_dset_name;
                        } /* end if */

                        /* unlim_dim fields */
                        mesg->storage.u.virt.list[i].unlim_dim_source = H5S_get_select_unlim_dim(mesg->storage.u.virt.list[i].source_select);
                        mesg->storage.u.virt.list[i].unlim_dim_virtual = H5S_get_select_unlim_dim(mesg->storage.u.virt.list[i].source_dset.virtual_select);
                        mesg->storage.u.virt.list[i].unlim_extent_source = HSIZE_UNDEF;
                        mesg->storage.u.virt.list[i].unlim_extent_virtual = HSIZE_UNDEF;
                        mesg->storage.u.virt.list[i].clip_size_source = HSIZE_UNDEF;
                        mesg->storage.u.virt.list[i].clip_size_virtual = HSIZE_UNDEF;

                        /* Clipped selections */
                        if(mesg->storage.u.virt.list[i].unlim_dim_virtual < 0) {
                            mesg->storage.u.virt.list[i].source_dset.clipped_source_select = mesg->storage.u.virt.list[i].source_select;
                            mesg->storage.u.virt.list[i].source_dset.clipped_virtual_select = mesg->storage.u.virt.list[i].source_dset.virtual_select;
                        } /* end if */

                        /* Check mapping for validity (do both pre and post
                         * checks here, since we had to allocate the entry list
                         * before decoding the selections anyways) */
                        if(H5D_virtual_check_mapping_pre(mesg->storage.u.virt.list[i].source_dset.virtual_select, mesg->storage.u.virt.list[i].source_select, H5O_VIRTUAL_STATUS_INVALID) < 0)
                            HGOTO_ERROR(H5E_OHDR, H5E_BADVALUE, NULL, "invalid mapping selections")
                         if(H5D_virtual_check_mapping_post(&mesg->storage.u.virt.list[i]) < 0)
                            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid mapping entry")

                        /* Update min_dims */
                        if(H5D_virtual_update_min_dims(mesg, i) < 0)
                            HGOTO_ERROR(H5E_OHDR, H5E_CANTINIT, NULL, "unable to update virtual dataset minimum dimensions")
                    } /* end for */

                    /* Read stored checksum */
                    UINT32DECODE(heap_block_p, stored_chksum)

                    /* Compute checksum */
                    computed_chksum = H5_checksum_metadata(heap_block, block_size - (size_t)4, 0);

                    /* Verify checksum */
                    if(stored_chksum != computed_chksum)
                        HGOTO_ERROR(H5E_OHDR, H5E_BADVALUE, NULL, "incorrect metadata checksum for global heap block")

                    /* Verify that the heap block size is correct */
                    if((size_t)(heap_block_p - heap_block) != block_size)
                        HGOTO_ERROR(H5E_OHDR, H5E_BADVALUE, NULL, "incorrect heap block size")
                } /* end if */

                /* Set the layout operations */
                mesg->ops = H5D_LOPS_VIRTUAL;

                break;

            case H5D_LAYOUT_ERROR:
            case H5D_NLAYOUTS:
            default:
                HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "Invalid layout class")
        } /* end switch */
    } /* end else */

    /* Set return value */
    ret_value = mesg;

done:
    if(ret_value == NULL)
        if(mesg) {
            if(mesg->type == H5D_VIRTUAL)
                if(H5D__virtual_reset_layout(mesg) < 0)
                    HDONE_ERROR(H5E_OHDR, H5E_CANTFREE, NULL, "unable to reset virtual layout")
            mesg = H5FL_FREE(H5O_layout_t, mesg);
        } /* end if */

    heap_block = (uint8_t *)H5MM_xfree(heap_block);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O__layout_decode() */


/*-------------------------------------------------------------------------
 * Function:    H5O__layout_encode
 *
 * Purpose:     Encodes a message.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, October  8, 1997
 *
 * Note:
 *      Quincey Koziol, 2004-5-21
 *      We write out version 3 messages by default now.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O__layout_encode(H5F_t *f, hbool_t H5_ATTR_UNUSED disable_shared, uint8_t *p, const void *_mesg)
{
    const H5O_layout_t      *mesg = (const H5O_layout_t *) _mesg;
    uint8_t                 *heap_block = NULL;
    size_t                  *str_size = NULL;
    unsigned                u;
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_STATIC

    /* check args */
    HDassert(f);
    HDassert(mesg);
    HDassert(p);

    /* Message version */
    *p++ = (uint8_t)((mesg->version < H5O_LAYOUT_VERSION_3) ?
             H5O_LAYOUT_VERSION_3 : mesg->version);

    /* Layout class */
    *p++ = mesg->type;

    /* Write out layout class specific information */
    switch(mesg->type) {
        case H5D_COMPACT:
            /* Size of raw data */
            UINT16ENCODE(p, mesg->storage.u.compact.size);

            /* Raw data */
            if(mesg->storage.u.compact.size > 0) {
                if(mesg->storage.u.compact.buf)
                    HDmemcpy(p, mesg->storage.u.compact.buf, mesg->storage.u.compact.size);
                else
                    HDmemset(p, 0, mesg->storage.u.compact.size);
                p += mesg->storage.u.compact.size;
            } /* end if */
            break;

        case H5D_CONTIGUOUS:
            /* Contiguous storage address */
            H5F_addr_encode(f, &p, mesg->storage.u.contig.addr);

            /* Contiguous storage size */
            H5F_ENCODE_LENGTH(f, p, mesg->storage.u.contig.size);
            break;

        case H5D_CHUNKED:
            if(mesg->version < H5O_LAYOUT_VERSION_4) {
                /* Number of dimensions */
                HDassert(mesg->u.chunk.ndims > 0 && mesg->u.chunk.ndims <= H5O_LAYOUT_NDIMS);
                *p++ = (uint8_t)mesg->u.chunk.ndims;

                /* B-tree address */
                H5F_addr_encode(f, &p, mesg->storage.u.chunk.idx_addr);

                /* Dimension sizes */
                for(u = 0; u < mesg->u.chunk.ndims; u++)
                    UINT32ENCODE(p, mesg->u.chunk.dim[u]);
            } /* end if */
            else {
                /* Chunk feature flags */
                *p++ = mesg->u.chunk.flags;

                /* Number of dimensions */
                HDassert(mesg->u.chunk.ndims > 0 && mesg->u.chunk.ndims <= H5O_LAYOUT_NDIMS);
                *p++ = (uint8_t)mesg->u.chunk.ndims;

                /* Encoded # of bytes for each chunk dimension */
                HDassert(mesg->u.chunk.enc_bytes_per_dim > 0 && mesg->u.chunk.enc_bytes_per_dim <= 8);
                *p++ = (uint8_t)mesg->u.chunk.enc_bytes_per_dim;

                /* Dimension sizes */
                for(u = 0; u < mesg->u.chunk.ndims; u++)
                    UINT64ENCODE_VAR(p, mesg->u.chunk.dim[u], mesg->u.chunk.enc_bytes_per_dim);

                /* Chunk index type */
                *p++ = (uint8_t)mesg->u.chunk.idx_type;

                switch(mesg->u.chunk.idx_type) {
                    case H5D_CHUNK_IDX_BTREE:
                        HGOTO_ERROR(H5E_OHDR, H5E_BADVALUE, FAIL, "v1 B-tree index type should never be in a v4 layout message")
                        break;

                    case H5D_CHUNK_IDX_NONE:       /* Implicit */
                        break;

                    case H5D_CHUNK_IDX_SINGLE:     /* Single Chunk */
                        /* Filter information */
                        if(mesg->u.chunk.flags & H5O_LAYOUT_CHUNK_SINGLE_INDEX_WITH_FILTER) {
                            H5F_ENCODE_LENGTH(f, p, mesg->storage.u.chunk.u.single.nbytes);
                            UINT32ENCODE(p, mesg->storage.u.chunk.u.single.filter_mask);
                        } /* end if */
                        break;

                    case H5D_CHUNK_IDX_FARRAY:
                        /* Fixed array creation parameters */
                        *p++ = mesg->u.chunk.u.farray.cparam.max_dblk_page_nelmts_bits;
                        break;

                    case H5D_CHUNK_IDX_EARRAY:
                        /* Extensible array creation parameters */
                        *p++ = mesg->u.chunk.u.earray.cparam.max_nelmts_bits;
                        *p++ = mesg->u.chunk.u.earray.cparam.idx_blk_elmts;
                        *p++ = mesg->u.chunk.u.earray.cparam.sup_blk_min_data_ptrs;
                        *p++ = mesg->u.chunk.u.earray.cparam.data_blk_min_elmts;
                        *p++ = mesg->u.chunk.u.earray.cparam.max_dblk_page_nelmts_bits;
                        break;

                    case H5D_CHUNK_IDX_BT2:       /* v2 B-tree index */
                        UINT32ENCODE(p, mesg->u.chunk.u.btree2.cparam.node_size);
                        *p++ = mesg->u.chunk.u.btree2.cparam.split_percent;
                        *p++ = mesg->u.chunk.u.btree2.cparam.merge_percent;
                        break;

                    case H5D_CHUNK_IDX_NTYPES:
                    default:
                        HGOTO_ERROR(H5E_OHDR, H5E_CANTENCODE, FAIL, "Invalid chunk index type")
                } /* end switch */

                /*
                 * Implicit index: Address of the chunks
                 * Single chunk index: address of the single chunk
                 * Other indexes: chunk index address
                 */
                H5F_addr_encode(f, &p, mesg->storage.u.chunk.idx_addr);
            } /* end else */
            break;

        case H5D_VIRTUAL:
            /* Encode heap ID for VDS info */
            H5F_addr_encode(f, &p, mesg->storage.u.virt.serial_list_hobjid.addr);
            UINT32ENCODE(p, mesg->storage.u.virt.serial_list_hobjid.idx);
            break;

        case H5D_LAYOUT_ERROR:
        case H5D_NLAYOUTS:
        default:
            HGOTO_ERROR(H5E_OHDR, H5E_CANTENCODE, FAIL, "Invalid layout class")
    } /* end switch */

done:

    heap_block = (uint8_t *)H5MM_xfree(heap_block);
    str_size = (size_t *)H5MM_xfree(str_size);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O__layout_encode() */


/*-------------------------------------------------------------------------
 * Function:    H5O__layout_copy
 *
 * Purpose:     Copies a message from _MESG to _DEST, allocating _DEST if
 *              necessary.
 *
 * Return:      Success:        Ptr to _DEST
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Wednesday, October  8, 1997
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O__layout_copy(const void *_mesg, void *_dest)
{
    const H5O_layout_t     *mesg = (const H5O_layout_t *) _mesg;
    H5O_layout_t           *dest = (H5O_layout_t *) _dest;
    void                   *ret_value = NULL;   /* Return value */

    FUNC_ENTER_STATIC

    /* check args */
    HDassert(mesg);

    /* Allocate destination message, if necessary */
    if(!dest && NULL == (dest = H5FL_MALLOC(H5O_layout_t)))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTALLOC, NULL, "layout message allocation failed")

    /* copy */
    *dest = *mesg;

    /* Special actions for each type of layout */
    switch(mesg->type) {
        case H5D_COMPACT:
            /* Deep copy the buffer for compact datasets also */
            if(mesg->storage.u.compact.size > 0) {
                /* Sanity check */
                HDassert(mesg->storage.u.compact.buf);

                /* Allocate memory for the raw data */
                if(NULL == (dest->storage.u.compact.buf = H5MM_malloc(dest->storage.u.compact.size)))
                    HGOTO_ERROR(H5E_OHDR, H5E_NOSPACE, NULL, "unable to allocate memory for compact dataset")

                /* Copy over the raw data */
                HDmemcpy(dest->storage.u.compact.buf, mesg->storage.u.compact.buf, dest->storage.u.compact.size);
            } /* end if */
            else
                HDassert(dest->storage.u.compact.buf == NULL);
            break;

        case H5D_CONTIGUOUS:
            /* Nothing required */
            break;

        case H5D_CHUNKED:
            /* Reset the pointer of the chunked storage index but not the address */
            if(dest->storage.u.chunk.ops)
                H5D_chunk_idx_reset(&dest->storage.u.chunk, FALSE);
            break;

        case H5D_VIRTUAL:
            if(H5D__virtual_copy_layout(dest) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, NULL, "unable to copy virtual layout")
            break;

        case H5D_LAYOUT_ERROR:
        case H5D_NLAYOUTS:
        default:
            HGOTO_ERROR(H5E_OHDR, H5E_CANTENCODE, NULL, "Invalid layout class")
    } /* end switch */

    /* Set return value */
    ret_value = dest;

done:
    if(ret_value == NULL)
        if(NULL == _dest)
            dest = H5FL_FREE(H5O_layout_t, dest);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O__layout_copy() */


/*-------------------------------------------------------------------------
 * Function:    H5O__layout_size
 *
 * Purpose:     Returns the size of the raw message in bytes.  If it's
 *              compact dataset, the data part is also included.
 *              This function doesn't take into account message alignment.
 *
 * Return:      Success:        Message data size in bytes
 *
 *              Failure:        0
 *
 * Programmer:  Robb Matzke
 *              Wednesday, October  8, 1997
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5O__layout_size(const H5F_t *f, hbool_t H5_ATTR_UNUSED disable_shared, const void *_mesg)
{
    const H5O_layout_t     *mesg = (const H5O_layout_t *) _mesg;
    size_t                  ret_value = 0;      /* Return value */

    FUNC_ENTER_STATIC_NOERR

    /* check args */
    HDassert(f);
    HDassert(mesg);

    /* Compute serialized size */
    /* (including possibly compact data) */
    ret_value = H5D__layout_meta_size(f, mesg, TRUE);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O__layout_size() */


/*-------------------------------------------------------------------------
 * Function:    H5O__layout_reset
 *
 * Purpose:     Frees resources within a data type message, but doesn't free
 *              the message itself.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Friday, September 13, 2002
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O__layout_reset(void *_mesg)
{
    H5O_layout_t     *mesg = (H5O_layout_t *)_mesg;
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_STATIC

    if(mesg) {
        /* Free the compact storage buffer */
        if(H5D_COMPACT == mesg->type)
            mesg->storage.u.compact.buf = H5MM_xfree(mesg->storage.u.compact.buf);
        else if(H5D_VIRTUAL == mesg->type)
            /* Free the virtual entry list */
            if(H5D__virtual_reset_layout(mesg) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTFREE, FAIL, "unable to reset virtual layout")

        /* Reset the message */
        mesg->type = H5D_CONTIGUOUS;
        mesg->version = H5O_LAYOUT_VERSION_DEFAULT;
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O__layout_reset() */


/*-------------------------------------------------------------------------
 * Function:	H5O__layout_free
 *
 * Purpose:	Free's the message
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Saturday, March 11, 2000
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O__layout_free(void *_mesg)
{
    H5O_layout_t     *mesg = (H5O_layout_t *) _mesg;

    FUNC_ENTER_STATIC_NOERR

    HDassert(mesg);

    /* Free resources within the message */
    H5O__layout_reset(mesg);

    (void)H5FL_FREE(H5O_layout_t, mesg);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O__layout_free() */


/*-------------------------------------------------------------------------
 * Function:    H5O__layout_delete
 *
 * Purpose:     Free file space referenced by message
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, March 19, 2003
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O__layout_delete(H5F_t *f, H5O_t *open_oh, void *_mesg)
{
    H5O_layout_t *mesg = (H5O_layout_t *) _mesg;
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_STATIC

    /* check args */
    HDassert(f);
    HDassert(open_oh);
    HDassert(mesg);

    /* Perform different actions, depending on the type of storage */
    switch(mesg->type) {
        case H5D_COMPACT:       /* Compact data storage */
            /* Nothing required */
            break;

        case H5D_CONTIGUOUS:    /* Contiguous block on disk */
            /* Free the file space for the raw data */
            if(H5D__contig_delete(f, &mesg->storage) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTFREE, FAIL, "unable to free raw data")
            break;

        case H5D_CHUNKED:       /* Chunked blocks on disk */
            /* Free the file space for the index & chunk raw data */
            if(H5D__chunk_delete(f, open_oh, &mesg->storage) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTFREE, FAIL, "unable to free raw data")
            break;

        case H5D_VIRTUAL:       /* Virtual dataset */
            /* Free the file space virtual dataset */
            if(H5D__virtual_delete(f, &mesg->storage) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTFREE, FAIL, "unable to free raw data")
            break;

        case H5D_LAYOUT_ERROR:
        case H5D_NLAYOUTS:
        default:
            HGOTO_ERROR(H5E_OHDR, H5E_BADTYPE, FAIL, "not valid storage type")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O__layout_delete() */


/*-------------------------------------------------------------------------
 * Function:    H5O__layout_pre_copy_file
 *
 * Purpose:     Perform any necessary actions before copying message between
 *              files.
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 * Programmer:  Vailin Choi; Dec 2017
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O__layout_pre_copy_file(H5F_t H5_ATTR_UNUSED *file_src, const void *mesg_src,
    hbool_t H5_ATTR_UNUSED *deleted, const H5O_copy_t *cpy_info, void H5_ATTR_UNUSED *udata)
{
    const H5O_layout_t *layout_src = (const H5O_layout_t *)mesg_src;  /* Source layout */
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(cpy_info);
    HDassert(cpy_info->file_dst);

    /* Check to ensure that the version of the message to be copied does not exceed
       the message version allowed by the destination file's high bound */
    if(layout_src->version > H5O_layout_ver_bounds[H5F_HIGH_BOUND(cpy_info->file_dst)])
        HGOTO_ERROR(H5E_OHDR, H5E_BADRANGE, FAIL, "layout message version out of bounds")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O__layout_pre_copy_file() */


/*-------------------------------------------------------------------------
 * Function:    H5O__layout_copy_file
 *
 * Purpose:     Copies a message from _MESG to _DEST in file
 *
 * Return:      Success:        Ptr to _DEST
 *
 *              Failure:        NULL
 *
 * Programmer:  Peter Cao
 *              July 23, 2005
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O__layout_copy_file(H5F_t *file_src, void *mesg_src, H5F_t *file_dst,
    hbool_t H5_ATTR_UNUSED *recompute_size, unsigned H5_ATTR_UNUSED *mesg_flags,
    H5O_copy_t *cpy_info, void *_udata)
{
    H5D_copy_file_ud_t *udata = (H5D_copy_file_ud_t *)_udata;   /* Dataset copying user data */
    H5O_layout_t       *layout_src = (H5O_layout_t *) mesg_src;
    H5O_layout_t       *layout_dst = NULL;
    hbool_t             copied = FALSE;         /* Whether the data was copied */
    void               *ret_value = NULL;       /* Return value */

    FUNC_ENTER_STATIC

    /* check args */
    HDassert(file_src);
    HDassert(layout_src);
    HDassert(file_dst);

    /* Copy the layout information */
    if(NULL == (layout_dst = (H5O_layout_t *)H5O__layout_copy(layout_src, NULL)))
        HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, NULL, "unable to copy layout")

    /* Copy the layout type specific information */
    switch(layout_src->type) {
        case H5D_COMPACT:
            if(layout_src->storage.u.compact.buf) {
                /* copy compact raw data */
                if(H5D__compact_copy(file_src, &layout_src->storage.u.compact, file_dst, &layout_dst->storage.u.compact, udata->src_dtype, cpy_info) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, NULL, "unable to copy chunked storage")
                copied = TRUE;
            } /* end if */
            break;

        case H5D_CONTIGUOUS:
            /* Compute the size of the contiguous storage for versions of the
             * layout message less than version 3 because versions 1 & 2 would
             * truncate the dimension sizes to 32-bits of information. - QAK 5/26/04
             */
            if(layout_src->version < H5O_LAYOUT_VERSION_3)
                layout_dst->storage.u.contig.size = H5S_extent_nelem(udata->src_space_extent) *
                                        H5T_get_size(udata->src_dtype);

            if(H5D__contig_is_space_alloc(&layout_src->storage)
                    || (cpy_info->shared_fo && H5D__contig_is_data_cached((const H5D_shared_t *)cpy_info->shared_fo))) {
                /* copy contiguous raw data */
                if(H5D__contig_copy(file_src, &layout_src->storage.u.contig, file_dst, &layout_dst->storage.u.contig, udata->src_dtype, cpy_info) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, NULL, "unable to copy contiguous storage")
                copied = TRUE;
            } /* end if */
            break;

        case H5D_CHUNKED:
            if(H5D__chunk_is_space_alloc(&layout_src->storage)
                    || (cpy_info->shared_fo && H5D__chunk_is_data_cached((const H5D_shared_t *)cpy_info->shared_fo))) {
                /* Create chunked layout */
                if(H5D__chunk_copy(file_src, &layout_src->storage.u.chunk, &layout_src->u.chunk, file_dst, &layout_dst->storage.u.chunk, udata->src_space_extent, udata->src_dtype, udata->common.src_pline, cpy_info) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, NULL, "unable to copy chunked storage")
                copied = TRUE;
            } /* end if */
            break;

        case H5D_VIRTUAL:
            /* Copy virtual layout.  Always copy so the memory fields get copied
             * properly. */
            if(H5D__virtual_copy(file_dst, layout_dst) < 0)
                HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, NULL, "unable to copy virtual storage")
            break;

        case H5D_LAYOUT_ERROR:
        case H5D_NLAYOUTS:
        default:
            HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, NULL, "Invalid layout class")
    } /* end switch */

    /* Check if copy routine was invoked (which frees the source datatype) */
    if(copied)
        udata->src_dtype = NULL;

    /* Set return value */
    ret_value = layout_dst;

done:
    if(!ret_value)
	if(layout_dst)
	    layout_dst = H5FL_FREE(H5O_layout_t, layout_dst);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O__layout_copy_file() */


/*-------------------------------------------------------------------------
 * Function:    H5O__layout_debug
 *
 * Purpose:     Prints debugging info for a message.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, October  8, 1997
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O__layout_debug(H5F_t H5_ATTR_UNUSED *f, const void *_mesg, FILE *stream,
    int indent, int fwidth)
{
    const H5O_layout_t     *mesg = (const H5O_layout_t *) _mesg;
    size_t                  u;

    FUNC_ENTER_STATIC_NOERR

    /* check args */
    HDassert(f);
    HDassert(mesg);
    HDassert(stream);
    HDassert(indent >= 0);
    HDassert(fwidth >= 0);

    HDfprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
              "Version:", mesg->version);
    switch(mesg->type) {
        case H5D_CHUNKED:
            HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
                      "Type:", "Chunked");

            /* Chunk # of dims & size */
            HDfprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
                      "Number of dimensions:",
                      (unsigned long)(mesg->u.chunk.ndims));
            HDfprintf(stream, "%*s%-*s {", indent, "", fwidth, "Size:");
            for(u = 0; u < (size_t)mesg->u.chunk.ndims; u++)
                HDfprintf(stream, "%s%lu", u ? ", " : "", (unsigned long)(mesg->u.chunk.dim[u]));
            HDfprintf(stream, "}\n");

            /* Index information */
            switch(mesg->u.chunk.idx_type) {
                case H5D_CHUNK_IDX_BTREE:
                    HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
                              "Index Type:", "v1 B-tree");
                    break;

                case H5D_CHUNK_IDX_NONE:
                    HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
                              "Index Type:", "Implicit");
                    break;

                case H5D_CHUNK_IDX_SINGLE:
                    HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
                              "Index Type:", "Single Chunk");
                    break;

                case H5D_CHUNK_IDX_FARRAY:
                    HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
                              "Index Type:", "Fixed Array");
                    /* (Should print the fixed array creation parameters) */
                    break;

                case H5D_CHUNK_IDX_EARRAY:
                    HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
                              "Index Type:", "Extensible Array");
                    /* (Should print the extensible array creation parameters) */
                    break;

                case H5D_CHUNK_IDX_BT2:
                    HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
                              "Index Type:", "v2 B-tree");
                    /* (Should print the v2-Btree creation parameters) */
                    break;

                case H5D_CHUNK_IDX_NTYPES:
                default:
                    HDfprintf(stream, "%*s%-*s %s (%u)\n", indent, "", fwidth,
                              "Index Type:", "Unknown", (unsigned)mesg->u.chunk.idx_type);
                    break;
            } /* end switch */
            HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
                      "Index address:", mesg->storage.u.chunk.idx_addr);
            break;

        case H5D_CONTIGUOUS:
            HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
                      "Type:", "Contiguous");
            HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
                      "Data address:", mesg->storage.u.contig.addr);
            HDfprintf(stream, "%*s%-*s %Hu\n", indent, "", fwidth,
                      "Data Size:", mesg->storage.u.contig.size);
            break;

        case H5D_COMPACT:
            HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
                      "Type:", "Compact");
            HDfprintf(stream, "%*s%-*s %Zu\n", indent, "", fwidth,
                      "Data Size:", mesg->storage.u.compact.size);
            break;

        case H5D_VIRTUAL:
            HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
                      "Type:", "Virtual");
            HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
                      "Global heap address:", mesg->storage.u.virt.serial_list_hobjid.addr);
            HDfprintf(stream, "%*s%-*s %Zu\n", indent, "", fwidth,
                      "Global heap index:", mesg->storage.u.virt.serial_list_hobjid.idx);
            for(u = 0; u < mesg->storage.u.virt.list_nused; u++) {
                HDfprintf(stream, "%*sMapping %u:\n", indent, "", u);
                HDfprintf(stream, "%*s%-*s %s\n", indent + 3, "", fwidth - 3,
                      "Virtual selection:", "<Not yet implemented>");
                HDfprintf(stream, "%*s%-*s %s\n", indent + 3, "", fwidth - 3,
                      "Source file name:", mesg->storage.u.virt.list[u].source_file_name);
                HDfprintf(stream, "%*s%-*s %s\n", indent + 3, "", fwidth - 3,
                      "Source dataset name:", mesg->storage.u.virt.list[u].source_dset_name);
                HDfprintf(stream, "%*s%-*s %s\n", indent + 3, "", fwidth - 3,
                      "Source selection:", "<Not yet implemented>");
            } /* end for */
            break;

        case H5D_LAYOUT_ERROR:
        case H5D_NLAYOUTS:
        default:
            HDfprintf(stream, "%*s%-*s %s (%u)\n", indent, "", fwidth,
                      "Type:", "Unknown", (unsigned)mesg->type);
            break;
    } /* end switch */

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O__layout_debug() */

