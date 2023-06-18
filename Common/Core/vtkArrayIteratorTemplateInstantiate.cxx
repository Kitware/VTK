/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayIteratorTemplateInstantiate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#define vtkArrayIteratorTemplateInstantiate_cxx

#include "vtkArrayIteratorTemplate.txx"

#include "vtkOStreamWrapper.h"

VTK_ABI_NAMESPACE_BEGIN
vtkInstantiateTemplateMacro(template class VTKCOMMONCORE_EXPORT vtkArrayIteratorTemplate);
template class VTKCOMMONCORE_EXPORT vtkArrayIteratorTemplate<vtkStdString>;
template class VTKCOMMONCORE_EXPORT vtkArrayIteratorTemplate<vtkVariant>;
VTK_ABI_NAMESPACE_END
