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

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkMedicalImageProperties, "1.3");
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
  };
  
  typedef vtkstd::vector<WindowLevelPreset> WindowLevelPresetPoolType;
  typedef vtkstd::vector<WindowLevelPreset>::iterator WindowLevelPresetPoolIterator;

  WindowLevelPresetPoolType WindowLevelPresetPool;
};

//----------------------------------------------------------------------------
vtkMedicalImageProperties::vtkMedicalImageProperties()
{
  this->Internals = new vtkMedicalImagePropertiesInternals;

  this->ImageDate   = NULL;
  this->ImageNumber = NULL;
  this->Modality    = NULL;
  this->PatientID   = NULL;
  this->PatientName = NULL;
  this->Series      = NULL;
  this->Study       = NULL;
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
  this->SetImageDate(NULL);
  this->SetImageNumber(NULL);
  this->SetModality(NULL);
  this->SetPatientID(NULL);
  this->SetPatientName(NULL);
  this->SetSeries(NULL);
  this->SetStudy(NULL);

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

  this->SetImageDate(p->GetImageDate());
  this->SetImageNumber(p->GetImageNumber());
  this->SetModality(p->GetModality());
  this->SetPatientID(p->GetPatientID());
  this->SetPatientName(p->GetPatientName());
  this->SetSeries(p->GetSeries());
  this->SetStudy(p->GetStudy());

  int nb_presets = p->GetNumberOfWindowLevelPresets();
  for (int i = 0; i < nb_presets; i++)
    {
    double w, l;
    p->GetNthWindowLevelPreset(i, &w, &l);
    this->AddWindowLevelPreset(w, l);
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
  os << "\n" << indent << "ImageDate: ";
  if (this->ImageDate)
    {
    os << this->ImageDate;
    }
  os << "\n" << indent << "ImageNumber: ";
  if (this->ImageNumber)
    {
    os << this->ImageNumber;
    }
  os << "\n" << indent << "Series: ";
  if (this->Series)
    {
    os << this->Series;
    }
  os << "\n" << indent << "Study: ";
  if (this->Study)
    {
    os << this->Study;
    }
  os << "\n" << indent << "Modality: ";
  if (this->Modality)
    {
    os << this->Modality;
    }
}
