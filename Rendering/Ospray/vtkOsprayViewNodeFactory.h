/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOsprayViewNodeFactory.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOsprayViewNodeFactory - matches vtk rendering classes to
// specific ospray ViewNode classes
// .SECTION Description
// Ensures that vtkOsprayPass makes ospray specific translator instances
// for every VTK rendering pipeline class instance it encounters.

#ifndef vtkOsprayViewNodeFactory_h
#define vtkOsprayViewNodeFactory_h

#include "vtkRenderingOsprayModule.h" // For export macro
#include "vtkViewNodeFactory.h"

class VTKRENDERINGOSPRAY_EXPORT vtkOsprayViewNodeFactory :
  public vtkViewNodeFactory
{
public:
  static vtkOsprayViewNodeFactory* New();
  vtkTypeMacro(vtkOsprayViewNodeFactory, vtkViewNodeFactory);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkOsprayViewNodeFactory();
  ~vtkOsprayViewNodeFactory();

private:
  vtkOsprayViewNodeFactory(const vtkOsprayViewNodeFactory&); // Not implemented.
  void operator=(const vtkOsprayViewNodeFactory&); // Not implemented.
};

#endif
