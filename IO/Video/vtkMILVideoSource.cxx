/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMILVideoSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMILVideoSource.h"
#include "vtkTimerLog.h"
#include "vtkObjectFactory.h"
#include "vtkCriticalSection.h"

#include <mil.h>
#include <cctype>
#include <cstring>

vtkStandardNewMacro(vtkMILVideoSource);

//----------------------------------------------------------------------------
vtkMILVideoSource::vtkMILVideoSource()
{
  this->Initialized = 0;

  this->FatalMILError = 0;

  this->ContrastLevel = 1.0;
  this->BrightnessLevel = 128;
  this->HueLevel = 0.0;
  this->SaturationLevel = 1.0;
  this->BlackLevel = 0.0;
  this->WhiteLevel = 255.0;

  this->VideoChannel = 0;
  this->VideoInput = VTK_MIL_MONO;
  this->VideoInputForColor = VTK_MIL_YC;
  this->VideoFormat = VTK_MIL_RS170;

  this->FrameMaxSize[0] = 640;
  this->FrameMaxSize[1] = 480;

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

  this->MILErrorMessages = 1;

  this->FlipFrames = 1; //apply vertical flip to each frame

  // for accurate timing
  this->LastTimeStamp = 0;
  this->LastFrameCount = 0;
  this->EstimatedFramePeriod = 0.033;
  this->NextFramePeriod = 0.033;
}

//----------------------------------------------------------------------------
vtkMILVideoSource::~vtkMILVideoSource()
{
  this->vtkMILVideoSource::ReleaseSystemResources();

  delete [] this->MILDigitizerDCF;
  this->MILDigitizerDCF = NULL;

  this->SetMILSystemType(0);
}

//----------------------------------------------------------------------------
void vtkMILVideoSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "VideoChannel: " << this->VideoChannel << "\n";

  os << indent << "ContrastLevel: " << this->ContrastLevel << "\n";

  os << indent << "BrightnessLevel: " << this->BrightnessLevel << "\n";

  os << indent << "HueLevel: " << this->HueLevel << "\n";

  os << indent << "SaturationLevel: " << this->SaturationLevel << "\n";

  os << indent << "BlackLevel: " << this->BlackLevel << "\n";

  os << indent << "WhiteLevel: " << this->WhiteLevel << "\n";

  os << indent << "VideoInput: ";
  switch (this->VideoInput)
  {
    case VTK_MIL_MONO:
      os << "Mono\n";
      break;
    case VTK_MIL_COMPOSITE:
      os << "Mono\n";
      break;
    case VTK_MIL_YC:
      os << "Mono\n";
      break;
    case VTK_MIL_RGB:
      os << "Mono\n";
      break;
    case VTK_MIL_DIGITAL:
      os << "Mono\n";
      break;
    default:
      os << "Unrecognized\n";
      break;
  }

  os << indent << "VideoFormat: ";
  switch (this->VideoFormat)
  {
    case VTK_MIL_RS170:
      os << "RS170\n";
      break;
    case VTK_MIL_NTSC:
      os << "NTSC\n";
      break;
    case VTK_MIL_CCIR:
      os << "CCIR\n";
      break;
    case VTK_MIL_PAL:
      os << "PAL\n";
      break;
    case VTK_MIL_SECAM:
      os << "SECAM\n";
      break;
    default:
      os << "Unrecognized\n";
      break;
  }

  os << indent << "MILSystemType: " <<
    (this->MILSystemType ? this->MILSystemType : "Default") << "\n";

  os << indent << "MILSystemNumber: " << this->MILSystemNumber << "\n";

  os << indent << "MILDigitizerDCF: " << (this->MILDigitizerDCF ?
    this->MILDigitizerDCF : "NULL") << "\n";

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
// load the DLL for the specified Matrox digitizer, for MIL 5 and MIL 6
char *vtkMILVideoSource::MILInterpreterForSystem(const char *system)
{
  char *dll_name;
  char *func_name;

  if (strcmp(system,VTK_MIL_CORONA) == 0)
  {
    dll_name = "milcor";
    func_name = "MDCoronaCommandDecoder";
  }
  else if (strcmp(system,VTK_MIL_METEOR) == 0)
  {
    dll_name = "milmet";
    func_name = "MDMeteorCommandDecoder";
  }
  else if (strcmp(system,VTK_MIL_METEOR_II) == 0)
  {
    dll_name = "milmet2";
    func_name = "MDMeteorIICommandDecoder";
  }
  else if (strcmp(system,VTK_MIL_METEOR_II_DIG) == 0)
  {
    dll_name = "milmet2d";
    func_name = "MDMeteorIIDigCommandDecoder";
  }
  else if (strcmp(system,VTK_MIL_PULSAR) == 0)
  {
    dll_name = "milpul";
    func_name = "MDPulsarCommandDecoder";
  }
  else if (strcmp(system,VTK_MIL_GENESIS) == 0)
  {
    dll_name = "milgen";
    func_name = "MDGenesisCommandDecoder";
  }
  else if (strcmp(system,VTK_MIL_ORION) == 0)
  {
    dll_name = "milorion";
    func_name = "MDOrionCommandDecoder";
  }
  else
  {
    dll_name = "unknown";
    func_name = "unknown";
  }

  // first try mil.dll (for later versions of mil)
  this->MILInterpreterDLL = "mil";
  HINSTANCE mil_lib = LoadLibrary("mil");
  if (mil_lib == 0)
  {
    return NULL;
  }
  void *proc_address = (void *)GetProcAddress(mil_lib,func_name);
  if (proc_address)
  {
    return proc_address;
  }

  // then try the device-specific dll
  this->MILInterpreterDLL = dll_name;
  mil_lib = LoadLibrary(dll_name);

  if (mil_lib == 0)
  {
    return NULL;
  }

  return (char *)GetProcAddress(mil_lib,func_name);
}

//----------------------------------------------------------------------------
static void vtkMILVideoSourceSetChannel(long digID, int channel)
{
  if (digID == 0)
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

  MdigChannel(digID,mil_channel);
}

//----------------------------------------------------------------------------
static void vtkMILVideoSourceSetLevel(long digID, int ref, float level)
{
  if (digID == 0)
  {
    return;
  }

  long int_level = M_MIN_LEVEL + level*(M_MAX_LEVEL-M_MIN_LEVEL);

  if (int_level < M_MIN_LEVEL)
  {
    int_level = M_MIN_LEVEL;
  }

  if (int_level > M_MAX_LEVEL)
  {
    int_level = M_MAX_LEVEL;
  }

  MdigReference(digID,ref,int_level);
}

//----------------------------------------------------------------------------
static void vtkMILVideoSourceSetSize(long digID, int size[3], int maxSize[2])
{
  if (digID == 0)
  {
    return;
  }

  int shrink_x = maxSize[0]/size[0];
  int shrink_y = maxSize[1]/size[1];
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

  MdigControl(digID,M_GRAB_SCALE_X,1.0/shrink_x);
  MdigControl(digID,M_GRAB_SCALE_Y,1.0/shrink_y);
}

//----------------------------------------------------------------------------
void vtkMILVideoSource::Initialize()
{
  static char *system_types[] = { VTK_MIL_METEOR, VTK_MIL_METEOR_II,
                                  VTK_MIL_METEOR_II_DIG, VTK_MIL_METEOR_II_CL,
                                  VTK_MIL_METEOR_II_1394, VTK_MIL_CORONA_II,
                                  VTK_MIL_CORONA, VTK_MIL_PULSAR,
                                  VTK_MIL_GENESIS, VTK_MIL_GENESIS_PLUS,
                                  VTK_MIL_ORION, VTK_MIL_CRONOS,
                                  VTK_MIL_ODYSSEY, 0 };

  if (this->Initialized || this->FatalMILError)
  {
    return;
  }

  this->Initialized = 1;

  // update the frame buffer now just in case there is an error
  this->UpdateFrameBuffer();

  if (this->MILAppID == 0)
  {
    this->MILAppID = MappAlloc(M_DEFAULT,M_NULL);
    if (this->MILAppID == 0)
    {
      this->ReleaseSystemResources();
      vtkErrorMacro(<< "Initialize: couldn't open MIL application\n");
      return;
    }
    this->MILAppInternallyAllocated = 1;
  }

  long version = MappInquire(M_VERSION,M_NULL);

  if (this->MILSysID == 0)
  {
    char *systemType;
    if (this->MILSystemType != VTK_MIL_DEFAULT)
    { // asked for a particular system by name
      if (version >= 7)
      {  // try MIL 7 style of allocation
        char tmptext[256];
        strncpy(tmptext,"\\\\.\\",4);
        strncpy(&tmptext[4],this->MILSystemType,252);
        this->MILSysID = MsysAlloc(tmptext,this->MILSystemNumber,
                                   M_DEFAULT,M_NULL);
      }
      else
      { // try MIL 5, MIL 6 which requires loading the appropriate DLL
        systemType = this->MILInterpreterForSystem(this->MILSystemType);
        if (systemType)
        {
          this->MILSysID = MsysAlloc(systemType, this->MILSystemNumber,
                                     M_DEFAULT,M_NULL);
        }
      }

      if (this->MILSysID == 0)
      {
        this->ReleaseSystemResources();
        vtkErrorMacro(<< "Initialize: couldn't find " << this->MILInterpreterDLL << ".dll\n");
        return;
      }
    }
    else
    { // try for any known MIL system
      MappControl(M_ERROR,M_PRINT_DISABLE);
      int i;
      for (i = 0; this->MILSysID == 0 && system_types[i] != 0; i++)
      {
        if (version >= 7)
        {
          // try MIL 7 style of allocation
          char tmptext[256];
          sprintf(tmptext,"\\\\.\\%s",system_types[i]);
          this->MILSysID = MsysAlloc(tmptext,this->MILSystemNumber,
                                     M_DEFAULT,M_NULL);
        }
        else
        { // try MIL 5, MIL 6 which requires loading the appropriate DLL
          systemType = this->MILInterpreterForSystem(system_types[i]);
          if (systemType)
          {
            this->MILSysID = MsysAlloc(systemType,this->MILSystemNumber,
                                     M_DEFAULT,M_NULL);
          }
        }
      }
      if (system_types[i] == 0)
      {
        this->ReleaseSystemResources();
        vtkErrorMacro(<< "Initialize: Couldn't find a Matrox frame grabber on the system\n");
        return;
      }
      MappControl(M_ERROR,M_PRINT_ENABLE);
    }
    this->MILSysInternallyAllocated = 1;
  }

  this->AllocateMILBuffer();

  this->AllocateMILDigitizer();

  MappControl(M_ERROR,
              ( this->MILErrorMessages ? M_PRINT_ENABLE : M_PRINT_DISABLE ));

  // update frame buffer again to reflect any changes
  this->UpdateFrameBuffer();
}

//----------------------------------------------------------------------------
void vtkMILVideoSource::ReleaseSystemResources()
{
  if (this->MILAppID != 0)
  {
    MappControl(M_ERROR, M_PRINT_DISABLE);
  }
  if (this->MILDigID)
  {
    if (this->Recording)
    {
      MdigHalt(this->MILDigID);
    }
    MdigGrabWait(this->MILDigID,M_GRAB_END);
    this->Recording = 0;
  }
  /*
  if (this->MILDispID != 0)
    {
    MdispDeselect(this->MILDispID,this->MILDispBufID);
    MdispFree(this->MILDispID);
    this->MILDispID = 0;
    }
  if (this->MILDispBufID != 0)
    {
    MbufFree(this->MILDispBufID);
    this->MILDispBufID = 0;
    }
  */
  if (this->MILBufID != 0)
  {
    MbufFree(this->MILBufID);
    this->MILBufID = 0;
  }
  if (this->MILDigID != 0)
  {
    //  The MdigFree call never returns if it is called by atexit(),
    //  and it doesn't seem to hurt anything if it isn't called.
    MdigFree(this->MILDigID);
    this->MILDigID = 0;
  }
  if (this->MILSysInternallyAllocated && this->MILSysID != 0)
  {
    MsysFree(this->MILSysID);
    this->MILSysID = 0;
  }
  if (this->MILAppInternallyAllocated && this->MILAppID != 0)
  {
    MappFree(this->MILAppID);
    this->MILAppID = 0;
  }
  this->Initialized = 0;
  this->FatalMILError = 0;
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
      if (format == VTK_MIL_CCIR ||
          format == VTK_MIL_PAL ||
          format == VTK_MIL_SECAM)
      {
        frame_stride = (int)(25/rate);
      }
    }
    if ((rate > 0 && ++(self->FrameCounter) >= frame_stride) ||
        self->ForceGrab)
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
    if (this->FrameIndex + 1 < this->FrameBufferSize)
    {
      this->FrameIndex++;
    }
  }

  int index = this->FrameBufferIndex;

  this->FrameBufferTimeStamps[index] =
    this->CreateTimeStampForFrame(this->LastFrameCount + 1);
  if (this->FrameCount++ == 0)
  {
    this->StartTimeStamp = this->FrameBufferTimeStamps[index];
  }

  void *ptr = ((reinterpret_cast<vtkDataArray *>( \
                       this->FrameBuffer[index]))->GetVoidPointer(0));
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

  this->Modified();

  this->FrameBufferMutex->Unlock();
}

//----------------------------------------------------------------------------
// for accurate timing of the transformation: this solves a differential
// equation that works to smooth out the jitter in the times that
// are returned by vtkTimerLog::GetUniversalTime() i.e. the system clock.
double vtkMILVideoSource::CreateTimeStampForFrame(unsigned long framecount)
{
  double timestamp = vtkTimerLog::GetUniversalTime();

  double frameperiod = ((timestamp - this->LastTimeStamp)/
                        (framecount - this->LastFrameCount));
  double deltaperiod = (frameperiod - this->EstimatedFramePeriod)*0.01;

  this->EstimatedFramePeriod += deltaperiod;
  this->LastTimeStamp += ((framecount - this->LastFrameCount)*
                          this->NextFramePeriod);
  this->LastFrameCount = framecount;

  double diffperiod = (timestamp - this->LastTimeStamp);

  if (diffperiod < -0.2 || diffperiod > 0.2)
  { // time is off by more than 0.2 seconds: reset the clock
    this->EstimatedFramePeriod -= deltaperiod;
    this->NextFramePeriod = this->EstimatedFramePeriod;
    this->LastTimeStamp = timestamp;
    return timestamp;
  }

  diffperiod *= 0.1;
  double maxdiff = 0.001;
  if (diffperiod < -maxdiff)
  {
    diffperiod = -maxdiff;
  }
  else if (diffperiod > maxdiff)
  {
    diffperiod = maxdiff;
  }

  this->NextFramePeriod = this->EstimatedFramePeriod + diffperiod;

  return this->LastTimeStamp;
}

//----------------------------------------------------------------------------
// Circulate the buffer and grab a frame.
// This particular implementation just copies random noise into the frames,
// you will certainly want to override this method (also note that this
// is the only method which you really have to override)
void vtkMILVideoSource::Grab()
{
  // ensure that the hardware is initialized.
  this->Initialize();
  if (!this->Initialized)
  {
    return;
  }

  if (!this->Recording)
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

//----------------------------------------------------------------------------
void vtkMILVideoSource::Play()
{
  vtkVideoSource::Play();
}

//----------------------------------------------------------------------------
void vtkMILVideoSource::Record()
{
  this->Initialize();
  if (!this->Initialized)
  {
    return;
  }

  if (this->Playing)
  {
    this->Stop();
  }

  if (this->Recording)
  {
    return;
  }

  this->Recording = 1;
  this->FrameCount = 0;

  MdigInquire(this->MILDigID,M_GRAB_FRAME_END_HANDLER_PTR,
              &this->OldHookFunction);
  MdigInquire(this->MILDigID,M_GRAB_FRAME_END_HANDLER_USER_PTR,
              &this->OldUserDataPtr);
  MdigHookFunction(this->MILDigID,M_GRAB_FRAME_END,
                   &vtkMILVideoSourceHook,
                   (void *)this);
  this->FrameCounter = 0;
  this->ForceGrab = 0;

  // for accurate timing
  this->LastTimeStamp = vtkTimerLog::GetUniversalTime();

  // this will call the hook function on every frame
  MdigGrabContinuous(this->MILDigID,this->MILBufID);

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMILVideoSource::Stop()
{
  if (this->Playing)
  {
    vtkVideoSource::Stop();
  }

  if (!this->Recording)
  {
    return;
  }

  this->Recording = 0;

  MdigHalt(this->MILDigID);
  MdigHookFunction(this->MILDigID,M_GRAB_FRAME_END,
                   (MDIGHOOKFCTPTR)this->OldHookFunction,
                   this->OldUserDataPtr);
  this->OldHookFunction = 0;
  MdigGrabWait(this->MILDigID,M_GRAB_END);

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMILVideoSource::SetMILErrorMessages(int yesno)
{
  if (this->MILErrorMessages == yesno)
  {
    return;
  }

  this->MILErrorMessages = yesno;
  this->Modified();

  if (this->Initialized)
  {
    MappControl(M_ERROR,( yesno ? M_PRINT_ENABLE : M_PRINT_DISABLE ));
  }
}

//----------------------------------------------------------------------------
void vtkMILVideoSource::SetFrameSize(int x, int y, int z)
{
  if (x == this->FrameSize[0] &&
      y == this->FrameSize[1] &&
      z == this->FrameSize[2])
  {
    return;
  }

  if (x < 1 || y < 1 || z != 1)
  {
    vtkErrorMacro(<< "SetFrameSize: Illegal frame size");
    return;
  }

  this->FrameSize[0] = x;
  this->FrameSize[1] = y;
  this->FrameSize[2] = z;

  if (this->Initialized)
  {
    this->FrameBufferMutex->Lock();
    this->UpdateFrameBuffer();
    vtkMILVideoSourceSetSize(this->MILDigID,
                             this->FrameSize,this->FrameMaxSize);
    this->AllocateMILBuffer();
    this->FrameBufferMutex->Unlock();
  }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMILVideoSource::SetOutputFormat(int format)
{
  if (format == this->OutputFormat)
  {
    return;
  }

  this->OutputFormat = format;

  // convert color format to number of scalar components
  int numComponents;

  switch (this->OutputFormat)
  {
    case VTK_RGBA:
      numComponents = 4;
      break;
    case VTK_RGB:
      numComponents = 3;
      break;
    case VTK_LUMINANCE:
      numComponents = 1;
      break;
    default:
      vtkErrorMacro(<< "SetOutputFormat: Unrecognized color format.");
      break;
  }
  this->NumberOfScalarComponents = numComponents;

  if (this->FrameBufferBitsPerPixel != numComponents*8)
  {
    this->FrameBufferMutex->Lock();
    this->FrameBufferBitsPerPixel = numComponents*8;
    if (this->Initialized)
    {
      this->UpdateFrameBuffer();
      this->AllocateMILBuffer();
    }
    this->FrameBufferMutex->Unlock();
  }

  // set video format to match the output format
  if (this->OutputFormat == VTK_RGB || this->OutputFormat == VTK_RGBA)
  {
    if (this->VideoFormat == VTK_MIL_RS170)
    {
      this->SetVideoFormat(VTK_MIL_NTSC);
    }
    if (this->VideoFormat == VTK_MIL_CCIR)
    {
      this->SetVideoFormat(VTK_MIL_PAL);
    }
    if (this->VideoInput == VTK_MIL_MONO)
    {
      this->SetVideoInput(this->VideoInputForColor);
    }
  }
  if (this->OutputFormat == VTK_LUMINANCE)
  {
    if (this->VideoFormat == VTK_MIL_NTSC)
    {
      this->SetVideoFormat(VTK_MIL_RS170);
    }
    if (this->VideoFormat == VTK_MIL_PAL)
    {
      this->SetVideoFormat(VTK_MIL_CCIR);
    }
    if (this->VideoInput == VTK_MIL_YC || this->VideoInput == VTK_MIL_COMPOSITE)
    {
      this->VideoInputForColor = this->VideoInput;
      this->SetVideoInput(VTK_MIL_MONO);
    }
  }

  this->Modified();
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
  this->Modified();

  vtkMILVideoSourceSetChannel(this->MILDigID,channel);
}

//----------------------------------------------------------------------------
void vtkMILVideoSource::SetBrightnessLevel(float brightness)
{
  if (this->BrightnessLevel == brightness)
  {
    return;
  }

  this->BrightnessLevel = brightness;
  this->Modified();

  vtkMILVideoSourceSetLevel(this->MILDigID,M_BRIGHTNESS_REF,brightness/255.0);
}

//----------------------------------------------------------------------------
void vtkMILVideoSource::SetContrastLevel(float contrast)
{
  if (this->ContrastLevel == contrast)
  {
    return;
  }

  this->ContrastLevel = contrast;
  this->Modified();

  vtkMILVideoSourceSetLevel(this->MILDigID,M_CONTRAST_REF,contrast/2.0);
}

//----------------------------------------------------------------------------
void vtkMILVideoSource::SetHueLevel(float hue)
{
  if (this->HueLevel == hue)
  {
    return;
  }

  this->HueLevel = hue;
  this->Modified();

  vtkMILVideoSourceSetLevel(this->MILDigID,M_HUE_REF,0.5+hue);
}

//----------------------------------------------------------------------------
void vtkMILVideoSource::SetSaturationLevel(float saturation)
{
  if (this->SaturationLevel == saturation)
  {
    return;
  }

  this->SaturationLevel = saturation;
  this->Modified();

  vtkMILVideoSourceSetLevel(this->MILDigID,M_SATURATION_REF,saturation/2.0);
}

//----------------------------------------------------------------------------
void vtkMILVideoSource::SetBlackLevel(float black)
{
  if (this->BlackLevel == black)
  {
    return;
  }

  this->BlackLevel = black;
  this->Modified();

  vtkMILVideoSourceSetLevel(this->MILDigID,M_BLACK_REF,black/255);
}

//----------------------------------------------------------------------------
void vtkMILVideoSource::SetWhiteLevel(float white)
{
  if (this->WhiteLevel == white)
  {
    return;
  }

  this->WhiteLevel = white;
  this->Modified();

  vtkMILVideoSourceSetLevel(this->MILDigID,M_WHITE_REF,white/255);
}

//----------------------------------------------------------------------------
void vtkMILVideoSource::AllocateMILDigitizer()
{
  char *format = "M_NTSC";
  int recording = this->Recording;

  if (this->MILDigID)
  {
    if (recording)
    {
      this->Stop();
    }
  }

  if (this->MILDigID != 0)
  {
    MdigFree(this->MILDigID);
  }

  switch (this->VideoFormat)
  {
    case VTK_MIL_RS170:
      format = "M_RS170";
      if (this->VideoInput == VTK_MIL_RGB)
      {
        format = "M_RS170_VIA_RGB";
      }
      break;
    case VTK_MIL_NTSC:
      format = "M_NTSC";
      if (this->VideoInput == VTK_MIL_YC)
      {
        format = "M_NTSC_YC";
      }
      if (this->VideoInput == VTK_MIL_RGB)
      {
        format = "M_NTSC_RGB";
      }
      break;
    case VTK_MIL_CCIR:
      format = "M_CCIR";
      if (this->VideoInput == VTK_MIL_RGB)
      {
        format = "M_CCIR_VIA_RGB";
      }
      this->FrameMaxSize[0] = 768;
      this->FrameMaxSize[1] = 576;
      break;
    case VTK_MIL_PAL:
    case VTK_MIL_SECAM:
      format = "M_PAL";
      if (this->VideoInput == VTK_MIL_YC)
      {
        format = "M_PAL_YC";
      }
      if (this->VideoInput == VTK_MIL_RGB)
      {
        format = "M_PAL_RGB";
      }
      this->FrameMaxSize[0] = 768;
      this->FrameMaxSize[1] = 576;
      break;
    case VTK_MIL_NONSTANDARD:
      this->FrameMaxSize[0] = 0;
      this->FrameMaxSize[1] = 0;
      break;
    default:
      vtkWarningMacro(<< "AllocateMILDigitizer: Unknown video format");
  }

  if (this->MILDigitizerDCF)
  {
    format = this->MILDigitizerDCF;
  }

  this->MILDigID = MdigAlloc(this->MILSysID,this->MILDigitizerNumber,format,
                             M_DEFAULT,M_NULL);

  if (this->MILDigID == 0)
  {
    vtkErrorMacro(<< "AllocateMILDigitizer:  Couldn't allocate MIL Digitizer\n");
    return;
  }

  vtkMILVideoSourceSetSize(this->MILDigID,this->FrameSize,this->FrameMaxSize);

  vtkMILVideoSourceSetChannel(this->MILDigID,this->VideoChannel);

  if (this->BrightnessLevel != 128)
  {
    vtkMILVideoSourceSetLevel(this->MILDigID,M_BRIGHTNESS_REF,
                              this->BrightnessLevel/255);
  }
  if (this->ContrastLevel != 1.0)
  {
    vtkMILVideoSourceSetLevel(this->MILDigID,M_CONTRAST_REF,
                              this->ContrastLevel/2.0);
  }
  if (this->HueLevel != 0.0)
  {
    vtkMILVideoSourceSetLevel(this->MILDigID,M_HUE_REF,
                              0.5+this->HueLevel);
  }
  if (this->SaturationLevel != 1.0)
  {
    vtkMILVideoSourceSetLevel(this->MILDigID,M_SATURATION_REF,
                              this->SaturationLevel/2.0);
  }
  if (this->BlackLevel != 0.0)
  {
    vtkMILVideoSourceSetLevel(this->MILDigID,M_BLACK_REF,
                              this->BlackLevel/255);
  }
  if (this->WhiteLevel != 255.0)
  {
    vtkMILVideoSourceSetLevel(this->MILDigID,M_WHITE_REF,
                              this->WhiteLevel/255);
  }

  if (this->MILDigID && this->MILBufID)
  {
    if (recording)
    {
      this->Record();
    }
  }
}

//----------------------------------------------------------------------------
void vtkMILVideoSource::AllocateMILBuffer()
{
  int recording = this->Recording;

  if (this->MILDigID != 0)
  {
    if (recording)
    {
      this->Stop();
    }
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
     this->MILBufID = MbufAlloc2d(this->MILSysID,this->FrameSize[0],
                                  this->FrameSize[1],
                                  8+M_UNSIGNED,M_IMAGE+M_GRAB,M_NULL);
  }
  else if (this->OutputFormat == VTK_RGB)
  {
    this->MILBufID = MbufAllocColor(this->MILSysID,3,this->FrameSize[0],
                                    this->FrameSize[1],
                                    8+M_UNSIGNED,M_IMAGE+M_GRAB+ \
                                    M_RGB24+M_PACKED,
                                    M_NULL);
  }
  else if (this->OutputFormat == VTK_RGBA)
  {
    this->MILBufID = MbufAllocColor(this->MILSysID,3,this->FrameSize[0],
                                    this->FrameSize[1],
                                    8+M_UNSIGNED,M_IMAGE+M_GRAB+M_RGB32+ \
                                    M_PACKED,
                                    M_NULL);
  }

  if (this->MILBufID == 0)
  {
    vtkErrorMacro(<< "AllocateMILBuffer:  Couldn't allocate MIL Buffer\n");
    return;
  }

  if (this->MILDigID != 0 && this->MILBufID != 0)
  {
    if (recording)
    {
      this->Record();
    }
  }
}



