/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoostRandomSparseArraySource.h

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

// .NAME vtkBoostRandomSparseArraySource - generates a sparse N-way array containing random values.
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef vtkBoostRandomSparseArraySource_h
#define vtkBoostRandomSparseArraySource_h

#include "vtkInfovisBoostGraphAlgorithmsModule.h" // For export macro
#include "vtkArrayDataAlgorithm.h"
#include "vtkArrayExtents.h"

class VTKINFOVISBOOSTGRAPHALGORITHMS_EXPORT vtkBoostRandomSparseArraySource : public vtkArrayDataAlgorithm
{
public:
  static vtkBoostRandomSparseArraySource* New();
  vtkTypeMacro(vtkBoostRandomSparseArraySource, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  // Description:
  // Sets the extents (dimensionality and size) of the output array
  void SetExtents(const vtkArrayExtents&);

  // Description:
  // Returns the extents (dimensionality and size) of the output array
  vtkArrayExtents GetExtents();
//ETX

  // Description:
  // Stores a random-number-seed for determining which elements within
  // the output matrix will have non-zero values
  vtkGetMacro(ElementProbabilitySeed, vtkTypeUInt32);
  vtkSetMacro(ElementProbabilitySeed, vtkTypeUInt32);

  // Description:
  // Stores the probability (in the range [0, 1]) that an element within
  // the output matrix will have a non-zero value
  vtkGetMacro(ElementProbability, double);
  vtkSetMacro(ElementProbability, double);

  // Description:
  // Stores a random-number-seed for computing random element values
  vtkGetMacro(ElementValueSeed, vtkTypeUInt32);
  vtkSetMacro(ElementValueSeed, vtkTypeUInt32);

  // Description:
  // Stores the minimum value of any element
  vtkGetMacro(MinValue, double);
  vtkSetMacro(MinValue, double);

  // Description:
  // Stores the maximum value of any element
  vtkGetMacro(MaxValue, double);
  vtkSetMacro(MaxValue, double);

//BTX
protected:
  vtkBoostRandomSparseArraySource();
  ~vtkBoostRandomSparseArraySource();

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

private:
  vtkBoostRandomSparseArraySource(const vtkBoostRandomSparseArraySource&); // Not implemented
  void operator=(const vtkBoostRandomSparseArraySource&);   // Not implemented

  vtkArrayExtents Extents;

  vtkTypeUInt32 ElementProbabilitySeed;
  double ElementProbability;

  vtkTypeUInt32 ElementValueSeed;
  double MinValue;
  double MaxValue;
//ETX
};

#endif

// VTK-HeaderTest-Exclude: vtkBoostRandomSparseArraySource.h
