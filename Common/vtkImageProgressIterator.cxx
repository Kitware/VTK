/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageProgressIterator.cxx
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

#ifndef CMAKE_NO_EXPLICIT_TEMPLATE_INSTANTIATION
#include "vtkImageProgressIterator.txx"

template class VTK_COMMON_EXPORT vtkImageProgressIterator<char>;
template class VTK_COMMON_EXPORT vtkImageProgressIterator<int>;
template class VTK_COMMON_EXPORT vtkImageProgressIterator<long>;
template class VTK_COMMON_EXPORT vtkImageProgressIterator<short>;
template class VTK_COMMON_EXPORT vtkImageProgressIterator<float>;
template class VTK_COMMON_EXPORT vtkImageProgressIterator<double>;
template class VTK_COMMON_EXPORT vtkImageProgressIterator<unsigned long>;
template class VTK_COMMON_EXPORT vtkImageProgressIterator<unsigned short>;
template class VTK_COMMON_EXPORT vtkImageProgressIterator<unsigned char>;
template class VTK_COMMON_EXPORT vtkImageProgressIterator<unsigned int>;

#endif



