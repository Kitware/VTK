/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMILVideoSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMILVideoSource - Matrox Imaging Library frame grabbers
// .SECTION Description
// vtkMILVideoSource provides an interface to Matrox Meteor, MeteorII
// and Corona video digitizers through the Matrox Imaging Library 
// interface.  In order to use this class, you must link VTK with mil.lib,
// MIL version 5.0 or higher is required.
// .SECTION Caveats
// With some capture cards, if this class is leaked and ReleaseSystemResources 
// is not called, you may have to reboot before you can capture again.
// vtkVideoSource used to keep a global list and delete the video sources
// if your program leaked, due to exit crashes that was removed.
// .SECTION See Also
// vtkWin32VideoSource vtkVideoSource

#ifndef __vtkMILVideoSource_h
#define __vtkMILVideoSource_h

#include "vtkVideoSource.h"

// digitizer hardware
#define VTK_MIL_DEFAULT        0
#define VTK_MIL_METEOR         "M_SYSTEM_METEOR"
#define VTK_MIL_METEOR_II      "M_SYSTEM_METEOR_II"
#define VTK_MIL_METEOR_II_DIG  "M_SYSTEM_METEOR_II_DIG"
#define VTK_MIL_METEOR_II_CL   "M_SYSTEM_METEOR_II_CL"
#define VTK_MIL_METEOR_II_1394 "M_SYSTEM_METEOR_II_1394"
#define VTK_MIL_CORONA         "M_SYSTEM_CORONA"
#define VTK_MIL_CORONA_II      "M_SYSTEM_CORONA_II"
#define VTK_MIL_PULSAR         "M_SYSTEM_PULSAR"
#define VTK_MIL_GENESIS        "M_SYSTEM_GENESIS"
#define VTK_MIL_GENESIS_PLUS   "M_SYSTEM_GENESIS_PLUS"
#define VTK_MIL_ORION          "M_SYSTEM_ORION"
#define VTK_MIL_CRONOS         "M_SYSTEM_CRONOS"
#define VTK_MIL_ODYSSEY        "M_SYSTEM_ODYSSEY"

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

class VTK_HYBRID_EXPORT vtkMILVideoSource : public vtkVideoSource
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
  // Set/Get the video levels for composite/SVideo: the valid ranges are: 
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
  // Set/Get the video levels for monochrome/RGB: valid values are
  // between 0.0 and 255.0. 
  virtual void SetBlackLevel(float value);
  virtual float GetBlackLevel() {
    return this->BlackLevel; };
  virtual void SetWhiteLevel(float value);
  virtual float GetWhiteLevel() {
    return this->WhiteLevel; };

  // Description:
  // Set the system which you want use.  If you don't specify a system,
  // then your primary digitizer will be autodetected.
  vtkSetStringMacro(MILSystemType);
  vtkGetStringMacro(MILSystemType);
  void SetMILSystemTypeToMeteor() { this->SetMILSystemType(VTK_MIL_METEOR); };
  void SetMILSystemTypeToMeteorII() { this->SetMILSystemType(VTK_MIL_METEOR_II); };
  void SetMILSystemTypeToMeteorIIDig() { this->SetMILSystemType(VTK_MIL_METEOR_II_DIG); };
  void SetMILSystemTypeToMeteorIICL() { this->SetMILSystemType(VTK_MIL_METEOR_II_CL); };
  void SetMILSystemTypeToMeteorII1394() { this->SetMILSystemType(VTK_MIL_METEOR_II_1394); };
  void SetMILSystemTypeToCorona() { this->SetMILSystemType(VTK_MIL_CORONA); };
  void SetMILSystemTypeToCoronaII() { this->SetMILSystemType(VTK_MIL_CORONA_II); };
  void SetMILSystemTypeToPulsar() { this->SetMILSystemType(VTK_MIL_PULSAR); };
  void SetMILSystemTypeToGenesis() { this->SetMILSystemType(VTK_MIL_GENESIS); };
  void SetMILSystemTypeToGenesisPlus() { this->SetMILSystemType(VTK_MIL_GENESIS_PLUS); };
  void SetMILSystemTypeToOrion() { this->SetMILSystemType(VTK_MIL_ORION); };
  void SetMILSystemTypeToCronos() { this->SetMILSystemType(VTK_MIL_CRONOS); };
  void SetMILSystemTypeToOdyssey() { this->SetMILSystemType(VTK_MIL_ODYSSEY); };
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

  virtual void AllocateMILDigitizer();
  virtual void AllocateMILBuffer();

  virtual char *MILInterpreterForSystem(const char *system);
  char *MILInterpreterDLL;

  int VideoChannel;
  int VideoInput;
  int VideoInputForColor;
  int VideoFormat;

  float ContrastLevel;
  float BrightnessLevel;
  float HueLevel;
  float SaturationLevel;

  float BlackLevel;
  float WhiteLevel;

  int FrameMaxSize[2];

  long MILAppID;
  long MILSysID;
  long MILDigID;
  long MILBufID;
  // long MILDispBufID;
  // long MILDispID;

  char *MILSystemType;
  int MILSystemNumber;

  int MILDigitizerNumber;
  char *MILDigitizerDCF;

  int MILErrorMessages;

  int MILAppInternallyAllocated;
  int MILSysInternallyAllocated;

  int FatalMILError;

  // Description:
  // Method for updating the virtual clock that accurately times the
  // arrival of each frame, more accurately than is possible with
  // the system clock alone because the virtual clock averages out the
  // jitter.
  double CreateTimeStampForFrame(unsigned long frame);

  double LastTimeStamp;
  unsigned long LastFrameCount;
  double EstimatedFramePeriod;
  double NextFramePeriod;

private:
  vtkMILVideoSource(const vtkMILVideoSource&);  // Not implemented.
  void operator=(const vtkMILVideoSource&);  // Not implemented.
};

#endif
