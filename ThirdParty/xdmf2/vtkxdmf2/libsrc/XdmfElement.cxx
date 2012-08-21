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
#include "XdmfElement.h"
#include "XdmfDOM.h"
#include <libxml/tree.h>
#include <string.h>

#define XDMF_EMPTY_REFERENCE   0x00
#define XDMF_ERROR_REFERENCE    -1

#define XDMF_XML_PRIVATE_DATA(e) e->_private

XdmfElementData::XdmfElementData(){
    this->ReferenceElement = NULL;
    this->CurrentXdmfElement = NULL;
}

XdmfElementData::~XdmfElementData() {
}

XdmfElement::XdmfElement() {
    this->DOM = NULL;
    this->Element = NULL;
    this->ElementName = NULL;
    this->IsReference = 0;
    this->ReferenceElement = NULL;
    this->State = XDMF_ELEMENT_STATE_UNINITIALIZED;
    this->CopyReferenceData = 1;
    this->RootWhenParsed = 0;
    this->DsmBuffer = NULL;
    this->DataXml = 0;
    this->InsertedDataXml = 0;
    this->DeleteOnGridDelete = 0;
}

XdmfElement::~XdmfElement() {
    if(this->DOM && (this->DOM->GetTree() == this->RootWhenParsed)){
        if(this->ReferenceElement){
         if(this->GetReferenceObject(this->ReferenceElement) == this){
             this->SetReferenceObject(this->ReferenceElement, XDMF_ELEMENT_STATE_UNINITIALIZED);
         }
     }
     this->ReferenceElement = NULL;
     if(this->Element){
         if(this->GetReferenceObject(this->Element) == this){
             this->SetReferenceObject(this->Element, XDMF_ELEMENT_STATE_UNINITIALIZED);
         }
     }
    }
    this->Element = NULL;
    if(this->ElementName) delete [] this->ElementName;
    if(this->DataXml) delete [] this->DataXml;
}

void XdmfElement::SetReferenceObject(XdmfXmlNode anElement, void *p){
    XdmfElementData *PrivateData;
    if(!anElement){
        XdmfErrorMessage("Element is NULL");
        return;
    }
    if(XDMF_XML_PRIVATE_DATA(anElement)){
        PrivateData = (XdmfElementData *)XDMF_XML_PRIVATE_DATA(anElement);
    }else{
        PrivateData = new XdmfElementData;
        XDMF_XML_PRIVATE_DATA(anElement) = (void *)PrivateData;
    }
    // XdmfDebug("Old Ref = " << ElementPrivateData->GetReferenceElement());
    // XdmfDebug("New Ref = " << p);
    PrivateData->SetReferenceElement((XdmfElement *)p);
}

void *
XdmfElement::GetReferenceObject(XdmfXmlNode anElement){
    XdmfElementData *ElementPrivateData;
    if(!anElement){
        XdmfErrorMessage("NULL Reference Element");
        return(NULL);
    }
    if(XDMF_XML_PRIVATE_DATA(anElement) == XDMF_EMPTY_REFERENCE){
        XdmfDebug("XML Node contains no initialized object");
        return(NULL);
    }
    ElementPrivateData = (XdmfElementData *)XDMF_XML_PRIVATE_DATA(anElement);
    if(ElementPrivateData->GetReferenceElement() == XDMF_EMPTY_REFERENCE){
        XdmfDebug("XML Node contains no initialized object");
        return(NULL);
    }
    return(ElementPrivateData->GetReferenceElement());
}

void XdmfElement::SetCurrentXdmfElement(XdmfXmlNode anElement, void *p){
    XdmfElementData *PrivateData;
    if(!anElement){
        XdmfErrorMessage("Element is NULL");
        return;
    }
    if(XDMF_XML_PRIVATE_DATA(anElement)){
        PrivateData = (XdmfElementData *)XDMF_XML_PRIVATE_DATA(anElement);
    }else{
        PrivateData = new XdmfElementData;
        XDMF_XML_PRIVATE_DATA(anElement) = (void *)PrivateData;
    }
    PrivateData->SetCurrentXdmfElement((XdmfElement *)p);
}

void *
XdmfElement::GetCurrentXdmfElement(XdmfXmlNode anElement){
    XdmfElementData *ElementPrivateData;
    if(!anElement){
        XdmfErrorMessage("NULL Reference Element");
        return(NULL);
    }
    ElementPrivateData = (XdmfElementData *)XDMF_XML_PRIVATE_DATA(anElement);
    if(!ElementPrivateData){
        return(NULL);
    }
    if(ElementPrivateData->GetCurrentXdmfElement() == XDMF_EMPTY_REFERENCE){
        XdmfDebug("XML Node contains no initialized object");
        return(NULL);
    }
    return(ElementPrivateData->GetCurrentXdmfElement());
}

XdmfInt32 XdmfElement::SetElement(XdmfXmlNode anElement, XdmfInt32 AssociateElement){
    if(!anElement) {
        XdmfErrorMessage("Element is NULL");
        return(XDMF_FAIL);
    }
    // Clear the ReferenceObject underlying node. This will also create ElementPrivateData if necessary
    XdmfDebug("Clearing ReferenceObject of XML node");
    this->SetReferenceObject(anElement, XDMF_EMPTY_REFERENCE);
    if(AssociateElement) this->SetCurrentXdmfElement(anElement, this);
    this->Element = anElement;
    if(this->DOM){
        this->RootWhenParsed = this->DOM->GetTree();
    }
    return(XDMF_SUCCESS);
}

XdmfInt32 XdmfElement::InsertChildElement(XdmfXmlNode Child){
    if(!this->DOM) {
        XdmfErrorMessage("No DOM has been set");
        return(XDMF_FAIL);
    }
    if(!this->Element) {
        XdmfErrorMessage("Current Element is empty");
        return(XDMF_FAIL);
    }
    if(this->DOM->Insert(this->Element, Child)){
        return(XDMF_SUCCESS);
    }
    return(XDMF_FAIL);
}


XdmfInt32
XdmfElement::Insert(XdmfElement *Child){
    XdmfXmlNode element;

    if(!this->DOM) {
        XdmfErrorMessage("No DOM has not been set : Parent must have a DOM and Element before insertion makes sense");
        return(XDMF_FAIL);
    }
    if(!this->GetElement()) {
        XdmfErrorMessage("No Element has not been set : Parent must have a DOM and Element before insertion makes sense");
        return(XDMF_FAIL);
    }
    if(!Child){
        XdmfErrorMessage("Child Element is NULL");
        return(XDMF_FAIL);
    }
    if(!Child->GetElementName()) {
        XdmfErrorMessage("Child Element has no ElementName");
        return(XDMF_FAIL);
    }
    Child->SetDOM(this->DOM);
    element = this->GetDOM()->InsertNew(this->Element, Child->GetElementName());
    if(!element){
        XdmfErrorMessage("Failed to Insert New Child XML Node");
        return(XDMF_FAIL);
    }
    if(Child->SetElement(element) != XDMF_SUCCESS){
        XdmfErrorMessage("Failed to set child XML node");
        return(XDMF_FAIL);
    }
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfElement::Copy(XdmfElement *){
    return(XDMF_SUCCESS);
}

XdmfInt32 XdmfElement::UpdateInformation(){
    XdmfConstString Value;

    XdmfElement *e;
    XdmfXmlNode ref;

    XdmfDebug("XdmfElement::UpdateInformation()");
    if(!this->DOM) {
        XdmfErrorMessage("No DOM has been set");
        return(XDMF_FAIL);
    }
    if(!this->Element){
        XdmfErrorMessage("No XML Node has been set");
        return(XDMF_FAIL);
    }
    Value = this->Get("Name");
    if(Value) this->SetName(Value);
    ref = this->CheckForReference(this->Element);
    if(ref == (XdmfXmlNode)XDMF_ERROR_REFERENCE){
        XdmfErrorMessage("Error Checking Reference");
        free((void*)Value);
        return(XDMF_FAIL);
    }
    if(ref){
        XdmfXmlNode node;

        // "this" is now the ReferenceObject for this->ReferenceElement
        XdmfDebug("Setting ReferenceObject and Following Chain");
        this->SetReferenceObject(this->ReferenceElement, this);
        while(ref){
            e = (XdmfElement *)this->GetReferenceObject(ref);
            if(e && (e != this)){
                XdmfDebug("Updating Information from another Object");
                // XdmfDebug(e->Serialize());
                // There if an Object associated with this node. UpdateInformation()?
                if(e->GetState() < XDMF_ELEMENT_STATE_LIGHT_PARSED){
                    // Could cause a chain of UpdateInformation() 
                    XdmfDebug("Call UpdateInformation on ReferenceObject");
                    e->UpdateInformation();
                }
                // Copy out appropriate information and return
                XdmfDebug("Copying Information from Reference Object");
                this->Element = e->Element;
                free((void*)Value);
                return(this->Copy(e));
            }
            // No ReferenceObject Set. Is this a Reference as well?
            node = this->FollowReference(ref);
            if(node){
                ref = node;
            }else{
                // Not a Reference. Is it the right Type ?
                if(STRCMP((const char *)ref->name, (const char *)this->ReferenceElement->name) != 0){
                    XdmfErrorMessage("Reference node " << Value << " is a " << ref->name << " not " << ReferenceElement->name);
                    delete [] Value;
                    return(XDMF_FAIL);
                }
                // If this is a derived Class, UpdateInformation will act on this target.
                this->SetElement(ref);
                // This is the end of the chain and there is no ReferenceObject for the XML node.
                XdmfDebug("Setting Reference Object");
                this->SetReferenceObject(ref, this);
                ref = NULL;
            }
        }
    }else{
        XdmfDebug("Setting Reference Object");
        this->SetReferenceObject(this->Element, this);
    }
    free((void*)Value);
    this->State = XDMF_ELEMENT_STATE_LIGHT_PARSED;
    return(XDMF_SUCCESS);
}

XdmfInt32 XdmfElement::Update(){
    XdmfXmlNode node, ref;
    XdmfElement *e;

    if(!this->DOM) {
        XdmfErrorMessage("No DOM has been set");
        return(XDMF_FAIL);
    }
    if(!this->Element){
        XdmfErrorMessage("No XML Node has been set");
        return(XDMF_FAIL);
    }
    if(this->GetIsReference()){
        // Don't assume that state has not changed since last UpdateInformation()
        ref = this->FollowReference(this->ReferenceElement);
        while(ref){
            e = (XdmfElement *)this->GetReferenceObject(ref);
            if(e && (e != this)){
                // Does it need Updating ?
                if(e->GetState() < XDMF_ELEMENT_STATE_LIGHT_PARSED) e->UpdateInformation();
                if(e->GetState() < XDMF_ELEMENT_STATE_HEAVY_READ) e->Update();
                this->Element = e->Element;
                return(this->Copy(e));
            }
            // No External Reference Object
            node = this->FollowReference(ref);
            if(node){
                ref = node;
            }else{
                // No Reference Object and this is the end of the chain
                // If this is a derived Class, Update will act on this target.
                this->SetElement(ref);
                // This is the end of the chain and there is no ReferenceObject for the XML node.
                XdmfDebug("Setting Reference Object");
                this->SetReferenceObject(ref, this);
                ref = NULL;
            }
        }
    }
    this->State = XDMF_ELEMENT_STATE_HEAVY_READ;
    return(XDMF_SUCCESS);
}

XdmfXmlNode
XdmfElement::FollowReference(XdmfXmlNode anElement){
    XdmfConstString Value;
    XdmfXmlNode     ref = NULL;

    if(!anElement){
        XdmfErrorMessage("Element is NULL");
        return((XdmfXmlNode)XDMF_ERROR_REFERENCE);
    }
    Value = this->DOM->Get(anElement, "Reference");
    if(Value){
        if(STRCASECMP(Value, "XML") == 0){
            Value = this->DOM->GetCData(anElement);
            if(!Value){
                XdmfErrorMessage("Reference to CDATA is NULL");
                return((XdmfXmlNode)XDMF_ERROR_REFERENCE);
            }
        }
        XdmfDebug("Following Reference to " << Value);
        ref = this->DOM->FindElementByPath(Value);
        if(!ref){
            XdmfErrorMessage("Can't Find Node of Path " << Value);
            return((XdmfXmlNode)XDMF_ERROR_REFERENCE);
        }
    }
    return(ref);
}

XdmfXmlNode
XdmfElement::CheckForReference(XdmfXmlNode anElement){

    XdmfXmlNode node;

    XdmfDebug("XdmfElement::CheckForReference(XdmfXmlNode Element)");
    if(!anElement) return((XdmfXmlNode)XDMF_ERROR_REFERENCE);
    // Does the Referenced Node Exist and is it of the Same Type
    node = this->FollowReference(anElement);
    if(node == (XdmfXmlNode)XDMF_ERROR_REFERENCE){
        XdmfErrorMessage("Error Following Reference");
        return((XdmfXmlNode)XDMF_ERROR_REFERENCE);
    }
    if(node){
        XdmfDebug("Element is a Reference");
        // Check Type (low level XML "name") against this->Element
        if(STRCMP((const char *)node->name, (const char *)anElement->name) != 0){
            XdmfErrorMessage("Reference node is a " << node->name << " not " << anElement->name);
            return((XdmfXmlNode)XDMF_ERROR_REFERENCE);
        }
    }else{
        // This is not a Reference Node
        return((XdmfXmlNode)XDMF_EMPTY_REFERENCE);
    }
    XdmfDebug("Setting ReferenceElement");
    this->ReferenceElement = anElement;
    this->SetIsReference(1);
    return(node);
}

XdmfConstString XdmfElement::Serialize(){
    if(!this->DOM) {
        XdmfErrorMessage("No DOM has been set");
        return(NULL);
    }
    if(!this->Element){
        XdmfErrorMessage("No XML Node has been set");
        return(NULL);
    }
    if(this->GetIsReference()){
        return(this->DOM->Serialize(this->ReferenceElement));
    }
    return(this->DOM->Serialize(this->Element));
}
XdmfConstString XdmfElement::GetElementType(){
    if(!this->DOM) {
        XdmfErrorMessage("No DOM has been set");
        return(NULL);
    }
    if(!this->Element){
        XdmfErrorMessage("No XML Node has been set");
        return(NULL);
    }
    return((XdmfConstString)this->Element->name);
}


XdmfInt32
XdmfElement::BuildFromDataXml(XdmfInt32 AllowCData){
    if(this->DataXml){
        if(AllowCData){
            char        first = 0;
            XdmfInt64   i = 0;

            while((first <= ' ') && (i < strlen(this->DataXml))){
                first = this->DataXml[i++];
                if((first > ' ') && (first != '<')){
                    this->Set("CData", this->DataXml);
                    return(XDMF_SUCCESS);
                }
            }
        }
        if(this->DOM){
            if(this->InsertedDataXml == this->DataXml){
                // Already done
                return(XDMF_SUCCESS);
            }
            if(this->DOM->InsertFromString(this->GetElement(), this->DataXml)){
                this->SetInsertedDataXml(this->DataXml);
                return(XDMF_SUCCESS);
            }else{
                XdmfErrorMessage("Error Inserting Raw XML : " << endl << this->DataXml);
                return(XDMF_FAIL);
            }
        }else{
            XdmfErrorMessage("Can't insert raw XML sine DOM is not set");
            return(XDMF_FAIL);
        }
    }
    return(XDMF_FAIL);
}

XdmfInt32 XdmfElement::Build(){
    XdmfConstString  name;

    XdmfInt32        i;

    name = this->GetName();
    if(name && (strlen(name) > 0)){
        this->Set("Name", name);
    }
    if(this->DOM){
        XdmfXmlNode myelement = this->GetElement();
        if(myelement){
            XdmfXmlNode childnode;
            XdmfElement *childelement;
            for(i=0;i<this->DOM->GetNumberOfChildren(myelement);i++){
                childnode = this->DOM->GetChild(i, myelement);
                childelement = (XdmfElement *)this->GetCurrentXdmfElement(childnode);
                if(childelement){
                    // cout << "Child Element Type of " << childelement << " = " << childelement->GetElementType() << endl;
                    childelement->Build();
                }
            }
        }
    }
    return(XDMF_SUCCESS);
}

XdmfInt32 XdmfElement::Set(XdmfConstString aName, XdmfConstString Value){
    if(!this->DOM) {
        XdmfErrorMessage("No DOM has been set");
        return(XDMF_FAIL);
    }
    if(!this->Element){
        XdmfErrorMessage("No XML Node has been set");
        return(XDMF_FAIL);
    }
    this->DOM->Set(this->Element, aName, Value);
    return(XDMF_SUCCESS);
}


XdmfConstString XdmfElement::Get(XdmfConstString aName){
    if(!this->DOM) {
        XdmfErrorMessage("No DOM has been set");
        return(NULL);
    }
    if(!this->Element){
        XdmfErrorMessage("No XML Node has been set");
        return(NULL);
    }
    return(this->DOM->Get(this->Element, aName));
}
