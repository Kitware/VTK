/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageOcclusionSpectrum.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageOcclusionSpectrum.h"

vtkStandardNewMacro(vtkImageOcclusionSpectrum);

//----------------------------------------------------------------------------
// Construct an instance of vtkImageOcclusionSpectrum fitler.
vtkImageOcclusionSpectrum::vtkImageOcclusionSpectrum()
{
  // by default process active point scalars
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
void vtkImageOcclusionSpectrum::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
