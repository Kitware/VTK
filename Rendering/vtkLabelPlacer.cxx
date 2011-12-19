/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLabelPlacer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkLabelPlacer.h"

#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCoordinate.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLabelHierarchy.h"
#include "vtkLabelHierarchyIterator.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSelectVisiblePoints.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTimerLog.h"
#include "vtkUnicodeString.h"
#include "vtkUnicodeStringArray.h"

#include <vector>

vtkStandardNewMacro(vtkLabelPlacer);
vtkCxxSetObjectMacro(vtkLabelPlacer,AnchorTransform,vtkCoordinate);

class vtkLabelPlacer::Internal
{
public:
  /// A single label's coordinates (adjusted so that the lower left screen coords are <0,0>).
  struct LabelRect
    {
    float x[4]; // xmin, xmax, ymin, ymax
    };
  /// A rectangular tile on the screen. It contains a set of labels that overlap it.
  struct ScreenTile
    {
    std::vector<LabelRect> Labels;
    ScreenTile() { }
    /// Is there space to place the given rectangle in this tile so that it doesn't overlap any labels in this tile?
    bool IsSpotOpen( float& opacity, struct LabelRect& r )
      {
      float d0, d1, d2, d3;
      for ( std::vector<LabelRect>::iterator it = this->Labels.begin(); it != this->Labels.end(); ++ it )
        {
        d0 = it->x[0] - r.x[1];
        d1 = r.x[0] - it->x[1];
        d2 = it->x[2] - r.x[3];
        d3 = r.x[2] - it->x[3];
        if ( d0 < 0. && d1 < 0. && d2 < 0. && d3 < 0. )
          return false;
        d0 = d0 < 0. ? 2. : 0.1 * d0;
        d1 = d1 < 0. ? 2. : 0.1 * d1;
        d2 = d2 < 0. ? 2. : 0.1 * d2;
        d3 = d3 < 0. ? 2. : 0.1 * d3;
        d0 = d0 < d1 ? d0 : d1;
        d2 = d2 < d3 ? d2 : d3;
        if ( d0 < 1. && d2 < 1. )
          {
          if ( d0 < opacity ) opacity = d0;
          if ( d2 < opacity ) opacity = d2;
          }
        }
      return true;
      }
    /// Prepare for the next frame.
    void Reset() { this->Labels.clear(); }
    void Insert( const LabelRect& rect )
      {
      this->Labels.push_back( rect );
      }
    };
  std::vector<std::vector<ScreenTile> > Tiles;
  float ScreenOrigin[2];
  float TileSize[2];
  int NumTiles[2];
  vtkSmartPointer<vtkIdTypeArray> NewLabelsPlaced;
  vtkSmartPointer<vtkIdTypeArray> LastLabelsPlaced;
  static int DumpPlaced;

  Internal( float viewport[4], float tilesize[2] )
    {
    this->NewLabelsPlaced = vtkSmartPointer<vtkIdTypeArray>::New();
    this->LastLabelsPlaced = vtkSmartPointer<vtkIdTypeArray>::New();
    this->ScreenOrigin[0] = viewport[0];
    this->ScreenOrigin[1] = viewport[2];
    this->TileSize[0] = tilesize[0];
    this->TileSize[1] = tilesize[1];
    this->NumTiles[0] = static_cast<int>( ceil( ( viewport[1] - viewport[0] ) / tilesize[0] ) );
    this->NumTiles[1] = static_cast<int>( ceil( ( viewport[3] - viewport[2] ) / tilesize[1] ) );
    this->Tiles.resize( this->NumTiles[0] );
    for ( int i = 0; i < this->NumTiles[0]; ++ i )
      this->Tiles[i].resize( this->NumTiles[1] );
    }

  bool PlaceLabel( float& opacity, float x0, float x1, float x2, float x3 )
    {
    // Translate to origin to simplify bucketing
    LabelRect r;
    r.x[0] = x0 - ScreenOrigin[0];
    r.x[1] = x1 - ScreenOrigin[0];
    r.x[2] = x2 - ScreenOrigin[1];
    r.x[3] = x3 - ScreenOrigin[1];
    // Determine intersected tiles
    float rx0 = x0 / TileSize[0];
    float rx1 = x1 / TileSize[0];
    float ry0 = x2 / TileSize[1];
    float ry1 = x3 / TileSize[1];
    int tx0 = static_cast<int>( floor( rx0 ) );
    int tx1 = static_cast<int>( ceil(  rx1 ) );
    int ty0 = static_cast<int>( floor( ry0 ) );
    int ty1 = static_cast<int>( ceil(  ry1 ) );
    if ( tx0 > NumTiles[0] || tx1 < 0 || ty0 > NumTiles[1] || ty1 < 0 )
      return false; // Don't intersect screen.
    if ( tx0 < 0 ) { tx0 = 0; rx0 = 0.; }
    if ( ty0 < 0 ) { ty0 = 0; ry0 = 0.; }
    if ( tx1 >= this->NumTiles[0] ) { tx1 = this->NumTiles[0] - 1; rx1 = tx1; }
    if ( ty1 >= this->NumTiles[1] ) { ty1 = this->NumTiles[1] - 1; ry1 = ty1; }
    // Check all applicable tiles for overlap.
    for ( int tx = tx0; tx <= tx1; ++ tx )
      {
      for ( int ty = ty0; ty <= ty1; ++ ty )
        {
        std::vector<ScreenTile>* trow = &this->Tiles[tx];
        // Do this check here for speed, even though we repeat w/ small mod below.
        if ( ! (*trow)[ty].IsSpotOpen( opacity, r ) ) 
          return false;
        }
      }
    // OK, we made it this far... we can place the label.
    // Add it to each tile it overlaps.
    for ( int tx = tx0; tx <= tx1; ++ tx )
      {
      for ( int ty = ty0; ty <= ty1; ++ ty )
        {
        this->Tiles[tx][ty].Insert( r );
        }
      }
    return true;
    }

  void Reset( float viewport[4], float tileSize[2] )
    {
    // Clear out any tiles we get to reuse
    for ( int tx = 0; tx < this->NumTiles[0]; ++ tx )
      for ( int ty = 0; ty < this->NumTiles[1]; ++ ty )
        this->Tiles[tx][ty].Reset();

    // Set new parameter values in case the viewport changed
    this->ScreenOrigin[0] = viewport[0];
    this->ScreenOrigin[1] = viewport[2];
    this->TileSize[0] = tileSize[0];
    this->TileSize[1] = tileSize[1];
    this->NumTiles[0] = static_cast<int>( ceil( ( viewport[1] - viewport[0] ) / tileSize[0] ) );
    this->NumTiles[1] = static_cast<int>( ceil( ( viewport[3] - viewport[2] ) / tileSize[1] ) );

    // Allocate new tiles (where required...)
    this->Tiles.resize( this->NumTiles[0] );
    for ( int i = 0; i < this->NumTiles[0]; ++ i )
      this->Tiles[i].resize( this->NumTiles[1] );

    // Save labels from the last frame for use later...
    vtkSmartPointer<vtkIdTypeArray> tmp = this->LastLabelsPlaced;
    this->LastLabelsPlaced = this->NewLabelsPlaced;
    this->NewLabelsPlaced = tmp;
    this->NewLabelsPlaced->Reset();
    }
};

int vtkLabelPlacer::Internal::DumpPlaced = 0;

vtkLabelPlacer::vtkLabelPlacer()
{
  this->Renderer = 0;
  this->Gravity = CenterCenter;
  this->AnchorTransform = vtkCoordinate::New();
  this->AnchorTransform->SetCoordinateSystemToWorld();
  this->MaximumLabelFraction = 0.05; // Take up no more than 5% of screen real estate with labels.
  this->Buckets = 0;
  this->PositionsAsNormals = false;
  //this->IteratorType = vtkLabelHierarchy::DEPTH_FIRST;
  //this->IteratorType = vtkLabelHierarchy::FULL_SORT;
  this->IteratorType = vtkLabelHierarchy::QUEUE;
  this->VisiblePoints = vtkSelectVisiblePoints::New();
  this->VisiblePoints->SetTolerance(0.002);
  this->UseUnicodeStrings = false;

  this->LastRendererSize[0] = 0;
  this->LastRendererSize[1] = 0;
  this->LastCameraPosition[0] = 0.0;
  this->LastCameraPosition[1] = 0.0;
  this->LastCameraPosition[2] = 0.0;
  this->LastCameraFocalPoint[0] = 0.0;
  this->LastCameraFocalPoint[1] = 0.0;
  this->LastCameraFocalPoint[2] = 0.0;
  this->LastCameraViewUp[0] = 0.0;
  this->LastCameraViewUp[1] = 0.0;
  this->LastCameraViewUp[2] = 0.0;
  this->LastCameraParallelScale = 0.0;

  this->OutputCoordinateSystem = vtkLabelPlacer::WORLD;

  this->OutputTraversedBounds = false;
  this->GeneratePerturbedLabelSpokes = false;
  this->UseDepthBuffer = false;

  this->SetNumberOfOutputPorts( 4 );
  //this->DebugOn();
}

vtkLabelPlacer::~vtkLabelPlacer()
{
  this->AnchorTransform->Delete();
  if ( this->Buckets )
    {
    delete this->Buckets;
    }
  this->VisiblePoints->Delete();
}

void vtkLabelPlacer::SetRenderer( vtkRenderer* ren )
{
  // Do not keep a reference count to avoid a reference loop
  if ( this->Renderer != ren )
    {
    this->Renderer = ren;
    this->VisiblePoints->SetRenderer(ren);
    this->Modified();
    }
}

void vtkLabelPlacer::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "Renderer: " << this->Renderer << "\n";
  os << indent << "AnchorTransform: " << this->AnchorTransform << "\n";
  os << indent << "Gravity: " << this->Gravity << "\n";
  os << indent << "MaximumLabelFraction: " << this->MaximumLabelFraction << "\n";
  os << indent << "PositionsAsNormals: " << ( this->PositionsAsNormals ? "ON" : "OFF" ) << "\n";
  os << indent << "UseUnicodeStrings: " << ( this->UseUnicodeStrings ? "ON" : "OFF" ) << "\n";
  os << indent << "IteratorType: " << this->IteratorType << "\n";
  os << indent << "OutputTraversedBounds: " << (this->OutputTraversedBounds ? "ON" : "OFF" ) << "\n";
  os << indent << "GeneratePerturbedLabelSpokes: " 
    << (this->GeneratePerturbedLabelSpokes ? "ON" : "OFF" ) << "\n";
  os << indent << "UseDepthBuffer: " 
    << (this->UseDepthBuffer ? "ON" : "OFF" ) << "\n";
  os << indent << "OutputCoordinateSystem: " << this->OutputCoordinateSystem << "\n";
}

/**\brief Set the default label gravity.
  *
  * This method does not allow invalid gravities to be specified.
  * The default value (CenterCenter) is both vertically and horizontally centered.
  * Baseline vertical gravity is not yet supported properly since no text is associated with labels yet.
  */
void vtkLabelPlacer::SetGravity( int gravity )
{
  if ( gravity == this->Gravity )
    return;

  if ( ! ( gravity & HorizontalBitMask ) )
    {
    vtkWarningMacro( "Ignoring gravity " << gravity << " with no horizontal bit set" );
    return;
    }

  if ( ! ( gravity & VerticalBitMask ) )
    {
    vtkWarningMacro( "Ignoring gravity " << gravity << " with no vertical bit set" );
    return;
    }

  this->Gravity = gravity;
  this->Modified();
}

unsigned long vtkLabelPlacer::GetMTime()
{
  // Check for minimal changes
  if ( this->Renderer )
    {
    int* sz = this->Renderer->GetSize();
    if ( this->LastRendererSize[0] != sz[0]
      || this->LastRendererSize[1] != sz[1] )
      {
      this->LastRendererSize[0] = sz[0];
      this->LastRendererSize[1] = sz[1];
      this->Modified();
      }
    vtkCamera* cam = this->Renderer->GetActiveCamera();
    if ( cam )
      {
      double* pos = cam->GetPosition();
      if ( this->LastCameraPosition[0] != pos[0]
        || this->LastCameraPosition[1] != pos[1]
        || this->LastCameraPosition[2] != pos[2] )
        {
        this->LastCameraPosition[0] = pos[0];
        this->LastCameraPosition[1] = pos[1];
        this->LastCameraPosition[2] = pos[2];
        this->Modified();
        }
      double* fp = cam->GetFocalPoint();
      if ( this->LastCameraFocalPoint[0] != fp[0]
        || this->LastCameraFocalPoint[1] != fp[1]
        || this->LastCameraFocalPoint[2] != fp[2] )
        {
        this->LastCameraFocalPoint[0] = fp[0];
        this->LastCameraFocalPoint[1] = fp[1];
        this->LastCameraFocalPoint[2] = fp[2];
        this->Modified();
        }
      double* up = cam->GetViewUp();
      if ( this->LastCameraViewUp[0] != up[0]
        || this->LastCameraViewUp[1] != up[1]
        || this->LastCameraViewUp[2] != up[2] )
        {
        this->LastCameraViewUp[0] = up[0];
        this->LastCameraViewUp[1] = up[1];
        this->LastCameraViewUp[2] = up[2];
        this->Modified();
        }
      double scale = cam->GetParallelScale();
      if( this->LastCameraParallelScale != scale)
        {
        this->LastCameraParallelScale = scale;
        this->Modified();
        }
      }
    }
  return Superclass::GetMTime();
}

int vtkLabelPlacer::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info )
{
  info->Set( vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkLabelHierarchy" );
  return 1;
}

int vtkLabelPlacer::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector )
{
  if ( ! this->Renderer )
    {
    vtkErrorMacro( "No renderer -- can't determine screen space size." );
    return 0;
    }

  if ( ! this->Renderer->GetRenderWindow() )
    {
    vtkErrorMacro( "No render window -- can't get window size to query z buffer." );
    return 0;
    }

  // This will trigger if you do something like ResetCamera before the Renderer or
  // RenderWindow have allocated their appropriate system resources (like creating
  // an OpenGL context)." Resource allocation must occure before we can use the Z
  // buffer.
  if ( this->Renderer->GetRenderWindow()->GetNeverRendered() )
    {
    vtkDebugMacro( "RenderWindow not initialized -- aborting update." );
    return 1;
    }

  vtkCamera* cam = this->Renderer->GetActiveCamera();
  if ( ! cam )
    {
    return 1;
    }

  vtkInformation* inInfo  =  inputVector[0]->GetInformationObject( 0 );
  vtkInformation* outInfo0 = outputVector->GetInformationObject( 0 );
  vtkInformation* outInfo1 = outputVector->GetInformationObject( 1 );
  vtkInformation* outInfo2 = outputVector->GetInformationObject( 2 );
  vtkInformation* outInfo3 = outputVector->GetInformationObject( 3 );

  vtkLabelHierarchy* inData = vtkLabelHierarchy::SafeDownCast(
    inInfo->Get( vtkDataObject::DATA_OBJECT() ) );
  vtkPolyData* ouData0 = vtkPolyData::SafeDownCast(
    outInfo0->Get( vtkDataObject::DATA_OBJECT() ) );
  vtkPolyData* ouData1 = vtkPolyData::SafeDownCast(
    outInfo1->Get( vtkDataObject::DATA_OBJECT() ) );
  vtkPolyData* ouData2 = vtkPolyData::SafeDownCast(
    outInfo2->Get( vtkDataObject::DATA_OBJECT() ) );
  vtkPolyData* ouData3 = vtkPolyData::SafeDownCast(
    outInfo3->Get( vtkDataObject::DATA_OBJECT() ) );

  vtkStringArray* nameArr0 = vtkStringArray::New();
  vtkUnicodeStringArray* nameUArr0 = vtkUnicodeStringArray::New();

  if( this->UseUnicodeStrings )
    {
    nameUArr0->SetName( "LabelText" );
    ouData0->GetPointData()->AddArray( nameUArr0 );
    }
  else
    {
    nameArr0->SetName( "LabelText" );
    ouData0->GetPointData()->AddArray( nameArr0 );
    }  
    nameArr0->Delete();
    nameUArr0->Delete();

  vtkDoubleArray* opArr0 = vtkDoubleArray::New();
  opArr0->SetName( "Opacity" );
  ouData0->GetPointData()->AddArray( opArr0 );
  opArr0->Delete();

  vtkIntArray* iconIndexArr1 = vtkIntArray::New();
  iconIndexArr1->SetName( "IconIndex" );
  ouData1->GetPointData()->AddArray( iconIndexArr1 );
  iconIndexArr1->Delete();

  vtkIntArray* idArr0 = vtkIntArray::New();
  idArr0->SetName( "ID" );
  ouData0->GetPointData()->AddArray( idArr0 );
  idArr0->Delete();

  vtkStringArray* nameArr = vtkStringArray::SafeDownCast( 
    inData->GetLabels());
  vtkUnicodeStringArray* nameUArr = vtkUnicodeStringArray::SafeDownCast( 
    inData->GetLabels());
  vtkIntArray* iconIndexArr = vtkIntArray::SafeDownCast( 
    inData->GetIconIndices());

  if ( ! inData )
    {
    // vtkErrorMacro( "No input data" );
    return 1;
    }

  vtkPoints* ipts = inData->GetPoints();
  if ( ! ipts )
    {
    return 1;
    }
  vtkDataArray* isz = inData->GetPointData()->GetArray( "LabelSize" );
  if ( ! isz ) //|| isz->GetNumberOfComponents() > 2 )
    {
    vtkWarningMacro( "Missing or improper label size point array -- output will be empty." );
    return 1;
    }

  // If the renderer size is zero, silently place no labels.
  int* renSize = this->Renderer->GetSize();
  if ( renSize[0] == 0 || renSize[1] == 0 )
    {
    return 1;
    }

  if ( ! ouData0 || ! ouData1 )
    {
    vtkErrorMacro( "No output data." );
    return 0;
    }

  // Prepare both icon and text output datasets
  vtkPoints* opts0 = ouData0->GetPoints();
  if ( ! opts0 )
    {
    opts0 = vtkPoints::New();
    ouData0->SetPoints( opts0 );
    opts0->FastDelete();
    }
  ouData0->Allocate();

  vtkPoints* opts1 = ouData1->GetPoints();
  if ( ! opts1 )
    {
    opts1 = vtkPoints::New();
    ouData1->SetPoints( opts1 );
    opts1->FastDelete();
    }
  ouData1->Allocate();

  vtkPoints* opts2 = ouData2->GetPoints();
  if ( ! opts2 )
    {
    opts2 = vtkPoints::New();
    ouData2->SetPoints( opts2 );
    opts2->FastDelete();
    }
  ouData2->Allocate();

  vtkPoints* opts3 = ouData3->GetPoints();
  vtkCellArray* ocells3 = ouData3->GetLines();
  if ( ! opts3 )
    {
    opts3 = vtkPoints::New();
    ouData3->SetPoints( opts3 );
    opts3->FastDelete();
    }
  if( ! ocells3 )
    {
    ocells3 = vtkCellArray::New();
    ouData3->SetLines( ocells3 );
    ocells3->FastDelete();
    }
  ouData3->Allocate();

  int tvpsz[4]; // tiled viewport size (and origin)
  // kd-tree bounds on screenspace (as floats... eventually we
  // should use a median kd-tree -- not naive version)
  float kdbounds[4];
  this->Renderer->GetTiledSizeAndOrigin(
    tvpsz, tvpsz + 1, tvpsz + 2, tvpsz + 3 );
  kdbounds[0] = tvpsz[2];
  kdbounds[1] = tvpsz[0] + tvpsz[2];
  kdbounds[2] = tvpsz[3];
  kdbounds[3] = tvpsz[1] + tvpsz[3];
  float tileSize[2] = { 128., 128. }; // fixed for now
  if (
    ! this->Buckets ||
    this->Buckets->NumTiles[0] * this->Buckets->TileSize[0] < tvpsz[2] ||
    this->Buckets->NumTiles[1] * this->Buckets->TileSize[1] < tvpsz[3] )
    {
    this->Buckets = new Internal( kdbounds, tileSize );
    }
  else
    {
    this->Buckets->Reset( kdbounds, tileSize );
    }

  float * zPtr = NULL;
  int placed = 0;
  int occluded = 0;

  double ll[3], lr[3], ul[3], ur[3];
  ll[2] = lr[2] = ul[2] = ur[2] = 0.;
  double x[3];
  double sz[4];
  int* dispx;
  double frustumPlanes[24];
  double aspect = this->Renderer->GetTiledAspectRatio();
  cam->GetFrustumPlanes( aspect, frustumPlanes );
  unsigned long allowableLabelArea = static_cast<unsigned long>
    ( ( ( kdbounds[1] - kdbounds[0] ) * ( kdbounds[3] - kdbounds[2] ) ) * this->MaximumLabelFraction );
  (void)allowableLabelArea;
  unsigned long renderedLabelArea = 0;
  unsigned long iteratedLabelArea = 0;
  double camVec[3];
  if ( this->PositionsAsNormals )
    {
    cam->GetViewPlaneNormal( camVec );
    }

  vtkLabelHierarchyIterator* inIter = inData->NewIterator(
    this->IteratorType, this->Renderer, cam, frustumPlanes, this->PositionsAsNormals, tileSize );

  if (this->OutputTraversedBounds)
    {
    inIter->SetTraversedBounds( ouData2 );
    }
  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  timer->StartTimer();

  inIter->Begin( this->Buckets->LastLabelsPlaced );
  this->Buckets->NewLabelsPlaced->Initialize();

  if(this->UseDepthBuffer)
    {
    zPtr = this->VisiblePoints->Initialize(true);
    }

  timer->StopTimer();
  vtkDebugMacro("Iterator initialization time: " << timer->GetElapsedTime());
  timer->StartTimer();
  for ( ; ! inIter->IsAtEnd(); inIter->Next() )
    {
    // Ignore labels that don't have text or an icon.
    vtkIdType labelType = inIter->GetType();
    int gravity = this->Gravity;
    if ( labelType == 0 )
      {
      gravity = HorizontalLeftBit | VerticalBaselineBit;
      }
    if ( labelType < 0 || labelType > 1 )
      {
      vtkDebugMacro("Arf. Bad label type " << labelType);
      continue;
      }

    inIter->GetPoint( x );
    // Cull points behind the camera. Cannot rely on hither-yon planes because the camera
    // position gets changed during vtkInteractorStyle::Dolly() and RequestData() called from
    // within ResetCameraClippingRange() before the frustum planes are updated.
    // Cull points outside hither-yon planes (other planes get tested below)
    double* eye = cam->GetPosition();
    double* dir = cam->GetViewPlaneNormal();
    if ( ( x[0] - eye[0] ) * dir[0] + ( x[1] - eye[1] ) * dir[1] + ( x[2] - eye[2] ) * dir[2] > 0 )
      {
      continue;
      }

    // Ignore labels pointing the wrong direction (HACK)
    if ( this->PositionsAsNormals )
      {
      if ( camVec[0] * x[0] + camVec[1] * x[1] + camVec[2] * x[2] < 0. )
        {
        continue;
        }
      }

    // Test for occlusion using the z-buffer
    if (this->UseDepthBuffer && !this->VisiblePoints->IsPointOccluded(x, zPtr))
      {
      occluded++;
      continue;
      }

    this->AnchorTransform->SetValue( x );
    dispx = this->AnchorTransform->GetComputedDisplayValue( this->Renderer );
    inIter->GetSize( sz );
    if ( sz[0] < 0 ) sz[0] = -sz[0];
    if ( sz[1] < 0 ) sz[1] = -sz[1];
    // !!!! Commented out a few lines here as sz[2] && sz[3] never are initialized
    // Move anchor so no "space" characters are included in the layout.
    //dispx[0] -= static_cast<int>( sz[2] );
    // By default, the anchor will be at the text baseline. Adjust if user has selected otherwise.
    //if ( ( gravity & VerticalBitMask ) != VerticalBaselineBit )
      //dispx[1] -= static_cast<int>( sz[3] );
    // Without this check things get *really* slow (at least with naive bucket placement tests)
    // FIXME: Don't count area clipped off by viewport when culling above is fixed.
    double t1, t2;
    switch ( gravity & HorizontalBitMask )
      {
    case HorizontalLeftBit:
      t1 = dispx[0] < kdbounds[0] ? kdbounds[0] : dispx[0];
      t2 = dispx[0] + sz[0];
      if ( t2 > kdbounds[1] )
       t2 = kdbounds[1];
      ll[0] = ul[0] = t1;
      lr[0] = ur[0] = t2;
      break;
    case HorizontalRightBit:
      t1 = dispx[0] - sz[0];
      if ( t1 < kdbounds[0] )
       t1 = kdbounds[0];
      t2 = dispx[0] > kdbounds[1] ? kdbounds[1] : dispx[0];
      ll[0] = ul[0] = t1;
      lr[0] = ur[0] = t2;
      break;
    default:
    case HorizontalCenterBit:
      t1 = dispx[0] - sz[0] / 2;
      if ( t1 < kdbounds[0] )
       t1 = kdbounds[0];
      t2 = dispx[0] + sz[0] / 2;
      if ( t2 > kdbounds[1] )
       t2 = kdbounds[1];
      ll[0] = ul[0] = t1;
      lr[0] = ur[0] = t2;
      break;
      }
    if ( ll[0] > kdbounds[1] || lr[0] < kdbounds[0] )
      {
      continue; // cull label not in frame
      }
    
    switch ( gravity & VerticalBitMask )
      {
    case VerticalBottomBit:
    case VerticalBaselineBit:
      t1 = dispx[1] < kdbounds[2] ? kdbounds[2] : dispx[1];
      t2 = dispx[1] + sz[1];
      if ( t2 > kdbounds[3] )
        t2 = kdbounds[3];
      ll[1] = lr[1] = t1;
      ul[1] = ur[1] = t2;
      break;
    case VerticalTopBit:
      t1 = dispx[1] - sz[1];
      if ( t1 < kdbounds[2] )
        t1 = kdbounds[2];
      t2 = dispx[1] > kdbounds[3] ? kdbounds[3] : dispx[1];
      ll[1] = lr[1] = t1;
      ul[1] = ur[1] = t2;
      break;
    default:
    case VerticalCenterBit:
      t1 = dispx[1] - sz[1] / 2;
      if ( t1 < kdbounds[2] )
        t1 = kdbounds[2];
      t2 = dispx[1] + sz[1] / 2;
      if ( t2 > kdbounds[3] )
        t2 = kdbounds[3];
      ll[1] = lr[1] = t1;
      ul[1] = ur[1] = t2;
      break;
      }
    if ( ll[1] > kdbounds[3] || lr[1] < kdbounds[2] )
      {
      continue; // cull label not in frame
      }

    if ( this->Debug )
      {
      vtkDebugMacro("Try: " << inIter->GetLabelId() << " (" << ll[0] << ", " << ll[1] << "  " << ur[0] << "," << ur[1] << ")");
      if ( labelType == 0 )
        {
        if( this->UseUnicodeStrings )
          {
          vtkDebugMacro("Area: " << renderedLabelArea << "  /  " << allowableLabelArea << " \"" << nameUArr->GetValue( inIter->GetLabelId() ).utf8_str() << "\"");
          }
        else
          {
          vtkDebugMacro("Area: " << renderedLabelArea << "  /  " << allowableLabelArea << " \"" << nameArr->GetValue( inIter->GetLabelId() ).c_str() << "\"");
          }
        }
      else
        {
        vtkDebugMacro("Area: " << renderedLabelArea << "  /  " << allowableLabelArea);
        }
      }

    iteratedLabelArea += static_cast<unsigned long>( sz[0] * sz[1] );

    // TODO: Is this necessary?
#if 0
    if ( iteratedLabelArea > 5 * allowableLabelArea )
      {
      vtkDebugMacro("Early exit due to large iterated label area");
      break;
      }
#endif

    float opacity = 1.;
    if ( this->Buckets->PlaceLabel( opacity, ll[0], ur[0], ll[1], ur[1] ) )
      {
      renderedLabelArea += static_cast<unsigned long>( sz[0] * sz[1] );
#if 0 
      if ( renderedLabelArea > allowableLabelArea )
        {
        vtkDebugMacro("Early exit due to large rendered label area");
        break;
        }
#endif // 0
      vtkIdType conn[4];
      OutputCoordinates coordSys = static_cast<OutputCoordinates>( this->OutputCoordinateSystem );
      if ( labelType == 0 )
        { // label is text
        if ( this->Buckets->DumpPlaced )
          {
          if( this->UseUnicodeStrings )
            {
            vtkDebugMacro(<< ll[0] << " -- " << ur[0] << ", " << ll[1] << " -- " << ur[1] << ": " << nameUArr->GetValue( inIter->GetLabelId() ).utf8_str());
            }
          else
            {
            vtkDebugMacro(<< ll[0] << " -- " << ur[0] << ", " << ll[1] << " -- " << ur[1] << ": " << nameArr->GetValue( inIter->GetLabelId() ).c_str());
            }
          }
        switch ( coordSys )
          {
        default:
        case WORLD:
          conn[0] = opts0->InsertNextPoint( x );
          break;
        case DISPLAY:
          conn[0] = opts0->InsertNextPoint( dispx[0], dispx[1], 0. );
          break;
          }
        // Store the anchor point in world coordinates
        ouData0->InsertNextCell( VTK_VERTEX, 1, conn );
        if( this->UseUnicodeStrings )
          {
          nameUArr0->InsertNextValue( nameUArr->GetValue( inIter->GetLabelId() ) );
          }
        else
          {
          nameArr0->InsertNextValue( nameArr->GetValue( inIter->GetLabelId() ) );
          }
        opArr0->InsertNextValue( opacity );
        idArr0->InsertNextValue( 0 );
        }
      else
        { // label is an icon
        if ( this->Buckets->DumpPlaced )
          {
          vtkDebugMacro(<< ll[0] << " -- " << ur[0] << ", " << ll[1] << " -- " << ur[1] << ": Icon " << iconIndexArr->GetValue( inIter->GetLabelId() ));
          }
        switch ( coordSys )
          {
        default:
        case WORLD:
          conn[0] = opts1->InsertNextPoint( x );
          break;
        case DISPLAY:
          conn[0] = opts1->InsertNextPoint( dispx[0], dispx[1], 0. );
          break;
          }
        vtkIdType cid = ouData1->InsertNextCell( VTK_VERTEX, 1, conn ); (void)cid;
        vtkDebugMacro("     Point: " << conn[0] << " (" << x[0] << "," << x[1] << "," << x[2] << ") Vertex: " << cid);
        iconIndexArr1->InsertNextValue( iconIndexArr->GetValue( inIter->GetLabelId() ) );
        }

      // Handle Spokes for perturbed points
      if(this->GeneratePerturbedLabelSpokes)
        {
        //inData->CenterPts
        //inData->
        }


      // Uncomment to actually store the previous labels.
      // Currently starting with a clean slate each time.
      this->Buckets->NewLabelsPlaced->InsertNextValue( inIter->GetLabelId() );
      vtkDebugMacro("Placed: " << inIter->GetLabelId() << " (" << ll[0] << ", " << ll[1] << "  " << ur[0] << "," << ur[1] << ") " << labelType);
      placed++;
      }
    }
  vtkDebugMacro("------");
  //cout << "Not Placed: " << notPlaced << endl;
  //cout << "Labels Occluded: " << occluded << endl;

  inIter->Delete();
  if (zPtr)
    {
    delete [] zPtr;
    }

  timer->StopTimer();
  vtkDebugMacro("Iteration time: " << timer->GetElapsedTime());

  return 1;
}

