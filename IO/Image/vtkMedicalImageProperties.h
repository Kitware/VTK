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
/**
 * @class   vtkMedicalImageProperties
 * @brief   some medical image properties.
 *
 * vtkMedicalImageProperties is a helper class that can be used by medical
 * image readers and applications to encapsulate medical image/acquisition
 * properties. Later on, this should probably be extended to add
 * any user-defined property.
 * @sa
 * vtkMedicalImageReader2
*/

#ifndef vtkMedicalImageProperties_h
#define vtkMedicalImageProperties_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkObject.h"

class vtkMedicalImagePropertiesInternals;

class VTKIOIMAGE_EXPORT vtkMedicalImageProperties : public vtkObject
{
public:
  static vtkMedicalImageProperties *New();
  vtkTypeMacro(vtkMedicalImageProperties,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Convenience method to reset all fields to an emptry string/value
   */
  virtual void Clear();

  //@{
  /**
   * Patient name
   * For ex: DICOM (0010,0010) = DOE,JOHN
   */
  vtkSetStringMacro(PatientName);
  vtkGetStringMacro(PatientName);
  //@}

  //@{
  /**
   * Patient ID
   * For ex: DICOM (0010,0020) = 1933197
   */
  vtkSetStringMacro(PatientID);
  vtkGetStringMacro(PatientID);
  //@}

  //@{
  /**
   * Patient age
   * Format: nnnD, nnW, nnnM or nnnY (eventually nnD, nnW, nnY)
   * with D (day), M (month), W (week), Y (year)
   * For ex: DICOM (0010,1010) = 031Y
   */
  vtkSetStringMacro(PatientAge);
  vtkGetStringMacro(PatientAge);
  //@}

  /**
   * Take as input a string in VR=AS (DICOM PS3.5) and extract either
   * different fields namely: year month week day
   * Return 0 on error, 1 on success
   * One can test fields if they are different from -1 upon success
   */
  static int GetAgeAsFields(const char *age, int &year, int &month, int &week, int &day);

  // For Tcl:
  // From C++ use GetPatientAge + GetAgeAsField
  // Those function parse a DICOM string, and return the value of the number
  // expressed this is either expressed in year, month or days. Thus if a
  // string is expressed in years
  // GetPatientAgeDay/GetPatientAgeWeek/GetPatientAgeMonth will return 0
  int GetPatientAgeYear();
  int GetPatientAgeMonth();
  int GetPatientAgeWeek();
  int GetPatientAgeDay();

  //@{
  /**
   * Patient sex
   * For ex: DICOM (0010,0040) = M
   */
  vtkSetStringMacro(PatientSex);
  vtkGetStringMacro(PatientSex);
  //@}

  //@{
  /**
   * Patient birth date
   * Format: yyyymmdd
   * For ex: DICOM (0010,0030) = 19680427
   */
  vtkSetStringMacro(PatientBirthDate);
  vtkGetStringMacro(PatientBirthDate);
  //@}

  // For Tcl:
  // From C++ use GetPatientBirthDate + GetDateAsFields
  int GetPatientBirthDateYear();
  int GetPatientBirthDateMonth();
  int GetPatientBirthDateDay();

  //@{
  /**
   * Study Date
   * Format: yyyymmdd
   * For ex: DICOM (0008,0020) = 20030617
   */
  vtkSetStringMacro(StudyDate);
  vtkGetStringMacro(StudyDate);
  //@}

  //@{
  /**
   * Acquisition Date
   * Format: yyyymmdd
   * For ex: DICOM (0008,0022) = 20030617
   */
  vtkSetStringMacro(AcquisitionDate);
  vtkGetStringMacro(AcquisitionDate);
  //@}

  // For Tcl:
  // From C++ use GetAcquisitionDate + GetDateAsFields
  int GetAcquisitionDateYear();
  int GetAcquisitionDateMonth();
  int GetAcquisitionDateDay();

  //@{
  /**
   * Study Time
   * Format: hhmmss.frac (any trailing component(s) can be omitted)
   * For ex: DICOM (0008,0030) = 162552.0705 or 230012, or 0012
   */
  vtkSetStringMacro(StudyTime);
  vtkGetStringMacro(StudyTime);
  //@}

  //@{
  /**
   * Acquisition time
   * Format: hhmmss.frac (any trailing component(s) can be omitted)
   * For ex: DICOM (0008,0032) = 162552.0705 or 230012, or 0012
   */
  vtkSetStringMacro(AcquisitionTime);
  vtkGetStringMacro(AcquisitionTime);
  //@}

  //@{
  /**
   * Image Date aka Content Date
   * Format: yyyymmdd
   * For ex: DICOM (0008,0023) = 20030617
   */
  vtkSetStringMacro(ImageDate);
  vtkGetStringMacro(ImageDate);
  //@}

  // For Tcl:
  // From C++ use GetImageDate + GetDateAsFields
  int GetImageDateYear();
  int GetImageDateMonth();
  int GetImageDateDay();

  /**
   * Take as input a string in ISO 8601 date (YYYY/MM/DD) and extract the
   * different fields namely: year month day
   * Return 0 on error, 1 on success
   */
  static int GetDateAsFields(const char *date, int &year, int &month, int &day);

  /**
   * Take as input a string in VR:TM format (HHMMSS) and extract the
   * different fields namely: hour, minute and second
   * Return 0 on error, 1 on success
   */
  static int GetTimeAsFields(const char *time, int &hour, int &minute, int &second /* , long &milliseconds */);

  /**
   * Take as input a string in ISO 8601 date (YYYY/MM/DD) and construct a
   * locale date based on the different fields (see GetDateAsFields to extract
   * different fields)
   * Return 0 on error, 1 on success
   */
  static int GetDateAsLocale(const char *date, char *locale);

  //@{
  /**
   * Image Time
   * Format: hhmmss.frac (any trailing component(s) can be omitted)
   * For ex: DICOM (0008,0033) = 162552.0705 or 230012, or 0012
   */
  vtkSetStringMacro(ImageTime);
  vtkGetStringMacro(ImageTime);
  //@}

  //@{
  /**
   * Image number
   * For ex: DICOM (0020,0013) = 1
   */
  vtkSetStringMacro(ImageNumber);
  vtkGetStringMacro(ImageNumber);
  //@}

  //@{
  /**
   * Series number
   * For ex: DICOM (0020,0011) = 902
   */
  vtkSetStringMacro(SeriesNumber);
  vtkGetStringMacro(SeriesNumber);
  //@}

  //@{
  /**
   * Series Description
   * User provided description of the Series
   * For ex: DICOM (0008,103e) = SCOUT
   */
  vtkSetStringMacro(SeriesDescription);
  vtkGetStringMacro(SeriesDescription);
  //@}

  //@{
  /**
   * Study ID
   * For ex: DICOM (0020,0010) = 37481
   */
  vtkSetStringMacro(StudyID);
  vtkGetStringMacro(StudyID);
  //@}

  //@{
  /**
   * Study description
   * For ex: DICOM (0008,1030) = BRAIN/C-SP/FACIAL
   */
  vtkSetStringMacro(StudyDescription);
  vtkGetStringMacro(StudyDescription);
  //@}

  //@{
  /**
   * Modality
   * For ex: DICOM (0008,0060)= CT
   */
  vtkSetStringMacro(Modality);
  vtkGetStringMacro(Modality);
  //@}

  //@{
  /**
   * Manufacturer
   * For ex: DICOM (0008,0070) = Siemens
   */
  vtkSetStringMacro(Manufacturer);
  vtkGetStringMacro(Manufacturer);
  //@}

  //@{
  /**
   * Manufacturer's Model Name
   * For ex: DICOM (0008,1090) = LightSpeed QX/i
   */
  vtkSetStringMacro(ManufacturerModelName);
  vtkGetStringMacro(ManufacturerModelName);
  //@}

  //@{
  /**
   * Station Name
   * For ex: DICOM (0008,1010) = LSPD_OC8
   */
  vtkSetStringMacro(StationName);
  vtkGetStringMacro(StationName);
  //@}

  //@{
  /**
   * Institution Name
   * For ex: DICOM (0008,0080) = FooCity Medical Center
   */
  vtkSetStringMacro(InstitutionName);
  vtkGetStringMacro(InstitutionName);
  //@}

  //@{
  /**
   * Convolution Kernel (or algorithm used to reconstruct the data)
   * For ex: DICOM (0018,1210) = Bone
   */
  vtkSetStringMacro(ConvolutionKernel);
  vtkGetStringMacro(ConvolutionKernel);
  //@}

  //@{
  /**
   * Slice Thickness (Nominal reconstructed slice thickness, in mm)
   * For ex: DICOM (0018,0050) = 0.273438
   */
  vtkSetStringMacro(SliceThickness);
  vtkGetStringMacro(SliceThickness);
  virtual double GetSliceThicknessAsDouble();
  //@}

  //@{
  /**
   * Peak kilo voltage output of the (x-ray) generator used
   * For ex: DICOM (0018,0060) = 120
   */
  vtkSetStringMacro(KVP);
  vtkGetStringMacro(KVP);
  //@}

  //@{
  /**
   * Gantry/Detector tilt (Nominal angle of tilt in degrees of the scanning
   * gantry.)
   * For ex: DICOM (0018,1120) = 15
   */
  vtkSetStringMacro(GantryTilt);
  vtkGetStringMacro(GantryTilt);
  virtual double GetGantryTiltAsDouble();
  //@}

  //@{
  /**
   * Echo Time
   * (Time in ms between the middle of the excitation pulse and the peak of
   * the echo produced)
   * For ex: DICOM (0018,0081) = 105
   */
  vtkSetStringMacro(EchoTime);
  vtkGetStringMacro(EchoTime);
  //@}

  //@{
  /**
   * Echo Train Length
   * (Number of lines in k-space acquired per excitation per image)
   * For ex: DICOM (0018,0091) = 35
   */
  vtkSetStringMacro(EchoTrainLength);
  vtkGetStringMacro(EchoTrainLength);
  //@}

  //@{
  /**
   * Repetition Time
   * The period of time in msec between the beginning of a pulse sequence and
   * the beginning of the succeeding (essentially identical) pulse sequence.
   * For ex: DICOM (0018,0080) = 2040
   */
  vtkSetStringMacro(RepetitionTime);
  vtkGetStringMacro(RepetitionTime);
  //@}

  //@{
  /**
   * Exposure time (time of x-ray exposure in msec)
   * For ex: DICOM (0018,1150) = 5
   */
  vtkSetStringMacro(ExposureTime);
  vtkGetStringMacro(ExposureTime);
  //@}

  //@{
  /**
   * X-ray tube current (in mA)
   * For ex: DICOM (0018,1151) = 400
   */
  vtkSetStringMacro(XRayTubeCurrent);
  vtkGetStringMacro(XRayTubeCurrent);
  //@}

  //@{
  /**
   * Exposure (The exposure expressed in mAs, for example calculated
   * from Exposure Time and X-ray Tube Current)
   * For ex: DICOM (0018,1152) = 114
   */
  vtkSetStringMacro(Exposure);
  vtkGetStringMacro(Exposure);
  //@}

  //@{
  /**
   * Get the direction cosine (default to 1,0,0,0,1,0)
   */
  vtkSetVector6Macro(DirectionCosine,double);
  vtkGetVector6Macro(DirectionCosine,double);
  //@}

  // Interface to allow insertion of user define values, for instance in DICOM
  // one would want to
  // store the Protocol Name (0018,1030), in this case one would do:
  // AddUserDefinedValue( "Protocol Name", "T1W/SE/1024" );
  virtual void AddUserDefinedValue(const char *name, const char *value);
  virtual const char *GetUserDefinedValue(const char *name);
  virtual unsigned int GetNumberOfUserDefinedValues();
  virtual const char *GetUserDefinedNameByIndex(unsigned int idx);
  virtual const char *GetUserDefinedValueByIndex(unsigned int idx);
  virtual void RemoveAllUserDefinedValues();

  //@{
  /**
   * Add/Remove/Query the window/level presets that may have been associated
   * to a medical image. Window is also known as 'width', level is also known
   * as 'center'. The same window/level pair can not be added twice.
   * As a convenience, a comment (aka Explanation) can be associated to
   * a preset.
   * For ex:
   * \verbatim
   * DICOM Window Center (0028,1050) = 00045\000470
   * DICOM Window Width  (0028,1051) = 0106\03412
   * DICOM Window Center Width Explanation (0028,1055) = WINDOW1\WINDOW2
   * \endverbatim
   */
  virtual int AddWindowLevelPreset(double w, double l);
  virtual void RemoveWindowLevelPreset(double w, double l);
  virtual void RemoveAllWindowLevelPresets();
  virtual int GetNumberOfWindowLevelPresets();
  virtual int HasWindowLevelPreset(double w, double l);
  virtual int GetWindowLevelPresetIndex(double w, double l);
  virtual int GetNthWindowLevelPreset(int idx, double *w, double *l);
  virtual double* GetNthWindowLevelPreset(int idx);
  virtual void SetNthWindowLevelPresetComment(int idx, const char *comment);
  virtual const char* GetNthWindowLevelPresetComment(int idx);
  //@}

  //@{
  /**
   * Mapping from a sliceidx within a volumeidx into a DICOM Instance UID
   * Some DICOM reader can populate this structure so that later on from
   * a slice index in a vtkImageData volume we can backtrack and find out
   * which 2d slice it was coming from
   */
  const char *GetInstanceUIDFromSliceID(int volumeidx, int sliceid);
  void SetInstanceUIDFromSliceID(int volumeidx, int sliceid, const char *uid);
  //@}

  /**
   * Provides the inverse mapping. Returns -1 if a slice for this uid is
   * not found.
   */
  int GetSliceIDFromInstanceUID(int &volumeidx, const char *uid);

  typedef enum {
    AXIAL = 0,
    CORONAL,
    SAGITTAL
  } OrientationType;

  int GetOrientationType(int volumeidx);
  void SetOrientationType(int volumeidx, int orientation);
  static const char *GetStringFromOrientationType(unsigned int type);

  /**
   * Copy the contents of p to this instance.
   */
  virtual void DeepCopy(vtkMedicalImageProperties *p);

protected:
  vtkMedicalImageProperties();
  ~vtkMedicalImageProperties();

  char *StudyDate;
  char *AcquisitionDate;
  char *StudyTime;
  char *AcquisitionTime;
  char *ConvolutionKernel;
  char *EchoTime;
  char *EchoTrainLength;
  char *Exposure;
  char *ExposureTime;
  char *GantryTilt;
  char *ImageDate;
  char *ImageNumber;
  char *ImageTime;
  char *InstitutionName;
  char *KVP;
  char *ManufacturerModelName;
  char *Manufacturer;
  char *Modality;
  char *PatientAge;
  char *PatientBirthDate;
  char *PatientID;
  char *PatientName;
  char *PatientSex;
  char *RepetitionTime;
  char *SeriesDescription;
  char *SeriesNumber;
  char *SliceThickness;
  char *StationName;
  char *StudyDescription;
  char *StudyID;
  char *XRayTubeCurrent;
  double DirectionCosine[6];

  /**
   * PIMPL Encapsulation for STL containers
   */
  vtkMedicalImagePropertiesInternals *Internals;

private:
  vtkMedicalImageProperties(const vtkMedicalImageProperties&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMedicalImageProperties&) VTK_DELETE_FUNCTION;
};

#endif
