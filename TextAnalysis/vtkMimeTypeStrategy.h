/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMimeTypeStrategy.h

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

#ifndef _vtkMimeTypeStrategy_h
#define _vtkMimeTypeStrategy_h

#include <vtkObject.h>
#include <vtkStdString.h> //Needed for lookup

// .NAME vtkMimeTypeStrategy - Abstract interface for an object that can identify the
// MIME type of a resource.
//
// .SECTION Description
// Concrete derivatives of vtkMimeTypeStrategy implement strategies for looking-up the
// MIME type of a resource, given its URI, its content, or both.
//
// .SECTION See Also
// vtkMimeTypes, vtkFileExtensionMimeTypeStrategy.
//
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

class VTK_TEXT_ANALYSIS_EXPORT vtkMimeTypeStrategy :
  public vtkObject
{
public:
  vtkTypeMacro(vtkMimeTypeStrategy, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Given a resource URI (which could be empty), and the resource contents (would could
  // be empty), implementations should return the MIME type of the resource, or empty-string
  // if the type cannot be identified.
  virtual vtkStdString Lookup(const vtkStdString& uri, const vtkTypeUInt8* begin, const vtkTypeUInt8* end) = 0;

protected:
  vtkMimeTypeStrategy();
  virtual ~vtkMimeTypeStrategy();

private:
  vtkMimeTypeStrategy(const vtkMimeTypeStrategy&); //Not implemented.
  void operator=(const vtkMimeTypeStrategy&); //Not implemented.
};

#endif // !_vtkMimeTypeStrategy_h

