/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRIBLight.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRIBLight - RIP Light
// .SECTION Description
// vtkRIBLight is a subclass of vtkLight that allows the user to
// specify light source shaders and shadow casting lights for use with RenderMan.
//
// .SECTION See Also
// vtkRIBExporter

#ifndef __vtkRIBLight_h
#define __vtkRIBLight_h

#include "vtkLight.h"

class vtkRIBRenderer;

class VTK_HYBRID_EXPORT vtkRIBLight : public vtkLight
{
public:
  static vtkRIBLight *New();
  vtkTypeMacro(vtkRIBLight,vtkLight);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkBooleanMacro(Shadows,int);
  vtkSetMacro(Shadows,int);
  vtkGetMacro(Shadows,int);

  void Render(vtkRenderer *ren, int index);
protected:
  vtkRIBLight();
  ~vtkRIBLight();

  vtkLight *Light;
  int Shadows;
private:
  vtkRIBLight(const vtkRIBLight&);  // Not implemented.
  void operator=(const vtkRIBLight&);  // Not implemented.
};

#endif
