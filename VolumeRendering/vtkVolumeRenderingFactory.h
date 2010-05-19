/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRenderingFactory.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVolumeRenderingFactory - 
// .SECTION Description

#ifndef __vtkVolumeRenderingFactory_h
#define __vtkVolumeRenderingFactory_h

#include "vtkObject.h"

class VTK_VOLUMERENDERING_EXPORT vtkVolumeRenderingFactory : public vtkObject
{
public:
  static vtkVolumeRenderingFactory *New();
  vtkTypeMacro(vtkVolumeRenderingFactory,vtkObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create and return an instance of the named vtk object.
  // This method first checks the vtkObjectFactory to support
  // dynamic loading. 
  static vtkObject* CreateInstance(const char* vtkclassname);

protected:
  vtkVolumeRenderingFactory() {};

private:
  vtkVolumeRenderingFactory(const vtkVolumeRenderingFactory&);  // Not implemented.
  void operator=(const vtkVolumeRenderingFactory&);  // Not implemented.
};

#endif
