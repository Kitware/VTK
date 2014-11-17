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
 * Created:             H5Oname.c
 *                      Aug 12 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             Object name message.
 *
 *-------------------------------------------------------------------------
 */

#define H5O_PACKAGE		/*suppress error about including H5Opkg	  */

#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Opkg.h"             /* Object headers			*/


/* PRIVATE PROTOTYPES */
static void *H5O_name_decode(H5F_t *f, hid_t dxpl_id, H5O_t *open_oh,
    unsigned mesg_flags, unsigned *ioflags, const uint8_t *p);
static herr_t H5O_name_encode(H5F_t *f, hbool_t disable_shared, uint8_t *p, const void *_mesg);
static void *H5O_name_copy(const void *_mesg, void *_dest);
static size_t H5O_name_size(const H5F_t *f, hbool_t disable_shared, const void *_mesg);
static herr_t H5O_name_reset(void *_mesg);
static herr_t H5O_name_debug(H5F_t *f, hid_t dxpl_id, const void *_mesg, FILE * stream,
			     int indent, int fwidth);

/* This message derives from H5O message class */
const H5O_msg_class_t H5O_MSG_NAME[1] = {{
    H5O_NAME_ID,            	/*message id number             */
    "name",                 	/*message name for debugging    */
    sizeof(H5O_name_t),     	/*native message size           */
    0,				/* messages are sharable?       */
    H5O_name_decode,        	/*decode message                */
    H5O_name_encode,        	/*encode message                */
    H5O_name_copy,          	/*copy the native value         */
    H5O_name_size,          	/*raw message size              */
    H5O_name_reset,         	/*free internal memory          */
    NULL,			/* free method			*/
    NULL,		        /* file delete method		*/
    NULL,			/* link method			*/
    NULL,			/*set share method		*/
    NULL,		    	/*can share method		*/
    NULL,			/* pre copy native value to file */
    NULL,			/* copy native value to file    */
    NULL,			/* post copy native value to file    */
    NULL,			/* get creation index		*/
    NULL,			/* set creation index		*/
    H5O_name_debug         	/*debug the message             */
}};


/*-------------------------------------------------------------------------
 * Function:    H5O_name_decode
 *
 * Purpose:     Decode a name message and return a pointer to a new
 *              native message struct.
 *
 * Return:      Success:        Ptr to new message in native struct.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug 12 1997
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_name_decode(H5F_t UNUSED *f, hid_t UNUSED dxpl_id, H5O_t UNUSED *open_oh,
    unsigned UNUSED mesg_flags, unsigned UNUSED *ioflags, const uint8_t *p)
{
    H5O_name_t          *mesg;
    void                *ret_value;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(f);
    HDassert(p);

    /* decode */
    if(NULL == (mesg = (H5O_name_t *)H5MM_calloc(sizeof(H5O_name_t))))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")
    if(NULL == (mesg->s = (char *)H5MM_strdup((const char *)p)))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Set return value */
    ret_value = mesg;

done:
    if(NULL == ret_value) {
        if(mesg)
            mesg = (H5O_name_t *)H5MM_xfree(mesg);
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_name_decode() */


/*-------------------------------------------------------------------------
 * Function:    H5O_name_encode
 *
 * Purpose:     Encodes a name message.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug 12 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_name_encode(H5F_t UNUSED *f, hbool_t UNUSED disable_shared, uint8_t *p, const void *_mesg)
{
    const H5O_name_t       *mesg = (const H5O_name_t *) _mesg;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* check args */
    HDassert(f);
    HDassert(p);
    HDassert(mesg && mesg->s);

    /* encode */
    HDstrcpy((char*)p, mesg->s);

    FUNC_LEAVE_NOAPI(SUCCEED)
}


/*-------------------------------------------------------------------------
 * Function:    H5O_name_copy
 *
 * Purpose:     Copies a message from _MESG to _DEST, allocating _DEST if
 *              necessary.
 *
 * Return:      Success:        Ptr to _DEST
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug 12 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5O_name_copy(const void *_mesg, void *_dest)
{
    const H5O_name_t       *mesg = (const H5O_name_t *) _mesg;
    H5O_name_t             *dest = (H5O_name_t *) _dest;
    void                *ret_value;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    /* check args */
    HDassert(mesg);

    if(!dest && NULL == (dest = (H5O_name_t *)H5MM_calloc(sizeof(H5O_name_t))))
	HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* copy */
    *dest = *mesg;
    if(NULL == (dest->s = H5MM_xstrdup(mesg->s)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed")

    /* Set return value */
    ret_value = dest;

done:
    if(NULL == ret_value)
        if(dest && NULL == _dest)
            dest = (H5O_name_t *)H5MM_xfree(dest);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5O_name_copy() */


/*-------------------------------------------------------------------------
 * Function:    H5O_name_size
 *
 * Purpose:     Returns the size of the raw message in bytes not
 *              counting the message typ or size fields, but only the data
 *              fields.  This function doesn't take into account
 *              alignment.
 *
 * Return:      Success:        Message data size in bytes w/o alignment.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug 12 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5O_name_size(const H5F_t UNUSED *f, hbool_t UNUSED disable_shared, const void *_mesg)
{
    const H5O_name_t       *mesg = (const H5O_name_t *) _mesg;
    size_t                  ret_value;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* check args */
    HDassert(f);
    HDassert(mesg);

    ret_value = mesg->s ? HDstrlen(mesg->s) + 1 : 0;

    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:    H5O_name_reset
 *
 * Purpose:     Frees internal pointers and resets the message to an
 *              initial state.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug 12 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_name_reset(void *_mesg)
{
    H5O_name_t             *mesg = (H5O_name_t *) _mesg;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* check args */
    HDassert(mesg);

    /* reset */
    mesg->s = (char *)H5MM_xfree(mesg->s);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5O_name_reset() */


/*-------------------------------------------------------------------------
 * Function:    H5O_name_debug
 *
 * Purpose:     Prints debugging info for the message.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug 12 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5O_name_debug(H5F_t UNUSED *f, hid_t UNUSED dxpl_id, const void *_mesg, FILE *stream,
	       int indent, int fwidth)
{
    const H5O_name_t	*mesg = (const H5O_name_t *)_mesg;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* check args */
    HDassert(f);
    HDassert(mesg);
    HDassert(stream);
    HDassert(indent >= 0);
    HDassert(fwidth >= 0);

    fprintf(stream, "%*s%-*s `%s'\n", indent, "", fwidth,
            "Name:",
            mesg->s);

    FUNC_LEAVE_NOAPI(SUCCEED)
}
