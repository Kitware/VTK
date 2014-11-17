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

#define H5O_PACKAGE		/*suppress error about including H5Opkg	  */
#define H5T_PACKAGE		/*prevent warning from including H5Tpkg   */

#include "H5private.h"		/* Generic Functions			*/
#include "H5Dprivate.h"		/* Datasets				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fprivate.h"		/* Files				*/
#include "H5FLprivate.h"	/* Free Lists				*/
#include "H5Gprivate.h"		/* Groups				*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Opkg.h"             /* Object headers			*/
#include "H5Tpkg.h"		/* Datatypes				*/
#include "H5VMprivate.h"		/* Vectors and arrays 			*/


/* PRIVATE PROTOTYPES */
static herr_t H5O_dtype_encode(H5F_t *f, uint8_t *p, const void *mesg);
static void *H5O_dtype_decode(H5F_t *f, hid_t dxpl_id, H5O_t *open_oh,
    unsigned mesg_flags, unsigned *ioflags, const uint8_t *p);
static void *H5O_dtype_copy(const void *_mesg, void *_dest);
static size_t H5O_dtype_size(const H5F_t *f, const void *_mesg);
static herr_t H5O_dtype_reset(void *_mesg);
static herr_t H5O_dtype_free(void *_mesg);
static herr_t H5O_dtype_set_share(void *_mesg, const H5O_shared_t *sh);
static htri_t H5O_dtype_can_share(const void *_mesg);
static herr_t H5O_dtype_pre_copy_file(H5F_t *file_src, const void *mesg_src,
    hbool_t *deleted, const H5O_copy_t *cpy_info, void *_udata);
static void *H5O_dtype_copy_file(H5F_t *file_src, const H5O_msg_class_t *mesg_type,
    void *native_src, H5F_t *file_dst, hbool_t *recompute_size,
    H5O_copy_t *cpy_info, void *udata, hid_t dxpl_id);
static herr_t H5O_dtype_shared_post_copy_upd(const H5O_loc_t *src_oloc,
    const void *mesg_src, H5O_loc_t *dst_oloc, void *mesg_dst, hid_t dxpl_id,
    H5O_copy_t *cpy_info);

static herr_t H5O_dtype_debug(H5F_t *f, hid_t dxpl_id, const void *_mesg,
    FILE * stream, int indent, int fwidth);

/* Set up & include shared message "interface" info */
#define H5O_SHARED_TYPE			H5O_MSG_DTYPE
#define H5O_SHARED_DECODE		H5O_dtype_shared_decode
#define H5O_SHARED_DECODE_REAL		H5O_dtype_decode
#define H5O_SHARED_ENCODE		H5O_dtype_shared_encode
#define H5O_SHARED_ENCODE_REAL		H5O_dtype_encode
#define H5O_SHARED_SIZE			H5O_dtype_shared_size
#define H5O_SHARED_SIZE_REAL		H5O_dtype_size
#define H5O_SHARED_DELETE		H5O_dtype_shared_delete
#undef H5O_SHARED_DELETE_REAL
#define H5O_SHARED_LINK			H5O_dtype_shared_link
#undef H5O_SHARED_LINK_REAL
#define H5O_SHARED_COPY_FILE		H5O_dtype_shared_copy_file
#define H5O_SHARED_COPY_FILE_REAL	H5O_dtype_copy_file
#define H5O_SHARED_POST_COPY_FILE	H5O_dtype_shared_post_copy_file
#undef  H5O_SHARED_POST_COPY_FILE_REAL
#define H5O_SHARED_POST_COPY_FILE_UPD   H5O_dtype_shared_post_copy_upd
#define H5O_SHARED_DEBUG		H5O_dtype_shared_debug
#define H5O_SHARED_DEBUG_REAL		H5O_dtype_debug
#include "H5Oshared.h"			/* Shared Object Header Message Callbacks */

/* Macros to check for the proper version of a datatype */
#ifdef H5_STRICT_FORMAT_CHECKS
/* If the version is too low, give an error.  No error if nochange is set
 * because in that case we are either debugging or deleting the object header */
#define H5O_DTYPE_CHECK_VERSION(DT, VERS, MIN_VERS, IOF, CLASS, ERR)           \
    if(((VERS) < (MIN_VERS)) && !(*(IOF) & H5O_DECODEIO_NOCHANGE))             \
        HGOTO_ERROR(H5E_DATATYPE, H5E_VERSION, ERR, "incorrect " CLASS " datatype version")
#else /* H5_STRICT_FORMAT_CHECKS */
/* If the version is too low and we are allowed to change the message, upgrade
 * it and mark the object header as dirty */
#define H5O_DTYPE_CHECK_VERSION(DT, VERS, MIN_VERS, IOF, CLASS, ERR)           \
    if(((VERS) < (MIN_VERS)) && !(*(IOF) & H5O_DECODEIO_NOCHANGE)) {           \
        (VERS) = (MIN_VERS);                                                   \
        if(H5T__upgrade_version((DT), (VERS)) < 0)                              \
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTSET, FAIL, "can't upgrade " CLASS " encoding version") \
        *(IOF) |= H5O_DECODEIO_DIRTY;                                          \
    } /* end if */
#endif /* H5_STRICT_FORMAT_CHECKS */

/* This message derives from H5O message class */
const H5O_msg_class_t H5O_MSG_DTYPE[1] = {{
    H5O_DTYPE_ID,		/* message id number		*/
    "datatype",			/* message name for debugging	*/
    sizeof(H5T_t),		/* native message size		*/
    H5O_SHARE_IS_SHARABLE|H5O_SHARE_IN_OHDR,	/* messages are sharable?       */
    H5O_dtype_shared_decode,	/* decode message		*/
    H5O_dtype_shared_encode,	/* encode message		*/
    H5O_dtype_copy,		/* copy the native value	*/
    H5O_dtype_shared_size,	/* size of raw message		*/
    H5O_dtype_reset,		/* reset method			*/
    H5O_dtype_free,		/* free method			*/
    H5O_dtype_shared_delete,	/* file delete method		*/
    H5O_dtype_shared_link,	/* link method			*/
    H5O_dtype_set_share,	/* set share method		*/
    H5O_dtype_can_share,	/* can share method		*/
    H5O_dtype_pre_copy_file,	/* pre copy native value to file */
    H5O_dtype_shared_copy_file,	/* copy native value to file    */
    H5O_dtype_shared_post_copy_file,	/* post copy native value to file */
    NULL,			/* get creation index		*/
    NULL,			/* set creation index		*/
    H5O_dtype_shared_debug	/* debug the message		*/
}};


/*-------------------------------------------------------------------------
 * Function:	H5O_dtype_decode_helper
 *
 * Purpose:	Decodes a datatype
 *
 * Return:	TRUE if we can upgrade the parent type's version even
 *                  with strict format checks
 *              FALSE if we cannot
 *              Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Monday, December  8, 1997
 *
 *-------------------------------------------------------------------------
 */
static htri_t
H5O_dtype_decode_helper(H5F_t *f, unsigned *ioflags/*in,out*/, const uint8_t **pp, H5T_t *dt)
{
    unsigned	flags, version;
    unsigned	i;
    size_t	z;
    htri_t      ret_value = FALSE;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(pp && *pp);
    HDassert(dt && dt->shared);

    /* Version, class & flags */
    UINT32DECODE(*pp, flags);
    version = (flags>>4) & 0x0f;
    if(version < H5O_DTYPE_VERSION_1 || version > H5O_DTYPE_VERSION_3)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTLOAD, FAIL, "bad version number for datatype message")
    dt->shared->version = version;
    dt->shared->type = (H5T_class_t)(flags & 0x0f);
    flags >>= 8;

    /* Size */
    UINT32DECODE(*pp, dt->shared->size);

    switch(dt->shared->type) {
        case H5T_INTEGER:
            /*
             * Integer types...
             */
            dt->shared->u.atomic.order = (flags & 0x1) ? H5T_ORDER_BE : H5T_ORDER_LE;
            dt->shared->u.atomic.lsb_pad = (flags & 0x2) ? H5T_PAD_ONE : H5T_PAD_ZERO;
            dt->shared->u.atomic.msb_pad = (flags & 0x4) ? H5T_PAD_ONE : H5T_PAD_ZERO;
            dt->shared->u.atomic.u.i.sign = (flags & 0x8) ? H5T_SGN_2 : H5T_SGN_NONE;
            UINT16DECODE(*pp, dt->shared->u.atomic.offset);
            UINT16DECODE(*pp, dt->shared->u.atomic.prec);
            break;

        case H5T_FLOAT:
            /*
             * Floating-point types...
             */
            dt->shared->u.atomic.order = (flags & 0x1) ? H5T_ORDER_BE : H5T_ORDER_LE;
            if(version >= H5O_DTYPE_VERSION_3) {
                /* Unsupported byte order*/
                if((flags & 0x40) && !(flags & 0x1))
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "bad byte order for datatype message")

                /* VAX order if both 1st and 6th bits are turned on*/
                if(flags & 0x40)
                    dt->shared->u.atomic.order = H5T_ORDER_VAX;
            } /* end if */
            dt->shared->u.atomic.lsb_pad = (flags & 0x2) ? H5T_PAD_ONE : H5T_PAD_ZERO;
            dt->shared->u.atomic.msb_pad = (flags & 0x4) ? H5T_PAD_ONE : H5T_PAD_ZERO;
            dt->shared->u.atomic.u.f.pad = (flags & 0x8) ? H5T_PAD_ONE : H5T_PAD_ZERO;
            switch((flags >> 4) & 0x03) {
                case 0:
                    dt->shared->u.atomic.u.f.norm = H5T_NORM_NONE;
                    break;

                case 1:
                    dt->shared->u.atomic.u.f.norm = H5T_NORM_MSBSET;
                    break;

                case 2:
                    dt->shared->u.atomic.u.f.norm = H5T_NORM_IMPLIED;
                    break;

                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unknown floating-point normalization")
            } /* end switch */
            dt->shared->u.atomic.u.f.sign = (flags >> 8) & 0xff;
            UINT16DECODE(*pp, dt->shared->u.atomic.offset);
            UINT16DECODE(*pp, dt->shared->u.atomic.prec);
            dt->shared->u.atomic.u.f.epos = *(*pp)++;
            dt->shared->u.atomic.u.f.esize = *(*pp)++;
            HDassert(dt->shared->u.atomic.u.f.esize > 0);
            dt->shared->u.atomic.u.f.mpos = *(*pp)++;
            dt->shared->u.atomic.u.f.msize = *(*pp)++;
            HDassert(dt->shared->u.atomic.u.f.msize > 0);
            UINT32DECODE(*pp, dt->shared->u.atomic.u.f.ebias);
            break;

        case H5T_TIME:  /* Time datatypes */
            dt->shared->u.atomic.order = (flags & 0x1) ? H5T_ORDER_BE : H5T_ORDER_LE;
            UINT16DECODE(*pp, dt->shared->u.atomic.prec);
            break;

        case H5T_STRING:
            /*
             * Character string types...
             */
            dt->shared->u.atomic.order = H5T_ORDER_NONE;
            dt->shared->u.atomic.prec = 8 * dt->shared->size;
            dt->shared->u.atomic.offset = 0;
            dt->shared->u.atomic.lsb_pad = H5T_PAD_ZERO;
            dt->shared->u.atomic.msb_pad = H5T_PAD_ZERO;

            dt->shared->u.atomic.u.s.pad = (H5T_str_t)(flags & 0x0f);
            dt->shared->u.atomic.u.s.cset = (H5T_cset_t)((flags >> 4) & 0x0f);
            break;

        case H5T_BITFIELD:
            /*
             * Bit fields...
             */
            dt->shared->u.atomic.order = (flags & 0x1) ? H5T_ORDER_BE : H5T_ORDER_LE;
            dt->shared->u.atomic.lsb_pad = (flags & 0x2) ? H5T_PAD_ONE : H5T_PAD_ZERO;
            dt->shared->u.atomic.msb_pad = (flags & 0x4) ? H5T_PAD_ONE : H5T_PAD_ZERO;
            UINT16DECODE(*pp, dt->shared->u.atomic.offset);
            UINT16DECODE(*pp, dt->shared->u.atomic.prec);
            break;

        case H5T_OPAQUE:
            /*
             * Opaque types...
             */
            z = flags & (H5T_OPAQUE_TAG_MAX - 1);
            HDassert(0 == (z & 0x7)); /*must be aligned*/
            if(NULL == (dt->shared->u.opaque.tag = (char *)H5MM_malloc(z + 1)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")
            HDmemcpy(dt->shared->u.opaque.tag, *pp, z);
            dt->shared->u.opaque.tag[z] = '\0';
            *pp += z;
            break;

        case H5T_COMPOUND:
            {
                unsigned offset_nbytes;         /* Size needed to encode member offsets */
                size_t max_memb_pos = 0;        /* Maximum member covered, so far */
                unsigned max_version = 0;       /* Maximum member version */
                hbool_t upgrade_to = 0;         /* Version number we can "soft" upgrade to */
                unsigned j;

                /* Compute the # of bytes required to store a member offset */
                offset_nbytes = H5VM_limit_enc_size((uint64_t)dt->shared->size);

                /*
                 * Compound datatypes...
                 */
                dt->shared->u.compnd.nmembs = flags & 0xffff;
                if(dt->shared->u.compnd.nmembs == 0)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_BADVALUE, FAIL, "invalid number of members: %u", dt->shared->u.compnd.nmembs)
                dt->shared->u.compnd.nalloc = dt->shared->u.compnd.nmembs;
                dt->shared->u.compnd.memb = (H5T_cmemb_t *)H5MM_calloc(dt->shared->u.compnd.nalloc * sizeof(H5T_cmemb_t));
                dt->shared->u.compnd.memb_size = 0;
                if(NULL == dt->shared->u.compnd.memb)
                    HGOTO_ERROR(H5E_DATATYPE, H5E_CANTALLOC, FAIL, "memory allocation failed")
                for(i = 0; i < dt->shared->u.compnd.nmembs; i++) {
                    unsigned ndims = 0;     /* Number of dimensions of the array field */
                    htri_t can_upgrade;     /* Whether we can upgrade this type's version */
                    hsize_t dim[H5O_LAYOUT_NDIMS];  /* Dimensions of the array */
                    H5T_t *array_dt;    /* Temporary pointer to the array datatype */
                    H5T_t *temp_type;   /* Temporary pointer to the field's datatype */

                    /* Decode the field name */
                    dt->shared->u.compnd.memb[i].name = H5MM_xstrdup((const char *)*pp);

                    /* Version 3 of the datatype message eliminated the padding to multiple of 8 bytes */
                    if(version >= H5O_DTYPE_VERSION_3)
                        /* Advance past name, including null terminator */
                        *pp += HDstrlen((const char *)*pp) + 1;
                    else
                        /* Advance multiple of 8 w/ null terminator */
                        *pp += ((HDstrlen((const char *)*pp) + 8) / 8) * 8;

                    /* Decode the field offset */
                    /* (starting with version 3 of the datatype message, use the minimum # of bytes required) */
                    if(version >= H5O_DTYPE_VERSION_3)
                        UINT32DECODE_VAR(*pp, dt->shared->u.compnd.memb[i].offset, offset_nbytes)
                    else
                        UINT32DECODE(*pp, dt->shared->u.compnd.memb[i].offset)

                    /* Older versions of the library allowed a field to have
                     * intrinsic 'arrayness'.  Newer versions of the library
                     * use the separate array datatypes. */
                    if(version == H5O_DTYPE_VERSION_1) {
                        /* Decode the number of dimensions */
                        ndims = *(*pp)++;
                        HDassert(ndims <= 4);
                        *pp += 3;		/*reserved bytes */

                        /* Skip dimension permutation */
                        *pp += 4;

                        /* Skip reserved bytes */
                        *pp += 4;

                        /* Decode array dimension sizes */
                        for(j = 0; j < 4; j++)
                            UINT32DECODE(*pp, dim[j]);
                    } /* end if */

                    /* Allocate space for the field's datatype */
                    if(NULL == (temp_type = H5T__alloc()))
                        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

                    /* Decode the field's datatype information */
                    if((can_upgrade = H5O_dtype_decode_helper(f, ioflags, pp, temp_type)) < 0) {
                        for(j = 0; j <= i; j++)
                            H5MM_xfree(dt->shared->u.compnd.memb[j].name);
                        H5MM_xfree(dt->shared->u.compnd.memb);
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTDECODE, FAIL, "unable to decode member type")
                    } /* end if */

                    /* Upgrade the version if we can and it is necessary */
                    if(can_upgrade && temp_type->shared->version > version) {
                        upgrade_to = temp_type->shared->version;

                        /* Pass "can_upgrade" flag down to parent type */
                        ret_value = TRUE;
                    } /* end if */

                    /* Go create the array datatype now, for older versions of the datatype message */
                    if(version == H5O_DTYPE_VERSION_1) {
                        /* Check if this member is an array field */
                        if(ndims > 0) {
                            /* Create the array datatype for the field */
                            if((array_dt = H5T__array_create(temp_type, ndims, dim)) == NULL) {
                                for(j = 0; j <= i; j++)
                                    H5MM_xfree(dt->shared->u.compnd.memb[j].name);
                                H5MM_xfree(dt->shared->u.compnd.memb);
                                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL, "unable to create array datatype")
                            } /* end if */

                            /* Close the base type for the array */
                            H5T_close(temp_type);

                            /* Make the array type the type that is set for the field */
                            temp_type = array_dt;

                            /* Reset array version if NOCHANGE is specified (i.e. h5debug) */
                            if(*ioflags & H5O_DECODEIO_NOCHANGE)
                                temp_type->shared->version = H5O_DTYPE_VERSION_1;
                            else {
                                /* Otherwise upgrade the compound version */
                                if(upgrade_to < temp_type->shared->version)
                                    upgrade_to = temp_type->shared->version;

                                /* Set the return value to indicate that we should freely
                                 * upgrade parent types */
                                ret_value = TRUE;
                            } /* end else */
                        } /* end if */
                    } /* end if */

                    /* Keep track of the maximum member version found */
                    if(temp_type->shared->version > max_version)
                        max_version = temp_type->shared->version;

                    /*
                     * Set the "force conversion" flag if VL datatype fields exist in this
                     * type or any component types
                     */
                    if(temp_type->shared->force_conv == TRUE)
                        dt->shared->force_conv = TRUE;

                    /* Member size */
                    dt->shared->u.compnd.memb[i].size = temp_type->shared->size;
                    dt->shared->u.compnd.memb_size += temp_type->shared->size;

                    /* Set the field datatype (finally :-) */
                    dt->shared->u.compnd.memb[i].type = temp_type;

                    /* Check if this field overlaps with a prior field */
                    /* (probably indicates that the file is corrupt) */
                    if(i > 0 && dt->shared->u.compnd.memb[i].offset < max_memb_pos) {
                        for(j = 0; j < i; j++)
                            if(dt->shared->u.compnd.memb[i].offset >= dt->shared->u.compnd.memb[j].offset
                                    && dt->shared->u.compnd.memb[i].offset < (dt->shared->u.compnd.memb[j].offset + dt->shared->u.compnd.memb[j].size))
                                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTDECODE, FAIL, "member overlaps with previous member")
                    } /* end if */

                    /* Update the maximum member position covered */
                    max_memb_pos = MAX(max_memb_pos, (dt->shared->u.compnd.memb[i].offset + dt->shared->u.compnd.memb[i].size));
                } /* end for */

                /* Check if the compound type is packed */
                H5T__update_packed(dt);

                /* Upgrade the compound if requested */
                if(version < upgrade_to) {
                    version = upgrade_to;
                    if(H5T__upgrade_version(dt, upgrade_to) < 0)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTSET, FAIL, "can't upgrade compound encoding version")
                    /* We won't mark the message dirty since there were no
                     * errors in the file, simply type versions that we will no
                     * longer encode. */
                } /* end if */

                /* Check that no member of this compound has a version greater
                 * than the compound itself. */
                H5O_DTYPE_CHECK_VERSION(dt, version, max_version, ioflags, "compound", FAIL)
            }
            break;

        case H5T_REFERENCE: /* Reference datatypes...  */
            dt->shared->u.atomic.order = H5T_ORDER_NONE;
            dt->shared->u.atomic.prec = 8 * dt->shared->size;
            dt->shared->u.atomic.offset = 0;
            dt->shared->u.atomic.lsb_pad = H5T_PAD_ZERO;
            dt->shared->u.atomic.msb_pad = H5T_PAD_ZERO;

            /* Set reference type */
            dt->shared->u.atomic.u.r.rtype = (H5R_type_t)(flags & 0x0f);

            /* Set extra information for object references, so the hobj_ref_t gets swizzled correctly */
            if(dt->shared->u.atomic.u.r.rtype == H5R_OBJECT) {
                /* Mark location this type as undefined for now.  The caller function should
                 * decide the location. */
                dt->shared->u.atomic.u.r.loc = H5T_LOC_BADLOC;

                /* This type needs conversion */
                dt->shared->force_conv = TRUE;
            } /* end if */
            break;

        case H5T_ENUM:
            /*
             * Enumeration datatypes...
             */
            dt->shared->u.enumer.nmembs = dt->shared->u.enumer.nalloc = flags & 0xffff;
            if(NULL == (dt->shared->parent = H5T__alloc()))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")
            if(H5O_dtype_decode_helper(f, ioflags, pp, dt->shared->parent) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTDECODE, FAIL, "unable to decode parent datatype")

            /* Check if the parent of this enum has a version greater than the
             * enum itself. */
            H5O_DTYPE_CHECK_VERSION(dt, version, dt->shared->parent->shared->version,
                ioflags, "enum", FAIL)

            if(NULL == (dt->shared->u.enumer.name = (char **)H5MM_calloc(dt->shared->u.enumer.nalloc * sizeof(char*))) ||
                    NULL == (dt->shared->u.enumer.value = (uint8_t *)H5MM_calloc(dt->shared->u.enumer.nalloc * dt->shared->parent->shared->size)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed")

            /* Names */
            for(i = 0; i < dt->shared->u.enumer.nmembs; i++) {
                dt->shared->u.enumer.name[i] = H5MM_xstrdup((const char*)*pp);

                /* Version 3 of the datatype message eliminated the padding to multiple of 8 bytes */
                if(version >= H5O_DTYPE_VERSION_3)
                    /* Advance past name, including null terminator */
                    *pp += HDstrlen((const char *)*pp) + 1;
                else
                    /* Advance multiple of 8 w/ null terminator */
                    *pp += ((HDstrlen((const char *)*pp) + 8) / 8) * 8;
            } /* end for */

            /* Values */
            HDmemcpy(dt->shared->u.enumer.value, *pp,
                     dt->shared->u.enumer.nmembs * dt->shared->parent->shared->size);
            *pp += dt->shared->u.enumer.nmembs * dt->shared->parent->shared->size;
            break;

        case H5T_VLEN:  /* Variable length datatypes...  */
            /* Set the type of VL information, either sequence or string */
            dt->shared->u.vlen.type = (H5T_vlen_type_t)(flags & 0x0f);
            if(dt->shared->u.vlen.type == H5T_VLEN_STRING) {
                dt->shared->u.vlen.pad  = (H5T_str_t)((flags >> 4) & 0x0f);
                dt->shared->u.vlen.cset = (H5T_cset_t)((flags >> 8) & 0x0f);
            } /* end if */

            /* Decode base type of VL information */
            if(NULL == (dt->shared->parent = H5T__alloc()))
                HGOTO_ERROR(H5E_DATATYPE, H5E_NOSPACE, FAIL, "memory allocation failed")
            if(H5O_dtype_decode_helper(f, ioflags, pp, dt->shared->parent) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTDECODE, FAIL, "unable to decode VL parent type")

            /* Check if the parent of this vlen has a version greater than the
             * vlen itself. */
            H5O_DTYPE_CHECK_VERSION(dt, version, dt->shared->parent->shared->version,
                ioflags, "vlen", FAIL)

            dt->shared->force_conv=TRUE;

            /* Mark location this type as undefined for now.  The caller function should
             * decide the location. */
            if(H5T_set_loc(dt, f, H5T_LOC_BADLOC) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "invalid datatype location")
            break;

        case H5T_ARRAY:  /* Array datatypes */
            /* Decode the number of dimensions */
            dt->shared->u.array.ndims = *(*pp)++;

            /* Double-check the number of dimensions */
            HDassert(dt->shared->u.array.ndims <= H5S_MAX_RANK);

            /* Skip reserved bytes, if version has them */
            if(version < H5O_DTYPE_VERSION_3)
                *pp += 3;

            /* Decode array dimension sizes & compute number of elements */
            for(i = 0, dt->shared->u.array.nelem = 1; i < (unsigned)dt->shared->u.array.ndims; i++) {
                UINT32DECODE(*pp, dt->shared->u.array.dim[i]);
                dt->shared->u.array.nelem *= dt->shared->u.array.dim[i];
            } /* end for */

            /* Skip array dimension permutations, if version has them */
            if(version < H5O_DTYPE_VERSION_3)
                *pp += dt->shared->u.array.ndims * 4;

            /* Decode base type of array */
            if(NULL == (dt->shared->parent = H5T__alloc()))
                HGOTO_ERROR(H5E_DATATYPE, H5E_NOSPACE, FAIL, "memory allocation failed")
            if(H5O_dtype_decode_helper(f, ioflags, pp, dt->shared->parent) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTDECODE, FAIL, "unable to decode array parent type")

            /* Check if the parent of this array has a version greater than the
             * array itself. */
            H5O_DTYPE_CHECK_VERSION(dt, version, dt->shared->parent->shared->version,
                ioflags, "array", FAIL)

            /* There should be no array datatypes with version < 2. */
            H5O_DTYPE_CHECK_VERSION(dt, version, H5O_DTYPE_VERSION_2, ioflags,
                "array", FAIL)

            /*
             * Set the "force conversion" flag if a VL base datatype is used or
             * or if any components of the base datatype are VL types.
             */
            if(dt->shared->parent->shared->force_conv == TRUE)
                dt->shared->force_conv = TRUE;
            break;

        case H5T_NO_CLASS:
        case H5T_NCLASSES:
        default:
            HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "unknown datatype class found")
    } /* end switch */

done:
    if(ret_value < 0) {
        if(dt != NULL) {
            if(dt->shared != NULL)
                dt->shared = H5FL_FREE(H5T_shared_t, dt->shared);
            dt = H5FL_FREE(H5T_t, dt);
        } /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_dtype_decode_helper() */


/*-------------------------------------------------------------------------
 * Function:	H5O_dtype_encode_helper
 *
 * Purpose:	Encodes a datatype.
 *
 * Note:	When changing the format of a datatype (or adding a new one),
 *		remember to change the upgrade version callback
 *		(H5T_upgrade_version_cb).
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Monday, December  8, 1997
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_dtype_encode_helper(const H5F_t *f, uint8_t **pp, const H5T_t *dt)
{
    unsigned	flags = 0;
    uint8_t	*hdr = (uint8_t *)*pp;
    unsigned	i;
    size_t	n, z;
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(pp && *pp);
    HDassert(dt);

    /* skip the type and class bit-field for now */
    *pp += 4;
    UINT32ENCODE(*pp, dt->shared->size);

    switch(dt->shared->type) {
        case H5T_INTEGER:
            /*
             * Integer datatypes...
             */
            switch (dt->shared->u.atomic.order) {
                case H5T_ORDER_LE:
                    break;		/*nothing */

                case H5T_ORDER_BE:
                    flags |= 0x01;
                    break;

                case H5T_ORDER_ERROR:
                case H5T_ORDER_VAX:
                case H5T_ORDER_MIXED:
                case H5T_ORDER_NONE:
                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "byte order is not supported in file format yet")
            } /* end switch */

            switch (dt->shared->u.atomic.lsb_pad) {
                case H5T_PAD_ZERO:
                    break;		/*nothing */

                case H5T_PAD_ONE:
                    flags |= 0x02;
                    break;

                case H5T_PAD_ERROR:
                case H5T_PAD_BACKGROUND:
                case H5T_NPAD:
                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "bit padding is not supported in file format yet")
            } /* end switch */

            switch (dt->shared->u.atomic.msb_pad) {
                case H5T_PAD_ZERO:
                    break;		/*nothing */

                case H5T_PAD_ERROR:
                case H5T_PAD_BACKGROUND:
                case H5T_NPAD:
                case H5T_PAD_ONE:
                    flags |= 0x04;
                    break;

                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "bit padding is not supported in file format yet")
            } /* end switch */

            switch (dt->shared->u.atomic.u.i.sign) {
                case H5T_SGN_NONE:
                    break;		/*nothing */

                case H5T_SGN_2:
                    flags |= 0x08;
                    break;

                case H5T_SGN_ERROR:
                case H5T_NSGN:
                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "sign scheme is not supported in file format yet")
            } /* end switch */

            UINT16ENCODE(*pp, dt->shared->u.atomic.offset);
            UINT16ENCODE(*pp, dt->shared->u.atomic.prec);
            break;

        case H5T_FLOAT:
            /*
             * Floating-point types...
             */
            switch (dt->shared->u.atomic.order) {
                case H5T_ORDER_LE:
                    break;		/*nothing*/

                case H5T_ORDER_BE:
                    flags |= 0x01;
                    break;

                case H5T_ORDER_VAX:     /*turn on 1st and 6th (reserved before adding VAX) bits*/
                    flags |= 0x41;
                    HDassert(dt->shared->version >= H5O_DTYPE_VERSION_3);
                    break;

                case H5T_ORDER_MIXED:
                case H5T_ORDER_ERROR:
                case H5T_ORDER_NONE:
                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "byte order is not supported in file format yet")
            } /* end switch */

            switch (dt->shared->u.atomic.lsb_pad) {
                case H5T_PAD_ZERO:
                    break;		/*nothing */

                case H5T_PAD_ONE:
                    flags |= 0x02;
                    break;

                case H5T_PAD_ERROR:
                case H5T_PAD_BACKGROUND:
                case H5T_NPAD:
                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "bit padding is not supported in file format yet")
            } /* end switch */

            switch (dt->shared->u.atomic.msb_pad) {
                case H5T_PAD_ZERO:
                    break;		/*nothing */

                case H5T_PAD_ONE:
                    flags |= 0x04;
                    break;

                case H5T_PAD_ERROR:
                case H5T_PAD_BACKGROUND:
                case H5T_NPAD:
                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "bit padding is not supported in file format yet")
            } /* end switch */

            switch (dt->shared->u.atomic.u.f.pad) {
                case H5T_PAD_ZERO:
                    break;		/*nothing */

                case H5T_PAD_ONE:
                    flags |= 0x08;
                    break;

                case H5T_PAD_ERROR:
                case H5T_PAD_BACKGROUND:
                case H5T_NPAD:
                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "bit padding is not supported in file format yet")
            } /* end switch */

            switch (dt->shared->u.atomic.u.f.norm) {
                case H5T_NORM_NONE:
                    break;		/*nothing */

                case H5T_NORM_MSBSET:
                    flags |= 0x10;
                    break;

                case H5T_NORM_IMPLIED:
                    flags |= 0x20;
                    break;

                case H5T_NORM_ERROR:
                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "normalization scheme is not supported in file format yet")
            } /* end switch */

            flags = (unsigned)(flags | ((dt->shared->u.atomic.u.f.sign << 8) & 0xff00));
            UINT16ENCODE(*pp, dt->shared->u.atomic.offset);
            UINT16ENCODE(*pp, dt->shared->u.atomic.prec);
            HDassert(dt->shared->u.atomic.u.f.epos <= 255);
            *(*pp)++ = (uint8_t)(dt->shared->u.atomic.u.f.epos);
            HDassert(dt->shared->u.atomic.u.f.esize <= 255);
            *(*pp)++ = (uint8_t)(dt->shared->u.atomic.u.f.esize);
            HDassert(dt->shared->u.atomic.u.f.mpos <= 255);
            *(*pp)++ = (uint8_t)(dt->shared->u.atomic.u.f.mpos);
            HDassert(dt->shared->u.atomic.u.f.msize <= 255);
            *(*pp)++ = (uint8_t)(dt->shared->u.atomic.u.f.msize);
            UINT32ENCODE(*pp, dt->shared->u.atomic.u.f.ebias);
            break;

        case H5T_TIME:  /* Time datatypes...  */
            switch (dt->shared->u.atomic.order) {
                case H5T_ORDER_LE:
                    break;		/*nothing */

                case H5T_ORDER_BE:
                    flags |= 0x01;
                    break;

                case H5T_ORDER_VAX:
                case H5T_ORDER_MIXED:
                case H5T_ORDER_ERROR:
                case H5T_ORDER_NONE:
                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "byte order is not supported in file format yet")
            } /* end switch */
            UINT16ENCODE(*pp, dt->shared->u.atomic.prec);
            break;

        case H5T_STRING:
            /*
             * Character string types... (not fully implemented)
             */
            HDassert(dt->shared->u.atomic.order == H5T_ORDER_NONE);
            HDassert(dt->shared->u.atomic.prec == 8 * dt->shared->size);
            HDassert(dt->shared->u.atomic.offset == 0);
            HDassert(dt->shared->u.atomic.lsb_pad == H5T_PAD_ZERO);
            HDassert(dt->shared->u.atomic.msb_pad == H5T_PAD_ZERO);

            flags = (unsigned)(flags | (dt->shared->u.atomic.u.s.pad & 0x0f));
            flags = (unsigned)(flags | ((((unsigned)dt->shared->u.atomic.u.s.cset) & 0x0f) << 4));
            break;

        case H5T_BITFIELD:
            /*
             * Bitfield datatypes...
             */
            switch (dt->shared->u.atomic.order) {
                case H5T_ORDER_LE:
                    break;		/*nothing */

                case H5T_ORDER_BE:
                    flags |= 0x01;
                    break;

                case H5T_ORDER_VAX:
                case H5T_ORDER_MIXED:
                case H5T_ORDER_ERROR:
                case H5T_ORDER_NONE:
                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "byte order is not supported in file format yet")
            } /* end switch */

            switch (dt->shared->u.atomic.lsb_pad) {
                case H5T_PAD_ZERO:
                    break;		/*nothing */

                case H5T_PAD_ONE:
                    flags |= 0x02;
                    break;

                case H5T_PAD_ERROR:
                case H5T_PAD_BACKGROUND:
                case H5T_NPAD:
                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "bit padding is not supported in file format yet")
            } /* end switch */

            switch (dt->shared->u.atomic.msb_pad) {
                case H5T_PAD_ZERO:
                    break;		/*nothing */

                case H5T_PAD_ONE:
                    flags |= 0x04;
                    break;

                case H5T_PAD_ERROR:
                case H5T_PAD_BACKGROUND:
                case H5T_NPAD:
                default:
                    HGOTO_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, FAIL, "bit padding is not supported in file format yet")
            } /* end switch */

            UINT16ENCODE(*pp, dt->shared->u.atomic.offset);
            UINT16ENCODE(*pp, dt->shared->u.atomic.prec);
            break;

        case H5T_OPAQUE:
            /*
             * Opaque datatypes...  The tag is stored in a field which is a
             * multiple of eight characters and null padded (not necessarily
             * null terminated).
             */
            {
                size_t	aligned;

                z = HDstrlen(dt->shared->u.opaque.tag);
                aligned = (z + 7) & (H5T_OPAQUE_TAG_MAX - 8);
                flags = (unsigned)(flags | aligned);
                HDmemcpy(*pp, dt->shared->u.opaque.tag, MIN(z,aligned));
                for(n = MIN(z, aligned); n < aligned; n++)
                    (*pp)[n] = 0;
                *pp += aligned;
            }
            break;

        case H5T_COMPOUND:
            {
                unsigned offset_nbytes;         /* Size needed to encode member offsets */

                /* Compute the # of bytes required to store a member offset */
                offset_nbytes = H5VM_limit_enc_size((uint64_t)dt->shared->size);

                /*
                 * Compound datatypes...
                 */
                flags = dt->shared->u.compnd.nmembs & 0xffff;
                for(i = 0; i < dt->shared->u.compnd.nmembs; i++) {
                    /* Sanity check */
                    /* (compound datatypes w/array members must be encoded w/version >= 2) */
                    HDassert(dt->shared->u.compnd.memb[i].type->shared->type != H5T_ARRAY || dt->shared->version >= H5O_DTYPE_VERSION_2);

                    /* Check that the version is at least as great as the member */
                    HDassert(dt->shared->version >= dt->shared->u.compnd.memb[i].type->shared->version);

                    /* Name */
                    HDstrcpy((char*)(*pp), dt->shared->u.compnd.memb[i].name);

                    /* Version 3 of the datatype message removed the padding to multiple of 8 bytes */
                    n = HDstrlen(dt->shared->u.compnd.memb[i].name);
                    if(dt->shared->version >= H5O_DTYPE_VERSION_3)
                        *pp += n + 1;
                    else {
                        /* Pad name to multiple of 8 bytes */
                        for(z = n + 1; z % 8; z++)
                            (*pp)[z] = '\0';
                        *pp += z;
                    } /* end if */

                    /* Member offset */
                    /* (starting with version 3 of the datatype message, use the minimum # of bytes required) */
                    if(dt->shared->version >= H5O_DTYPE_VERSION_3)
                        UINT32ENCODE_VAR(*pp, (uint32_t)dt->shared->u.compnd.memb[i].offset, offset_nbytes)
                    else
                        UINT32ENCODE(*pp, dt->shared->u.compnd.memb[i].offset)

                    /* If we don't have any array fields, write out the old style
                     * member information, for better backward compatibility
                     * Write out all zeros for the array information, though...
                     */
                    if(dt->shared->version == H5O_DTYPE_VERSION_1) {
                        unsigned	j;

                        /* Dimensionality */
                        *(*pp)++ = 0;

                        /* Reserved */
                        *(*pp)++ = 0;
                        *(*pp)++ = 0;
                        *(*pp)++ = 0;

                        /* Dimension permutation */
                        UINT32ENCODE(*pp, 0);

                        /* Reserved */
                        UINT32ENCODE(*pp, 0);

                        /* Dimensions */
                        for(j = 0; j < 4; j++)
                            UINT32ENCODE(*pp, 0);
                    } /* end if */

                    /* Subtype */
                    if(H5O_dtype_encode_helper(f, pp, dt->shared->u.compnd.memb[i].type) < 0)
                        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTENCODE, FAIL, "unable to encode member type")
                } /* end for */
            }
            break;

        case H5T_REFERENCE:
            flags |= (dt->shared->u.atomic.u.r.rtype & 0x0f);
            break;

        case H5T_ENUM:
            /* Check that the version is at least as great as the parent */
            HDassert(dt->shared->version >= dt->shared->parent->shared->version);

            /*
             * Enumeration datatypes...
             */
            flags = dt->shared->u.enumer.nmembs & 0xffff;

            /* Parent type */
            if(H5O_dtype_encode_helper(f, pp, dt->shared->parent) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTENCODE, FAIL, "unable to encode parent datatype")

            /* Names, each a multiple of eight bytes */
            for(i = 0; i < dt->shared->u.enumer.nmembs; i++) {
                /* Name */
                HDstrcpy((char*)(*pp), dt->shared->u.enumer.name[i]);

                /* Version 3 of the datatype message removed the padding to multiple of 8 bytes */
                n = HDstrlen(dt->shared->u.enumer.name[i]);
                if(dt->shared->version >= H5O_DTYPE_VERSION_3)
                    *pp += n + 1;
                else {
                    /* Pad to multiple of 8 bytes */
                    for(z = n + 1; z % 8; z++)
                        (*pp)[z] = '\0';
                    *pp += z;
                } /* end for */
            } /* end for */

            /* Values */
            HDmemcpy(*pp, dt->shared->u.enumer.value, dt->shared->u.enumer.nmembs * dt->shared->parent->shared->size);
            *pp += dt->shared->u.enumer.nmembs * dt->shared->parent->shared->size;
            break;

        case H5T_VLEN:  /* Variable length datatypes...  */
            /* Check that the version is at least as great as the parent */
            HDassert(dt->shared->version >= dt->shared->parent->shared->version);

            flags |= (dt->shared->u.vlen.type & 0x0f);
            if(dt->shared->u.vlen.type == H5T_VLEN_STRING) {
                flags = (unsigned)(flags | (((unsigned)dt->shared->u.vlen.pad   & 0x0f) << 4));
                flags = (unsigned)(flags | (((unsigned)dt->shared->u.vlen.cset  & 0x0f) << 8));
            } /* end if */

            /* Encode base type of VL information */
            if(H5O_dtype_encode_helper(f, pp, dt->shared->parent) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTENCODE, FAIL, "unable to encode VL parent type")
            break;

        case H5T_ARRAY:  /* Array datatypes */
            /* Double-check the number of dimensions */
            HDassert(dt->shared->u.array.ndims <= H5S_MAX_RANK);

            /* Check that the version is valid */
            HDassert(dt->shared->version >= H5O_DTYPE_VERSION_2);

            /* Check that the version is at least as great as the parent */
            HDassert(dt->shared->version >= dt->shared->parent->shared->version);

            /* Encode the number of dimensions */
            HDassert(dt->shared->u.array.ndims <= UCHAR_MAX);
            *(*pp)++ = (uint8_t)dt->shared->u.array.ndims;

            /* Drop this information for Version 3 of the format */
            if(dt->shared->version < H5O_DTYPE_VERSION_3) {
                /* Reserved */
                *(*pp)++ = '\0';
                *(*pp)++ = '\0';
                *(*pp)++ = '\0';
            } /* end if */

            /* Encode array dimensions */
            for(i = 0; i < (unsigned)dt->shared->u.array.ndims; i++)
                UINT32ENCODE(*pp, dt->shared->u.array.dim[i]);

            /* Drop this information for Version 3 of the format */
            if(dt->shared->version < H5O_DTYPE_VERSION_3) {
                /* Encode 'fake' array dimension permutations */
                for(i = 0; i < (unsigned)dt->shared->u.array.ndims; i++)
                    UINT32ENCODE(*pp, i);
            } /* end if */

            /* Encode base type of array's information */
            if(H5O_dtype_encode_helper(f, pp, dt->shared->parent) < 0)
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTENCODE, FAIL, "unable to encode VL parent type")
            break;

        case H5T_NO_CLASS:
        case H5T_NCLASSES:
        default:
            /*nothing */
            break;
    } /* end switch */

    /* Encode the type's class, version and bit field */
    *hdr++ = (uint8_t)(((unsigned)(dt->shared->type) & 0x0f) | (dt->shared->version << 4));
    *hdr++ = (uint8_t)((flags >> 0) & 0xff);
    *hdr++ = (uint8_t)((flags >> 8) & 0xff);
    *hdr++ = (uint8_t)((flags >> 16) & 0xff);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_dtype_encode_helper() */


/*--------------------------------------------------------------------------
 NAME
    H5O_dtype_decode
 PURPOSE
    Decode a message and return a pointer to a memory struct
	with the decoded information
 USAGE
    void *H5O_dtype_decode(f, dxpl_id, mesg_flags, p)
	H5F_t *f;		IN: pointer to the HDF5 file struct
        hid_t dxpl_id;          IN: DXPL for any I/O
        unsigned mesg_flags;    IN: Message flags to influence decoding
	const uint8 *p;		IN: the raw information buffer
 RETURNS
    Pointer to the new message in native order on success, NULL on failure
 DESCRIPTION
	This function decodes the "raw" disk form of a simple datatype message
    into a struct in memory native format.  The struct is allocated within this
    function using malloc() and is returned to the caller.
--------------------------------------------------------------------------*/
static void *
H5O_dtype_decode(H5F_t *f, hid_t UNUSED dxpl_id, H5O_t UNUSED *open_oh, unsigned UNUSED mesg_flags,
    unsigned *ioflags/*in,out*/, const uint8_t *p)
{
    H5T_t	*dt = NULL;
    void        *ret_value;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(p);

    /* Allocate datatype message */
    if(NULL == (dt = H5T__alloc()))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Perform actual decode of message */
    if(H5O_dtype_decode_helper(f, ioflags, &p, dt) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTDECODE, NULL, "can't decode type")

    /* Set return value */
    ret_value = dt;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_dtype_decode() */


/*--------------------------------------------------------------------------
 NAME
    H5O_dtype_encode
 PURPOSE
    Encode a simple datatype message
 USAGE
    herr_t H5O_dtype_encode(f, raw_size, p, mesg)
	H5F_t *f;	  IN: pointer to the HDF5 file struct
	size_t raw_size;	IN: size of the raw information buffer
	const uint8 *p;		IN: the raw information buffer
	const void *mesg;	IN: Pointer to the simple datatype struct
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
	This function encodes the native memory form of the simple datatype
    message in the "raw" disk form.
--------------------------------------------------------------------------*/
static herr_t
H5O_dtype_encode(H5F_t *f, uint8_t *p, const void *mesg)
{
    const H5T_t	   *dt = (const H5T_t *) mesg;
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(f);
    HDassert(p);
    HDassert(dt);

    /* encode */
    if(H5O_dtype_encode_helper(f, &p, dt) < 0)
	HGOTO_ERROR(H5E_DATATYPE, H5E_CANTENCODE, FAIL, "can't encode type")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_dtype_encode() */


/*--------------------------------------------------------------------------
 NAME
    H5O_dtype_copy
 PURPOSE
    Copies a message from MESG to DEST, allocating DEST if necessary.
 USAGE
    void *H5O_dtype_copy(mesg, dest)
	const void *mesg;	IN: Pointer to the source simple datatype
				    struct
	const void *dest;	IN: Pointer to the destination simple
				    datatype struct
 RETURNS
    Pointer to DEST on success, NULL on failure
 DESCRIPTION
	This function copies a native (memory) simple datatype message,
    allocating the destination structure if necessary.
--------------------------------------------------------------------------*/
static void *
H5O_dtype_copy(const void *_src, void *_dst)
{
    const H5T_t		   *src = (const H5T_t *) _src;
    H5T_t		   *dst;
    void 		   *ret_value;  /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(src);

    /* Copy */
    if(NULL == (dst = H5T_copy(src, H5T_COPY_ALL)))
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "can't copy type")

    /* Was result already allocated? */
    if(_dst) {
        *((H5T_t *) _dst) = *dst;
        dst = H5FL_FREE(H5T_t, dst);
        dst = (H5T_t *) _dst;
    } /* end if */

    /* Set return value */
    ret_value = dst;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_dtype_copy() */


/*--------------------------------------------------------------------------
 NAME
    H5O_dtype_size
 PURPOSE
    Return the raw message size in bytes
 USAGE
    void *H5O_dtype_size(f, mesg)
	H5F_t *f;	  IN: pointer to the HDF5 file struct
	const void *mesg;     IN: Pointer to the source simple datatype struct
 RETURNS
    Size of message on success, 0 on failure
 DESCRIPTION
	This function returns the size of the raw simple datatype message on
    success.  (Not counting the message type or size fields, only the data
    portion of the message).  It doesn't take into account alignment.
--------------------------------------------------------------------------*/
static size_t
H5O_dtype_size(const H5F_t *f, const void *_mesg)
{
    const H5T_t	*dt = (const H5T_t *)_mesg;
    unsigned	u;                      /* Local index variable */
    size_t	ret_value;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(f);
    HDassert(dt);

    /* Set the common size information */
    ret_value = 4 +     /* Type, class & flags */
        4;              /* Size of datatype */

    /* Add in the property field length for each datatype class */
    switch(dt->shared->type) {
        case H5T_INTEGER:
            ret_value += 4;
            break;

        case H5T_FLOAT:
            ret_value += 12;
            break;

        case H5T_TIME:
            ret_value += 2;
            break;

        case H5T_BITFIELD:
            ret_value += 4;
            break;

        case H5T_OPAQUE:
            ret_value += (HDstrlen(dt->shared->u.opaque.tag) + 7) & (H5T_OPAQUE_TAG_MAX - 8);
            break;

        case H5T_COMPOUND:
            {
                unsigned offset_nbytes;         /* Size needed to encode member offsets */

                /* Compute the # of bytes required to store a member offset */
                offset_nbytes = H5VM_limit_enc_size((uint64_t)dt->shared->size);

                /* Compute the total size needed to encode compound datatype */
                for(u = 0; u < dt->shared->u.compnd.nmembs; u++) {
                    size_t name_len;    /* Length of field's name */

                    /* Get length of field's name */
                    name_len = HDstrlen(dt->shared->u.compnd.memb[u].name);

                    /* Versions of the format >= 3 don't pad out the name */
                    if(dt->shared->version >= H5O_DTYPE_VERSION_3)
                        ret_value += name_len + 1;
                    else
                        ret_value += ((name_len + 8) / 8) * 8;

                    /* Check for encoding array datatype or using the latest file format */
                    /* (starting with version 3 of the datatype message, use the minimum # of bytes required) */
                    if(dt->shared->version >= H5O_DTYPE_VERSION_3)
                        ret_value += offset_nbytes; 	/*member offset*/
                    else if(dt->shared->version == H5O_DTYPE_VERSION_2)
                        ret_value += 4; 	/*member offset*/
                    else
                        ret_value += 4 +	/*member offset*/
                             1 +		/*dimensionality*/
                             3 +		/*reserved*/
                             4 +		/*permutation*/
                             4 +		/*reserved*/
                             16;		/*dimensions*/
                    ret_value += H5O_dtype_size(f, dt->shared->u.compnd.memb[u].type);
                } /* end for */
            }
            break;

        case H5T_ENUM:
            ret_value += H5O_dtype_size(f, dt->shared->parent);
            for(u = 0; u < dt->shared->u.enumer.nmembs; u++) {
                size_t name_len;    /* Length of field's name */

                /* Get length of field's name */
                name_len = HDstrlen(dt->shared->u.enumer.name[u]);

                /* Versions of the format >= 3 don't pad out the name */
                if(dt->shared->version >= H5O_DTYPE_VERSION_3)
                    ret_value += name_len + 1;
                else
                    ret_value += ((name_len + 8) / 8) * 8;
            } /* end for */
            ret_value += dt->shared->u.enumer.nmembs * dt->shared->parent->shared->size;
            break;

        case H5T_VLEN:
            ret_value += H5O_dtype_size(f, dt->shared->parent);
            break;

        case H5T_ARRAY:
            ret_value += 1; /* ndims */
            if(dt->shared->version < H5O_DTYPE_VERSION_3)
                ret_value += 3; /* reserved bytes*/
            ret_value += 4 * dt->shared->u.array.ndims; /* dimensions */
            if(dt->shared->version < H5O_DTYPE_VERSION_3)
                ret_value += 4 * dt->shared->u.array.ndims; /* dimension permutations */
            ret_value += H5O_dtype_size(f, dt->shared->parent);
            break;

        case H5T_NO_CLASS:
        case H5T_STRING:
        case H5T_REFERENCE:
        case H5T_NCLASSES:
        default:
            /*no properties */
            break;
    } /* end switch */

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5O_dtype_size() */


/*-------------------------------------------------------------------------
 * Function:	H5O_dtype_reset
 *
 * Purpose:	Frees resources within a message, but doesn't free
 *		the message itself.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Tuesday, December  9, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_dtype_reset(void *_mesg)
{
    H5T_t		   *dt = (H5T_t *) _mesg;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if(dt)
        H5T__free(dt);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_dtype_reset() */


/*-------------------------------------------------------------------------
 * Function:	H5O_dtype_free
 *
 * Purpose:	Free's the message
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *              Thursday, March 30, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_dtype_free(void *mesg)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(mesg);

    ((H5T_t *) mesg)->shared = H5FL_FREE(H5T_shared_t, ((H5T_t *) mesg)->shared);
    mesg = H5FL_FREE(H5T_t, mesg);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_dtype_free() */


/*-------------------------------------------------------------------------
 * Function:	H5O_dtype_set_share
 *
 * Purpose:	Copies sharing information from SH into the message.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Thursday, June	4, 1998
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_dtype_set_share(void *_mesg/*in,out*/, const H5O_shared_t *sh)
{
    H5T_t *dt = (H5T_t *)_mesg;
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(dt);
    HDassert(sh);

    /* Make sure the shared message location is initialized, so that it
     * either has valid sharing information or is set to 0.
     */
    HDassert(sh->type <= H5O_SHARE_TYPE_HERE);

    /* Make sure we're not sharing a committed type in the heap */
    HDassert(sh->type == H5O_SHARE_TYPE_COMMITTED ||
        (dt->shared->state != H5T_STATE_OPEN && dt->shared->state != H5T_STATE_NAMED));

    /* Copy the shared information */
    if(H5O_set_shared(&(dt->sh_loc), sh) < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_CANTCOPY, FAIL, "unable to copy shared message info")

    /* If this is now a committed datatype, set its state properly. */
    if(sh->type == H5O_SHARE_TYPE_COMMITTED) {
        dt->shared->state = H5T_STATE_NAMED;

        /* Set up the object location for the datatype also */
        dt->oloc.file = sh->file;
        dt->oloc.addr = sh->u.loc.oh_addr;;
        dt->oloc.holding_file = FALSE;
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_dtype_set_share() */


/*-------------------------------------------------------------------------
 * Function:	H5O_dtype_can_share
 *
 * Purpose:	Determines if this datatype is allowed to be shared or
 *              not.  Immutable datatypes or datatypes that are already
 *              shared cannot be shared (again).
 *
 * Return:	TRUE if datatype can be shared
 *              FALSE if datatype may not shared
 *              Negative on failure
 *
 * Programmer:	James Laird
 *		Monday, October 16, 2006
 *
 *-------------------------------------------------------------------------
 */
static htri_t
H5O_dtype_can_share(const void *_mesg)
{
    const H5T_t	*mesg = (const H5T_t *)_mesg;
    htri_t tri_ret;
    htri_t ret_value = TRUE;

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(mesg);

    /* Don't share immutable datatypes */
    if((tri_ret = H5T_is_immutable(mesg)) > 0)
        HGOTO_DONE(FALSE)
    else if(tri_ret < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_BADTYPE, FAIL, "can't tell if datatype is immutable")

    /* Don't share committed datatypes */
    if((tri_ret = H5T_committed(mesg)) > 0)
        HGOTO_DONE(FALSE)
    else if(tri_ret < 0)
        HGOTO_ERROR(H5E_OHDR, H5E_BADTYPE, FAIL, "can't tell if datatype is shared")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_dtype_can_share() */


/*-------------------------------------------------------------------------
 * Function:    H5O_dtype_pre_copy_file
 *
 * Purpose:     Perform any necessary actions before copying message between
 *              files
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              November 21, 2005
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_dtype_pre_copy_file(H5F_t *file_src, const void *mesg_src,
    hbool_t UNUSED *deleted, const H5O_copy_t UNUSED *cpy_info,
    void *_udata)
{
    const H5T_t	*dt_src = (const H5T_t *)mesg_src;  /* Source datatype */
    H5D_copy_file_ud_t *udata = (H5D_copy_file_ud_t *)_udata;   /* Dataset copying user data */
    herr_t ret_value = SUCCEED;          /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(file_src);
    HDassert(dt_src);

    /* If the user data is non-NULL, assume we are copying a dataset
     * and check if we need to make a copy of the datatype for later in
     * the object copying process.  (We currently only need to make a copy
     * of the datatype if it's a vlen or reference datatype, or if the layout
     * message is an early version, but since the layout information isn't
     * available here, we just make a copy in all situations)
     */
    if(udata) {
        /* Create a memory copy of the variable-length datatype */
        if(NULL == (udata->src_dtype = H5T_copy(dt_src, H5T_COPY_TRANSIENT)))
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to copy")

        /* Set the location of the source datatype to describe the disk form of the data */
        if(H5T_set_loc(udata->src_dtype, file_src, H5T_LOC_DISK) < 0)
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "cannot mark datatype on disk")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_dtype_pre_copy_file() */


/*-------------------------------------------------------------------------
 * Function:    H5O_dtype_copy_file
 *
 * Purpose:     Copy a native datatype message from one file to another.
 *
 * Return:      Success:        Native copy of message
 *              Failure:        NULL
 *
 * Programmer:  James Laird
 *              December 12, 2006
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_dtype_copy_file(H5F_t UNUSED *file_src, const H5O_msg_class_t *mesg_type,
    void *native_src, H5F_t *file_dst, hbool_t UNUSED *recompute_size,
    H5O_copy_t UNUSED *cpy_info, void UNUSED *udata, hid_t UNUSED dxpl_id)
{
    H5T_t *dst_mesg;            /* Destination datatype */
    void *ret_value;            /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Perform a normal copy of the object header message */
    if(NULL == (dst_mesg = (H5T_t *)H5O_dtype_copy(native_src, NULL)))
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "unable to copy")

    /* The datatype will be in the new file; set its location. */
    if(H5T_set_loc(dst_mesg, file_dst, H5T_LOC_DISK) < 0)
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "unable to set location")

    ret_value = dst_mesg;

done:
    if(NULL == ret_value)
        H5O_msg_free(mesg_type->id, dst_mesg);
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_dtype_copy_file() */


/*-------------------------------------------------------------------------
 * Function:    H5O_dtype_shared_post_copy_upd
 *
 * Purpose:     Update a message after the shared message operations
 *              during the post-copy loop
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Neil Fortner
 *              November 8, 2011
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_dtype_shared_post_copy_upd(const H5O_loc_t UNUSED *src_oloc,
    const void UNUSED *mesg_src, H5O_loc_t UNUSED *dst_oloc, void *mesg_dst,
    hid_t UNUSED dxpl_id, H5O_copy_t UNUSED *cpy_info)
{
    H5T_t       *dt_dst = (H5T_t *)mesg_dst;    /* Destination datatype */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if(dt_dst->sh_loc.type == H5O_SHARE_TYPE_COMMITTED) {
        HDassert(H5T_committed(dt_dst));
        dt_dst->oloc.file = dt_dst->sh_loc.file;
        dt_dst->oloc.addr = dt_dst->sh_loc.u.loc.oh_addr;
    } /* end if */
    else
        HDassert(!H5T_committed(dt_dst));

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_dtype_shared_post_copy_upd */


/*--------------------------------------------------------------------------
 NAME
    H5O_dtype_debug
 PURPOSE
    Prints debugging information for a message
 USAGE
    void *H5O_dtype_debug(f, mesg, stream, indent, fwidth)
	H5F_t *f;		IN: pointer to the HDF5 file struct
	const void *mesg;	IN: Pointer to the source simple datatype
				    struct
	FILE *stream;		IN: Pointer to the stream for output data
	int indent;		IN: Amount to indent information by
	int fwidth;		IN: Field width (?)
 RETURNS
    Non-negative on success/Negative on failure
 DESCRIPTION
	This function prints debugging output to the stream passed as a
    parameter.
--------------------------------------------------------------------------*/
static herr_t
H5O_dtype_debug(H5F_t *f, hid_t dxpl_id, const void *mesg, FILE *stream,
		int indent, int fwidth)
{
    const H5T_t		*dt = (const H5T_t*)mesg;
    const char		*s;
    char		buf[256];
    unsigned		i;
    size_t		k;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* check args */
    HDassert(f);
    HDassert(dt);
    HDassert(stream);
    HDassert(indent >= 0);
    HDassert(fwidth >= 0);

    switch (dt->shared->type) {
        case H5T_INTEGER:
            s = "integer";
            break;

        case H5T_FLOAT:
            s = "floating-point";
            break;

        case H5T_TIME:
            s = "date and time";
            break;

        case H5T_STRING:
            s = "text string";
            break;

        case H5T_BITFIELD:
            s = "bit field";
            break;

        case H5T_OPAQUE:
            s = "opaque";
            break;

        case H5T_COMPOUND:
            s = "compound";
            break;

        case H5T_REFERENCE:
            s = "reference";
            break;

        case H5T_ENUM:
            s = "enum";
            break;

        case H5T_ARRAY:
            s = "array";
            break;

        case H5T_VLEN:
            s = "vlen";
            break;

        case H5T_NO_CLASS:
        case H5T_NCLASSES:
        default:
            sprintf(buf, "H5T_CLASS_%d", (int)(dt->shared->type));
            s = buf;
            break;
    } /* end switch */
    fprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
	    "Type class:",
	    s);

    fprintf(stream, "%*s%-*s %lu byte%s\n", indent, "", fwidth,
	    "Size:",
	    (unsigned long)(dt->shared->size), 1 == dt->shared->size ? "" : "s");

    fprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
		"Version:", dt->shared->version);

    if (H5T_COMPOUND == dt->shared->type) {
	fprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
		"Number of members:",
		dt->shared->u.compnd.nmembs);
	for(i = 0; i < dt->shared->u.compnd.nmembs; i++) {
	    sprintf(buf, "Member %u:", i);
	    fprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
		    buf,
		    dt->shared->u.compnd.memb[i].name);
	    fprintf(stream, "%*s%-*s %lu\n", indent+3, "", MAX(0, fwidth-3),
		    "Byte offset:",
		    (unsigned long)(dt->shared->u.compnd.memb[i].offset));
	    H5O_dtype_debug(f, dxpl_id, dt->shared->u.compnd.memb[i].type, stream,
			    indent + 3, MAX(0, fwidth - 3));
	}
    } else if(H5T_ENUM == dt->shared->type) {
	fprintf(stream, "%*s%s\n", indent, "", "Base type:");
	H5O_dtype_debug(f, dxpl_id, dt->shared->parent, stream, indent+3, MAX(0, fwidth-3));
	fprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
		"Number of members:",
		dt->shared->u.enumer.nmembs);
	for(i = 0; i < dt->shared->u.enumer.nmembs; i++) {
	    sprintf(buf, "Member %u:", i);
	    fprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
		    buf,
		    dt->shared->u.enumer.name[i]);
	    fprintf(stream, "%*s%-*s 0x", indent, "", fwidth,
		    "Raw bytes of value:");
	    for(k = 0; k < dt->shared->parent->shared->size; k++)
		fprintf(stream, "%02x",
			dt->shared->u.enumer.value[i*dt->shared->parent->shared->size + k]);
	    fprintf(stream, "\n");
	} /* end for */

    } else if(H5T_OPAQUE == dt->shared->type) {
	fprintf(stream, "%*s%-*s \"%s\"\n", indent, "", fwidth,
		"Tag:", dt->shared->u.opaque.tag);
    } else if(H5T_REFERENCE == dt->shared->type) {
	fprintf(stream, "%*s%-*s\n", indent, "", fwidth,
		"Fix dumping reference types!");
    } else if(H5T_STRING == dt->shared->type) {
        switch(dt->shared->u.atomic.u.s.cset) {
            case H5T_CSET_ASCII:
                s = "ASCII";
                break;

            case H5T_CSET_UTF8:
                s = "UTF-8";
                break;

            case H5T_CSET_RESERVED_2:
            case H5T_CSET_RESERVED_3:
            case H5T_CSET_RESERVED_4:
            case H5T_CSET_RESERVED_5:
            case H5T_CSET_RESERVED_6:
            case H5T_CSET_RESERVED_7:
            case H5T_CSET_RESERVED_8:
            case H5T_CSET_RESERVED_9:
            case H5T_CSET_RESERVED_10:
            case H5T_CSET_RESERVED_11:
            case H5T_CSET_RESERVED_12:
            case H5T_CSET_RESERVED_13:
            case H5T_CSET_RESERVED_14:
            case H5T_CSET_RESERVED_15:
                sprintf(buf, "H5T_CSET_RESERVED_%d", (int)(dt->shared->u.atomic.u.s.cset));
                s = buf;
                break;

            case H5T_CSET_ERROR:
            default:
                sprintf(buf, "Unknown character set: %d", (int)(dt->shared->u.atomic.u.s.cset));
                s = buf;
                break;
        } /* end switch */
        fprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
                "Character Set:",
                s);

        switch(dt->shared->u.atomic.u.s.pad) {
            case H5T_STR_NULLTERM:
                s = "NULL Terminated";
                break;

            case H5T_STR_NULLPAD:
                s = "NULL Padded";
                break;

            case H5T_STR_SPACEPAD:
                s = "Space Padded";
                break;

            case H5T_STR_RESERVED_3:
            case H5T_STR_RESERVED_4:
            case H5T_STR_RESERVED_5:
            case H5T_STR_RESERVED_6:
            case H5T_STR_RESERVED_7:
            case H5T_STR_RESERVED_8:
            case H5T_STR_RESERVED_9:
            case H5T_STR_RESERVED_10:
            case H5T_STR_RESERVED_11:
            case H5T_STR_RESERVED_12:
            case H5T_STR_RESERVED_13:
            case H5T_STR_RESERVED_14:
            case H5T_STR_RESERVED_15:
                sprintf(buf, "H5T_STR_RESERVED_%d", (int)(dt->shared->u.atomic.u.s.pad));
                s = buf;
                break;

            case H5T_STR_ERROR:
            default:
                sprintf(buf, "Unknown string padding: %d", (int)(dt->shared->u.atomic.u.s.pad));
                s = buf;
                break;
        } /* end switch */
        fprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
                "String Padding:",
                s);
    } else if(H5T_VLEN == dt->shared->type) {
        switch(dt->shared->u.vlen.type) {
            case H5T_VLEN_SEQUENCE:
                s = "sequence";
                break;

            case H5T_VLEN_STRING:
                s = "string";
                break;

            case H5T_VLEN_BADTYPE:
            case H5T_VLEN_MAXTYPE:
            default:
                sprintf(buf, "H5T_VLEN_%d", dt->shared->u.vlen.type);
                s = buf;
                break;
        } /* end switch */
        fprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
                "Vlen type:", s);

        switch(dt->shared->u.vlen.loc) {
            case H5T_LOC_MEMORY:
                s = "memory";
                break;

            case H5T_LOC_DISK:
                s = "disk";
                break;

            case H5T_LOC_BADLOC:
            case H5T_LOC_MAXLOC:
            default:
                sprintf(buf, "H5T_LOC_%d", (int)dt->shared->u.vlen.loc);
                s = buf;
                break;
        } /* end switch */
        fprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
                "Location:", s);

        /* Extra information for VL-strings */
        if(dt->shared->u.vlen.type == H5T_VLEN_STRING) {
            switch(dt->shared->u.vlen.cset) {
                case H5T_CSET_ASCII:
                    s = "ASCII";
                    break;

                case H5T_CSET_UTF8:
                    s = "UTF-8";
                    break;

                case H5T_CSET_RESERVED_2:
                case H5T_CSET_RESERVED_3:
                case H5T_CSET_RESERVED_4:
                case H5T_CSET_RESERVED_5:
                case H5T_CSET_RESERVED_6:
                case H5T_CSET_RESERVED_7:
                case H5T_CSET_RESERVED_8:
                case H5T_CSET_RESERVED_9:
                case H5T_CSET_RESERVED_10:
                case H5T_CSET_RESERVED_11:
                case H5T_CSET_RESERVED_12:
                case H5T_CSET_RESERVED_13:
                case H5T_CSET_RESERVED_14:
                case H5T_CSET_RESERVED_15:
                    sprintf(buf, "H5T_CSET_RESERVED_%d", (int)(dt->shared->u.vlen.cset));
                    s = buf;
                    break;

                case H5T_CSET_ERROR:
                default:
                    sprintf(buf, "Unknown character set: %d", (int)(dt->shared->u.vlen.cset));
                    s = buf;
                    break;
            } /* end switch */
            fprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
                    "Character Set:",
                    s);

            switch(dt->shared->u.vlen.pad) {
                case H5T_STR_NULLTERM:
                    s = "NULL Terminated";
                    break;

                case H5T_STR_NULLPAD:
                    s = "NULL Padded";
                    break;

                case H5T_STR_SPACEPAD:
                    s = "Space Padded";
                    break;

                case H5T_STR_RESERVED_3:
                case H5T_STR_RESERVED_4:
                case H5T_STR_RESERVED_5:
                case H5T_STR_RESERVED_6:
                case H5T_STR_RESERVED_7:
                case H5T_STR_RESERVED_8:
                case H5T_STR_RESERVED_9:
                case H5T_STR_RESERVED_10:
                case H5T_STR_RESERVED_11:
                case H5T_STR_RESERVED_12:
                case H5T_STR_RESERVED_13:
                case H5T_STR_RESERVED_14:
                case H5T_STR_RESERVED_15:
                    sprintf(buf, "H5T_STR_RESERVED_%d", (int)(dt->shared->u.vlen.pad));
                    s = buf;
                    break;

                case H5T_STR_ERROR:
                default:
                    sprintf(buf, "Unknown string padding: %d", (int)(dt->shared->u.vlen.pad));
                    s = buf;
                    break;
            } /* end switch */
            fprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
                    "String Padding:",
                    s);
        } /* end if */
    } else if(H5T_ARRAY == dt->shared->type) {
	fprintf(stream, "%*s%-*s %u\n", indent, "", fwidth,
		"Rank:",
		dt->shared->u.array.ndims);
        fprintf(stream, "%*s%-*s {", indent, "", fwidth, "Dim Size:");
        for(i = 0; i < dt->shared->u.array.ndims; i++)
            fprintf(stream, "%s%u", (i ? ", " : ""), (unsigned)dt->shared->u.array.dim[i]);
        fprintf(stream, "}\n");
	fprintf(stream, "%*s%s\n", indent, "", "Base type:");
	H5O_dtype_debug(f, dxpl_id, dt->shared->parent, stream, indent + 3, MAX(0, fwidth - 3));
    } else {
	switch (dt->shared->u.atomic.order) {
            case H5T_ORDER_LE:
                s = "little endian";
                break;

            case H5T_ORDER_BE:
                s = "big endian";
                break;

            case H5T_ORDER_VAX:
                s = "VAX";
                break;

            case H5T_ORDER_NONE:
                s = "none";
                break;

            case H5T_ORDER_MIXED:
                s = "mixed";
                break;

            case H5T_ORDER_ERROR:
            default:
                sprintf(buf, "H5T_ORDER_%d", dt->shared->u.atomic.order);
                s = buf;
                break;
	} /* end switch */
	fprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
		"Byte order:",
		s);

	fprintf(stream, "%*s%-*s %lu bit%s\n", indent, "", fwidth,
		"Precision:",
		(unsigned long)(dt->shared->u.atomic.prec),
		1==dt->shared->u.atomic.prec?"":"s");

	fprintf(stream, "%*s%-*s %lu bit%s\n", indent, "", fwidth,
		"Offset:",
		(unsigned long)(dt->shared->u.atomic.offset),
		1==dt->shared->u.atomic.offset?"":"s");

	switch (dt->shared->u.atomic.lsb_pad) {
            case H5T_PAD_ZERO:
                s = "zero";
                break;

            case H5T_PAD_ONE:
                s = "one";
                break;

            case H5T_PAD_BACKGROUND:
                s = "background";
                break;

            case H5T_PAD_ERROR:
            case H5T_NPAD:
            default:
                s = "pad?";
                break;
	} /* end switch */
	fprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
		"Low pad type:", s);

	switch (dt->shared->u.atomic.msb_pad) {
            case H5T_PAD_ZERO:
                s = "zero";
                break;

            case H5T_PAD_ONE:
                s = "one";
                break;

            case H5T_PAD_BACKGROUND:
                s = "background";
                break;

            case H5T_PAD_ERROR:
            case H5T_NPAD:
            default:
                s = "pad?";
                break;
	} /* end switch */
	fprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
		"High pad type:", s);

	if (H5T_FLOAT == dt->shared->type) {
	    switch (dt->shared->u.atomic.u.f.pad) {
                case H5T_PAD_ZERO:
                    s = "zero";
                    break;

                case H5T_PAD_ONE:
                    s = "one";
                    break;

                case H5T_PAD_BACKGROUND:
                    s = "background";
                    break;

                case H5T_PAD_ERROR:
                case H5T_NPAD:
                default:
                    if (dt->shared->u.atomic.u.f.pad < 0)
                        sprintf(buf, "H5T_PAD_%d", -(dt->shared->u.atomic.u.f.pad));
                    else
                        sprintf(buf, "bit-%d", dt->shared->u.atomic.u.f.pad);
                    s = buf;
                    break;
	    } /* end switch */
	    fprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
		    "Internal pad type:", s);

	    switch (dt->shared->u.atomic.u.f.norm) {
                case H5T_NORM_IMPLIED:
                    s = "implied";
                    break;

                case H5T_NORM_MSBSET:
                    s = "msb set";
                    break;

                case H5T_NORM_NONE:
                    s = "none";
                    break;

                case H5T_NORM_ERROR:
                default:
                    sprintf(buf, "H5T_NORM_%d", (int) (dt->shared->u.atomic.u.f.norm));
                    s = buf;
	    } /* end switch */
	    fprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
		    "Normalization:", s);

	    fprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
		    "Sign bit location:",
		    (unsigned long) (dt->shared->u.atomic.u.f.sign));

	    fprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
		    "Exponent location:",
		    (unsigned long) (dt->shared->u.atomic.u.f.epos));

	    fprintf(stream, "%*s%-*s 0x%08lx\n", indent, "", fwidth,
		    "Exponent bias:",
		    (unsigned long) (dt->shared->u.atomic.u.f.ebias));

	    fprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
		    "Exponent size:",
		    (unsigned long) (dt->shared->u.atomic.u.f.esize));

	    fprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
		    "Mantissa location:",
		    (unsigned long) (dt->shared->u.atomic.u.f.mpos));

	    fprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
		    "Mantissa size:",
		    (unsigned long) (dt->shared->u.atomic.u.f.msize));

	} else if (H5T_INTEGER == dt->shared->type) {
	    switch (dt->shared->u.atomic.u.i.sign) {
                case H5T_SGN_NONE:
                    s = "none";
                    break;

                case H5T_SGN_2:
                    s = "2's comp";
                    break;

                case H5T_SGN_ERROR:
                case H5T_NSGN:
                default:
                    sprintf(buf, "H5T_SGN_%d", (int) (dt->shared->u.atomic.u.i.sign));
                    s = buf;
                    break;
	    } /* end switch */
	    fprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
		    "Sign scheme:", s);
	}
    }

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_dtype_debug() */

