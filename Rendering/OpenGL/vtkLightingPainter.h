/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLightingPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLightingPainter
 * @brief   abstract class defining interface for painter
 * that can handle lightin.
*/

#ifndef vtkLightingPainter_h
#define vtkLightingPainter_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkPolyDataPainter.h"

class VTKRENDERINGOPENGL_EXPORT vtkLightingPainter : public vtkPolyDataPainter
{
public:
  static vtkLightingPainter* New();
  vtkTypeMacro(vtkLightingPainter, vtkPolyDataPainter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkLightingPainter();
  ~vtkLightingPainter() VTK_OVERRIDE;

private:
  vtkLightingPainter(const vtkLightingPainter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkLightingPainter&) VTK_DELETE_FUNCTION;
};

#endif
