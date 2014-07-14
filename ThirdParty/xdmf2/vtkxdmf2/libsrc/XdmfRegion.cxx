#include "XdmfRegion.h"

#include "XdmfDataItem.h"
#include "XdmfDataDesc.h"
#include "XdmfArray.h"
#include "XdmfDOM.h"

namespace xdmf2
{

XdmfRegion::XdmfRegion() {
  this->SetElementName("Region");
  this->ValuesAreMine = 1;
  this->RegionType = XDMF_REGION_TYPE_UNSET;
  this->Values = NULL;
  this->ShapeDesc = new XdmfDataDesc();
  this->Active = 0;
  }

XdmfRegion::~XdmfRegion() {
  if( this->ValuesAreMine && this->Values )  delete this->Values;
  delete this->ShapeDesc;
  }

XdmfInt32
XdmfRegion::Release(){
  if( this->ValuesAreMine && this->Values )  delete this->Values;
  this->Values = NULL;
  return(XDMF_SUCCESS);
}

XdmfInt32
XdmfRegion::Insert( XdmfElement *Child){
    if(Child && (
        XDMF_WORD_CMP(Child->GetElementName(), "Attribute") ||
        XDMF_WORD_CMP(Child->GetElementName(), "DataItem") ||
        XDMF_WORD_CMP(Child->GetElementName(), "Information")
        )){
        return(XdmfElement::Insert(Child));
    }else{
        XdmfErrorMessage("Region can only Insert Attribute, DataItem or Information elements");
    }
    return(XDMF_FAIL);
}

XdmfInt32
XdmfRegion::Build(){
    if(XdmfElement::Build() != XDMF_SUCCESS) return(XDMF_FAIL);
    this->Set("RegionType", this->GetRegionTypeAsString());
    if(this->Values){
        XdmfDataItem    *di = NULL;
        XdmfXmlNode     node;
        //! Is there already a DataItem
        node = this->DOM->FindDataElement(0, this->GetElement());
        if(node) {
            di = (XdmfDataItem *)this->GetCurrentXdmfElement(node);
        }
        if(!di){
            di = new XdmfDataItem;
            node = this->DOM->InsertNew(this->GetElement(), "DataItem");
            di->SetDOM(this->DOM);
            di->SetElement(node);
        }
        di->SetArray(this->Values);
        if(this->Values->GetNumberOfElements() > 100) di->SetFormat(XDMF_FORMAT_HDF);
        di->Build();

    }
    return(XDMF_SUCCESS);
}

XdmfConstString
XdmfRegion::GetRegionTypeAsString( void ){
  switch ( this->RegionType ){
    case XDMF_REGION_TYPE_CELL :
      return( "Cell" );
    case XDMF_REGION_TYPE_FACE :
      return( "Face" );
    case XDMF_REGION_TYPE_EDGE :
      return( "Edge" );
    case XDMF_REGION_TYPE_NODE :
      return( "Node" );
    case XDMF_REGION_TYPE_UNSET :
      return( "Unset" );
    default :
      break;
    }
    XdmfErrorMessage("Unknown RegionType = " << this->RegionType);
    return(0);
}


XdmfInt32
XdmfRegion::SetRegionTypeFromString( XdmfConstString regionType ){
if( XDMF_WORD_CMP( regionType, "Cell" ) ) {
  this->RegionType = XDMF_REGION_TYPE_CELL;
} else if( XDMF_WORD_CMP( regionType, "Face" ) ) {
  this->RegionType = XDMF_REGION_TYPE_FACE;
} else if( XDMF_WORD_CMP( regionType, "Edge" ) ) {
  this->RegionType = XDMF_REGION_TYPE_EDGE;
} else if( XDMF_WORD_CMP( regionType, "Node" ) ) {
  this->RegionType = XDMF_REGION_TYPE_NODE;
} else {
  XdmfErrorMessage("Unknown Region Type " << regionType );
  return( XDMF_FAIL );
  }
return( XDMF_SUCCESS );
}

XdmfInt32
XdmfRegion::SetValues(XdmfArray *someValues){
    if(someValues == this->Values) return(XDMF_SUCCESS);
    if(this->ValuesAreMine && this->Values) delete this->Values;
    this->ValuesAreMine = 0;
    this->Values = someValues;
    return(XDMF_SUCCESS);
}

XdmfArray *
XdmfRegion::GetValues(XdmfInt32 Create){
    if(!this->Values && Create){
        this->Values = new XdmfArray;
        this->ValuesAreMine = 1;
    }
    return(this->Values);
}

XdmfInt32
XdmfRegion::UpdateInformation() {

XdmfConstString  Region;

if(XdmfElement::UpdateInformation() != XDMF_SUCCESS) return(XDMF_FAIL);
if( XDMF_WORD_CMP(this->GetElementType(), "Region") == 0){
    XdmfErrorMessage("Element type" << this->GetElementType() << " is not of type 'Region'");
    return(XDMF_FAIL);
}

Region = this->Get( "Active" );
this->Active = 0;
if ( Region ){
  if( XDMF_WORD_CMP( Region, "1" ) ) {
    this->Active = 1;
  }
}

Region = this->Get( "RegionType" );
if( Region ){
  this->SetRegionTypeFromString( Region );
} else {
  this->RegionType = XDMF_REGION_TYPE_NODE;
}

Region= this->Get( "Dimensions" );
if( Region ){
  this->ShapeDesc->SetShapeFromString( Region );
}else{
    XdmfXmlNode  ValuesNode;
    ValuesNode = this->DOM->FindDataElement( 0, Element );
    if(!ValuesNode){
        XdmfErrorMessage("Dimensions of Region not set in XML and no DataItem found");
    }
    Region = this->DOM->Get( ValuesNode, "Dimensions" );
    if(!Region){
        XdmfErrorMessage("Dimensions of Region not set in XML or DataItem");
        return(XDMF_FAIL);
    }else{
        this->ShapeDesc->SetShapeFromString( Region );
    }
}
if(!this->Name) this->SetName(GetUnique("Region_"));
return( XDMF_SUCCESS );
}

XdmfInt32
XdmfRegion::Update() {

XdmfInt32  Status;
XdmfXmlNode  ValuesNode;
XdmfDataItem ValueReader;

// check this out
if(XdmfElement::Update() != XDMF_SUCCESS) return(XDMF_FAIL);

if( this->RegionType == XDMF_REGION_TYPE_UNSET ){
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
    XdmfErrorMessage("Error Retrieving Data Values");
    return( XDMF_FAIL );
    }
} else {
  XdmfErrorMessage("Element has no Data");
  return( XDMF_FAIL );
  }
return( XDMF_SUCCESS );
}

}
