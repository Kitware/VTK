/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssignMimeType.h

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

// .NAME vtkAssignMimeType - Assigns MIME types to a collection of documents.
//
// .SECTION Description
// Given a table containing document URIs and contents, tries to assign a MIME type
// to each document.
//
// Inputs:
//   Input port 0: (required) A vtkTable containing document URIs and contents
//   (which could be binary).
//
// Outputs:
//   Output port 0: The same table with an additional "mime_type" column that contains the
//     MIME type identified for each document, or empty-string.
//
// Use SetInputArrayToProcess(0, ...) to specify the input table column that contains
// URIs (must be a vtkStringArray).
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

#ifndef __vtkAssignMimeType_h
#define __vtkAssignMimeType_h

#include "vtkTableAlgorithm.h"
class vtkMimeTypes;

class VTK_TEXT_ANALYSIS_EXPORT vtkAssignMimeType :
  public vtkTableAlgorithm
{
public:
  static vtkAssignMimeType* New();
  vtkTypeMacro(vtkAssignMimeType, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specifies the name of the output MIME type array.  Default: "mime_type". 
  vtkSetStringMacro(OutputArray);
  vtkGetStringMacro(OutputArray);

  // Description:
  // Specifies a default MIME type that will be assigned to files whose MIME type
  // can't otherwise be identified.  Set this to "text/plain" if you want to analyze
  // files that would otherwise be ignored (such as files without a known file
  // extension, files without any file extension, etc).  Default: empty string.
  vtkSetStringMacro(DefaultMimeType);
  vtkGetStringMacro(DefaultMimeType);

  // Description:
  // Assign a custom vtkMimeTypes object to this filter.  This makes it possible
  // to work with arbitrary Mime Type strategies.
  void SetMimeTypes(vtkMimeTypes *m);
  vtkGetObjectMacro(MimeTypes, vtkMimeTypes);

//BTX
protected:
  vtkAssignMimeType();
  ~vtkAssignMimeType();

  virtual int RequestData(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

private:
  vtkAssignMimeType(const vtkAssignMimeType &); // Not implemented.
  void operator=(const vtkAssignMimeType &); // Not implemented.

  char* OutputArray;
  char* DefaultMimeType;

  vtkMimeTypes* MimeTypes;
//ETX
};

#endif // __vtkAssignMimeType_h
