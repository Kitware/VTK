/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFileExtensionMimeTypeStrategy.h

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

#ifndef _vtkFileExtensionMimeTypeStrategy_h
#define _vtkFileExtensionMimeTypeStrategy_h

#include <vtkMimeTypeStrategy.h>

// .NAME vtkFileExtentionMimeTypeStrategy - Determines the MIME type of a resource
// based on a hard-coded list of file extensions.
//
// .SECTION See Also
// vtkMimeTypeStrategy, vtkMimeTypes.
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

class VTK_TEXT_ANALYSIS_EXPORT vtkFileExtensionMimeTypeStrategy :
  public vtkMimeTypeStrategy
{
public:
  static vtkFileExtensionMimeTypeStrategy* New();
  vtkTypeMacro(vtkFileExtensionMimeTypeStrategy, vtkMimeTypeStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkStdString Lookup(const vtkStdString& uri, const vtkTypeUInt8* begin, const vtkTypeUInt8* end);

protected:
  vtkFileExtensionMimeTypeStrategy();
  virtual ~vtkFileExtensionMimeTypeStrategy();
   
private:
  vtkFileExtensionMimeTypeStrategy(const vtkFileExtensionMimeTypeStrategy&); //Not implemented.
  void operator=(const vtkFileExtensionMimeTypeStrategy&); //Not implemented.

//BTX
  class implementation;
//ETX
};

#endif // !_vtkFileExtensionMimeTypeStrategy_h

