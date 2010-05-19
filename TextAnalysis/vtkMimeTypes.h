/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMimeTypes.h

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

#ifndef _vtkMimeTypes_h
#define _vtkMimeTypes_h

#include <vtkObject.h>
#include <vtkStdString.h> //Needed for lookup

class vtkMimeTypeStrategy;

// .NAME vtkMimeTypes - Determines the MIME type of a resource.
//
// .SECTION Description
// vtkMimeTypes is a helper class for determining the MIME type of a resource at runtime.
// To use it, create an instance of vtkMimeTypes, then call the Lookup() method to
// determine the MIME type of each resource of-interest.
//
// vtkMimeTypes relies on a set of strategy objects to perform the actual lookups.
// These strategy objects may determine the MIME type based on arbitrary methods,
// including looking at file extensions, examining the contents of the resource, or
// some combination thereof.
//
// By default, vtkMimeTypes is configured with a simple cross-platform strategy
// that identifies resources based on a hard-coded list of filename extensions, but
// you can supplement this process with your own strategies.  The list of strategies
// is executed in-order to determine the MIME type of a resource, so earlier strategies
// "override" later strategies.
//
// .SECTION See Also
// vtkMimeTypeStrategy, vtkFileExtensionMimeTypeStrategy.
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

class VTK_TEXT_ANALYSIS_EXPORT vtkMimeTypes :
  public vtkObject
{
public:
  static vtkMimeTypes* New();
  vtkTypeMacro(vtkMimeTypes, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Clear the list of strategies.
  void ClearStrategies();
  // Description:
  // Prepend a strategy to the list of strategies.  vtkMimeTypes assumes ownership
  // of the supplied object.
  void PrependStrategy(vtkMimeTypeStrategy* strategy);
  // Description:
  // Append a strategy to the list of strategies.  vtkMimeTypes assumes ownership
  // of the supplied object.
  void AppendStrategy(vtkMimeTypeStrategy* strategy);

  // Description:
  // Given a resource URI, returns the MIME-type of the resource, or empty-string
  // if the type cannot be identified.
  vtkStdString Lookup(const vtkStdString& uri);
  // Description:
  // Given the contents of a resource, returns the MIME-type of the resource, or
  // empty-string if the type cannot be identified.
  vtkStdString Lookup(const char* begin, const char* end);
  // Description:
  // Given the contents of a resource, returns the MIME-type of the resource, or
  // empty-string if the type cannot be identified.
  vtkStdString Lookup(const vtkTypeUInt8* begin, const vtkTypeUInt8* end);
  // Description:
  // Given a resource URI and its contents, returns the MIME-type of the resource,
  // or empty-string if the type cannot be identified.
  vtkStdString Lookup(const vtkStdString& uri, const char* begin, const char* end);
  // Description:
  // Given a resource URI and its contents, returns the MIME-type of the resource,
  // or empty-string if the type cannot be identified.
  vtkStdString Lookup(const vtkStdString& uri, const vtkTypeUInt8* begin, const vtkTypeUInt8* end);

  // Description:
  // Returns true iff a Mime pattern matches the given type.  Handles wildcards
  // so the pattern "*/*" will match any type (including empty type), and "text/*"
  // will match "text/plain", "text/html", "text/xml", etc.
  static bool Match(const vtkStdString& pattern, const vtkStdString& type);

//BTX
private:
  vtkMimeTypes();
  ~vtkMimeTypes();

  vtkMimeTypes(const vtkMimeTypes&); //Not implemented.
  void operator=(const vtkMimeTypes&); //Not implemented.

  class Implementation;
  Implementation* const Internal;
//ETX
};

#endif // !_vtkMimeTypes_h

