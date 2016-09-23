/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWidgetRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAbstractPropPicker.h"
#include "vtkWidgetRepresentation.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorObserver.h"
#include "vtkPickingManager.h"

//----------------------------------------------------------------------
vtkWidgetRepresentation::vtkWidgetRepresentation()
{
  this->Renderer = NULL;

  this->InteractionState = 0;
  this->StartEventPosition[0] = 0.0;
  this->StartEventPosition[1] = 0.0;
  this->StartEventPosition[2] = 0.0;

  this->PlaceFactor = 0.5;
  this->Placed = 0;
  this->HandleSize = 0.05;
  this->ValidPick = 0;
  this->HandleSize = 0.01;

  this->InitialBounds[0] = this->InitialBounds[2] = this->InitialBounds[4] = 0.0;
  this->InitialBounds[1] = this->InitialBounds[3] = this->InitialBounds[5] = 1.0;

  this->InitialLength = 0.0;

  this->NeedToRender = 0;

  this->PickingManaged = true;
}

//----------------------------------------------------------------------
vtkWidgetRepresentation::~vtkWidgetRepresentation()
{
  this->UnRegisterPickers();
}

//----------------------------------------------------------------------
void vtkWidgetRepresentation::SetRenderer(vtkRenderer *ren)
{
  if ( ren == this->Renderer )
  {
    return;
  }

  this->UnRegisterPickers();
  this->Renderer = ren;
  this->PickersModified();
  this->Modified();
}

//----------------------------------------------------------------------------
vtkRenderer* vtkWidgetRepresentation::GetRenderer()
{
  return this->Renderer;
}

//----------------------------------------------------------------------------
void vtkWidgetRepresentation::RegisterPickers()
{
}

//----------------------------------------------------------------------------
void vtkWidgetRepresentation::UnRegisterPickers()
{
  vtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
  {
    return;
  }

  pm->RemoveObject(this);
}

//----------------------------------------------------------------------------
void vtkWidgetRepresentation::PickersModified()
{
  vtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
  {
    return;
  }

  this->UnRegisterPickers();
  this->RegisterPickers();
}

//----------------------------------------------------------------------------
vtkPickingManager* vtkWidgetRepresentation::GetPickingManager()
{
  if (!this->Renderer ||
      !this->Renderer->GetRenderWindow() ||
      !this->Renderer->GetRenderWindow()->GetInteractor() ||
      !this->Renderer->GetRenderWindow()->GetInteractor()->GetPickingManager())
  {
    return 0;
  }

  return
    this->Renderer->GetRenderWindow()->GetInteractor()->GetPickingManager();
}

//------------------------------------------------------------------------------
vtkAssemblyPath* vtkWidgetRepresentation::
GetAssemblyPath(double X, double Y, double Z, vtkAbstractPropPicker* picker)
{
  vtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
  {
    picker->Pick(X, Y, Z, this->Renderer);
    return picker->GetPath();
  }

  return pm->GetAssemblyPath(X, Y, 0., picker, this->Renderer, this);
}

//----------------------------------------------------------------------
void vtkWidgetRepresentation::AdjustBounds(double bounds[6], double newBounds[6],
                                           double center[3])
{
  center[0] = (bounds[0] + bounds[1])/2.0;
  center[1] = (bounds[2] + bounds[3])/2.0;
  center[2] = (bounds[4] + bounds[5])/2.0;

  newBounds[0] = center[0] + this->PlaceFactor*(bounds[0]-center[0]);
  newBounds[1] = center[0] + this->PlaceFactor*(bounds[1]-center[0]);
  newBounds[2] = center[1] + this->PlaceFactor*(bounds[2]-center[1]);
  newBounds[3] = center[1] + this->PlaceFactor*(bounds[3]-center[1]);
  newBounds[4] = center[2] + this->PlaceFactor*(bounds[4]-center[2]);
  newBounds[5] = center[2] + this->PlaceFactor*(bounds[5]-center[2]);
}

//----------------------------------------------------------------------
void vtkWidgetRepresentation::ShallowCopy(vtkProp *prop)
{
  vtkWidgetRepresentation *rep = vtkWidgetRepresentation::SafeDownCast(prop);
  if ( rep )
  {
    this->SetPlaceFactor(rep->GetPlaceFactor());
    this->SetHandleSize(rep->GetHandleSize());
  }
  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
int vtkWidgetRepresentation::ComputeInteractionState(int, int, int)
{
  return 0;
}

//----------------------------------------------------------------------
double vtkWidgetRepresentation::SizeHandlesInPixels(double factor,
                                                    double pos[3])
{
  //
  int i;
  vtkRenderer *renderer;

  if ( !this->ValidPick || !(renderer=this->Renderer) ||
       !renderer->GetActiveCamera() )
  {
    return (this->HandleSize * factor * this->InitialLength);
  }
  else
  {
    double radius, z;
    double lowerLeft[4], upperRight[4];
    double focalPoint[4];

    vtkInteractorObserver::ComputeWorldToDisplay(this->Renderer,
                                                 pos[0], pos[1], pos[2],
                                                 focalPoint);
    z = focalPoint[2];

    double x = focalPoint[0] - this->HandleSize/2.0;
    double y = focalPoint[1] - this->HandleSize/2.0;
    vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer,x,y,z,lowerLeft);

    x = focalPoint[0] + this->HandleSize/2.0;
    y = focalPoint[1] + this->HandleSize/2.0;
    vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer,x,y,z,upperRight);

    for (radius=0.0, i=0; i<3; i++)
    {
      radius += (upperRight[i] - lowerLeft[i]) *
        (upperRight[i] - lowerLeft[i]);
    }
    return (factor * (sqrt(radius) / 2.0));
  }
}

//----------------------------------------------------------------------
double vtkWidgetRepresentation::SizeHandlesRelativeToViewport(double factor,
                                                              double pos[3])
{
  int i;
  vtkRenderer *renderer;

  if ( !this->ValidPick || !(renderer=this->Renderer) ||
       !renderer->GetActiveCamera() )
  {
    return (this->HandleSize * factor * this->InitialLength);
  }
  else
  {
    double radius, z;
    double windowLowerLeft[4], windowUpperRight[4];
    double *viewport = renderer->GetViewport();
    int *winSize = renderer->GetRenderWindow()->GetSize();
    double focalPoint[4];

    vtkInteractorObserver::ComputeWorldToDisplay(this->Renderer,
                                                 pos[0], pos[1], pos[2],
                                                 focalPoint);
    z = focalPoint[2];

    double x = winSize[0] * viewport[0];
    double y = winSize[1] * viewport[1];
    vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer,x,y,z,windowLowerLeft);

    x = winSize[0] * viewport[2];
    y = winSize[1] * viewport[3];
    vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer,x,y,z,windowUpperRight);

    for (radius=0.0, i=0; i<3; i++)
    {
      radius += (windowUpperRight[i] - windowLowerLeft[i]) *
        (windowUpperRight[i] - windowLowerLeft[i]);
    }

    return (sqrt(radius) * factor * this->HandleSize);
  }
}

//----------------------------------------------------------------------
void vtkWidgetRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Renderer: " << this->Renderer.GetPointer() << "\n";
  os << indent << "Interaction State: " << this->InteractionState << "\n";
  os << indent << "Handle Size: " << this->HandleSize << "\n";
  os << indent << "Need to Render: " << (this->NeedToRender ? "On\n" : "Off\n");
  os << indent << "Place Factor: " << this->PlaceFactor << "\n";
}
