/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaProperty.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMesaProperty - Mesa property
// .SECTION Description
// vtkMesaProperty is a concrete implementation of the abstract class 
// vtkProperty. vtkMesaProperty interfaces to the Mesa rendering library.

#ifndef __vtkMesaProperty_h
#define __vtkMesaProperty_h

#include "vtkProperty.h"

class vtkMesaRenderer;

class VTK_RENDERING_EXPORT vtkMesaProperty : public vtkProperty
{
public:
  static vtkMesaProperty *New();
  vtkTypeMacro(vtkMesaProperty,vtkProperty);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implement base class method.
  void Render(vtkActor *a, vtkRenderer *ren);

  // Description:
  // Implement base class method.
  void BackfaceRender(vtkActor *a, vtkRenderer *ren);

protected:
  vtkMesaProperty() {};
  ~vtkMesaProperty() {};
private:
  vtkMesaProperty(const vtkMesaProperty&);  // Not implemented.
  void operator=(const vtkMesaProperty&);  // Not implemented.
};

#endif
