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

// .NAME vtkTableToArray - converts a vtkTable to a matrix.
//
// .SECTION Description
// Converts a vtkTable into a dense matrix.  Use AddColumn() to
// designate one-to-many table columns that will become columns in the
// output matrix.a
//
// Using AddColumn() it is possible to duplicate / reorder columns in
// arbitrary ways.
//
// .SECTION Caveats
// Only produces vtkDenseArray<double>, regardless of the input table column types.
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkTableToArray_h
#define __vtkTableToArray_h

#include "vtkArrayDataAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkTableToArray : public vtkArrayDataAlgorithm
{
public:
  static vtkTableToArray* New();
  vtkTypeMacro(vtkTableToArray, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Reset the list of input table columns that will be mapped to columns
  // in the output matrix.
  void ClearColumns();
  // Description:
  // Add a column by name to the list of input table columns that will be
  // mapped to columns in the output matrix.
  void AddColumn(const char* name);
  // Description:
  // Add a column by index to the list of input table columns that will be
  // mapped to columns in the output matrix.
  void AddColumn(vtkIdType index);
  // Description:
  // Add every input table column to the output matrix.
  void AddAllColumns();

//BTX
protected:
  vtkTableToArray();
  ~vtkTableToArray();

  int FillInputPortInformation(int, vtkInformation*);

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

private:
  vtkTableToArray(const vtkTableToArray&); // Not implemented
  void operator=(const vtkTableToArray&);   // Not implemented

  class implementation;
  implementation* const Implementation;
//ETX
};

#endif

