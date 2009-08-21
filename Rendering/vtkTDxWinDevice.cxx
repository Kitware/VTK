/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTDxWinDevice.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTDxWinDevice.h"

// Most of the code is derived from the SDK with sample code
// Cube3dPolling.cpp from archive Cube3Dpolling.zip from 3DConnexion.

#include <assert.h>

#include "vtkTDxMotionEventInfo.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"

vtkCxxRevisionMacro(vtkTDxWinDevice,"1.1");
vtkStandardNewMacro(vtkTDxWinDevice);

#include "atlbase.h" // for CComPtr<> (a smart pointer)

// for ISensor and IKeyboard
#import "progid:TDxInput.Device.1" no_namespace

VOID CALLBACK vtkTDxWinDeviceTimerProc(HWND hwnd,
                                       UINT uMsg,
                                       UINT_PTR idEvent,
                                       DWORD dwTime);

// It would be better to have the following variables as member of variable
// but the SetTimer on windows is only initialized with a function pointer
// without calldata.

class vtkTDxWinDevicePrivate
{
public:
  vtkTDxWinDevicePrivate()
    {
      this->Sensor=0;
      this->Keyboard=0;
      this->KeyStates=0;
      this->LastTimeStamp=0;
      this->Interactor=0;
    }
  CComPtr<ISensor> Sensor;
  CComPtr<IKeyboard> Keyboard;
  __int64 KeyStates;
  DWORD LastTimeStamp;
  vtkRenderWindowInteractor *Interactor;
};

vtkTDxWinDevicePrivate vtkTDxWinDevicePrivateObject;

// ----------------------------------------------------------------------------
// Description:
// Default constructor.
vtkTDxWinDevice::vtkTDxWinDevice()
{
//  this->DebugOn();
}
  
// ----------------------------------------------------------------------------
// Description:
// Destructor. If the device is not initialized, do nothing. If the device
// is initialized, close the device.
vtkTDxWinDevice::~vtkTDxWinDevice()
{
  if(this->Initialized)
    {
    this->Close();
    }
}
  
// ----------------------------------------------------------------------------
// Description:
// Initialize the device with the current display and window ids.
// It updates the value of GetInitialized().
// Initialization can fail. You must look for the value of
// GetInitialized() before processing further.
// \pre not_yet_initialized: !GetInitialized()
void vtkTDxWinDevice::Initialize()
{
  assert("pre: not_yet_initialized" && !this->GetInitialized());
  
  bool status=false;
  
  HRESULT hr;
  CComPtr<IUnknown> device;

  // Create the device object
  hr=device.CoCreateInstance(__uuidof(Device));
  status=SUCCEEDED(hr);
  if(status)
    {
    CComPtr<ISimpleDevice> d;
    
    hr=device.QueryInterface(&d);
    status=SUCCEEDED(hr);
    if(status)
      {
      vtkTDxWinDevicePrivate *o=&vtkTDxWinDevicePrivateObject;
      
      // Get the interfaces to the sensor and the keyboard;
      o->Sensor=d->Sensor;
      o->Keyboard=d->Keyboard;
      o->Interactor=this->Interactor;
      
      // Connect to the driver
      d->Connect();
      // Create timer used to poll the 3dconnexion device
      this->TimerId =::SetTimer(NULL,0,25,vtkTDxWinDeviceTimerProc);
      vtkDebugMacro(<< "Connected to COM-object for 3dConnexion device.");
      }
    else
      {
      vtkWarningMacro(<<"Could not get the device interface.");
      }
    }
  else
    {
    vtkWarningMacro( << "CoCreateInstance failed");
    }
  this->Initialized=status;
}

// ----------------------------------------------------------------------------
// Description:
// Close the device. This is called by the destructor.
// You don't have to close the device explicitly, as the destructor do it
// automatically, but you can.
// \pre initialized: GetInitialized().
// \post restored: !GetInitialized()
void vtkTDxWinDevice::Close()
{
  assert("pre: initialized" && this->GetInitialized());
  
  vtkDebugMacro(<< "Close()" );
  
  CComPtr<ISimpleDevice> d;
  
  // Kill the timer used to poll the sensor and keyboard
  if(this->TimerId!=0)
    {
    ::KillTimer(NULL,this->TimerId);
    this->TimerId=0;
    }
  
  // Release the sensor and keyboard interfaces
  vtkTDxWinDevicePrivate *o=&vtkTDxWinDevicePrivateObject;
  if(o->Sensor!=0)
    {
    o->Sensor->get_Device(reinterpret_cast<IDispatch**>(&d));
    o->Sensor.Release();
    }
  
  if(o->Keyboard!=0)
    {
    o->Keyboard.Release();
    }
  
  if(d!=0)
    {
    // Disconnect it from the driver
    d->Disconnect();
    d.Release();
    }
  
  this->Initialized=false;
  
  assert("post: restored" && !this->GetInitialized());
}

// ----------------------------------------------------------------------------
void vtkTDxWinDevice::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

// ----------------------------------------------------------------------------
// The timer callback is used to poll the 3d input device for change of
// keystates and the cap displacement values.
VOID CALLBACK vtkTDxWinDeviceTimerProc(HWND vtkNotUsed(hwnd),
                                       UINT vtkNotUsed(uMsg),
                                       UINT_PTR vtkNotUsed(idEvent),
                                       DWORD vtkNotUsed(dwTime))
{ 
  vtkTDxWinDevicePrivate *o=&vtkTDxWinDevicePrivateObject;
  
  if(o->Keyboard!=0)
    {
    // Check if any change to the keyboard state
    try
      {
      long nKeys;
      nKeys = o->Keyboard->Keys;
      long i;
      for (i=1; i<=nKeys; i++)
        {
        __int64 mask = (__int64)1<<(i-1);
        VARIANT_BOOL isPressed;
        isPressed = o->Keyboard->IsKeyDown(i);
        if (isPressed == VARIANT_TRUE)
          {
          if (!(o->KeyStates & mask))
            {
            o->KeyStates |= mask;
            int buttonInfo=static_cast<int>(i);
            o->Interactor->InvokeEvent(vtkCommand::TDxButtonPressEvent,
                                          &buttonInfo);
            }
          }
        else
          {
          o->KeyStates &= ~mask;
          }
        }
      // Test the special keys
      for (i=30; i<=31; i++)
        {
        __int64 mask = (__int64)1<<(i-1);
        VARIANT_BOOL isPressed;
        isPressed = o->Keyboard->IsKeyDown(i);
        if (isPressed == VARIANT_TRUE)
          {
          if (!(o->KeyStates & mask))
            {
            o->KeyStates |= mask;
            int buttonInfo=static_cast<int>(i);
            o->Interactor->InvokeEvent(vtkCommand::TDxButtonPressEvent,
                                          &buttonInfo);
            }
          }
        else
          {
          o->KeyStates &= ~mask;
          }
        }
      }
    catch (...)
      {
      // Some sort of exception handling
      }
    }
  if(o->Sensor!=0)
    {
    try
      {
      CComPtr<IAngleAxis> r=o->Sensor->Rotation;
      CComPtr<IVector3D> t=o->Sensor->Translation;
      
      // On Windows, The angle/axis object is
      // the instant rotation with the vector of rotation+one angle.
      // which is different from the Mac and Unix API...
      
      // Check if the cap is still displaced
      if(r->Angle > 0. || t->Length > 0.)
        {
        double timeFactor=1.0;
        DWORD currentTime=::GetTickCount();
        if(o->LastTimeStamp!=0)
          {
          timeFactor=static_cast<double>(currentTime-o->LastTimeStamp)
            /o->Sensor->Period;
          }
        o->LastTimeStamp=currentTime;
        
        double ScaleRotation=1024.0;
        double ScaleTranslation=512.0;
        double Sensitivity=1.0;
        
        t->Length /= ScaleTranslation*Sensitivity;
        r->Angle /= ScaleRotation*Sensitivity;
        
#if 0
        MathFrame FrameTransRot;
        MathFrameTranslation(&FrameTransRot, pTranslation->X, pTranslation->Y, pTranslation->Z);
        
        
        MathFrameRotation( &FrameTransRot, 
                           pRotation->X * pRotation->Angle, 
                           pRotation->Y * pRotation->Angle,
                           pRotation->Z * pRotation->Angle );
        
        MathFrameMultiplication( &FrameTransRot, &FrameCube, &FrameCube );
        if ( FrameCube.MathTranslation[2] > NearPosition )
          {
          FrameCube.MathTranslation[2] = NearPosition;
          }
        
        if (MainhWnd)
          {
          DisplayCube( MainhWnd, &FrameCube, &VGAVideo );
          }
#endif
        vtkTDxMotionEventInfo motionInfo;
        motionInfo.X=t->X;
        motionInfo.Y=t->Y;
        motionInfo.Z=t->Z;
        motionInfo.A=r->Angle*r->X; // yes, this is wrong, TODO
        motionInfo.B=100.0*r->Angle*r->Y;
        motionInfo.C=r->Angle*r->Z;
        o->Interactor->InvokeEvent(vtkCommand::TDxMotionEvent,
                                   &motionInfo);
        }
      else
        {
        o->LastTimeStamp=0;
        }
      r.Release();
      t.Release();
      }
    catch (...)
      {
      // Some sort of exception handling
      }
    }
}
