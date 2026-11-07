// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#define vtkDeviceDataArray_cxx

#include "vtkDeviceDataArray.txx"

VTK_ABI_NAMESPACE_BEGIN
template class VTKCOMMONCORE_EXPORT vtkDeviceDataArray<char>;
template class VTKCOMMONCORE_EXPORT vtkDeviceDataArray<double>;
template class VTKCOMMONCORE_EXPORT vtkDeviceDataArray<float>;
template class VTKCOMMONCORE_EXPORT vtkDeviceDataArray<int>;
template class VTKCOMMONCORE_EXPORT vtkDeviceDataArray<long>;
template class VTKCOMMONCORE_EXPORT vtkDeviceDataArray<long long>;
template class VTKCOMMONCORE_EXPORT vtkDeviceDataArray<short>;
template class VTKCOMMONCORE_EXPORT vtkDeviceDataArray<signed char>;
template class VTKCOMMONCORE_EXPORT vtkDeviceDataArray<unsigned char>;
template class VTKCOMMONCORE_EXPORT vtkDeviceDataArray<unsigned int>;
template class VTKCOMMONCORE_EXPORT vtkDeviceDataArray<unsigned long>;
template class VTKCOMMONCORE_EXPORT vtkDeviceDataArray<unsigned long long>;
template class VTKCOMMONCORE_EXPORT vtkDeviceDataArray<unsigned short>;
VTK_ABI_NAMESPACE_END
