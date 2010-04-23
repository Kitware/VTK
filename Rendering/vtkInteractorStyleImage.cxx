/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInteractorStyleImage.h"

#include "vtkAbstractPropPicker.h"
#include "vtkAssemblyPath.h"
#include "vtkCallbackCommand.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"

vtkStandardNewMacro(vtkInteractorStyleImage);

//----------------------------------------------------------------------------
vtkInteractorStyleImage::vtkInteractorStyleImage() 
{
  this->WindowLevelStartPosition[0]   = 0;
  this->WindowLevelStartPosition[1]   = 0;  

  this->WindowLevelCurrentPosition[0] = 0;
  this->WindowLevelCurrentPosition[1] = 0;  
}

//----------------------------------------------------------------------------
vtkInteractorStyleImage::~vtkInteractorStyleImage() 
{
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::StartWindowLevel() 
{
  if (this->State != VTKIS_NONE) 
    {
    return;
    }
  this->StartState(VTKIS_WINDOW_LEVEL);
  this->InvokeEvent(vtkCommand::StartWindowLevelEvent,this);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::EndWindowLevel() 
{
  if (this->State != VTKIS_WINDOW_LEVEL) 
    {
    return;
    }
  this->InvokeEvent(vtkCommand::EndWindowLevelEvent, this);
  this->StopState();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::StartPick() 
{
  if (this->State != VTKIS_NONE) 
    {
    return;
    }
  this->StartState(VTKIS_PICK);
  this->InvokeEvent(vtkCommand::StartPickEvent, this);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::EndPick() 
{
  if (this->State != VTKIS_PICK) 
    {
    return;
    }
  this->InvokeEvent(vtkCommand::EndPickEvent, this);
  this->StopState();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::OnMouseMove() 
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  switch (this->State) 
    {
    case VTKIS_WINDOW_LEVEL:
      this->FindPokedRenderer(x, y);
      this->WindowLevel();
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      break;

    case VTKIS_PICK:
      this->FindPokedRenderer(x, y);
      this->Pick();
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      break;
    }

  // Call parent to handle all other states and perform additional work

  this->Superclass::OnMouseMove();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::OnLeftButtonDown() 
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  // Redefine this button to handle window/level
  this->GrabFocus(this->EventCallbackCommand);
  if (!this->Interactor->GetShiftKey() && !this->Interactor->GetControlKey()) 
    {
    this->WindowLevelStartPosition[0] = x;
    this->WindowLevelStartPosition[1] = y;      
    this->StartWindowLevel();
    }

  // The rest of the button + key combinations remain the same

  else
    {
    this->Superclass::OnLeftButtonDown();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::OnLeftButtonUp()
{
  switch (this->State) 
    {
    case VTKIS_WINDOW_LEVEL:
      this->EndWindowLevel();
      if ( this->Interactor )
        {
        this->ReleaseFocus();
        }
      break;
    }
  
  // Call parent to handle all other states and perform additional work

  this->Superclass::OnLeftButtonUp();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::OnRightButtonDown() 
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  // Redefine this button + shift to handle pick
  this->GrabFocus(this->EventCallbackCommand);
  if (this->Interactor->GetShiftKey())
    {
    this->StartPick();
    }

  // The rest of the button + key combinations remain the same

  else
    {
    this->Superclass::OnRightButtonDown();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::OnRightButtonUp() 
{
  switch (this->State) 
    {
    case VTKIS_PICK:
      this->EndPick();
      if ( this->Interactor )
        {
        this->ReleaseFocus();
        }
      break;
    }

  // Call parent to handle all other states and perform additional work

  this->Superclass::OnRightButtonUp();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::OnChar() 
{
  vtkRenderWindowInteractor *rwi = this->Interactor;

  switch (rwi->GetKeyCode()) 
    {
    case 'f' :      
    case 'F' :
      {
      this->AnimState = VTKIS_ANIM_ON;
      vtkAssemblyPath *path=NULL;
      this->FindPokedRenderer(rwi->GetEventPosition()[0],
                              rwi->GetEventPosition()[1]);
      rwi->GetPicker()->Pick(rwi->GetEventPosition()[0],
                             rwi->GetEventPosition()[1], 0.0, 
                             this->CurrentRenderer);
      vtkAbstractPropPicker *picker;
      if ( (picker=vtkAbstractPropPicker::SafeDownCast(rwi->GetPicker())) )
        {
        path = picker->GetPath();
        }
      if ( path != NULL )
        {
        rwi->FlyToImage(this->CurrentRenderer,picker->GetPickPosition());
        }
      this->AnimState = VTKIS_ANIM_OFF;
      break;
      }

    case 'r' :      
    case 'R' :
      // Allow either shift/ctrl to trigger the usual 'r' binding
      // otherwise trigger reset window level event
      if (rwi->GetShiftKey() || rwi->GetControlKey())
        {
        this->Superclass::OnChar();
        }
      else
        {
          this->InvokeEvent(vtkCommand::ResetWindowLevelEvent, this);
        }
      break;

    default:
      this->Superclass::OnChar();
      break;
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::WindowLevel()
{
  vtkRenderWindowInteractor *rwi = this->Interactor;

  this->WindowLevelCurrentPosition[0] = rwi->GetEventPosition()[0];
  this->WindowLevelCurrentPosition[1] = rwi->GetEventPosition()[1];
  
  this->InvokeEvent(vtkCommand::WindowLevelEvent, this);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::Pick()
{
  this->InvokeEvent(vtkCommand::PickEvent, this);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "Window Level Current Position: ("
     << this->WindowLevelCurrentPosition[0] << ", "
     << this->WindowLevelCurrentPosition[1] << ")" << endl;

  os << indent << "Window Level Start Position: ("
     << this->WindowLevelStartPosition[0] << ", "
     << this->WindowLevelStartPosition[1] << ")" << endl;
}
