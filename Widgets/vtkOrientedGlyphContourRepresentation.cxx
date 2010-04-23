/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOrientedGlyphContourRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOrientedGlyphContourRepresentation.h"
#include "vtkCleanPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkAssemblyPath.h"
#include "vtkMath.h"
#include "vtkInteractorObserver.h"
#include "vtkLine.h"
#include "vtkCoordinate.h"
#include "vtkGlyph3D.h"
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
#include "vtkOpenGL.h"
#include "vtkSphereSource.h"

vtkStandardNewMacro(vtkOrientedGlyphContourRepresentation);

//----------------------------------------------------------------------
vtkOrientedGlyphContourRepresentation::vtkOrientedGlyphContourRepresentation()
{
  // Initialize state
  this->InteractionState = vtkContourRepresentation::Outside;

  this->CursorShape = NULL;
  this->ActiveCursorShape = NULL;

  this->HandleSize = 0.01;
  
  this->PointPlacer = vtkFocalPlanePointPlacer::New();
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
  
  this->Glypher = vtkGlyph3D::New();
  this->Glypher->SetInput(this->FocalData);
  this->Glypher->SetVectorModeToUseNormal();
  this->Glypher->OrientOn();
  this->Glypher->ScalingOn();
  this->Glypher->SetScaleModeToDataScalingOff();
  this->Glypher->SetScaleFactor(1.0);

  this->ActiveGlypher = vtkGlyph3D::New();
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

  this->Mapper = vtkPolyDataMapper::New();
  this->Mapper->SetInput(this->Glypher->GetOutput());
  this->Mapper->SetResolveCoincidentTopologyToPolygonOffset();
  this->Mapper->ScalarVisibilityOff();
  this->Mapper->ImmediateModeRenderingOn();

  this->ActiveMapper = vtkPolyDataMapper::New();
  this->ActiveMapper->SetInput(this->ActiveGlypher->GetOutput());
  this->ActiveMapper->SetResolveCoincidentTopologyToPolygonOffset();
  this->ActiveMapper->ScalarVisibilityOff();
  this->ActiveMapper->ImmediateModeRenderingOn();

  // Set up the initial properties
  this->CreateDefaultProperties();

  this->Actor = vtkActor::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetProperty(this->Property);

  this->ActiveActor = vtkActor::New();
  this->ActiveActor->SetMapper(this->ActiveMapper);
  this->ActiveActor->SetProperty(this->ActiveProperty);

  this->Lines = vtkPolyData::New();
  this->LinesMapper = vtkPolyDataMapper::New();
  this->LinesMapper->SetInput( this->Lines );
  
  this->LinesActor = vtkActor::New();
  this->LinesActor->SetMapper( this->LinesMapper );
  this->LinesActor->SetProperty( this->LinesProperty );
  
  this->InteractionOffset[0] = 0.0;
  this->InteractionOffset[1] = 0.0;

  this->AlwaysOnTop = 0;
  
  this->SelectedNodesPoints = NULL;
  this->SelectedNodesData = NULL;
  this->SelectedNodesCursorShape = NULL;
  this->SelectedNodesGlypher = NULL;
  this->SelectedNodesMapper = NULL;
  this->SelectedNodesActor = NULL;
}

//----------------------------------------------------------------------
vtkOrientedGlyphContourRepresentation::~vtkOrientedGlyphContourRepresentation()
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
  
  // Clear the selected nodes representation
  if(this->SelectedNodesPoints)
    {
    this->SelectedNodesPoints->Delete();
    }
  if(this->SelectedNodesData)
    {
    this->SelectedNodesData->Delete();
    }
  if(this->SelectedNodesCursorShape)
    {
    this->SelectedNodesCursorShape->Delete();
    }
  if(this->SelectedNodesGlypher)
    {
    this->SelectedNodesGlypher->Delete();
    }
  if(this->SelectedNodesMapper)
    {
    this->SelectedNodesMapper->Delete();
    }
  if(this->SelectedNodesActor)
    {
    this->SelectedNodesActor->Delete();
    }
}

//----------------------------------------------------------------------
void vtkOrientedGlyphContourRepresentation::SetCursorShape(vtkPolyData *shape)
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
vtkPolyData *vtkOrientedGlyphContourRepresentation::GetCursorShape()
{
  return this->CursorShape;
}

//----------------------------------------------------------------------
void vtkOrientedGlyphContourRepresentation::SetActiveCursorShape(vtkPolyData *shape)
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
vtkPolyData *vtkOrientedGlyphContourRepresentation::GetActiveCursorShape()
{
  return this->ActiveCursorShape;
}

//----------------------------------------------------------------------
void vtkOrientedGlyphContourRepresentation::SetRenderer(vtkRenderer *ren)
{
  //  this->WorldPosition->SetViewport(ren);
  this->Superclass::SetRenderer(ren);
}

//-------------------------------------------------------------------------
int vtkOrientedGlyphContourRepresentation::ComputeInteractionState(int X, int Y, int vtkNotUsed(modified))
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
void vtkOrientedGlyphContourRepresentation::StartWidgetInteraction(double startEventPos[2])
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
void vtkOrientedGlyphContourRepresentation::WidgetInteraction(double eventPos[2])
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
void vtkOrientedGlyphContourRepresentation::Translate(double eventPos[2])
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
    }
  else
    {
    // I really want to track the closest point here,
    // but I am postponing this at the moment....
    }
}

//----------------------------------------------------------------------
void vtkOrientedGlyphContourRepresentation::ShiftContour(double eventPos[2])
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
void vtkOrientedGlyphContourRepresentation::ScaleContour(double eventPos[2])
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
void vtkOrientedGlyphContourRepresentation::ComputeCentroid(
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
void vtkOrientedGlyphContourRepresentation::Scale(double eventPos[2])
{
  // Get the current scale factor
  double sf = this->Glypher->GetScaleFactor();

  // Compute the scale factor
  int *size = this->Renderer->GetSize();
  double dPos = static_cast<double>(eventPos[1]-this->LastEventPosition[1]);
  sf *= (1.0 + 2.0*(dPos / size[1])); //scale factor of 2.0 is arbitrary
  
  // Scale the handle
  this->Glypher->SetScaleFactor(sf);
  if(this->ShowSelectedNodes && this->SelectedNodesGlypher)
    {
    this->SelectedNodesGlypher->SetScaleFactor(sf);
    }
}

//----------------------------------------------------------------------
void vtkOrientedGlyphContourRepresentation::CreateDefaultProperties()
{
  this->Property = vtkProperty::New();
  this->Property->SetColor(1.0,1.0,1.0);
  this->Property->SetLineWidth(0.5);
  this->Property->SetPointSize(3);

  this->ActiveProperty = vtkProperty::New();
  this->ActiveProperty->SetColor(0.0,1.0,0.0);
  this->ActiveProperty->SetRepresentationToWireframe();
  this->ActiveProperty->SetAmbient(1.0);
  this->ActiveProperty->SetDiffuse(0.0);
  this->ActiveProperty->SetSpecular(0.0);
  this->ActiveProperty->SetLineWidth(1.0);
  
  this->LinesProperty = vtkProperty::New();
  this->LinesProperty->SetAmbient(1.0);
  this->LinesProperty->SetDiffuse(0.0);
  this->LinesProperty->SetSpecular(0.0);
  this->LinesProperty->SetColor(1,1,1);
  this->LinesProperty->SetLineWidth(1);
}

//----------------------------------------------------------------------
void vtkOrientedGlyphContourRepresentation::BuildLines()
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
  
  this->Lines->SetPoints( points );
  this->Lines->SetLines( lines );
  
  points->Delete();
  lines->Delete();
}

//----------------------------------------------------------------------
vtkPolyData * 
vtkOrientedGlyphContourRepresentation::GetContourRepresentationAsPolyData()
{
  // Get the points in this contour as a vtkPolyData. 
  return this->Lines; 
}

//----------------------------------------------------------------------
void vtkOrientedGlyphContourRepresentation::BuildRepresentation()
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
  int i;
  if(this->ShowSelectedNodes && this->SelectedNodesGlypher) 
    {
    this->SelectedNodesGlypher->SetScaleFactor( distance * this->HandleSize );
    this->FocalPoint->Reset();
    this->FocalPoint->SetNumberOfPoints(0);  
    this->FocalData->GetPointData()->GetNormals()->SetNumberOfTuples(0);
    this->SelectedNodesPoints->Reset();
    this->SelectedNodesPoints->SetNumberOfPoints(0);  
    this->SelectedNodesData->GetPointData()->GetNormals()->SetNumberOfTuples(0);
    for ( i = 0; i < numPoints; i++ )
      {
      if ( i != this->ActiveNode )  
        {
        double worldPos[3];
        double worldOrient[9];
        this->GetNthNodeWorldPosition( i, worldPos );
        this->GetNthNodeWorldOrientation( i, worldOrient );
        if(this->GetNthNodeSelected(i))
          {
          this->SelectedNodesPoints->InsertNextPoint(worldPos );
          this->SelectedNodesData->GetPointData()->GetNormals()->InsertNextTuple(worldOrient+6);
          }
        else
          {
          this->FocalPoint->InsertNextPoint(worldPos );
          this->FocalData->GetPointData()->GetNormals()->InsertNextTuple(worldOrient+6);
          }
        }
      }
    this->SelectedNodesPoints->Modified();
    this->SelectedNodesData->GetPointData()->GetNormals()->Modified();
    this->SelectedNodesData->Modified();
    }   
  else
    {
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
    int idx = 0;
    for ( i = 0; i < numPoints; i++ )
      {
      if ( i != this->ActiveNode )  
        {
        double worldPos[3];
        double worldOrient[9];
        this->GetNthNodeWorldPosition( i, worldPos );
        this->GetNthNodeWorldOrientation( i, worldOrient );
        this->FocalPoint->SetPoint(idx, worldPos );
        this->FocalData->GetPointData()->GetNormals()->SetTuple(idx,worldOrient+6);
        idx++;
        }
      }
    }
    
  this->FocalPoint->Modified();
  this->FocalData->GetPointData()->GetNormals()->Modified();
  this->FocalData->Modified();

  if ( this->ActiveNode >= 0 &&
       this->ActiveNode < this->GetNumberOfNodes() )
    {
    double worldPos[3];
    double worldOrient[9];
    this->GetNthNodeWorldPosition( this->ActiveNode, worldPos );
    this->GetNthNodeWorldOrientation( this->ActiveNode, worldOrient );
    this->ActiveFocalPoint->SetPoint(0, worldPos );
    this->ActiveFocalData->GetPointData()->GetNormals()->SetTuple(0,worldOrient+6);

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
void vtkOrientedGlyphContourRepresentation::GetActors(vtkPropCollection *pc)
{
  this->Actor->GetActors(pc);
  this->ActiveActor->GetActors(pc);
  this->LinesActor->GetActors(pc);
  if(this->ShowSelectedNodes && this->SelectedNodesActor)
    {
    this->SelectedNodesActor->GetActors(pc);
    }
}

//----------------------------------------------------------------------
void vtkOrientedGlyphContourRepresentation::ReleaseGraphicsResources(vtkWindow *win)
{
  this->Actor->ReleaseGraphicsResources(win);
  this->ActiveActor->ReleaseGraphicsResources(win);
  this->LinesActor->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int vtkOrientedGlyphContourRepresentation::RenderOverlay(vtkViewport *viewport)
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

//-----------------------------------------------------------------------------
int vtkOrientedGlyphContourRepresentation::RenderOpaqueGeometry(
  vtkViewport *viewport)
{
  // Since we know RenderOpaqueGeometry gets called first, will do the
  // build here
  this->BuildRepresentation();
  
  GLboolean flag = GL_FALSE;
  if ( this->AlwaysOnTop 
      && (this->ActiveActor->GetVisibility() ||
          this->LinesActor->GetVisibility()))
    {
    glGetBooleanv(GL_DEPTH_TEST,&flag);
    if(flag)
      {
      glDisable( GL_DEPTH_TEST );
      }
    }

  int count=0;
  count += this->LinesActor->RenderOpaqueGeometry(viewport);
  if ( this->Actor->GetVisibility() )
    {
    count += this->Actor->RenderOpaqueGeometry(viewport);
    }
  if ( this->ActiveActor->GetVisibility() )
    {
    count += this->ActiveActor->RenderOpaqueGeometry(viewport);
    }
  if(this->ShowSelectedNodes && this->SelectedNodesActor &&
      this->SelectedNodesActor->GetVisibility())
    {
    count += this->SelectedNodesActor->RenderOpaqueGeometry(viewport);
    }

  if(flag && this->AlwaysOnTop
          && (this->ActiveActor->GetVisibility() ||
              this->LinesActor->GetVisibility()))
    {
    glEnable( GL_DEPTH_TEST );
    }
    
  return count;
}

//-----------------------------------------------------------------------------
int vtkOrientedGlyphContourRepresentation::RenderTranslucentPolygonalGeometry(
  vtkViewport *viewport)
{
  int count=0;
  count += this->LinesActor->RenderTranslucentPolygonalGeometry(viewport);
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
int vtkOrientedGlyphContourRepresentation::HasTranslucentPolygonalGeometry()
{
  int result=0;
  result |= this->LinesActor->HasTranslucentPolygonalGeometry();
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

//----------------------------------------------------------------------------
void vtkOrientedGlyphContourRepresentation::SetLineColor(
  double r, double g, double b)
{
  if(this->GetLinesProperty())
    {
    this->GetLinesProperty()->SetColor(r, g, b);
    }
}

//----------------------------------------------------------------------
void vtkOrientedGlyphContourRepresentation::SetShowSelectedNodes(
  int flag )
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this
    << "): setting ShowSelectedNodes to " << flag);
  if (this->ShowSelectedNodes != flag)
    {
    this->ShowSelectedNodes = flag;
    this->Modified();

    if(this->ShowSelectedNodes)
      {
      if(!this->SelectedNodesActor)
        {
        this->CreateSelectedNodesRepresentation();
        }
      else
        {
        this->SelectedNodesActor->SetVisibility(1);
        }
      }
    else
      {
      if(this->SelectedNodesActor)
        {
        this->SelectedNodesActor->SetVisibility(0);
        }
      }
    }  
}

//----------------------------------------------------------------------
double* vtkOrientedGlyphContourRepresentation::GetBounds()
{
  return this->Lines->GetPoints() ? 
         this->Lines->GetPoints()->GetBounds() : NULL;
}

//-----------------------------------------------------------------------------
void vtkOrientedGlyphContourRepresentation::CreateSelectedNodesRepresentation()
{
  vtkSphereSource *sphere = vtkSphereSource::New();
  sphere->SetThetaResolution(12);
  sphere->SetRadius(0.3);
  this->SelectedNodesCursorShape = sphere->GetOutput();
  this->SelectedNodesCursorShape->Register(this);
  sphere->Delete();

  // Represent the position of the cursor
  this->SelectedNodesPoints = vtkPoints::New();
  this->SelectedNodesPoints->SetNumberOfPoints(100);
  //this->SelectedNodesPoints->SetNumberOfPoints(1);
  //this->SelectedNodesPoints->SetPoint(0, 0.0,0.0,0.0);

  vtkDoubleArray *normals = vtkDoubleArray::New();
  normals->SetNumberOfComponents(3);
  normals->SetNumberOfTuples(100);
  normals->SetNumberOfTuples(1);
  double n[3] = {0,0,0};
  normals->SetTuple(0,n);

  this->SelectedNodesData = vtkPolyData::New();
  this->SelectedNodesData->SetPoints(this->SelectedNodesPoints);
  this->SelectedNodesData->GetPointData()->SetNormals(normals);  
  normals->Delete();

  this->SelectedNodesGlypher = vtkGlyph3D::New();
  this->SelectedNodesGlypher->SetInput(this->SelectedNodesData);
  this->SelectedNodesGlypher->SetVectorModeToUseNormal();
  this->SelectedNodesGlypher->OrientOn();
  this->SelectedNodesGlypher->ScalingOn();
  this->SelectedNodesGlypher->SetScaleModeToDataScalingOff();
  this->SelectedNodesGlypher->SetScaleFactor(1.0);

  this->SelectedNodesGlypher->SetSource(this->SelectedNodesCursorShape);

  this->SelectedNodesMapper = vtkPolyDataMapper::New();
  this->SelectedNodesMapper->SetInput(this->SelectedNodesGlypher->GetOutput());
  this->SelectedNodesMapper->SetResolveCoincidentTopologyToPolygonOffset();
  this->SelectedNodesMapper->ScalarVisibilityOff();
  this->SelectedNodesMapper->ImmediateModeRenderingOn();

  vtkProperty* selProperty = vtkProperty::New();
  selProperty->SetColor(0.0,1.0,0.0);
  selProperty->SetLineWidth(0.5);
  selProperty->SetPointSize(3);

  this->SelectedNodesActor = vtkActor::New();
  this->SelectedNodesActor->SetMapper(this->SelectedNodesMapper);
  this->SelectedNodesActor->SetProperty(selProperty);
  selProperty->Delete();
}

//-----------------------------------------------------------------------------
void vtkOrientedGlyphContourRepresentation::PrintSelf(ostream& os,
                                                      vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Always On Top: " 
     << (this->AlwaysOnTop ? "On\n" : "Off\n");
  os << indent << "ShowSelectedNodes: " << this->ShowSelectedNodes << endl;

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
