/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMedicalImageProperties.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMedicalImageProperties.h"
#include "vtkObjectFactory.h"

#include <vtksys/stl/string>
#include <vtksys/stl/vector>
#include <time.h> // for strftime

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkMedicalImageProperties, "1.17");
vtkStandardNewMacro(vtkMedicalImageProperties);

//----------------------------------------------------------------------------
class vtkMedicalImagePropertiesInternals
{
public:
  class WindowLevelPreset
  {
  public:
    double Window;
    double Level;
    vtksys_stl::string Comment;
  };

  typedef vtkstd::vector<WindowLevelPreset> WindowLevelPresetPoolType;
  typedef vtkstd::vector<WindowLevelPreset>::iterator WindowLevelPresetPoolIterator;

  WindowLevelPresetPoolType WindowLevelPresetPool;
};

//----------------------------------------------------------------------------
vtkMedicalImageProperties::vtkMedicalImageProperties()
{
  this->Internals = new vtkMedicalImagePropertiesInternals;

  this->AcquisitionDate        = NULL;
  this->AcquisitionTime        = NULL;
  this->ConvolutionKernel      = NULL;
  this->EchoTime               = NULL;
  this->EchoTrainLength        = NULL;
  this->Exposure               = NULL;
  this->ExposureTime           = NULL;
  this->GantryTilt             = NULL;
  this->ImageDate              = NULL;
  this->ImageNumber            = NULL;
  this->ImageTime              = NULL;
  this->InstitutionName        = NULL;
  this->KVP                    = NULL;
  this->ManufacturerModelName  = NULL;
  this->Modality               = NULL;
  this->PatientAge             = NULL;
  this->PatientBirthDate       = NULL;
  this->PatientID              = NULL;
  this->PatientName            = NULL;
  this->PatientSex             = NULL;
  this->RepetitionTime         = NULL;
  this->SeriesDescription      = NULL;
  this->SeriesNumber           = NULL;
  this->SliceThickness         = NULL;
  this->StationName            = NULL;
  this->StudyDescription       = NULL;
  this->StudyID                = NULL;
  this->XRayTubeCurrent        = NULL;
}

//----------------------------------------------------------------------------
vtkMedicalImageProperties::~vtkMedicalImageProperties()
{
  if (this->Internals)
    {
    delete this->Internals;
    this->Internals = NULL;
    }

  this->Clear();
}

//----------------------------------------------------------------------------
void vtkMedicalImageProperties::Clear()
{
  this->SetAcquisitionDate(NULL);
  this->SetAcquisitionTime(NULL);
  this->SetConvolutionKernel(NULL);
  this->SetEchoTime(NULL);
  this->SetEchoTrainLength(NULL);
  this->SetExposure(NULL);
  this->SetExposureTime(NULL);
  this->SetGantryTilt(NULL);
  this->SetImageDate(NULL);
  this->SetImageNumber(NULL);
  this->SetImageTime(NULL);
  this->SetInstitutionName(NULL);
  this->SetKVP(NULL);
  this->SetManufacturerModelName(NULL);
  this->SetModality(NULL);
  this->SetPatientAge(NULL);
  this->SetPatientBirthDate(NULL);
  this->SetPatientID(NULL);
  this->SetPatientName(NULL);
  this->SetPatientSex(NULL);
  this->SetRepetitionTime(NULL);
  this->SetSeriesDescription(NULL);
  this->SetSeriesNumber(NULL);
  this->SetSliceThickness(NULL);
  this->SetStationName(NULL);
  this->SetStudyDescription(NULL);
  this->SetStudyID(NULL);
  this->SetXRayTubeCurrent(NULL);

  this->RemoveAllWindowLevelPresets();
}

//----------------------------------------------------------------------------
void vtkMedicalImageProperties::DeepCopy(vtkMedicalImageProperties *p)
{
  if (p == NULL)
    {
    return;
    }

  this->Clear();

  this->SetAcquisitionDate(p->GetAcquisitionDate());
  this->SetAcquisitionTime(p->GetAcquisitionTime());
  this->SetConvolutionKernel(p->GetConvolutionKernel());
  this->SetEchoTime(p->GetEchoTime());
  this->SetEchoTrainLength(p->GetEchoTrainLength());
  this->SetExposure(p->GetExposure());
  this->SetExposureTime(p->GetExposureTime());
  this->SetGantryTilt(p->GetGantryTilt());
  this->SetImageDate(p->GetImageDate());
  this->SetImageNumber(p->GetImageNumber());
  this->SetImageTime(p->GetImageTime());
  this->SetInstitutionName(p->GetInstitutionName());
  this->SetKVP(p->GetKVP());
  this->SetManufacturerModelName(p->GetManufacturerModelName());
  this->SetModality(p->GetModality());
  this->SetPatientAge(p->GetPatientAge());
  this->SetPatientBirthDate(p->GetPatientBirthDate());
  this->SetPatientID(p->GetPatientID());
  this->SetPatientName(p->GetPatientName());
  this->SetPatientSex(p->GetPatientSex());
  this->SetRepetitionTime(p->GetRepetitionTime());
  this->SetSeriesDescription(p->GetSeriesDescription());
  this->SetSeriesNumber(p->GetSeriesNumber());
  this->SetSliceThickness(p->GetSliceThickness());
  this->SetStationName(p->GetStationName());
  this->SetStudyDescription(p->GetStudyDescription());
  this->SetStudyID(p->GetStudyID());
  this->SetXRayTubeCurrent(p->GetXRayTubeCurrent());

  int nb_presets = p->GetNumberOfWindowLevelPresets();
  for (int i = 0; i < nb_presets; i++)
    {
    double w, l;
    p->GetNthWindowLevelPreset(i, &w, &l);
    this->AddWindowLevelPreset(w, l);
    this->SetNthWindowLevelPresetComment(
      this->GetNumberOfWindowLevelPresets() - 1,
      p->GetNthWindowLevelPresetComment(i));
    }
}

//----------------------------------------------------------------------------
void vtkMedicalImageProperties::AddWindowLevelPreset(
  double w, double l)
{
  if (!this->Internals || this->HasWindowLevelPreset(w, l))
    {
    return;
    }

  vtkMedicalImagePropertiesInternals::WindowLevelPreset preset;
  preset.Window = w;
  preset.Level = l;
  this->Internals->WindowLevelPresetPool.push_back(preset);
}

//----------------------------------------------------------------------------
int vtkMedicalImageProperties::HasWindowLevelPreset(double w, double l)
{
  if (this->Internals)
    {
    vtkMedicalImagePropertiesInternals::WindowLevelPresetPoolIterator it =
      this->Internals->WindowLevelPresetPool.begin();
    vtkMedicalImagePropertiesInternals::WindowLevelPresetPoolIterator end =
      this->Internals->WindowLevelPresetPool.end();
    for (; it != end; ++it)
      {
      if ((*it).Window == w && (*it).Level == l)
        {
        return 1;
        }
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkMedicalImageProperties::RemoveWindowLevelPreset(double w, double l)
{
  if (this->Internals)
    {
    vtkMedicalImagePropertiesInternals::WindowLevelPresetPoolIterator it =
      this->Internals->WindowLevelPresetPool.begin();
    vtkMedicalImagePropertiesInternals::WindowLevelPresetPoolIterator end =
      this->Internals->WindowLevelPresetPool.end();
    for (; it != end; ++it)
      {
      if ((*it).Window == w && (*it).Level == l)
        {
        this->Internals->WindowLevelPresetPool.erase(it);
        break;
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkMedicalImageProperties::RemoveAllWindowLevelPresets()
{
  if (this->Internals)
    {
    this->Internals->WindowLevelPresetPool.clear();
    }
}

//----------------------------------------------------------------------------
int vtkMedicalImageProperties::GetNumberOfWindowLevelPresets()
{
  return this->Internals ? this->Internals->WindowLevelPresetPool.size() : 0;
}

//----------------------------------------------------------------------------
int vtkMedicalImageProperties::GetNthWindowLevelPreset(
  int idx, double *w, double *l)
{
  if (this->Internals &&
      idx >= 0 && idx < this->GetNumberOfWindowLevelPresets())
    {
    *w = this->Internals->WindowLevelPresetPool[idx].Window;
    *l = this->Internals->WindowLevelPresetPool[idx].Level;
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
double* vtkMedicalImageProperties::GetNthWindowLevelPreset(int idx)

{
  static double wl[2];
  if (this->GetNthWindowLevelPreset(idx, wl, wl + 1))
    {
    return wl;
    }
  return NULL;
}

//----------------------------------------------------------------------------
const char* vtkMedicalImageProperties::GetNthWindowLevelPresetComment(
  int idx)
{
  if (this->Internals &&
      idx >= 0 && idx < this->GetNumberOfWindowLevelPresets())
    {
    return this->Internals->WindowLevelPresetPool[idx].Comment.c_str();
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkMedicalImageProperties::SetNthWindowLevelPresetComment(
  int idx, const char *comment)
{
  if (this->Internals &&
      idx >= 0 && idx < this->GetNumberOfWindowLevelPresets())
    {
    this->Internals->WindowLevelPresetPool[idx].Comment =
      (comment ? comment : "");
    }
}

//----------------------------------------------------------------------------
double vtkMedicalImageProperties::GetSliceThicknessAsDouble()
{
  if (this->SliceThickness)
    {
    return atof(this->SliceThickness);
    }
  return 0;
}

//----------------------------------------------------------------------------
double vtkMedicalImageProperties::GetGantryTiltAsDouble()
{
  if (this->GantryTilt)
    {
    return atof(this->GantryTilt);
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkMedicalImageProperties::GetDateAsFields(const char *date, int &year,
  int &month, int &day)
{
  if( !date )
    {
    return 0;
    }

  size_t len = strlen(date);
  if( len == 8 )
    {
    // DICOM V3
    if( sscanf(date, "%04d%02d%02d", &year, &month, &day) != 3 )
      {
      return 0;
      }
    }
  else if( len == 10 )
    {
    // Some *very* old ACR-NEMA
    if( sscanf(date, "%04d.%02d.%02d", &year, &month, &day) != 3 )
      {
      return 0;
      }
    }
  else
    {
    return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
// Some  buggy versions of gcc complain about the use of %c: warning: `%c'
// yields only last 2 digits of year in some locales.  Of course  program-
// mers  are  encouraged  to  use %c, it gives the preferred date and time
// representation. One meets all kinds of strange obfuscations to  circum-
// vent this gcc problem. A relatively clean one is to add an intermediate
// function. This is described as bug #3190 in gcc bugzilla:
// [-Wformat-y2k doesn't belong to -Wall - it's hard to avoid]
inline size_t
my_strftime(char *s, size_t max, const char *fmt, const struct tm *tm)
{
  return strftime(s, max, fmt, tm);
}
// Helper function to convert a DICOM iso date format into a locale one
// locale buffer should be typically char locale[200]
int vtkMedicalImageProperties::GetDateAsLocale(const char *iso, char *locale)
{
  int year, month, day;
  if( vtkMedicalImageProperties::GetDateAsFields(iso, year, month, day) )
    {
    struct tm date;
    memset(&date,0, sizeof(date));
    date.tm_mday = day;
    // month are expressed in the [0-11] range:
    date.tm_mon = month - 1;
    // structure is date starting at 1900
    date.tm_year = year - 1900;
    my_strftime(locale, 200, "%x", &date);
    return 1;
    }
  return 0;
}
//----------------------------------------------------------------------------
int vtkMedicalImageProperties::GetPatientBirthDateYear()
{
  const char *date = this->GetPatientBirthDate();
  int year, month, day;
  vtkMedicalImageProperties::GetDateAsFields(date, year, month, day);
  return year;
}
//----------------------------------------------------------------------------
int vtkMedicalImageProperties::GetPatientBirthDateMonth()
{
  const char *date = this->GetPatientBirthDate();
  int year, month, day;
  vtkMedicalImageProperties::GetDateAsFields(date, year, month, day);
  return month;
}
//----------------------------------------------------------------------------
int vtkMedicalImageProperties::GetPatientBirthDateDay()
{
  const char *date = this->GetPatientBirthDate();
  int year, month, day;
  vtkMedicalImageProperties::GetDateAsFields(date, year, month, day);
  return day;
}
//----------------------------------------------------------------------------
int vtkMedicalImageProperties::GetAcquisitionDateYear()
{
  const char *date = this->GetAcquisitionDate();
  int year, month, day;
  vtkMedicalImageProperties::GetDateAsFields(date, year, month, day);
  return year;
}
//----------------------------------------------------------------------------
int vtkMedicalImageProperties::GetAcquisitionDateMonth()
{
  const char *date = this->GetAcquisitionDate();
  int year, month, day;
  vtkMedicalImageProperties::GetDateAsFields(date, year, month, day);
  return month;
}
//----------------------------------------------------------------------------
int vtkMedicalImageProperties::GetAcquisitionDateDay()
{
  const char *date = this->GetAcquisitionDate();
  int year, month, day;
  vtkMedicalImageProperties::GetDateAsFields(date, year, month, day);
  return day;
}
//----------------------------------------------------------------------------
int vtkMedicalImageProperties::GetImageDateYear()
{
  const char *date = this->GetImageDate();
  int year, month, day;
  vtkMedicalImageProperties::GetDateAsFields(date, year, month, day);
  return year;
}
//----------------------------------------------------------------------------
int vtkMedicalImageProperties::GetImageDateMonth()
{
  const char *date = this->GetImageDate();
  int year, month, day;
  vtkMedicalImageProperties::GetDateAsFields(date, year, month, day);
  return month;
}
//----------------------------------------------------------------------------
int vtkMedicalImageProperties::GetImageDateDay()
{
  const char *date = this->GetImageDate();
  int year, month, day;
  vtkMedicalImageProperties::GetDateAsFields(date, year, month, day);
  return day;
}

//----------------------------------------------------------------------------
void vtkMedicalImageProperties::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << "\n" << indent << "PatientName: ";
  if (this->PatientName)
    {
    os << this->PatientName;
    }

  os << "\n" << indent << "PatientID: ";
  if (this->PatientID)
    {
    os << this->PatientID;
    }

  os << "\n" << indent << "PatientAge: ";
  if (this->PatientAge)
    {
    os << this->PatientAge;
    }

  os << "\n" << indent << "PatientSex: ";
  if (this->PatientSex)
    {
    os << this->PatientSex;
    }

  os << "\n" << indent << "PatientBirthDate: ";
  if (this->PatientBirthDate)
    {
    os << this->PatientBirthDate;
    }

  os << "\n" << indent << "ImageDate: ";
  if (this->ImageDate)
    {
    os << this->ImageDate;
    }

  os << "\n" << indent << "ImageTime: ";
  if (this->ImageTime)
    {
    os << this->ImageTime;
    }

  os << "\n" << indent << "ImageNumber: ";
  if (this->ImageNumber)
    {
    os << this->ImageNumber;
    }

  os << "\n" << indent << "AcquisitionDate: ";
  if (this->AcquisitionDate)
    {
    os << this->AcquisitionDate;
    }

  os << "\n" << indent << "AcquisitionTime: ";
  if (this->AcquisitionTime)
    {
    os << this->AcquisitionTime;
    }

  os << "\n" << indent << "SeriesNumber: ";
  if (this->SeriesNumber)
    {
    os << this->SeriesNumber;
    }

  os << "\n" << indent << "SeriesDescription: ";
  if (this->SeriesDescription)
    {
    os << this->SeriesDescription;
    }

  os << "\n" << indent << "StudyDescription: ";
  if (this->StudyDescription)
    {
    os << this->StudyDescription;
    }

  os << "\n" << indent << "StudyID: ";
  if (this->StudyID)
    {
    os << this->StudyID;
    }

  os << "\n" << indent << "Modality: ";
  if (this->Modality)
    {
    os << this->Modality;
    }

  os << "\n" << indent << "ManufacturerModelName: ";
  if (this->ManufacturerModelName)
    {
    os << this->ManufacturerModelName;
    }

  os << "\n" << indent << "StationName: ";
  if (this->StationName)
    {
    os << this->StationName;
    }

  os << "\n" << indent << "InstitutionName: ";
  if (this->InstitutionName)
    {
    os << this->InstitutionName;
    }

  os << "\n" << indent << "ConvolutionKernel: ";
  if (this->ConvolutionKernel)
    {
    os << this->ConvolutionKernel;
    }

  os << "\n" << indent << "SliceThickness: ";
  if (this->SliceThickness)
    {
    os << this->SliceThickness;
    }

  os << "\n" << indent << "KVP: ";
  if (this->KVP)
    {
    os << this->KVP;
    }

  os << "\n" << indent << "GantryTilt: ";
  if (this->GantryTilt)
    {
    os << this->GantryTilt;
    }

  os << "\n" << indent << "EchoTime: ";
  if (this->EchoTime)
    {
    os << this->EchoTime;
    }

  os << "\n" << indent << "EchoTrainLength: ";
  if (this->EchoTrainLength)
    {
    os << this->EchoTrainLength;
    }

  os << "\n" << indent << "RepetitionTime: ";
  if (this->RepetitionTime)
    {
    os << this->RepetitionTime;
    }

  os << "\n" << indent << "ExposureTime: ";
  if (this->ExposureTime)
    {
    os << this->ExposureTime;
    }

  os << "\n" << indent << "XRayTubeCurrent: ";
  if (this->XRayTubeCurrent)
    {
    os << this->XRayTubeCurrent;
    }

  os << "\n" << indent << "Exposure: ";
  if (this->Exposure)
    {
    os << this->Exposure;
    }
}
