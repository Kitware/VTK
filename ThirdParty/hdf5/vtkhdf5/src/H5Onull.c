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
 * Created:             H5Onull.c
 *                      Aug  6 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             The null message.
 *
 *-------------------------------------------------------------------------
 */

#define H5O_PACKAGE		/*suppress error about including H5Opkg	  */

#include "H5private.h"		/* Generic Functions			*/
#include "H5Opkg.h"             /* Object headers			*/


/* This message derives from H5O message class */
const H5O_msg_class_t H5O_MSG_NULL[1] = {{
    H5O_NULL_ID,            	/*message id number             */
    "null",                 	/*message name for debugging    */
    0,                      	/*native message size           */
    0,				/* messages are sharable?       */
    NULL,                   	/*no decode method              */
    NULL,                   	/*no encode method              */
    NULL,                   	/*no copy method                */
    NULL,                   	/*no size method                */
    NULL,                   	/*no reset method               */
    NULL,                   	/*no free method                */
    NULL,		    	/*no file delete method         */
    NULL,		    	/*no link method		*/
    NULL,	            	/*no set share method	        */
    NULL,		    	/*no can share method		*/
    NULL,		    	/*no pre copy native value to file */
    NULL,		    	/*no copy native value to file  */
    NULL,		    	/*no post copy native value to file */
    NULL,			/*no get creation index		*/
    NULL,			/*no set creation index		*/
    NULL                    	/*no debug method               */
}};

