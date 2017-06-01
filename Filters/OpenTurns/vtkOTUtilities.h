/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOTUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @brief
 * Set of utilities for OpenTURNS<->VTK conversions
 *
*/

#ifndef vtkOTUtilities_h
#define vtkOTUtilities_h

#include "openturns/DistributionFactoryImplementation.hxx" // For vtkOTDistributionImplementationWrapper

namespace OT
{
class NumericalSample;
}

class vtkDataArray;
class vtkDataArrayCollection;
class vtkPoints;

class vtkOTUtilities
{
public:
  /**
   * Methods to convert a collection of uni-dimensional data array
   * into a single NumericalSample
   * The number of arrays will determine the number of components of
   * the NumericalSample.
   * Arrays are suposed to have the same number of tuples, and the
   * NumericalSample will also have the same number of tuples.
   * This method allocate a new OT::NumericalSample and returns it,
   * so it will need to be freed.
   */
  static OT::NumericalSample* SingleDimArraysToNumericalSample(vtkDataArrayCollection* arrays);

  /**
   * Methods to convert a multi-component array into a NumericalSample.
   * The numerical sample will have the same dimension as the data array.
   * This method allocate a new OT::NumericalSample and returns it,
   * so it will need to be freed.
   */
  static OT::NumericalSample* ArrayToNumericalSample(vtkDataArray* arr);

  /**
   * Methods to convert a NumericalSample into a multi-component array.
   * The data array will have the same dimension as the numerical sample.
   * This method allocates a new vtkDataArray and returns it,
   * so it will need to be deleted.
   */
  static vtkDataArray* NumericalSampleToArray(OT::NumericalSample* ns);
};

/**
 * Wrapper class to allow forward declaration of a
 * OT::DistributionFactoryImplementation::Implementation
 */
class vtkOTDistributionImplementationWrapper
{
public:
  vtkOTDistributionImplementationWrapper(OT::DistributionFactoryImplementation::Implementation impl)
    : Implementation(impl){};

  OT::DistributionFactoryImplementation::Implementation Implementation;
};
#endif
// VTK-HeaderTest-Exclude: vtkOTUtilities.h
