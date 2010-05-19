/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextExtraction.h

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

#ifndef __vtkTextExtraction_h
#define __vtkTextExtraction_h

#include <vtkTableAlgorithm.h>

class vtkTextExtractionStrategy;

// .NAME vtkTextExtraction - Extracts text from documents based on their MIME type.
//
// .SECTION Description
// Given a table containing document ids, URIs, Mime types and document contents,
// extracts plain text from each document, and generates a list of 'tags' that
// delineate ranges of text.  The actual work of extracting text and generating tags
// is performed by an ordered list of vtkTextExtractionStrategy objects.
//
// By default, vtkTextExtraction has just a single strategy for extracting plain
// text documents.  Callers will almost certainly want to supplement or replace
// the default with their own strategies.
//
// Inputs:
//   Input port 0: (required) A vtkTable containing document ids, Mime types and
//     document contents (which could be binary).
//
// Outputs:
//   Output port 0: The same table with an additional "text" column that contains the
//     text extracted from each document.
//   Output port 1: A table of document tags that includes "document", "uri", "begin",
//     "end", and "type" columns.
//
// Use SetInputArrayToProcess(0, ...) to specify the input table column that contains
// document ids (must be a vtkIdTypeArray).  Default: "document".
//
// Use SetInputArrayToProcess(1, ...) to specify the input table column that contains
// URIs (must be a vtkStringArray).  Default: "uri".
//
// Use SetInputArrayToProcess(2, ...) to specify the input table column that contains
// Mime types (must be a vtkStringArray).  Default: "mime_type".
//
// Use SetInputArrayToProcess(3, ...) to specify the input table column that contains
// document contents (must be a vtkStringArray).  Default: "content".
//
// .SECTION Caveats
// The input document contents array must be a string array, even though the individual
// document contents may be binary data.
//
// .SECTION See Also
// vtkTextExtractionStrategy, vtkPlainTextExtractionStrategy
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

class VTK_TEXT_ANALYSIS_EXPORT vtkTextExtraction :
  public vtkTableAlgorithm
{
public:
  static vtkTextExtraction* New();
  vtkTypeMacro(vtkTextExtraction, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Clear the list of strategies.
  void ClearStrategies();

  // Description:
  // Prepend a strategy to the list of strategies.  vtkTextExtraction assumes ownership
  // of the supplied object.
  void PrependStrategy(vtkTextExtractionStrategy* strategy);
  // Description:
  // Append a strategy to the list of strategies.  vtkTextExtraction assumes ownership
  // of the supplied object.
  void AppendStrategy(vtkTextExtractionStrategy* strategy);

  // Description:
  // Specifies the name of the output text array.  Default: "text". 
  vtkSetStringMacro(OutputArray);
  vtkGetStringMacro(OutputArray);

//BTX
protected:
  vtkTextExtraction();
  ~vtkTextExtraction();

  virtual int RequestData(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

private:
  vtkTextExtraction(const vtkTextExtraction &); // Not implemented.
  void operator=(const vtkTextExtraction &); // Not implemented.

  char* OutputArray;

  class Implementation;
  Implementation* const Internal;
//ETX
};

#endif // __vtkTextExtraction_h

