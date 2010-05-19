/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTDxUnixDevice.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTDxUnixDevice.h"

#include <assert.h>

#include <X11/Xlib.h> // Needed for X types used in the public interface
// Display *DisplayId; // Actually a "Display *" but we cannot include Xlib.h
//  Window WindowId; // Actually a "Window" but we cannot include Xlib.h
  

#define SGI // Used in xdrvlib.h to define ParameterCheck

// xdrvlib.h does not have the usual __cplusplus extern "C" guard
extern "C" {
#include "xdrvlib.h" // Magellan X-Window driver API.
}
  
#include "vtkTDxMotionEventInfo.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkTDxUnixDevice);

// ----------------------------------------------------------------------------
// Description:
// Default constructor. Just set initial values for
// DisplayId (0), WindowId (0)), TranslationScale (1.0),
// RotationScale (1.0).
vtkTDxUnixDevice::vtkTDxUnixDevice()
{
  this->DisplayId=0;
  this->WindowId=0;
  this->TranslationScale=1.0;
  this->RotationScale=1.0;
  this->Interactor=0;
//  this->DebugOn();
}
  
// ----------------------------------------------------------------------------
// Description:
// Destructor. If the device is not initialized, do nothing. If the device
// is initialized, close the device.
vtkTDxUnixDevice::~vtkTDxUnixDevice()
{
  if(this->Initialized)
    {
    this->Close();
    }
}

// ----------------------------------------------------------------------------
// Description:
// Get the ID of the X Display. Initial value is 0.
vtkTDxUnixDeviceDisplay *vtkTDxUnixDevice::GetDisplayId() const
{
  return this->DisplayId;
}
  
// ----------------------------------------------------------------------------
// Description:
// Get the ID of the X Window. Initial value is 0.
vtkTDxUnixDeviceWindow vtkTDxUnixDevice::GetWindowId() const
{
  return this->WindowId;
}

// ----------------------------------------------------------------------------
// Description:
// Set the ID of the X Display.
// \pre not_yet_initialized: !GetInitialized()
void vtkTDxUnixDevice::SetDisplayId(vtkTDxUnixDeviceDisplay *id)
{
  assert("pre: not_yet_initialized" && !this->GetInitialized());
  if(this->DisplayId!=id)
    {
    this->DisplayId=id;
    this->Modified();
    }
}

// ----------------------------------------------------------------------------
// Description:
// Set the ID of the X Window.
// \pre not_yet_initialized: !GetInitialized()
void vtkTDxUnixDevice::SetWindowId(vtkTDxUnixDeviceWindow id)
{
  assert("pre: not_yet_initialized" && !this->GetInitialized());
  if(this->WindowId!=id)
    {
    this->WindowId=id;
    this->Modified();
    }
}
  
// ----------------------------------------------------------------------------
// Description:
// Initialize the device with the current display and window ids.
// It updates the value of GetInitialized().
// Initialization can fail. You must look for the value of
// GetInitialized() before processing further.
// \pre not_yet_initialized: !GetInitialized()
// \pre valid_display: GetDisplayId()!=0
// \pre valid_window: GetWindowId()!=0
// \pre valid_interactor: GetInteractor()!=0
void vtkTDxUnixDevice::Initialize()
{
  assert("pre: not_yet_initialized" && !this->GetInitialized());
  assert("pre: valid_display" && this->GetDisplayId()!=0);
  assert("pre: valid_window" && this->GetWindowId()!=0);
  assert("pre: valid_interactor" && this->GetInteractor()!=0);
  
  int status=MagellanInit(static_cast<Display *>(this->DisplayId),
                          static_cast<Window>(this->WindowId));
  this->Initialized=status==1;
}

// ----------------------------------------------------------------------------
// Description:
// Close the device. This is called by the destructor.
// You don't have to close the device explicitly, as the destructor do it
// automatically, but you can.
// \pre initialized: GetInitialized().
// \post restored: !GetInitialized()
void vtkTDxUnixDevice::Close()
{
  assert("pre: initialized" && this->GetInitialized());
  
  vtkDebugMacro(<< "Close()" );
  MagellanClose(static_cast<Display *>(this->DisplayId));
  this->Initialized=false;
  
  assert("post: restored" && !this->GetInitialized());
}

// ----------------------------------------------------------------------------
// Description:
// Translate the X11 event by invoking a VTK event, if the event came from
// the device.
// Return true if the event passed in argument was effectively an event from
// the device, return false otherwise.
// \pre initialized: GetInitialized()
// \pre e_exists: e!=0
// \pre e_is_client_message: e->type==ClientMessage
bool vtkTDxUnixDevice::ProcessEvent(const vtkTDxUnixDeviceXEvent *e)
{
  assert("pre: initialized" && this->GetInitialized());
  assert("e_exists" && e!=0);
  assert("e_is_client_message" &&
         static_cast<const XEvent *>(e)->type==ClientMessage);
  
  MagellanFloatEvent info;
  
  const XEvent *event=static_cast<const XEvent *>(e);
  
  int deviceEvent=MagellanTranslateEvent(
    static_cast<Display *>(this->DisplayId),
    const_cast<XEvent *>(event),
    &info,
    this->TranslationScale,
    this->RotationScale);
  
  vtkDebugMacro(<< "deviceEvent=" << deviceEvent);
  
  vtkTDxMotionEventInfo motionInfo;
  int buttonInfo;
  double axis[3];
  
  bool result;
  switch(deviceEvent)
    {
    case MagellanInputMotionEvent:
      vtkDebugMacro(<< "it is MagellanInputMotionEvent");
      MagellanRemoveMotionEvents(static_cast<Display *>(this->DisplayId));
      motionInfo.X=info.MagellanData[MagellanX];
      motionInfo.Y=info.MagellanData[MagellanY];
      
      // On Unix, the Z axis is reversed (wrong). We want to have a
      // right-handed coordinate system, so positive Z has to come towards us,
      // as on Windows.
      motionInfo.Z=-info.MagellanData[MagellanZ];
      
      axis[0]=info.MagellanData[MagellanA];
      axis[1]=info.MagellanData[MagellanB];
      
      // On Unix, the Z axis is reserved (wrong).
      axis[2]=-info.MagellanData[MagellanC];
      
      motionInfo.Angle=vtkMath::Norm(axis);
      if(motionInfo.Angle==0.0)
        {
        motionInfo.AxisX=0.0;
        motionInfo.AxisY=0.0;
        motionInfo.AxisZ=1.0;
        }
      else
        {
        motionInfo.AxisX=axis[0]/motionInfo.Angle;
        motionInfo.AxisY=axis[1]/motionInfo.Angle;
        motionInfo.AxisZ=axis[2]/motionInfo.Angle;
        }
      
      this->Interactor->InvokeEvent(vtkCommand::TDxMotionEvent,&motionInfo);
      result=true;
      break;
    case MagellanInputButtonPressEvent:
      vtkDebugMacro(<< "it is  MagellanInputButtonPressEvent");
      buttonInfo=info.MagellanButton;
      this->Interactor->InvokeEvent(vtkCommand::TDxButtonPressEvent,
                                    &buttonInfo);
      result=true;
      break;
    case MagellanInputButtonReleaseEvent:
      vtkDebugMacro(<< "it is  MagellanInputButtonReleaseEvent");
      buttonInfo=info.MagellanButton;
      this->Interactor->InvokeEvent(vtkCommand::TDxButtonReleaseEvent,
                                    &buttonInfo);
      result=true;
      break;
    default:
      vtkDebugMacro(<< "it is not a Magellan event");
      result=false;
      break;
    }
  
  return result;
}

// ----------------------------------------------------------------------------
// Description:
// Set the sensitivity of the device for the current application.
// A neutral value is 1.0.
// \pre initialized: GetInitialized()
void vtkTDxUnixDevice::SetSensitivity(double sensitivity)
{
  assert("pre: initialized" && this->GetInitialized());
  
  MagellanApplicationSensitivity(static_cast<Display *>(this->DisplayId),
                                 sensitivity);
}

// ----------------------------------------------------------------------------
void vtkTDxUnixDevice::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "RotationScale: " << this->RotationScale << endl;
  os << indent << "TranslationScale: " << this->TranslationScale << endl;
}
