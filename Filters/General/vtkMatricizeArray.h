/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMatricizeArray.h

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
 * @class   vtkMatricizeArray
 * @brief   Convert an array of arbitrary dimensions to a
 * matrix.
 *
 *
 * Given a sparse input array of arbitrary dimension, creates a sparse output
 * matrix (vtkSparseArray<double>) where each column is a slice along an
 * arbitrary dimension from the source.
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
*/

#ifndef vtkMatricizeArray_h
#define vtkMatricizeArray_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkArrayDataAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkMatricizeArray : public vtkArrayDataAlgorithm
{
public:
  static vtkMatricizeArray* New();
  vtkTypeMacro(vtkMatricizeArray, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Returns the 0-numbered dimension that will be mapped to columns in the output
   */
  vtkGetMacro(SliceDimension, vtkIdType);
  //@}

  //@{
  /**
   * Sets the 0-numbered dimension that will be mapped to columns in the output
   */
  vtkSetMacro(SliceDimension, vtkIdType);
  //@}

protected:
  vtkMatricizeArray();
  ~vtkMatricizeArray() override;

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*) override;

private:
  vtkMatricizeArray(const vtkMatricizeArray&) = delete;
  void operator=(const vtkMatricizeArray&) = delete;

  class Generator;

  vtkIdType SliceDimension;
};

#endif

