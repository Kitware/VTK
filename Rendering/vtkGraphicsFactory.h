/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphicsFactory.h
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
// .NAME vtkGraphicsFactory - 
// .SECTION Description

#ifndef __vtkGraphicsFactory_h
#define __vtkGraphicsFactory_h

#include "vtkObject.h"

class VTK_RENDERING_EXPORT vtkGraphicsFactory : public vtkObject
{
public:
  static vtkGraphicsFactory *New() {return new vtkGraphicsFactory;};
  vtkTypeRevisionMacro(vtkGraphicsFactory,vtkObject);

  // Description:
  // Create and return an instance of the named vtk object.
  // This method first checks the vtkObjectFactory to support
  // dynamic loading. 
  static vtkObject* CreateInstance(const char* vtkclassname);

  // Description:
  // What rendering library has the user requested
  static const char *GetRenderLibrary();
  
protected:
  vtkGraphicsFactory() {};
private:
  vtkGraphicsFactory(const vtkGraphicsFactory&);  // Not implemented.
  void operator=(const vtkGraphicsFactory&);  // Not implemented.
};

#endif
