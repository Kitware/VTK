/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDiagonalMatrixSource.h

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

// .NAME vtkDiagonalMatrixSource - generates a sparse or dense square matrix
// with user-specified values for the diagonal, superdiagonal, and subdiagonal.
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkDiagonalMatrixSource_h
#define __vtkDiagonalMatrixSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkArrayDataAlgorithm.h"

class VTKFILTERSSOURCES_EXPORT vtkDiagonalMatrixSource : public vtkArrayDataAlgorithm
{
public:
  static vtkDiagonalMatrixSource* New();
  vtkTypeMacro(vtkDiagonalMatrixSource, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  // Determines whether the output matrix will be dense or sparse
  enum StorageType
  {
    DENSE,
    SPARSE
  };
//ETX

  vtkGetMacro(ArrayType, int);
  vtkSetMacro(ArrayType, int);

  // Description:
  // Stores the extents of the output matrix (which is square)
  vtkGetMacro(Extents, vtkIdType);
  vtkSetMacro(Extents, vtkIdType);

  // Description:
  // Stores the value that will be assigned to diagonal elements (default: 1)
  vtkGetMacro(Diagonal, double);
  vtkSetMacro(Diagonal, double);

  // Description:
  // Stores the value that will be assigned to superdiagonal elements (default: 0)
  vtkGetMacro(SuperDiagonal, double);
  vtkSetMacro(SuperDiagonal, double);

  // Description:
  // Stores the value that will be assigned to subdiagonal elements (default: 0)
  vtkGetMacro(SubDiagonal, double);
  vtkSetMacro(SubDiagonal, double);

  // Description:
  // Controls the output matrix row dimension label.
  // Default: "rows"
  vtkGetStringMacro(RowLabel);
  vtkSetStringMacro(RowLabel);

  // Description:
  // Controls the output matrix column dimension label.
  // Default: "columns"
  vtkGetStringMacro(ColumnLabel);
  vtkSetStringMacro(ColumnLabel);

protected:
  vtkDiagonalMatrixSource();
  ~vtkDiagonalMatrixSource();

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

private:
  vtkDiagonalMatrixSource(const vtkDiagonalMatrixSource&); // Not implemented
  void operator=(const vtkDiagonalMatrixSource&);   // Not implemented

  vtkArray* GenerateDenseArray();
  vtkArray* GenerateSparseArray();

  int ArrayType;
  vtkIdType Extents;
  double Diagonal;
  double SuperDiagonal;
  double SubDiagonal;
  char* RowLabel;
  char* ColumnLabel;
};

#endif

