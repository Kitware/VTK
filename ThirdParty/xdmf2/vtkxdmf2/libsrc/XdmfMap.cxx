#include "XdmfMap.h"

#include "XdmfDataItem.h"
#include "XdmfDataDesc.h"
#include "XdmfArray.h"
#include "XdmfDOM.h"

XdmfMap::XdmfMap() {
  this->SetElementName("Map");
  this->IdsAreMine = 1;
  this->MapIndexAreMine = 1;
  this->MapDataAreMine = 1;
  this->MapType = XDMF_MAP_TYPE_UNSET;
  this->Ids = NULL;
  this->MapIndex = NULL;
  this->MapData = NULL;
  this->ItemLength = 0;
  this->MapLength = 0;
  }

XdmfMap::~XdmfMap() {
  if( this->IdsAreMine && this->Ids )  delete this->Ids;
  if( this->MapIndexAreMine && this->MapIndex )  delete this->MapIndex;
  if( this->MapDataAreMine && this->MapData )  delete this->MapData;
  }

XdmfInt32
XdmfMap::Release(){
  if( this->IdsAreMine && this->Ids )  delete this->Ids;
  this->Ids = NULL;
  if( this->MapIndexAreMine && this->Ids )  delete this->MapIndex;
  this->MapIndex = NULL;
  if( this->MapDataAreMine && this->Ids )  delete this->MapData;
  this->MapData = NULL;
  return(XDMF_SUCCESS);
}

XdmfInt32
XdmfMap::Insert( XdmfElement *Child){
    if(Child && (
        XDMF_WORD_CMP(Child->GetElementName(), "DataItem") ||
        XDMF_WORD_CMP(Child->GetElementName(), "Information")
        )){
        return(XdmfElement::Insert(Child));
    }else{
        XdmfErrorMessage("Map can only Insert DataItem or Information elements");
    }
    return(XDMF_FAIL);
}

XdmfInt32
XdmfMap::Build(){
    if(XdmfElement::Build() != XDMF_SUCCESS) return(XDMF_FAIL);
    this->Set("MapType", this->GetMapTypeAsString());
    if(this->ItemLength > 0){
        ostrstream   StringOutput;
        StringOutput << this->ItemLength << ends;
        this->Set("ItemLength", StringOutput.str());
    }
    if(this->MapLength > 0){
        ostrstream   StringOutput;
        StringOutput << this->MapLength << ends;
        this->Set("MapLength", StringOutput.str());
    }
    if(this->Ids){
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
        di->SetArray(this->Ids);
        if(this->Ids->GetNumberOfElements() > 100) di->SetFormat(XDMF_FORMAT_HDF);
        di->Build();

    }
    if(this->MapIndex){
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
        di->SetArray(this->MapIndex);
        if(this->MapIndex->GetNumberOfElements() > 100) di->SetFormat(XDMF_FORMAT_HDF);
        di->Build();

    }
    if(this->MapData){
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
        di->SetArray(this->MapData);
        if(this->MapData->GetNumberOfElements() > 100) di->SetFormat(XDMF_FORMAT_HDF);
        di->Build();

    }
    return(XDMF_SUCCESS);
}

XdmfConstString
XdmfMap::GetMapTypeAsString( void ){
  switch ( this->MapType ){
    case XDMF_MAP_TYPE_CELL :
      return( "Cell" );
    case XDMF_MAP_TYPE_FACE :
      return( "Face" );
    case XDMF_MAP_TYPE_EDGE :
      return( "Edge" );
    case XDMF_MAP_TYPE_NODE :
      return( "Node" );
    case XDMF_MAP_TYPE_UNSET :
      return( "Unset" );
    default :
      break;
    }
    XdmfErrorMessage("Unknown MapType = " << this->MapType);
    return(0);
}


XdmfInt32
XdmfMap::SetMapTypeFromString( XdmfConstString regionType ){
if( XDMF_WORD_CMP( regionType, "Cell" ) ) {
  this->MapType = XDMF_MAP_TYPE_CELL;
} else if( XDMF_WORD_CMP( regionType, "Face" ) ) {
  this->MapType = XDMF_MAP_TYPE_FACE;
} else if( XDMF_WORD_CMP( regionType, "Edge" ) ) {
  this->MapType = XDMF_MAP_TYPE_EDGE;
} else if( XDMF_WORD_CMP( regionType, "Node" ) ) {
  this->MapType = XDMF_MAP_TYPE_NODE;
} else {
  XdmfErrorMessage("Unknown Set Type " << regionType );
  return( XDMF_FAIL );
  }
return( XDMF_SUCCESS );
}

// Ids
XdmfInt32
XdmfMap::SetIds(XdmfArray *someIds){
    if(someIds == this->Ids) return(XDMF_SUCCESS);
    if(this->IdsAreMine && this->Ids) delete this->Ids;
    this->IdsAreMine = 0;
    this->Ids = someIds;
    return(XDMF_SUCCESS);
}

XdmfArray *
XdmfMap::GetIds(XdmfInt32 Create){
    if(!this->Ids && Create){
        this->Ids = new XdmfArray;
        this->IdsAreMine = 1;
    }
    return(this->Ids);
}

// MapIndex
XdmfInt32
XdmfMap::SetMapIndex(XdmfArray *someMapIndex){
    if(someMapIndex == this->MapIndex) return(XDMF_SUCCESS);
    if(this->MapIndexAreMine && this->MapIndex) delete this->MapIndex;
    this->MapIndexAreMine = 0;
    this->MapIndex = someMapIndex;
    return(XDMF_SUCCESS);
}

XdmfArray *
XdmfMap::GetMapIndex(XdmfInt32 Create){
    if(!this->MapIndex && Create){
        this->MapIndex = new XdmfArray;
        this->MapIndexAreMine = 1;
    }
    return(this->MapIndex);
}

// MapData
XdmfInt32
XdmfMap::SetMapData(XdmfArray *someMapData){
    if(someMapData == this->MapData) return(XDMF_SUCCESS);
    if(this->MapDataAreMine && this->MapData) delete this->MapData;
    this->MapDataAreMine = 0;
    this->MapData = someMapData;
    return(XDMF_SUCCESS);
}

XdmfArray *
XdmfMap::GetMapData(XdmfInt32 Create){
    if(!this->MapData && Create){
        this->MapData = new XdmfArray;
        this->MapDataAreMine = 1;
    }
    return(this->MapData);
}

XdmfInt32
XdmfMap::UpdateInformation() {

XdmfConstString  Value;

if(XdmfElement::UpdateInformation() != XDMF_SUCCESS) return(XDMF_FAIL);
if( XDMF_WORD_CMP(this->GetElementType(), "Map") == 0){
    XdmfErrorMessage("Element type" << this->GetElementType() << " is not of type 'Map'");
    return(XDMF_FAIL);
}

Value = this->Get( "MapType" );
if( Value ){
  this->SetMapTypeFromString( Value );
} else {
  this->MapType = XDMF_MAP_TYPE_NODE;
}

Value = this->Get("ItemLength");
if(Value){
    XdmfInt32   iValue = 0;
    istrstream Value_ist(const_cast<char*>(Value), strlen(Value) );
    Value_ist >> iValue;
    this->SetItemLength(iValue);
}
Value = this->Get("MapLength");
if(Value){
    XdmfInt64   iValue = 0;
    istrstream Value_ist(const_cast<char*>(Value), strlen(Value) );
    Value_ist >> iValue;
    this->SetMapLength(iValue);
}
if(!this->Name) this->SetName(GetUnique("Map_"));
return( XDMF_SUCCESS );
}

XdmfInt32
XdmfMap::Update() {

XdmfInt32   Status;
XdmfInt32   NumberOfDataItems = 1;
XdmfInt32   i;

// check this out
if(XdmfElement::Update() != XDMF_SUCCESS) return(XDMF_FAIL);

if( this->MapType == XDMF_MAP_TYPE_UNSET ){
  Status = this->UpdateInformation();
  if( Status == XDMF_FAIL ) {
    XdmfErrorMessage("Can't Initialize");
    return( XDMF_FAIL );
    }
  }

NumberOfDataItems = this->DOM->FindNumberOfElements("DataItem", this->Element);
if(NumberOfDataItems < 2){
    XdmfErrorMessage("Map must have at least 2 DataItems");
    return(XDMF_FAIL);
}

for(i=0 ; i < NumberOfDataItems ; i++){
    XdmfXmlNode IdsNode;
    XdmfInt32   *Mine;
    XdmfArray   **Array;

    if(NumberOfDataItems == 2){
        switch (i){
            case 0 :
                Mine = &this->MapIndexAreMine;
                Array = &this->MapIndex;
                break;
            default  :
                Mine = &this->MapDataAreMine;
                Array = &this->MapData;
                break;
        }
    }else{
        // NumberOfDataItems == 3
        switch (i){
            case 0 :
                Mine = &this->IdsAreMine;
                Array = &this->Ids;
                break;
            case 1 :
                Mine = &this->MapIndexAreMine;
                Array = &this->MapIndex;
                break;
            default  :
                Mine = &this->MapDataAreMine;
                Array = &this->MapData;
                break;
        }
    }
    IdsNode = this->DOM->FindDataElement(i, Element );
    if( IdsNode ){
        XdmfDataItem ValueReader;
        ValueReader.SetDOM( this->DOM );
        ValueReader.SetDsmBuffer(this->DsmBuffer);
        if(ValueReader.SetElement(IdsNode) == XDMF_FAIL) return(XDMF_FAIL);
        if(ValueReader.UpdateInformation() == XDMF_FAIL) return(XDMF_FAIL);
        if(ValueReader.Update() == XDMF_FAIL) return(XDMF_FAIL);
        if( *Mine && *Array){
            delete *Array;
            *Mine = 0;
        }
        // Steal the array
        *Array = ValueReader.GetArray();
        if( *Array == 0 ) {
            XdmfErrorMessage("Error Retrieving Data Ids");
            return( XDMF_FAIL );
        }
        ValueReader.SetArrayIsMine(0);
        *Mine = 1;
    } else {
        XdmfErrorMessage("Map does not have enough DataItems. Error reading DataItem #" << i);
        return( XDMF_FAIL );
    }
}
return( XDMF_SUCCESS );
}
