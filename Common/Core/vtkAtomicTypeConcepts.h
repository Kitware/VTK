/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAtomicTypeConcepts.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkAtomicTypeConcepts_h
#define vtkAtomicTypeConcepts_h

#include <limits>

namespace vtk
{
namespace atomic
{
namespace detail
{

template <bool> struct CompileTimeCheck;
template <> struct CompileTimeCheck<true> {};

template <typename T> struct IntegralType
{
  CompileTimeCheck<std::numeric_limits<T>::is_specialized &&
                   std::numeric_limits<T>::is_integer &&
                   (sizeof(T) == 4 || sizeof(T) == 8)> c;
};

} // detail
} // atomic
} // vtk

#endif
// VTK-HeaderTest-Exclude: vtkAtomicTypeConcepts.h
