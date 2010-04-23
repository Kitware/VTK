/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGPUInfo.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGPUInfo.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkGPUInfo);

// ----------------------------------------------------------------------------
vtkGPUInfo::vtkGPUInfo()
{
  this->DedicatedVideoMemory=0;
  this->DedicatedSystemMemory=0;
  this->SharedSystemMemory=0;
}

// ----------------------------------------------------------------------------
vtkGPUInfo::~vtkGPUInfo()
{
}

// ----------------------------------------------------------------------------
void vtkGPUInfo::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Dedicated Video Memory in bytes: " << this->DedicatedVideoMemory
     << endl;
  os << indent << "Dedicated System Memory in bytes: " << this->DedicatedSystemMemory
     << endl;
  os << indent << "Shared System Memory in bytes: " << this->SharedSystemMemory
     << endl;
}
