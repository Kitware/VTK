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
#include "XdmfAttribute.h"

#include "XdmfDataItem.h"
#include "XdmfDataDesc.h"
#include "XdmfArray.h"
#include "XdmfDOM.h"
#include "XdmfInformation.h"

namespace xdmf2
{

XdmfAttribute::XdmfAttribute() {
  this->SetElementName("Attribute");
  this->AttributeType = XDMF_ATTRIBUTE_TYPE_NONE;
  this->ValuesAreMine = 1;
  this->Values = NULL;
  this->ShapeDesc = new XdmfDataDesc();
  this->Active = 0;
  this->LightDataLimit = 100;
  this->Units = NULL;    // Ian Curington, HR Wallingford Ltd.
  }

XdmfAttribute::~XdmfAttribute() {
  if( this->ValuesAreMine && this->Values )  delete this->Values;
  delete this->ShapeDesc;
  if(this->Units) delete [] this->Units;  // Ian Curington, HR Wallingford Ltd.
}

XdmfInt32
XdmfAttribute::Release(){
  if( this->ValuesAreMine && this->Values )  delete this->Values;
  this->Values = NULL;
  return(XDMF_SUCCESS);
}

XdmfInt32
XdmfAttribute::Insert( XdmfElement *Child){
    if(Child && (
        XDMF_WORD_CMP(Child->GetElementName(), "DataItem") ||
        XDMF_WORD_CMP(Child->GetElementName(), "Information")
        )){
        return(XdmfElement::Insert(Child));
    }else{
        XdmfErrorMessage("Attribute can only Insert DataItem or Information elements");
    }
    return(XDMF_FAIL);
}

XdmfDataItem * XdmfAttribute::GetDataItem(){
    XdmfDataItem *di = NULL;
    XdmfXmlNode Node = this->DOM->FindDataElement(0, this->GetElement());
    
    if(Node) {
        di = (XdmfDataItem *)this->GetCurrentXdmfElement(Node);
    } 
    if(!di){
        di = new XdmfDataItem;
        Node = this->DOM->InsertNew(this->GetElement(), "DataItem");
        di->SetDOM(this->DOM);
        di->SetElement(Node);
    }
    return(di);
}

XdmfInformation * XdmfAttribute::GetInformation(const XdmfInt32 Index){
    if(Index < this->DOM->FindNumberOfElements("Information", this->Element )){
      XdmfInformation *di = NULL;
      XdmfXmlNode Node =  this->DOM->FindElement( "Information", Index, this->Element, 0);
      if(Node) {
        di = new XdmfInformation;
        di->SetDeleteOnGridDelete(true);
        di->SetDOM( this->DOM );
        di->SetElement( Node );
        di->UpdateInformation();
        return(di);
      }
    }else{
        XdmfErrorMessage("Grid has " << this->DOM->FindNumberOfElements("Information", this->Element ) << " Information. Index " << Index << " is out of range");
    }
    return(NULL);
}

XdmfInt32
XdmfAttribute::Build(){
    if(XdmfElement::Build() != XDMF_SUCCESS) return(XDMF_FAIL);
    if (this->GetActive())
      {
      this->Set("Active", "1");
      }
    this->Set("AttributeType", this->GetAttributeTypeAsString());
    this->Set("Center", this->GetAttributeCenterAsString());
    if(this->BuildFromDataXml() == XDMF_SUCCESS) return(XDMF_SUCCESS);
    if(this->Values){
        XdmfDataItem * di = this->GetDataItem(); 
        di->SetArray(this->Values);
        if(this->Values->GetNumberOfElements() > this->LightDataLimit) di->SetFormat(XDMF_FORMAT_HDF);
        di->Build();
        this->SetCurrentXdmfElement(di->GetElement(), NULL);
        delete di;
    }
// PATCH September 09, Ian Curington, HR Wallingford Ltd.
	if(this->Units)
	{
		this->Set("Units", this->GetUnits());
	}
// end patch

    return(XDMF_SUCCESS);
}

XdmfConstString
XdmfAttribute::GetAttributeTypeAsString( void ){
  switch ( this->AttributeType ){
    case XDMF_ATTRIBUTE_TYPE_SCALAR  :
      return("Scalar");
    case XDMF_ATTRIBUTE_TYPE_VECTOR  :
      return("Vector");
    case XDMF_ATTRIBUTE_TYPE_TENSOR  :
      return("Tensor");
    case XDMF_ATTRIBUTE_TYPE_MATRIX  :
      return("Matrix");
    case XDMF_ATTRIBUTE_TYPE_TENSOR6 :
      return("Tensor6");
    case XDMF_ATTRIBUTE_TYPE_GLOBALID :
      return("GlobalId");
    default :
      break;
    }
return("None");
}

XdmfInt32
XdmfAttribute::SetAttributeTypeFromString( XdmfConstString attributeType ){
XdmfInt64  Dimensions[3];

XdmfDebug("Setting Type to " << attributeType );
if( XDMF_WORD_CMP( attributeType, "Scalar" ) ) {
  this->AttributeType = XDMF_ATTRIBUTE_TYPE_SCALAR;
  Dimensions[0] = 1;
  this->ShapeDesc->SetShape( 1, Dimensions );
} else if( XDMF_WORD_CMP( attributeType, "Vector" ) ) {
  this->AttributeType = XDMF_ATTRIBUTE_TYPE_VECTOR;
  Dimensions[0] = 3;
  this->ShapeDesc->SetShape( 1, Dimensions );
} else if( XDMF_WORD_CMP( attributeType, "Tensor" ) ) {
  this->AttributeType = XDMF_ATTRIBUTE_TYPE_TENSOR;
  Dimensions[0] = 3;
  Dimensions[1] = 3;
  this->ShapeDesc->SetShape( 2, Dimensions );
} else if( XDMF_WORD_CMP( attributeType, "Matrix" ) ) {
  this->AttributeType = XDMF_ATTRIBUTE_TYPE_MATRIX;
} else if( XDMF_WORD_CMP( attributeType, "Tensor6" ) ) {
  this->AttributeType = XDMF_ATTRIBUTE_TYPE_TENSOR6;
  Dimensions[0] = 6;
} else if( XDMF_WORD_CMP( attributeType, "GlobalId" ) ) {
  this->AttributeType = XDMF_ATTRIBUTE_TYPE_GLOBALID;
  Dimensions[0] = 1;
} else {
  XdmfErrorMessage("Unknown Attribute Type " << attributeType );
  return( XDMF_FAIL );
  }
return( XDMF_SUCCESS );
}

XdmfConstString
XdmfAttribute::GetAttributeCenterAsString( void ){
  switch ( this->AttributeCenter ){
    case XDMF_ATTRIBUTE_CENTER_GRID :
      return( "Grid" );
    case XDMF_ATTRIBUTE_CENTER_CELL :
      return( "Cell" );
    case XDMF_ATTRIBUTE_CENTER_FACE :
      return( "Face" );
    case XDMF_ATTRIBUTE_CENTER_EDGE :
      return( "Edge" );
    case XDMF_ATTRIBUTE_CENTER_NODE :
      return( "Node" );
    default :
      break;
    }
return( "Node" );
}

XdmfInt32
XdmfAttribute::SetAttributeCenterFromString( XdmfConstString attributeCenter ){

if( XDMF_WORD_CMP( attributeCenter, "Grid" ) ) {
  this->AttributeCenter = XDMF_ATTRIBUTE_CENTER_GRID;
} else if( XDMF_WORD_CMP( attributeCenter, "Cell" ) ) {
  this->AttributeCenter = XDMF_ATTRIBUTE_CENTER_CELL;
} else if( XDMF_WORD_CMP( attributeCenter, "Face" ) ) {
  this->AttributeCenter = XDMF_ATTRIBUTE_CENTER_FACE;
} else if( XDMF_WORD_CMP( attributeCenter, "Edge" ) ) {
  this->AttributeCenter = XDMF_ATTRIBUTE_CENTER_EDGE;
} else if( XDMF_WORD_CMP( attributeCenter, "Node" ) ) {
  this->AttributeCenter = XDMF_ATTRIBUTE_CENTER_NODE;
} else {
  XdmfErrorMessage("Unknown Attribute Center " << attributeCenter );
  return( XDMF_FAIL );
  }
return( XDMF_SUCCESS );
}

XdmfInt32
XdmfAttribute::SetValues(XdmfArray *someValues){
    if(someValues == this->Values) return(XDMF_SUCCESS);
    if(this->ValuesAreMine && this->Values) delete this->Values;
    this->ValuesAreMine = 0;
    this->Values = someValues;
    return(XDMF_SUCCESS);
}

XdmfArray *
XdmfAttribute::GetValues(XdmfInt32 Create){
    if(!this->Values && Create){
        this->Values = new XdmfArray;
        this->ValuesAreMine = 1;
    }
    return(this->Values);
}

XdmfInt32
XdmfAttribute::UpdateInformation() {

XdmfConstString  Attribute;

if(XdmfElement::UpdateInformation() != XDMF_SUCCESS) return(XDMF_FAIL);
if( XDMF_WORD_CMP(this->GetElementType(), "Attribute") == 0){
    XdmfErrorMessage("Element type" << this->GetElementType() << " is not of type 'Attribute'");
    return(XDMF_FAIL);
}

Attribute = this->Get( "AttributeType" );
if(!Attribute) Attribute = this->Get( "Type" );
if( Attribute ){
  this->SetAttributeTypeFromString( Attribute );
} else {
  this->AttributeType = XDMF_ATTRIBUTE_TYPE_SCALAR;
}
free((void*)Attribute);

// PATCH September 09, Ian Curinton HR Wallingford Ltd.
Attribute = this->Get( "Units" );
if( Attribute ){
  this->SetUnits( Attribute );
} else {
  if(this->Units) delete [] this->Units;
  this->Units = NULL;
}
// end patch
free((void*)Attribute);

Attribute = this->Get( "Active" );
this->Active = 0;
if ( Attribute ){
  if( XDMF_WORD_CMP( Attribute, "1" ) ) {
    this->Active = 1;
  }
}
free((void*)Attribute);

Attribute = this->Get( "Center" );
if( Attribute ){
  this->SetAttributeCenterFromString( Attribute );
} else {
  this->AttributeCenter = XDMF_ATTRIBUTE_CENTER_NODE;
}
free((void*)Attribute);

Attribute = this->Get( "Dimensions" );
if( Attribute ){
  this->ShapeDesc->SetShapeFromString( Attribute );
}else{
    XdmfXmlNode  ValuesNode;
    ValuesNode = this->DOM->FindDataElement( 0, Element );
    if(!ValuesNode){
        XdmfErrorMessage("Dimensions of Attribute not set in XML and no DataItem found");
    }
    Attribute = this->DOM->Get( ValuesNode, "Dimensions" );
    if(!Attribute){
        XdmfErrorMessage("Dimensions of Attribute not set in XML or DataItem");
        free((void*)Attribute);
        return(XDMF_FAIL);
    }else{
        this->ShapeDesc->SetShapeFromString( Attribute );
    }
}
if(!this->Name) this->SetName(GetUnique("Attribute_"));
free((void*)Attribute);
return( XDMF_SUCCESS );
}

XdmfInt32
XdmfAttribute::Update() {

XdmfInt32  Status;
XdmfXmlNode  ValuesNode;
XdmfDataItem ValueReader;



if(XdmfElement::Update() != XDMF_SUCCESS) return(XDMF_FAIL);
if( this->AttributeType == XDMF_ATTRIBUTE_TYPE_NONE ){
  Status = this->UpdateInformation();
  if( Status == XDMF_FAIL ) {
    XdmfErrorMessage("Can't Initialize");
    return( XDMF_FAIL );
    }
  }

ValuesNode = this->DOM->FindDataElement( 0, Element );
if( ValuesNode ){
  ValueReader.SetDOM( this->DOM );
  ValueReader.SetDsmBuffer(this->DsmBuffer);
  if( this->ValuesAreMine && this->Values ){
    delete this->Values;
    this->Values = NULL;
  } else {
  }
  if(ValueReader.SetElement(ValuesNode) == XDMF_FAIL) return(XDMF_FAIL);
  if(ValueReader.UpdateInformation() == XDMF_FAIL) return(XDMF_FAIL);
  if(ValueReader.Update() == XDMF_FAIL) return(XDMF_FAIL);
  // Steal the array
  this->Values = ValueReader.GetArray();
  ValueReader.SetArrayIsMine(0);
  this->ValuesAreMine = 1;
  if( !this->Values ) {
    XdmfErrorMessage("Error Retriving Data Values");
    return( XDMF_FAIL );
    }
} else {
  XdmfErrorMessage("Element has no Data");
  return( XDMF_FAIL );
  }
return( XDMF_SUCCESS );
}

}
