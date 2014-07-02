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
/*     Copyright @ 2002 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/
#ifndef __XdmfHDFSupport_h
#define __XdmfHDFSupport_h

#ifndef SWIG
#include "XdmfObject.h"
#include "H5public.h"

#if (H5_VERS_MAJOR>1)||((H5_VERS_MAJOR==1)&&(H5_VERS_MINOR>=6))
#include "hdf5.h"
#else
extern "C" {
#include "hdf5.h"
}
#endif

#endif /* SWIG */

namespace xdmf2
{

extern XDMF_EXPORT hid_t    XdmfTypeToHDF5Type( XdmfInt32 XdmfType );
extern XDMF_EXPORT XdmfInt32  HDF5TypeToXdmfType( hid_t HDF5Type );
extern XDMF_EXPORT XdmfConstString  XdmfTypeToString( XdmfInt32 XdmfType );
extern XDMF_EXPORT XdmfInt32  StringToXdmfType( XdmfConstString TypeName );
extern XDMF_EXPORT XdmfConstString  XdmfTypeToClassString( XdmfInt32 XdmfType );

}
#endif // __XdmfHDFSupport_h
