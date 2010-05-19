/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaLight.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMesaLight - Mesa light
// .SECTION Description
// vtkMesaLight is a concrete implementation of the abstract class vtkLight.
// vtkMesaLight interfaces to the Mesa rendering library.

#ifndef __vtkMesaLight_h
#define __vtkMesaLight_h

#include "vtkLight.h"

class vtkMesaRenderer;

class VTK_RENDERING_EXPORT vtkMesaLight : public vtkLight
{
public:
  static vtkMesaLight *New();
  vtkTypeMacro(vtkMesaLight,vtkLight);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implement base class method.
  void Render(vtkRenderer *ren,int light_index);
  
protected:  
  vtkMesaLight() {};
  ~vtkMesaLight() {};
private:
  vtkMesaLight(const vtkMesaLight&);  // Not implemented.
  void operator=(const vtkMesaLight&);  // Not implemented.
};

#endif

