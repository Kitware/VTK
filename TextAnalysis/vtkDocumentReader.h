/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDocumentReader.h

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

#ifndef __vtkDocumentReader_h
#define __vtkDocumentReader_h

#include <vtkTableAlgorithm.h>

class vtkStdString;

// .NAME vtkDocumentReader - Reads documents into memory for text analysis.
//
// .SECTION Description
// Reads zero-to-many documents into memory, producing a vtkTable suitable
// for use as an input to other VTK text analysis filters.
//
// Parameters:
//   "Files": a collection of filesystem paths to be loaded.
//
// Outputs:
//   Output port 0: A vtkTable containing "document", "uri", and "content" columns.
//
// The output "document" column will contain a zero-based integer document index;
// "uri" will contain the filepath to the document formatted as a file:// URI;
// "contents" will contain the binary contents of the document.
//
// .SECTION Caveats
// As a workaround, vtkDocumentReader stores the contents of each document
// in the "contents" column, which is a vtkStdString array.  Note that the
// contents of a document may actually be binary data, so check the MIME-Type
// before treating the contents as a string.
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

class VTK_TEXT_ANALYSIS_EXPORT vtkDocumentReader :
  public vtkTableAlgorithm
{
public:
  static vtkDocumentReader* New();
  vtkTypeMacro(vtkDocumentReader, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a file to be loaded.
  void AddFile(const char* file);
  void AddFile(const vtkStdString& file);

  // Description:
  // Clear the list of files to be loaded.
  void ClearFiles();

//BTX
protected:
  vtkDocumentReader();
  ~vtkDocumentReader();

  virtual int RequestData(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

private:
  vtkDocumentReader(const vtkDocumentReader &); // Not implemented.
  void operator=(const vtkDocumentReader &); // Not implemented.

  friend class vtkPDocumentReader;
  void AddFile(const vtkStdString& file, const vtkIdType id);

  class Implementation;
  Implementation* const Internal;
//ETX
};

#endif // __vtkDocumentReader_h

