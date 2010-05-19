/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk3DWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtk3DWidget.h"

#include "vtkCallbackCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProp3D.h"
#include "vtkDataSet.h"
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"


vtkCxxSetObjectMacro(vtk3DWidget,Prop3D,vtkProp3D);
vtkCxxSetObjectMacro(vtk3DWidget,Input,vtkDataSet);

//----------------------------------------------------------------------------
vtk3DWidget::vtk3DWidget()
{
  this->Placed = 0;
  this->Prop3D = NULL;
  this->Input = NULL;
  this->PlaceFactor = 0.5;

  this->Priority = 0.5;
  
  this->HandleSize = 0.01;
  this->ValidPick = 0;
}

//----------------------------------------------------------------------------
vtk3DWidget::~vtk3DWidget()
{
  if ( this->Input )
    {
    this->Input->Delete();
    this->Input = NULL;
    }
  if ( this->Prop3D )
    {
    this->Prop3D->Delete();
    this->Prop3D = NULL;
    }
}

//----------------------------------------------------------------------------
void vtk3DWidget::PlaceWidget()
{
  double bounds[6];

  if ( this->Prop3D )
    {
    this->Prop3D->GetBounds(bounds);
    }
  else if ( this->Input )
    {
    this->Input->Update();
    this->Input->GetBounds(bounds);
    }
  else
    {
    vtkErrorMacro(<<"No input or prop defined for widget placement");
    bounds[0] = -1.0;
    bounds[1] = 1.0;
    bounds[2] = -1.0;
    bounds[3] = 1.0;
    bounds[4] = -1.0;
    bounds[5] = 1.0;
    }
  
  this->PlaceWidget(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
}

//----------------------------------------------------------------------------
void vtk3DWidget::PlaceWidget(double xmin, double xmax, 
                              double ymin, double ymax, 
                              double zmin, double zmax)
{
  double bounds[6];

  bounds[0] = xmin;
  bounds[1] = xmax;
  bounds[2] = ymin;
  bounds[3] = ymax;
  bounds[4] = zmin;
  bounds[5] = zmax;
  
  this->PlaceWidget(bounds);
  this->InvokeEvent(vtkCommand::PlaceWidgetEvent,NULL);  
  this->Placed = 1;
}

//----------------------------------------------------------------------------
void vtk3DWidget::AdjustBounds(double bounds[6], 
                               double newBounds[6], double center[3])
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

//----------------------------------------------------------------------------
double vtk3DWidget::SizeHandles(double factor)
{
  int i;
  vtkRenderer *renderer;

  if ( !this->ValidPick || !(renderer=this->CurrentRenderer) || 
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

    this->ComputeWorldToDisplay(this->LastPickPosition[0], 
                                this->LastPickPosition[1],
                                this->LastPickPosition[2], focalPoint);
    z = focalPoint[2];

    double x = winSize[0] * viewport[0];
    double y = winSize[1] * viewport[1];
    this->ComputeDisplayToWorld(x,y,z,windowLowerLeft);

    x = winSize[0] * viewport[2];
    y = winSize[1] * viewport[3];
    this->ComputeDisplayToWorld(x,y,z,windowUpperRight);

    for (radius=0.0, i=0; i<3; i++) 
      {
      radius += (windowUpperRight[i] - windowLowerLeft[i]) *
        (windowUpperRight[i] - windowLowerLeft[i]);
      }

    return (sqrt(radius) * factor * this->HandleSize);
    }
}


//----------------------------------------------------------------------------
void vtk3DWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Prop3D: " << this->Prop3D << "\n";
  os << indent << "Input: " << this->Input << "\n";
  os << indent << "Handle Size: " << this->HandleSize << "\n";
  os << indent << "Place Factor: " << this->PlaceFactor << "\n";

}


