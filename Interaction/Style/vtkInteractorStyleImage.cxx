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
#include "vtkPropCollection.h"

#include "vtkCallbackCommand.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkCamera.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkImageSlice.h"
#include "vtkImageMapper3D.h"
#include "vtkImageProperty.h"

vtkStandardNewMacro(vtkInteractorStyleImage);

//----------------------------------------------------------------------------
vtkInteractorStyleImage::vtkInteractorStyleImage()
{
  this->WindowLevelStartPosition[0]   = 0;
  this->WindowLevelStartPosition[1]   = 0;

  this->WindowLevelCurrentPosition[0] = 0;
  this->WindowLevelCurrentPosition[1] = 0;

  this->WindowLevelInitial[0] = 1.0; // Window
  this->WindowLevelInitial[1] = 0.5; // Level

  this->CurrentImageProperty = 0;

  this->InteractionMode = VTKIS_IMAGE2D;

  this->XViewRightVector[0] = 0;
  this->XViewRightVector[1] = 1;
  this->XViewRightVector[2] = 0;

  this->XViewUpVector[0] = 0;
  this->XViewUpVector[1] = 0;
  this->XViewUpVector[2] = -1;

  this->YViewRightVector[0] = 1;
  this->YViewRightVector[1] = 0;
  this->YViewRightVector[2] = 0;

  this->YViewUpVector[0] = 0;
  this->YViewUpVector[1] = 0;
  this->YViewUpVector[2] = -1;

  this->ZViewRightVector[0] = 1;
  this->ZViewRightVector[1] = 0;
  this->ZViewRightVector[2] = 0;

  this->ZViewUpVector[0] = 0;
  this->ZViewUpVector[1] = 1;
  this->ZViewUpVector[2] = 0;
}

//----------------------------------------------------------------------------
vtkInteractorStyleImage::~vtkInteractorStyleImage()
{
  if (this->CurrentImageProperty)
    {
    this->CurrentImageProperty->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::StartWindowLevel()
{
  if (this->State != VTKIS_NONE)
    {
    return;
    }
  this->StartState(VTKIS_WINDOW_LEVEL);

  // Get the last (the topmost) image
  this->SetCurrentImageToNthImage(-1);

  if (this->HandleObservers &&
      this->HasObserver(vtkCommand::StartWindowLevelEvent))
    {
    this->InvokeEvent(vtkCommand::StartWindowLevelEvent, this);
    }
  else
    {
    if (this->CurrentImageProperty)
      {
      vtkImageProperty *property = this->CurrentImageProperty;
      this->WindowLevelInitial[0] = property->GetColorWindow();
      this->WindowLevelInitial[1] = property->GetColorLevel();
      }
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::EndWindowLevel()
{
  if (this->State != VTKIS_WINDOW_LEVEL)
    {
    return;
    }
  if (this->HandleObservers)
    {
    this->InvokeEvent(vtkCommand::EndWindowLevelEvent, this);
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
  if (this->HandleObservers)
    {
    this->InvokeEvent(vtkCommand::StartPickEvent, this);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::EndPick()
{
  if (this->State != VTKIS_PICK)
    {
    return;
    }
  if (this->HandleObservers)
    {
    this->InvokeEvent(vtkCommand::EndPickEvent, this);
    }
  this->StopState();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::StartSlice()
{
  if (this->State != VTKIS_NONE)
    {
    return;
    }
  this->StartState(VTKIS_SLICE);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::EndSlice()
{
  if (this->State != VTKIS_SLICE)
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
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      break;

    case VTKIS_PICK:
      this->FindPokedRenderer(x, y);
      this->Pick();
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      break;

    case VTKIS_SLICE:
      this->FindPokedRenderer(x, y);
      this->Slice();
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

  // If shift is held down, do a rotation
  else if (this->InteractionMode == VTKIS_IMAGE3D &&
           this->Interactor->GetShiftKey())
    {
    this->StartRotate();
    }

  // If ctrl is held down in slicing mode, slice the image
  else if (this->InteractionMode == VTKIS_IMAGE_SLICING &&
           this->Interactor->GetControlKey())
    {
    this->StartSlice();
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

    case VTKIS_SLICE:
      this->EndSlice();
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
void vtkInteractorStyleImage::OnMiddleButtonDown()
{
  this->FindPokedRenderer(this->Interactor->GetEventPosition()[0],
                          this->Interactor->GetEventPosition()[1]);
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  // If shift is held down, change the slice
  if ((this->InteractionMode == VTKIS_IMAGE3D ||
       this->InteractionMode == VTKIS_IMAGE_SLICING) &&
      this->Interactor->GetShiftKey())
    {
    this->StartSlice();
    }

   // The rest of the button + key combinations remain the same

  else
    {
    this->Superclass::OnMiddleButtonDown();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::OnMiddleButtonUp()
{
  switch (this->State)
    {
    case VTKIS_SLICE:
      this->EndSlice();
      if ( this->Interactor )
        {
        this->ReleaseFocus();
        }
      break;
    }

  // Call parent to handle all other states and perform additional work

  this->Superclass::OnMiddleButtonUp();
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

  else if (this->InteractionMode == VTKIS_IMAGE3D &&
           this->Interactor->GetControlKey())
    {
    this->StartSlice();
    }
  else if (this->InteractionMode == VTKIS_IMAGE_SLICING &&
           this->Interactor->GetControlKey())
    {
    this->StartSpin();
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

    case VTKIS_SLICE:
      this->EndSlice();
      if ( this->Interactor )
        {
        this->ReleaseFocus();
        }
      break;

    case VTKIS_SPIN:
      if ( this->Interactor )
        {
        this->EndSpin();
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
      else if (this->HandleObservers &&
               this->HasObserver(vtkCommand::ResetWindowLevelEvent))
        {
        this->InvokeEvent(vtkCommand::ResetWindowLevelEvent, this);
        }
      else if (this->CurrentImageProperty)
        {
        vtkImageProperty *property = this->CurrentImageProperty;
        property->SetColorWindow(this->WindowLevelInitial[0]);
        property->SetColorLevel(this->WindowLevelInitial[1]);
        this->Interactor->Render();
        }
      break;

    case 'x' :
    case 'X' :
      {
      this->SetImageOrientation(this->XViewRightVector, this->XViewUpVector);
      this->Interactor->Render();
      }
      break;

    case 'y' :
    case 'Y' :
      {
      this->SetImageOrientation(this->YViewRightVector, this->YViewUpVector);
      this->Interactor->Render();
      }
      break;

    case 'z' :
    case 'Z' :
      {
      this->SetImageOrientation(this->ZViewRightVector, this->ZViewUpVector);
      this->Interactor->Render();
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

  if (this->CurrentImageProperty)
    {
    int *size = this->CurrentRenderer->GetSize();

    double window = this->WindowLevelInitial[0];
    double level = this->WindowLevelInitial[1];

    // Compute normalized delta

    double dx = (this->WindowLevelCurrentPosition[0] -
                 this->WindowLevelStartPosition[0]) * 4.0 / size[0];
    double dy = (this->WindowLevelStartPosition[1] -
                 this->WindowLevelCurrentPosition[1]) * 4.0 / size[1];

    // Scale by current values

    if ( fabs( window ) > 0.01 )
      {
      dx = dx * window;
      }
    else
      {
      dx = dx * ( window < 0 ? -0.01 : 0.01 );
      }
    if ( fabs( level ) > 0.01 )
      {
      dy = dy * level;
      }
    else
      {
      dy = dy * ( level < 0 ? -0.01 : 0.01 );
      }

    // Abs so that direction does not flip

    if ( window < 0.0 )
      {
      dx = -1 * dx;
      }
    if ( level < 0.0 )
      {
      dy = -1 * dy;
      }

    // Compute new window level

    double newWindow = dx + window;
    double newLevel = level - dy;

    if ( newWindow < 0.01 )
      {
      newWindow = 0.01;
      }

    this->CurrentImageProperty->SetColorWindow(newWindow);
    this->CurrentImageProperty->SetColorLevel(newLevel);

    this->Interactor->Render();
    }
  else
    {
    this->InvokeEvent(vtkCommand::WindowLevelEvent, this);
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::Pick()
{
  this->InvokeEvent(vtkCommand::PickEvent, this);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::Slice()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;
  int dy = rwi->GetEventPosition()[1] - rwi->GetLastEventPosition()[1];

  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  double *range = camera->GetClippingRange();
  double distance = camera->GetDistance();

  // scale the interaction by the height of the viewport
  double viewportHeight = 0.0;
  if (camera->GetParallelProjection())
    {
    viewportHeight = camera->GetParallelScale();
    }
  else
    {
    double angle = vtkMath::RadiansFromDegrees(camera->GetViewAngle());
    viewportHeight = 2.0*distance*tan(0.5*angle);
    }

  int *size = this->CurrentRenderer->GetSize();
  double delta = dy*viewportHeight/size[1];
  distance += delta;

  // clamp the distance to the clipping range
  if (distance < range[0])
    {
    distance = range[0] + viewportHeight*1e-3;
    }
  if (distance > range[1])
    {
    distance = range[1] - viewportHeight*1e-3;
    }
  camera->SetDistance(distance);

  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::SetImageOrientation(
  const double leftToRight[3], const double viewUp[3])
{
  if (this->CurrentRenderer)
    {
    // the cross product points out of the screen
    double vector[3];
    vtkMath::Cross(leftToRight, viewUp, vector);
    double focus[3];
    vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
    camera->GetFocalPoint(focus);
    double d = camera->GetDistance();
    camera->SetPosition(focus[0] + d*vector[0],
                        focus[1] + d*vector[1],
                        focus[2] + d*vector[2]);
    camera->SetFocalPoint(focus);
    camera->SetViewUp(viewUp);
    }
}

//----------------------------------------------------------------------------
// This is a way of dealing with images as if they were layers.
// It looks through the renderer's list of props and sets the
// interactor ivars from the Nth image that it finds.  You can
// also use negative numbers, i.e. -1 will return the last image,
// -2 will return the second-to-last image, etc.
void vtkInteractorStyleImage::SetCurrentImageToNthImage(int i)
{
  if (!this->CurrentRenderer)
    {
    return;
    }

  vtkPropCollection *props = this->CurrentRenderer->GetViewProps();
  vtkProp *prop = 0;
  vtkAssemblyPath *path;
  vtkImageSlice *imageProp = 0;
  vtkCollectionSimpleIterator pit;

  for (int k = 0; k < 2; k++)
    {
    int j = 0;
    for (props->InitTraversal(pit); (prop = props->GetNextProp(pit)); )
      {
      bool foundImageProp = false;
      for (prop->InitPathTraversal(); (path = prop->GetNextPath()); )
        {
        vtkProp *tryProp = path->GetLastNode()->GetViewProp();
        if ( (imageProp = vtkImageSlice::SafeDownCast(tryProp)) != 0 )
          {
          if (j == i)
            {
            foundImageProp = true;
            break;
            }
          imageProp = 0;
          j++;
          }
        }
      if (foundImageProp)
        {
        break;
        }
      }
    if (i < 0)
      {
      i += j;
      }
    }

  vtkImageProperty *property = 0;
  if (imageProp)
    {
    property = imageProp->GetProperty();
    }

  if (property != this->CurrentImageProperty)
    {
    if (this->CurrentImageProperty)
      {
      this->CurrentImageProperty->Delete();
      }

    this->CurrentImageProperty = property;

    if (this->CurrentImageProperty)
      {
      this->CurrentImageProperty->Register(this);
      }
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImage::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Window Level Current Position: ("
     << this->WindowLevelCurrentPosition[0] << ", "
     << this->WindowLevelCurrentPosition[1] << ")\n";

  os << indent << "Window Level Start Position: ("
     << this->WindowLevelStartPosition[0] << ", "
     << this->WindowLevelStartPosition[1] << ")\n";

  os << indent << "Interaction Mode: ";
  if (this->InteractionMode == VTKIS_IMAGE2D)
    {
    os << "Image2D\n";
    }
  else if (this->InteractionMode == VTKIS_IMAGE3D)
    {
    os << "Image3D\n";
    }
  else if (this->InteractionMode == VTKIS_IMAGE_SLICING)
    {
    os << "ImageSlicing\n";
    }
  else
    {
    os << "Unknown\n";
    }

  os << indent << "X View Right Vector: ("
     << this->XViewRightVector[0] << ", "
     << this->XViewRightVector[1] << ", "
     << this->XViewRightVector[2] << ")\n";

  os << indent << "X View Up Vector: ("
     << this->XViewUpVector[0] << ", "
     << this->XViewUpVector[1] << ", "
     << this->XViewUpVector[2] << ")\n";

  os << indent << "Y View Right Vector: ("
     << this->YViewRightVector[0] << ", "
     << this->YViewRightVector[1] << ", "
     << this->YViewRightVector[2] << ")\n";

  os << indent << "Y View Up Vector: ("
     << this->YViewUpVector[0] << ", "
     << this->YViewUpVector[1] << ", "
     << this->YViewUpVector[2] << ")\n";

  os << indent << "Z View Right Vector: ("
     << this->ZViewRightVector[0] << ", "
     << this->ZViewRightVector[1] << ", "
     << this->ZViewRightVector[2] << ")\n";

  os << indent << "Z View Up Vector: ("
     << this->ZViewUpVector[0] << ", "
     << this->ZViewUpVector[1] << ", "
     << this->ZViewUpVector[2] << ")\n";
}
