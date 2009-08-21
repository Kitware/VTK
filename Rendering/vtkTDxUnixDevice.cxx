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

#define SGI // Used in xdrvlib.h to define ParameterCheck

// xdrvlib.h does not have the usual __cplusplus extern "C" guard
extern "C" {
#include "xdrvlib.h" // Magellan X-Window driver API.
}
  
#include "vtkTDxMotionEventInfo.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"

vtkCxxRevisionMacro(vtkTDxUnixDevice,"1.1");
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
Display *vtkTDxUnixDevice::GetDisplayId() const
{
  return this->DisplayId;
}
  
// ----------------------------------------------------------------------------
// Description:
// Get the ID of the X Window. Initial value is 0.
Window vtkTDxUnixDevice::GetWindowId() const
{
  return this->WindowId;
}

// ----------------------------------------------------------------------------
// Description:
// Set the ID of the X Display.
// \pre not_yet_initialized: !GetInitialized()
void vtkTDxUnixDevice::SetDisplayId(Display *id)
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
void vtkTDxUnixDevice::SetWindowId(Window id)
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
  
  int status=MagellanInit(this->DisplayId,this->WindowId);
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
  MagellanClose(this->DisplayId);
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
bool vtkTDxUnixDevice::ProcessEvent(const XEvent *e)
{
  assert("pre: initialized" && this->GetInitialized());
  assert("e_exists" && e!=0);
  assert("e_is_client_message" && e->type==ClientMessage);
  
  MagellanFloatEvent info;
  
  int deviceEvent=MagellanTranslateEvent(this->DisplayId,
                                         const_cast<XEvent *>(e),
                                         &info,
                                         this->TranslationScale,
                                         this->RotationScale);
  
  vtkDebugMacro(<< "deviceEvent=" << deviceEvent);
  
  vtkTDxMotionEventInfo motionInfo;
  int buttonInfo;
  
  bool result;
  switch(deviceEvent)
    {
    case MagellanInputMotionEvent:
      vtkDebugMacro(<< "it is MagellanInputMotionEvent");
      MagellanRemoveMotionEvents(this->DisplayId);
      motionInfo.X=info.MagellanData[MagellanX];
      motionInfo.Y=info.MagellanData[MagellanY];
      motionInfo.Z=info.MagellanData[MagellanZ];
      motionInfo.A=info.MagellanData[MagellanA];
      motionInfo.B=info.MagellanData[MagellanB];
      motionInfo.C=info.MagellanData[MagellanC];
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
  
  MagellanApplicationSensitivity(this->DisplayId,sensitivity);
}

// ----------------------------------------------------------------------------
void vtkTDxUnixDevice::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
