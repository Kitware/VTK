/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTDxMacDevice.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTDxMacDevice.h"

#include <assert.h>

#include "vtkTDxMotionEventInfo.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkMath.h"

#include <map>
#include <cstring> // for strlen()

vtkStandardNewMacro(vtkTDxMacDevice);

void vtkTDxMacDeviceMessageHandler(io_connect_t connection,
                                   natural_t messageType,
                                   void *messageArgument);

class vtkLessThanClientID
{
public:
  bool operator()(const UInt16 &id1, const UInt16 &id2) const
    {
      return id1<id2;
    }
};


// It would be better to have the following variables as member of variable
// but the message handle registration through the 3DConnexion SDK is a piece
// of crap: it only takes a callback pointer but no callback data!
std::map<UInt16,vtkTDxMacDevice *,vtkLessThanClientID> vtkClientIDToDeviceObject;

// ----------------------------------------------------------------------------
// Description:
// Default constructor.
vtkTDxMacDevice::vtkTDxMacDevice()
{
  this->ClientApplicationName=0;
  this->SetClientApplicationName("3DxClientTest");
  this->ClientID=0;
  this->LastButtonState=0; // all buttons released.
//  this->DebugOn();
}
  
// ----------------------------------------------------------------------------
// Description:
// Destructor. If the device is not initialized, do nothing. If the device
// is initialized, close the device.
vtkTDxMacDevice::~vtkTDxMacDevice()
{
  if(this->Initialized)
    {
    this->Close();
    }
  this->SetClientApplicationName(0);
}

// ----------------------------------------------------------------------------
// Description:
// Initialize the device with the current display and window ids.
// It updates the value of GetInitialized().
// Initialization can fail. You must look for the value of
// GetInitialized() before processing further.
// \pre not_yet_initialized: !GetInitialized()
// \pre valid_name: GetClientApplicationName()!=0
void vtkTDxMacDevice::Initialize()
{
  assert("pre: not_yet_initialized" && !this->GetInitialized());
  assert("pre: valid_name" && this->GetClientApplicationName()!=0);

  if(vtkClientIDToDeviceObject.size()==0)
    {
    OSErr result=InstallConnexionHandlers(vtkTDxMacDeviceMessageHandler,0L,0L);
    this->Initialized=result==noErr; // 0
    }
  else
    {
    this->Initialized=true;
    }
  
  this->LastButtonState=0; // all buttons released.
  
  unsigned char *pString=
    this->CStringToPascalString(this->ClientApplicationName);
  
  // this does not work, we have to use kConnexionClientWildcard and '\0'
  // this->ClientID=RegisterConnexionClient('MCTt',pString,kConnexionClientModeTakeOver,kConnexionMaskAll);
  
  this->ClientID=RegisterConnexionClient(kConnexionClientWildcard,'\0',
                                          kConnexionClientModeTakeOver,
                                          kConnexionMaskAll);
  
  vtkDebugMacro(<< "Registered with ClientID=" << this->ClientID);
  
  delete[] pString;
  
  vtkClientIDToDeviceObject.insert(std::pair<const UInt16,vtkTDxMacDevice *>(this->ClientID,this));
}

// ----------------------------------------------------------------------------
// Description:
// Close the device. This is called by the destructor.
// You don't have to close the device explicitly, as the destructor do it
// automatically, but you can.
// \pre initialized: GetInitialized().
// \post restored: !GetInitialized()
void vtkTDxMacDevice::Close()
{
  assert("pre: initialized" && this->GetInitialized());
  
  vtkDebugMacro(<< "Close()" );
  UnregisterConnexionClient(this->ClientID);
  
  // remove it from map
  std::map<UInt16,vtkTDxMacDevice *,vtkLessThanClientID>::iterator it=
    vtkClientIDToDeviceObject.find(this->ClientID);
  
  if(it==vtkClientIDToDeviceObject.end())
    {
    vtkErrorMacro(<< "No matching vtkTDxMacDevice object for clientID=" << this->ClientID);
    }
  else
    {
    vtkClientIDToDeviceObject.erase(it);
    }

  // only if the map is empty.
  if(vtkClientIDToDeviceObject.size()==0)
    {
    CleanupConnexionHandlers();
    }
  
  this->ClientID=0;
  this->LastButtonState=0; // all buttons released.
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
// \pre s_exists: s!=0
// \pre client_matches: s->client==this->ClientID
void vtkTDxMacDevice::ProcessEvent(const ConnexionDeviceState *s)
{
  assert("pre: initialized" && this->GetInitialized());
  assert("pre: s_exists" && s!=0);
  assert("pre: client_matches" && s->client==this->ClientID);
  
  vtkTDxMotionEventInfo motionInfo;
  int buttonInfo;
  double axis[3];
  UInt16 mask;
  bool pressed;
  
  switch(s->command)
    {
    case kConnexionCmdHandleAxis:
      vtkDebugMacro(<< "it is kConnexionCmdHandleAxis");
      motionInfo.X=s->axis[0]; // Tx: SInt16 between -1024 and 1024
      
      // On Mac, the Y and Z axes are reversed (wrong). We want to have a
      // right-handed coordinate system, so positive Y has to go from bottom to
      // top and positive Z has to come towards us, as on Windows.
      
      motionInfo.Y=-s->axis[1]; // Ty: SInt16 between -1024 and 1024
      motionInfo.Z=-s->axis[2]; // Tz: SInt16 between -1024 and 1014
      
      axis[0]=s->axis[3]; // Rx: SInt16 between -1024 and 1024
      
      // On Mac, the Y and Z axes are reversed (wrong).
      axis[1]=-s->axis[4]; // Ry: SInt16 between -1024 and 1024
      axis[2]=-s->axis[5]; // Rz: SInt16 between -1024 and 1024
      
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
      break;
    case kConnexionCmdHandleButtons:
      // find which button changed
      // (from release to press or from press to release)

      // mask shoud have only one bit at 1, the one of the button that changed.
      vtkDebugMacro("lastbuttons=" << hex << this->LastButtonState << dec);
      vtkDebugMacro("buttons=" << hex << s->buttons << dec);
      mask=s->buttons^this->LastButtonState;
      vtkDebugMacro("mask=" << hex << mask << dec);
      this->LastButtonState=s->buttons;
      
      pressed=(s->buttons&mask)!=0;
      
      // find the button number (starting at 0).
      buttonInfo=0;
      mask=mask>>1;
      while(mask!=0)
        {
        mask=mask>>1;
        ++buttonInfo;
        }
      if(pressed)
        {
        vtkDebugMacro(<< "it is kConnexionCmdHandleButtons (press) event");
        this->Interactor->InvokeEvent(vtkCommand::TDxButtonPressEvent,
                                      &buttonInfo);
        }
      else
        {
        vtkDebugMacro(<< "it is kConnexionCmdHandleButtons (release) event");
        this->Interactor->InvokeEvent(vtkCommand::TDxButtonReleaseEvent,
                                      &buttonInfo);
        }
      break;
    default:
      // ignore kConnexionCmd(None|HandleRawData|AppSpecific)
      break;
    }
}

// ----------------------------------------------------------------------------
void vtkTDxMacDevice::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "ClientApplicationName=";
  if(this->ClientApplicationName!=0)
    {
    os << this->ClientApplicationName << endl;
    }
  else
    {
    os << "(none)" << endl;
    }
}

// ----------------------------------------------------------------------------
// Description:
// Convert a C string to a Pascal String. Allocation happens with new[].
// It is the responsability of the user to call delete[].
//
// Apple specific. String literal starting with \p are pascal strings: it is
// an unsigned char array starting with the length and terminated by \0. The
// length does not include the length value neither the \0.
// Pascal strings literal are allowed with apple specific gcc option
// -fpascal-strings. Pascal literal are writable is -fwritable-strings is
// set. -Wwrite-strings is a related warning flag.
// \pre s_exists: s!=0
// \pre s_small_enough: strlen(s)<=255
// \post result_exists: result!=0
unsigned char *vtkTDxMacDevice::CStringToPascalString(const char *s)
{
  assert("pre: s_exists" && s!=0);
  assert("pre: s_small_enough" && strlen(s)<=255);
  
  size_t l=strlen(s);
  unsigned char *result=new unsigned char[l+2]; 
  
  result[0]=static_cast<unsigned char>(l);
  result[l+1]=0;
  size_t i=0;
  while(i<l)
    {
    result[i+1]=static_cast<unsigned char>(s[i]);
    ++i;
    }
  
  assert("post result_exists" && result!=0);
  return result;
}

// ----------------------------------------------------------------------------
// The timer callback is used to poll the 3d input device for change of
// keystates and the cap displacement values.
void vtkTDxMacDeviceMessageHandler(io_connect_t connection,
                                   natural_t messageType,
                                   void *messageArgument)
{ 
  ConnexionDeviceState *s;
  std::map<UInt16,vtkTDxMacDevice *,vtkLessThanClientID>::iterator it;
  vtkTDxMacDevice *device;
  
  switch(messageType)
    {
    case kConnexionMsgDeviceState:
      s=static_cast<ConnexionDeviceState *>(messageArgument);
      it=vtkClientIDToDeviceObject.find(s->client);
      
      if(it==vtkClientIDToDeviceObject.end())
        {
        // it can happen during initialization phase because of a race condition:
        // clientID is registered with the system, events can happen but the
        // line that records the clientID is the map is not executed yet.
        // No worries.
        return;
        }
      
      device=(*it).second;
      device->ProcessEvent(s);
      break;
    default:
      // other messageTypes can happen and should be ignored.
      break;
    }
}
