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
// Funded by CEA, DAM, DIF, F-91297 Arpajon, France
#ifndef vtkConstantImplicitBackend_h
#define vtkConstantImplicitBackend_h

#include "vtkCommonImplicitArraysModule.h"
#include "vtkSetGet.h" // for vtkNotUsed

/**
 * \struct vtkConstantImplicitBackend
 * \brief A utility structure serving as a backend for constant implicit arrays
 *
 * This structure can be classified as a closure and can be called using syntax similar to a
 * function.
 *
 * At construction it takes one parameter which is the constant value that it returns from its main
 * function call regardless of index.
 *
 * An example of potential usage in a vtkImplicitArray
 * ```
 * double constant = some_number;
 * vtkNew<vtkImplicitArray<vtkConstantImplicitBackend<double>>> constArray;
 * constArray->SetBackend(std::make_shared<vtkConstantImplicitBackend<double>>(constant));
 * constArray->SetNumberOfTuples(however_many_you_want);
 * constArray->SetNumberOfComponents(whatever_youd_like);
 * double value = constArray->GetTypedComponent(index_in_tuple_range, index_in_component_range);
 * CHECK(constant == value); // always true
 * ```
 */
VTK_ABI_NAMESPACE_BEGIN
template <typename ValueType>
struct vtkConstantImplicitBackend final
{
  /**
   * A non-trivially contructible constructor
   *
   * \param val the constant value to return for all indeces
   */
  vtkConstantImplicitBackend(ValueType val)
    : Value(val)
  {
  }

  /**
   * The main call method for the backend
   *
   * \return the constant value
   */
  ValueType operator()(int vtkNotUsed(index)) const { return this->Value; }

  /**
   * The constant value stored in the backend
   */
  const ValueType Value;
};
VTK_ABI_NAMESPACE_END

#endif // vtkConstantImplicitBackend_h
