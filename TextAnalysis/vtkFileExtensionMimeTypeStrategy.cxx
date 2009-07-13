/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFileExtensionMimeTypeStrategy.cxx

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

#include <vtkFileExtensionMimeTypeStrategy.h>
#include <vtkObjectFactory.h>
#include <vtkStdString.h>

#include <boost/algorithm/string.hpp>

vtkCxxRevisionMacro(vtkFileExtensionMimeTypeStrategy, "1.2");
vtkStandardNewMacro(vtkFileExtensionMimeTypeStrategy);

class vtkFileExtensionMimeTypeStrategy::implementation
{
public:
  static bool Lookup(const vtkStdString& path, const vtkStdString& suffix, const vtkStdString& mime_type, vtkStdString& result)
  {
    if(boost::iends_with(path, suffix))
      {
      result = mime_type;
      return true;
      }
      
    return false;
  }
};

vtkFileExtensionMimeTypeStrategy::vtkFileExtensionMimeTypeStrategy()
{
}

vtkFileExtensionMimeTypeStrategy::~vtkFileExtensionMimeTypeStrategy()
{
}

void vtkFileExtensionMimeTypeStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkStdString vtkFileExtensionMimeTypeStrategy::Lookup(const vtkStdString& path)
{
  vtkStdString mime_type;

  if(implementation::Lookup(path, ".ai", "application/postscript", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".bmp", "image/bmp", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".c", "text/plain", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".cpp", "text/plain", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".css", "text/css", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".csv", "text/csv", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".cxx", "text/plain", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".dll", "application/octet-stream", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".doc", "application/msword", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".dot", "application/msword", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".dvi", "application/x-dvi", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".eps", "application/postscript", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".exe", "application/octet-stream", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".gz", "application/x-gzip", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".gif", "image/gif", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".h", "text/plain", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".hpp", "text/plain", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".htm", "text/html", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".html", "text/html", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".hxx", "text/plain", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".ico", "image/x-icon", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".jpe", "image/jpeg", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".jpeg", "image/jpeg", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".jpg", "image/jpeg", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".js", "application/x-javascript", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".latex", "application/x-latex", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".lib", "application/octet-stream", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".pdf", "application/pdf", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".png", "image/png", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".ppt", "application/vnd.ms-powerpoint", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".ps", "application/postscript", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".shtml", "text/html", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".tcl", "application/x-tcl", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".tif", "image/tiff", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".tiff", "image/tiff", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".txt", "text/plain", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".xls", "application/vnd.ms-excel", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".xml", "text/xml", mime_type)) return mime_type;
  if(implementation::Lookup(path, ".zip", "application/zip", mime_type)) return mime_type;

  return vtkStdString();
}

