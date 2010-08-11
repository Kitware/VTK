/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextExtractionStrategy.h

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

// .NAME vtkTextExtractionStrategy - Abstract interface for an object that can extract
// tagged text from a resource.
//
// .SECTION Description
// Concrete derivatives of vtkTextExtractionStrategy implement strategies for extracting
// text from a resource, given its Mime type and content.
//
// .SECTION See Also
// vtkTextExtraction, vtkPlainTextExtractionStrategy.
//
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef _vtkTextExtractionStrategy_h
#define _vtkTextExtractionStrategy_h

#include <vtkObject.h>

class vtkIdTypeArray;
class vtkStdString;
class vtkStringArray;
class vtkUnicodeString;
class vtkUnicodeStringArray;

class VTK_TEXT_ANALYSIS_EXPORT vtkTextExtractionStrategy :
  public vtkObject
{
public:
  vtkTypeMacro(vtkTextExtractionStrategy, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Given a resource Mime type and content, implementations should return 'true' if they can
  // extract text from resources with the given Mime type, otherwise false.  If the implementation
  // can handle the resource, it should return any text that can be extracted, and append a set
  // of zero-to-many tags to the given tag arrays.  Note that at a minimum, implementations
  // should generate a "TEXT" tag that encloses the body of the text content.
  // 
  // A resource URI is provided for reference; in general, implementations shouldn't need to
  // use the URI to access the resource content, since it is already loaded into memory.
  virtual bool Extract(
    const vtkIdType document,
    const vtkStdString& uri,
    const vtkStdString& mime_type,
    const vtkTypeUInt8* content_begin,
    const vtkTypeUInt8* content_end,
    vtkUnicodeString& text,
    vtkIdTypeArray* tag_document,
    vtkIdTypeArray* tag_begin,
    vtkIdTypeArray* tag_end,
    vtkStringArray* tag_type) = 0;

protected:
  vtkTextExtractionStrategy();
  virtual ~vtkTextExtractionStrategy();

private:
  vtkTextExtractionStrategy(const vtkTextExtractionStrategy&); //Not implemented.
  void operator=(const vtkTextExtractionStrategy&); //Not implemented.
};

#endif // !_vtkTextExtractionStrategy_h

