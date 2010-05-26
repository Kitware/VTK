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

// .NAME vtkArrayWeights - Stores a collection of weighting factors.
//
// .SECTION Description
// vtkArrayWeights provides storage for a collection of weights to be
// used when merging / interpolating N-way arrays.  Convenience
// constructors are provided for working with one, two, three, and four
// weighting factors.  For arbitrary collections of weights, use
// SetCount() and operator[] to assign values.
//
// vtkArrayWeights is most commonly used with the vtkInterpolate()
// function to compute weighted sums of vtkArray objects.
//
// .SECTION See Also
// vtkArray, vtkArraySlices
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National
// Laboratories.

#ifndef __vtkArrayWeights_h
#define __vtkArrayWeights_h

#include "vtkSystemIncludes.h"

class vtkArrayWeightsStorage; // pimpl

class VTK_COMMON_EXPORT vtkArrayWeights
{
public:
  // Description:
  // Create an empty collection of weights
  vtkArrayWeights();

  // Description:
  // Create a collection containing one weight.
  vtkArrayWeights(double i);

  // Description:
  // Create a collection containing two weights.
  vtkArrayWeights(double i, double j);

  // Description:
  // Create a collection containing three weights.
  vtkArrayWeights(double i, double j, double k);

  // Description:
  // Create a collection containing four weights.
  vtkArrayWeights(double i, double j, double k, double l);

  // Description:
  // Destructor.
  ~vtkArrayWeights();

  // Description:
  // Returns the number of weights stored in this container.
  vtkIdType GetCount() const;

  // Description:
  // Sets the number of weights stored in this container.  Note that each
  // weight will be reset to 0.0 after calling SetCount(), use operator[]
  // to assign the desired value for each weight.
  void SetCount(vtkIdType count);

  // Description:
  // Accesses the i-th weight in the collection.
  double& operator[](vtkIdType);

  // Description:
  // Accesses the i-th weight in the collection.
  const double& operator[](vtkIdType) const;

protected:
  vtkArrayWeightsStorage *Storage;
};

#endif

