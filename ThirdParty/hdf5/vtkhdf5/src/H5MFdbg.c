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
 * Created:             H5MFdbg.c
 *                      Jan 31 2008
 *                      Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:             File memory management debugging functions.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#define H5F_PACKAGE		/*suppress error about including H5Fpkg	  */
#define H5MF_PACKAGE		/*suppress error about including H5MFpkg  */
#define H5MF_DEBUGGING          /* Need access to file space debugging routines */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fpkg.h"             /* File access				*/
#include "H5MFpkg.h"		/* File memory management		*/

#ifdef H5MF_ALLOC_DEBUG_DUMP

/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/

/* User data for free space section iterator callback */
typedef struct {
    H5FS_t *fspace;             /* Free space manager */
    FILE *stream;               /* Stream for output */
    int indent;                 /* Indention amount */
    int fwidth;                 /* Field width amount */
} H5MF_debug_iter_ud_t;


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/


/*********************/
/* Package Variables */
/*********************/


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/


/*-------------------------------------------------------------------------
 * Function:	H5MF_sects_debug_cb
 *
 * Purpose:	Prints debugging info about a free space section for a file
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		January 31 2008
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5MF_sects_debug_cb(const H5FS_section_info_t *_sect, void *_udata)
{
    const H5MF_free_section_t *sect = (const H5MF_free_section_t *)_sect;       /* Section to dump info */
    H5MF_debug_iter_ud_t *udata = (H5MF_debug_iter_ud_t *)_udata;         /* User data for callbacks */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5MF_sects_debug_cb)

    /*
     * Check arguments.
     */
    HDassert(sect);
    HDassert(udata);

    /* Print generic section information */
    HDfprintf(udata->stream, "%*s%-*s %s\n", udata->indent, "", udata->fwidth,
	      "Section type:",
	      (sect->sect_info.type == H5MF_FSPACE_SECT_SIMPLE ? "simple" : "unknown"));
    HDfprintf(udata->stream, "%*s%-*s %a\n", udata->indent, "", udata->fwidth,
	      "Section address:",
	      sect->sect_info.addr);
    HDfprintf(udata->stream, "%*s%-*s %Hu\n", udata->indent, "", udata->fwidth,
	      "Section size:",
	      sect->sect_info.size);
    HDfprintf(udata->stream, "%*s%-*s %Hu\n", udata->indent, "", udata->fwidth,
	      "End of section:",
	      (haddr_t)((sect->sect_info.addr + sect->sect_info.size) - 1));
    HDfprintf(udata->stream, "%*s%-*s %s\n", udata->indent, "", udata->fwidth,
	      "Section state:",
	      (sect->sect_info.state == H5FS_SECT_LIVE ? "live" : "serialized"));

    /* Dump section-specific debugging information */
    if(H5FS_sect_debug(udata->fspace, _sect, udata->stream, udata->indent + 3, MAX(0, udata->fwidth - 3)) < 0)
        HGOTO_ERROR(H5E_RESOURCE, H5E_BADITER, FAIL, "can't dump section's debugging info")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MF_sects_debug_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5MF_sects_dump
 *
 * Purpose:	Prints debugging info about free space sections for a file.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Quincey Koziol
 *		koziol@hdfgroup.org
 *		Jan 31 2008
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5MF_sects_dump(H5F_t *f, hid_t dxpl_id, FILE *stream)
{
    haddr_t eoa;                        /* End of allocated space in the file */
    haddr_t ma_addr = HADDR_UNDEF;      /* Base "metadata aggregator" address */
    hsize_t ma_size = 0;                /* Size of "metadata aggregator" */
    haddr_t sda_addr = HADDR_UNDEF;     /* Base "small data aggregator" address */
    hsize_t sda_size = 0;               /* Size of "small data aggregator" */
    H5FD_mem_t type;                    /* Memory type for iteration */
    int indent = 0;                     /* Amount to indent */
    int fwidth = 50;                    /* Field width */
    herr_t ret_value = SUCCEED;         /* Return value */

    FUNC_ENTER_NOAPI(H5MF_sects_dump, FAIL)
#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: Dumping file free space sections\n", FUNC);
#endif /* H5MF_ALLOC_DEBUG */

    /*
     * Check arguments.
     */
    HDassert(f);
    HDassert(stream);

    /* Retrieve the 'eoa' for the file */
    if(HADDR_UNDEF == (eoa = H5FD_get_eoa(f->shared->lf, H5FD_MEM_DEFAULT)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGET, FAIL, "driver get_eoa request failed")
#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: for type = H5FD_MEM_DEFAULT, eoa = %a\n", FUNC, eoa);
#endif /* H5MF_ALLOC_DEBUG */

    /* Retrieve metadata aggregator info, if available */
    H5MF_aggr_query(f, &(f->shared->meta_aggr), &ma_addr, &ma_size);
#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: ma_addr = %a, ma_size = %Hu, end of ma = %a\n", FUNC, ma_addr, ma_size, (haddr_t)((ma_addr + ma_size) - 1));
#endif /* H5MF_ALLOC_DEBUG */

    /* Retrieve 'small data' aggregator info, if available */
    H5MF_aggr_query(f, &(f->shared->sdata_aggr), &sda_addr, &sda_size);
#ifdef H5MF_ALLOC_DEBUG
HDfprintf(stderr, "%s: sda_addr = %a, sda_size = %Hu, end of sda = %a\n", FUNC, sda_addr, sda_size, (haddr_t)((sda_addr + sda_size) - 1));
#endif /* H5MF_ALLOC_DEBUG */

    /* Iterate over all the free space types that have managers and dump each free list's space */
    for(type = H5FD_MEM_DEFAULT; type < H5FD_MEM_NTYPES; H5_INC_ENUM(H5FD_mem_t, type)) {
        /* Print header for type */
        HDfprintf(stream, "%*sFile Free Space Info for type = %u:\n", indent, "", (unsigned)type);

        /* Check for this type being mapped to another type */
        if(H5FD_MEM_DEFAULT == f->shared->fs_type_map[type] ||
                type == f->shared->fs_type_map[type]) {
            /* Retrieve the 'eoa' for this file memory type */
            if(HADDR_UNDEF == (eoa = H5FD_get_eoa(f->shared->lf, type)))
                HGOTO_ERROR(H5E_RESOURCE, H5E_CANTGET, FAIL, "driver get_eoa request failed")
            HDfprintf(stream, "%*s%-*s %a\n", indent + 3, "", MAX(0, fwidth - 3),
                      "eoa:",
                      eoa);

            /* Print header for sections */
            HDfprintf(stream, "%*sSections:\n", indent + 3, "");

            /* If there is a free space manager for this type, iterate over them */
            if(f->shared->fs_man[type]) {
                H5MF_debug_iter_ud_t udata;        /* User data for callbacks */

                /* Prepare user data for section iteration callback */
                udata.fspace = f->shared->fs_man[type];
                udata.stream = stream;
                udata.indent = indent + 6;
                udata.fwidth = MAX(0, fwidth - 6);

                /* Iterate over all the free space sections */
                if(H5FS_sect_iterate(f, dxpl_id, f->shared->fs_man[type], H5MF_sects_debug_cb, &udata) < 0)
                    HGOTO_ERROR(H5E_HEAP, H5E_BADITER, FAIL, "can't iterate over heap's free space")
            } /* end if */
            else {
                /* No sections of this type */
                HDfprintf(stream, "%*s<none>\n", indent + 6, "");
            } /* end else */
        } /* end if */
        else {
            HDfprintf(stream, "%*sMapped to type = %u\n", indent, "", (unsigned)f->shared->fs_type_map[type]);
        } /* end else */
    } /* end for */

done:
HDfprintf(stderr, "%s: Done dumping file free space sections\n", FUNC);
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5MF_sects_dump() */
#endif /* H5MF_ALLOC_DEBUG_DUMP */

