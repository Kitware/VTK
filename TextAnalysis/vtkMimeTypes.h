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

/// Helper class for determining the MIME-type of files at runtime.  To use,
/// create an instance of vtkMimeTypes, then call the Lookup() method to
/// determine the MIME-type of each file of-interest.
class VTK_TEXT_ANALYSIS_EXPORT vtkMimeTypes :
  public vtkObject
{
public:
  static vtkMimeTypes* New();
  vtkTypeRevisionMacro(vtkMimeTypes, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Returns the MIME-type of a file, or empty-string if the type cannot be identified
  vtkStdString Lookup(const vtkStdString& path);

private:
  vtkMimeTypes();
  ~vtkMimeTypes();

  vtkMimeTypes(const vtkMimeTypes&); //Not implemented.
  void operator=(const vtkMimeTypes&); //Not implemented.

//BTX
  class implementation;
  implementation* const Implementation;
//ETX
};

#endif // !_vtkMimeTypes_h

