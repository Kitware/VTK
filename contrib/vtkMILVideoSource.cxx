/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMILVideoSource.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-1999 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include <ctype.h>
#include <string.h>
#include "vtkTimerLog.h"
#include "vtkMILVideoSource.h"
#include <mil.h>
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkMILVideoSource* vtkMILVideoSource::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMILVideoSource");
  if(ret)
    {
    return (vtkMILVideoSource*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMILVideoSource;
}

//----------------------------------------------------------------------------
vtkMILVideoSource::vtkMILVideoSource()
{
  this->Initialized = 0;

  this->OldHookFunction = 0;
  this->OldUserDataPtr = 0;

  this->MILAppID = 0;
  this->MILSysID = 0;
  this->MILDigID = 0;
  this->MILBufID = 0;
  //this->MILDispBufID = 0;
  //this->MILDispID = 0;

  this->MILAppInternallyAllocated = 0;
  this->MILSysInternallyAllocated = 0;

  this->MILSystemType = VTK_MIL_DEFAULT;
  this->MILSystemNumber = M_DEFAULT;

  this->MILDigitizerNumber = M_DEFAULT;
  this->MILDigitizerDCF = NULL;

  this->MILErrorMessages = 0;

  this->FlipFrames = 1; //apply vertical flip to each frame
}

//----------------------------------------------------------------------------
vtkMILVideoSource::~vtkMILVideoSource()
{ 
  if (this->MILDigID)
    {
    if (this->Playing)
      {
      MdigHalt(this->MILDigID);
      }
    MdigGrabWait(this->MILDigID,M_GRAB_END);
    }

  /*
  if (this->MILDispID != 0)
    {
    MdispDeselect(this->MILDispID,this->MILDispBufID);
    MdispFree(this->MILDispID);
    }
  if (this->MILDispBufID != 0)
    {
    MbufFree(this->MILDispBufID);
    }
  */
  if (this->MILBufID != 0)
    {
    MbufFree(this->MILBufID);
    }
  if (this->MILDigID != 0)
    {
    MdigFree(this->MILDigID);
    }
  if (this->MILSysInternallyAllocated && this->MILSysID != 0)
    {
    MsysFree(this->MILSysID);
    }
  if (this->MILAppInternallyAllocated && this->MILAppID != 0)
    {
    MappFree(this->MILAppID);
    }

  if (this->MILDigitizerDCF != NULL)
    {
    delete [] this->MILDigitizerDCF;
    }
}

//----------------------------------------------------------------------------
void vtkMILVideoSource::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  
  vtkMILVideoSource::PrintSelf(os,indent);
  
  os << indent << "MILSystemType: ";
  switch (this->MILSystemType)
    {
    case VTK_MIL_DEFAULT:
      os << "Default\n";
      break;
    case VTK_MIL_METEOR:
      os << "Meteor\n";
      break;
    case VTK_MIL_METEOR_II:
      os << "MeteorII\n";
      break;
    case VTK_MIL_METEOR_II_DIG:
      os << "MeteorIIDig\n";
      break;
    case VTK_MIL_PULSAR:
      os << "Pulsar\n";
      break;
    case VTK_MIL_CORONA:
      os << "Corona\n";
      break;
    case VTK_MIL_GENESIS:
      os << "Genesis\n";
      break;
    default:
      os << "Unrecognized\n";
      break;
    }

  os << indent << "MILSystemNumber: " << this->MILSystemNumber << "\n";

  os << indent << "MILDigitizerDCF: " << this->MILDigitizerDCF << "\n";

  os << indent << "MILDigitizerNumber: " << this->MILDigitizerNumber << "\n";

  os << indent << "MILErrorMessages: " << (this->MILErrorMessages ? "On\n" : "Off\n");

  os << indent << "MILAppID: " << this->MILAppID << "\n";
  os << indent << "MILSysID: " << this->MILSysID << "\n";
  os << indent << "MILDigID: " << this->MILDigID << "\n";
  os << indent << "MILBufID: " << this->MILBufID << "\n";
  //  os << indent << "MILDispBufID: " << this->MILDispBufID << "\n";
  //  os << indent << "MILDispID: " << this->MILDispID << "\n";
}

//----------------------------------------------------------------------------
// load the DLL for the specified Matrox digitizer
static void *vtkMILVideoSourceSystemType(int system)
{
  char *dll_name;
  char *func_name;

  switch (system)
    {
    case VTK_MIL_CORONA:
      dll_name = "milcor";
      func_name = "MDCoronaCommandDecoder";
      break;
    case VTK_MIL_METEOR:
      dll_name = "milmet";
      func_name = "MDMeteorCommandDecoder";
      break;
    case VTK_MIL_METEOR_II:
      dll_name = "milmet2";
      func_name = "MDMeteorIICommandDecoder";
      break;
    case VTK_MIL_METEOR_II_DIG:
      dll_name = "milmet2d";
      func_name = "MDMeteorIIDigCommandDecoder";
      break;
    case VTK_MIL_PULSAR:
      dll_name = "milpul";
      func_name = "MDPulsarCommandDecoder";
      break;
    case VTK_MIL_GENESIS:
      dll_name = "milgen";
      func_name = "MDGenesisCommandDecoder";
      break;
    default:
      dll_name = "unknown";
      func_name = "unknown";
    }
      
  return (void *)GetProcAddress(LoadLibrary(dll_name),func_name);
}

//----------------------------------------------------------------------------
void vtkMILVideoSource::Initialize()
{
  static int system_types[] = { VTK_MIL_METEOR, VTK_MIL_METEOR_II, VTK_MIL_CORONA,
				VTK_MIL_PULSAR, VTK_MIL_METEOR_II_DIG, VTK_MIL_GENESIS, 0 };

  if (this->Initialized)
    {
    return;
    }

  this->Initialized = 1;

  if (this->MILAppID == 0)
    {
    MappAlloc(M_DEFAULT,&this->MILAppID);
    this->MILAppInternallyAllocated = 1;    
    }

  if (this->MILSysID == 0)
    {
    if (this->MILSystemType != VTK_MIL_DEFAULT)
      { // try for requested system
      MsysAlloc(vtkMILVideoSourceSystemType(this->MILSystemType),this->MILSystemNumber,
	      M_DEFAULT,&this->MILSysID);
      }
    else
      {
      MappControl(M_ERROR,M_PRINT_DISABLE);
      int i;
      for (i = 0; this->MILSysID == 0 && system_types[i] != 0; i++)
	{
	this->MILSysID = MsysAlloc(vtkMILVideoSourceSystemType(system_types[i]),
				   this->MILSystemNumber,M_DEFAULT,M_NULL);
	}
      if (this->MILSysID == 0)
	{
        vtkErrorMacro(<< "Initialize: Couldn't find a Matrox frame grabber on the system");
	}
      }
      
    this->MILSysInternallyAllocated = 1;
    }

  this->AllocateMILBuffer();

  this->AllocateMILDigitizer();

  MappControl(M_ERROR,( this->MILErrorMessages ? M_PRINT_ENABLE : M_PRINT_DISABLE ));

  this->UpdateFrameBuffer();
}  

//----------------------------------------------------------------------------
long MFTYPE vtkMILVideoSourceHook(long HookType, MIL_ID EventID, void *UserPtr)
{
  vtkMILVideoSource *self = (vtkMILVideoSource *)UserPtr;

  if (HookType == M_GRAB_FRAME_END)
    {
    double time = 1000;
    float rate = self->GetFrameRate();
    int format = self->GetVideoFormat();
    int frame_stride;
    if (rate > 0)
      {
      frame_stride = (int)(30/rate);
      if (format == VTK_VIDEO_CCIR || 
	  format == VTK_VIDEO_PAL ||
	  format == VTK_VIDEO_SECAM)
	{
	frame_stride = (int)(25/rate);
	}
      }
    if ((rate > 0 && ++(self->FrameCounter) >= frame_stride) || self->ForceGrab)
      {
      self->InternalGrab();
      self->FrameCounter = 0;
      self->ForceGrab = 0;
      }
    }
  if (self->OldHookFunction)
    {
    return ((MDIGHOOKFCTPTR)self->OldHookFunction)(HookType,EventID,
					     self->OldUserDataPtr);
    }
  else
    {
    return M_NULL;
    }
}

//----------------------------------------------------------------------------
void vtkMILVideoSource::InternalGrab()
{
  this->FrameBufferMutex->Lock();

  if (this->AutoAdvance)
    {
    this->AdvanceFrameBuffer(1);
    }

  int index = this->FrameBufferIndex;

  this->FrameBufferTimeStamps[index] = vtkTimerLog::GetCurrentTime();

  void *ptr = ((vtkScalars *)this->FrameBuffer[index])->GetVoidPointer(0);
  int depth = this->FrameBufferBitsPerPixel/8;

  int offsetX = this->FrameBufferExtent[0];
  int offsetY = this->FrameBufferExtent[2];

  int sizeX = this->FrameBufferExtent[1] - this->FrameBufferExtent[0] + 1;
  int sizeY = this->FrameBufferExtent[3] - this->FrameBufferExtent[2] + 1;

  if (sizeX > 0 && sizeY > 0)
    {
    if (depth == 1)
      {
      MbufGet2d(this->MILBufID,offsetX,offsetY,sizeX,sizeY,ptr);
      }
    else if (depth == 3)
      {
      MbufGetColor2d(this->MILBufID,M_RGB24+M_PACKED,M_ALL_BAND,
		     offsetX,offsetY,sizeX,sizeY,ptr);
      }
    else if (depth == 4) 
      {
      MbufGetColor2d(this->MILBufID,M_RGB32+M_PACKED,M_ALL_BAND,
		     offsetX,offsetY,sizeX,sizeY,ptr);
      }
    }

  this->FrameBufferMutex->Unlock();

  this->Modified();
}
  

//----------------------------------------------------------------------------
// Circulate the buffer and grab a frame.
// This particular implementation just copies random noise into the frames,
// you will certainly want to override this method (also note that this
// is the only method which you really have to override)
void vtkMILVideoSource::Grab(int numFrames)
{
  if (numFrames > this->FrameBufferSize || numFrames < 1)
    {
    vtkErrorMacro(<< "Grab: # of frames must be at least 1");
    }

  // ensure that the hardware is initialized.
  this->Initialize();

  int f;
  for (f = 0; f < numFrames; f++) 
    {
    if (!this->Playing)
      {
      MdigGrab(this->MILDigID,this->MILBufID);
      MdigGrabWait(this->MILDigID,M_GRAB_END);
      this->InternalGrab();
      }
    else
      {
      this->ForceGrab = 1;
      }
    }
}

//----------------------------------------------------------------------------
void vtkMILVideoSource::Play()
{
  this->Initialize();

  if (this->Playing)
    {
    return;
    }

  this->Playing = 1;

  MdigInquire(this->MILDigID,M_GRAB_FRAME_END_HANDLER_PTR,
	      &this->OldHookFunction);
  MdigInquire(this->MILDigID,M_GRAB_FRAME_END_HANDLER_USER_PTR,
	      &this->OldUserDataPtr);
  MdigHookFunction(this->MILDigID,M_GRAB_FRAME_END,
		   &vtkMILVideoSourceHook,
		   (void *)this);
  this->FrameCounter = 0;
  this->ForceGrab = 0;

  // this will call the hook function on every frame
  MdigGrabContinuous(this->MILDigID,this->MILBufID);
}
    
//----------------------------------------------------------------------------
void vtkMILVideoSource::Stop()
{
  if (!this->Playing)
    {
    return;
    }

  this->Playing = 0;

  MdigHalt(this->MILDigID);
  MdigHookFunction(this->MILDigID,M_GRAB_FRAME_END,
		   (MDIGHOOKFCTPTR)this->OldHookFunction,
		   OldUserDataPtr);
  this->OldHookFunction == 0;
  MdigGrabWait(this->MILDigID,M_GRAB_END);
}

//----------------------------------------------------------------------------
void vtkMILVideoSource::SetMILErrorMessages(int yesno)
{
  if (this->MILErrorMessages == yesno)
    {
    return;
    }

  this->MILErrorMessages = yesno;

  if (this->Initialized)
    {
    MappControl(M_ERROR,( yesno ? M_PRINT_ENABLE : M_PRINT_DISABLE ));
    }
}


//----------------------------------------------------------------------------
void vtkMILVideoSource::SetOutputFormat(int format)
{
  if (this->OutputFormat == format)
    {
    return;
    }

  // do upper class stuff
  this->vtkVideoSource::SetOutputFormat(format);

  // don't do anything if the digitizer isn't initialized
  if (this->Initialized)
    {
    this->AllocateMILBuffer();
    }
}

//----------------------------------------------------------------------------
void vtkMILVideoSource::SetVideoFormat(int format)
{
  if (this->VideoFormat == format)
    {
    return;
    }

  this->VideoFormat = format;
  
  // don't do anything if the digitizer isn't initialized
  if (this->Initialized)
    {
    this->AllocateMILDigitizer();
    }
}  

//----------------------------------------------------------------------------
void vtkMILVideoSource::SetVideoInput(int input)
{
  if (this->VideoInput == input)
    {
    return;
    }
  
  this->VideoInput = input;

  // don't do anything if the digitizer isn't initialized
  if (this->Initialized)
    {
    this->AllocateMILDigitizer();
    }
}  

//----------------------------------------------------------------------------
void vtkMILVideoSource::SetVideoChannel(int channel)
{
  if (this->VideoChannel == channel)
    {
    return;
    }

  this->VideoChannel = channel;

  if (this->MILDigID == 0)
    {
    return;
    }

  int mil_channel = M_DEFAULT;

  switch(channel)
    {
    case 0:
      mil_channel = M_CH0;
      break;
    case 1:
      mil_channel = M_CH1;
      break;
    case 2:
      mil_channel = M_CH2;
      break;
    case 3:
      mil_channel = M_CH3;
      break;
    }

  MdigChannel(this->MILDigID,mil_channel);
}

//----------------------------------------------------------------------------
void vtkMILVideoSource::AllocateMILDigitizer()
{
  int maxwidth = 640;
  int maxheight = 480;
  char *format = "M_NTSC";

  if (this->MILDigID)
    {
    if (this->Playing)
      {
      MdigHalt(this->MILDigID);
      }
    MdigGrabWait(this->MILDigID,M_GRAB_END);
    }

  if (this->MILDigID != 0)
    {
    MdigFree(this->MILDigID);
    }

  switch (this->VideoFormat)
    {
    case VTK_VIDEO_RS170:
      format = "M_RS170";
      if (this->VideoInput == VTK_VIDEO_RGB)
	{
	format = "M_RS170_VIA_RGB";
	}
      break;
    case VTK_VIDEO_NTSC:
      format = "M_NTSC";
      if (this->VideoInput == VTK_VIDEO_YC)
	{
	format = "M_NTSC_YC";
	}
      if (this->VideoInput == VTK_VIDEO_RGB)
	{
	format = "M_NTSC_RGB";
	}
      break;
    case VTK_VIDEO_CCIR:
      format = "M_CCIR";
      if (this->VideoInput == VTK_VIDEO_RGB)
	{
	format = "M_CCIR_VIA_RGB";
	}
      maxwidth = 768;
      maxheight = 576;
      break;
    case VTK_VIDEO_PAL:
    case VTK_VIDEO_SECAM:
      format = "M_PAL";
      if (this->VideoInput == VTK_VIDEO_YC)
	{
	format = "M_PAL_YC";
	}
      if (this->VideoInput == VTK_VIDEO_RGB)
	{
	format = "M_PAL_RGB";
	}
      maxwidth = 768;
      maxheight = 576;
      break;
    case VTK_VIDEO_NONSTANDARD:
      maxwidth = 0;
      maxheight = 0;
      break;
    default:
      vtkWarningMacro(<< "AllocateMILDigitizer: Unknown video format");
    }

  if (this->MILDigitizerDCF)
    {
    format = this->MILDigitizerDCF;
    }

  int shrink_x = maxwidth/this->FrameSize[0];
  int shrink_y = maxheight/this->FrameSize[1];
  if (shrink_x < 1)
    {
    shrink_x = 1;
    }
  if (shrink_y < 1)
    {
    shrink_y = 1;
    }
  
  // convert shrink_x, shrink_y to power of 2
  int i;
  for (i = 0; shrink_x; i++)
    {
    shrink_x = shrink_x >> 1;
    }
  shrink_x = 1 << (i-1);
  for (i = 0; shrink_y; i++)
    {
    shrink_y = shrink_y >> 1;
    }
  shrink_y = 1 << (i-1);
  
  MdigAlloc(this->MILSysID,this->MILDigitizerNumber,format,M_DEFAULT,&this->MILDigID);
  MdigControl(this->MILDigID,M_GRAB_SCALE_X,1.0/shrink_x);
  MdigControl(this->MILDigID,M_GRAB_SCALE_Y,1.0/shrink_y);

  int channel = this->VideoChannel;
  this->VideoChannel = -1;
  this->SetVideoChannel(channel);

  if (this->MILDigID && this->MILBufID)
    {
    if (this->Playing)
      {
      MdigGrabContinuous(this->MILDigID,this->MILBufID);
      }
    }
}

//----------------------------------------------------------------------------
void vtkMILVideoSource::AllocateMILBuffer()
{
  if (this->MILDigID)
    {
    if (this->Playing)
      {
      MdigHalt(this->MILDigID);
      }
    MdigGrabWait(this->MILDigID,M_GRAB_END);
    }

  if (this->MILBufID != 0)
    {
    MbufFree(this->MILBufID);
    }

  if (this->OutputFormat != VTK_LUMINANCE && 
      this->OutputFormat != VTK_RGB &&
      this->OutputFormat != VTK_RGBA)
    {
    vtkWarningMacro(<< "Initialize: unsupported OutputFormat");
    this->vtkVideoSource::SetOutputFormat(VTK_LUMINANCE);
    } 

  if (this->OutputFormat == VTK_LUMINANCE)
    {
    MbufAlloc2d(this->MILSysID,this->FrameSize[0],this->FrameSize[1],
		8+M_UNSIGNED,M_IMAGE+M_GRAB,&this->MILBufID);
    }
  else if (this->OutputFormat == VTK_RGB)
    {
    MbufAllocColor(this->MILSysID,3,this->FrameSize[0],this->FrameSize[1],
		   8+M_UNSIGNED,M_IMAGE+M_GRAB+M_RGB24+M_PACKED,
		   &this->MILBufID);
    }
  else if (this->OutputFormat == VTK_RGBA)
    {
    MbufAllocColor(this->MILSysID,3,this->FrameSize[0],this->FrameSize[1],
		   8+M_UNSIGNED,M_IMAGE+M_GRAB+M_RGB32+M_PACKED,
		   &this->MILBufID);
    }

  if (this->MILDigID && this->MILBufID)
    {
    if (this->Playing)
      {
      MdigGrabContinuous(this->MILDigID,this->MILBufID);
      }
    }
}



