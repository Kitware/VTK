/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayPrint.h

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
// .NAME vtkArrayPrint - Print arrays in different formats

// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National
// Laboratories.

#ifndef __vtkArrayPrint_h
#define __vtkArrayPrint_h

#include "vtkTypedArray.h"

/// @relates vtkArrayPrint
/// Serializes the contents of an array to a stream as a series of
/// coordinates.  For 2D arrays of double values, the output is compatible
/// with the MatrixMarket "Coordinate Text File" format.
template<typename T>
void vtkPrintCoordinateFormat(ostream& stream, vtkTypedArray<T>* array);

/// @relates vtkArrayPrint
/// Serializes the contents of a matrix to a stream in human-readable form.
template<typename T>
void vtkPrintMatrixFormat(ostream& stream, vtkTypedArray<T>* matrix);

/// @relates vtkArrayPrint
/// Serializes the contents of a vector to a stream in human-readable form.
template<typename T>
void vtkPrintVectorFormat(ostream& stream, vtkTypedArray<T>* vector);

#include "vtkArrayPrint.txx"

#endif
// VTK-HeaderTest-Exclude: vtkArrayPrint.h
