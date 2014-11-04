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

// On Visual Studio, older than VS9, CoInitializeEx() is not automatically
// defined if the minimum compatibility OS is not specified.
// The Spacenavigator is supported on Windows 2000, XP, Vista
#if defined(_MSC_VER) && (_MSC_VER<1500) // 1500=VS9(2008)
# ifndef _WIN32_WINNT
# define _WIN32_WINNT 0x501 // for CoInitializeEx(), 0x0501 means target Windows XP or later
# endif
#endif

#include "vtkTDxWinDevice.h"

// Most of the code is derived from the SDK with sample code
// Cube3dPolling.cpp from archive Cube3Dpolling.zip from 3DConnexion.

#include <cassert>

#include "vtkTDxMotionEventInfo.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"

#include <map>

vtkStandardNewMacro(vtkTDxWinDevice);

#include <atlbase.h> // for CComPtr<> (a smart pointer)

#include <BaseTsd.h> // for UINT_PTR

// for ISensor and IKeyboard
#import "progid:TDxInput.Device.1" no_namespace

VOID CALLBACK vtkTDxWinDeviceTimerProc(HWND hwnd,
                                       UINT uMsg,
                                       UINT_PTR idEvent,
                                       DWORD dwTime);

class vtkLessThanWindowHandle
{
public:
  bool operator()(const HWND &h1, const HWND &h2) const
    {
      return h1<h2;
    }
};


// It would be better to have the following variables as member of variable
// but the SetTimer on windows is only initialized with a function pointer
// without calldata.
std::map<HWND,vtkTDxWinDevice *,vtkLessThanWindowHandle> vtkWindowHandleToDeviceObject;

std::map<HWND,vtkTDxWinDevice *,vtkLessThanWindowHandle> vtkWindowHandleToDeviceObjectConnection;

class vtkTDxWinDevicePrivate
{
public:
  vtkTDxWinDevicePrivate()
    {
      this->TimerId=0;
      this->Sensor=0;
      this->Keyboard=0;
      this->KeyStates=0;
      this->LastTimeStamp=0;
      this->Interactor=0;
    }
  UINT_PTR TimerId;
  CComPtr<ISensor> Sensor;
  CComPtr<IKeyboard> Keyboard;
  __int64 KeyStates;
  DWORD LastTimeStamp;
  vtkRenderWindowInteractor *Interactor;
};

vtkTDxWinDevicePrivate vtkTDxWinDevicePrivateObject;

HRESULT HresultCodes[13]={S_OK,
                          REGDB_E_CLASSNOTREG,
                          CLASS_E_NOAGGREGATION,
                          E_NOINTERFACE,
                          E_POINTER,
                          E_ABORT,
                          E_ACCESSDENIED,
                          E_FAIL,
                          E_HANDLE,
                          E_INVALIDARG,
                          E_NOTIMPL,
                          E_OUTOFMEMORY,
                          E_UNEXPECTED};

const char *HresultStrings[13]={"S_OK",
                                "REGDB_E_CLASSNOTREG",
                                "CLASS_E_NOAGGREGATION",
                                "E_NOINTERFACE",
                                "E_POINTER",
                                "E_ABORT",
                                "E_ACCESSDENIED",
                                "E_FAIL",
                                "E_HANDLE",
                                "E_INVALIDARG",
                                "E_NOTIMPL",
                                "E_OUTOFMEMORY",
                                "E_UNEXPECTED"};

const UINT_PTR VTK_IDT_TDX_TIMER=1664; // Random beer.

// ----------------------------------------------------------------------------
// Description:
// Return a human readable version of an HRESULT.
const char *HresultCodeToString(HRESULT hr)
{
  int i=0;
  bool found=false;
  while(!found && i<13)
    {
    found=HresultCodes[i]==hr;
    ++i;
    }
  const char *result;
  if(found)
    {
    result=HresultStrings[i-1];
    }
  else
    {
    result="unknown";
    }
  return result;
}

// ----------------------------------------------------------------------------
// Description:
// Default constructor.
vtkTDxWinDevice::vtkTDxWinDevice()
{
  this->WindowHandle=0;
  this->Private=new vtkTDxWinDevicePrivate;
  this->IsListening=false;
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
  delete this->Private;
}

// ----------------------------------------------------------------------------
// Description:
// Get the handle of the window. Initial value is 0.
HWND vtkTDxWinDevice::GetWindowHandle() const
{
  return this->WindowHandle;
}

// ----------------------------------------------------------------------------
// Description:
// Set the handle of the window.
// \pre not_yet_initialized: !GetInitialized()
void vtkTDxWinDevice::SetWindowHandle(HWND hWnd)
{
  assert("pre: not_yet_initialized" && !this->GetInitialized());
  if(this->WindowHandle!=hWnd)
    {
    this->WindowHandle=hWnd;
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
void vtkTDxWinDevice::Initialize()
{
  assert("pre: not_yet_initialized" && !this->GetInitialized());

  bool status=false;

  HRESULT hr=0;
  CComPtr<IUnknown> device;

  bool alreadyConnected=vtkWindowHandleToDeviceObjectConnection.size()!=0;

  if(alreadyConnected)
    {
      // take the first one, to copy the device information.
      std::map<HWND,vtkTDxWinDevice *,vtkLessThanWindowHandle>::iterator it=vtkWindowHandleToDeviceObjectConnection.begin();
      vtkTDxWinDevice *other=(*it).second;
      vtkTDxWinDevicePrivate *o=this->Private;
      vtkTDxWinDevicePrivate *otherO=other->Private;
      o->Sensor=otherO->Sensor;
      o->Keyboard=otherO->Keyboard;
      o->Interactor=this->Interactor;

      vtkWindowHandleToDeviceObjectConnection.insert(std::pair<const HWND,vtkTDxWinDevice *>(this->WindowHandle,this));
      this->Initialized=true;
    }
  else
    {
      hr=::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED );
      if (!SUCCEEDED(hr))
        {
          vtkWarningMacro( << "CoInitializeEx failed. hresult=0x" << hex << hr << dec << HresultCodeToString(hr));
          this->Initialized=status;
          return;
        }
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
              vtkTDxWinDevicePrivate *o= this->Private;

              // Get the interfaces to the sensor and the keyboard;
              o->Sensor=d->Sensor;
              o->Keyboard=d->Keyboard;
              o->Interactor=this->Interactor;

              // Connect to the driver
              d->Connect();

              vtkWindowHandleToDeviceObjectConnection.insert(std::pair<const HWND,vtkTDxWinDevice *>(this->WindowHandle,this));

              vtkDebugMacro(<< "Connected to COM-object for 3dConnexion device.");
            }
          else
            {
              vtkWarningMacro(<<"Could not get the device interface. hresult=0x" << hex << hr << dec << HresultCodeToString(hr));
            }
        }
      else
        {
        // CoCreateInstance Failed.
        // It means there is no driver installed.
        // Just return silently: don't display warning message.
        }
      this->Initialized=status;
    }
}

// ----------------------------------------------------------------------------
bool vtkTDxWinDevice::GetIsListening() const
{
  return this->IsListening;
}

// ----------------------------------------------------------------------------
// \pre initialized: GetInitialized()
// \pre not_yet: !GetIsListening()
void vtkTDxWinDevice::StartListening()
{
  assert("pre: initialized" && this->GetInitialized());
  assert("pre: not_yet" && !this->GetIsListening());

  // Create timer used to poll the 3dconnexion device
  this->Private->TimerId =::SetTimer(this->WindowHandle,VTK_IDT_TDX_TIMER,25,
                                     vtkTDxWinDeviceTimerProc);

  vtkWindowHandleToDeviceObject.insert(
    std::pair<const HWND,vtkTDxWinDevice *>(this->WindowHandle,this));

  this->IsListening=true;

  vtkDebugMacro(<< "Start listening on window="  << this->WindowHandle);
}

// ----------------------------------------------------------------------------
// \pre initialized: GetInitialized()
// \pre is_listening: GetIsListening()
void vtkTDxWinDevice::StopListening()
{
  assert("pre: initialized" && this->GetInitialized());
  assert("pre: is_listening" && this->GetIsListening());

  // Kill the timer used to poll the sensor and keyboard
  ::KillTimer(this->WindowHandle,VTK_IDT_TDX_TIMER);
  this->Private->TimerId=0;
  this->IsListening=false;

  std::map<HWND,vtkTDxWinDevice *,vtkLessThanWindowHandle>::iterator it=vtkWindowHandleToDeviceObject.find(this->WindowHandle);

  if(it==vtkWindowHandleToDeviceObject.end())
    {
    vtkErrorMacro(<< "No matching vtkTDxWinDevice object for window hwnd=" << this->WindowHandle);
    }
  else
    {
    vtkWindowHandleToDeviceObject.erase(it);
    }
  vtkDebugMacro(<< "Stop listening on  window=" << this->WindowHandle);
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

  if(this->IsListening)
    {
    this->StopListening();
    }

  std::map<HWND,vtkTDxWinDevice *,vtkLessThanWindowHandle>::iterator it=vtkWindowHandleToDeviceObjectConnection.find(this->WindowHandle);

  if(it==vtkWindowHandleToDeviceObjectConnection.end())
    {
    vtkErrorMacro(<< "No matching vtkTDxWinDevice object for window hwnd=" << this->WindowHandle);
    }
  else
    {
    vtkWindowHandleToDeviceObjectConnection.erase(it);
    }

  if(vtkWindowHandleToDeviceObjectConnection.size()==0)
    {
      // Release the sensor and keyboard interfaces
      vtkTDxWinDevicePrivate *o=this->Private;
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
void vtkTDxWinDevice::ProcessEvent(void)
{
  vtkTDxWinDevicePrivate *o=this->Private;
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
            vtkDebugMacro(<<"button press event:"<< buttonInfo);
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
            vtkDebugMacro(<<"button press event (special):"<< buttonInfo);
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

        vtkTDxMotionEventInfo motionInfo;
        motionInfo.X=t->X;
        motionInfo.Y=t->Y;
        motionInfo.Z=t->Z;
        motionInfo.Angle=r->Angle;
        motionInfo.AxisX=r->X;
        motionInfo.AxisY=r->Y;
        motionInfo.AxisZ=r->Z;
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

// ----------------------------------------------------------------------------
// The timer callback is used to poll the 3d input device for change of
// keystates and the cap displacement values.
VOID CALLBACK vtkTDxWinDeviceTimerProc(HWND hwnd,
                                       UINT vtkNotUsed(uMsg),
                                       UINT_PTR vtkNotUsed(idEvent),
                                       DWORD vtkNotUsed(dwTime))
{
  std::map<HWND,vtkTDxWinDevice *,vtkLessThanWindowHandle>::iterator it=vtkWindowHandleToDeviceObject.find(hwnd);

  if(it==vtkWindowHandleToDeviceObject.end())
    {
    //vtkGenericWarningMacro(<< "No matching vtkTDxWinDevice object for window hwnd=" << hwnd);
    return;
    }
  vtkTDxWinDevice *device=(*it).second;
  device->ProcessEvent();
}
