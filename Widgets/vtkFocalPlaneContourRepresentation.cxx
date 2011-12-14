/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFocalPlaneContourRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFocalPlaneContourRepresentation.h"
#include "vtkHandleRepresentation.h"
#include "vtkCoordinate.h"
#include "vtkRenderer.h"
#include "vtkObjectFactory.h"
#include "vtkBox.h"
#include "vtkInteractorObserver.h"
#include "vtkMath.h"
#include "vtkFocalPlanePointPlacer.h"
#include "vtkContourLineInterpolator.h"
#include "vtkLine.h"
#include "vtkCamera.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"

#include <vector>
#include <set>
#include <algorithm>
#include <iterator>


//----------------------------------------------------------------------
vtkFocalPlaneContourRepresentation::vtkFocalPlaneContourRepresentation()
{
  this->PointPlacer              = vtkFocalPlanePointPlacer::New();
}

//----------------------------------------------------------------------
vtkFocalPlaneContourRepresentation::~vtkFocalPlaneContourRepresentation()
{
}

//----------------------------------------------------------------------
// Compute the world position from the display position for this given
// point using the renderer.
int vtkFocalPlaneContourRepresentation::GetIntermediatePointWorldPosition(int n, 
                                                                int idx, 
                                                                double point[3])
{
  if ( n < 0 ||
       static_cast<unsigned int>(n) >= this->Internal->Nodes.size() )
    {
    return 0;
    }
  
  if ( idx < 0 ||
       static_cast<unsigned int>(idx) >= this->Internal->Nodes[n]->Points.size() )
    {
    return 0;
    }
  
  double p[4], fp[4], z, dispPos[2];
  this->Renderer->GetActiveCamera()->GetFocalPoint(fp);
  vtkInteractorObserver::ComputeWorldToDisplay(this->Renderer, 
                                      fp[0], fp[1], fp[2], fp);
  z = fp[2];
  
  dispPos[0] = this->Internal->Nodes[n]->Points[idx]->NormalizedDisplayPosition[0];
  dispPos[1] = this->Internal->Nodes[n]->Points[idx]->NormalizedDisplayPosition[1];
  this->Renderer->NormalizedDisplayToDisplay( dispPos[0], dispPos[1] );
  
  vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, 
                                dispPos[0], dispPos[1], z, p);
  
  point[0] = p[0];
  point[1] = p[1];
  point[2] = p[2];
  
  return 1;
}  

//----------------------------------------------------------------------
// Compute the world position from the display position for this given
// point using the renderer.
int vtkFocalPlaneContourRepresentation::GetIntermediatePointDisplayPosition(int n, 
                                                                int idx, 
                                                                double point[3])
{
  if ( n < 0 ||
       static_cast<unsigned int>(n) >= this->Internal->Nodes.size() )
    {
    return 0;
    }
  
  if ( idx < 0 ||
       static_cast<unsigned int>(idx) >= this->Internal->Nodes[n]->Points.size() )
    {
    return 0;
    }
  
  point[0] = this->Internal->Nodes[n]->Points[idx]->NormalizedDisplayPosition[0];
  point[1] = this->Internal->Nodes[n]->Points[idx]->NormalizedDisplayPosition[1];
  this->Renderer->NormalizedDisplayToDisplay( point[0], point[1] );
  
  return 1;
}  

//----------------------------------------------------------------------
int vtkFocalPlaneContourRepresentation::GetNthNodeDisplayPosition( 
                                     int n, double displayPos[2] )
{
  if ( n < 0 ||
       static_cast<unsigned int>(n) >= this->Internal->Nodes.size() )
    {
    return 0;
    }
  
  displayPos[0] = this->Internal->Nodes[n]->NormalizedDisplayPosition[0];
  displayPos[1] = this->Internal->Nodes[n]->NormalizedDisplayPosition[1];
  this->Renderer->NormalizedDisplayToDisplay( displayPos[0], displayPos[1] );

  return 1;
}

//----------------------------------------------------------------------
int vtkFocalPlaneContourRepresentation::GetNthNodeWorldPosition( 
                                     int n, double worldPos[3] )
{
  if ( n < 0 ||
       static_cast<unsigned int>(n) >= this->Internal->Nodes.size() )
    {
    return 0;
    }
  
  double p[4], fp[4], z, dispPos[2];
  this->Renderer->GetActiveCamera()->GetFocalPoint(fp);
  vtkInteractorObserver::ComputeWorldToDisplay(this->Renderer, 
                                      fp[0], fp[1], fp[2], fp);
  z = fp[2];
  dispPos[0] = this->Internal->Nodes[n]->NormalizedDisplayPosition[0];
  dispPos[1] = this->Internal->Nodes[n]->NormalizedDisplayPosition[1];
  this->Renderer->NormalizedDisplayToDisplay( dispPos[0], dispPos[1] );
  vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, 
                                dispPos[0], dispPos[1], z, p);
  
  worldPos[0] = p[0];
  worldPos[1] = p[1];
  worldPos[2] = p[2];

  return 1;
}

//----------------------------------------------------------------------
void vtkFocalPlaneContourRepresentation
::UpdateContourWorldPositionsBasedOnDisplayPositions()
{
  double p[4], fp[4], z, dispPos[2];
  this->Renderer->GetActiveCamera()->GetFocalPoint(fp);
  vtkInteractorObserver::ComputeWorldToDisplay(this->Renderer, 
                                      fp[0], fp[1], fp[2], fp);
  z = fp[2];
  
  for(unsigned int i=0;i<this->Internal->Nodes.size();i++)
    {
      
    dispPos[0] = this->Internal->Nodes[i]->NormalizedDisplayPosition[0];
    dispPos[1] = this->Internal->Nodes[i]->NormalizedDisplayPosition[1];
    this->Renderer->NormalizedDisplayToDisplay( dispPos[0], dispPos[1] );
    
    vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, 
                                    dispPos[0], dispPos[1], z, p);
    
    this->Internal->Nodes[i]->WorldPosition[0] = p[0];
    this->Internal->Nodes[i]->WorldPosition[1] = p[1];
    this->Internal->Nodes[i]->WorldPosition[2] = p[2];
    
    for (unsigned int j=0;j<this->Internal->Nodes[i]->Points.size();j++)
      {
      dispPos[0] = this->Internal->Nodes[i]->Points[j]->NormalizedDisplayPosition[0];
      dispPos[1] = this->Internal->Nodes[i]->Points[j]->NormalizedDisplayPosition[1];
      this->Renderer->NormalizedDisplayToDisplay( dispPos[0], dispPos[1] );
      
      vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, 
                                      dispPos[0], dispPos[1], z, p);
      
      this->Internal->Nodes[i]->Points[j]->WorldPosition[0] = p[0];
      this->Internal->Nodes[i]->Points[j]->WorldPosition[1] = p[1];
      this->Internal->Nodes[i]->Points[j]->WorldPosition[2] = p[2];
      }
    }
}

//---------------------------------------------------------------------
int vtkFocalPlaneContourRepresentation::UpdateContour()
{
  this->PointPlacer->UpdateInternalState();
  
  if ( this->ContourBuildTime > this->Renderer->GetMTime() &&
       this->ContourBuildTime > this->PointPlacer->GetMTime() )
    {
    // Contour does not need to be rebuilt
    return 0;
    }
  
  // The representation maintains its true positions based on display positions.
  // Sync the world positions in terms of the current display positions.
  // The superclass will do the line interpolation etc from the world positions
  // 
  // TODO:
  // I don't want to rebuild the contour every time the renderer changes
  // state. For instance there is no reason for me to re-do this when a W/L
  // action is performed on the window. How do I know if relevant events have 
  // transpired like zoom or scale changes ?
  //   In the current state, the 'if' statement above is invariably true.
  // 
  this->UpdateContourWorldPositionsBasedOnDisplayPositions();
  
  unsigned int i;
  for(i=0; (i+1)<this->Internal->Nodes.size(); i++)
    {
    this->UpdateLine(i, i+1);
    }
  
  if ( this->ClosedLoop )
    {
    this->UpdateLine( static_cast<int>(this->Internal->Nodes.size())-1, 0);
    }
  this->BuildLines();
  
  this->ContourBuildTime.Modified();
  return 1;
}

//----------------------------------------------------------------------
void vtkFocalPlaneContourRepresentation::UpdateLines( int index )
{
  this->Superclass::UpdateLines(index);
}

//----------------------------------------------------------------------
void vtkFocalPlaneContourRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

