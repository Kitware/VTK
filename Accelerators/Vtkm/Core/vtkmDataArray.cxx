// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2019 Sandia Corporation.
// SPDX-FileCopyrightText: Copyright 2019 UT-Battelle, LLC.
// SPDX-FileCopyrightText: Copyright 2019 Los Alamos National Security.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-LANL-USGov
#define vtkmDataArray_cxx

#include "vtkmDataArray.h"

VTK_ABI_NAMESPACE_BEGIN
template class VTKACCELERATORSVTKMCORE_EXPORT vtkmDataArray<char>;
template class VTKACCELERATORSVTKMCORE_EXPORT vtkmDataArray<double>;
template class VTKACCELERATORSVTKMCORE_EXPORT vtkmDataArray<float>;
template class VTKACCELERATORSVTKMCORE_EXPORT vtkmDataArray<int>;
template class VTKACCELERATORSVTKMCORE_EXPORT vtkmDataArray<long>;
template class VTKACCELERATORSVTKMCORE_EXPORT vtkmDataArray<long long>;
template class VTKACCELERATORSVTKMCORE_EXPORT vtkmDataArray<short>;
template class VTKACCELERATORSVTKMCORE_EXPORT vtkmDataArray<signed char>;
template class VTKACCELERATORSVTKMCORE_EXPORT vtkmDataArray<unsigned char>;
template class VTKACCELERATORSVTKMCORE_EXPORT vtkmDataArray<unsigned int>;
template class VTKACCELERATORSVTKMCORE_EXPORT vtkmDataArray<unsigned long>;
template class VTKACCELERATORSVTKMCORE_EXPORT vtkmDataArray<unsigned long long>;
template class VTKACCELERATORSVTKMCORE_EXPORT vtkmDataArray<unsigned short>;
VTK_ABI_NAMESPACE_END
