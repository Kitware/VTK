/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMILVideoSource.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkMILVideoSource - Matrox Imaging Library frame grabbers
// .SECTION Description
// vtkMILVideoSource provides an interface to Matrox Meteor, MeteorII
// and Corona video digitizers through the Matrox Imaging Library 
// interface.  In order to use this class, you must link VTK with mil.lib,
// MIL version 5.0 or higher is required.

// .SECTION See Also
// vtkWin32VideoSource vtkVideoSource

#ifndef __vtkMILVideoSource_h
#define __vtkMILVideoSource_h

#include "vtkVideoSource.h"

// digitizer hardware
#define VTK_MIL_DEFAULT       0
#define VTK_MIL_METEOR        1
#define VTK_MIL_METEOR_II     2
#define VTK_MIL_METEOR_II_DIG 3
#define VTK_MIL_CORONA        4
#define VTK_MIL_PULSAR        5
#define VTK_MIL_GENESIS       6

// video inputs: 
#define VTK_MIL_MONO          0
#define VTK_MIL_COMPOSITE     1
#define VTK_MIL_YC            2
#define VTK_MIL_RGB           3
#define VTK_MIL_DIGITAL       4

// video formats:
#define VTK_MIL_RS170         0
#define VTK_MIL_NTSC          1
#define VTK_MIL_CCIR          2 
#define VTK_MIL_PAL           3 
#define VTK_MIL_SECAM         4
#define VTK_MIL_NONSTANDARD   5       

class VTK_EXPORT vtkMILVideoSource : public vtkVideoSource
{
public:
  static vtkMILVideoSource *New();
  vtkTypeMacro(vtkMILVideoSource,vtkVideoSource);
  void PrintSelf(ostream& os, vtkIndent indent);   

  // Description:
  // Standard VCR functionality: Record incoming video.
  void Record();

  // Description:
  // Standard VCR functionality: Play recorded video.
  void Play();

  // Description:
  // Standard VCR functionality: Stop recording or playing.
  void Stop();

  // Description:
  // Grab a single video frame.
  void Grab();
 
  // Description:
  // Request a particular frame size (set the third value to 1).
  void SetFrameSize(int x, int y, int z);
  
  // Description:
  // Request a particular output format (default: VTK_RGB).
  void SetOutputFormat(int format);

  // Description:
  // Set/Get the video channel
  virtual void SetVideoChannel(int channel);
  vtkGetMacro(VideoChannel, int);

  // Description:
  // Set/Get the video format
  virtual void SetVideoFormat(int format);
  void SetVideoFormatToNTSC() { this->SetVideoFormat(VTK_MIL_NTSC); };
  void SetVideoFormatToPAL() { this->SetVideoFormat(VTK_MIL_PAL); };
  void SetVideoFormatToSECAM() { this->SetVideoFormat(VTK_MIL_SECAM); };
  void SetVideoFormatToRS170() { this->SetVideoFormat(VTK_MIL_RS170); };
  void SetVideoFormatToCCIR() { this->SetVideoFormat(VTK_MIL_CCIR); };
  void SetVideoFormatToNonStandard() { 
    this->SetVideoFormat(VTK_MIL_NONSTANDARD); };
  vtkGetMacro(VideoFormat,int);
  
  // Description:
  // Set/Get the video input
  virtual void SetVideoInput(int input);
  void SetVideoInputToMono() { this->SetVideoInput(VTK_MIL_MONO); };
  void SetVideoInputToComposite() {this->SetVideoInput(VTK_MIL_COMPOSITE);};
  void SetVideoInputToYC() { this->SetVideoInput(VTK_MIL_YC); };
  void SetVideoInputToRGB() { this->SetVideoInput(VTK_MIL_RGB); };
  void SetVideoInputToDigital() { this->SetVideoInput(VTK_MIL_DIGITAL); };
  vtkGetMacro(VideoInput,int);

  // Description:
  // Set/Get the video levels: the valid ranges are: 
  // Contrast [0.0,2.0] 
  // Brighness [0.0,255.0] 
  // Hue [-0.5,0.5] 
  // Saturation [0.0,2.0] 
  virtual void SetContrastLevel(float contrast);
  vtkGetMacro(ContrastLevel,float);
  virtual void SetBrightnessLevel(float brightness);
  vtkGetMacro(BrightnessLevel,float);
  virtual void SetHueLevel(float hue);
  vtkGetMacro(HueLevel,float);
  virtual void SetSaturationLevel(float saturation);
  vtkGetMacro(SaturationLevel,float);

  // Description:
  // Set the system which you want use.  If you don't specify a system,
  // then an attempt will be made to autodetect your system.
  vtkSetMacro(MILSystemType,int);
  vtkGetMacro(MILSystemType,int);
  void SetMILSystemTypeToMeteor() { this->SetMILSystemType(VTK_MIL_METEOR); };
  void SetMILSystemTypeToMeteorII() { this->SetMILSystemType(VTK_MIL_METEOR_II); };
  void SetMILSystemTypeToCorona() { this->SetMILSystemType(VTK_MIL_CORONA); };
  void SetMILSystemTypeToPulsar() { this->SetMILSystemType(VTK_MIL_PULSAR); };
  void SetMILSystemTypeToMeteorIIDig() { this->SetMILSystemType(VTK_MIL_METEOR_II_DIG); };
  void SetMILSystemTypeToGenesis() { this->SetMILSystemType(VTK_MIL_GENESIS); };

  // Description:
  // Set the system number if you have multiple systems of the same type
  vtkSetMacro(MILSystemNumber,int);
  vtkGetMacro(MILSystemNumber,int);

  // Description:
  // Set the DCF filename for non-standard video formats
  vtkSetStringMacro(MILDigitizerDCF);
  vtkGetStringMacro(MILDigitizerDCF);

  // Description:
  // Set the digitizer number for systems with multiple digitizers
  vtkSetMacro(MILDigitizerNumber,int);
  vtkGetMacro(MILDigitizerNumber,int);

  // Description:
  // Set whether to display MIL error messages (default on)
  virtual void SetMILErrorMessages(int yesno);
  vtkBooleanMacro(MILErrorMessages,int);
  vtkGetMacro(MILErrorMessages,int);

  // Description:
  // Allows fine-grained control 
  vtkSetMacro(MILAppID,long);
  vtkGetMacro(MILAppID,long);
  vtkSetMacro(MILSysID,long);
  vtkGetMacro(MILSysID,long);
  vtkGetMacro(MILDigID,long);
  vtkGetMacro(MILBufID,long);

  // Description:
  // Initialize the driver (this is called automatically when the
  // first grab is done).
  void Initialize();

  // Description:
  // Free the driver (this is called automatically inside the
  // destructor).
  void ReleaseSystemResources();

  // Description:
  // For internal use only
  void *OldHookFunction;
  void *OldUserDataPtr;
  int FrameCounter;
  int ForceGrab;
  void InternalGrab();

protected:
  vtkMILVideoSource();
  ~vtkMILVideoSource();
  vtkMILVideoSource(const vtkMILVideoSource&);
  void operator=(const vtkMILVideoSource&);

  virtual void AllocateMILDigitizer();
  virtual void AllocateMILBuffer();

  virtual void *MILInterpreterForSystem(int system);
  char *MILInterpreterDLL;

  int VideoChannel;
  int VideoInput;
  int VideoInputForColor;
  int VideoFormat;

  float ContrastLevel;
  float BrightnessLevel;
  float HueLevel;
  float SaturationLevel;

  int FrameMaxSize[2];

  long MILAppID;
  long MILSysID;
  long MILDigID;
  long MILBufID;
  // long MILDispBufID;
  // long MILDispID;

  int MILSystemType;
  int MILSystemNumber;

  int MILDigitizerNumber;
  char *MILDigitizerDCF;

  int MILErrorMessages;

  int MILAppInternallyAllocated;
  int MILSysInternallyAllocated;

  int FatalMILError;
};

#endif
