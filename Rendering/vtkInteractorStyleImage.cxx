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

vtkCxxRevisionMacro(vtkInteractorStyleImage, "1.17");
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
void vtkInteractorStyleImage::OnMouseMove(int ctrl, 
                                          int shift,
                                          int x, 
                                          int y) 
{
  switch (this->State) 
    {
    case VTKIS_WINDOW_LEVEL:
      this->FindPokedCamera(x, y);
      this->WindowLevel();
      break;

    case VTKIS_PICK:
      this->FindPokedCamera(x, y);
      this->Pick();
      break;
    }

  // Call parent to handle all other states and perform additional work

  this->Superclass::OnMouseMove(ctrl, shift, x, y);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::OnLeftButtonDown(int ctrl, 
                                               int shift, 
                                               int x, 
                                               int y) 
{
  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }
  
  // Redefine this button to handle window/level

  if (!shift && !ctrl) 
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
    this->Superclass::OnLeftButtonDown(ctrl, shift, x, y);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::OnLeftButtonUp(int ctrl, 
                                             int shift, 
                                             int x, 
                                             int y)
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

  this->Superclass::OnLeftButtonUp(ctrl, shift, x, y);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::OnRightButtonDown(int ctrl, 
                                                int shift, 
                                                int x, 
                                                int y) 
{
  this->FindPokedRenderer(x, y);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  // Redefine this button + shift to handle pick

  if (shift)
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
    this->Superclass::OnRightButtonDown(ctrl, shift, x, y);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::OnRightButtonUp(int ctrl, 
                                              int shift, 
                                              int x, 
                                              int y) 
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

  this->Superclass::OnRightButtonUp(ctrl, shift, x, y);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::OnChar(int ctrl, 
                                     int shift, 
                                     char keycode, 
                                     int repeatcount) 
{
  vtkRenderWindowInteractor *rwi = this->Interactor;

  switch (keycode) 
    {
    case 'f' :      
    case 'F' :
      {
      this->AnimState = VTKIS_ANIM_ON;
      vtkAssemblyPath *path=NULL;
      this->FindPokedRenderer(this->LastPos[0],this->LastPos[1]);
      rwi->GetPicker()->Pick(this->LastPos[0],this->LastPos[1], 0.0, 
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
      if (shift || ctrl)
        {
        this->Superclass::OnChar(ctrl, shift, keycode, repeatcount);
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
      this->Superclass::OnChar(ctrl, shift, keycode, repeatcount);
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
