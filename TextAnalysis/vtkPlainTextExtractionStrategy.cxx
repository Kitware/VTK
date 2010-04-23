/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlainTextExtractionStrategy.cxx

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

#include <vtkIdTypeArray.h>
#include <vtkObjectFactory.h>
#include <vtkMimeTypes.h>
#include <vtkPlainTextExtractionStrategy.h>
#include <vtkStringArray.h>
#include <vtkUnicodeStringArray.h>

vtkStandardNewMacro(vtkPlainTextExtractionStrategy);

vtkPlainTextExtractionStrategy::vtkPlainTextExtractionStrategy()
{
}

vtkPlainTextExtractionStrategy::~vtkPlainTextExtractionStrategy()
{
}

void vtkPlainTextExtractionStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

bool vtkPlainTextExtractionStrategy::Extract(
  const vtkIdType document,
  const vtkStdString& vtkNotUsed(uri),
  const vtkStdString& mime_type,
  const vtkTypeUInt8* content_begin,
  const vtkTypeUInt8* content_end,
  vtkUnicodeString& text,
  vtkIdTypeArray* tag_document,
  vtkIdTypeArray* tag_begin,
  vtkIdTypeArray* tag_end,
  vtkStringArray* tag_type)
{
  // Determine whether we can handle this content or not ...
  bool supported = false;
  if(vtkMimeTypes::Match("text/*", mime_type))
    supported = true;
  else if(vtkMimeTypes::Match("application/x-latex", mime_type))
    supported = true;
  else if(vtkMimeTypes::Match("application/x-tex", mime_type))
    supported = true;
  if(!supported)
    return false;

  // Extract text from the content ...
  text = vtkUnicodeString::from_utf8(reinterpret_cast<const char*>(content_begin), reinterpret_cast<const char*>(content_end));

  // Generate a tag for the content ...
  tag_document->InsertNextValue(document);
  tag_begin->InsertNextValue(0);
  tag_end->InsertNextValue(text.character_count());
  tag_type->InsertNextValue("TEXT");

  return true;
}

