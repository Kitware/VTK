
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRAdapter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

// .NAME vtkRAdapter - This is a utility class to convert VTK array data and 
//  VTK tables to and from Gnu R S expression (SEXP) data structures.  It is used
//  with the R .Call interface and the embedded R interpreter.
//
// .SECTION Description
//
//  This class creates deep copies of input data.  Created R SEXP variables created
//  by these functions can be freed by the R garbage collector by calling UNPROTECT(1).
//  The conversions are performed for double and integer data types.
//
// .SECTION See Also
//  vtkRinterface vtkRcalculatorFilter
//
// .SECTION Thanks
//  Developed by Thomas Otahal at Sandia National Laboratories.
//


#ifndef __vtkRAdapter
#define __vtkRAdapter

#include "vtkObject.h"
#include "Rinternals.h" // Needed for Rinternals.h SEXP data structure

class vtkInformation;
class vtkInformationVector;
class vtkDataArray;
class vtkArray;
class vtkTable;

class VTK_GRAPHICS_EXPORT vtkRAdapter : public vtkObject
{

public:

  vtkTypeRevisionMacro(vtkRAdapter, vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkRAdapter *New();

//BTX
  // Description:
  // Create a vtkDataArray copy of GNU R input matrix vaiable (deep copy, allocates memory)
  // Input is a R matrix or vector of doubles or integers
  static vtkDataArray* RToVTKDataArray(SEXP variable);

  // Description:
  // Create a vtkArray copy of the GNU R input variable multi-dimensional array (deep copy, allocates memory)
  // Input is a R multi-dimensional array of doubles or integers
  static vtkArray* RToVTKArray(SEXP variable);

  // Description:
  // Create a GNU R matrix copy of the input vtkDataArray da (deep copy, allocates memory)
  static SEXP VTKDataArrayToR(vtkDataArray* da);

  // Description:
  // Create a GNU R multi-dimensional array copy of the input vtkArray da (deep copy, allocates memory)
  static SEXP VTKArrayToR(vtkArray* da);

  // Description:
  // Create a GNU R matrix copy of the input vtkTable table (deep copy, allocates memory)
  static SEXP VTKTableToR(vtkTable* table);

  // Description:
  // Create a vtkTable copy of the GNU R input matrix variable (deep copy, allocates memory)
  // Input is R list of equal length vectors or a matrix.
  static vtkTable* RToVTKTable(SEXP variable);
//ETX

protected:

  vtkRAdapter() {};
  ~vtkRAdapter() {};

private:

  vtkRAdapter(const vtkRAdapter&); // Not implemented
  void operator=(const vtkRAdapter&); // Not implemented

};


#endif



