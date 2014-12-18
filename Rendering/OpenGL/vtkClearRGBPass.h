/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClearRGBPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkClearRGBPass - Paint in the color buffer.
// .SECTION Description
// Clear the color buffer to the specified color.
//
// .SECTION See Also
// vtkValuePasses

#ifndef vtkClearRGBPass_h
#define vtkClearRGBPass_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkRenderPass.h"

class vtkOpenGLRenderWindow;

class VTKRENDERINGOPENGL_EXPORT vtkClearRGBPass : public vtkRenderPass
{
public:
  static vtkClearRGBPass *New();
  vtkTypeMacro(vtkClearRGBPass,vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Perform rendering according to a render state s.
  virtual void Render(const vtkRenderState *s);

  // Description:
  // Set/Get the background color of the rendering screen using an rgb color
  // specification.
  vtkSetVector3Macro(Background,double);
  vtkGetVector3Macro(Background,double);

 protected:
  // Description:
  // Default constructor.
  vtkClearRGBPass();

  // Description:
  // Destructor.
  virtual ~vtkClearRGBPass();

  double Background[3];

 private:
  vtkClearRGBPass(const vtkClearRGBPass&);  // Not implemented.
  void operator=(const vtkClearRGBPass&);  // Not implemented.
};

#endif
