/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOverrideInformation.cxx
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
#include "vtkOverrideInformation.h"

vtkCxxRevisionMacro(vtkOverrideInformation, "1.5");
vtkStandardNewMacro(vtkOverrideInformation);

vtkOverrideInformation::vtkOverrideInformation()
{
  this->ClassOverrideName = 0;
  this->ClassOverrideWithName = 0;
  this->Description = 0;
  this->ObjectFactory = 0;
}

vtkOverrideInformation::~vtkOverrideInformation()
{
  delete [] this->ClassOverrideName;
  delete [] this->ClassOverrideWithName;
  delete [] this->Description;
  if(this->ObjectFactory)
    {
    this->ObjectFactory->Delete();
    }
}


void vtkOverrideInformation::PrintSelf(ostream& os,
                                       vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent
     << "Override: " << this->ClassOverrideName 
     << "\nWith: " << this->ClassOverrideWithName 
     << "\nDescription: " << this->Description;
  os << indent << "From Factory:\n";
  if(this->ObjectFactory)
    {
    this->ObjectFactory->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    vtkIndent n = indent.GetNextIndent();
    os << n << "(NULL)\n";
    }
}


