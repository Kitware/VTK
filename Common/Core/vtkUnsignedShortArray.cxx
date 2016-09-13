/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnsignedShortArray.cxx

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
VTK_AOS_DATA_ARRAY_TEMPLATE_INSTANTIATE(unsigned short);

#include "vtkUnsignedShortArray.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkUnsignedShortArray);

//----------------------------------------------------------------------------
vtkUnsignedShortArray::vtkUnsignedShortArray()
{
}

//----------------------------------------------------------------------------
vtkUnsignedShortArray::~vtkUnsignedShortArray()
{
}

//----------------------------------------------------------------------------
void vtkUnsignedShortArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->RealSuperclass::PrintSelf(os,indent);
}
