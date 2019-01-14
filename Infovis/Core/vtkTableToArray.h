/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableToArray.h

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
 * @class   vtkTableToArray
 * @brief   converts a vtkTable to a matrix.
 *
 *
 * Converts a vtkTable into a dense matrix.  Use AddColumn() to
 * designate one-to-many table columns that will become columns in the
 * output matrix.a
 *
 * Using AddColumn() it is possible to duplicate / reorder columns in
 * arbitrary ways.
 *
 * @warning
 * Only produces vtkDenseArray<double>, regardless of the input table column types.
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
*/

#ifndef vtkTableToArray_h
#define vtkTableToArray_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkArrayDataAlgorithm.h"

class VTKINFOVISCORE_EXPORT vtkTableToArray : public vtkArrayDataAlgorithm
{
public:
  static vtkTableToArray* New();
  vtkTypeMacro(vtkTableToArray, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Reset the list of input table columns that will be mapped to columns
   * in the output matrix.
   */
  void ClearColumns();
  /**
   * Add a column by name to the list of input table columns that will be
   * mapped to columns in the output matrix.
   */
  void AddColumn(const char* name);
  /**
   * Add a column by index to the list of input table columns that will be
   * mapped to columns in the output matrix.
   */
  void AddColumn(vtkIdType index);
  /**
   * Add every input table column to the output matrix.
   */
  void AddAllColumns();

protected:
  vtkTableToArray();
  ~vtkTableToArray() override;

  int FillInputPortInformation(int, vtkInformation*) override;

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*) override;

private:
  vtkTableToArray(const vtkTableToArray&) = delete;
  void operator=(const vtkTableToArray&) = delete;

  class implementation;
  implementation* const Implementation;

};

#endif

