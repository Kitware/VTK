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

/****************/
/* Module Setup */
/****************/

#define H5F_PACKAGE		/*suppress error about including H5Fpkg	  */
#define H5G_PACKAGE		/*suppress error about including H5Gpkg	  */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fpkg.h"             /* File access				*/
#include "H5FDprivate.h"	/* File drivers				*/
#include "H5FLprivate.h"        /* Free Lists                           */
#include "H5Gpkg.h"		/* Groups		  		*/
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5MMprivate.h"        /* Memory management                    */
#include "H5Pprivate.h"		/* Property lists			*/
#include "H5SMprivate.h"        /* Shared Object Header Messages        */


/****************/
/* Local Macros */
/****************/

/* Maximum size of super-block buffers */
#define H5F_MAX_SUPERBLOCK_SIZE  134


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/

/* Metadata cache (H5AC) callbacks */
static H5F_super_t *H5F_sblock_load(H5F_t *f, hid_t dxpl_id, haddr_t addr, void *udata);
static herr_t H5F_sblock_flush(H5F_t *f, hid_t dxpl_id, hbool_t destroy, haddr_t addr, H5F_super_t *sblock);
static herr_t H5F_sblock_dest(H5F_t *f, H5F_super_t * sblock);
static herr_t H5F_sblock_clear(H5F_t *f, H5F_super_t *sblock, hbool_t destroy);
static herr_t H5F_sblock_size(const H5F_t *f, const H5F_super_t *sblock, size_t *size_ptr);


/*********************/
/* Package Variables */
/*********************/

/* H5F inherits cache-like properties from H5AC */
const H5AC_class_t H5AC_SUPERBLOCK[1] = {{
    H5AC_SUPERBLOCK_ID,
    (H5AC_load_func_t)H5F_sblock_load,
    (H5AC_flush_func_t)H5F_sblock_flush,
    (H5AC_dest_func_t)H5F_sblock_dest,
    (H5AC_clear_func_t)H5F_sblock_clear,
    (H5AC_size_func_t)H5F_sblock_size,
}};

/*****************************/
/* Library Private Variables */
/*****************************/

/* Declare extern the free list to manage the H5F_super_t struct */
H5FL_EXTERN(H5F_super_t);


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:    H5F_sblock_load
 *
 * Purpose:     Loads the superblock from the file, and deserializes
 *              its information into the H5F_super_t structure.
 *
 * Return:      Success:        SUCCEED
 *              Failure:        NULL
 *
 * Programmer:  Mike McGreevy
 *              mamcgree@hdfgroup.org
 *              April 8, 2009
 *
 *-------------------------------------------------------------------------
 */
static H5F_super_t *
H5F_sblock_load(H5F_t *f, hid_t dxpl_id, haddr_t UNUSED addr, void *_udata)
{
    H5F_super_t        *sblock = NULL;      /* File's superblock */
    haddr_t             base_addr = HADDR_UNDEF;        /* Base address of file */
    uint8_t             sbuf[H5F_MAX_SUPERBLOCK_SIZE];     /* Buffer for superblock */
    H5P_genplist_t     *c_plist;            /* File creation property list  */
    H5F_file_t         *shared;             /* shared part of `file'        */
    H5FD_t             *lf;                 /* file driver part of `shared' */
    haddr_t             stored_eoa;         /*relative end-of-addr in file  */
    haddr_t             eof;                /*end of file address           */
    uint8_t             sizeof_addr;        /* Size of offsets in the file (in bytes) */
    uint8_t             sizeof_size;        /* Size of lengths in the file (in bytes) */
    const size_t        fixed_size = H5F_SUPERBLOCK_FIXED_SIZE; /*fixed sizeof superblock   */
    size_t              variable_size;      /*variable sizeof superblock    */
    uint8_t            *p;                  /* Temporary pointer into encoding buffer */
    unsigned            super_vers;         /* Superblock version          */
    hbool_t            *dirtied = (hbool_t *)_udata;  /* Set up dirtied out value */
    H5F_super_t        *ret_value;          /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check arguments */
    HDassert(f);
    HDassert(H5F_addr_eq(addr, 0));
    HDassert(dirtied);

    /* Short cuts */
    shared = f->shared;
    lf = shared->lf;

    /* Get the shared file creation property list */
    if(NULL == (c_plist = (H5P_genplist_t *)H5I_object(shared->fcpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "can't get property list")

    /* Get the base address for the file in the VFD */
    if(HADDR_UNDEF == (base_addr = H5FD_get_base_addr(lf)))
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, NULL, "failed to get base address for file driver")

    /* Allocate space for the superblock */
    if(NULL == (sblock = H5FL_CALLOC(H5F_super_t)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Read fixed-size portion of the superblock */
    p = sbuf;
    H5_CHECK_OVERFLOW(fixed_size, size_t, haddr_t);
    if(H5FD_set_eoa(lf, H5FD_MEM_SUPER, (haddr_t)fixed_size) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, NULL, "set end of space allocation request failed")
    if(H5FD_read(lf, dxpl_id, H5FD_MEM_SUPER, (haddr_t)0, fixed_size, p) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_READERROR, NULL, "unable to read superblock")

    /* Skip over signature (already checked when locating the superblock) */
    p += H5F_SIGNATURE_LEN;

    /* Superblock version */
    super_vers = *p++;
    if(super_vers > HDF5_SUPERBLOCK_VERSION_LATEST)
        HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, NULL, "bad superblock version number")
    if(H5P_set(c_plist, H5F_CRT_SUPER_VERS_NAME, &super_vers) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, NULL, "unable to set superblock version")

    /* Record the superblock version */
    sblock->super_vers = super_vers;

    /* Sanity check */
    HDassert(((size_t)(p - sbuf)) == fixed_size);

    /* Determine the size of the variable-length part of the superblock */
    variable_size = (size_t)H5F_SUPERBLOCK_VARLEN_SIZE(super_vers, f);
    HDassert(variable_size > 0);
    HDassert(fixed_size + variable_size <= sizeof(sbuf));

    /* Read in variable-sized portion of superblock */
    if(H5FD_set_eoa(lf, H5FD_MEM_SUPER, (haddr_t)(fixed_size + variable_size)) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, NULL, "set end of space allocation request failed")
    if(H5FD_read(lf, dxpl_id, H5FD_MEM_SUPER, (haddr_t)fixed_size, variable_size, p) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "unable to read superblock")

    /* Check for older version of superblock format */
    if(super_vers < HDF5_SUPERBLOCK_VERSION_2) {
        uint32_t	status_flags;	    /* File status flags	   */
        unsigned        btree_k[H5B_NUM_BTREE_ID];  /* B-tree internal node 'K' values */
        unsigned        sym_leaf_k;         /* Symbol table leaf node's 'K' value */

        /* Freespace version (hard-wired) */
        if(HDF5_FREESPACE_VERSION != *p++)
            HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, NULL, "bad free space version number")

        /* Root group version number (hard-wired) */
        if(HDF5_OBJECTDIR_VERSION != *p++)
            HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, NULL, "bad object directory version number")

        /* Skip over reserved byte */
        p++;

        /* Shared header version number (hard-wired) */
        if(HDF5_SHAREDHEADER_VERSION != *p++)
            HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, NULL, "bad shared-header format version number")

        /* Size of file addresses */
        sizeof_addr = *p++;
        if(sizeof_addr != 2 && sizeof_addr != 4 &&
                sizeof_addr != 8 && sizeof_addr != 16 && sizeof_addr != 32)
            HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, NULL, "bad byte number in an address")
        if(H5P_set(c_plist, H5F_CRT_ADDR_BYTE_NUM_NAME, &sizeof_addr) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, NULL, "unable to set byte number in an address")
        shared->sizeof_addr = sizeof_addr;  /* Keep a local copy also */

        /* Size of file sizes */
        sizeof_size = *p++;
        if(sizeof_size != 2 && sizeof_size != 4 &&
                sizeof_size != 8 && sizeof_size != 16 && sizeof_size != 32)
            HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, NULL, "bad byte number for object size")
        if(H5P_set(c_plist, H5F_CRT_OBJ_BYTE_NUM_NAME, &sizeof_size) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, NULL, "unable to set byte number for object size")
        shared->sizeof_size = sizeof_size;  /* Keep a local copy also */

        /* Skip over reserved byte */
        p++;

        /* Various B-tree sizes */
        UINT16DECODE(p, sym_leaf_k);
        if(sym_leaf_k == 0)
            HGOTO_ERROR(H5E_FILE, H5E_BADRANGE, NULL, "bad symbol table leaf node 1/2 rank")
        if(H5P_set(c_plist, H5F_CRT_SYM_LEAF_NAME, &sym_leaf_k) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, NULL, "unable to set rank for symbol table leaf nodes")
        sblock->sym_leaf_k = sym_leaf_k;    /* Keep a local copy also */

        /* Need 'get' call to set other array values */
        if(H5P_get(c_plist, H5F_CRT_BTREE_RANK_NAME, btree_k) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "unable to get rank for btree internal nodes")
        UINT16DECODE(p, btree_k[H5B_SNODE_ID]);
        if(btree_k[H5B_SNODE_ID] == 0)
            HGOTO_ERROR(H5E_FILE, H5E_BADRANGE, NULL, "bad 1/2 rank for btree internal nodes")
        /*
         * Delay setting the value in the property list until we've checked
         * for the indexed storage B-tree internal 'K' value later.
         */

        /* File status flags (not really used yet) */
        UINT32DECODE(p, status_flags);
        HDassert(status_flags <= 255);
        sblock->status_flags = (uint8_t)status_flags;
        if(sblock->status_flags & ~H5F_SUPER_ALL_FLAGS)
            HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, NULL, "bad flag value for superblock")

        /*
         * If the superblock version # is greater than 0, read in the indexed
         * storage B-tree internal 'K' value
         */
        if(super_vers > HDF5_SUPERBLOCK_VERSION_DEF) {
            UINT16DECODE(p, btree_k[H5B_CHUNK_ID]);
            /* Reserved bytes are present only in version 1 */
            if(super_vers == HDF5_SUPERBLOCK_VERSION_1)
                p += 2;   /* reserved */
        } /* end if */
        else
            btree_k[H5B_CHUNK_ID] = HDF5_BTREE_CHUNK_IK_DEF;

        /* Set the B-tree internal node values, etc */
        if(H5P_set(c_plist, H5F_CRT_BTREE_RANK_NAME, btree_k) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, NULL, "unable to set rank for btree internal nodes")
        HDmemcpy(sblock->btree_k, btree_k, sizeof(unsigned) * (size_t)H5B_NUM_BTREE_ID);    /* Keep a local copy also */

        /* Remainder of "variable-sized" portion of superblock */
        H5F_addr_decode(f, (const uint8_t **)&p, &sblock->base_addr/*out*/);
        H5F_addr_decode(f, (const uint8_t **)&p, &sblock->ext_addr/*out*/);
        H5F_addr_decode(f, (const uint8_t **)&p, &stored_eoa/*out*/);
        H5F_addr_decode(f, (const uint8_t **)&p, &sblock->driver_addr/*out*/);

        /* Allocate space for the root group symbol table entry */
        HDassert(!sblock->root_ent);
        if(NULL == (sblock->root_ent = (H5G_entry_t *)H5MM_calloc(sizeof(H5G_entry_t))))
            HGOTO_ERROR(H5E_FILE, H5E_CANTALLOC, NULL, "can't allocate space for root group symbol table entry")

        /* decode the root group symbol table entry */
        if(H5G_ent_decode(f, (const uint8_t **)&p, sblock->root_ent) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTDECODE, NULL, "can't decode root group symbol table entry")

        /* Set the root group address to the correct value */
        sblock->root_addr = sblock->root_ent->header;

        /*
         * Check if superblock address is different from base address and
         * adjust base address and "end of address" address if so.
         */
        if(!H5F_addr_eq(base_addr, sblock->base_addr)) {
            /* Check if the superblock moved earlier in the file */
            if(H5F_addr_lt(base_addr, sblock->base_addr))
                stored_eoa -= (sblock->base_addr - base_addr);
            else
                /* The superblock moved later in the file */
                stored_eoa += (base_addr - sblock->base_addr);

            /* Adjust base address for offsets of the HDF5 data in the file */
            sblock->base_addr = base_addr;

            /* Set the base address for the file in the VFD now */
            if(H5FD_set_base_addr(lf, sblock->base_addr) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTSET, NULL, "failed to set base address for file driver")

            /* Indicate that the superblock should be marked dirty */
            *dirtied = TRUE;
        } /* end if */

        /* This step is for h5repart tool only. If user wants to change file driver
         *  from family to sec2 while using h5repart, set the driver address to
         *  undefined to let the library ignore the family driver information saved
         *  in the superblock.
         */
        if(H5F_HAS_FEATURE(f, H5FD_FEAT_IGNORE_DRVRINFO)) {
            /* Eliminate the driver info */
            sblock->driver_addr = HADDR_UNDEF;

            /* Indicate that the superblock should be marked dirty */
            *dirtied = TRUE;
        } /* end if */

        /* Decode the optional driver information block */
        if(H5F_addr_defined(sblock->driver_addr)) {
            uint8_t dbuf[H5F_MAX_DRVINFOBLOCK_SIZE];     /* Buffer for driver info block */
            char drv_name[9];       /* Name of driver */
            unsigned drv_vers;      /* Version of driver info block */
            size_t drv_variable_size; /* Size of variable-length portion of driver info block, in bytes */

            /* Read in fixed-sized portion of driver info block */
            p = dbuf;
            if(H5FD_set_eoa(lf, H5FD_MEM_SUPER, sblock->driver_addr + H5F_DRVINFOBLOCK_HDR_SIZE) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, NULL, "set end of space allocation request failed")
            if(H5FD_read(lf, dxpl_id, H5FD_MEM_SUPER, sblock->driver_addr, (size_t)H5F_DRVINFOBLOCK_HDR_SIZE, p) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "unable to read driver information block")

            /* Version number */
            drv_vers = *p++;
            if(drv_vers != HDF5_DRIVERINFO_VERSION_0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "bad driver information block version number")

            p += 3; /* reserved bytes */

            /* Driver info size */
            UINT32DECODE(p, drv_variable_size);

            /* Sanity check */
            HDassert(H5F_DRVINFOBLOCK_HDR_SIZE + drv_variable_size <= sizeof(dbuf));

            /* Driver name and/or version */
            HDstrncpy(drv_name, (const char *)p, (size_t)8);
            drv_name[8] = '\0';
            p += 8; /* advance past name/version */

            /* Check if driver matches driver information saved. Unfortunately, we can't push this
             * function to each specific driver because we're checking if the driver is correct.
             */
            if(!HDstrncmp(drv_name, "NCSAfami", (size_t)8) && HDstrcmp(lf->cls->name, "family"))
                HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "family driver should be used")
            if(!HDstrncmp(drv_name, "NCSAmult", (size_t)8) && HDstrcmp(lf->cls->name, "multi"))
                HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "multi driver should be used")

            /* Read in variable-sized portion of driver info block */
            if(H5FD_set_eoa(lf, H5FD_MEM_SUPER, sblock->driver_addr + H5F_DRVINFOBLOCK_HDR_SIZE + drv_variable_size) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, NULL, "set end of space allocation request failed")
            if(H5FD_read(lf, dxpl_id, H5FD_MEM_SUPER, sblock->driver_addr + H5F_DRVINFOBLOCK_HDR_SIZE, drv_variable_size, p) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "unable to read file driver information")

            /* Decode driver information */
            if(H5FD_sb_decode(lf, drv_name, p) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "unable to decode driver information")
        } /* end if */
    } /* end if */
    else {
        uint32_t computed_chksum;       /* Computed checksum  */
        uint32_t read_chksum;           /* Checksum read from file  */

        /* Size of file addresses */
        sizeof_addr = *p++;
        if(sizeof_addr != 2 && sizeof_addr != 4 &&
                sizeof_addr != 8 && sizeof_addr != 16 && sizeof_addr != 32)
            HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, NULL, "bad byte number in an address")
        if(H5P_set(c_plist, H5F_CRT_ADDR_BYTE_NUM_NAME, &sizeof_addr) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, NULL, "unable to set byte number in an address")
        shared->sizeof_addr = sizeof_addr;  /* Keep a local copy also */

        /* Size of file sizes */
        sizeof_size = *p++;
        if(sizeof_size != 2 && sizeof_size != 4 &&
                sizeof_size != 8 && sizeof_size != 16 && sizeof_size != 32)
            HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, NULL, "bad byte number for object size")
        if(H5P_set(c_plist, H5F_CRT_OBJ_BYTE_NUM_NAME, &sizeof_size) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, NULL, "unable to set byte number for object size")
        shared->sizeof_size = sizeof_size;  /* Keep a local copy also */

        /* File status flags (not really used yet) */
        sblock->status_flags = *p++;
        if(sblock->status_flags & ~H5F_SUPER_ALL_FLAGS)
            HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, NULL, "bad flag value for superblock")

        /* Base, superblock extension, end of file & root group object header addresses */
        H5F_addr_decode(f, (const uint8_t **)&p, &sblock->base_addr/*out*/);
        H5F_addr_decode(f, (const uint8_t **)&p, &sblock->ext_addr/*out*/);
        H5F_addr_decode(f, (const uint8_t **)&p, &stored_eoa/*out*/);
        H5F_addr_decode(f, (const uint8_t **)&p, &sblock->root_addr/*out*/);

        /* Compute checksum for superblock */
        computed_chksum = H5_checksum_metadata(sbuf, (size_t)(p - sbuf), 0);

        /* Decode checksum */
        UINT32DECODE(p, read_chksum);

        /* Verify correct checksum */
        if(read_chksum != computed_chksum)
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "bad checksum on driver information block")

        /*
         * Check if superblock address is different from base address and
         * adjust base address and "end of address" address if so.
         */
        if(!H5F_addr_eq(base_addr, sblock->base_addr)) {
            /* Check if the superblock moved earlier in the file */
            if(H5F_addr_lt(base_addr, sblock->base_addr))
                stored_eoa -= (sblock->base_addr - base_addr);
            else
                /* The superblock moved later in the file */
                stored_eoa += (base_addr - sblock->base_addr);

            /* Adjust base address for offsets of the HDF5 data in the file */
            sblock->base_addr = base_addr;

            /* Set the base address for the file in the VFD now */
            if(H5FD_set_base_addr(lf, sblock->base_addr) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTSET, NULL, "failed to set base address for file driver")

            /* Indicate that the superblock should be marked dirty */
            *dirtied = TRUE;
        } /* end if */

        /* Get the B-tree internal node values, etc */
        if(H5P_get(c_plist, H5F_CRT_BTREE_RANK_NAME, sblock->btree_k) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "unable to get rank for btree internal nodes")
        if(H5P_get(c_plist, H5F_CRT_SYM_LEAF_NAME, &sblock->sym_leaf_k) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, NULL, "unable to get rank for btree internal nodes")
    } /* end else */

    /*
     * The user-defined data is the area of the file before the base
     * address.
     */
    if(H5P_set(c_plist, H5F_CRT_USER_BLOCK_NAME, &sblock->base_addr) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, NULL, "unable to set userblock size")

    /*
     * Make sure that the data is not truncated. One case where this is
     * possible is if the first file of a family of files was opened
     * individually.
     */
    if(HADDR_UNDEF == (eof = H5FD_get_eof(lf)))
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "unable to determine file size")

    /* (Account for the stored EOA being absolute offset -QAK) */
    if((eof + sblock->base_addr) < stored_eoa)
        HGOTO_ERROR(H5E_FILE, H5E_TRUNCATED, NULL, "truncated file: eof = %llu, sblock->base_addr = %llu, stored_eoa = %llu", (unsigned long long)eof, (unsigned long long)sblock->base_addr, (unsigned long long)stored_eoa)

    /*
     * Tell the file driver how much address space has already been
     * allocated so that it knows how to allocate additional memory.
     */
    /* (Account for the stored EOA being absolute offset -NAF) */
    if(H5FD_set_eoa(lf, H5FD_MEM_SUPER, stored_eoa - sblock->base_addr) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "unable to set end-of-address marker for file")

    /* Read the file's superblock extension, if there is one. */
    if(H5F_addr_defined(sblock->ext_addr)) {
        H5O_loc_t ext_loc;      /* "Object location" for superblock extension */
        H5O_btreek_t btreek;    /* v1 B-tree 'K' value message from superblock extension */
        H5O_drvinfo_t drvinfo;  /* Driver info message from superblock extension */
        htri_t status;          /* Status for message existing */

        /* Sanity check - superblock extension should only be defined for
         *      superblock version >= 2.
         */
        HDassert(super_vers >= HDF5_SUPERBLOCK_VERSION_2);

        /* Check for superblock extension being located "outside" the stored
         *      'eoa' value, which can occur with the split/multi VFD.
         */
        if(H5F_addr_gt(sblock->ext_addr, stored_eoa)) {
            /* Set the 'eoa' for the object header memory type large enough
             *  to give some room for a reasonably sized superblock extension.
             *  (This is _rather_ a kludge -QAK)
             */
            if(H5FD_set_eoa(lf, H5FD_MEM_OHDR, (haddr_t)(sblock->ext_addr + 1024)) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "unable to set end-of-address marker for file")
        } /* end if */

        /* Open the superblock extension */
	if(H5F_super_ext_open(f, sblock->ext_addr, &ext_loc) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENOBJ, NULL, "unable to open file's superblock extension")

        /* Check for the extension having a 'driver info' message */
        if((status = H5O_msg_exists(&ext_loc, H5O_DRVINFO_ID, dxpl_id)) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTGET, NULL, "unable to read object header")
        if(status) {
            /* Check for ignoring the driver info for this file */
            if(H5F_HAS_FEATURE(f, H5FD_FEAT_IGNORE_DRVRINFO)) {
                /* Indicate that the superblock should be marked dirty */
                *dirtied = TRUE;
            } /* end if */
            else {
                /* Retrieve the 'driver info' structure */
                if(NULL == H5O_msg_read(&ext_loc, H5O_DRVINFO_ID, &drvinfo, dxpl_id))
                    HGOTO_ERROR(H5E_FILE, H5E_CANTGET, NULL, "driver info message not present")

                /* Check if driver matches driver information saved. Unfortunately, we can't push this
                 * function to each specific driver because we're checking if the driver is correct.
                 */
                if(!HDstrncmp(drvinfo.name, "NCSAfami", (size_t)8) && HDstrcmp(lf->cls->name, "family"))
                    HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "family driver should be used")
                if(!HDstrncmp(drvinfo.name, "NCSAmult", (size_t)8) && HDstrcmp(lf->cls->name, "multi"))
                    HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "multi driver should be used")

                /* Decode driver information */
                if(H5FD_sb_decode(lf, drvinfo.name, drvinfo.buf) < 0)
                    HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "unable to decode driver information")

                /* Reset driver info message */
                H5O_msg_reset(H5O_DRVINFO_ID, &drvinfo);
            } /* end else */
        } /* end if */

        /* Read in the shared OH message information if there is any */
        if(H5SM_get_info(&ext_loc, c_plist, dxpl_id) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, NULL, "unable to read SOHM table information")

        /* Check for the extension having a 'v1 B-tree "K"' message */
        if((status = H5O_msg_exists(&ext_loc, H5O_BTREEK_ID, dxpl_id)) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTGET, NULL, "unable to read object header")
        if(status) {
            /* Retrieve the 'v1 B-tree "K"' structure */
            if(NULL == H5O_msg_read(&ext_loc, H5O_BTREEK_ID, &btreek, dxpl_id))
                HGOTO_ERROR(H5E_FILE, H5E_CANTGET, NULL, "v1 B-tree 'K' info message not present")

            /* Set non-default v1 B-tree 'K' value info from file */
            sblock->btree_k[H5B_CHUNK_ID] = btreek.btree_k[H5B_CHUNK_ID];
            sblock->btree_k[H5B_SNODE_ID] = btreek.btree_k[H5B_SNODE_ID];
            sblock->sym_leaf_k = btreek.sym_leaf_k;

            /* Set non-default v1 B-tree 'K' values in the property list */
            if(H5P_set(c_plist, H5F_CRT_BTREE_RANK_NAME, btreek.btree_k) < 0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, NULL, "unable to set rank for btree internal nodes")
            if(H5P_set(c_plist, H5F_CRT_SYM_LEAF_NAME, &btreek.sym_leaf_k) < 0)
                HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, NULL, "unable to set rank for symbol table leaf nodes")
        } /* end if */

        /* Close superblock extension */
        if(H5F_super_ext_close(f, &ext_loc, dxpl_id, FALSE) < 0)
	    HGOTO_ERROR(H5E_FILE, H5E_CANTRELEASE, NULL, "unable to close file's superblock extension")
    } /* end if */

    /* Set return value */
    ret_value = sblock;

done:
    /* Release the [possibly partially initialized] superblock on errors */
    if(!ret_value && sblock)
        if(H5F_super_free(sblock) < 0)
            HDONE_ERROR(H5E_FILE, H5E_CANTFREE, NULL, "unable to destroy superblock data")

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5F_sblock_load() */


/*-------------------------------------------------------------------------
 * Function:    H5F_sblock_flush
 *
 * Purpose:     Flushes the superblock.
 *
 * Return:      Success:        SUCCEED
 *              Failure:        NULL
 *
 * Programmer:  Mike McGreevy
 *              mamcgree@hdfgroup.org
 *              April 8, 2009
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_sblock_flush(H5F_t *f, hid_t dxpl_id, hbool_t destroy, haddr_t UNUSED addr,
    H5F_super_t *sblock)
{
    herr_t          ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT

    /* check arguments */
    HDassert(f);
    HDassert(H5F_addr_eq(addr, 0));
    HDassert(sblock);

    if(sblock->cache_info.is_dirty) {
        uint8_t         buf[H5F_MAX_SUPERBLOCK_SIZE + H5F_MAX_DRVINFOBLOCK_SIZE];  /* Superblock & driver info blockencoding buffer */
        uint8_t        *p;                          /* Ptr into encoding buffer */
        haddr_t         rel_eoa;                    /* Relative EOA for file */
        size_t          superblock_size;            /* Size of superblock, in bytes */
        size_t          driver_size;                /* Size of driver info block (bytes)*/

        /* Encode the common portion of the file superblock for all versions */
        p = buf;
        HDmemcpy(p, H5F_SIGNATURE, (size_t)H5F_SIGNATURE_LEN);
        p += H5F_SIGNATURE_LEN;
        *p++ = (uint8_t)sblock->super_vers;

        /* Check for older version of superblock format */
        if(sblock->super_vers < HDF5_SUPERBLOCK_VERSION_2) {
            *p++ = (uint8_t)HDF5_FREESPACE_VERSION;     /* (hard-wired) */
            *p++ = (uint8_t)HDF5_OBJECTDIR_VERSION;     /* (hard-wired) */
            *p++ = 0;   /* reserved*/

            *p++ = (uint8_t)HDF5_SHAREDHEADER_VERSION;  /* (hard-wired) */
            *p++ = (uint8_t)H5F_SIZEOF_ADDR(f);
            *p++ = (uint8_t)H5F_SIZEOF_SIZE(f);
            *p++ = 0;   /* reserved */

            UINT16ENCODE(p, sblock->sym_leaf_k);
            UINT16ENCODE(p, sblock->btree_k[H5B_SNODE_ID]);
            UINT32ENCODE(p, (uint32_t)sblock->status_flags);

            /*
             * Versions of the superblock >0 have the indexed storage B-tree
             * internal 'K' value stored
             */
            if(sblock->super_vers > HDF5_SUPERBLOCK_VERSION_DEF) {
                UINT16ENCODE(p, sblock->btree_k[H5B_CHUNK_ID]);
                *p++ = 0;   /*reserved */
                *p++ = 0;   /*reserved */
            } /* end if */

            H5F_addr_encode(f, &p, sblock->base_addr);
            H5F_addr_encode(f, &p, sblock->ext_addr);
            rel_eoa = H5FD_get_eoa(f->shared->lf, H5FD_MEM_SUPER);
            H5F_addr_encode(f, &p, (rel_eoa + sblock->base_addr));
            H5F_addr_encode(f, &p, sblock->driver_addr);

            /* Encode the root group object entry, including the cached stab info */
            if(H5G_ent_encode(f, &p, sblock->root_ent) < 0)
                HGOTO_ERROR(H5E_FILE, H5E_CANTENCODE, FAIL, "can't encode root group symbol table entry")

            /* Encode the driver information block. */
            H5_ASSIGN_OVERFLOW(driver_size, H5FD_sb_size(f->shared->lf), hsize_t, size_t);

            /* Checking whether driver block address is defined here is to handle backward
             * compatibility.  If the file was created with v1.6 library or earlier and no
             * driver info block was written in the superblock, we don't write it either even
             * though there's some driver info.  Otherwise, the driver block extended will
             * overwrite the (meta)data right after the superblock. This situation happens to
             * the family driver particularly.  SLU - 2009/3/24
             */
            if(driver_size > 0 && H5F_addr_defined(sblock->driver_addr)) {
                char driver_name[9];    /* Name of driver, for driver info block */
                uint8_t *dbuf = p;      /* Pointer to beginning of driver info */

                /* Encode the driver information block */
                *p++ = HDF5_DRIVERINFO_VERSION_0; /* Version */
                *p++ = 0; /* reserved */
                *p++ = 0; /* reserved */
                *p++ = 0; /* reserved */

                /* Driver info size, excluding header */
                UINT32ENCODE(p, driver_size);

                /* Encode driver-specific data */
                if(H5FD_sb_encode(f->shared->lf, driver_name, dbuf + H5F_DRVINFOBLOCK_HDR_SIZE) < 0)
                    HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "unable to encode driver information")

                /* Store driver name (set in 'H5FD_sb_encode' call above) */
                HDmemcpy(p, driver_name, (size_t)8);

                /* Advance buffer pointer past name & variable-sized portion of driver info */
                /* (for later use in computing the superblock size) */
                p += 8 + driver_size;
            } /* end if */
        } /* end if */
        else {
            uint32_t        chksum;                 /* Checksum temporary variable      */
            H5O_loc_t       *root_oloc;             /* Pointer to root group's object location */

            /* Size of file addresses & offsets, and status flags */
            *p++ = (uint8_t)H5F_SIZEOF_ADDR(f);
            *p++ = (uint8_t)H5F_SIZEOF_SIZE(f);
            *p++ = sblock->status_flags;

            /* Base, superblock extension & end of file addresses */
            H5F_addr_encode(f, &p, sblock->base_addr);
            H5F_addr_encode(f, &p, sblock->ext_addr);
            rel_eoa = H5FD_get_eoa(f->shared->lf, H5FD_MEM_SUPER);
            H5F_addr_encode(f, &p, (rel_eoa + sblock->base_addr));

            /* Retrieve information for root group */
            if(NULL == (root_oloc = H5G_oloc(f->shared->root_grp)))
                HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "unable to retrieve root group information")

            /* Encode address of root group's object header */
            H5F_addr_encode(f, &p, root_oloc->addr);

            /* Compute superblock checksum */
            chksum = H5_checksum_metadata(buf, ((size_t)H5F_SUPERBLOCK_SIZE(sblock->super_vers, f) - H5F_SIZEOF_CHKSUM), 0);

            /* Superblock checksum */
            UINT32ENCODE(p, chksum);

            /* Sanity check */
            HDassert((size_t)(p - buf) == (size_t)H5F_SUPERBLOCK_SIZE(sblock->super_vers, f));
        } /* end else */

        /* Retrieve the total size of the superblock info */
        H5_ASSIGN_OVERFLOW(superblock_size, (p - buf), ptrdiff_t, size_t);

        /* Double check we didn't overrun the block (unlikely) */
        HDassert(superblock_size <= sizeof(buf));

        /* Write superblock */
        /* (always at relative address 0) */
        if(H5FD_write(f->shared->lf, dxpl_id, H5FD_MEM_SUPER, (haddr_t)0, superblock_size, buf) < 0)
            HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "unable to write superblock")

        /* Check for newer version of superblock format & superblock extension */
        if(sblock->super_vers >= HDF5_SUPERBLOCK_VERSION_2 && H5F_addr_defined(sblock->ext_addr)) {
            /* Check for ignoring the driver info for this file */
            if(!H5F_HAS_FEATURE(f, H5FD_FEAT_IGNORE_DRVRINFO)) {
                /* Check for driver info message */
                H5_ASSIGN_OVERFLOW(driver_size, H5FD_sb_size(f->shared->lf), hsize_t, size_t);
                if(driver_size > 0) {
                    H5O_drvinfo_t drvinfo;      /* Driver info */
                    H5O_loc_t 	ext_loc; 	/* "Object location" for superblock extension */
                    uint8_t dbuf[H5F_MAX_DRVINFOBLOCK_SIZE];  /* Driver info block encoding buffer */

                    /* Sanity check */
                    HDassert(driver_size <= H5F_MAX_DRVINFOBLOCK_SIZE);

                    /* Encode driver-specific data */
                    if(H5FD_sb_encode(f->shared->lf, drvinfo.name, dbuf) < 0)
                        HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "unable to encode driver information")

                    /* Open the superblock extension's object header */
                    if(H5F_super_ext_open(f, sblock->ext_addr, &ext_loc) < 0)
                        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENOBJ, FAIL, "unable to open file's superblock extension")

                    /* Write driver info information to the superblock extension */
                    drvinfo.len = driver_size;
                    drvinfo.buf = dbuf;
                    if(H5O_msg_write(&ext_loc, H5O_DRVINFO_ID, H5O_MSG_FLAG_DONTSHARE, H5O_UPDATE_TIME, &drvinfo, dxpl_id) < 0)
                        HGOTO_ERROR(H5E_FILE, H5E_WRITEERROR, FAIL, "unable to update driver info header message")

                    /* Close the superblock extension object header */
                    if(H5F_super_ext_close(f, &ext_loc, dxpl_id, FALSE) < 0)
                        HGOTO_ERROR(H5E_FILE, H5E_CANTCLOSEOBJ, FAIL, "unable to close file's superblock extension")
                } /* end if */
            } /* end if */
        } /* end if */

        /* Reset the dirty flag.  */
        sblock->cache_info.is_dirty = FALSE;
    } /* end if */

    if(destroy)
        if(H5F_sblock_dest(f, sblock) < 0)
            HGOTO_ERROR(H5E_FSPACE, H5E_CLOSEERROR, FAIL, "can't close superblock")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5F_sblock_flush() */


/*-------------------------------------------------------------------------
 * Function:    H5F_sblock_dest
 *
 * Purpose:     Frees memory used by the superblock.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Mike McGreevy
 *              April 8, 2009
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_sblock_dest(H5F_t UNUSED *f, H5F_super_t* sblock)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* Sanity check */
    HDassert(sblock);

    /* Free superblock */
    if(H5F_super_free(sblock) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTFREE, FAIL, "unable to destroy superblock")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5F_sblock_dest() */


/*-------------------------------------------------------------------------
 * Function:    H5F_sblock_clear
 *
 * Purpose:     Mark the superblock as no longer being dirty.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Mike McGreevy
 *              April 8, 2009
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_sblock_clear(H5F_t *f, H5F_super_t *sblock, hbool_t destroy)
{
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /*
     * Check arguments.
     */
    HDassert(sblock);

    /* Reset the dirty flag.  */
    sblock->cache_info.is_dirty = FALSE;

    if(destroy)
        if(H5F_sblock_dest(f, sblock) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTFREE, FAIL, "unable to delete superblock")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5F_sblock_clear() */


/*-------------------------------------------------------------------------
 * Function:    H5F_sblock_size
 *
 * Purpose:     Returns the size of the superblock encoded on disk.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Mike McGreevy
 *              April 8, 2009
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5F_sblock_size(const H5F_t *f, const H5F_super_t *sblock, size_t *size_ptr)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* check arguments */
    HDassert(f);
    HDassert(sblock);
    HDassert(size_ptr);

    /* Set size value */
    *size_ptr = (size_t)H5F_SUPERBLOCK_SIZE(sblock->super_vers, f);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5F_sblock_size() */

