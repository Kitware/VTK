/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPNGReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGDALVectorReader.h"

// VTK includes
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDoubleArray.h"
#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkPolyData.h"

// GDAL OGR includes
#include <ogrsf_frmts.h>

// C++ includes
#include <vector> // Requires STL vector

vtkStandardNewMacro(vtkGDALVectorReader);

int vtkGDALVectorReader::OGRRegistered = 0;

// -----------------------------------------------------------------------------
class vtkGDALVectorReader::Internal
{
public:
  Internal( const char* srcName, int srcMode, int appendFeatures, int addFeatIds )
    {
    this->Source = OGRSFDriverRegistrar::Open( srcName, srcMode, &this->Driver );
    if ( ! this->Source )
      {
      this->LastError = CPLGetLastErrorMsg();
      }
    else
      {
      this->LastError = 0;
      }
    this->LayerIdx = 0;
    this->AppendFeatures = appendFeatures;
    this->AddFeatureIds = addFeatIds;
    }
  ~Internal()
    {
    if ( this->Source )
      {
      OGRDataSource::DestroyDataSource( this->Source );
      }
    }

  void SetupPolyData( vtkPolyData **pd, vtkCellArray **lines,
    vtkCellArray **verts, std::vector<vtkAbstractArray*> *fields,
    int numFields, OGRFeatureDefn* fdef )
    {
    *pd = vtkPolyData::New();

    vtkAbstractArray* arr;
    int f;
    for ( f = 0; f < numFields; ++f )
      {
      OGRFieldDefn* ffdef = fdef->GetFieldDefn( f );
      switch ( ffdef->GetType() )
        {
      case OFTInteger:
        arr = vtkIntArray::New();
        break;
      case OFTReal:
        arr = vtkDoubleArray::New();
        break;
      case OFTString:
      default: // When in doubt, it's a string!
        arr = vtkStringArray::New();
        break;
        }
      arr->SetName( ffdef->GetNameRef() );
      fields->push_back( arr );
      (*pd)->GetCellData()->AddArray( arr );
      arr->FastDelete();
      }
    if (this->AddFeatureIds)
      {
      vtkNew<vtkIdTypeArray> featIds;
      featIds->SetName("_vtkPedigreeIds");
      (*pd)->GetCellData()->SetPedigreeIds(featIds.GetPointer());
      fields->push_back(featIds.GetPointer());
      }

    *lines = vtkCellArray::New();
    *verts  = vtkCellArray::New();

    (*pd)->SetLines(*lines);
    (*pd)->SetVerts(*verts);

    (*lines)->FastDelete();
    (*verts)->FastDelete();
    }

  bool ReadLayer( OGRLayer* layer, vtkMultiBlockDataSet* mbds )
    {
    OGRFeature* feat;
    vtkPolyData *pd = 0;
    vtkIdType nTotPoly = 0;
    vtkCellArray *lines = 0, *verts = 0;

    OGRFeatureDefn* fdef = layer->GetLayerDefn();
    int numFields = fdef->GetFieldCount();
    std::vector<vtkAbstractArray*> fields;

    if ( this->AppendFeatures )
      {
      this->SetupPolyData(&pd, &lines, &verts, &fields, numFields, fdef);
      }

    while ( ( feat = layer->GetNextFeature() ) )
      {
      if ( !this->AppendFeatures )
        {
        fields.clear();
        this->SetupPolyData(&pd, &lines, &verts, &fields, numFields, fdef);
        mbds->SetBlock( this->LayerIdx, pd );
        ++this->LayerIdx;
        pd->FastDelete();
        nTotPoly = 0;
        }

      vtkPoints* pts = pd->GetPoints();
      if ( ! pts )
        {
        pts = vtkPoints::New();
        pts->SetDataTypeToDouble();
        pd->SetPoints( pts );
        pts->FastDelete();
        }

      // Insert points and lines to represent the geometry of each feature.
      OGRGeometry* geom = feat->GetGeometryRef();
      vtkIdType nPoly = this->insertGeometryRecursive( geom, pd, pts, lines, verts );
      if ( ! nPoly )
        {
        continue;
        }

      nTotPoly += nPoly;

      // Now insert the field values for this geometry once for each cell created
      // (We have to copy the values when there are multiple polygons or polygons
      //  with inner rings.)
      vtkIdType i;
      int ival;
      double rval;
      const char* sval;
      vtkIntArray* iarr;
      vtkDoubleArray* rarr;
      vtkStringArray* sarr;
      for ( int f = 0; f < numFields; ++f )
        {
        OGRFieldDefn* ffdef = fdef->GetFieldDefn( f );
        switch ( ffdef->GetType() )
          {
        case OFTInteger:
          iarr = vtkIntArray::SafeDownCast( fields[f] );
          ival = feat->GetFieldAsInteger( f );
          for ( i = 0; i < nPoly; ++i )
            {
            iarr->InsertNextValue( ival );
            }
          break;
        case OFTReal:
          rarr = vtkDoubleArray::SafeDownCast( fields[f] );
          rval = feat->GetFieldAsDouble( f );
          for ( i = 0; i < nPoly; ++i )
            {
            rarr->InsertNextValue( rval );
            }
          break;
        case OFTString:
        default:
          sarr = vtkStringArray::SafeDownCast( fields[f] );
          sval = feat->GetFieldAsString( f );
          for ( i = 0; i < nPoly; ++i )
            {
            sarr->InsertNextValue( sval );
            }
          }
        }
      if (this->AddFeatureIds)
        {
        vtkIdTypeArray* idarr = vtkIdTypeArray::SafeDownCast(fields[numFields]);
        for ( i = 0; i < nPoly; ++i )
          {
          idarr->InsertNextValue(feat->GetFID());
          }
        }
      OGRFeature::DestroyFeature(feat);
      }

    if ( this->AppendFeatures )
      {
      mbds->SetBlock( this->LayerIdx, pd );
      ++this->LayerIdx;
      pd->FastDelete();
      }

    return nTotPoly ? true : false;
    }

  vtkIdType insertGeometryRecursive( OGRGeometry* geom, vtkPolyData* pd,
    vtkPoints* pts, vtkCellArray* lines, vtkCellArray* verts )
    {
    if ( ! geom )
      {
      return 0;
      }

    std::vector<vtkIdType> ptIds;
    OGRPoint* gpt;
    OGRLineString* gls;
    OGRLinearRing* grng;
    OGRPolygon* gpl;
    OGRGeometryCollection* gcol;
    //OGRMultiPolygon* gmpl;
    //OGRMultiLineString* gmls;
    //OGRMultiPoint* gmpt;
    int num;
    vtkIdType i;
    vtkIdType nCells = 0;

    switch ( geom->getGeometryType() )
      {
    case wkbUnknown:
      return 0;

    case wkbPoint:
    case wkbPoint25D:
      gpt = (OGRPoint*) geom;
      ptIds.push_back( pts->InsertNextPoint( gpt->getX(), gpt->getY(), gpt->getZ() ) );
      verts->InsertNextCell(1, &(ptIds[0]));
      ++nCells;
      break;

    case wkbLinearRing: // OGR docs imply that wkbLinearRing may not inherit wkbLineString in the future.
    case wkbLineString:
    case wkbLineString25D:
      gls = (OGRLineString*) geom;
      for ( int p = 0; p < gls->getNumPoints(); ++p )
        {
        // insert ring points
        ptIds.push_back( pts->InsertNextPoint(
                           gls->getX( p ), gls->getY( p ), gls->getZ( p ) ) );
        }
      // insert ring line segments
      lines->InsertNextCell( (int) ptIds.size(), &(ptIds[0]) );
      ++nCells;
      break;

    case wkbPolygon:
    case wkbPolygon25D:
      gpl = (OGRPolygon*) geom;
      grng = gpl->getExteriorRing();
      nCells += this->insertGeometryRecursive( grng, pd, pts, lines, verts );
      num = gpl->getNumInteriorRings();
      for ( i = 0 ; i < num; ++i )
        {
        grng = gpl->getInteriorRing( i );
        nCells += this->insertGeometryRecursive( grng, pd, pts, lines, verts );
        }
      break;

    case wkbMultiPoint:
    case wkbMultiPoint25D:
    case wkbMultiLineString:
    case wkbMultiLineString25D:
    case wkbMultiPolygon:
    case wkbMultiPolygon25D:
    case wkbGeometryCollection:
    case wkbGeometryCollection25D:
      gcol = (OGRGeometryCollection*) geom;
      num = gcol->getNumGeometries();
      for ( i = 0 ; i < num; ++i )
        {
        nCells += this->insertGeometryRecursive(
                    gcol->getGeometryRef( i ), pd, pts, lines, verts );
        }
      break;

    case wkbNone:
      return 0;
      }

    return nCells;
    }

  OGRDataSource* Source;
  OGRSFDriver* Driver;
  const char* LastError;
  int LayerIdx;
  int AppendFeatures;
  int AddFeatureIds;
};

// -----------------------------------------------------------------------------
vtkGDALVectorReader::vtkGDALVectorReader()
{
  this->FileName = 0;
  this->Implementation = 0;
  this->ActiveLayer = -1;

  this->SetNumberOfInputPorts( 0 );

  if ( ! vtkGDALVectorReader::OGRRegistered )
    {
    vtkGDALVectorReader::OGRRegistered = 1;
    OGRRegisterAll();
    }

  this->AppendFeatures = 0;
  this->AddFeatureIds = 0;
}

// -----------------------------------------------------------------------------
vtkGDALVectorReader::~vtkGDALVectorReader()
{
  this->SetFileName( 0 );
  delete this->Implementation;
}

// -----------------------------------------------------------------------------
void vtkGDALVectorReader::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "FileName: " << ( this->FileName ? this->FileName : "(null)" ) << "\n";
  os << indent << "Implementation: " << this->Implementation << "\n";
  os << indent << "AppendFeatures: " << (this->AppendFeatures ? "ON" : "OFF") << "\n";
  os << indent << "AddFeatureIds: " << (this->AddFeatureIds ? "ON" : "OFF") << "\n";
}

// -----------------------------------------------------------------------------
int vtkGDALVectorReader::GetNumberOfLayers()
{
  if ( this->InitializeInternal() == VTK_ERROR )
    {
    return -1;
    }

  return this->Implementation->Source->GetLayerCount();
}

// -----------------------------------------------------------------------------
int vtkGDALVectorReader::GetLayerType(int layerIndex)
{
  if ( this->InitializeInternal() == VTK_ERROR )
    {
    return -1;
    }

  OGRLayer* layer = this->Implementation->Source->GetLayer( layerIndex );

  if ( !layer )
    {
    return -1;
    }

  switch ( layer->GetGeomType() )
    {
    case wkbUnknown:
      return VTK_EMPTY_CELL;
    case wkbPoint:
    case wkbPoint25D:
      return VTK_VERTEX;
    case wkbLinearRing: // OGR docs imply that wkbLinearRing may not inherit wkbLineString in the future.
    case wkbLineString:
    case wkbLineString25D:
      return VTK_LINE;
    case wkbPolygon:
    case wkbPolygon25D:
      return VTK_POLYGON;
    case wkbMultiPoint:
    case wkbMultiPoint25D:
      return VTK_POLY_VERTEX;
    case wkbMultiLineString:
    case wkbMultiLineString25D:
      return VTK_POLY_LINE;
    case wkbMultiPolygon:
    case wkbMultiPolygon25D:
      return VTK_POLYGON;
    case wkbGeometryCollection:
    case wkbGeometryCollection25D:
      return VTK_NUMBER_OF_CELL_TYPES;
    case wkbNone:
      return -1;
    default:
      return -1;
    }
}

// -----------------------------------------------------------------------------
int vtkGDALVectorReader::GetFeatureCount(int layerIndex)
 {
  if ( this->InitializeInternal() == VTK_ERROR )
    {
    return -1;
    }

  OGRLayer* layer = this->Implementation->Source->GetLayer( layerIndex );

  if ( !layer )
    {
    return -1;
    }

  return layer->GetFeatureCount();
}

// -----------------------------------------------------------------------------
int vtkGDALVectorReader::GetActiveLayerType()
{
  return
    this->ActiveLayer < 0 || this->ActiveLayer >= this->GetNumberOfLayers() ?
    -1 : this->GetLayerType(this->ActiveLayer);
}

// -----------------------------------------------------------------------------
int vtkGDALVectorReader::GetActiveLayerFeatureCount()
{
  return
    this->ActiveLayer < 0 || this->ActiveLayer >= this->GetNumberOfLayers() ?
    0 : this->GetFeatureCount(this->ActiveLayer);
}

// -----------------------------------------------------------------------------
const char* vtkGDALVectorReader::GetLayerProjection(int layerIndex)
{
  if ( layerIndex < 0 )
    {
    vtkErrorMacro( << "Layer index cannot be negative");
    }

  std::map<int, std::string>::const_iterator itr =
    this->LayersProjection.find (layerIndex );
  if ( itr != this->LayersProjection.end() )
    {
    return itr->second.c_str();
    }
  else
    {
    return NULL;
    }
}

// -----------------------------------------------------------------------------
std::map<int, std::string> vtkGDALVectorReader::GetLayersProjection()
{
  return this->LayersProjection;
}

// -----------------------------------------------------------------------------
int vtkGDALVectorReader::RequestInformation( vtkInformation* request,
  vtkInformationVector** inputVector, vtkInformationVector* outputVector )
{
  (void)request;
  (void)inputVector;
  (void)outputVector;

  return 1;
}

// -----------------------------------------------------------------------------
int vtkGDALVectorReader::RequestData( vtkInformation* request,
  vtkInformationVector** inputVector, vtkInformationVector* outputVector )
{
  (void)request;
  (void)inputVector;

  if ( ! this->FileName )
    {
    //vtkWarningMacro( "No filename specified" );
    return 0;
    }

  vtkMultiBlockDataSet* mbds = 0;
  vtkInformation* oi = outputVector->GetInformationObject( 0 );
  if ( ! oi )
    {
    return 0;
    }

  mbds = vtkMultiBlockDataSet::SafeDownCast( oi->Get( vtkDataObject::DATA_OBJECT() ) );
  if ( ! mbds )
    {
    return 0;
    }

  // Deleting this->Implementation is required in order to force re-reading each
  // time RequestData() is executed.
  delete this->Implementation;
  this->Implementation = 0;

  if (this->InitializeInternal() == VTK_ERROR)
    {
    return 1;
    }

  vtkGDALVectorReader::Internal* p = this->Implementation;

  int lastLayer = p->Source->GetLayerCount() - 1;
  int startLayer =
    this->ActiveLayer < 0 || this->ActiveLayer >= lastLayer ?
    0 : this->ActiveLayer;
  int endLayer = this->ActiveLayer < 0 || this->ActiveLayer >= lastLayer ?
    lastLayer : this->ActiveLayer;
  for ( int layerIdx = startLayer; layerIdx <= endLayer; ++layerIdx )
    {
    OGRLayer* layer = p->Source->GetLayer( layerIdx );
    if ( ! layer )
      {
      continue;
      }

    if(layer->GetSpatialRef())
      {
      char *projStr;
      layer->GetSpatialRef()->exportToWkt(&projStr);
      this->LayersProjection[layerIdx] = std::string(projStr);
      OGRFree(projStr);
      }

    p->ReadLayer( layer, mbds );
    }

  return 1;
}

// -----------------------------------------------------------------------------
int vtkGDALVectorReader::InitializeInternal()
{
  if ( !this->FileName )
    {
    vtkErrorMacro(<< "FileName not set or empty:");
    return VTK_ERROR;
    }

  if ( !this->Implementation )
    {
    this->Implementation = new vtkGDALVectorReader::Internal(
                             this->FileName, 0 ,
                             this->AppendFeatures, this->AddFeatureIds );
    if ( ! this->Implementation || this->Implementation->LastError )
      {
      if ( this->Implementation )
        {
        vtkErrorMacro( << this->Implementation->LastError );
        delete this->Implementation;
        this->Implementation = 0;
        }
      return VTK_ERROR;
      }
    }

  return VTK_OK;
}
