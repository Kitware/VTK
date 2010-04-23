/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaCamera.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMesaCamera - Mesa camera
// .SECTION Description
// vtkMesaCamera is a concrete implementation of the abstract class
// vtkCamera.  vtkMesaCamera interfaces to the Mesa rendering library.

#ifndef __vtkMesaCamera_h
#define __vtkMesaCamera_h

#include "vtkCamera.h"

class vtkMesaRenderer;

class VTK_RENDERING_EXPORT vtkMesaCamera : public vtkCamera
{
public:
  static vtkMesaCamera *New();
  vtkTypeMacro(vtkMesaCamera,vtkCamera);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implement base class method.
  void Render(vtkRenderer *ren);

  void UpdateViewport(vtkRenderer *ren);
  
protected:  
  vtkMesaCamera() {};
  ~vtkMesaCamera() {};
private:
  vtkMesaCamera(const vtkMesaCamera&);  // Not implemented.
  void operator=(const vtkMesaCamera&);  // Not implemented.
};

#endif
