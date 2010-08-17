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
 * Created:		H5Pfcpl.c
 *			January  6 1998
 *			Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:		File creation property list class routines
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
#include "H5Bprivate.h"		/* B-tree subclass names		*/
#include "H5Eprivate.h"		/* Error handling			*/
#include "H5Fprivate.h"		/* Files				*/
#include "H5SMprivate.h"        /* Shared object header messages	*/
#include "H5Ppkg.h"		/* Property lists		 	*/


/****************/
/* Local Macros */
/****************/

/* ========= File Creation properties ============ */
/* Definitions for the size of the file user block in bytes */
#define H5F_CRT_USER_BLOCK_SIZE      sizeof(hsize_t)
#define H5F_CRT_USER_BLOCK_DEF       0
/* Definitions for the 1/2 rank for symbol table leaf nodes */
#define H5F_CRT_SYM_LEAF_SIZE        sizeof(unsigned)
/* Definitions for the 1/2 rank for btree internal nodes    */
#define H5F_CRT_BTREE_RANK_SIZE      sizeof(unsigned[H5B_NUM_BTREE_ID])
#define H5F_CRT_BTREE_RANK_DEF       {HDF5_BTREE_SNODE_IK_DEF,HDF5_BTREE_CHUNK_IK_DEF}
/* Definitions for byte number in an address                */
#define H5F_CRT_ADDR_BYTE_NUM_SIZE   sizeof(uint8_t)
#define H5F_CRT_ADDR_BYTE_NUM_DEF    H5F_OBJ_ADDR_SIZE
/* Definitions for byte number for object size              */
#define H5F_CRT_OBJ_BYTE_NUM_SIZE     sizeof(uint8_t)
#define H5F_CRT_OBJ_BYTE_NUM_DEF      H5F_OBJ_SIZE_SIZE
/* Definitions for version number of the superblock         */
#define H5F_CRT_SUPER_VERS_SIZE       sizeof(unsigned)
#define H5F_CRT_SUPER_VERS_DEF        HDF5_SUPERBLOCK_VERSION_DEF
/* Definitions for shared object header messages */
#define H5F_CRT_SHMSG_NINDEXES_SIZE    sizeof(unsigned)
#define H5F_CRT_SHMSG_NINDEXES_DEF     (0)
#define H5F_CRT_SHMSG_INDEX_TYPES_SIZE sizeof(unsigned[H5O_SHMESG_MAX_NINDEXES])
#define H5F_CRT_SHMSG_INDEX_TYPES_DEF  {0,0,0,0,0,0}
#define H5F_CRT_SHMSG_INDEX_MINSIZE_SIZE sizeof(unsigned[H5O_SHMESG_MAX_NINDEXES])
#define H5F_CRT_SHMSG_INDEX_MINSIZE_DEF {250,250,250,250,250,250}
/* Definitions for shared object header list/btree phase change cutoffs */
#define H5F_CRT_SHMSG_LIST_MAX_SIZE     sizeof(unsigned)
#define H5F_CRT_SHMSG_LIST_MAX_DEF      (50)
#define H5F_CRT_SHMSG_BTREE_MIN_SIZE    sizeof(unsigned)
#define H5F_CRT_SHMSG_BTREE_MIN_DEF     (40)


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
static herr_t H5P_fcrt_reg_prop(H5P_genclass_t *pclass);

/*********************/
/* Package Variables */
/*********************/

/* File creation property list class library initialization object */
const H5P_libclass_t H5P_CLS_FCRT[1] = {{
    "file create",		/* Class name for debugging     */
    &H5P_CLS_GROUP_CREATE_g,	/* Parent class ID              */
    &H5P_CLS_FILE_CREATE_g,	/* Pointer to class ID          */
    &H5P_LST_FILE_CREATE_g,	/* Pointer to default property list ID */
    H5P_fcrt_reg_prop,		/* Default property registration routine */
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


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:    H5P_fcrt_reg_prop
 *
 * Purpose:     Register the file creation property list class's properties
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              October 31, 2006
 *-------------------------------------------------------------------------
 */
static herr_t
H5P_fcrt_reg_prop(H5P_genclass_t *pclass)
{
    hsize_t userblock_size = H5F_CRT_USER_BLOCK_DEF;    /* Default userblock size */
    unsigned sym_leaf_k = H5F_CRT_SYM_LEAF_DEF;         /* Default size for symbol table leaf nodes */
    unsigned btree_k[H5B_NUM_BTREE_ID] = H5F_CRT_BTREE_RANK_DEF;    /* Default 'K' values for B-trees in file */
    uint8_t sizeof_addr = H5F_CRT_ADDR_BYTE_NUM_DEF;     /* Default size of addresses in the file */
    uint8_t sizeof_size = H5F_CRT_OBJ_BYTE_NUM_DEF;      /* Default size of sizes in the file */
    unsigned superblock_ver = H5F_CRT_SUPER_VERS_DEF;   /* Default superblock version # */
    unsigned num_sohm_indexes    = H5F_CRT_SHMSG_NINDEXES_DEF;
    unsigned sohm_index_flags[H5O_SHMESG_MAX_NINDEXES]    = H5F_CRT_SHMSG_INDEX_TYPES_DEF;
    unsigned sohm_index_minsizes[H5O_SHMESG_MAX_NINDEXES] = H5F_CRT_SHMSG_INDEX_MINSIZE_DEF;
    unsigned sohm_list_max  = H5F_CRT_SHMSG_LIST_MAX_DEF;
    unsigned sohm_btree_min  = H5F_CRT_SHMSG_BTREE_MIN_DEF;
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5P_fcrt_reg_prop)

    /* Register the user block size */
    if(H5P_register_real(pclass, H5F_CRT_USER_BLOCK_NAME, H5F_CRT_USER_BLOCK_SIZE, &userblock_size, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
         HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the 1/2 rank for symbol table leaf nodes */
    if(H5P_register_real(pclass, H5F_CRT_SYM_LEAF_NAME, H5F_CRT_SYM_LEAF_SIZE, &sym_leaf_k, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
         HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the 1/2 rank for btree internal nodes */
    if(H5P_register_real(pclass, H5F_CRT_BTREE_RANK_NAME, H5F_CRT_BTREE_RANK_SIZE, btree_k, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
         HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the byte number for an address */
    if(H5P_register_real(pclass, H5F_CRT_ADDR_BYTE_NUM_NAME, H5F_CRT_ADDR_BYTE_NUM_SIZE, &sizeof_addr, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
         HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the byte number for object size */
    if(H5P_register_real(pclass, H5F_CRT_OBJ_BYTE_NUM_NAME, H5F_CRT_OBJ_BYTE_NUM_SIZE, &sizeof_size, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
         HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the superblock version number */
    if(H5P_register_real(pclass, H5F_CRT_SUPER_VERS_NAME, H5F_CRT_SUPER_VERS_SIZE, &superblock_ver, NULL, NULL, NULL, NULL, NULL, NULL, NULL) < 0)
         HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the shared OH message information */
    if(H5P_register_real(pclass,H5F_CRT_SHMSG_NINDEXES_NAME, H5F_CRT_SHMSG_NINDEXES_SIZE, &num_sohm_indexes,NULL,NULL,NULL,NULL,NULL,NULL,NULL)<0)
         HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")
    if(H5P_register_real(pclass,H5F_CRT_SHMSG_INDEX_TYPES_NAME, H5F_CRT_SHMSG_INDEX_TYPES_SIZE, &sohm_index_flags,NULL,NULL,NULL,NULL,NULL,NULL,NULL)<0)
         HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")
    if(H5P_register_real(pclass,H5F_CRT_SHMSG_INDEX_MINSIZE_NAME, H5F_CRT_SHMSG_INDEX_MINSIZE_SIZE, &sohm_index_minsizes,NULL,NULL,NULL,NULL,NULL,NULL,NULL)<0)
         HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

    /* Register the shared OH cutoff size information */
    if(H5P_register_real(pclass,H5F_CRT_SHMSG_LIST_MAX_NAME, H5F_CRT_SHMSG_LIST_MAX_SIZE, &sohm_list_max,NULL,NULL,NULL,NULL,NULL,NULL,NULL)<0)
         HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")
    if(H5P_register_real(pclass,H5F_CRT_SHMSG_BTREE_MIN_NAME, H5F_CRT_SHMSG_BTREE_MIN_SIZE, &sohm_btree_min,NULL,NULL,NULL,NULL,NULL,NULL,NULL)<0)
         HGOTO_ERROR(H5E_PLIST, H5E_CANTINSERT, FAIL, "can't insert property into class")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5P_fcrt_reg_prop() */


/*-------------------------------------------------------------------------
 * Function:	H5Pget_version
 *
 * Purpose:	Retrieves version information for various parts of a file.
 *
 *		SUPER:		The file super block.
 *		FREELIST:	The global free list.
 *		STAB:		The root symbol table entry.
 *		SHHDR:		Shared object headers.
 *
 *		Any (or even all) of the output arguments can be null
 *		pointers.
 *
 * Return:	Success:	Non-negative, version information is returned
 *				through the arguments.
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *		Wednesday, January  7, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_version(hid_t plist_id, unsigned *super/*out*/, unsigned *freelist/*out*/,
    unsigned *stab/*out*/, unsigned *shhdr/*out*/)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_API(H5Pget_version, FAIL)
    H5TRACE5("e", "ixxxx", plist_id, super, freelist, stab, shhdr);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Get values */
    if(super)
        if(H5P_get(plist, H5F_CRT_SUPER_VERS_NAME, super) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get superblock version")
    if(freelist)
        *freelist = HDF5_FREESPACE_VERSION;     /* (hard-wired) */
    if(stab)
        *stab = HDF5_OBJECTDIR_VERSION;         /* (hard-wired) */
    if(shhdr)
        *shhdr = HDF5_SHAREDHEADER_VERSION;     /* (hard-wired) */

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pget_version() */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_userblock
 *
 * Purpose:	Sets the userblock size field of a file creation property
 *		list.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Tuesday, January  6, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_userblock(hid_t plist_id, hsize_t size)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value = SUCCEED; /* return value */

    FUNC_ENTER_API(H5Pset_userblock, FAIL)
    H5TRACE2("e", "ih", plist_id, size);

    /* Sanity check non-zero userblock sizes */
    if(size > 0) {
        /* Check that the userblock size is >=512 */
        if(size < 512)
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "userblock size is non-zero and less than 512")

        /* Check that the userblock size is a power of two */
        if(!POWER_OF_TWO(size))
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "userblock size is non-zero and not a power of two")
    } /* end if */

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_FILE_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Set value */
    if(H5P_set(plist, H5F_CRT_USER_BLOCK_NAME, &size) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set user block")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pset_userblock() */


/*-------------------------------------------------------------------------
 * Function:	H5Pget_userblock
 *
 * Purpose:	Queries the size of a user block in a file creation property
 *		list.
 *
 * Return:	Success:	Non-negative, size returned through SIZE argument.
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *		Wednesday, January  7, 1998
 *
 * Modifications:
 *
 *		Raymond Lu, Oct 14, 2001
 *		Changed to the new generic property list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_userblock(hid_t plist_id, hsize_t *size)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value=SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pget_userblock, FAIL);
    H5TRACE2("e", "i*h", plist_id, size);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Get value */
    if (size)
        if(H5P_get(plist, H5F_CRT_USER_BLOCK_NAME, size) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL,"can't get user block");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pset_sizes
 *
 * Purpose:	Sets file size-of addresses and sizes.	PLIST_ID should be a
 *		file creation property list.  A value of zero causes the
 *		property to not change.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Tuesday, January  6, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_sizes(hid_t plist_id, size_t sizeof_addr, size_t sizeof_size)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value = SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pset_sizes, FAIL)
    H5TRACE3("e", "izz", plist_id, sizeof_addr, sizeof_size);

    /* Check arguments */
    if(sizeof_addr) {
        if(sizeof_addr != 2 && sizeof_addr != 4 && sizeof_addr != 8 && sizeof_addr != 16)
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file haddr_t size is not valid")
    } /* end if */
    if(sizeof_size) {
        if(sizeof_size != 2 && sizeof_size != 4 && sizeof_size != 8 && sizeof_size != 16)
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file size_t size is not valid")
    } /* end if */

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Set value */
    if(sizeof_addr) {
        uint8_t tmp_sizeof_addr = (uint8_t)sizeof_addr;

        if(H5P_set(plist, H5F_CRT_ADDR_BYTE_NUM_NAME, &tmp_sizeof_addr) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set byte number for an address")
    } /* end if */
    if(sizeof_size) {
        uint8_t tmp_sizeof_size = (uint8_t)sizeof_size;

        if(H5P_set(plist, H5F_CRT_OBJ_BYTE_NUM_NAME, &tmp_sizeof_size) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set byte number for object ")
    } /* end if */

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pset_sizes() */


/*-------------------------------------------------------------------------
 * Function:	H5Pget_sizes
 *
 * Purpose:	Returns the size of address and size quantities stored in a
 *		file according to a file creation property list.  Either (or
 *		even both) SIZEOF_ADDR and SIZEOF_SIZE may be null pointers.
 *
 * Return:	Success:	Non-negative, sizes returned through arguments.
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *		Wednesday, January  7, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_sizes(hid_t plist_id, size_t *sizeof_addr, size_t *sizeof_size)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t ret_value = SUCCEED;   /* return value */

    FUNC_ENTER_API(H5Pget_sizes, FAIL)
    H5TRACE3("e", "i*z*z", plist_id, sizeof_addr, sizeof_size);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Get values */
    if(sizeof_addr) {
        uint8_t tmp_sizeof_addr;

        if(H5P_get(plist, H5F_CRT_ADDR_BYTE_NUM_NAME, &tmp_sizeof_addr) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get byte number for an address")
        *sizeof_addr = tmp_sizeof_addr;
    } /* end if */
    if(sizeof_size) {
        uint8_t tmp_sizeof_size;

        if(H5P_get(plist, H5F_CRT_OBJ_BYTE_NUM_NAME, &tmp_sizeof_size) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get byte number for object ")
        *sizeof_size = tmp_sizeof_size;
    } /* end if */

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pget_sizes() */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_sym_k
 *
 * Purpose:	IK is one half the rank of a tree that stores a symbol
 *		table for a group.  Internal nodes of the symbol table are on
 *		average 75% full.  That is, the average rank of the tree is
 *		1.5 times the value of IK.
 *
 *		LK is one half of the number of symbols that can be stored in
 *		a symbol table node.  A symbol table node is the leaf of a
 *		symbol table tree which is used to store a group.  When
 *		symbols are inserted randomly into a group, the group's
 *		symbol table nodes are 75% full on average.  That is, they
 *		contain 1.5 times the number of symbols specified by LK.
 *
 *		Either (or even both) of IK and LK can be zero in which case
 *		that value is left unchanged.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Tuesday, January  6, 1998
 *
 * Modifications:
 *
 *		Raymond Lu, Oct 14, 2001
 *         	Changed to the new generic property list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_sym_k(hid_t plist_id, unsigned ik, unsigned lk)
{
    unsigned btree_k[H5B_NUM_BTREE_ID];
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5Pset_sym_k, FAIL);
    H5TRACE3("e", "iIuIu", plist_id, ik, lk);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Set values */
    if (ik > 0) {
        if(H5P_get(plist, H5F_CRT_BTREE_RANK_NAME, btree_k) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get rank for btree interanl nodes");
        btree_k[H5B_SNODE_ID] = ik;
        if(H5P_set(plist, H5F_CRT_BTREE_RANK_NAME, btree_k) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set rank for btree nodes");
    }
    if (lk > 0)
        if(H5P_set(plist, H5F_CRT_SYM_LEAF_NAME, &lk) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set rank for symbol table leaf nodes");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pget_sym_k
 *
 * Purpose:	Retrieves the symbol table B-tree 1/2 rank (IK) and the
 *		symbol table leaf node 1/2 size (LK).  See H5Pset_sym_k() for
 *		details. Either (or even both) IK and LK may be null
 *		pointers.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Wednesday, January  7, 1998
 *
 * Modifications:
 *
 *		Raymond Lu
 *		Changed to the new generic property list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_sym_k(hid_t plist_id, unsigned *ik /*out */ , unsigned *lk /*out */ )
{
    unsigned btree_k[H5B_NUM_BTREE_ID];
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5Pget_sym_k, FAIL);
    H5TRACE3("e", "ixx", plist_id, ik, lk);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Get values */
    if (ik) {
        if(H5P_get(plist, H5F_CRT_BTREE_RANK_NAME, btree_k) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get rank for btree nodes");
        *ik = btree_k[H5B_SNODE_ID];
    }
    if (lk)
        if(H5P_get(plist, H5F_CRT_SYM_LEAF_NAME, lk) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get rank for symbol table leaf nodes");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pset_istore_k
 *
 * Purpose:	IK is one half the rank of a tree that stores chunked raw
 *		data.  On average, such a tree will be 75% full, or have an
 *		average rank of 1.5 times the value of IK.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Tuesday, January  6, 1998
 *
 * Modifications:
 *
 *		Raymond Lu, Oct 14, 2001
 *		Changed to the new generic property list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_istore_k(hid_t plist_id, unsigned ik)
{
    unsigned btree_k[H5B_NUM_BTREE_ID];
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5Pset_istore_k, FAIL);
    H5TRACE2("e", "iIu", plist_id, ik);

    /* Check arguments */
    if (ik == 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "istore IK value must be positive");

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Set value */
    if(H5P_get(plist, H5F_CRT_BTREE_RANK_NAME, btree_k) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get rank for btree interanl nodes");
    btree_k[H5B_CHUNK_ID] = ik;
    if(H5P_set(plist, H5F_CRT_BTREE_RANK_NAME, btree_k) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set rank for btree interanl nodes");

done:
    FUNC_LEAVE_API(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:	H5Pget_istore_k
 *
 * Purpose:	Queries the 1/2 rank of an indexed storage B-tree.  See
 *		H5Pset_istore_k() for details.	The argument IK may be the
 *		null pointer.
 *
 * Return:	Success:	Non-negative, size returned through IK
 *
 *		Failure:	Negative
 *
 * Programmer:	Robb Matzke
 *		Wednesday, January  7, 1998
 *
 * Modifications:
 *
 *		Raymond Lu, Oct 14, 2001
 *		Changed to the new generic property list.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_istore_k(hid_t plist_id, unsigned *ik /*out */ )
{
    unsigned btree_k[H5B_NUM_BTREE_ID];
    H5P_genplist_t *plist;              /* Property list pointer */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_API(H5Pget_istore_k, FAIL)
    H5TRACE2("e", "ix", plist_id, ik);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Get value */
    if(ik) {
        if(H5P_get(plist, H5F_CRT_BTREE_RANK_NAME, btree_k) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get rank for btree interanl nodes");
        *ik = btree_k[H5B_CHUNK_ID];
    } /* end if */

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pget_istore_k() */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_shared_mesg_nindexes
 *
 * Purpose:	Set the number of Shared Object Header Message (SOHM)
 *              indexes specified in this property list.  If this is
 *              zero then shared object header messages are disabled
 *              for this file.
 *
 *              These indexes can then be configured with
 *              H5Pset_shared_mesg_index.  H5Pset_shared_mesg_phase_chage
 *              also controls settings for all indexes.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	James Laird
 *		Monday, October 9, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_shared_mesg_nindexes(hid_t plist_id, unsigned nindexes)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5Pset_shared_mesg_nindexes, FAIL);
    H5TRACE2("e", "iIu", plist_id, nindexes);

    /* Check argument */
    if (nindexes > H5O_SHMESG_MAX_NINDEXES)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "number of indexes is greater than H5O_SHMESG_MAX_NINDEXES");

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id, H5P_FILE_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    if(H5P_set(plist, H5F_CRT_SHMSG_NINDEXES_NAME, &nindexes) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't set number of indexes");

done:
    FUNC_LEAVE_API(ret_value);
} /* end H5Pset_shared_mesg_nindexes() */


/*-------------------------------------------------------------------------
 * Function:	H5Pget_shared_mesg_nindexes
 *
 * Purpose:	Get the number of Shared Object Header Message (SOHM)
 *              indexes specified in this property list.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	James Laird
 *		Monday, October 9, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_shared_mesg_nindexes(hid_t plist_id, unsigned *nindexes)
{
    H5P_genplist_t *plist;              /* Property list pointer */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_API(H5Pget_shared_mesg_nindexes, FAIL)
    H5TRACE2("e", "i*Iu", plist_id, nindexes);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    if(H5P_get(plist, H5F_CRT_SHMSG_NINDEXES_NAME, nindexes) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get number of indexes");

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pget_shared_mesg_nindexes() */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_shared_mesg_index
 *
 * Purpose:	Configure a given shared message index.  Sets the types of
 *              message that should be stored in this index and the minimum
 *              size of a message in the index.
 *
 *              INDEX_NUM is zero-indexed (in a file with three indexes,
 *              they are numbered 0, 1, and 2).
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	James Laird
 *		Wednesday, April 5, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_shared_mesg_index(hid_t plist_id, unsigned index_num, unsigned mesg_type_flags, unsigned min_mesg_size)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    unsigned    nindexes;               /* Number of SOHM indexes */
    unsigned    type_flags[H5O_SHMESG_MAX_NINDEXES]; /* Array of mesg_type_flags*/
    unsigned    minsizes[H5O_SHMESG_MAX_NINDEXES]; /* Array of min_mesg_sizes*/
    herr_t      ret_value = SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5Pset_shared_mesg_index, FAIL)
    H5TRACE4("e", "iIuIuIu", plist_id, index_num, mesg_type_flags, min_mesg_size);

    /* Check arguments */
    if(mesg_type_flags > H5O_SHMESG_ALL_FLAG)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "unrecognized flags in mesg_type_flags")

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Read the current number of indexes */
    if(H5P_get(plist, H5F_CRT_SHMSG_NINDEXES_NAME, &nindexes) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get number of indexes")

    /* Range check */
    if(index_num >= nindexes)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "index_num is too large; no such index");

    /* Get arrays of type flags and message sizes */
    if(H5P_get(plist, H5F_CRT_SHMSG_INDEX_TYPES_NAME, type_flags) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get current index type flags")
    if(H5P_get(plist, H5F_CRT_SHMSG_INDEX_MINSIZE_NAME, minsizes) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get current min sizes")

    /* Set values in arrays */
    type_flags[index_num] = mesg_type_flags;
    minsizes[index_num] = min_mesg_size;

    /* Write arrays back to plist */
    if(H5P_set(plist, H5F_CRT_SHMSG_INDEX_TYPES_NAME, type_flags) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set index type flags")
    if(H5P_set(plist, H5F_CRT_SHMSG_INDEX_MINSIZE_NAME, minsizes) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set min mesg sizes")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Pset_shared_mesg_index() */


/*-------------------------------------------------------------------------
 * Function:	H5Pget_shared_mesg_index
 *
 * Purpose:	Get information about a given shared message index.  Gets
 *              the types of message that are stored in the index and the
 *              minimum size of a message in the index.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	James Laird
 *		Wednesday, April 5, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_shared_mesg_index(hid_t plist_id, unsigned index_num, unsigned *mesg_type_flags, unsigned *min_mesg_size)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    unsigned    nindexes;               /* Number of SOHM indexes */
    unsigned    type_flags[H5O_SHMESG_MAX_NINDEXES]; /* Array of mesg_type_flags*/
    unsigned    minsizes[H5O_SHMESG_MAX_NINDEXES]; /* Array of min_mesg_sizes*/
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5Pget_shared_mesg_index, FAIL);
    H5TRACE4("e", "iIu*Iu*Iu", plist_id, index_num, mesg_type_flags,
             min_mesg_size);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID")

    /* Read the current number of indexes */
    if(H5P_get(plist, H5F_CRT_SHMSG_NINDEXES_NAME, &nindexes) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get number of indexes")

    if(index_num >= nindexes)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "index_num is greater than number of indexes in property list")

    /* Get arrays of type flags and message sizes */
    if(H5P_get(plist, H5F_CRT_SHMSG_INDEX_TYPES_NAME, type_flags) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get current index type flags")
    if(H5P_get(plist, H5F_CRT_SHMSG_INDEX_MINSIZE_NAME, minsizes) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get current min sizes")

    /* Get values from arrays */
    if(mesg_type_flags)
        *mesg_type_flags = type_flags[index_num];
    if(min_mesg_size)
        *min_mesg_size = minsizes[index_num];

done:
    FUNC_LEAVE_API(ret_value);
} /* end H5Pset_shared_mesg_index() */


/*-------------------------------------------------------------------------
 * Function:	H5Pset_shared_mesg_phase_change
 *
 * Purpose:	Sets the cutoff values for indexes storing shared object
 *              header messages in this file.  If more than max_list
 *              messages are in an index, that index will become a B-tree.
 *              Likewise, a B-tree index containing fewer than min_btree
 *              messages will be converted to a list.
 *
 *              If the max_list is zero then SOHM indexes in this file will
 *              never be lists but will be created as B-trees.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	James Laird
 *		Wednesday, April 5, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_shared_mesg_phase_change(hid_t plist_id, unsigned max_list, unsigned min_btree)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5Pset_shared_mesg_phase_change, FAIL);
    H5TRACE3("e", "iIuIu", plist_id, max_list, min_btree);

    /* Check that values are sensible.  The min_btree value must be no greater
     * than the max list plus one.
     *
     * Range check to make certain they will fit into encoded form.
     */
    if(max_list + 1 < min_btree)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "minimum B-tree value is greater than maximum list value")
    if(max_list > H5O_SHMESG_MAX_LIST_SIZE)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "max list value is larger than H5O_SHMESG_MAX_LIST_SIZE")
    if(min_btree > H5O_SHMESG_MAX_LIST_SIZE)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "min btree value is larger than H5O_SHMESG_MAX_LIST_SIZE")

    /* Avoid the strange case where max_list == 0 and min_btree == 1, so deleting the
     * last message in a B-tree makes it become an empty list.
     */
    if(max_list == 0)
        min_btree = 0;

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    if(H5P_set(plist, H5F_CRT_SHMSG_LIST_MAX_NAME, &max_list) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't set list maximum in property list");
    if(H5P_set(plist, H5F_CRT_SHMSG_BTREE_MIN_NAME, &min_btree) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't set B-tree minimum in property list");

done:
    FUNC_LEAVE_API(ret_value);
} /* end H5Pset_shared_mesg_phase_change() */


/*-------------------------------------------------------------------------
 * Function:	H5Pget_shared_mesg_phase_change
 *
 * Purpose:	Gets the maximum size of a SOHM list index before it becomes
 *              a B-tree.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	James Laird
 *		Wednesday, April 5, 2006
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_shared_mesg_phase_change(hid_t plist_id, unsigned *max_list, unsigned *min_btree)
{
    H5P_genplist_t *plist;      /* Property list pointer */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_API(H5Pget_shared_mesg_phase_change, FAIL);
    H5TRACE3("e", "i*Iu*Iu", plist_id, max_list, min_btree);

    /* Get the plist structure */
    if(NULL == (plist = H5P_object_verify(plist_id,H5P_FILE_CREATE)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "can't find object for ID");

    /* Get value */
    if (max_list) {
        if(H5P_get(plist, H5F_CRT_SHMSG_LIST_MAX_NAME, max_list) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get list maximum");
    }
    if (min_btree) {
        if(H5P_get(plist, H5F_CRT_SHMSG_BTREE_MIN_NAME, min_btree) < 0)
            HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "can't get SOHM information");
    }

done:
    FUNC_LEAVE_API(ret_value);
} /* end H5Pget_shared_mesg_phase_change() */

