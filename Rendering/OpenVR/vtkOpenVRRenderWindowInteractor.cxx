/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenVRRenderWindowInteractor.cxx

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

#include "vtkOpenVRRenderWindowInteractor.h"
#include "vtkOpenVRRenderWindow.h"
#include "vtkRendererCollection.h"


#include "vtkActor.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"
#include "vtkOpenVRCamera.h"
#include "vtkNew.h"
#include "vtkInteractorStyle3D.h"
#include "vtkPropPicker3D.h"

vtkStandardNewMacro(vtkOpenVRRenderWindowInteractor);

void (*vtkOpenVRRenderWindowInteractor::ClassExitMethod)(void *) = (void (*)(void *))NULL;
void *vtkOpenVRRenderWindowInteractor::ClassExitMethodArg = (void *)NULL;
void (*vtkOpenVRRenderWindowInteractor::ClassExitMethodArgDelete)(void *) = (void (*)(void *))NULL;

//----------------------------------------------------------------------------
// Construct object so that light follows camera motion.
vtkOpenVRRenderWindowInteractor::vtkOpenVRRenderWindowInteractor()
{
}

//----------------------------------------------------------------------------
vtkOpenVRRenderWindowInteractor::~vtkOpenVRRenderWindowInteractor()
{
}

void vtkOpenVRRenderWindowInteractor::SetPhysicalTranslation(vtkCamera *camin, double t1, double t2, double t3)
{
  vtkOpenVRCamera *cam =
    static_cast<vtkOpenVRCamera *>(camin);
  cam->SetTranslation(t1,t2,t3);
}

double *vtkOpenVRRenderWindowInteractor::GetPhysicalTranslation(vtkCamera *camin)
{
  vtkOpenVRCamera *cam =
    static_cast<vtkOpenVRCamera *>(camin);
  return cam->GetTranslation();
}

void vtkOpenVRRenderWindowInteractor::ConvertPoseToWorldCoordinates(
  vtkRenderer *ren,
  vr::TrackedDevicePose_t &tdPose,
  double pos[3],
  double wxyz[4])
{
  // get the position and orientation of the button press
  for (int i = 0; i < 3; i++)
  {
    pos[i] = tdPose.mDeviceToAbsoluteTracking.m[i][3];
  }

  vtkOpenVRCamera *cam =
    static_cast<vtkOpenVRCamera *>(ren->GetActiveCamera());
  double distance = cam->GetDistance();
  double *trans = cam->GetTranslation();

  for (int i = 0; i < 3; i++)
  {
    pos[i] = pos[i]*distance - trans[i];
  }

  double ortho[3][3];
  for (int i = 0; i < 3; i++)
  {
    ortho[0][i] = tdPose.mDeviceToAbsoluteTracking.m[0][i];
    ortho[1][i] = tdPose.mDeviceToAbsoluteTracking.m[1][i];
    ortho[2][i] = tdPose.mDeviceToAbsoluteTracking.m[2][i];
  }
  if (vtkMath::Determinant3x3(ortho) < 0)
  {
    ortho[0][2] = -ortho[0][2];
    ortho[1][2] = -ortho[1][2];
    ortho[2][2] = -ortho[2][2];
  }
  vtkMath::Matrix3x3ToQuaternion(ortho, wxyz);

  // calc the return value wxyz
 double mag = sqrt( wxyz[1] * wxyz[1] + wxyz[2] * wxyz[2] + wxyz[3] * wxyz[3] );

  if ( mag != 0.0 )
  {
    wxyz[0] = 2.0 * vtkMath::DegreesFromRadians( atan2( mag, wxyz[0] ) );
    wxyz[1] /= mag;
    wxyz[2] /= mag;
    wxyz[3] /= mag;
  }
  else
  {
    wxyz[0] = 0.0;
    wxyz[1] = 0.0;
    wxyz[2] = 0.0;
    wxyz[3] = 1.0;
  }
}

void vtkOpenVRRenderWindowInteractor::UpdateTouchPadPosition(
  vr::IVRSystem *pHMD,
   vr::TrackedDeviceIndex_t tdi)
{
  vr::VRControllerState_t cstate;
  pHMD->GetControllerState(tdi,&cstate);

  for (unsigned int i = 0; i < vr::k_unControllerStateAxisCount; i++)
  {
    if (pHMD->GetInt32TrackedDeviceProperty(tdi,
      static_cast<vr::ETrackedDeviceProperty>(vr::ETrackedDeviceProperty::Prop_Axis0Type_Int32 + i))
      == vr::EVRControllerAxisType::k_eControllerAxis_TrackPad)
    {
      this->SetTouchPadPosition(cstate.rAxis[i].x,cstate.rAxis[i].y);
    }
  }
}

//----------------------------------------------------------------------------
void  vtkOpenVRRenderWindowInteractor::StartEventLoop()
{
  this->StartedMessageLoop = 1;
  this->Done = false;

  vtkOpenVRRenderWindow *renWin =
    vtkOpenVRRenderWindow::SafeDownCast(this->RenderWindow);

  vtkCollectionSimpleIterator rit;
  renWin->GetRenderers()->InitTraversal(rit);
  vtkRenderer *ren = renWin->GetRenderers()->GetNextRenderer(rit);

  while (!this->Done)
  {
    this->DoOneEvent(renWin, ren);
  }
}

void vtkOpenVRRenderWindowInteractor::DoOneEvent(vtkOpenVRRenderWindow *renWin, vtkRenderer *ren)
{
  vr::IVRSystem *pHMD = renWin->GetHMD();

  if (!pHMD)
  {
    return;
  }

  vr::VREvent_t event;

  bool result =
    pHMD->PollNextEvent(
      &event,
      sizeof(vr::VREvent_t));

  if (result)
  {
    vr::TrackedDeviceIndex_t tdi = event.trackedDeviceIndex;

    // is it a controller button action?
    if (pHMD->GetTrackedDeviceClass(tdi) ==
        vr::ETrackedDeviceClass::TrackedDeviceClass_Controller &&
          (event.eventType == vr::VREvent_ButtonPress ||
           event.eventType == vr::VREvent_ButtonUnpress))
    {
      vr::ETrackedControllerRole role = pHMD->GetControllerRoleForTrackedDeviceIndex(tdi);

      this->UpdateTouchPadPosition(pHMD,tdi);

      // 0 = right hand 1 = left
      int pointerIndex =
        (role == vr::ETrackedControllerRole::TrackedControllerRole_RightHand ? 0 : 1);
      this->PointerIndexLookup[pointerIndex] = tdi;

      vr::TrackedDevicePose_t &tdPose = renWin->GetTrackedDevicePose(tdi);
      double pos[3];
      double wxyz[4];
      this->ConvertPoseToWorldCoordinates(ren, tdPose, pos, wxyz);

      // so even though we have world coordinates we have to convert them to
      // screen coordinates because all of VTKs picking code is currently
      // based on screen coordinates
      ren->SetWorldPoint(pos[0],pos[1],pos[2],1.0);
      ren->WorldToDisplay();
      double *displayCoords = ren->GetDisplayPoint();

      this->SetEventPosition(displayCoords[0],displayCoords[1],pointerIndex);
      this->SetWorldEventPosition(pos[0],pos[1],pos[2],pointerIndex);
      this->SetPhysicalEventPosition(
        tdPose.mDeviceToAbsoluteTracking.m[0][3],
        tdPose.mDeviceToAbsoluteTracking.m[1][3],
        tdPose.mDeviceToAbsoluteTracking.m[2][3],
        pointerIndex);
      this->SetWorldEventOrientation(wxyz[0],wxyz[1],wxyz[2],wxyz[3],pointerIndex);
      this->SetPointerIndex(pointerIndex);

      if (event.eventType == vr::VREvent_ButtonPress)
      {
        if (event.data.controller.button == vr::EVRButtonId::k_EButton_Axis1)
        {
          this->LeftButtonPressEvent();
        }
        if (event.data.controller.button == vr::EVRButtonId::k_EButton_Axis0)
        {
          this->RightButtonPressEvent();
        }
        if (event.data.controller.button == vr::EVRButtonId::k_EButton_ApplicationMenu)
        {
          this->Done = true;
        }
      }
      if (event.eventType == vr::VREvent_ButtonUnpress)
      {
        if (event.data.controller.button == vr::EVRButtonId::k_EButton_Axis1)
        {
          this->LeftButtonReleaseEvent();
        }
        if (event.data.controller.button == vr::EVRButtonId::k_EButton_Axis0)
        {
          this->RightButtonReleaseEvent();
        }
      }
    }
  }
  else
  {
    // if pointers are down track the movement
    if (this->PointersDownCount)
    {
      for (int i = 0; i < VTKI_MAX_POINTERS; i++)
      {
        if (this->PointersDown[i])
        {
          this->UpdateTouchPadPosition(pHMD,
           static_cast<vr::TrackedDeviceIndex_t>(this->PointerIndexLookup[i]));
          vr::TrackedDevicePose_t &tdPose =
            renWin->GetTrackedDevicePose(
            static_cast<vr::TrackedDeviceIndex_t>(this->PointerIndexLookup[i]));
          double pos[3];
          double wxyz[4];
          this->ConvertPoseToWorldCoordinates(ren, tdPose, pos, wxyz);

          // so even though we have world coordinates we have to convert them to
          // screen coordinates because all of VTKs picking code is currently
          // based on screen coordinates
          ren->SetWorldPoint(pos[0],pos[1],pos[2],1.0);
          ren->WorldToDisplay();
          double *displayCoords = ren->GetDisplayPoint();
          this->SetEventPosition(displayCoords[0], displayCoords[1], i);
          this->SetWorldEventPosition(pos[0],pos[1],pos[2],i);
          this->SetWorldEventOrientation(wxyz[0],wxyz[1],wxyz[2],wxyz[3],i);
          this->SetPhysicalEventPosition(
            tdPose.mDeviceToAbsoluteTracking.m[0][3],
            tdPose.mDeviceToAbsoluteTracking.m[1][3],
            tdPose.mDeviceToAbsoluteTracking.m[2][3],
            i);
        }
      }
      this->MouseMoveEvent();
    }
  }
  renWin->Render();
}


//----------------------------------------------------------------------------
void vtkOpenVRRenderWindowInteractor::Initialize()
{
  // make sure we have a RenderWindow and camera
  if ( ! this->RenderWindow)
  {
    vtkErrorMacro(<<"No renderer defined!");
    return;
  }
  if (this->Initialized)
  {
    return;
  }

  vtkOpenVRRenderWindow *ren =
    vtkOpenVRRenderWindow::SafeDownCast(this->RenderWindow);
  int *size;

  this->Initialized = 1;
  // get the info we need from the RenderingWindow

  size = ren->GetSize();
  ren->GetPosition();
  this->Enable();
  this->Size[0] = size[0];
  this->Size[1] = size[1];
}

//----------------------------------------------------------------------------
void vtkOpenVRRenderWindowInteractor::TerminateApp(void)
{
  this->Done = true;
}

//----------------------------------------------------------------------------
int vtkOpenVRRenderWindowInteractor::InternalCreateTimer(
  int vtkNotUsed(timerId),
  int vtkNotUsed(timerType),
  unsigned long vtkNotUsed(duration))
{
  // todo
  return 0;
}

//----------------------------------------------------------------------------
int vtkOpenVRRenderWindowInteractor::InternalDestroyTimer(
  int vtkNotUsed(platformTimerId))
{
  // todo
  return 0;
}


//----------------------------------------------------------------------------
// Specify the default function to be called when an interactor needs to exit.
// This callback is overridden by an instance ExitMethod that is defined.
void
vtkOpenVRRenderWindowInteractor::SetClassExitMethod(void (*f)(void *),void *arg)
{
  if ( f != vtkOpenVRRenderWindowInteractor::ClassExitMethod
       || arg != vtkOpenVRRenderWindowInteractor::ClassExitMethodArg)
  {
    // delete the current arg if there is a delete method
    if ((vtkOpenVRRenderWindowInteractor::ClassExitMethodArg)
        && (vtkOpenVRRenderWindowInteractor::ClassExitMethodArgDelete))
    {
      (*vtkOpenVRRenderWindowInteractor::ClassExitMethodArgDelete)
        (vtkOpenVRRenderWindowInteractor::ClassExitMethodArg);
    }
    vtkOpenVRRenderWindowInteractor::ClassExitMethod = f;
    vtkOpenVRRenderWindowInteractor::ClassExitMethodArg = arg;

    // no call to this->Modified() since this is a class member function
  }
}

//----------------------------------------------------------------------------
// Set the arg delete method.  This is used to free user memory.
void
vtkOpenVRRenderWindowInteractor::SetClassExitMethodArgDelete(void (*f)(void *))
{
  if (f != vtkOpenVRRenderWindowInteractor::ClassExitMethodArgDelete)
  {
    vtkOpenVRRenderWindowInteractor::ClassExitMethodArgDelete = f;

    // no call to this->Modified() since this is a class member function
  }
}

//----------------------------------------------------------------------------
void vtkOpenVRRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "StartedMessageLoop: " << this->StartedMessageLoop << endl;
}

//----------------------------------------------------------------------------
void vtkOpenVRRenderWindowInteractor::ExitCallback()
{
  if (this->HasObserver(vtkCommand::ExitEvent))
  {
    this->InvokeEvent(vtkCommand::ExitEvent,NULL);
  }
  else if (this->ClassExitMethod)
  {
    (*this->ClassExitMethod)(this->ClassExitMethodArg);
  }

  this->TerminateApp();
}
