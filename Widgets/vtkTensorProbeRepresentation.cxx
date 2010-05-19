/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTensorProbeRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTensorProbeRepresentation.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyData.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkInteractorObserver.h"
#include "vtkLine.h"
#include "vtkCoordinate.h"
#include "vtkRenderWindow.h"
#include "vtkCellArray.h"
#include "vtkMath.h"
#include "vtkGenericCell.h"

#define min(x,y) ((x<y) ? (x) : (y))
#define max(x,y) ((x>y) ? (x) : (y))


//----------------------------------------------------------------------
vtkTensorProbeRepresentation::vtkTensorProbeRepresentation()
{
  this->Trajectory = NULL;
  this->TrajectoryMapper = vtkPolyDataMapper::New();
  this->TrajectoryActor = vtkActor::New();
  this->TrajectoryActor->SetMapper(this->TrajectoryMapper);
  this->ProbePosition[0] = VTK_DOUBLE_MAX;
  this->ProbePosition[1] = VTK_DOUBLE_MAX;
  this->ProbePosition[2] = VTK_DOUBLE_MAX;
  this->ProbeCellId = -1;
}

//----------------------------------------------------------------------
vtkTensorProbeRepresentation::~vtkTensorProbeRepresentation()
{
  this->SetTrajectory(NULL);
  this->TrajectoryMapper->Delete();
  this->TrajectoryActor->Delete();
}

//----------------------------------------------------------------------
void vtkTensorProbeRepresentation::SetTrajectory( vtkPolyData * args )
{
  if (this->Trajectory != args)                                       
    {                                                           
    vtkPolyData * tempSGMacroVar = this->Trajectory;                          
    this->Trajectory = args;                                          
    if (this->Trajectory != NULL) 
      { 
      this->Trajectory->Register(this); 
      } 
    if (tempSGMacroVar != NULL)                                 
      {                                                         
      tempSGMacroVar->UnRegister(this);                         
      }                                                         
    this->TrajectoryMapper->SetInput( this->Trajectory );
    this->Modified();
    }                                                           
}

//----------------------------------------------------------------------
int vtkTensorProbeRepresentation::Move( double motionVector[2] )
{
  if (motionVector[0] == 0.0 && motionVector[1] == 0.0)
    {
    return 0;
    }

  vtkIdType cellId;
  double displayPos[4], p2[3], p[4];

  this->GetProbePosition(p);
  cellId = this->GetProbeCellId();

  p[3] = 1.0;
  this->Renderer->SetWorldPoint(p);
  this->Renderer->WorldToDisplay();
  this->Renderer->GetDisplayPoint(displayPos);
  
  displayPos[0] += motionVector[0];
  displayPos[1] += motionVector[1];

  this->FindClosestPointOnPolyline( displayPos, p2, cellId );

  if (vtkMath::Distance2BetweenPoints(p,p2) > 0.0)
    {
    this->SetProbePosition( p2 );
    this->SetProbeCellId( cellId );
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------
void vtkTensorProbeRepresentation
::FindClosestPointOnPolyline( 
    double displayPos[2], double closestWorldPos[3], vtkIdType &cellId,
    int maxSpeed )
{
  vtkIdType npts=0, *ptIds=0;

  this->Trajectory->GetLines()->GetCell( 0, npts, ptIds );
  vtkPoints *points = this->Trajectory->GetPoints();

  vtkIdType minCellId = max( this->ProbeCellId - maxSpeed, 0 );
  vtkIdType maxCellId = min( this->ProbeCellId + maxSpeed, npts-1 );

  double closestT=0.0, closestDist = VTK_DOUBLE_MAX, 
         pprev[3]= {0.0, 0.0, 0.0}, t, closestPt[3], dist, 
         x[3] = { displayPos[0], displayPos[1], 0.0 };
         
  for (vtkIdType id = minCellId; id <= maxCellId; id++)
    {

    double p[4];
    points->GetPoint(id, p);
    p[3] = 1.0;

    this->Renderer->SetWorldPoint(p);
    this->Renderer->WorldToDisplay();
    this->Renderer->GetDisplayPoint(p);

    if (id != minCellId)
      {
      p[2] = 0.0;
      dist = vtkLine::DistanceToLine( x, p, pprev, t, closestPt );
      if (t < 0.0 || t > 1.0)
        {
        double d1 = vtkMath::Distance2BetweenPoints(x,pprev);
        double d2 = vtkMath::Distance2BetweenPoints(x,p);
        if (d1 < d2)
          {
          t = 1.0;
          dist = d1;
          }
        else
          {
          t = 0.0;
          dist = d2;
          }
        }

      if (dist < closestDist)
        {
        closestDist = dist;
        closestT = t;
        closestPt[0] = p[0];
        closestPt[1] = p[1];
        closestPt[2] = p[2];
        cellId = id-1;
        }
      }

    pprev[0] = p[0];
    pprev[1] = p[1];
    }

  double p1[3], p2[3];
  points->GetPoint(cellId, p1);
  points->GetPoint(cellId+1, p2);

  closestWorldPos[0] = closestT* p1[0] + (1-closestT) * p2[0];
  closestWorldPos[1] = closestT* p1[1] + (1-closestT) * p2[1];
  closestWorldPos[2] = closestT* p1[2] + (1-closestT) * p2[2];
}  

//----------------------------------------------------------------------
// Set the probe position as the one closest to the center
void vtkTensorProbeRepresentation::Initialize()
{
  if (this->ProbePosition[0] == VTK_DOUBLE_MAX && this->Trajectory)
    {
    double p[3];
    vtkPoints *points = this->Trajectory->GetPoints();
    points->GetPoint(0, p);

    this->SetProbeCellId(0);
    this->SetProbePosition(p);
    }
}

//----------------------------------------------------------------------
int vtkTensorProbeRepresentation
::RenderOpaqueGeometry(vtkViewport *viewport)
{
  // Since we know RenderOpaqueGeometry gets called first, will do the
  // build here
  this->BuildRepresentation();

  int count = 0;
  count += this->TrajectoryActor->RenderOpaqueGeometry(viewport);
  return count;
}

//----------------------------------------------------------------------
void vtkTensorProbeRepresentation::BuildRepresentation()
{
  this->Initialize();
}

//----------------------------------------------------------------------
void vtkTensorProbeRepresentation::GetActors(vtkPropCollection *pc)
{
  this->TrajectoryActor->GetActors(pc);
}

//----------------------------------------------------------------------
void vtkTensorProbeRepresentation::ReleaseGraphicsResources(vtkWindow *win)
{
  this->TrajectoryActor->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
void vtkTensorProbeRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  os << indent << "TrajectoryActor: " << this->TrajectoryActor << endl;
  os << indent << "TrajectoryMapper: " << this->TrajectoryMapper << endl;
  os << indent << "Trajectory: " << this->Trajectory << endl;
  os << indent << "ProbePosition: (" 
     << this->ProbePosition[0] << "," 
     << this->ProbePosition[1] << ","
     << this->ProbePosition[2] << ")" << endl;
  os << indent << "ProbeCellId: " << this->ProbeCellId << endl; 
}

