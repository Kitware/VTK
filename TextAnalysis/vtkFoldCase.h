/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFoldCase.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

// .NAME vtkFoldCase - Converts a collection of strings to lower-case.
//
// .SECTION Description
// Given an array of strings, converts each string to its lower-case representation.
//
// Parameters:
//   ResultArray: The name of the array containing the folded-case text.
//
// Inputs:
//   Input port 0: (required) A vtkTable containing a column of text.
//
// Outputs:
//   Output port 0: The same table, plus the array of folded-case text.
//
// Use SetInputArrayToProcess(0, ...) to specify the "text" array.
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef __vtkFoldCase_h
#define __vtkFoldCase_h

#include <vtkTableAlgorithm.h>

class VTK_TEXT_ANALYSIS_EXPORT vtkFoldCase :
  public vtkTableAlgorithm
{
public:
  static vtkFoldCase* New();
  vtkTypeMacro(vtkFoldCase, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkSetStringMacro(ResultArray);
  vtkGetStringMacro(ResultArray);

//BTX
protected:
  vtkFoldCase();
  ~vtkFoldCase();

  virtual int RequestData(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  char* ResultArray;

private:
  vtkFoldCase(const vtkFoldCase &); // Not implemented.
  void operator=(const vtkFoldCase &); // Not implemented.
//ETX
};

#endif // __vtkFoldCase_h

