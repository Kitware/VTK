/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLLight.h
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
// .NAME vtkOpenGLLight - OpenGL light
// .SECTION Description
// vtkOpenGLLight is a concrete implementation of the abstract class vtkLight.
// vtkOpenGLLight interfaces to the OpenGL rendering library.

#ifndef __vtkOpenGLLight_h
#define __vtkOpenGLLight_h

#include "vtkLight.h"

class vtkOpenGLRenderer;

class VTK_RENDERING_EXPORT vtkOpenGLLight : public vtkLight
{
public:
  static vtkOpenGLLight *New();
  vtkTypeRevisionMacro(vtkOpenGLLight,vtkLight);

  // Description:
  // Implement base class method.
  void Render(vtkRenderer *ren,int light_index);
  
protected:  
  vtkOpenGLLight() {};
  ~vtkOpenGLLight() {};
private:
  vtkOpenGLLight(const vtkOpenGLLight&);  // Not implemented.
  void operator=(const vtkOpenGLLight&);  // Not implemented.
};

#endif

