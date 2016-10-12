/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSignedCharArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Instantiate superclass first to give the template a DLL interface.
#define VTK_AOS_DATA_ARRAY_TEMPLATE_INSTANTIATING
#include "vtkAOSDataArrayTemplate.txx"
VTK_AOS_DATA_ARRAY_TEMPLATE_INSTANTIATE(signed char);

#include "vtkSignedCharArray.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSignedCharArray);

//----------------------------------------------------------------------------
vtkSignedCharArray::vtkSignedCharArray()
{
}

//----------------------------------------------------------------------------
vtkSignedCharArray::~vtkSignedCharArray()
{
}

//----------------------------------------------------------------------------
void vtkSignedCharArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->RealSuperclass::PrintSelf(os,indent);
}
