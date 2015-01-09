/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Values              */
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
#include "XdmfDataTransform.h"
#include "XdmfArray.h"

namespace xdmf2
{

XdmfDataTransform::XdmfDataTransform() {
    this->ItemType = XDMF_ITEM_FUNCTION;
}

XdmfDataTransform::~XdmfDataTransform() {
}

XdmfInt32 
XdmfDataTransform::UpdateInformation(){
    if(XdmfDataItem::UpdateInformation() != XDMF_SUCCESS) return(XDMF_FAIL);
    return(XDMF_SUCCESS);
}

}
