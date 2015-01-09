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
#include "XdmfRoot.h"
#include "XdmfDOM.h"
#include <stdlib.h>

namespace xdmf2
{

XdmfRoot::XdmfRoot() {
    this->SetElementName("Xdmf");
    this->Version = (XdmfFloat32)XDMF_VERSION;
    this->XInclude = 1;
}

XdmfRoot::~XdmfRoot() {
}

XdmfInt32 XdmfRoot::UpdateInformation(){
    XdmfConstString Value;

    XdmfElement::UpdateInformation();
    Value = this->Get("Version");
    if(Value) this->SetVersion((XdmfFloat32)atof(Value));
    Value = this->Get("XInclude");
    if(!Value) this->SetXInclude(atoi(Value));
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfRoot::Insert( XdmfElement *Child){
    if(Child && (
        XDMF_WORD_CMP(Child->GetElementName(), "Domain") ||
        XDMF_WORD_CMP(Child->GetElementName(), "DataItem") ||
        XDMF_WORD_CMP(Child->GetElementName(), "Information")
        )){
        return(XdmfElement::Insert(Child));
    }else{
        XdmfErrorMessage("Xdmf Root can only Insert Domain | DataItem | Information elements, not a " << Child->GetElementName());
    }
    return(XDMF_FAIL);
}

XdmfInt32 XdmfRoot::Build(){
    static char VersionBuf[80];
    ostrstream  aVersion(VersionBuf,80);

    if(!this->GetElement()){
        if(this->GetDOM()){
            XdmfXmlNode  node;

            node = this->GetDOM()->Create(this->GetElementName());
            this->SetElement(node);
        }
    }
    if(XdmfElement::Build() != XDMF_SUCCESS) return(XDMF_FAIL);
    // Version and XInclude
    aVersion << this->Version << ends;
    this->Set("Version", (XdmfConstString)aVersion.str());
    return(XDMF_SUCCESS);
}

}
