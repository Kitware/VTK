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
#include "XdmfValues.h"
#include "XdmfDataItem.h"
#include "XdmfArray.h"

namespace xdmf2
{

XdmfValues::XdmfValues() {
    this->Format = -1;
}

XdmfValues::~XdmfValues() {
}

XdmfInt32 
XdmfValues::Inherit(XdmfDataItem *DataItem) {
    if(!DataItem){
        XdmfErrorMessage("DataItem to copy is NULL");
        return(XDMF_FAIL);
    }
    if(this->SetDOM(DataItem->GetDOM()) != XDMF_SUCCESS){
        XdmfErrorMessage("Error Setting DOM");
        return(XDMF_FAIL);
    }
    // if(this->SetElement(DataItem->GetElement()) != XDMF_SUCCESS){
    //     XdmfErrorMessage("Error Setting Element");
    //     return(XDMF_FAIL);
    // }
    // Do it by hand so the _private member of the structure is maintained.
    if(!DataItem->GetElement()){
        XdmfErrorMessage("Element is NULL");
        return(XDMF_FAIL);
    }
    this->Element = DataItem->GetElement();
    if(this->SetFormat(DataItem->GetFormat()) != XDMF_SUCCESS){
        XdmfErrorMessage("Error Setting Element");
        return(XDMF_FAIL);
    }
    //if(this->DataDesc && this->DataDescIsMine){
    //    delete this->DataDesc;
    //}
    //this->DataDescIsMine = 0;
    if(this->SetDataDesc(DataItem->GetDataDesc()) != XDMF_SUCCESS){
        XdmfErrorMessage("Error Setting DataDesc");
        return(XDMF_FAIL);
    }
    //this->DataDescIsMine = 0;
    return(XDMF_SUCCESS);
}

// Override this
XdmfArray *
XdmfValues::Read(XdmfArray *){
    return(NULL);
}

// Override this
XdmfInt32 
XdmfValues::Write(XdmfArray *, XdmfConstString ){
    return(XDMF_FAIL);
}

}
