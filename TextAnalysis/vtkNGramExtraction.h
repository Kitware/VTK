/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNGramExtraction.h

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

#ifndef __vtkNGramExtraction_h
#define __vtkNGramExtraction_h

#include <vtkTableAlgorithm.h>

// .NAME vtkNGramExtraction - Converts a collection of tokens into a collection of N-grams.
//
// .SECTION Description
// Given a table containing tokens, generates a table containing N-grams.
//
// Inputs:
//   Input port 0: (required) A vtkTable containing document, begin, end, and text
//     columns.
//
// Outputs:
//   Output port 0: A vtkTable containing "document", "begin", "end", "type", and "text"
//     columns. 
//
// Use SetInputArrayToProcess(0, ...) to specify the "document" array.
// Use SetInputArrayToProcess(1, ...) to specify the "begin" array.
// Use SetInputArrayToProcess(2, ...) to specify the "end" array.
// Use SetInputArrayToProcess(3, ...) to specify the "text" array.
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

class VTK_TEXT_ANALYSIS_EXPORT vtkNGramExtraction :
  public vtkTableAlgorithm
{
public:
  static vtkNGramExtraction* New();
  vtkTypeMacro(vtkNGramExtraction, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Controls the type of N-gram to produce.  Default: 1 (unigrams).
  vtkSetMacro(N, vtkIdType);
  vtkGetMacro(N, vtkIdType);

//BTX
protected:
  vtkNGramExtraction();
  ~vtkNGramExtraction();

  virtual int RequestData(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

private:
  vtkNGramExtraction(const vtkNGramExtraction &); // Not implemented.
  void operator=(const vtkNGramExtraction &); // Not implemented.

  vtkIdType N;
//ETX
};

#endif // __vtkNGramExtraction_h

