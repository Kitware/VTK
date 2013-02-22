/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphicsFactory.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkObject.h"

class VTKRENDERINGCORE_EXPORT vtkGraphicsFactory : public vtkObject
{
public:
  static vtkGraphicsFactory *New();
  vtkTypeMacro(vtkGraphicsFactory, vtkObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create and return an instance of the named vtk object.
  // This method first checks the vtkObjectFactory to support
  // dynamic loading.
  static vtkObject* CreateInstance(const char* vtkclassname);

  // Description:
  // What rendering library has the user requested
  static const char *GetRenderLibrary();

  // Description:
  // This option enables the creation of Mesa classes
  // instead of the OpenGL classes when using mangled Mesa.
  static void SetUseMesaClasses(int use);
  static int  GetUseMesaClasses();

  // Description:
  // This option enables the off-screen only mode. In this mode no X calls will
  // be made even when interactor is used.
  static void SetOffScreenOnlyMode(int use);
  static int  GetOffScreenOnlyMode();

protected:
  vtkGraphicsFactory() {}

  static int UseMesaClasses;
  static int OffScreenOnlyMode;

private:
  vtkGraphicsFactory(const vtkGraphicsFactory&);  // Not implemented.
  void operator=(const vtkGraphicsFactory&);  // Not implemented.
};

#endif
