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
#include "XdmfGrid.h"

#include "XdmfDOM.h"
#include "XdmfDataItem.h"
#include "XdmfArray.h"
#include "XdmfTopology.h"
#include "XdmfGeometry.h"
#include "XdmfInformation.h"
#include "XdmfAttribute.h"
#include "XdmfTime.h"
#include "XdmfSet.h"

namespace xdmf2
{

XdmfGrid *HandleToXdmfGrid( XdmfString Source ){
  XdmfObject  *TempObj;
  XdmfGrid   *Grid;

  TempObj = HandleToXdmfObject( Source );
  Grid = (XdmfGrid *)TempObj;
//  XdmfErrorMessage("Pointer = " << Source);
//  XdmfErrorMessage("Grid = " << Grid );
//  XdmfErrorMessage("Name " << Grid->GetClassName() );
//  XdmfErrorMessage("Type " << Grid->GetTopologyTypeAsString() );
  return( Grid );
  }

XdmfGrid::XdmfGrid() {
  this->SetElementName("Grid");
  this->Geometry = new XdmfGeometry;
  this->GeometryIsMine = 1;
  this->Topology = new XdmfTopology;
  this->TopologyIsMine = 1;
  this->Time = new XdmfTime;
  this->TimeIsMine = 1;
  this->Sets = (XdmfSet **)calloc(1, sizeof( XdmfSet * ));
  this->Attribute = (XdmfAttribute **)calloc(1, sizeof( XdmfAttribute * ));
  this->Children = (XdmfGrid **)calloc(1, sizeof( XdmfGrid * ));
  this->Informations = (XdmfInformation **)calloc(1, sizeof( XdmfInformation * ));
  this->AssignedAttribute = NULL;
  this->NumberOfSets = 0;
  this->NumberOfAttributes = 0;
  this->NumberOfInformations = 0;
  this->GridType = XDMF_GRID_UNSET;
  this->CollectionType = XDMF_GRID_COLLECTION_UNSET;
  this->NumberOfChildren = 0;
  this->Debug = 0;
  this->BuildTime = 0;
  }

XdmfGrid::~XdmfGrid() {
  XdmfInt32  Index;
  if( this->GeometryIsMine && this->Geometry ) delete this->Geometry;
  if( this->TopologyIsMine && this->Topology ) delete this->Topology;
  if( this->TimeIsMine && this->Time ) delete this->Time;
  for ( Index = 0; Index < this->NumberOfAttributes; Index ++ ){
    if (this->Attribute[Index]->GetDeleteOnGridDelete()){
        delete this->Attribute[Index];
    }
  }
  free(this->Attribute);
  for ( Index = 0; Index < this->NumberOfChildren; Index ++ ){
    if (this->Children[Index]->GetDeleteOnGridDelete()){
        delete this->Children[Index];
    }
  }
  free(this->Children);
  for ( Index = 0; Index < this->NumberOfSets; Index ++ ){
    if (this->Sets[Index]->GetDeleteOnGridDelete()){
        delete this->Sets[Index];
    }
  }
  free(this->Sets);
  for ( Index = 0; Index < this->NumberOfInformations; Index ++ ){
    if (this->Informations[Index]->GetDeleteOnGridDelete()){
        delete this->Informations[Index];
    }
  }
  free(this->Informations);
}

XdmfInt32
XdmfGrid::Release(){
  XdmfInt32  Index;
  if( this->GeometryIsMine && this->Geometry ) this->Geometry->Release();
  if( this->TopologyIsMine && this->Topology ) this->Topology->Release();
  for ( Index = 0; Index < this->NumberOfAttributes; Index ++ )
    {
    this->Attribute[Index]->Release();
    }
   return(XDMF_SUCCESS);
}

XdmfInt32
XdmfGrid::InsertTopology(){
    if(!this->Topology->GetElement()){
        XdmfXmlNode node;
        node = this->GetDOM()->InsertNew(this->GetElement(), "Topology");
        if(!node) return(XDMF_FAIL);
        this->Topology->SetDOM(this->GetDOM());
        if(this->Topology->SetElement(node) != XDMF_SUCCESS) return(XDMF_FAIL);
    } 
    return(XDMF_SUCCESS);
}
XdmfInt32
XdmfGrid::InsertGeometry(){
    if(!this->Geometry->GetElement()){
        XdmfXmlNode node;
        node = this->GetDOM()->InsertNew(this->GetElement(), "Geometry");
        if(!node) return(XDMF_FAIL);
        this->Geometry->SetDOM(this->GetDOM());
        if(this->Geometry->SetElement(node) != XDMF_SUCCESS) return(XDMF_FAIL);
    } 
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfGrid::Insert( XdmfElement *Child){
    if(Child && (
        XDMF_WORD_CMP(Child->GetElementName(), "Grid") ||
        XDMF_WORD_CMP(Child->GetElementName(), "Geometry") ||
        XDMF_WORD_CMP(Child->GetElementName(), "Topology") ||
        XDMF_WORD_CMP(Child->GetElementName(), "Attribute") ||
        XDMF_WORD_CMP(Child->GetElementName(), "Region") ||
        XDMF_WORD_CMP(Child->GetElementName(), "Set") ||
        XDMF_WORD_CMP(Child->GetElementName(), "DataItem") ||
        XDMF_WORD_CMP(Child->GetElementName(), "Time") ||
        XDMF_WORD_CMP(Child->GetElementName(), "Information")
        )){
        XdmfInt32   status = XdmfElement::Insert(Child);
        if((status == XDMF_SUCCESS) && XDMF_WORD_CMP(Child->GetElementName(), "Set")){
            XdmfSet *ChildSet = (XdmfSet *)Child;
            this->NumberOfSets++;
            this->Sets = ( XdmfSet **)realloc( this->Sets,
                this->NumberOfSets * sizeof( XdmfSet * ));
            if(!this->Sets) {
                XdmfErrorMessage("Realloc of Set List Failed");
                return(XDMF_FAIL);
            }
            this->Sets[this->NumberOfSets - 1] = ChildSet;
            }
        if((status == XDMF_SUCCESS) && XDMF_WORD_CMP(Child->GetElementName(), "Information")){
            XdmfInformation *ChildInfo = (XdmfInformation *)Child;
            this->NumberOfInformations++;
            this->Informations = ( XdmfInformation **)realloc( this->Informations,
                this->NumberOfInformations * sizeof( XdmfInformation * ));
            if(!this->Informations) {
                XdmfErrorMessage("Realloc of Information List Failed");
                return(XDMF_FAIL);
            }
            this->Informations[this->NumberOfInformations - 1] = ChildInfo;
            }
        if((status == XDMF_SUCCESS) && XDMF_WORD_CMP(Child->GetElementName(), "Attribute")){
            XdmfAttribute *ChildAttr = (XdmfAttribute *)Child;
            this->NumberOfAttributes++;
            this->Attribute = ( XdmfAttribute **)realloc( this->Attribute,
                this->NumberOfAttributes * sizeof( XdmfAttribute * ));
            if(!this->Attribute) {
                XdmfErrorMessage("Realloc of Attribute List Failed");
                return(XDMF_FAIL);
            }
            this->Attribute[this->NumberOfAttributes - 1] = ChildAttr;
            }
        if((status == XDMF_SUCCESS) && XDMF_WORD_CMP(Child->GetElementName(), "Grid")){
            XdmfGrid *ChildGrid = (XdmfGrid *)Child;
            XdmfInt32 nchild = this->NumberOfChildren + 1;
            this->Children = (XdmfGrid **)realloc(this->Children, nchild * sizeof(XdmfGrid *));
            this->Children[this->NumberOfChildren] = ChildGrid;
            this->NumberOfChildren = nchild;
            if((ChildGrid->GridType & XDMF_GRID_MASK) == XDMF_GRID_UNIFORM){
                if(ChildGrid->InsertTopology() != XDMF_SUCCESS) return(XDMF_FAIL);
                if(ChildGrid->InsertGeometry() != XDMF_SUCCESS) return(XDMF_FAIL);
            }
            return(XDMF_SUCCESS);
        }
        if (status == XDMF_SUCCESS) return(XDMF_SUCCESS);        
    }else{
        XdmfErrorMessage("Grid can only Insert Grid | Geometry | Topology | Attribute | Set | Region | DataItem | Information elements, not a " << Child->GetElementName());
    }
    return(XDMF_FAIL);
}

XdmfInt32
XdmfGrid::Build(){
    if(XdmfElement::Build() != XDMF_SUCCESS) return(XDMF_FAIL);
    this->Set("GridType", this->GetGridTypeAsString());
    if(this->GridType == XDMF_GRID_COLLECTION){
        this->Set("CollectionType", this->GetCollectionTypeAsString());
    }
    /*
    if((this->GridType & XDMF_GRID_MASK) == XDMF_GRID_UNIFORM){
        if(this->InsertTopology() != XDMF_SUCCESS) return(XDMF_FAIL);
        this->Topology->Build();
        if(this->InsertGeometry() != XDMF_SUCCESS) return(XDMF_FAIL);
        this->Geometry->Build();
    }else{
    }
    */
    if(this->BuildTime && this->Time){
            if(!this->Time->GetDOM()){
                if(this->Insert(this->Time) != XDMF_SUCCESS){
                    XdmfErrorMessage("Grid Cannot Insert current XdmfTime");
                    return(XDMF_FAIL);
                }
            }
            if(this->Time->Build() != XDMF_SUCCESS) return(XDMF_FAIL);
    }
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfGrid::SetCollectionTypeFromString(XdmfConstString aCollectionType){
    if(XDMF_WORD_CMP(aCollectionType, "Temporal")){
        this->SetCollectionType(XDMF_GRID_COLLECTION_TEMPORAL);
    }else if(XDMF_WORD_CMP(aCollectionType, "Spatial")){
        this->SetCollectionType(XDMF_GRID_COLLECTION_SPATIAL);
    }else{
        XdmfErrorMessage("Unknown Collection Type : " << aCollectionType);
        return(XDMF_FAIL);
    }
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfGrid::SetGridTypeFromString(XdmfConstString aGridType){
    if(XDMF_WORD_CMP(aGridType, "Uniform")){
        this->SetGridType(XDMF_GRID_UNIFORM);
    }else if(XDMF_WORD_CMP(aGridType, "Tree")){
        this->SetGridType(XDMF_GRID_TREE);
    }else if(XDMF_WORD_CMP(aGridType, "Collection")){
        this->SetGridType(XDMF_GRID_COLLECTION);
    }else if(XDMF_WORD_CMP(aGridType, "Subset")){
        this->SetGridType(XDMF_GRID_SUBSET);
    }else{
        XdmfErrorMessage("Unknown Grid Type : " << aGridType);
        return(XDMF_FAIL);
    }
    return(XDMF_SUCCESS);
}

XdmfConstString
XdmfGrid::GetCollectionTypeAsString(){
    if((this->GridType & XDMF_GRID_MASK) == XDMF_GRID_COLLECTION){
        switch(this->CollectionType){
            case XDMF_GRID_COLLECTION_TEMPORAL :
                return("Temporal");
            case XDMF_GRID_COLLECTION_SPATIAL :
                return("Spatial");
            default :
                return("Unset");
        }
    }
    return(0);
}

XdmfConstString
XdmfGrid::GetGridTypeAsString(){
    if(this->GridType & XDMF_GRID_MASK){
        switch(this->GridType & XDMF_GRID_MASK) {
            case XDMF_GRID_UNIFORM :
                return("Uniform");
            case XDMF_GRID_COLLECTION :
                return("Collection");
            case XDMF_GRID_TREE :
                return("Tree");
            case XDMF_GRID_SUBSET :
                return("Subset");
            default :
                XdmfErrorMessage("Unknown Grid Type");
                return(0);
        }
    }else{
        return("Uniform");
    }
}

// Derived Version
XdmfInt32
XdmfGrid::Copy(XdmfElement *Source){
    XdmfGrid *g;

    XdmfDebug("XdmfGrid::Copy(XdmfElement *Source)");
    g = (XdmfGrid *)Source;
//    cout << "Copy Grid Information from " << g << endl;
    this->Topology = g->GetTopology();
    this->TopologyIsMine = 0;
    if( this->GeometryIsMine && this->Geometry ) delete this->Geometry;
    this->Geometry = g->GetGeometry();
    this->GeometryIsMine = 0;
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfGrid::AssignAttribute( XdmfAttribute *attribute ){
XdmfInt32 Status = 0;

if( attribute ){
  if(!attribute->GetDsmBuffer()) attribute->SetDsmBuffer(this->DsmBuffer);
  attribute->Update();
  // Status = attribute->SetBaseAttribute( this, this->BaseGrid );
  this->AssignedAttribute = attribute;
} else {
  XdmfErrorMessage("Attribute is NULL");
  return( XDMF_FAIL );
}
return( Status );
}

XdmfInt32
XdmfGrid::AssignAttribute( XdmfInt64 Index ){
XdmfInt32 Status;

Status = this->AssignAttribute( this->Attribute[ Index ] );
return( Status );
}

XdmfInt32
XdmfGrid::AssignAttributeByIndex( XdmfInt64 Index ){
XdmfInt32 Status;

Status = this->AssignAttribute( this->Attribute[ Index ] );
return( Status );
}

XdmfInt32
XdmfGrid::AssignAttributeByName( XdmfString name ){
XdmfInt64 i;
XdmfInt32 Status = XDMF_FAIL;

for( i = 0 ; i < this->NumberOfAttributes ; i++ ){
  if( XDMF_WORD_CMP( this->Attribute[i]->GetName(), name ) ){
    Status = this->AssignAttribute( this->Attribute[ i ] );
    break;
  }
}
return( Status );
}

XdmfInt64
XdmfGrid::GetAssignedAttributeIndex( void ){
XdmfInt64 i;

for( i = 0 ; i < this->NumberOfAttributes ; i++ ){
  if( this->AssignedAttribute ==  this->Attribute[ i ] ){
    return( i );
    }
  }
return(0);
}

XdmfInt32
XdmfGrid::UpdateInformation() {

XdmfXmlNode anElement;
XdmfInt32  Status = XDMF_FAIL;
XdmfConstString  attribute;


if(XdmfElement::UpdateInformation() != XDMF_SUCCESS) return(XDMF_FAIL);
if( XDMF_WORD_CMP(this->GetElementType(), "Grid") == 0){
    XdmfErrorMessage("Element type" << this->GetElementType() << " is not of type 'Grid'");
    return(XDMF_FAIL);
}

// Allow for "GridType" or "Type"
attribute = this->Get("GridType");
if(!attribute) attribute = this->Get("Type");
if( XDMF_WORD_CMP(attribute, "Collection") ){
    free((void*)attribute);
    this->GridType = XDMF_GRID_COLLECTION;
    attribute = this->Get("CollectionType");
    if(attribute){
        if( XDMF_WORD_CMP(attribute, "Temporal") ){
            this->SetCollectionType(XDMF_GRID_COLLECTION_TEMPORAL);
        }else if( XDMF_WORD_CMP(attribute, "Spatial") ){
            this->SetCollectionType(XDMF_GRID_COLLECTION_SPATIAL);
        }
    }
}else if( XDMF_WORD_CMP(attribute, "Subset") ){
    this->GridType = XDMF_GRID_SUBSET;
}else if( XDMF_WORD_CMP(attribute, "Tree") ){
    this->GridType = XDMF_GRID_TREE;
}else if( XDMF_WORD_CMP(attribute, "Uniform") ){
    this->GridType = XDMF_GRID_UNIFORM;
}else{
    if(attribute){
        XdmfErrorMessage("Unknown Grid Type " << attribute);
        free((void*)attribute);
        return(XDMF_FAIL);
    }
    // If Type is NULL use default
    this->GridType = XDMF_GRID_UNIFORM;
}
free((void*)attribute);
if( this->GridType & XDMF_GRID_MASK){
    // SubSet Tree or Collection
    XdmfInt32  i, nchild;
    XdmfXmlNode node;

    nchild = this->NumberOfChildren;
    if (this->Children && nchild){
        for(i=0 ; i < nchild ; i++){
            delete this->Children[i];
        }
    }
    nchild = this->DOM->FindNumberOfElements("Grid", this->Element);
    this->NumberOfChildren = nchild;
    this->Children = (XdmfGrid **)realloc(this->Children, nchild * sizeof(XdmfGrid *));
    anElement = this->DOM->FindElement("Time", 0, this->Element);
    if(anElement){
        if(this->Time->SetDOM( this->DOM ) == XDMF_FAIL) return(XDMF_FAIL);
        if(this->Time->SetElement(anElement) == XDMF_FAIL) return(XDMF_FAIL);
        Status = this->Time->UpdateInformation();
        if( Status == XDMF_FAIL ){
            XdmfErrorMessage("Error Reading Time");
            return( XDMF_FAIL );
        }
    }
    anElement = this->DOM->FindElement("Geometry", 0, this->Element);
    for(i=0 ; i < nchild ; i++){
        if (i==0) {
          node = this->DOM->FindElement("Grid", i, this->Element);
        } else {
          node = this->DOM->FindNextElement("Grid", node);
        }

        if(!node) {
            XdmfErrorMessage("Can't find Child Grid #" << i);
            return(XDMF_FAIL);
        }
        this->Children[i] = new XdmfGrid;
        this->Children[i]->SetDeleteOnGridDelete(true);
        if(this->Children[i]->SetDOM(this->DOM) == XDMF_FAIL) {
          delete this->Children[i]; 
          return(XDMF_FAIL);
        }
        if(this->Children[i]->SetElement(node) == XDMF_FAIL) {
          delete this->Children[i];
          return(XDMF_FAIL);
        }
        // cout << "Calling update info for child " << i << endl;
        if(this->Children[i]->UpdateInformation() == XDMF_FAIL) {
          delete this->Children[i];
          return(XDMF_FAIL);
        }
        if(this->Time->GetTimeType() != XDMF_TIME_UNSET) {
          if(this->Children[i]->GetTime()->SetTimeFromParent(this->Time, i) != XDMF_SUCCESS) {
            delete this->Children[i];
            return(XDMF_FAIL);
          }
        }
    }
    if((this->GridType & XDMF_GRID_MASK) == XDMF_GRID_SUBSET){
        // Selection is the First Element Under Grid
        XdmfXmlNode select = 0;
        XdmfGrid        *target;
        XdmfDataDesc    *shape;

//        cout << "Getting SubGrid Selection " << endl;
        attribute = this->Get("Section");
        if( XDMF_WORD_CMP(attribute, "All") ){
//            cout << ":::: Selecting ALL" << endl;
            this->GridType |= XDMF_GRID_SECTION_ALL;
        }else if( XDMF_WORD_CMP(attribute, "DataItem") ){
//            cout << ":::: Selecting DataItem 1" << endl;
            this->GridType |= XDMF_GRID_SECTION_DATA_ITEM;
            select = this->DOM->FindElement("DataItem", 0, this->Element);
            if(!select){
                XdmfErrorMessage("Section = DataItem but DataItem == 0");
                free((void*)attribute);
                return(XDMF_FAIL);
            }
        }else{
            // default
//            cout << ":::: Selecting DataItem 2" << endl;
            select = this->DOM->FindElement("DataItem", 0, this->Element);
            if(select){
                this->GridType |= XDMF_GRID_SECTION_DATA_ITEM;
            }else{
                this->GridType |= XDMF_GRID_SECTION_ALL;
            }

        }
        free((void*)attribute);
        target = this->Children[0];
        if(!target){
            XdmfErrorMessage("No Target Grid Spceified for Subset");
            return(XDMF_FAIL);
        }

        if(this->GridType &  XDMF_GRID_SECTION_ALL) {
            /*
            shape = this->Topology->GetShapeDesc();
            shape->CopyShape(target->GetTopology()->GetShapeDesc());
            */
            if(this->TopologyIsMine && this->Topology) delete this->Topology;
            this->Topology = target->GetTopology();
            this->TopologyIsMine = 0;
        }else if(select){



            XdmfDataItem    *di = new XdmfDataItem;
            di->SetDOM(this->DOM);
            di->SetElement(select);
            di->UpdateInformation();
            di->Update();
//          cout << "UpdateInfo - Select Cells : " << di->GetArray()->GetValues() << endl;
            shape = this->Topology->GetShapeDesc();
            shape->CopyShape(di->GetDataDesc());
            delete di;
            this->Topology->SetTopologyType(target->GetTopology()->GetTopologyType());
            this->Topology->SetNodesPerElement(target->GetTopology()->GetNodesPerElement());
        }
    }
//    return(XDMF_SUCCESS);
}else{
// Handle Uniform Grid
    // cout << "Update Uniform Grid" << endl;
    anElement = this->DOM->FindElement("Time", 0, this->Element);
    if(anElement){
        if(this->Time->SetDOM( this->DOM ) == XDMF_FAIL) return(XDMF_FAIL);
        if(this->Time->SetElement(anElement) == XDMF_FAIL) return(XDMF_FAIL);
        Status = this->Time->UpdateInformation();
        if( Status == XDMF_FAIL ){
            XdmfErrorMessage("Error Reading Time");
            return( XDMF_FAIL );
        }
    }
    anElement = this->DOM->FindElement("Topology", 0, this->Element);
    if(anElement){
        if(this->Topology->SetDOM( this->DOM ) == XDMF_FAIL) return(XDMF_FAIL);
        if(this->Topology->SetElement(anElement) == XDMF_FAIL) return(XDMF_FAIL);
        Status = this->Topology->UpdateInformation();
        if( Status == XDMF_FAIL ){
            XdmfErrorMessage("Error Reading Topology");
            return( XDMF_FAIL );
        }
    }
    anElement = this->DOM->FindElement("Geometry", 0, this->Element);
    if(anElement){
        if(this->Geometry->SetDOM( this->DOM ) == XDMF_FAIL) return(XDMF_FAIL);
        if(this->Geometry->SetElement(anElement) == XDMF_FAIL) return(XDMF_FAIL);
        Status = this->Geometry->UpdateInformation();
        if( Status == XDMF_FAIL ){
            XdmfErrorMessage("Error Reading Geometry");
            return( XDMF_FAIL );
        }
    }
}
if(!this->Name) this->SetName( GetUnique("Grid_" ) );
// Get Attributes
XdmfInt32 OldNumberOfAttributes = this->NumberOfAttributes;
this->NumberOfAttributes = this->DOM->FindNumberOfElements("Attribute", this->Element );
if( this->NumberOfAttributes > 0 ){
  XdmfInt32  Index;
  XdmfAttribute  *iattribute;
  XdmfXmlNode    AttributeElement;

  for ( Index = 0; Index < OldNumberOfAttributes; Index ++ )
    {
    delete this->Attribute[Index];
    }
  this->Attribute = ( XdmfAttribute **)realloc( this->Attribute,
      this->NumberOfAttributes * sizeof( XdmfAttribute * ));
  for( Index = 0 ; Index < this->NumberOfAttributes ; Index++ ){
    iattribute = new XdmfAttribute;
    iattribute->SetDeleteOnGridDelete(true);
    this->Attribute[Index] = iattribute;
    if (Index == 0) {
      AttributeElement = this->DOM->FindElement( "Attribute", Index, this->Element );
    } else {
      AttributeElement = this->DOM->FindNextElement( "Attribute", AttributeElement);
    }
    iattribute->SetDOM( this->DOM );    
    iattribute->SetElement( AttributeElement );
    iattribute->UpdateInformation();
    }
}
// Get Sets
XdmfInt32 OldNumberOfSets = this->NumberOfSets;
this->NumberOfSets = this->DOM->FindNumberOfElements("Set", this->Element );
if( this->NumberOfSets > 0 ){
  XdmfInt32  Index;
  XdmfSet  *iSet;
  XdmfXmlNode    lSetElement;

  for ( Index = 0; Index < OldNumberOfSets; Index ++ )
    {
    delete this->Sets[Index];
    }
  this->Sets = ( XdmfSet **)realloc( this->Sets,
      this->NumberOfSets * sizeof( XdmfSet * ));
  for( Index = 0 ; Index < this->NumberOfSets ; Index++ ){
    iSet = new XdmfSet;
    iSet->SetDeleteOnGridDelete(true);
    this->Sets[Index] = iSet;
    if (Index==0) {
      lSetElement = this->DOM->FindElement( "Set", Index, this->Element );
    } else {
      lSetElement = this->DOM->FindNextElement( "Set", lSetElement);
    }
    iSet->SetDOM( this->DOM );    
    iSet->SetElement( lSetElement );
    iSet->UpdateInformation();
    }
}
// Get Information
XdmfInt32 OldNumberOfInformation = this->NumberOfInformations;
this->NumberOfInformations = this->DOM->FindNumberOfElements("Information", this->Element );
if( this->NumberOfInformations > 0 ){
  XdmfInt32  Index;
  XdmfInformation  *iInformation;
  XdmfXmlNode    lInformationElement;

  for ( Index = 0; Index < OldNumberOfInformation; Index ++ )
    {
    delete this->Informations[Index];
    }
  this->Informations = ( XdmfInformation **)realloc( this->Informations,
      this->NumberOfInformations * sizeof( XdmfInformation * ));
  for( Index = 0 ; Index < this->NumberOfInformations ; Index++ ){
    iInformation = new XdmfInformation;
    iInformation->SetDeleteOnGridDelete(true);
    this->Informations[Index] = iInformation;
    if (Index==0) {
      lInformationElement = this->DOM->FindElement( "Information", Index, this->Element, 0);
    } else {
      lInformationElement = this->DOM->FindNextElement( "Information", lInformationElement, 0);
    }
    iInformation->SetDOM( this->DOM );
    iInformation->SetElement( lInformationElement );
    iInformation->UpdateInformation();
    }
}

return( XDMF_SUCCESS );
}

XdmfInt32
XdmfGrid::Update() {



//cout << " In Update" << endl;
if(XdmfElement::Update() != XDMF_SUCCESS) return(XDMF_FAIL);
if(this->GridType == XDMF_GRID_UNSET) {
    if(this->UpdateInformation() == XDMF_FAIL){
        XdmfErrorMessage("Error Initializing Grid");
        return(XDMF_FAIL);
    }
}
if((this->GridType & XDMF_GRID_MASK) != XDMF_GRID_UNIFORM){
    XdmfInt32   i;
    // SubSet, Tree or Collection
    for(i=0; i < this->NumberOfChildren ; i++){
        if(!this->Children[i]->GetDsmBuffer()) this->Children[i]->SetDsmBuffer(this->DsmBuffer);
        if(this->Children[i]->Update() == XDMF_FAIL){
            XdmfErrorMessage("Error in Update() of Child Grid " << i);
            return(XDMF_FAIL);
        }
    }
    if((this->GridType & XDMF_GRID_MASK) == XDMF_GRID_SUBSET){
        // Selection is the First Element Under Grid
        XdmfXmlNode select;
        XdmfGrid        *target;

//        cout << " Getting SubSet" << endl;
        target = this->Children[0];
        if( this->GeometryIsMine && this->Geometry ) delete this->Geometry;
        this->Geometry = target->GetGeometry();
        this->GeometryIsMine = 0;

        if((this->GridType & XDMF_GRID_SECTION_MASK) == XDMF_GRID_SECTION_ALL){
//        cout << " Getting SubSet all" << endl;
            if(this->TopologyIsMine && this->Topology) delete this->Topology;
            this->Topology = target->GetTopology();
            this->TopologyIsMine = 0;
//            cout << "Conns = " << this->Topology->GetConnectivity()->GetValues() << endl;
//        cout << " Done Getting SubSet all" << endl;
        }else if((this->GridType & XDMF_GRID_SECTION_MASK) == XDMF_GRID_SECTION_DATA_ITEM){
//            cout << "Getting SubGrid Selection " << endl;
            select = this->DOM->FindDataElement(0, this->Element);
            if(select){
                XdmfDataItem    *di = new XdmfDataItem;
                XdmfArray       *celloff, *newconn;
                XdmfInt64       i1, o, o1, len, total;
                XdmfInt64       *cell, cellsize = 100;

                cell = new XdmfInt64[ cellsize ];
                di->SetDOM(this->DOM);
                di->SetElement(select);
                di->UpdateInformation();
                di->Update();
//                cout << "Update - Select Cells : " << di->GetArray()->GetValues() << endl;
                celloff = target->GetTopology()->GetCellOffsets();
                newconn = new XdmfArray;
                newconn->SetNumberOfElements(target->GetTopology()->GetConnectivity()->GetNumberOfElements());
                total = 0;
                for(i1=0; i1< di->GetArray()->GetNumberOfElements() ; i1++){
                    o = celloff->GetValueAsInt64(di->GetArray()->GetValueAsInt64(i1));
                    o1 = celloff->GetValueAsInt64(di->GetArray()->GetValueAsInt64(i1) + 1);
//                    cout << " Getting " << o << " thru " << o1 << endl;
                    len = o1 - o;
                    if(len > cellsize){
                        cellsize = len + 1;
                        delete [] cell;
                        cell = new XdmfInt64[ cellsize ];
                    }
//                    cout << " Conns = " << target->GetTopology()->GetConnectivity()->GetValues(o, len) << endl;
                    if(target->GetTopology()->GetConnectivity()->GetValues(o, cell, len) != XDMF_SUCCESS){
                        XdmfErrorMessage("Error Getting Cell Connectivity " << o << " to " << o1 );
                        return(XDMF_FAIL);
                    }
                    newconn->SetValues(total, cell, len);
//                    cout << " Offset " << i1 << " = " << o << " len = " << len << " total " << total << endl;
                    total += len;

                }
                newconn->SetNumberOfElements(total);
                this->Topology->SetConnectivity(newconn);
                delete [] cell;
            }
        }
    }
    return(XDMF_SUCCESS);
}
if(!this->Topology->GetDsmBuffer()) this->Topology->SetDsmBuffer(this->DsmBuffer);
if(this->Topology->Update() == XDMF_FAIL){
    XdmfErrorMessage("Error in Update() of Topology");
    return(XDMF_FAIL);
}
if(!this->Geometry->GetDsmBuffer()) this->Geometry->SetDsmBuffer(this->DsmBuffer);
if(this->Geometry->Update() == XDMF_FAIL){
    XdmfErrorMessage("Error in Update() of Geometry");
    return(XDMF_FAIL);
}
return( XDMF_SUCCESS );
}

XdmfGrid *
XdmfGrid::GetChild(XdmfInt32 Index){
    if(this->GridType & XDMF_GRID_MASK){
        if(Index < this->NumberOfChildren){
            return(this->Children[Index]);
        }else{
            XdmfErrorMessage("Grid has " << this->NumberOfChildren << " children. Index " << Index << " is out of range");
        }
    }else{
        XdmfErrorMessage("Grid is Uniform so it has no children");
    }
return(NULL);
}

XdmfInformation *
XdmfGrid::GetInformation(XdmfInt32 Index){
    if(Index < this->NumberOfInformations){
        return(this->Informations[Index]);
    }else{
        XdmfErrorMessage("Grid has " << this->NumberOfInformations << " children. Index " << Index << " is out of range");
    }
    return(NULL);
}

XdmfInt32
XdmfGrid::IsUniform(){
    if(this->GridType & XDMF_GRID_MASK) return(XDMF_FALSE);
    return(XDMF_TRUE);
}

XdmfInt32
XdmfGrid::FindGridsAtTime(XdmfTime *theTime, XdmfArray *ArrayToFill, XdmfFloat64 Epsilon, XdmfInt32 Append){
    XdmfInt64   i, index = 0, nchild;

    nchild = this->GetNumberOfChildren();
    if(!nchild) return(XDMF_FALSE);
    if(Append){
        index = ArrayToFill->GetNumberOfElements();
        ArrayToFill->SetNumberOfElements(index + nchild);
    }else{
        index = 0;
        ArrayToFill->SetNumberOfElements(nchild);
    }
    for(i=0 ; i < this->GetNumberOfChildren() ; i++){
        // cout << "IsValid(" << i << ") = " << this->GetChild(i)->GetTime()->IsValid(Time) << endl;
        if(this->GetChild(i)->GetTime()->IsValid(theTime)){
            ArrayToFill->SetValueFromInt64(index, i);
            index++;
        }
    }
    if(index){
        // Squeeze
        ArrayToFill->SetNumberOfElements(index);
    }else{
        return(XDMF_FAIL);
    }
return(XDMF_SUCCESS);
}

XdmfInt32
XdmfGrid::FindGridsInTimeRange(XdmfFloat64 TimeMin, XdmfFloat64 TimeMax, XdmfArray *ArrayToFill){
    XdmfInt64   i, index = 0, nchild;

    nchild = this->GetNumberOfChildren();
    if(!nchild) return(XDMF_FALSE);
    ArrayToFill->SetNumberType(XDMF_INT64_TYPE);
    ArrayToFill->SetNumberOfElements(nchild);
    for(i=0 ; i < nchild ; i++){
        // cout << "IsValid(" << i << ") = " << this->GetChild(i)->GetTime()->IsValid(TimeMin, TimeMax) << endl;
        if(this->GetChild(i)->GetTime()->IsValid(TimeMin, TimeMax)){
            ArrayToFill->SetValueFromInt64(index, i);
            index++;
        }
    }
    if(index){
        // Squeeze
        ArrayToFill->SetNumberOfElements(index);
    }else{
        return(XDMF_FAIL);
    }
return(XDMF_SUCCESS);
}

}
