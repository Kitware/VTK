/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayPrint.txx

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

#ifndef __vtkArrayPrint_txx
#define __vtkArrayPrint_txx

#include "vtkArrayCoordinates.h"
#include <algorithm>
#include <iterator>

template<typename T>
void vtkPrintCoordinateFormat(ostream& stream, vtkTypedArray<T>* array)
{
  if(!array)
    {
    vtkGenericWarningMacro(<< "vtkPrintCoordinateFormat() requires a non-NULL array as input.");
    return;
    }

  const vtkArrayExtents extents = array->GetExtents();
  const vtkIdType dimensions = array->GetDimensions();
  const vtkIdType non_null_size = array->GetNonNullSize();

  for(vtkIdType i = 0; i != dimensions; ++i)
    stream << extents[i] << " ";
  stream << array->GetNonNullSize() << "\n";

  vtkArrayCoordinates coordinates;
  for(vtkIdType n = 0; n != non_null_size; ++n)
    {
    array->GetCoordinatesN(n, coordinates);
    for(vtkIdType i = 0; i != dimensions; ++i)
      stream << coordinates[i] << " ";
    stream << array->GetValueN(n) << "\n";
    }
}

template<typename T>
void vtkPrintMatrixFormat(ostream& stream, vtkTypedArray<T>* matrix)
{
  if(!matrix)
    {
    vtkGenericWarningMacro(<< "vtkPrintMatrixFormat() requires a non-NULL array as input.");
    return;
    }

  if(matrix->GetDimensions() != 2)
    {
    vtkGenericWarningMacro(<< "vtkPrintMatrixFormat() requires a matrix (2-way array) as input.");
    return;
    }

  const vtkArrayRange rows = matrix->GetExtent(0);
  const vtkArrayRange columns = matrix->GetExtent(1);

  for(vtkIdType row = rows.GetBegin(); row != rows.GetEnd(); ++row)
    {
    for(vtkIdType column = columns.GetBegin(); column != columns.GetEnd(); ++column)
      {
      stream << matrix->GetValue(vtkArrayCoordinates(row, column)) << " ";
      }
    stream << "\n";
    }
}

template<typename T>
void vtkPrintVectorFormat(ostream& stream, vtkTypedArray<T>* vector)
{
  if(!vector)
    {
    vtkGenericWarningMacro(<< "vtkPrintVectorFormat() requires a non-NULL array as input.");
    return;
    }

  if(vector->GetDimensions() != 1)
    {
    vtkGenericWarningMacro(<< "vtkPrintVectorFormat() requires a vector (1-way array) as input.");
    return;
    }

  const vtkArrayRange rows = vector->GetExtent(0);

  for(vtkIdType row = rows.GetBegin(); row != rows.GetEnd(); ++row)
    {
    stream << vector->GetValue(vtkArrayCoordinates(row)) << "\n";
    }
}

#endif

