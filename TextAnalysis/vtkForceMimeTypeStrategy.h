/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkForceMimeTypeStrategy.h

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

#ifndef _vtkForceMimeTypeStrategy_h
#define _vtkForceMimeTypeStrategy_h

#include <vtkMimeTypeStrategy.h>

// .NAME vtkForceMimeTypeStrategy - Returns a specific mime type no matter what
//
// .SECTION Description
// vtkForceMimeTypeStrategy returns the same mime type for all files.  The type
// can be specified by the caller, and defaults to text/plain.
//
// .SECTION See Also
// vtkMimeTypeStrategy, vtkMimeTypes.
//
// .SECTION Thanks 
// Developed by Andy Wilson (atwilso@sandia.gov) at Sandia National Laboratories.

class VTK_TEXT_ANALYSIS_EXPORT vtkForceMimeTypeStrategy :
  public vtkMimeTypeStrategy
{
public:
  static vtkForceMimeTypeStrategy* New();
  vtkTypeMacro(vtkForceMimeTypeStrategy, vtkMimeTypeStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkGetStringMacro(MimeType);
  vtkSetStringMacro(MimeType);

  vtkStdString Lookup(const vtkStdString& uri, const vtkTypeUInt8* begin, const vtkTypeUInt8* end);

//BTX
protected:
  vtkForceMimeTypeStrategy();
  virtual ~vtkForceMimeTypeStrategy();

  char* MimeType;
 
private:
  vtkForceMimeTypeStrategy(const vtkForceMimeTypeStrategy&); //Not implemented.
  void operator=(const vtkForceMimeTypeStrategy&); //Not implemented.
//ETX
};

#endif // !_vtkForceMimeTypeStrategy_h

