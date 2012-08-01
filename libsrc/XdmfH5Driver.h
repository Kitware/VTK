/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : Id  */
/*  Date : $Date$ */
/*  Version : $Revision$ */
/*                                                                 */
/*  Author:                                                        */
/*     Jerry A. Clarke                                             */
/*     clarke@arl.army.mil                                         */
/*     US Army Research Laboratory                                 */
/*     Aberdeen Proving Ground, MD                                 */
/*                                                                 */
/*     Copyright @ 2007 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/
#ifndef H5FDdsm_H
#define H5FDdsm_H
#include "XdmfDsmBuffer.h"

#include "H5Ipublic.h"
#include "H5pubconf.h"

#include "XdmfExport.h"

#define H5FD_DSM  (H5FD_dsm_init())
/* Allocate memory in multiples of this size by default */
#define H5FD_DSM_INCREMENT    1000000

extern "C" {
XDMF_EXPORT hid_t H5FD_dsm_init(void);
}
XDMF_EXPORT herr_t H5Pset_fapl_dsm(hid_t fapl_id, size_t increment, XdmfDsmBuffer *buffer);
XDMF_EXPORT herr_t H5Pget_fapl_dsm(hid_t fapl_id, size_t *increment/*out*/, XdmfDsmBuffer **buffer);

#endif
