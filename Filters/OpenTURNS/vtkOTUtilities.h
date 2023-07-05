// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @brief
 * Set of utilities for OpenTURNS<->VTK conversions
 *
 */

#ifndef vtkOTUtilities_h
#define vtkOTUtilities_h

#include "vtkABINamespace.h"

namespace OT
{
class Sample;
}

VTK_ABI_NAMESPACE_BEGIN

class vtkDataArray;
class vtkDataArrayCollection;
class vtkPoints;

class vtkOTUtilities
{
public:
  /**
   * Methods to convert a collection of uni-dimensional data array
   * into a single Sample
   * The number of arrays will determine the number of components of
   * the Sample.
   * Arrays are supposed to have the same number of tuples, and the
   * Sample will also have the same number of tuples.
   * This method allocate a new Sample and returns it,
   * so it is caller's responsibility to delete it with the delete operator.
   */
  static OT::Sample* SingleDimArraysToSample(vtkDataArrayCollection* arrays);

  /**
   * Methods to convert a multi-component array into a Sample.
   * The sample will have the same dimension as the data array.
   * This method allocate a new Sample and returns it,
   * so it is caller's responsibility to delete it with the delete operator.
   */
  static OT::Sample* ArrayToSample(vtkDataArray* arr);

  /**
   * Methods to convert a Sample into a multi-component array.
   * The data array will have the same dimension as the sample.
   * This method allocates a new vtkDataArray and returns it,
   * so it is caller's responsibility to delete it with the delete operator.
   */
  static vtkDataArray* SampleToArray(OT::Sample* ns);
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkOTUtilities.h
