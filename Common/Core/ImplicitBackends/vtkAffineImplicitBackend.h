/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAffineImplicitBackend.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkAffineImplicitBackend_h
#define vtkAffineImplicitBackend_h

/**
 * \struct vtkAffineImplicitBackend
 * \brief A utility structure serving as a backend for affine (as a function of the index) implicit
 * arrays
 *
 * At construction it takes two parameters: the slope of the map and the intercept
 */

template <typename ValueType>
struct vtkAffineImplicitBackend
{
  vtkAffineImplicitBackend(ValueType slope, ValueType origin)
    : Slope(slope)
    , Origin(origin){};

  ValueType operator()(int index) const { return this->Slope * index + this->Origin; };

  ValueType Slope;
  ValueType Origin;
};

#endif // vtkAffineImplicitBackend_h
