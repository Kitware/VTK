/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAdjacencyMatrixToEdgeTable.h

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
 * @class   vtkAdjacencyMatrixToEdgeTable
 *
 *
 * Treats a dense 2-way array of doubles as an adacency matrix and converts it into a
 * vtkTable suitable for use as an edge table with vtkTableToGraph.
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
*/

#ifndef vtkAdjacencyMatrixToEdgeTable_h
#define vtkAdjacencyMatrixToEdgeTable_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkTableAlgorithm.h"

class VTKINFOVISCORE_EXPORT vtkAdjacencyMatrixToEdgeTable : public vtkTableAlgorithm
{
public:
  static vtkAdjacencyMatrixToEdgeTable* New();
  vtkTypeMacro(vtkAdjacencyMatrixToEdgeTable, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Specifies whether rows or columns become the "source" in the output edge table.
   * 0 = rows, 1 = columns.  Default: 0
   */
  vtkGetMacro(SourceDimension, vtkIdType);
  vtkSetMacro(SourceDimension, vtkIdType);
  //@}

  //@{
  /**
   * Controls the name of the output table column that contains edge weights.
   * Default: "value"
   */
  vtkGetStringMacro(ValueArrayName);
  vtkSetStringMacro(ValueArrayName);
  //@}

  //@{
  /**
   * Specifies the minimum number of adjacent edges to include for each source vertex.
   * Default: 0
   */
  vtkGetMacro(MinimumCount, vtkIdType);
  vtkSetMacro(MinimumCount, vtkIdType);
  //@}

  //@{
  /**
   * Specifies a minimum threshold that an edge weight must exceed to be included in
   * the output.
   * Default: 0.5
   */
  vtkGetMacro(MinimumThreshold, double);
  vtkSetMacro(MinimumThreshold, double);
  //@}

protected:
  vtkAdjacencyMatrixToEdgeTable();
  ~vtkAdjacencyMatrixToEdgeTable();

  int FillInputPortInformation(int, vtkInformation*);

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

  vtkIdType SourceDimension;
  char* ValueArrayName;
  vtkIdType MinimumCount;
  double MinimumThreshold;

private:
  vtkAdjacencyMatrixToEdgeTable(const vtkAdjacencyMatrixToEdgeTable&) VTK_DELETE_FUNCTION;
  void operator=(const vtkAdjacencyMatrixToEdgeTable&) VTK_DELETE_FUNCTION;
};

#endif

