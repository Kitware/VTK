/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleImage.cxx
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
#include "vtkInteractorStyleImage.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkCommand.h"

vtkCxxRevisionMacro(vtkInteractorStyleImage, "1.20");
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
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::EndWindowLevel() 
{
  if (this->State != VTKIS_WINDOW_LEVEL) 
    {
    return;
    }
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
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::EndPick() 
{
  if (this->State != VTKIS_PICK) 
    {
    return;
    }
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
      break;

    case VTKIS_PICK:
      this->FindPokedRenderer(x, y);
      this->Pick();
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

  if (!this->Interactor->GetShiftKey() && !this->Interactor->GetControlKey()) 
    {
    this->StartWindowLevel();
    this->WindowLevelStartPosition[0] = x;
    this->WindowLevelStartPosition[1] = y;      
    if (this->HasObserver(vtkCommand::StartWindowLevelEvent)) 
      {
      this->InvokeEvent(vtkCommand::StartWindowLevelEvent,this);
      }
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
      if (this->HasObserver(vtkCommand::EndWindowLevelEvent))
        {
        this->InvokeEvent(vtkCommand::EndWindowLevelEvent, this);
        }
      this->EndWindowLevel();
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

  if (this->Interactor->GetShiftKey())
    {
    this->StartPick();
    if (this->HasObserver(vtkCommand::StartPickEvent)) 
      {
      this->InvokeEvent(vtkCommand::StartPickEvent, this);
      }
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
      if (this->HasObserver(vtkCommand::EndPickEvent)) 
        {
        this->InvokeEvent(vtkCommand::EndPickEvent, this);
        }
      this->EndPick();
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
        if (this->HasObserver(vtkCommand::ResetWindowLevelEvent)) 
          {
          this->InvokeEvent(vtkCommand::ResetWindowLevelEvent, this);
          }
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
  
  if (this->HasObserver(vtkCommand::WindowLevelEvent)) 
    {
    this->InvokeEvent(vtkCommand::WindowLevelEvent, this);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::Pick()
{
  if (this->HasObserver(vtkCommand::PickEvent)) 
    {
    this->InvokeEvent(vtkCommand::PickEvent, this);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "Window Level Current Position: " <<
    this->WindowLevelCurrentPosition << endl;

  os << indent << "Window Level Start Position: " <<
    this->WindowLevelStartPosition << endl;
}
