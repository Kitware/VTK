/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagingFactory.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImagingFactory - 
// .SECTION Description

#ifndef __vtkImagingFactory_h
#define __vtkImagingFactory_h

#include "vtkObject.h"

class VTK_RENDERING_EXPORT vtkImagingFactory : public vtkObject
{
public:
  static vtkImagingFactory *New() {return new vtkImagingFactory;};
  vtkTypeRevisionMacro(vtkImagingFactory,vtkObject);

  // Description:
  // Create and return an instance of the named vtk object.
  // This method first checks the vtkObjectFactory to support
  // dynamic loading. 
  static vtkObject* CreateInstance(const char* vtkclassname);

protected:
  vtkImagingFactory() {};
private:
  vtkImagingFactory(const vtkImagingFactory&);  // Not implemented.
  void operator=(const vtkImagingFactory&);  // Not implemented.
};

#endif
