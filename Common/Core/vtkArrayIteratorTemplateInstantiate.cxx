// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#define vtkArrayIteratorTemplateInstantiate_cxx

#include "vtkArrayIteratorTemplate.txx"

#include "vtkOStreamWrapper.h"

VTK_ABI_NAMESPACE_BEGIN
vtkInstantiateTemplateMacro(template class VTKCOMMONCORE_EXPORT vtkArrayIteratorTemplate);
template class VTKCOMMONCORE_EXPORT vtkArrayIteratorTemplate<vtkStdString>;
template class VTKCOMMONCORE_EXPORT vtkArrayIteratorTemplate<vtkVariant>;
VTK_ABI_NAMESPACE_END
