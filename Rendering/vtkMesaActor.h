/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMesaActor - Mesa actor
// .SECTION Description
// vtkMesaActor is a concrete implementation of the abstract class vtkActor.
// vtkMesaActor interfaces to the Mesa rendering library.

#ifndef __vtkMesaActor_h
#define __vtkMesaActor_h

#include "vtkActor.h"

class vtkMesaRenderer;

class VTK_RENDERING_EXPORT vtkMesaActor : public vtkActor
{
protected:
  
public:
  static vtkMesaActor *New();
  vtkTypeMacro(vtkMesaActor,vtkActor);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Actual actor render method.
  void Render(vtkRenderer *ren, vtkMapper *mapper);
  
  // Description:
  // Create a vtkMesaProperty, used by the super class to 
  // create a compatible class with the vtkMesaActor.
  vtkProperty* MakeProperty();
protected:
  vtkMesaActor() {};
  ~vtkMesaActor() {};

private:
  vtkMesaActor(const vtkMesaActor&);  // Not implemented.
  void operator=(const vtkMesaActor&);  // Not implemented.
};

#endif

