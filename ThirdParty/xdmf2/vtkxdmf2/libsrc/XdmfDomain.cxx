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
#include "XdmfDomain.h"
#include "XdmfGrid.h"

XdmfDomain::XdmfDomain() {
    this->SetElementName("Domain");
}

XdmfDomain::~XdmfDomain() {
}

XdmfInt32 XdmfDomain::UpdateInformation(){

    if(XdmfElement::UpdateInformation() != XDMF_SUCCESS) return(XDMF_FAIL);
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfDomain::Insert( XdmfElement *Child){
    if(Child && (
        XDMF_WORD_CMP(Child->GetElementName(), "DataItem") ||
        XDMF_WORD_CMP(Child->GetElementName(), "Grid") ||
        XDMF_WORD_CMP(Child->GetElementName(), "Information")
        )){
        XdmfInt32 status = XdmfElement::Insert(Child);
        if((status == XDMF_SUCCESS) && XDMF_WORD_CMP(Child->GetElementName(), "Grid")){
            XdmfGrid *ChildGrid = (XdmfGrid *)Child;
            if((ChildGrid->GetGridType() & XDMF_GRID_MASK) == XDMF_GRID_UNIFORM){
                if(ChildGrid->InsertTopology() != XDMF_SUCCESS) return(XDMF_FAIL);
                if(ChildGrid->InsertGeometry() != XDMF_SUCCESS) return(XDMF_FAIL);
            }
        }
        return(status);
    }else{
        XdmfErrorMessage("Domain can only Insert Grid | DataItem | Information elements, not a " << Child->GetElementName());
    }
    return(XDMF_FAIL);
}

XdmfInt32 XdmfDomain::Build(){
    if(XdmfElement::Build() != XDMF_SUCCESS) return(XDMF_FAIL);
    return(XDMF_SUCCESS);
}
