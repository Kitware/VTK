/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOrientedGlyphFocalPlaneContourRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOrientedGlyphFocalPlaneContourRepresentation.h"
#include "vtkCleanPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkActor2D.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkObjectFactory.h"
#include "vtkProperty2D.h"
#include "vtkAssemblyPath.h"
#include "vtkMath.h"
#include "vtkInteractorObserver.h"
#include "vtkLine.h"
#include "vtkCoordinate.h"
#include "vtkGlyph2D.h"
#include "vtkCursor2D.h"
#include "vtkCylinderSource.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkTransform.h"
#include "vtkCamera.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkFocalPlanePointPlacer.h"
#include "vtkBezierContourLineInterpolator.h"

vtkStandardNewMacro(vtkOrientedGlyphFocalPlaneContourRepresentation);

//----------------------------------------------------------------------
vtkOrientedGlyphFocalPlaneContourRepresentation::vtkOrientedGlyphFocalPlaneContourRepresentation()
{

  // Initialize state
  this->InteractionState = vtkContourRepresentation::Outside;

  this->CursorShape = NULL;
  this->ActiveCursorShape = NULL;

  this->HandleSize = 0.01;
  
  this->LineInterpolator = vtkBezierContourLineInterpolator::New();
  
  // Represent the position of the cursor
  this->FocalPoint = vtkPoints::New();
  this->FocalPoint->SetNumberOfPoints(100);
  this->FocalPoint->SetNumberOfPoints(1);
  this->FocalPoint->SetPoint(0, 0.0,0.0,0.0);
  
  vtkDoubleArray *normals = vtkDoubleArray::New();
  normals->SetNumberOfComponents(3);
  normals->SetNumberOfTuples(100);
  normals->SetNumberOfTuples(1);
  double n[3] = {0,0,0};
  normals->SetTuple(0,n);
  
  // Represent the position of the cursor
  this->ActiveFocalPoint = vtkPoints::New();
  this->ActiveFocalPoint->SetNumberOfPoints(100);
  this->ActiveFocalPoint->SetNumberOfPoints(1);
  this->ActiveFocalPoint->SetPoint(0, 0.0,0.0,0.0);
  
  vtkDoubleArray *activeNormals = vtkDoubleArray::New();
  activeNormals->SetNumberOfComponents(3);
  activeNormals->SetNumberOfTuples(100);
  activeNormals->SetNumberOfTuples(1);
  activeNormals->SetTuple(0,n);
  
  this->FocalData = vtkPolyData::New();
  this->FocalData->SetPoints(this->FocalPoint);
  this->FocalData->GetPointData()->SetNormals(normals);  
  normals->Delete();

  this->ActiveFocalData = vtkPolyData::New();
  this->ActiveFocalData->SetPoints(this->ActiveFocalPoint);
  this->ActiveFocalData->GetPointData()->SetNormals(activeNormals);  
  activeNormals->Delete();
  
  this->Glypher = vtkGlyph2D::New();
  this->Glypher->SetInput(this->FocalData);
  this->Glypher->SetVectorModeToUseNormal();
  this->Glypher->OrientOn();
  this->Glypher->ScalingOn();
  this->Glypher->SetScaleModeToDataScalingOff();
  this->Glypher->SetScaleFactor(1.0);

  this->ActiveGlypher = vtkGlyph2D::New();
  this->ActiveGlypher->SetInput(this->ActiveFocalData);
  this->ActiveGlypher->SetVectorModeToUseNormal();
  this->ActiveGlypher->OrientOn();
  this->ActiveGlypher->ScalingOn();
  this->ActiveGlypher->SetScaleModeToDataScalingOff();
  this->ActiveGlypher->SetScaleFactor(1.0);

  // The transformation of the cursor will be done via vtkGlyph3D
  // By default a vtkCursor2D will be used to define the cursor shape
  vtkCursor2D *cursor2D = vtkCursor2D::New();
  cursor2D->AllOff();
  cursor2D->PointOn();
  cursor2D->Update();
  this->SetCursorShape( cursor2D->GetOutput() );
  cursor2D->Delete();

  vtkCylinderSource *cylinder = vtkCylinderSource::New();
  cylinder->SetResolution(64);
  cylinder->SetRadius(0.5);
  cylinder->SetHeight(0.0);
  cylinder->CappingOff();
  cylinder->SetCenter(0,0,0);

  vtkCleanPolyData* clean = vtkCleanPolyData::New();
  clean->PointMergingOn();
  clean->CreateDefaultLocator();
  clean->SetInputConnection(0,cylinder->GetOutputPort(0));

  vtkTransform *t = vtkTransform::New();
  t->RotateZ(90.0);

  vtkTransformPolyDataFilter *tpd = vtkTransformPolyDataFilter::New();
  tpd->SetInputConnection( 0, clean->GetOutputPort(0) );
  tpd->SetTransform( t );
  clean->Delete();
  cylinder->Delete();
  
  tpd->Update();
  this->SetActiveCursorShape(tpd->GetOutput());
  tpd->Delete();
  t->Delete();
  
  this->Glypher->SetSource(this->CursorShape);
  this->ActiveGlypher->SetSource(this->ActiveCursorShape);

  this->Mapper = vtkPolyDataMapper2D::New();
  this->Mapper->SetInput(this->Glypher->GetOutput());
  //this->Mapper->SetResolveCoincidentTopologyToPolygonOffset();
  this->Mapper->ScalarVisibilityOff();
  //this->Mapper->ImmediateModeRenderingOn();

  this->ActiveMapper = vtkPolyDataMapper2D::New();
  this->ActiveMapper->SetInput(this->ActiveGlypher->GetOutput());
  //this->ActiveMapper->SetResolveCoincidentTopologyToPolygonOffset();
  this->ActiveMapper->ScalarVisibilityOff();
  //this->ActiveMapper->ImmediateModeRenderingOn();

  // Set up the initial properties
  this->CreateDefaultProperties();

  this->Actor = vtkActor2D::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetProperty(this->Property);

  this->ActiveActor = vtkActor2D::New();
  this->ActiveActor->SetMapper(this->ActiveMapper);
  this->ActiveActor->SetProperty(this->ActiveProperty);

  this->Lines = vtkPolyData::New();
  this->LinesMapper = vtkPolyDataMapper2D::New();
  this->LinesMapper->SetInput( this->Lines );
  
  this->LinesActor = vtkActor2D::New();
  this->LinesActor->SetMapper( this->LinesMapper );
  this->LinesActor->SetProperty( this->LinesProperty );
  
  this->InteractionOffset[0] = 0.0;
  this->InteractionOffset[1] = 0.0;

  this->LinesWorldCoordinates        = vtkPolyData::New();
  this->ContourPlaneDirectionCosines = vtkMatrix4x4::New();
}

//----------------------------------------------------------------------
vtkOrientedGlyphFocalPlaneContourRepresentation::~vtkOrientedGlyphFocalPlaneContourRepresentation()
{
  this->FocalPoint->Delete();
  this->FocalData->Delete();

  this->ActiveFocalPoint->Delete();
  this->ActiveFocalData->Delete();
  
  this->SetCursorShape( NULL );
  this->SetActiveCursorShape( NULL );

  this->Glypher->Delete();
  this->Mapper->Delete();
  this->Actor->Delete();

  this->ActiveGlypher->Delete();
  this->ActiveMapper->Delete();
  this->ActiveActor->Delete();
  
  this->Lines->Delete();
  this->LinesMapper->Delete();
  this->LinesActor->Delete();
  
  this->Property->Delete();
  this->ActiveProperty->Delete();
  this->LinesProperty->Delete();

  this->LinesWorldCoordinates->Delete();
  this->ContourPlaneDirectionCosines->Delete();
}

//----------------------------------------------------------------------
void vtkOrientedGlyphFocalPlaneContourRepresentation::SetCursorShape(vtkPolyData *shape)
{
  if ( shape != this->CursorShape )
    {
    if ( this->CursorShape )
      {
      this->CursorShape->Delete();
      }
    this->CursorShape = shape;
    if ( this->CursorShape )
      {
      this->CursorShape->Register(this);
      }
    if ( this->CursorShape )
      {
      this->Glypher->SetSource(this->CursorShape);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------
vtkPolyData *vtkOrientedGlyphFocalPlaneContourRepresentation::GetCursorShape()
{
  return this->CursorShape;
}

//----------------------------------------------------------------------
void vtkOrientedGlyphFocalPlaneContourRepresentation::SetActiveCursorShape(vtkPolyData *shape)
{
  if ( shape != this->ActiveCursorShape )
    {
    if ( this->ActiveCursorShape )
      {
      this->ActiveCursorShape->Delete();
      }
    this->ActiveCursorShape = shape;
    if ( this->ActiveCursorShape )
      {
      this->ActiveCursorShape->Register(this);
      }
    if ( this->ActiveCursorShape )
      {
      this->ActiveGlypher->SetSource(this->ActiveCursorShape);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------
vtkPolyData *vtkOrientedGlyphFocalPlaneContourRepresentation::GetActiveCursorShape()
{
  return this->ActiveCursorShape;
}

//----------------------------------------------------------------------
void vtkOrientedGlyphFocalPlaneContourRepresentation::SetRenderer(vtkRenderer *ren)
{
  //  this->WorldPosition->SetViewport(ren);
  this->Superclass::SetRenderer(ren);
}

//-------------------------------------------------------------------------
int vtkOrientedGlyphFocalPlaneContourRepresentation::ComputeInteractionState(int X, int Y, int vtkNotUsed(modified))
{
  
  double pos[4], xyz[3];
  this->FocalPoint->GetPoint(0,pos);
  pos[3] = 1.0;
  this->Renderer->SetWorldPoint(pos);
  this->Renderer->WorldToDisplay();
  this->Renderer->GetDisplayPoint(pos);
  
  xyz[0] = static_cast<double>(X);
  xyz[1] = static_cast<double>(Y);
  xyz[2] = pos[2];
  
  this->VisibilityOn();
  double tol2 = this->PixelTolerance * this->PixelTolerance;
  if ( vtkMath::Distance2BetweenPoints(xyz,pos) <= tol2 )
    {
    this->InteractionState = vtkContourRepresentation::Nearby;
    if ( !this->ActiveCursorShape )
      {
      this->VisibilityOff();
      }
    }
  else
    {
    this->InteractionState = vtkContourRepresentation::Outside;
    if ( !this->CursorShape )
      {
      this->VisibilityOff();
      }
    }

  return this->InteractionState;
}

//----------------------------------------------------------------------
// Record the current event position, and the rectilinear wipe position.
void vtkOrientedGlyphFocalPlaneContourRepresentation::StartWidgetInteraction(double startEventPos[2])
{
  this->StartEventPosition[0] = startEventPos[0];
  this->StartEventPosition[1] = startEventPos[1];
  this->StartEventPosition[2] = 0.0;

  this->LastEventPosition[0] = startEventPos[0];
  this->LastEventPosition[1] = startEventPos[1];
  
  // How far is this in pixels from the position of this widget?
  // Maintain this during interaction such as translating (don't
  // force center of widget to snap to mouse position)
  
  // convert position to display coordinates
  double pos[2];
  this->GetNthNodeDisplayPosition(this->ActiveNode, pos);

  this->InteractionOffset[0] = pos[0] - startEventPos[0];
  this->InteractionOffset[1] = pos[1] - startEventPos[1];
  
}


//----------------------------------------------------------------------
// Based on the displacement vector (computed in display coordinates) and
// the cursor state (which corresponds to which part of the widget has been
// selected), the widget points are modified.
// First construct a local coordinate system based on the display coordinates
// of the widget.
void vtkOrientedGlyphFocalPlaneContourRepresentation::WidgetInteraction(double eventPos[2])
{
  // Process the motion
  if ( this->CurrentOperation == vtkContourRepresentation::Translate )
    {
    this->Translate(eventPos);
    }
  if ( this->CurrentOperation == vtkContourRepresentation::Shift )
    {
    this->ShiftContour(eventPos);
    }
  if ( this->CurrentOperation == vtkContourRepresentation::Scale )
    {
    this->ScaleContour(eventPos);
    }

  // Book keeping
  this->LastEventPosition[0] = eventPos[0];
  this->LastEventPosition[1] = eventPos[1];
}

//----------------------------------------------------------------------
// Translate everything
void vtkOrientedGlyphFocalPlaneContourRepresentation::Translate(double eventPos[2])
{
  double ref[3];
  
  if ( !this->GetActiveNodeWorldPosition( ref ) )
    {
    return;
    }
  
  double displayPos[2];
  displayPos[0] = eventPos[0] + this->InteractionOffset[0];
  displayPos[1] = eventPos[1] + this->InteractionOffset[1];
  
  // vtkFocalPlaneContourRepresentation won't have SetActiveNodeToWorldPosition
  // it will have SetActiveNodeToDisplayPosition.. add the display pos
  // for the 3D below

  double worldPos[3];
  double worldOrient[9];
  if ( this->PointPlacer->ComputeWorldPosition(this->Renderer,
                                               displayPos, ref, worldPos,
                                               worldOrient ) )
    {
    this->SetActiveNodeToWorldPosition(worldPos, worldOrient);
    }
  else
    {
    // I really want to track the closest point here,
    // but I am postponing this at the moment....
    }
}

//----------------------------------------------------------------------
void vtkOrientedGlyphFocalPlaneContourRepresentation::ShiftContour(double
eventPos[2])
{
  double ref[3];

  if ( !this->GetActiveNodeWorldPosition( ref ) )
    {
    return;
    }

  double displayPos[2];
  displayPos[0] = eventPos[0] + this->InteractionOffset[0];
  displayPos[1] = eventPos[1] + this->InteractionOffset[1];

  double worldPos[3];
  double worldOrient[9] = {1.0,0.0,0.0,
                           0.0,1.0,0.0,
                           0.0,0.0,1.0};
  if ( this->PointPlacer->ComputeWorldPosition(this->Renderer,
                                               displayPos, ref, worldPos,
                                               worldOrient ) )
    {

    this->SetActiveNodeToWorldPosition(worldPos, worldOrient);

    double vector[3];
    vector[0] = worldPos[0] - ref[0];
    vector[1] = worldPos[1] - ref[1];
    vector[2] = worldPos[2] - ref[2];

    for ( int i = 0; i < this->GetNumberOfNodes(); i++ )
      {
      if( i != this->ActiveNode )
        {
        this->GetNthNodeWorldPosition( i, ref );
        worldPos[0] = ref[0] + vector[0];
        worldPos[1] = ref[1] + vector[1];
        worldPos[2] = ref[2] + vector[2];
        this->SetNthNodeWorldPosition( i, worldPos, worldOrient );
        }
      }
    }
}
//----------------------------------------------------------------------
void vtkOrientedGlyphFocalPlaneContourRepresentation::ScaleContour(double
eventPos[2])
{
  double ref[3];

  if ( !this->GetActiveNodeWorldPosition( ref ) )
    {
    return;
    }

  double centroid[3];
  ComputeCentroid( centroid );

  double r2 = vtkMath::Distance2BetweenPoints( ref, centroid );

  double displayPos[2];
  displayPos[0] = eventPos[0] + this->InteractionOffset[0];
  displayPos[1] = eventPos[1] + this->InteractionOffset[1];

  double worldPos[3];
  double worldOrient[9] = {1.0,0.0,0.0,
                           0.0,1.0,0.0,
                           0.0,0.0,1.0};
  if ( this->PointPlacer->ComputeWorldPosition(this->Renderer,
                                               displayPos, ref, worldPos,
                                               worldOrient ) )
    {
    double d2 = vtkMath::Distance2BetweenPoints( worldPos, centroid );
    if( d2 != 0. )
      {
      double ratio = sqrt( d2 / r2 );
//       this->SetActiveNodeToWorldPosition(worldPos, worldOrient);

      for ( int i = 0; i < this->GetNumberOfNodes(); i++ )
        {
//         if( i != this->ActiveNode )
          {
          this->GetNthNodeWorldPosition( i, ref );
          worldPos[0] = centroid[0] + ratio * ( ref[0] - centroid[0] );
          worldPos[1] = centroid[0] + ratio * ( ref[1] - centroid[1] );
          worldPos[2] = centroid[0] + ratio * ( ref[2] - centroid[2] );
          this->SetNthNodeWorldPosition( i, worldPos, worldOrient );
          }
        }
      }
    }
}

//----------------------------------------------------------------------
void vtkOrientedGlyphFocalPlaneContourRepresentation::ComputeCentroid(
  double* ioCentroid )
{
  double p[3];
  ioCentroid[0] = 0.;
  ioCentroid[1] = 0.;
  ioCentroid[2] = 0.;

  for ( int i = 0; i < this->GetNumberOfNodes(); i++ )
    {
    this->GetNthNodeWorldPosition( i, p );
    ioCentroid[0] += p[0];
    ioCentroid[1] += p[1];
    ioCentroid[2] += p[2];
    }
  double inv_N = 1. / static_cast< double >( this->GetNumberOfNodes() );
  ioCentroid[0] *= inv_N;
  ioCentroid[1] *= inv_N;
  ioCentroid[2] *= inv_N;
}

//----------------------------------------------------------------------
void vtkOrientedGlyphFocalPlaneContourRepresentation::Scale(double eventPos[2])
{
  // Get the current scale factor
  double sf = this->Glypher->GetScaleFactor();

  // Compute the scale factor
  int *size = this->Renderer->GetSize();
  double dPos = static_cast<double>(eventPos[1]-this->LastEventPosition[1]);
  sf *= (1.0 + 2.0*(dPos / size[1])); //scale factor of 2.0 is arbitrary
  
  // Scale the handle
  this->Glypher->SetScaleFactor(sf);
}

//----------------------------------------------------------------------
void vtkOrientedGlyphFocalPlaneContourRepresentation::CreateDefaultProperties()
{
  this->Property = vtkProperty2D::New();
  this->Property->SetColor(1.0,1.0,1.0);
  this->Property->SetLineWidth(0.5);
  this->Property->SetPointSize(3);

  this->ActiveProperty = vtkProperty2D::New();
  this->ActiveProperty->SetColor(0.0,1.0,0.0);
  this->ActiveProperty->SetLineWidth(1.0);
  
  this->LinesProperty = vtkProperty2D::New();
  this->LinesProperty->SetColor(1,1,1);
  this->LinesProperty->SetLineWidth(1);
}

//----------------------------------------------------------------------
void vtkOrientedGlyphFocalPlaneContourRepresentation::BuildLines()
{
  vtkPoints *points = vtkPoints::New();
  vtkCellArray *lines = vtkCellArray::New();
  
  int i, j;
  vtkIdType index = 0;
  
  int count = this->GetNumberOfNodes();
  for ( i = 0; i < this->GetNumberOfNodes(); i++ )
    {
    count += this->GetNumberOfIntermediatePoints(i);
    }
  
  points->SetNumberOfPoints(count);
  vtkIdType numLines;
  
  if ( this->ClosedLoop && count > 0 )
    {
    numLines = count+1;
    }
  else
    {
    numLines = count;
    }

  if ( numLines > 0 )
    {
    vtkIdType *lineIndices = new vtkIdType[numLines];
    
    double pos[3];
    for ( i = 0; i < this->GetNumberOfNodes(); i++ )
      {
      // Add the node
      this->GetNthNodeDisplayPosition( i, pos );
      points->InsertPoint( index, pos );
      lineIndices[index] = index;
      index++;
      
      int numIntermediatePoints = this->GetNumberOfIntermediatePoints(i);
      
      for ( j = 0; j < numIntermediatePoints; j++ )
        {
        this->GetIntermediatePointDisplayPosition( i, j, pos );
        points->InsertPoint( index, pos );
        lineIndices[index] = index;
        index++;
        }
      }
    
    if ( this->ClosedLoop )
      {
      lineIndices[index] = 0;
      }
    
    lines->InsertNextCell( numLines, lineIndices );
    delete [] lineIndices;
    }
  
  this->Lines->SetPoints( points );
  this->Lines->SetLines( lines );
  
  points->Delete();
  lines->Delete();
}

//----------------------------------------------------------------------
// Returns the direction cosines of the plane on which the contour lies
// on in world co-ordinates. This would be the same matrix that would be
// set in vtkImageReslice or vtkImagePlaneWidget if there were a plane
// passing through the contour points. The origin passed here must be the 
// origin on the image data under the contour. 
vtkMatrix4x4 * vtkOrientedGlyphFocalPlaneContourRepresentation
::GetContourPlaneDirectionCosines( const double origin[3] )
{
  if (this->ContourPlaneDirectionCosines->GetMTime() 
                       >= this->Renderer->GetMTime() ||
      this->ContourPlaneDirectionCosines->GetMTime() 
                       >=    this->Lines->GetMTime() )
    {
    return this->ContourPlaneDirectionCosines;
    }
    
  double pDisplay[3], pWorld[4];
  double fp[4], z, xAxis[3];
  
  this->Renderer->GetActiveCamera()->GetFocalPoint(fp);

  double *vup = this->Renderer->GetActiveCamera()->GetViewUp();
  double *directionOfProjection = this->Renderer->GetActiveCamera()->GetDirectionOfProjection();
 
  vtkInteractorObserver::ComputeWorldToDisplay(this->Renderer, 
                                     fp[0], fp[1], fp[2], fp);
  z = fp[2];

  // This is the Y axis
  this->ContourPlaneDirectionCosines->SetElement(0, 1, vup[0]);
  this->ContourPlaneDirectionCosines->SetElement(1, 1, vup[1]);
  this->ContourPlaneDirectionCosines->SetElement(2, 1, vup[2]);
  this->ContourPlaneDirectionCosines->SetElement(3, 1, 0.0);

  // The Z Axis
  vtkMath::Cross( vup, directionOfProjection, xAxis );
  this->ContourPlaneDirectionCosines->SetElement(0, 2, directionOfProjection[0]);
  this->ContourPlaneDirectionCosines->SetElement(1, 2, directionOfProjection[1]);
  this->ContourPlaneDirectionCosines->SetElement(2, 2, directionOfProjection[2]);
  this->ContourPlaneDirectionCosines->SetElement(3, 2, 0.0);
  
  // X axis = Cross of y and z
  this->ContourPlaneDirectionCosines->SetElement(0, 0, -xAxis[0]);
  this->ContourPlaneDirectionCosines->SetElement(1, 0, -xAxis[1]);
  this->ContourPlaneDirectionCosines->SetElement(2, 0, -xAxis[2]);
  this->ContourPlaneDirectionCosines->SetElement(3, 0, 0.0);
 
  // What point does the origin of the display co-ordinates map to in world
  // co-ordinates with respect to the world co-ordinate origin ?
  pDisplay[0] = 0.0; 
  pDisplay[1] = 0.0;
  vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, 
      pDisplay[0], pDisplay[1], z, pWorld);
  this->ContourPlaneDirectionCosines->SetElement(0, 3, pWorld[0] - origin[0]);
  this->ContourPlaneDirectionCosines->SetElement(1, 3, pWorld[1] - origin[1]);
  this->ContourPlaneDirectionCosines->SetElement(2, 3, pWorld[2] - origin[2]);
  this->ContourPlaneDirectionCosines->SetElement(3, 3, 1.0);

  return this->ContourPlaneDirectionCosines;
}

//----------------------------------------------------------------------
// Returns the contour representation as polydata in world co-ordinates
// For this class, the contour is overlayed on the focal plane.
//
vtkPolyData * vtkOrientedGlyphFocalPlaneContourRepresentation
::GetContourRepresentationAsPolyData()
{
  // Get the points in this contour as a vtkPolyData. 

  vtkPoints *points = vtkPoints::New();
  vtkCellArray *lines = vtkCellArray::New();
  
  int i, j;
  vtkIdType index = 0;
  
  int count = this->GetNumberOfNodes();
  for ( i = 0; i < this->GetNumberOfNodes(); i++ )
    {
    count += this->GetNumberOfIntermediatePoints(i);
    }
  
  points->SetNumberOfPoints(count);
  vtkIdType numLines;
  
  if ( this->ClosedLoop && count > 0 )
    {
    numLines = count+1;
    }
  else
    {
    numLines = count;
    }

  if ( numLines > 0 )
    {
    vtkIdType *lineIndices = new vtkIdType[numLines];
    
    double pos[3];
    for ( i = 0; i < this->GetNumberOfNodes(); i++ )
      {
      // Add the node
      this->GetNthNodeWorldPosition( i, pos );
      points->InsertPoint( index, pos );
      lineIndices[index] = index;
      index++;
      
      int numIntermediatePoints = this->GetNumberOfIntermediatePoints(i);
      
      for ( j = 0; j < numIntermediatePoints; j++ )
        {
        this->GetIntermediatePointWorldPosition( i, j, pos );
        points->InsertPoint( index, pos );
        lineIndices[index] = index;
        index++;
        }
      }
    
    if ( this->ClosedLoop )
      {
      lineIndices[index] = 0;
      }
    
    lines->InsertNextCell( numLines, lineIndices );
    delete [] lineIndices;
    }
  
  this->LinesWorldCoordinates->SetPoints( points );
  this->LinesWorldCoordinates->SetLines( lines );
  
  points->Delete();
  lines->Delete();

  return this->LinesWorldCoordinates; 
}

//----------------------------------------------------------------------
void vtkOrientedGlyphFocalPlaneContourRepresentation::BuildRepresentation()
{
  // Make sure we are up to date with any changes made in the placer
  this->UpdateContour();
  
  double p1[4], p2[4];
  this->Renderer->GetActiveCamera()->GetFocalPoint(p1);

  p1[3] = 1.0;
  this->Renderer->SetWorldPoint(p1);
  this->Renderer->WorldToView();
  this->Renderer->GetViewPoint(p1);
  
  double depth = p1[2];
  double aspect[2];
  this->Renderer->ComputeAspect();
  this->Renderer->GetAspect(aspect);
  
  p1[0] = -aspect[0];
  p1[1] = -aspect[1];
  this->Renderer->SetViewPoint(p1);
  this->Renderer->ViewToWorld();
  this->Renderer->GetWorldPoint(p1);
  
  p2[0] = aspect[0];
  p2[1] = aspect[1];
  p2[2] = depth;
  p2[3] = 1.0;
  this->Renderer->SetViewPoint(p2);
  this->Renderer->ViewToWorld();
  this->Renderer->GetWorldPoint(p2);
  
  double distance = 
    sqrt( vtkMath::Distance2BetweenPoints(p1,p2) );

  int *size = this->Renderer->GetRenderWindow()->GetSize();
  double viewport[4];
  this->Renderer->GetViewport(viewport);
  
  double x, y, scale;
  
  x = size[0] * (viewport[2]-viewport[0]);
  y = size[1] * (viewport[3]-viewport[1]);
  
  scale = sqrt( x*x + y*y );
  
  
  distance = 1000* distance / scale;
  
  this->Glypher->SetScaleFactor( distance * this->HandleSize );
  this->ActiveGlypher->SetScaleFactor( distance * this->HandleSize );
    
  int numPoints = this->GetNumberOfNodes();
  
  if ( this->ActiveNode >= 0 &&
       this->ActiveNode < this->GetNumberOfNodes() )
    {
    this->FocalPoint->SetNumberOfPoints(numPoints-1);  
    this->FocalData->GetPointData()->GetNormals()->SetNumberOfTuples(numPoints-1);
    }
  else
    {
    this->FocalPoint->SetNumberOfPoints(numPoints);  
    this->FocalData->GetPointData()->GetNormals()->SetNumberOfTuples(numPoints);
    }

  int i;
  int idx = 0;
  for ( i = 0; i < numPoints; i++ )
    {
    if ( i != this->ActiveNode )  
      {
      double displayPos[3];
      this->GetNthNodeDisplayPosition( i, displayPos );
      this->FocalPoint->SetPoint(idx, displayPos );
      idx++;
      }
    }
  
  this->FocalPoint->Modified();
  this->FocalData->GetPointData()->GetNormals()->Modified();
  this->FocalData->Modified();

  if ( this->ActiveNode >= 0 &&
       this->ActiveNode < this->GetNumberOfNodes() )
    {
      double displayPos[3];
      this->GetNthNodeDisplayPosition( i, displayPos );
      this->ActiveFocalPoint->SetPoint(0, displayPos );

      this->ActiveFocalPoint->Modified();
      this->ActiveFocalData->GetPointData()->GetNormals()->Modified();
      this->ActiveFocalData->Modified();
      this->ActiveActor->VisibilityOn();
    }
  else
    {
      this->ActiveActor->VisibilityOff();
    }

}

//----------------------------------------------------------------------
void vtkOrientedGlyphFocalPlaneContourRepresentation::GetActors2D(vtkPropCollection *pc)
{
  this->Actor->GetActors2D(pc);
  this->ActiveActor->GetActors2D(pc);
  this->LinesActor->GetActors2D(pc);
}

//----------------------------------------------------------------------
void vtkOrientedGlyphFocalPlaneContourRepresentation::ReleaseGraphicsResources(vtkWindow *win)
{
  this->Actor->ReleaseGraphicsResources(win);
  this->ActiveActor->ReleaseGraphicsResources(win);
  this->LinesActor->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int vtkOrientedGlyphFocalPlaneContourRepresentation::RenderOverlay(vtkViewport *viewport)
{
  int count=0;
  count += this->LinesActor->RenderOverlay(viewport);
  if ( this->Actor->GetVisibility() )
    {
    count +=  this->Actor->RenderOverlay(viewport);
    }
  if ( this->ActiveActor->GetVisibility() )
    {
    count +=  this->ActiveActor->RenderOverlay(viewport);
    }
  return count;
}

//----------------------------------------------------------------------
int vtkOrientedGlyphFocalPlaneContourRepresentation::RenderOpaqueGeometry(vtkViewport *viewport)
{
  // Since we know RenderOpaqueGeometry gets called first, will do the
  // build here
  this->BuildRepresentation();
  
  int count = this->LinesActor->RenderOpaqueGeometry(viewport);
  if ( this->Actor->GetVisibility() )
    {
    count += this->Actor->RenderOpaqueGeometry(viewport);
    }
  if ( this->ActiveActor->GetVisibility() )
    {
    count += this->ActiveActor->RenderOpaqueGeometry(viewport);
    }
  return count;
}

//-----------------------------------------------------------------------------
int vtkOrientedGlyphFocalPlaneContourRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport *viewport)
{
  int count = this->LinesActor->RenderTranslucentPolygonalGeometry(viewport);
  if ( this->Actor->GetVisibility() )
    {
    count += this->Actor->RenderTranslucentPolygonalGeometry(viewport);
    }
  if ( this->ActiveActor->GetVisibility() )
    {
    count += this->ActiveActor->RenderTranslucentPolygonalGeometry(viewport);
    }
  return count;
}

//-----------------------------------------------------------------------------
int vtkOrientedGlyphFocalPlaneContourRepresentation::HasTranslucentPolygonalGeometry()
{
  int result = this->LinesActor->HasTranslucentPolygonalGeometry();
  if ( this->Actor->GetVisibility() )
    {
    result |= this->Actor->HasTranslucentPolygonalGeometry();
    }
  if ( this->ActiveActor->GetVisibility() )
    {
    result |= this->ActiveActor->HasTranslucentPolygonalGeometry();
    }
  return result;
}


//-----------------------------------------------------------------------------
void vtkOrientedGlyphFocalPlaneContourRepresentation::PrintSelf(
  ostream& os,
  vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "InteractionOffset: (" << this->InteractionOffset[0] 
    << "," << this->InteractionOffset[1] << ")" << endl;

  if ( this->Property )
    {
    os << indent << "Property: " << this->Property << "\n";
    }
  else
    {
    os << indent << "Property: (none)\n";
    }

  if ( this->ActiveProperty )
    {
    os << indent << "Active Property: " << this->ActiveProperty << "\n";
    }
  else
    {
    os << indent << "Active Property: (none)\n";
    }

  if ( this->LinesProperty )
    {
    os << indent << "Lines Property: " << this->LinesProperty << "\n";
    }
  else
    {
    os << indent << "Lines Property: (none)\n";
    }
}

