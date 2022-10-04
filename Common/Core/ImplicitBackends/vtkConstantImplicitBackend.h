/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConstantImplicitBackend.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkConstantImplicitBackend_h
#define vtkConstantImplicitBackend_h

#include "vtkSetGet.h"

/**
 * \struct vtkConstantImplicitBackend
 * \brief A utility structure serving as a backend for constant implicit arrays
 */

template <typename ValueType>
struct vtkConstantImplicitBackend
{
  vtkConstantImplicitBackend(ValueType val)
    : Value(val){};

  ValueType operator()(int vtkNotUsed(index)) const { return this->Value; };

  ValueType Value;
};

#endif // vtkConstantImplicitBackend_h
