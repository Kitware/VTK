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

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkMedicalImageProperties, "1.1");
vtkStandardNewMacro(vtkMedicalImageProperties);

//----------------------------------------------------------------------------
vtkMedicalImageProperties::vtkMedicalImageProperties()
{
  this->Date        = NULL;
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
  this->Clear();
}

//----------------------------------------------------------------------------
void vtkMedicalImageProperties::Clear()
{
  this->SetDate(NULL);
  this->SetImageNumber(NULL);
  this->SetModality(NULL);
  this->SetPatientID(NULL);
  this->SetPatientName(NULL);
  this->SetSeries(NULL);
  this->SetStudy(NULL);
}

//----------------------------------------------------------------------------
void vtkMedicalImageProperties::DeepCopy(vtkMedicalImageProperties *p)
{
  if (p == NULL)
    {
    return;
    }

  this->SetDate(p->GetDate());
  this->SetImageNumber(p->GetImageNumber());
  this->SetModality(p->GetModality());
  this->SetPatientID(p->GetPatientID());
  this->SetPatientName(p->GetPatientName());
  this->SetSeries(p->GetSeries());
  this->SetStudy(p->GetStudy());
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
  os << "\n" << indent << "Date: ";
  if (this->Date)
    {
    os << this->Date;
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
