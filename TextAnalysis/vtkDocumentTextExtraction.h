/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDocumentTextExtraction.h

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

#ifndef __vtkDocumentTextExtraction_h
#define __vtkDocumentTextExtraction_h

#include <vtkTableAlgorithm.h>

// .NAME vtkDocumentTextExtraction - Extracts text from documents based on their MIME type.
//
// .SECTION Description
// Given a table containing MIME types and document contents, extracts plain text from
// each document.
//
// Inputs:
//   Input port 0: (required) A vtkTable containing MIME types and document contents
//   (which could be binary).
//
// Outputs:
//   Output port 0: The same table with an additional "text" column that contains the
//     text extracted from each document.
//
// Use SetInputArrayToProcess(0, ...) to specify the input table column that contains
// MIME types (must be a vtkStringArray).
//
// Use SetInputArrayToProcess(1, ...) to specify the input table column that contains
// document contents (must be a vtkStringArray).
//
// .SECTION Caveats
// The input document contents array must be a string array, even though the individual
// document contents may be binary data.
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

class VTK_TEXT_ANALYSIS_EXPORT vtkDocumentTextExtraction :
  public vtkTableAlgorithm
{
public:
  static vtkDocumentTextExtraction* New();
  vtkTypeRevisionMacro(vtkDocumentTextExtraction, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkDocumentTextExtraction();
  ~vtkDocumentTextExtraction();

  virtual int RequestData(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

private:
  vtkDocumentTextExtraction(const vtkDocumentTextExtraction &); // Not implemented.
  void operator=(const vtkDocumentTextExtraction &); // Not implemented.

  char* OutputColumn;
//ETX
};

#endif // __vtkDocumentTextExtraction_h

