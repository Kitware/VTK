/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMedicalImageReader2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMedicalImageReader2.h"
#include "vtkObjectFactory.h"

#include "vtkMedicalImageProperties.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMedicalImageReader2);

//----------------------------------------------------------------------------
vtkMedicalImageReader2::vtkMedicalImageReader2()
{
  this->MedicalImageProperties = vtkMedicalImageProperties::New();
}

//----------------------------------------------------------------------------
vtkMedicalImageReader2::~vtkMedicalImageReader2()
{
  if (this->MedicalImageProperties)
  {
    this->MedicalImageProperties->Delete();
    this->MedicalImageProperties = NULL;
  }
}

//----------------------------------------------------------------------------
void vtkMedicalImageReader2::SetPatientName(const char *arg)
{
  if (this->MedicalImageProperties)
  {
    this->MedicalImageProperties->SetPatientName(arg);
  }
}

//----------------------------------------------------------------------------
const char* vtkMedicalImageReader2::GetPatientName()
{
  if (this->MedicalImageProperties)
  {
    return this->MedicalImageProperties->GetPatientName();
  }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkMedicalImageReader2::SetPatientID(const char *arg)
{
  if (this->MedicalImageProperties)
  {
    this->MedicalImageProperties->SetPatientID(arg);
  }
}

//----------------------------------------------------------------------------
const char* vtkMedicalImageReader2::GetPatientID()
{
  if (this->MedicalImageProperties)
  {
    return this->MedicalImageProperties->GetPatientID();
  }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkMedicalImageReader2::SetDate(const char *arg)
{
  if (this->MedicalImageProperties)
  {
    this->MedicalImageProperties->SetImageDate(arg);
  }
}

//----------------------------------------------------------------------------
const char* vtkMedicalImageReader2::GetDate()
{
  if (this->MedicalImageProperties)
  {
    return this->MedicalImageProperties->GetImageDate();
  }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkMedicalImageReader2::SetSeries(const char *arg)
{
  if (this->MedicalImageProperties)
  {
    this->MedicalImageProperties->SetSeriesNumber(arg);
  }
}

//----------------------------------------------------------------------------
const char* vtkMedicalImageReader2::GetSeries()
{
  if (this->MedicalImageProperties)
  {
    return this->MedicalImageProperties->GetSeriesNumber();
  }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkMedicalImageReader2::SetStudy(const char *arg)
{
  if (this->MedicalImageProperties)
  {
    this->MedicalImageProperties->SetStudyID(arg);
  }
}

//----------------------------------------------------------------------------
const char* vtkMedicalImageReader2::GetStudy()
{
  if (this->MedicalImageProperties)
  {
    return this->MedicalImageProperties->GetStudyID();
  }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkMedicalImageReader2::SetImageNumber(const char *arg)
{
  if (this->MedicalImageProperties)
  {
    this->MedicalImageProperties->SetImageNumber(arg);
  }
}

//----------------------------------------------------------------------------
const char* vtkMedicalImageReader2::GetImageNumber()
{
  if (this->MedicalImageProperties)
  {
    return this->MedicalImageProperties->GetImageNumber();
  }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkMedicalImageReader2::SetModality(const char *arg)
{
  if (this->MedicalImageProperties)
  {
    this->MedicalImageProperties->SetModality(arg);
  }
}

//----------------------------------------------------------------------------
const char* vtkMedicalImageReader2::GetModality()
{
  if (this->MedicalImageProperties)
  {
    return this->MedicalImageProperties->GetModality();
  }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkMedicalImageReader2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->MedicalImageProperties)
  {
    os << indent << "Medical Image Properties:\n";
    this->MedicalImageProperties->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "MedicalImageProperties: (none)\n";
  }
}
