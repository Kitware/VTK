/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtk3DWidget.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtk3DWidget.h"
#include "vtkCallbackCommand.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtk3DWidget, "1.2");

vtk3DWidget::vtk3DWidget()
{
  this->Prop3D = NULL;
  this->Input = NULL;
  this->Mode = 1;
  this->State = vtk3DWidget::Start;
  this->Placed = 0;

  this->Interactor = NULL;
  this->WidgetCallbackCommand = vtkCallbackCommand::New();
  this->WidgetCallbackCommand->SetClientData(this); 
 
  this->CurrentRenderer = NULL;
  this->OldX = 0.0;
  this->OldY = 0.0;
  
  this->Priority = 1.0;
  this->KeyPressActivation = 1;
  this->KeyPressActivationValue = 'W';
}

vtk3DWidget::~vtk3DWidget()
{
  if ( this->Input )
    {
    this->Input->Delete();
    this->Input = NULL;
    }
}

void vtk3DWidget::PlaceWidget()
{
  float bounds[6];

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
  
  this->PlaceWidget(bounds);
}

// Description:
// Transform from display to world coordinates.
// WorldPt has to be allocated as 4 vector
void vtk3DWidget::ComputeDisplayToWorld(double x, double y,
                                         double z, double *worldPt)
{
  this->CurrentRenderer->SetDisplayPoint(x, y, z);
  this->CurrentRenderer->DisplayToWorld();
  this->CurrentRenderer->GetWorldPoint(worldPt);
  if (worldPt[3])
    {
    worldPt[0] /= worldPt[3];
    worldPt[1] /= worldPt[3];
    worldPt[2] /= worldPt[3];
    worldPt[3] = 1.0;
    }
}

// Description:
// Transform from world to display coordinates.
// displayPt has to be allocated as 3 vector
void vtk3DWidget::ComputeWorldToDisplay(double x, double y,
                                         double z, double *displayPt)
{
  this->CurrentRenderer->SetWorldPoint(x, y, z, 1.0);
  this->CurrentRenderer->WorldToDisplay();
  this->CurrentRenderer->GetDisplayPoint(displayPt);
}

void vtk3DWidget::OnChar(int ctrl, int shift, char keycode, int repeatcount) 
{
  // catch additional keycodes otherwise
  if ( this->KeyPressActivation )
    {
    if (keycode == this->KeyPressActivationValue )
      {
      if ( this->Mode == vtk3DWidget::WidgetOff )
        {
        this->On();
        }
      else
        {
        this->Off();
        }
      this->WidgetCallbackCommand->SetAbortFlag(1);
      }
    }//if activation enabled
}

void vtk3DWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Prop3D: " << this->Prop3D << "\n";
  os << indent << "Input: " << this->Input << "\n";
  os << indent << "Priority: " << this->Priority << "\n";
  os << indent << "Key Press Activation: " 
     << (this->KeyPressActivation ? "On" : "Off") << "\n";
  os << indent << "Key Press Activation Value: " 
     << this->KeyPressActivationValue << "\n";
}


