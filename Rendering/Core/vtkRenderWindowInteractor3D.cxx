/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderWindowInteractor3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>

#include "vtkRenderWindowInteractor3D.h"
#include "vtkRendererCollection.h"
#include "vtkInteractorStyle3D.h"

#include "vtkActor.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkPropPicker3D.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkRenderWindowInteractor3D);

//----------------------------------------------------------------------------
// Construct object so that light follows camera motion.
vtkRenderWindowInteractor3D::vtkRenderWindowInteractor3D()
{
  this->MouseInWindow = 0;
  this->StartedMessageLoop = 0;
  vtkNew<vtkInteractorStyle3D> style;
  this->SetInteractorStyle(style.Get());
  this->Done = false;
}

//----------------------------------------------------------------------------
vtkRenderWindowInteractor3D::~vtkRenderWindowInteractor3D()
{
}

//----------------------------------------------------------------------
// Creates an instance of vtkPropPicker by default
vtkAbstractPropPicker *vtkRenderWindowInteractor3D::CreateDefaultPicker()
{
  return vtkPropPicker3D::New();
}

//----------------------------------------------------------------------------
void vtkRenderWindowInteractor3D::Enable()
{
  if (this->Enabled)
  {
    return;
  }
  this->Enabled = 1;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkRenderWindowInteractor3D::Disable()
{
  if (!this->Enabled)
  {
    return;
  }

  this->Enabled = 0;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkRenderWindowInteractor3D::TerminateApp(void)
{
  this->Done = true;
}

//----------------------------------------------------------------------------
void vtkRenderWindowInteractor3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "StartedMessageLoop: " << this->StartedMessageLoop << endl;
}

//----------------------------------------------------------------------------
void vtkRenderWindowInteractor3D::SetTranslation3D(double val[3])
{
  this->LastTranslation3D[0] = this->Translation3D[0];
  this->LastTranslation3D[1] = this->Translation3D[1];
  this->LastTranslation3D[2] = this->Translation3D[2];
  if (this->Translation3D[0] != val[0] ||
      this->Translation3D[1] != val[1] ||
      this->Translation3D[2] != val[2])
  {
    this->Translation3D[0] = val[0];
    this->Translation3D[1] = val[1];
    this->Translation3D[2] = val[2];
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkRenderWindowInteractor3D::RecognizeGesture(vtkCommand::EventIds event)
{
  // we know we are in multitouch now, so start recognizing

  // more than two pointers we ignore
  if (this->PointersDownCount > 2)
  {
    return;
  }

  // store the initial positions
  if (event == vtkCommand::LeftButtonPressEvent)
  {
    for (int i = 0; i < VTKI_MAX_POINTERS; i++)
    {
      if (this->PointersDown[i])
      {
        this->StartingPhysicalEventPositions[i][0] =
          this->PhysicalEventPositions[i][0];
        this->StartingPhysicalEventPositions[i][1] =
          this->PhysicalEventPositions[i][1];
        this->StartingPhysicalEventPositions[i][2] =
          this->PhysicalEventPositions[i][2];
      }
    }
    // we do not know what the gesture is yet
    this->CurrentGesture = vtkCommand::StartEvent;
    return;
  }

  // end the gesture if needed
  if (event == vtkCommand::LeftButtonReleaseEvent)
  {
    if (this->CurrentGesture == vtkCommand::PinchEvent)
    {
      this->EndPinchEvent();
    }
    if (this->CurrentGesture == vtkCommand::PanEvent)
    {
      this->EndPanEvent();
    }
    this->CurrentGesture = vtkCommand::StartEvent;
    return;
  }

  // what are the two pointers we are working with
  int count = 0;
  double *posVals[2];
  double *startVals[2];
  for (int i = 0; i < VTKI_MAX_POINTERS; i++)
  {
    if (this->PointersDown[i])
    {
      posVals[count] = this->PhysicalEventPositions[i];
      startVals[count] = this->StartingPhysicalEventPositions[i];
      count++;
    }
  }

  // The meat of the algorithm
  // on move events we analyze them to determine what type
  // of movement it is and then deal with it.
  if (event == vtkCommand::MouseMoveEvent)
  {
    // calculate the distances
    double originalDistance = sqrt(
      vtkMath::Distance2BetweenPoints(startVals[0], startVals[1]));
    double newDistance = sqrt(
      vtkMath::Distance2BetweenPoints(posVals[0], posVals[1]));

    // calculate the translations
    double trans[3];
      trans[0] = (posVals[0][0] - startVals[0][0] + posVals[1][0] - startVals[1][0])/2.0;
      trans[1] = (posVals[0][1] - startVals[0][1] + posVals[1][1] - startVals[1][1])/2.0;
      trans[2] = (posVals[0][2] - startVals[0][2] + posVals[1][2] - startVals[1][2])/2.0;

    // OK we want to
    // - immediately respond to the user
    // - allow the user to zoom without panning (saves focal point)
    // - allow the user to rotate without panning (saves focal point)

    // do we know what gesture we are doing yet? If not
    // see if we can figure it out
    if (this->CurrentGesture == vtkCommand::StartEvent)
    {
      // pinch is a move to/from the center point
      // rotate is a move along the circumference
      // pan is a move of the center point
      // compute the distance along each of these axes in meters
      // the first to break thresh wins
      double thresh = 0.05; // in meters

      double pinchDistance = fabs(newDistance - originalDistance);
      double panDistance = sqrt(trans[0]*trans[0] + trans[1]*trans[1] + trans[2]*trans[2]);
      if (pinchDistance > thresh
          && pinchDistance > panDistance)
      {
        this->CurrentGesture = vtkCommand::PinchEvent;
        this->Scale = 1.0;
        this->StartPinchEvent();
      }
      else if (panDistance > thresh)
      {
        this->CurrentGesture = vtkCommand::PanEvent;
        this->Translation3D[0] = 0.0;
        this->Translation3D[1] = 0.0;
        this->Translation3D[2] = 0.0;
        this->StartPanEvent();
      }
    }

    // if we have found a specific type of movement then
    // handle it
    if (this->CurrentGesture == vtkCommand::PinchEvent)
    {
      this->SetScale(newDistance/originalDistance);
      this->PinchEvent();
    }

    if (this->CurrentGesture == vtkCommand::PanEvent)
    {
      this->SetTranslation3D(trans);
      this->PanEvent();
    }

  }

}

//------------------------------------------------------------------
void vtkRenderWindowInteractor3D::RightButtonPressEvent()
{
  if (!this->Enabled)
  {
    return;
  }

  // are we translating multitouch into gestures?
  if (this->RecognizeGestures)
  {
    if (!this->PointersDown[this->PointerIndex])
    {
      this->PointersDown[this->PointerIndex] = 1;
      this->PointersDownCount++;
    }
    // do we have multitouch
    if (this->PointersDownCount > 1)
    {
      // did we just transition to multitouch?
      if (this->PointersDownCount == 2)
      {
        this->InvokeEvent(vtkCommand::RightButtonReleaseEvent, NULL);
      }
      // handle the gesture
      this->RecognizeGesture(vtkCommand::RightButtonPressEvent);
      return;
    }
  }

  this->InvokeEvent(vtkCommand::RightButtonPressEvent, NULL);
}

//------------------------------------------------------------------
void vtkRenderWindowInteractor3D::RightButtonReleaseEvent()
{
  if (!this->Enabled)
  {
    return;
  }

  if (this->RecognizeGestures)
  {
    if (this->PointersDown[this->PointerIndex])
    {
      this->PointersDown[this->PointerIndex] = 0;
      this->PointersDownCount--;
    }
    // do we have multitouch
    if (this->PointersDownCount > 1)
    {
      // handle the gesture
      this->RecognizeGesture(vtkCommand::RightButtonReleaseEvent);
      return;
    }
  }
  this->InvokeEvent(vtkCommand::RightButtonReleaseEvent, NULL);
}
