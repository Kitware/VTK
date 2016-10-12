/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayWeights.h

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkArrayWeights
 * @brief   Stores a collection of weighting factors.
 *
 *
 * vtkArrayWeights provides storage for a collection of weights to be
 * used when merging / interpolating N-way arrays.  Convenience
 * constructors are provided for working with one, two, three, and four
 * weighting factors.  For arbitrary collections of weights, use
 * SetCount() and operator[] to assign values.
 *
 * vtkArrayWeights is most commonly used with the vtkInterpolate()
 * function to compute weighted sums of vtkArray objects.
 *
 * @sa
 * vtkArray, vtkArraySlices
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National
 * Laboratories.
*/

#ifndef vtkArrayWeights_h
#define vtkArrayWeights_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkSystemIncludes.h"

class vtkArrayWeightsStorage; // pimpl

class VTKCOMMONCORE_EXPORT vtkArrayWeights
{
public:
  /**
   * Create an empty collection of weights
   */
  vtkArrayWeights();

  /**
   * Copy the weights from another object.
   */
  vtkArrayWeights(const vtkArrayWeights& other);

  /**
   * Create a collection containing one weight.
   */
  vtkArrayWeights(double i);

  /**
   * Create a collection containing two weights.
   */
  vtkArrayWeights(double i, double j);

  /**
   * Create a collection containing three weights.
   */
  vtkArrayWeights(double i, double j, double k);

  /**
   * Create a collection containing four weights.
   */
  vtkArrayWeights(double i, double j, double k, double l);

  /**
   * Destructor.
   */
  ~vtkArrayWeights();

  /**
   * Returns the number of weights stored in this container.
   */
  vtkIdType GetCount() const;

  /**
   * Sets the number of weights stored in this container.  Note that each
   * weight will be reset to 0.0 after calling SetCount(), use operator[]
   * to assign the desired value for each weight.
   */
  void SetCount(vtkIdType count);

  /**
   * Accesses the i-th weight in the collection.
   */
  double& operator[](vtkIdType);

  /**
   * Accesses the i-th weight in the collection.
   */
  const double& operator[](vtkIdType) const;

  /**
   * Assignment operator.
   */
  vtkArrayWeights& operator=(const vtkArrayWeights& other);

protected:
  vtkArrayWeightsStorage *Storage;
};

#endif

// VTK-HeaderTest-Exclude: vtkArrayWeights.h
