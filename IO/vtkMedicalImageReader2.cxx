/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMedicalImageReader2.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMedicalImageReader2.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkMedicalImageReader2, "1.2");
vtkStandardNewMacro(vtkMedicalImageReader2);

vtkMedicalImageReader2::vtkMedicalImageReader2()
{
  this->PatientName = 0;
  this->PatientID = 0;
  this->Date = 0;
  this->ImageNumber = 0;
  this->Series = 0;
  this->Study = 0;
}

vtkMedicalImageReader2::~vtkMedicalImageReader2()
{
  delete [] this->PatientName;
  this->PatientName = NULL;
  delete [] this->PatientID;
  this->PatientID = NULL;
  delete [] this->Date;
  this->Date = NULL;
  delete [] this->ImageNumber;
  this->ImageNumber = NULL;
  delete [] this->Series;
  this->Series = NULL;
  delete [] this->Study;
  this->Study = NULL;
}

void vtkMedicalImageReader2::PrintSelf(ostream& os, vtkIndent indent)
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
}
