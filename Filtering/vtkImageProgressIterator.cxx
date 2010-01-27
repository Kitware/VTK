/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageProgressIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkConfigure.h"

#if defined(_MSC_VER) && !defined(VTK_DISPLAY_WIN32_WARNINGS)
#pragma warning ( disable : 4275 )
#endif

// Do not include vtkImageIterator.txx here - will cause implicit template
// instantiation, breaking symbol visibility for the iterator with GCC.
// Added back in due to issues encountered with MSVC - see VTK dashboards 01-27-10
#include "vtkImageIterator.txx"
#include "vtkImageProgressIterator.txx"

#ifndef VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION

template class VTK_FILTERING_EXPORT vtkImageProgressIterator<signed char>;
template class VTK_FILTERING_EXPORT vtkImageProgressIterator<char>;
template class VTK_FILTERING_EXPORT vtkImageProgressIterator<int>;
template class VTK_FILTERING_EXPORT vtkImageProgressIterator<long>;
template class VTK_FILTERING_EXPORT vtkImageProgressIterator<short>;
template class VTK_FILTERING_EXPORT vtkImageProgressIterator<float>;
template class VTK_FILTERING_EXPORT vtkImageProgressIterator<double>;
template class VTK_FILTERING_EXPORT vtkImageProgressIterator<unsigned long>;
template class VTK_FILTERING_EXPORT vtkImageProgressIterator<unsigned short>;
template class VTK_FILTERING_EXPORT vtkImageProgressIterator<unsigned char>;
template class VTK_FILTERING_EXPORT vtkImageProgressIterator<unsigned int>;
#if defined(VTK_TYPE_USE_LONG_LONG)
template class VTK_FILTERING_EXPORT vtkImageProgressIterator<long long>;
template class VTK_FILTERING_EXPORT vtkImageProgressIterator<unsigned long long>;
#endif
#if defined(VTK_TYPE_USE___INT64)
template class VTK_FILTERING_EXPORT vtkImageProgressIterator<__int64>;
template class VTK_FILTERING_EXPORT vtkImageProgressIterator<unsigned __int64>;
#endif

#endif

