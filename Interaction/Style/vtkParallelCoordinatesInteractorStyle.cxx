/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkParallelCoordinatesInteractorStyle.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
#include "vtkParallelCoordinatesInteractorStyle.h"

#include "vtkAbstractPropPicker.h"
#include "vtkAssemblyPath.h"
#include "vtkCallbackCommand.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkViewport.h"

//#include <wx/wx.h>

vtkStandardNewMacro(vtkParallelCoordinatesInteractorStyle);

//----------------------------------------------------------------------------
vtkParallelCoordinatesInteractorStyle::vtkParallelCoordinatesInteractorStyle()
{
  this->CursorStartPosition[0]   = 0;
  this->CursorStartPosition[1]   = 0;

  this->CursorCurrentPosition[0] = 0;
  this->CursorCurrentPosition[1] = 0;

  this->CursorLastPosition[0] = 0;
  this->CursorLastPosition[1] = 0;

  this->State = INTERACT_HOVER;
}

//----------------------------------------------------------------------------
vtkParallelCoordinatesInteractorStyle::~vtkParallelCoordinatesInteractorStyle()
{
}
//----------------------------------------------------------------------------
void vtkParallelCoordinatesInteractorStyle::OnMouseMove()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  this->FindPokedRenderer(x, y);

  this->CursorLastPosition[0] = this->CursorCurrentPosition[0];
  this->CursorLastPosition[1] = this->CursorCurrentPosition[1];
  this->CursorCurrentPosition[0] = x;
  this->CursorCurrentPosition[1] = y;

  switch (this->State)
  {
    case INTERACT_HOVER:
      this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
      break;
    case INTERACT_INSPECT:
      this->Inspect(x,y);
      break;
    case INTERACT_ZOOM:
      this->Zoom();
      break;
    case INTERACT_PAN:
      this->Pan();
      break;
    default:
      // Call parent to handle all other states and perform additional work
      // This seems like it should always be called, but it's creating duplicate
      // interaction events to get invoked.
      this->Superclass::OnMouseMove();
      break;
  }


}

//----------------------------------------------------------------------------
void vtkParallelCoordinatesInteractorStyle::OnLeftButtonDown()
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

  if (!this->Interactor->GetShiftKey() &&
      !this->Interactor->GetControlKey())
  {
    this->CursorStartPosition[0] = x;
    this->CursorStartPosition[1] = y;
    this->CursorLastPosition[0] = x;
    this->CursorLastPosition[1] = y;
    this->CursorCurrentPosition[0] = x;
    this->CursorCurrentPosition[1] = y;
    this->StartInspect(x,y);//ManipulateAxes(x,y);
  }
/*  else if (!this->Interactor->GetControlKey())
    {
    this->CursorStartPosition[0] = x;
    this->CursorStartPosition[1] = y;
    this->CursorLastPosition[0] = x;
    this->CursorLastPosition[1] = y;
    this->CursorCurrentPosition[0] = x;
    this->CursorCurrentPosition[1] = y;
    this->StartSelectData(x,y);
    }*/
  else // The rest of the button + key combinations remain the same
  {
    this->Superclass::OnLeftButtonDown();
  }
}

//----------------------------------------------------------------------------
void vtkParallelCoordinatesInteractorStyle::OnLeftButtonUp()
{
  if (this->State == INTERACT_INSPECT)
  {
    this->EndInspect();//ManipulateAxes();

    if ( this->Interactor )
    {
      this->ReleaseFocus();
    }
  }
/*  else if (this->State == INTERACT_SELECT_DATA)
    {
    this->EndSelectData();

    if ( this->Interactor )
    {
    this->ReleaseFocus();
    }
    } */

  // Call parent to handle all other states and perform additional work
  this->Superclass::OnLeftButtonUp();
}

//----------------------------------------------------------------------------
void vtkParallelCoordinatesInteractorStyle::OnMiddleButtonDown()
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

  if (!this->Interactor->GetShiftKey() &&
      !this->Interactor->GetControlKey())
  {
    this->CursorStartPosition[0] = x;
    this->CursorStartPosition[1] = y;
    this->CursorLastPosition[0] = x;
    this->CursorLastPosition[1] = y;
    this->CursorCurrentPosition[0] = x;
    this->CursorCurrentPosition[1] = y;
    this->StartPan();
  }
  // The rest of the button + key combinations remain the same
  else
  {
    this->Superclass::OnMiddleButtonDown();
  }
}

//----------------------------------------------------------------------------
void vtkParallelCoordinatesInteractorStyle::OnMiddleButtonUp()
{
  if (this->State == INTERACT_PAN)
  {
    this->EndPan();
    if ( this->Interactor )
    {
      this->ReleaseFocus();
    }
  }

  // Call parent to handle all other states and perform additional work

  this->Superclass::OnMiddleButtonUp();
}

//----------------------------------------------------------------------------
void vtkParallelCoordinatesInteractorStyle::OnRightButtonDown()
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

  if (!this->Interactor->GetShiftKey() &&
      !this->Interactor->GetControlKey())
  {
    this->CursorStartPosition[0] = x;
    this->CursorStartPosition[1] = y;
    this->CursorLastPosition[0] = x;
    this->CursorLastPosition[1] = y;
    this->CursorCurrentPosition[0] = x;
    this->CursorCurrentPosition[1] = y;
    this->StartZoom();
  }
  // The rest of the button + key combinations remain the same
  else
  {
    this->Superclass::OnRightButtonDown();
  }
}

//----------------------------------------------------------------------------
void vtkParallelCoordinatesInteractorStyle::OnRightButtonUp()
{
  if (this->State == INTERACT_ZOOM)
  {
    this->EndZoom();
    if ( this->Interactor )
    {
      this->ReleaseFocus();
    }
  }

  // Call parent to handle all other states and perform additional work

  this->Superclass::OnRightButtonUp();
}

//----------------------------------------------------------------------------
void vtkParallelCoordinatesInteractorStyle::OnLeave()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  this->FindPokedRenderer(x, y);

  this->CursorLastPosition[0] = this->CursorCurrentPosition[0];
  this->CursorLastPosition[1] = this->CursorCurrentPosition[1];
  this->CursorCurrentPosition[0] = x;
  this->CursorCurrentPosition[1] = y;

  switch (this->State)
  {
    case INTERACT_HOVER:
      this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
      break;
    case INTERACT_INSPECT:
      this->Inspect(x,y);
      break;
    case INTERACT_ZOOM:
      this->Zoom();
      break;
    case INTERACT_PAN:
      this->Pan();
      break;
    default:
      // Call parent to handle all other states and perform additional work
      // This seems like it should always be called, but it's creating duplicate
      // interaction events to get invoked.
      this->Superclass::OnLeave();
      break;
  }
}

//----------------------------------------------------------------------------
void vtkParallelCoordinatesInteractorStyle::OnChar()
{
  vtkRenderWindowInteractor *rwi = this->Interactor;

  switch (rwi->GetKeyCode())
  {
    case 'f' :
    case 'F' :
      break;

    case 'r' :
    case 'R' :
      this->InvokeEvent(vtkCommand::UpdateEvent,NULL);
      break;

    default:
      this->Superclass::OnChar();
      break;
  }
}

//----------------------------------------------------------------------------
void vtkParallelCoordinatesInteractorStyle::StartInspect(int vtkNotUsed(x), int vtkNotUsed(y))
{
  this->State = INTERACT_INSPECT;
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
}

//----------------------------------------------------------------------------
void vtkParallelCoordinatesInteractorStyle::Inspect(int vtkNotUsed(x), int vtkNotUsed(y))
{
  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
}

//----------------------------------------------------------------------------
void vtkParallelCoordinatesInteractorStyle::EndInspect()
{
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->State = INTERACT_HOVER;
}

//----------------------------------------------------------------------------
void vtkParallelCoordinatesInteractorStyle::StartZoom()
{
  this->State = INTERACT_ZOOM;
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);

}

//----------------------------------------------------------------------------
void vtkParallelCoordinatesInteractorStyle::Zoom()
{
  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
}

//----------------------------------------------------------------------------
void vtkParallelCoordinatesInteractorStyle::EndZoom()
{
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->State = INTERACT_HOVER;
}

//----------------------------------------------------------------------------
void vtkParallelCoordinatesInteractorStyle::StartPan()
{
  this->State = INTERACT_PAN;
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);

}

//----------------------------------------------------------------------------
void vtkParallelCoordinatesInteractorStyle::Pan()
{
  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
}

//----------------------------------------------------------------------------
void vtkParallelCoordinatesInteractorStyle::EndPan()
{
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->State = INTERACT_HOVER;
}

//----------------------------------------------------------------------------
void vtkParallelCoordinatesInteractorStyle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Cursor Current Position: ("
     << this->CursorCurrentPosition[0] << ", "
     << this->CursorCurrentPosition[1] << ")" << endl;

  os << indent << "Cursor Start Position: ("
     << this->CursorStartPosition[0] << ", "
     << this->CursorStartPosition[1] << ")" << endl;

  os << indent << "Cursor Last Position: ("
     << this->CursorLastPosition[0] << ", "
     << this->CursorLastPosition[1] << ")" << endl;
}
void vtkParallelCoordinatesInteractorStyle::GetCursorStartPosition(vtkViewport *viewport, double pos[2])
{
  int *size = viewport->GetSize();
  pos[0] = static_cast<double>(this->CursorStartPosition[0]) / size[0];
  pos[1] = static_cast<double>(this->CursorStartPosition[1]) / size[1];
}

void vtkParallelCoordinatesInteractorStyle::GetCursorCurrentPosition(vtkViewport *viewport, double pos[2])
{
  int *size = viewport->GetSize();
  pos[0] = static_cast<double>(this->CursorCurrentPosition[0]) / size[0];
  pos[1] = static_cast<double>(this->CursorCurrentPosition[1]) / size[1];
}

void vtkParallelCoordinatesInteractorStyle::GetCursorLastPosition(vtkViewport *viewport, double pos[2])
{
  int *size = viewport->GetSize();
  pos[0] = static_cast<double>(this->CursorLastPosition[0]) / size[0];
  pos[1] = static_cast<double>(this->CursorLastPosition[1]) / size[1];
}
