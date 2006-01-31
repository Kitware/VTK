/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMedicalImageProperties.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMedicalImageProperties - some medical image properties.
// .SECTION Description
// vtkMedicalImageProperties is a helper class that can be used by medical
// image readers and applications to encapsulate medical image/acquisition
// properties. Later on, this should probably be extended to add
// any user-defined property.
// .SECTION See Also
// vtkMedicalImageReader2

#ifndef __vtkMedicalImageProperties_h
#define __vtkMedicalImageProperties_h

#include "vtkObject.h"

class vtkMedicalImagePropertiesInternals;

class VTK_IO_EXPORT vtkMedicalImageProperties : public vtkObject
{
public:
  static vtkMedicalImageProperties *New();
  vtkTypeRevisionMacro(vtkMedicalImageProperties,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);   

  // Description:
  // Convenience method to reset all fields to an emptry string/value
  virtual void Clear();

  // Description:
  // Patient name
  // For ex: DICOM (0010,0010) = DOE,JOHN
  vtkSetStringMacro(PatientName);
  vtkGetStringMacro(PatientName);

  // Description:
  // Patient ID
  // For ex: DICOM (0010,0020) = 1933197
  vtkSetStringMacro(PatientID);
  vtkGetStringMacro(PatientID);

  // Description:
  // Patient age
  // Format: nnnD, nnW, nnnM or nnnY (eventually nnD, nnW, nnY)
  //         with D (day), M (month), W (week), Y (year)
  // For ex: DICOM (0010,1010) = 031Y
  vtkSetStringMacro(PatientAge);
  vtkGetStringMacro(PatientAge);

  // Description:
  // Patient sex
  // For ex: DICOM (0010,0040) = M
  vtkSetStringMacro(PatientSex);
  vtkGetStringMacro(PatientSex);

  // Description:
  // Patient birth date
  // Format: yyyymmdd
  // For ex: DICOM (0010,0030) = 19680427
  vtkSetStringMacro(PatientBirthDate);
  vtkGetStringMacro(PatientBirthDate);

  // For Tcl:
  // From C++ use GetPatientBirthDate + GetDateAsFields
  int GetPatientBirthDateYear();
  int GetPatientBirthDateMonth();
  int GetPatientBirthDateDay();

  // Description:
  // Acquisition Date
  // Format: yyyymmdd
  // For ex: DICOM (0008,0022) = 20030617
  vtkSetStringMacro(AcquisitionDate);
  vtkGetStringMacro(AcquisitionDate);

  // For Tcl:
  // From C++ use GetAcquisitionDate + GetDateAsFields
  int GetAcquisitionDateYear();
  int GetAcquisitionDateMonth();
  int GetAcquisitionDateDay();

  // Description:
  // Acquisition time
  // Format: hhmmss.frac (any trailing component(s) can be ommited)
  // For ex: DICOM (0008,0032) = 162552.0705 or 230012, or 0012
  vtkSetStringMacro(AcquisitionTime);
  vtkGetStringMacro(AcquisitionTime);

  // Description:
  // Image Date
  // Format: yyyymmdd
  // For ex: DICOM (0008,0023) = 20030617
  vtkSetStringMacro(ImageDate);
  vtkGetStringMacro(ImageDate);

  // For Tcl:
  // From C++ use GetImageDate + GetDateAsFields
  int GetImageDateYear();
  int GetImageDateMonth();
  int GetImageDateDay();
  
  // Description:
  // Take as input a string in ISO 8601 date (YYYY/MM/DD) and extract the different fields
  // namely: year month day
  // Return 0 on error, 1 on success
  static int GetDateAsFields(const char *date, int &year, int &month, int &day);

  // Description:
  // Image Time
  // Format: hhmmss.frac (any trailing component(s) can be ommited)
  // For ex: DICOM (0008,0033) = 162552.0705 or 230012, or 0012
  vtkSetStringMacro(ImageTime);
  vtkGetStringMacro(ImageTime);

  // Description:
  // Image number
  // For ex: DICOM (0020,0013) = 1
  vtkSetStringMacro(ImageNumber);
  vtkGetStringMacro(ImageNumber);

  // Description:
  // Series number
  // For ex: DICOM (0020,0011) = 902
  vtkSetStringMacro(SeriesNumber);
  vtkGetStringMacro(SeriesNumber);

  // Description:
  // Study ID
  // For ex: DICOM (0020,0010) = 37481
  vtkSetStringMacro(StudyID);
  vtkGetStringMacro(StudyID);

  // Description:
  // Study description
  // For ex: DICOM (0008,1030) = BRAIN/C-SP/FACIAL
  vtkSetStringMacro(StudyDescription);
  vtkGetStringMacro(StudyDescription);

  // Description:
  // Modality
  // For ex: DICOM (0008,0060)= CT
  vtkSetStringMacro(Modality);
  vtkGetStringMacro(Modality);

  // Description:
  // Manufacturer's Model Name
  // For ex: DICOM (0008,1090) = LightSpeed QX/i
  vtkSetStringMacro(ManufacturerModelName);
  vtkGetStringMacro(ManufacturerModelName);

  // Description:
  // Station Name
  // For ex: DICOM (0008,1010) = LSPD_OC8
  vtkSetStringMacro(StationName);
  vtkGetStringMacro(StationName);

  // Description:
  // Institution Name
  // For ex: DICOM (0008,0080) = FooCity Medical Center
  vtkSetStringMacro(InstitutionName);
  vtkGetStringMacro(InstitutionName);

  // Description:
  // Convolution Kernel (or algorithm used to reconstruct the data)
  // For ex: DICOM (0018,1210) = Bone
  vtkSetStringMacro(ConvolutionKernel);
  vtkGetStringMacro(ConvolutionKernel);

  // Description:
  // Slice Thickness
  // For ex: DICOM (0018,0050) = 0.273438
  vtkSetStringMacro(SliceThickness);
  vtkGetStringMacro(SliceThickness);
  virtual double GetSliceThicknessAsDouble();

  // Description:
  // Peak kilo voltage output of the (x-ray) generator used
  // For ex: DICOM (0018,0060) = 120
  vtkSetStringMacro(KVP);
  vtkGetStringMacro(KVP);

  // Description:
  // Gantry/Detector tilt (Nominal angle of tilt in degrees of the scanning
  // gantry.)
  // For ex: DICOM (0018,1120) = 15
  vtkSetStringMacro(GantryTilt);
  vtkGetStringMacro(GantryTilt);
  virtual double GetGantryTiltAsDouble();

  // Description:
  // Exposure time (time of x-ray exposure in msec)
  // For ex: DICOM (0018,1150) = 5
  vtkSetStringMacro(ExposureTime);
  vtkGetStringMacro(ExposureTime);

  // Description:
  // X-ray tube current (in mA)
  // For ex: DICOM (0018,1151) = 400
  vtkSetStringMacro(XRayTubeCurrent);
  vtkGetStringMacro(XRayTubeCurrent);

  // Description:
  // Exposure (The exposure expressed in mAs, for example calculated 
  // from Exposure Time and X-ray Tube Current)
  // For ex: DICOM (0018,1152) = 114
  vtkSetStringMacro(Exposure);
  vtkGetStringMacro(Exposure);

  // Description:
  // Copy the contents of p to this instance. 
  virtual void DeepCopy(vtkMedicalImageProperties *p);

  // Description:
  // Add/Remove/Query the window/level presets that may have been associated
  // to a medical image. Window is also known as 'width', level is also known
  // as 'center'. The same window/level pair can not be added twice.
  // As a convenience, a comment can be associated to a preset.
  // For ex: DICOM Window Center (0028,1050) = 00045\000470
  //         DICOM Window Width  (0028,1051) = 0106\03412
  virtual void AddWindowLevelPreset(double w, double l);
  virtual void RemoveWindowLevelPreset(double w, double l);
  virtual void RemoveAllWindowLevelPresets();
  virtual int GetNumberOfWindowLevelPresets();
  virtual int HasWindowLevelPreset(double w, double l);
  virtual int GetNthWindowLevelPreset(int idx, double *w, double *l);
  virtual double* GetNthWindowLevelPreset(int idx);
  virtual void SetNthWindowLevelPresetComment(int idx, const char *comment);
  virtual const char* GetNthWindowLevelPresetComment(int idx);

protected:
  vtkMedicalImageProperties();
  ~vtkMedicalImageProperties();

  char *AcquisitionDate;
  char *AcquisitionTime;
  char *ConvolutionKernel;
  char *Exposure;
  char *ExposureTime;
  char *GantryTilt;
  char *ImageDate;
  char *ImageNumber;
  char *ImageTime;
  char *InstitutionName;
  char *KVP;
  char *ManufacturerModelName;
  char *Modality;
  char *PatientAge;
  char *PatientBirthDate;
  char *PatientID;
  char *PatientName;
  char *PatientSex;
  char *SeriesNumber;
  char *SliceThickness;
  char *StationName;
  char *StudyDescription;
  char *StudyID;
  char *XRayTubeCurrent;

  // Description:
  // PIMPL Encapsulation for STL containers
  //BTX
  vtkMedicalImagePropertiesInternals *Internals;
  //ETX
  
private:
  vtkMedicalImageProperties(const vtkMedicalImageProperties&); // Not implemented.
  void operator=(const vtkMedicalImageProperties&); // Not implemented.
};

#endif
