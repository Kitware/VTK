/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableToSparseArray.h
  
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

#ifndef __vtkTableToSparseArray_h
#define __vtkTableToSparseArray_h

#include "vtkArrayDataAlgorithm.h"

// .NAME vtkTableToSparseArray - converts a vtkTable into a sparse array.
//
// .SECTION Description
// Converts a vtkTable into a sparse array.  Use AddCoordinateColumn() to
// designate one-to-many table columns that contain coordinates for each
// array value, and SetValueColumn() to designate the table column that 
// contains array values.
//
// Thus, the number of dimensions in the output array will equal the number
// of calls to AddCoordinateColumn().
//
// The coordinate columns will also be used to populate dimension labels
// in the output array.
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

class VTK_INFOVIS_EXPORT vtkTableToSparseArray : public vtkArrayDataAlgorithm
{
public:
  static vtkTableToSparseArray* New();
  vtkTypeMacro(vtkTableToSparseArray, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the set of input table columns that will be mapped to coordinates
  // in the output sparse array.
  void ClearCoordinateColumns();
  void AddCoordinateColumn(const char* name);

  // Description:
  // Specify the input table column that will be mapped to values in the output array.
  void SetValueColumn(const char* name);
  const char* GetValueColumn();

//BTX
protected:
  vtkTableToSparseArray();
  ~vtkTableToSparseArray();

  int FillInputPortInformation(int, vtkInformation*);

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

private:
  vtkTableToSparseArray(const vtkTableToSparseArray&); // Not implemented
  void operator=(const vtkTableToSparseArray&);   // Not implemented

  class implementation;
  implementation* const Implementation;
//ETX
};

#endif

