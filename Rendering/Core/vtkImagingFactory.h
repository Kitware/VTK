/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagingFactory.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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
  static vtkImagingFactory *New();
  vtkTypeMacro(vtkImagingFactory,vtkObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create and return an instance of the named vtk object.
  // This method first checks the vtkObjectFactory to support
  // dynamic loading. 
  static vtkObject* CreateInstance(const char* vtkclassname);

  // Description:
  // This option enables the creation of Mesa classes
  // instead of the OpenGL classes when using mangled Mesa.
  static void SetUseMesaClasses(int use);
  static int  GetUseMesaClasses();

protected:
  vtkImagingFactory() {};

  static int UseMesaClasses;

private:
  vtkImagingFactory(const vtkImagingFactory&);  // Not implemented.
  void operator=(const vtkImagingFactory&);  // Not implemented.
};

#endif
