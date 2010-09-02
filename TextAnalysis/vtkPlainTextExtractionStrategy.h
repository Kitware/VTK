/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlainTextExtractionStrategy.h

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

// .NAME vtkPlainTextExtractionStrategy - text extraction strategy that works with text/* data.
//
// .SECTION Description
// Concrete implementation of vtkTextExtractionStrategy that works with text/* MIME types.
// vtkPlainTextExtractionStrategy trivially converts the contents of the given resource into
// text.  It is intended mainly as a "strategy of last resort", since more sophisticated
// strategies may wish to parse-out structured content.
//
// Generates a single "TEXT" tag that incorporates the entire text content.
//
// .SECTION See Also
// vtkTextExtraction, vtkTextExtractionStrategy.
//
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef _vtkPlainTextExtractionStrategy_h
#define _vtkPlainTextExtractionStrategy_h

#include <vtkTextExtractionStrategy.h>

class VTK_TEXT_ANALYSIS_EXPORT vtkPlainTextExtractionStrategy :
  public vtkTextExtractionStrategy
{
public:
  static vtkPlainTextExtractionStrategy* New();
  vtkTypeMacro(vtkPlainTextExtractionStrategy, vtkTextExtractionStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);

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
    vtkStringArray* tag_type);

protected:
  vtkPlainTextExtractionStrategy();
  virtual ~vtkPlainTextExtractionStrategy();

private:
  vtkPlainTextExtractionStrategy(const vtkPlainTextExtractionStrategy&); //Not implemented.
  void operator=(const vtkPlainTextExtractionStrategy&); //Not implemented.
};

#endif // !_vtkPlainTextExtractionStrategy_h

