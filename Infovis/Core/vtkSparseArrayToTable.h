/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSparseArrayToTable.h

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
 * @class   vtkSparseArrayToTable
 * @brief   Converts a sparse array to a vtkTable.
 *
 *
 * Converts any sparse array to a vtkTable containing one row for each value
 * stored in the array.  The table will contain one column of coordinates for each
 * dimension in the source array, plus one column of array values.  A common use-case
 * for vtkSparseArrayToTable would be converting a sparse array into a table
 * suitable for use as an input to vtkTableToGraph.
 *
 * The coordinate columns in the output table will be named using the dimension labels
 * from the source array,  The value column name can be explicitly set using
 * SetValueColumn().
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
*/

#ifndef vtkSparseArrayToTable_h
#define vtkSparseArrayToTable_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkTableAlgorithm.h"

class VTKINFOVISCORE_EXPORT vtkSparseArrayToTable : public vtkTableAlgorithm
{
public:
  static vtkSparseArrayToTable* New();
  vtkTypeMacro(vtkSparseArrayToTable, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Specify the name of the output table column that contains array values.
   * Default: "value"
   */
  vtkGetStringMacro(ValueColumn);
  vtkSetStringMacro(ValueColumn);
  //@}

protected:
  vtkSparseArrayToTable();
  ~vtkSparseArrayToTable() VTK_OVERRIDE;

  int FillInputPortInformation(int, vtkInformation*) VTK_OVERRIDE;

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*) VTK_OVERRIDE;

  char* ValueColumn;

private:
  vtkSparseArrayToTable(const vtkSparseArrayToTable&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSparseArrayToTable&) VTK_DELETE_FUNCTION;
};

#endif

