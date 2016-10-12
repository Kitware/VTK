/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHandleRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHandleRepresentation.h"
#include "vtkCoordinate.h"
#include "vtkInteractorObserver.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkPointPlacer.h"


vtkCxxSetObjectMacro(vtkHandleRepresentation, PointPlacer, vtkPointPlacer );

//----------------------------------------------------------------------
vtkHandleRepresentation::vtkHandleRepresentation()
{
  // Positions are maintained via a vtkCoordinate
  this->DisplayPosition = vtkCoordinate::New();
  this->DisplayPosition->SetCoordinateSystemToDisplay();

  this->WorldPosition = vtkCoordinate::New();
  this->WorldPosition->SetCoordinateSystemToWorld();

  this->InteractionState = vtkHandleRepresentation::Outside;
  this->Tolerance = 15;
  this->ActiveRepresentation = 0;
  this->Constrained = 0;
  this->PointPlacer = vtkPointPlacer::New();

  this->DisplayPositionTime.Modified();
  this->WorldPositionTime.Modified();
}

//----------------------------------------------------------------------
vtkHandleRepresentation::~vtkHandleRepresentation()
{
  this->DisplayPosition->Delete();
  this->WorldPosition->Delete();
  this->SetPointPlacer(NULL);
}

//----------------------------------------------------------------------
void vtkHandleRepresentation::SetDisplayPosition(double displyPos[3])
{
  if (this->Renderer && this->PointPlacer)
  {
    if (this->PointPlacer->ValidateDisplayPosition( this->Renderer, displyPos ))
    {
      double worldPos[3], worldOrient[9];
      if (this->PointPlacer->ComputeWorldPosition(
            this->Renderer, displyPos, worldPos, worldOrient ))
      {
        this->DisplayPosition->SetValue(displyPos);
        this->WorldPosition->SetValue(worldPos);
        this->DisplayPositionTime.Modified();
      }
    }
  }
  else
  {
    this->DisplayPosition->SetValue(displyPos);
    this->DisplayPositionTime.Modified();
  }
}

//----------------------------------------------------------------------
void vtkHandleRepresentation::GetDisplayPosition(double pos[3])
{
  // The position is really represented in the world position; the display
  // position is a convenience to go back and forth between coordinate systems.
  // Also note that the window size may have changed, so it's important to
  // update the display position.
  if ( this->Renderer && (this->WorldPositionTime > this->DisplayPositionTime ||
                          (this->Renderer->GetVTKWindow() &&
                           this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime)) )
  {
    int *p = this->WorldPosition->GetComputedDisplayValue(this->Renderer);
    this->DisplayPosition->SetValue(p[0],p[1],0.0);
  }
  this->DisplayPosition->GetValue(pos);
}

//----------------------------------------------------------------------
double* vtkHandleRepresentation::GetDisplayPosition()
{
  // The position is really represented in the world position; the display
  // position is a convenience to go back and forth between coordinate systems.
  // Also note that the window size may have changed, so it's important to
  // update the display position.
  if ( this->Renderer && (this->WorldPositionTime > this->DisplayPositionTime ||
                          (this->Renderer->GetVTKWindow() &&
                           this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime)) )
  {
    int *p = this->WorldPosition->GetComputedDisplayValue(this->Renderer);
    this->DisplayPosition->SetValue(p[0],p[1],0.0);
  }
  return this->DisplayPosition->GetValue();
}

//----------------------------------------------------------------------
void vtkHandleRepresentation::SetWorldPosition(double pos[3])
{
  if (this->Renderer && this->PointPlacer)
  {
    if (this->PointPlacer->ValidateWorldPosition( pos ))
    {
      this->WorldPosition->SetValue(pos);
      this->WorldPositionTime.Modified();
    }
  }
  else
  {
    this->WorldPosition->SetValue(pos);
    this->WorldPositionTime.Modified();
  }
}

//----------------------------------------------------------------------
void vtkHandleRepresentation::GetWorldPosition(double pos[3])
{
  this->WorldPosition->GetValue(pos);
}

//----------------------------------------------------------------------
double* vtkHandleRepresentation::GetWorldPosition()
{
  return this->WorldPosition->GetValue();
}

//----------------------------------------------------------------------
int vtkHandleRepresentation::CheckConstraint(vtkRenderer *vtkNotUsed(renderer),
                                             double vtkNotUsed(pos)[2])
{
  return 1;
}

//----------------------------------------------------------------------
void vtkHandleRepresentation::SetRenderer(vtkRenderer *ren)
{
  this->DisplayPosition->SetViewport(ren);
  this->WorldPosition->SetViewport(ren);
  this->Superclass::SetRenderer(ren);

  // Okay this is weird. If a display position was set previously before
  // the renderer was specified, then the coordinate systems are not
  // synchronized.
  if ( this->DisplayPositionTime > this->WorldPositionTime )
  {
    double p[3];
    this->DisplayPosition->GetValue(p);
    this->SetDisplayPosition(p); //side affect updated world pos
  }
}

//----------------------------------------------------------------------
void vtkHandleRepresentation::DeepCopy(vtkProp *prop)
{
  vtkHandleRepresentation *rep = vtkHandleRepresentation::SafeDownCast(prop);
  if ( rep )
  {
    this->SetTolerance(rep->GetTolerance());
    this->SetActiveRepresentation(rep->GetActiveRepresentation());
    this->SetConstrained(rep->GetConstrained());
    this->SetPointPlacer(rep->GetPointPlacer());
  }
  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
void vtkHandleRepresentation::ShallowCopy(vtkProp *prop)
{
  vtkHandleRepresentation *rep = vtkHandleRepresentation::SafeDownCast(prop);
  if ( rep )
  {
    this->SetTolerance(rep->GetTolerance());
    this->SetActiveRepresentation(rep->GetActiveRepresentation());
    this->SetConstrained(rep->GetConstrained());
  }
  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
vtkMTimeType vtkHandleRepresentation::GetMTime()
{
  vtkMTimeType mTime=this->Superclass::GetMTime();
  vtkMTimeType wMTime=this->WorldPosition->GetMTime();
  mTime = ( wMTime > mTime ? wMTime : mTime );
  vtkMTimeType dMTime=this->DisplayPosition->GetMTime();
  mTime = ( dMTime > mTime ? dMTime : mTime );

  return mTime;
}

//----------------------------------------------------------------------
void vtkHandleRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  double p[3];
  this->GetDisplayPosition(p);
  os << indent << "Display Position: (" << p[0] << ", "
               << p[1] << ", " << p[2] << ")\n";

  this->GetWorldPosition(p);
  os << indent << "World Position: (" << p[0] << ", "
               << p[1] << ", " << p[2] << ")\n";

  os << indent << "Constrained: "
     << (this->Constrained ? "On" : "Off") << "\n";

  os << indent << "Tolerance: " << this->Tolerance << "\n";

  os << indent << "Active Representation: "
     << (this->ActiveRepresentation ? "On" : "Off") << "\n";

  if ( this->PointPlacer )
  {
    os << indent << "PointPlacer:\n";
    this->PointPlacer->PrintSelf( os, indent.GetNextIndent() );
  }
  else
  {
    os << indent << "PointPlacer: (none)\n";
  }

  // this->InteractionState is printed in superclass
  // this is commented to avoid PrintSelf errors
}
