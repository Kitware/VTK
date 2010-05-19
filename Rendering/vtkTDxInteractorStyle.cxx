/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTDxInteractorStyle.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTDxInteractorStyle.h"

#include "vtkCommand.h"
#include "vtkTDxInteractorStyleSettings.h"
#include "vtkTDxMotionEventInfo.h" // Borland needs it.


vtkCxxSetObjectMacro(vtkTDxInteractorStyle,Settings,
                     vtkTDxInteractorStyleSettings);

// ----------------------------------------------------------------------------
vtkTDxInteractorStyle::vtkTDxInteractorStyle()
{
  this->Renderer=0;
  this->Settings=vtkTDxInteractorStyleSettings::New();
}

// ----------------------------------------------------------------------------
vtkTDxInteractorStyle::~vtkTDxInteractorStyle()
{
  if(this->Settings!=0)
    {
    this->Settings->Delete();
    }
}

// ----------------------------------------------------------------------------
void vtkTDxInteractorStyle::ProcessEvent(vtkRenderer *renderer,
                                         unsigned long event,
                                         void *calldata)
{
  vtkDebugMacro(<<"vtkTDxInteractorStyle::ProcessEvent()");
  this->Renderer=renderer;
  
  vtkTDxMotionEventInfo *motionInfo;
  int *buttonInfo;
  
  switch(event)
    {
    case vtkCommand::TDxMotionEvent:
      vtkDebugMacro(<<"vtkTDxInteractorStyle::ProcessEvent() TDxMotionEvent");
      motionInfo=static_cast<vtkTDxMotionEventInfo *>(calldata);
      this->OnMotionEvent(motionInfo);
      break;
    case vtkCommand::TDxButtonPressEvent:
      vtkDebugMacro(<<"vtkTDxInteractorStyle::ProcessEvent() TDxButtonPressEvent");
      buttonInfo=static_cast<int *>(calldata);
      this->OnButtonPressedEvent(*buttonInfo);
      break;
    case vtkCommand::TDxButtonReleaseEvent:
      vtkDebugMacro(<<"vtkTDxInteractorStyle::ProcessEvent() TDxButtonReleaseEvent");
      buttonInfo=static_cast<int *>(calldata);
      this->OnButtonReleasedEvent(*buttonInfo);
      break;
    }
}

// ----------------------------------------------------------------------------
void vtkTDxInteractorStyle::OnMotionEvent(
  vtkTDxMotionEventInfo *vtkNotUsed(motionInfo))
{
  vtkDebugMacro(<<"vtkTDxInteractorStyle::OnMotionEvent()");
}
 
// ----------------------------------------------------------------------------
void vtkTDxInteractorStyle::OnButtonPressedEvent(int vtkNotUsed(button))
{
  vtkDebugMacro(<<"vtkTDxInteractorStyle::OnButtonPressedEvent()");
}
  
// ----------------------------------------------------------------------------
void vtkTDxInteractorStyle::OnButtonReleasedEvent(int vtkNotUsed(button))
{
  vtkDebugMacro(<<"vtkTDxInteractorStyle::OnButtonReleasedEvent()");
}

//----------------------------------------------------------------------------
void vtkTDxInteractorStyle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Settings: ";
  if(this->Settings==0)
    {
    os << "(none)" << endl;
    }
  else
    {
    os << endl;
    this->Settings->PrintSelf(os,indent.GetNextIndent());
    }
}
