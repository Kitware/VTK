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
/*     Copyright @ 2007 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/
#include "XdmfGeometry.h"

#include "XdmfTopology.h"
#include "XdmfDataItem.h"
#include "XdmfArray.h"
#include "XdmfDOM.h"
#include "XdmfHDF.h"

XdmfGeometry *GetXdmfGeometryHandle( void *Pointer ){
  //XdmfGeometry *tmp = (XdmfGeometry *)Pointer;
  return((XdmfGeometry *)Pointer);
}

XdmfGeometry::XdmfGeometry() {
  this->SetElementName("Geometry");
  this->GeometryType = XDMF_GEOMETRY_NONE;
  this->Points = NULL;
  this->PointsAreMine = 1;
  this->VectorX = NULL;
  this->VectorY = NULL;
  this->VectorZ = NULL;
  this->SetOrigin( 0, 0, 0 );
  this->SetDxDyDz( 0, 0, 0 );
  this->LightDataLimit = 100;
  this->Units = NULL;       // Ian Curington, HR Wallingford Ltd.
  this->VectorXIsMine = 0;
  this->VectorYIsMine = 0;
  this->VectorZIsMine = 0;
}

XdmfGeometry::~XdmfGeometry() 
{
  if( this->PointsAreMine && this->Points )  delete this->Points;
  if(this->Units) delete [] this->Units;    // Ian Curington, HR Wallingford Ltd.
  if(this->VectorX && this->VectorXIsMine) delete this->VectorX;
  if(this->VectorY && this->VectorYIsMine) delete this->VectorY;
  if(this->VectorZ && this->VectorZIsMine) delete this->VectorZ;
}

XdmfInt32
XdmfGeometry::Release()
{
  XdmfXmlNode node;
  XdmfInt32 Index = 0;
  XdmfXmlNode Node;

  if( this->PointsAreMine && this->Points ){
    delete this->Points;
    this->Points = NULL;
  }
  // this->NumberOfPoints = 0;
  Node = this->GetElement();
  node = this->DOM->FindDataElement(Index++, Node);
  // No Need to Release DataItems() since Data has been read 
  // and Stored in Internal Points
  return(XDMF_SUCCESS);
}
// Returns an existing DataItem or build a new one
XdmfDataItem *
XdmfGeometry::GetDataItem(XdmfInt32 Index, XdmfXmlNode Node){
  XdmfDataItem *di = NULL;
  XdmfXmlNode node;

  node = this->DOM->FindDataElement(Index, Node);
  if(node) {
    di = (XdmfDataItem *)this->GetCurrentXdmfElement(node);
  }
  if(!di){
    di = new XdmfDataItem;
    node = this->DOM->InsertNew(this->GetElement(), "DataItem");
    di->SetDOM(this->DOM);
    di->SetElement(node);
  }
  return(di);
}


XdmfInt32
XdmfGeometry::Build(){
  XdmfDataItem    *di = NULL;
  XdmfArray       *array;

  //cout << "Building Geometry" << endl;
  if(XdmfElement::Build() != XDMF_SUCCESS) return(XDMF_FAIL);
  this->Set("GeometryType", this->GetGeometryTypeAsString());
  // Build Children from String , if provided
  if(this->BuildFromDataXml() == XDMF_SUCCESS) return(XDMF_SUCCESS);
  switch( this->GeometryType ){
      case XDMF_GEOMETRY_VXVYVZ:
        if(!this->VectorX || !this->VectorY || !this->VectorZ){
          XdmfErrorMessage("Vx Vy and Vz must be set");
          return(XDMF_FAIL);
        }
        // Vx
        di = this->GetDataItem(0, this->GetElement());
        di->SetArray(this->VectorX);
        if(this->VectorX->GetNumberOfElements() > this->LightDataLimit) di->SetFormat(XDMF_FORMAT_HDF);
        di->Build();
        this->SetCurrentXdmfElement(di->GetElement(), NULL);
        delete di;
        // Vy
        di = this->GetDataItem(1, this->GetElement());
        di->SetArray(this->VectorY);
        if(this->VectorY->GetNumberOfElements() > this->LightDataLimit) di->SetFormat(XDMF_FORMAT_HDF);
        di->Build();
        this->SetCurrentXdmfElement(di->GetElement(), NULL);
        delete di;
        // Vx
        di = this->GetDataItem(2, this->GetElement());
        di->SetArray(this->VectorZ);
        if(this->VectorZ->GetNumberOfElements() > this->LightDataLimit) di->SetFormat(XDMF_FORMAT_HDF);
        di->Build();
        this->SetCurrentXdmfElement(di->GetElement(), NULL);
        delete di;
        break;
      case XDMF_GEOMETRY_VXVY:
        if(!this->VectorX || !this->VectorY){
          XdmfErrorMessage("Vx and Vy  must be set");
          return(XDMF_FAIL);
        }
        // Vx
        di = this->GetDataItem(0, this->GetElement());
        di->SetArray(this->VectorX);
        if(this->VectorX->GetNumberOfElements() > this->LightDataLimit) di->SetFormat(XDMF_FORMAT_HDF);
        di->Build();
        this->SetCurrentXdmfElement(di->GetElement(), NULL);
        delete di;
        // Vy
        di = this->GetDataItem(1, this->GetElement());
        di->SetArray(this->VectorY);
        if(this->VectorY->GetNumberOfElements() > this->LightDataLimit) di->SetFormat(XDMF_FORMAT_HDF);
        di->Build();
        this->SetCurrentXdmfElement(di->GetElement(), NULL);
        delete di;
        break;
      case XDMF_GEOMETRY_ORIGIN_DXDYDZ:
        // Origin
        di = this->GetDataItem(0, this->GetElement());
        di->SetFormat(XDMF_FORMAT_XML);
        di->SetName("Origin");
        array = di->GetArray();
        array->SetNumberOfElements(3);
        array->SetValues(0, this->Origin, 3);
        di->Build();
        this->SetCurrentXdmfElement(di->GetElement(), NULL);
        delete di;
        // DxDyDz
        di = this->GetDataItem(1, this->GetElement());
        di->SetFormat(XDMF_FORMAT_XML);
        di->SetName("Spacing");
        array = di->GetArray();
        array->SetNumberOfElements(3);
        array->SetValues(0, this->DxDyDz, 3);
        di->Build();
        this->SetCurrentXdmfElement(di->GetElement(), NULL);
        delete di;
        break;
      case XDMF_GEOMETRY_ORIGIN_DXDY:
        // Origin
        di = this->GetDataItem(0, this->GetElement());
        di->SetFormat(XDMF_FORMAT_XML);
        di->SetName("Origin");
        array = di->GetArray();
        array->SetNumberOfElements(2);
        array->SetValues(0, this->Origin, 2);
        di->Build();
        this->SetCurrentXdmfElement(di->GetElement(), NULL);
        delete di;
        // DxDy
        di = this->GetDataItem(1, this->GetElement());
        di->SetFormat(XDMF_FORMAT_XML);
        di->SetName("Spacing");
        array = di->GetArray();
        array->SetNumberOfElements(2);
        array->SetValues(0, this->DxDyDz, 2);
        di->Build();
        this->SetCurrentXdmfElement(di->GetElement(), NULL);
        delete di;
        break;
      case XDMF_GEOMETRY_NONE:
        break;
      default :
        if(this->Points){
          di = this->GetDataItem(0, this->GetElement());
          di->SetArray(this->Points);
          if(this->Points->GetNumberOfElements() > this->LightDataLimit) di->SetFormat(XDMF_FORMAT_HDF);
          di->Build();
          this->SetCurrentXdmfElement(di->GetElement(), NULL);
          delete di;
        }else{
          XdmfErrorMessage("XdmfGeometry->Points must be set for Geometry Type " << this->GetGeometryTypeAsString());
          return(XDMF_FAIL);
        }
        break;
  }

  // PATCH September 09,  Ian Curington, HR Wallingford Ltd.
  if(this->Units)
  {
    this->Set("Units", this->GetUnits());
  }
  // end patch

  return(XDMF_SUCCESS);
}

XdmfInt32
XdmfGeometry::Insert( XdmfElement *Child){
  if(Child && (
    XDMF_WORD_CMP(Child->GetElementName(), "DataItem") ||
    XDMF_WORD_CMP(Child->GetElementName(), "Information")
    )){
      return(XdmfElement::Insert(Child));
  }else{
    XdmfErrorMessage("Geometry can only Insert DataItem or Information elements");
  }
  return(XDMF_FAIL);
}

XdmfInt32
XdmfGeometry::SetOrigin( XdmfFloat64 X, XdmfFloat64 Y, XdmfFloat64 Z ){

  this->Origin[0] = X;
  this->Origin[1] = Y;
  this->Origin[2] = Z;
  return( XDMF_SUCCESS );
}

XdmfInt32
XdmfGeometry::SetOrigin( XdmfFloat64 *origin ){
  return( this->SetOrigin( origin[0], origin[1], origin[2] ) );
}

XdmfInt32
XdmfGeometry::SetDxDyDz( XdmfFloat64 Dx, XdmfFloat64 Dy, XdmfFloat64 Dz ){
  this->DxDyDz[0] = Dx;
  this->DxDyDz[1] = Dy;
  this->DxDyDz[2] = Dz;
  return( XDMF_SUCCESS );
}


XdmfInt32
XdmfGeometry::SetDxDyDz( XdmfFloat64 *dxDyDz ){
  return( this->SetDxDyDz( dxDyDz[0], dxDyDz[1], dxDyDz[2] ) );
}


XdmfArray *
XdmfGeometry::GetPoints(XdmfInt32 Create){
  if(!this->Points && Create){
    this->Points = new XdmfArray;
    this->PointsAreMine = 1;
  }
  return(this->Points);
}

XdmfInt32
XdmfGeometry::SetPoints( XdmfArray *points ){
  if(this->Points == points) return(XDMF_SUCCESS);
  if( this->PointsAreMine && this->Points ) delete this->Points;
  this->PointsAreMine = 0;
  this->Points = points;
  return( XDMF_SUCCESS );
}

XdmfInt32
XdmfGeometry::SetGeometryTypeFromString( XdmfConstString geometryType ){

  if( XDMF_WORD_CMP( geometryType, "X_Y_Z" ) ){
    this->GeometryType = XDMF_GEOMETRY_X_Y_Z;
    return(XDMF_SUCCESS);
  }
  if( XDMF_WORD_CMP( geometryType, "X_Y" ) ){
    this->GeometryType = XDMF_GEOMETRY_X_Y;
    return(XDMF_SUCCESS);
  }
  if( XDMF_WORD_CMP( geometryType, "XY" ) ){
    this->GeometryType = XDMF_GEOMETRY_XY;
    return(XDMF_SUCCESS);
  }
  if( XDMF_WORD_CMP( geometryType, "XYZ" ) ){
    this->GeometryType = XDMF_GEOMETRY_XYZ;
    return(XDMF_SUCCESS);
  }
  if( XDMF_WORD_CMP( geometryType, "ORIGIN_DXDYDZ" ) ){
    this->GeometryType = XDMF_GEOMETRY_ORIGIN_DXDYDZ;
    return(XDMF_SUCCESS);
  }
  if( XDMF_WORD_CMP( geometryType, "ORIGIN_DXDY" ) ){
    this->GeometryType = XDMF_GEOMETRY_ORIGIN_DXDY;
    return(XDMF_SUCCESS);
  }
  if( XDMF_WORD_CMP( geometryType, "VXVYVZ" ) ){
    this->GeometryType = XDMF_GEOMETRY_VXVYVZ;
    return(XDMF_SUCCESS);
  }
  if( XDMF_WORD_CMP( geometryType, "VXVY" ) ){
    this->GeometryType = XDMF_GEOMETRY_VXVY;
    return(XDMF_SUCCESS);
  }
  if( XDMF_WORD_CMP( geometryType, "NONE" ) ){
    this->GeometryType = XDMF_GEOMETRY_NONE;
    return(XDMF_SUCCESS);
  }

  return( XDMF_FAIL );
}

XdmfString
XdmfGeometry::GetGeometryTypeAsString( void ){
  static char Value[80];

  switch( this->GeometryType ){
  case XDMF_GEOMETRY_VXVYVZ:
    strcpy( Value, "VXVYVZ" );
    break;
  case XDMF_GEOMETRY_VXVY:
    strcpy( Value, "VXVY" );
    break;
  case XDMF_GEOMETRY_ORIGIN_DXDYDZ:
    strcpy( Value, "ORIGIN_DXDYDZ" );
    break;
  case XDMF_GEOMETRY_ORIGIN_DXDY:
    strcpy( Value, "ORIGIN_DXDY" );
    break;
  case XDMF_GEOMETRY_X_Y_Z :
    strcpy( Value, "X_Y_Z" );
    break;
  case XDMF_GEOMETRY_X_Y :
    strcpy( Value, "X_Y" );
    break;
  case XDMF_GEOMETRY_XY :
    strcpy( Value, "XY" );
    break;
  case XDMF_GEOMETRY_NONE :
    strcpy( Value, "NONE");
    break;
  default :
    strcpy( Value, "XYZ" );
    break;
  }
  return( Value );
}

XdmfInt32
XdmfGeometry::UpdateInformation() {
  XdmfConstString  Attribute;

  if(XdmfElement::UpdateInformation() != XDMF_SUCCESS) return(XDMF_FAIL);
  if( XDMF_WORD_CMP(this->GetElementType(), "Geometry") == 0){
    XdmfErrorMessage("Element type" << this->GetElementType() << " is not of type 'Geometry'");
    return(XDMF_FAIL);
  }

  // PATCH September 09, Ian Curington, HR Wallingford Ltd.
  Attribute = this->Get( "Units" );
  if( Attribute ){
    this->SetUnits( Attribute );
  } else {
    if(this->Units) delete [] this->Units;
    this->Units = NULL;
  }
  // end patch
  free((void*)Attribute);
  Attribute = this->Get( "GeometryType" );
  if(!Attribute){
    Attribute = this->Get( "Type" );
  }
  if( Attribute ){
    if(this->SetGeometryTypeFromString( Attribute ) != XDMF_SUCCESS){
      XdmfErrorMessage("No such Geometry Type : " << Attribute);
      free((void*)Attribute);
      return(XDMF_FAIL);
    }
  } else {
    this->GeometryType = XDMF_GEOMETRY_XYZ;
  }
  if(!this->Name) this->SetName(GetUnique("Geometry_"));
  free((void*)Attribute);
  return( XDMF_SUCCESS );
}

XdmfInt32
XdmfGeometry::Update() {


  XdmfInt32  ArrayIndex;
  XdmfInt64  Start[ XDMF_MAX_DIMENSION ];
  XdmfInt64  Stride[ XDMF_MAX_DIMENSION ];
  XdmfInt64  Count[ XDMF_MAX_DIMENSION ];
  XdmfXmlNode     PointsElement;
  XdmfArray       *points = NULL;
  XdmfArray       *TmpArray;

  if( this->GeometryType == XDMF_GEOMETRY_NONE ){
    if( this->UpdateInformation() == XDMF_FAIL ){
      XdmfErrorMessage("Can't Initialize From Element");
      return( XDMF_FAIL );
    }
  }
  if(XdmfElement::Update() != XDMF_SUCCESS) return(XDMF_FAIL);
  for(ArrayIndex = 0 ; ArrayIndex < XDMF_MAX_DIMENSION ; ArrayIndex++){
    Start[ArrayIndex] = 0;
    Stride[ArrayIndex] = 1;
    Count[ArrayIndex] = 0;
  }
  ArrayIndex = 0;
  if( ( this->GeometryType == XDMF_GEOMETRY_X_Y_Z ) ||
    ( this->GeometryType == XDMF_GEOMETRY_X_Y ) ||
    ( this->GeometryType == XDMF_GEOMETRY_XYZ ) ||
    ( this->GeometryType == XDMF_GEOMETRY_XY ) ){
      do {
        // Read the Data
        XdmfDebug("Reading Points ( SubElement #" << ArrayIndex + 1 << " )" );
        PointsElement = this->DOM->FindDataElement( ArrayIndex, Element );
        if( PointsElement ){
          XdmfDataItem PointsItem;
          if(PointsItem.SetDOM( this->DOM ) == XDMF_FAIL) return(XDMF_FAIL);
          if(PointsItem.SetElement(PointsElement, 0) == XDMF_FAIL) return(XDMF_FAIL);
          if(PointsItem.UpdateInformation() == XDMF_FAIL) return(XDMF_FAIL);
          PointsItem.SetDsmBuffer(this->DsmBuffer);
          if(PointsItem.Update() == XDMF_FAIL) return(XDMF_FAIL);
          TmpArray = PointsItem.GetArray();
          if( TmpArray ){
            if( !points ){
              switch( this->GeometryType ){
                case XDMF_GEOMETRY_X_Y_Z :
                  points = new XdmfArray;
                  points->CopyType( TmpArray );
                  points->SetNumberOfElements( TmpArray->GetNumberOfElements() * 3 );
                  break;
                case XDMF_GEOMETRY_XY :
                  points = new XdmfArray;
                  points->CopyType( TmpArray );
                  // points->SetNumberOfElements( TmpArray->GetNumberOfElements() / 2 * 3 );
                  Start[0] = TmpArray->GetNumberOfElements() / 2;
                  Start[1] = 3;
                  points->SetShape( 2 , Start );
                  // operator '=' is overloaded
                  *points = 0;
                  break;
                case XDMF_GEOMETRY_X_Y :
                  points = new XdmfArray;
                  points->CopyType( TmpArray );
                  points->SetNumberOfElements( TmpArray->GetNumberOfElements() * 3 );
                  // operator '=' is overloaded
                  *points = 0;
                  break;
                default :
                  points = TmpArray;
                  // Assure DataItem Destructor does not delete XdmfArray
                  PointsItem.SetArrayIsMine(0);
                  break;
              }
            }
            // We Have made Points Rank = 1 if not XYZ
            switch( this->GeometryType ){
            case XDMF_GEOMETRY_X_Y_Z :
              Start[0] = ArrayIndex;
              Stride[0] = 3;
              points->SelectHyperSlab( Start, Stride, NULL );
              CopyArray( TmpArray, points);
              this->NumberOfPoints = TmpArray->GetNumberOfElements();
              break;
            case XDMF_GEOMETRY_XY :
              Stride[0] = 1;
              Stride[1] = 1;
              Count[0] = TmpArray->GetNumberOfElements() / 2;
              Count[1] = 2;
              points->SelectHyperSlab( NULL, Stride, Count);
              CopyArray( TmpArray, points);
              this->NumberOfPoints = TmpArray->GetNumberOfElements() / 2 ;
              break;
            case XDMF_GEOMETRY_X_Y :
              Start[0] = ArrayIndex;
              Stride[0] = 3;
              points->SelectHyperSlab( Start, Stride, NULL );
              CopyArray( TmpArray, points);
              this->NumberOfPoints = TmpArray->GetNumberOfElements();
              break;
            default :
              // points = TmpArray so do nothing
              this->NumberOfPoints = TmpArray->GetNumberOfElements() / 3;
              break;
            }
          }
        } 
        ArrayIndex++;
      } while( ( ArrayIndex < 3 ) && ( PointsElement != NULL ) );
  } else {
    if( this->GeometryType == XDMF_GEOMETRY_ORIGIN_DXDYDZ ) {
      XdmfDataItem PointsItem;
      PointsItem.SetDOM(this->DOM);
      PointsItem.SetDsmBuffer(this->DsmBuffer);
      XdmfDebug("Reading Origin and Dx, Dy, Dz" );
      PointsElement = this->DOM->FindDataElement(0, this->Element );
      if( PointsElement ){
        if(PointsItem.SetElement(PointsElement, 0) == XDMF_FAIL) return(XDMF_FAIL);
        if(PointsItem.UpdateInformation() == XDMF_FAIL) return(XDMF_FAIL);
        if(PointsItem.Update() == XDMF_FAIL) return(XDMF_FAIL);
        TmpArray = PointsItem.GetArray();
        if( TmpArray ){
          TmpArray->GetValues( 0, this->Origin, 3 );
        }
        PointsElement = this->DOM->FindDataElement(1, this->Element );
        if( PointsElement ){
          if(PointsItem.SetElement(PointsElement, 0) == XDMF_FAIL) return(XDMF_FAIL);
          if(PointsItem.UpdateInformation() == XDMF_FAIL) return(XDMF_FAIL);
          if(PointsItem.Update() == XDMF_FAIL) return(XDMF_FAIL);
          TmpArray = PointsItem.GetArray();
          if( TmpArray ){
            TmpArray->GetValues( 0, this->DxDyDz, 3 );
          }
        } else {
          XdmfErrorMessage("No Dx, Dy, Dz Specified");
          return( XDMF_FAIL );
        }
      } else {
        XdmfErrorMessage("No Origin Specified");
        return( XDMF_FAIL );
      }
    } else if( this->GeometryType == XDMF_GEOMETRY_ORIGIN_DXDY ) {
      XdmfDataItem PointsItem;
      PointsItem.SetDOM(this->DOM);
      PointsItem.SetDsmBuffer(this->DsmBuffer);
      XdmfDebug("Reading Origin and Dx, Dy" );
      PointsElement = this->DOM->FindDataElement(0, this->Element );
      if( PointsElement ){
        if(PointsItem.SetElement(PointsElement, 0) == XDMF_FAIL) return(XDMF_FAIL);
        if(PointsItem.UpdateInformation() == XDMF_FAIL) return(XDMF_FAIL);
        if(PointsItem.Update() == XDMF_FAIL) return(XDMF_FAIL);
        TmpArray = PointsItem.GetArray();
        if( TmpArray ){
          TmpArray->GetValues( 0, this->Origin, 2 );
        }
        PointsElement = this->DOM->FindDataElement(1, this->Element );
        if( PointsElement ){
          if(PointsItem.SetElement(PointsElement, 0) == XDMF_FAIL) return(XDMF_FAIL);
          if(PointsItem.UpdateInformation() == XDMF_FAIL) return(XDMF_FAIL);
          if(PointsItem.Update() == XDMF_FAIL) return(XDMF_FAIL);
          TmpArray = PointsItem.GetArray();
          if( TmpArray ){
            TmpArray->GetValues( 0, this->DxDyDz, 2 );
          }
        } else {
          XdmfErrorMessage("No Dx, Dy Specified");
          return( XDMF_FAIL );
        }
      } else {
        XdmfErrorMessage("No Origin Specified");
        return( XDMF_FAIL );
      }
    } else if( this->GeometryType == XDMF_GEOMETRY_VXVYVZ ) {
      XdmfDebug("Reading VectorX, VectorY, VectorZ" );
      PointsElement = this->DOM->FindDataElement(0, this->Element );
      if( PointsElement ){
        XdmfDataItem PointsItem;
        PointsItem.SetDOM(this->DOM);
        if(PointsItem.SetElement(PointsElement, 0) == XDMF_FAIL) return(XDMF_FAIL);
        if(PointsItem.UpdateInformation() == XDMF_FAIL) return(XDMF_FAIL);
        if(PointsItem.Update() == XDMF_FAIL) return(XDMF_FAIL);
        TmpArray = PointsItem.GetArray();
        if(!TmpArray){
          XdmfErrorMessage("Error Reading Points X Vector");
          return(XDMF_FAIL);
        }
        this->VectorX = TmpArray;
        this->VectorXIsMine = 1;
        PointsItem.SetArrayIsMine(0);
      } else {
        XdmfErrorMessage("No VectorX Specified");
        return( XDMF_FAIL );
      }
      PointsElement = this->DOM->FindDataElement(1, this->Element );
      if( PointsElement ){
        XdmfDataItem PointsItem;
        PointsItem.SetDOM(this->DOM);
        if(PointsItem.SetElement(PointsElement, 0) == XDMF_FAIL) return(XDMF_FAIL);
        if(PointsItem.UpdateInformation() == XDMF_FAIL) return(XDMF_FAIL);
        if(PointsItem.Update() == XDMF_FAIL) return(XDMF_FAIL);
        TmpArray = PointsItem.GetArray();
        if(!TmpArray){
          XdmfErrorMessage("Error Reading Points Y Vector");
          return(XDMF_FAIL);
        }
        this->VectorY = TmpArray;
        this->VectorYIsMine = 1;
        PointsItem.SetArrayIsMine(0);
      } else {
        XdmfErrorMessage("No VectorY Specified");
        return( XDMF_FAIL );
      }
      PointsElement = this->DOM->FindDataElement(2, this->Element );
      if( PointsElement ){
        XdmfDataItem PointsItem;
        PointsItem.SetDOM(this->DOM);
        if(PointsItem.SetElement(PointsElement, 0) == XDMF_FAIL) return(XDMF_FAIL);
        if(PointsItem.UpdateInformation() == XDMF_FAIL) return(XDMF_FAIL);
        if(PointsItem.Update() == XDMF_FAIL) return(XDMF_FAIL);
        TmpArray = PointsItem.GetArray();
        if(!TmpArray){
          XdmfErrorMessage("Error Reading Points Z Vector");
          return(XDMF_FAIL);
        }
        this->VectorZ = TmpArray;
        this->VectorZIsMine = 1;
        PointsItem.SetArrayIsMine(0);
      } else {
        XdmfErrorMessage("No VectorZ Specified");
        return( XDMF_FAIL );
      }
    } else if( this->GeometryType == XDMF_GEOMETRY_VXVY) {
      XdmfDebug("Reading VectorX, VectorY" );
      PointsElement = this->DOM->FindDataElement(0, this->Element );
      if( PointsElement ){
        XdmfDataItem PointsItem;
        PointsItem.SetDOM(this->DOM);
        if(PointsItem.SetElement(PointsElement, 0) == XDMF_FAIL) return(XDMF_FAIL);
        if(PointsItem.UpdateInformation() == XDMF_FAIL) return(XDMF_FAIL);
        if(PointsItem.Update() == XDMF_FAIL) return(XDMF_FAIL);
        TmpArray = PointsItem.GetArray();
        if(!TmpArray){
          XdmfErrorMessage("Error Reading Points X Vector");
          return(XDMF_FAIL);
        }
        this->VectorX = TmpArray;
        this->VectorXIsMine = 1;
        PointsItem.SetArrayIsMine(0);
      } else {
        XdmfErrorMessage("No VectorX Specified");
        return( XDMF_FAIL );
      }
      PointsElement = this->DOM->FindDataElement(1, this->Element );
      if( PointsElement ){
        XdmfDataItem PointsItem;
        PointsItem.SetDOM(this->DOM);
        if(PointsItem.SetElement(PointsElement, 0) == XDMF_FAIL) return(XDMF_FAIL);
        if(PointsItem.UpdateInformation() == XDMF_FAIL) return(XDMF_FAIL);
        if(PointsItem.Update() == XDMF_FAIL) return(XDMF_FAIL);
        TmpArray = PointsItem.GetArray();
        if(!TmpArray){
          XdmfErrorMessage("Error Reading Points Y Vector");
          return(XDMF_FAIL);
        }
        this->VectorY = TmpArray;
        this->VectorYIsMine = 1;
        PointsItem.SetArrayIsMine(0);
      } else {
        XdmfErrorMessage("No VectorY Specified");
        return( XDMF_FAIL );
      }
    }
  }
  if( points ){
    this->SetPoints( points );
    this->PointsAreMine = 1;
  }
  return( XDMF_SUCCESS );
}
