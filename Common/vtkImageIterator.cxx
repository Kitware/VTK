/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageIterator.cxx
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

#ifndef CMAKE_NO_EXPLICIT_TEMPLATE_INSTATIATION
#include "vtkImageIterator.txx"

template class VTK_COMMON_EXPORT vtkImageIterator<char>;
template class VTK_COMMON_EXPORT vtkImageIterator<int>;
template class VTK_COMMON_EXPORT vtkImageIterator<long>;
template class VTK_COMMON_EXPORT vtkImageIterator<short>;
template class VTK_COMMON_EXPORT vtkImageIterator<float>;
template class VTK_COMMON_EXPORT vtkImageIterator<double>;
template class VTK_COMMON_EXPORT vtkImageIterator<unsigned long>;
template class VTK_COMMON_EXPORT vtkImageIterator<unsigned short>;
template class VTK_COMMON_EXPORT vtkImageIterator<unsigned char>;
template class VTK_COMMON_EXPORT vtkImageIterator<unsigned int>;

#endif
