/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLPolyDataMapper2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLPolyDataMapper2D - 2D PolyData support for OpenGL
// .SECTION Description
// vtkOpenGLPolyDataMapper2D provides 2D PolyData annotation support for
// vtk under OpenGL.  Normally the user should use vtkPolyDataMapper2D
// which in turn will use this class.

// .SECTION See Also
// vtkPolyDataMapper2D

#ifndef __vtkOpenGL2PolyDataMapper2D_h
#define __vtkOpenGL2PolyDataMapper2D_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkPolyDataMapper2D.h"

class vtkRenderer;
class vtkPoints;

namespace vtkgl {struct CellBO; }

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGL2PolyDataMapper2D : public vtkPolyDataMapper2D
{
public:
  vtkTypeMacro(vtkOpenGL2PolyDataMapper2D, vtkPolyDataMapper2D);
  static vtkOpenGL2PolyDataMapper2D *New();
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Actually draw the poly data.
  void RenderOverlay(vtkViewport* viewport, vtkActor2D* actor);

protected:
  vtkOpenGL2PolyDataMapper2D();
  ~vtkOpenGL2PolyDataMapper2D();

  // Description:
  // Build the shader source code
  virtual void BuildShader(std::string &VertexCode, std::string &fragmentCode, vtkViewport *ren, vtkActor2D *act);

  // Description:
  // Determine what shader to use and compile/link it
  virtual void UpdateShader(vtkgl::CellBO &cellBO, vtkViewport *viewport, vtkActor2D *act);

    // Description:
  // Set the shader parameteres related to the Camera
  void SetCameraShaderParameters(vtkgl::CellBO &cellBO, vtkViewport *viewport, vtkActor2D *act);

  // Description:
  // Set the shader parameteres related to the property
  void SetPropertyShaderParameters(vtkgl::CellBO &cellBO, vtkViewport *viewport, vtkActor2D *act);

  // Description:
  // Update the scene when necessary.
  void UpdateVBO(vtkActor2D *act, vtkViewport *viewport);

  class Private;
  Private *Internal;

  vtkTimeStamp VBOUpdateTime; // When was the VBO updated?
  vtkPoints *TransformedPoints;

private:
  vtkOpenGL2PolyDataMapper2D(const vtkOpenGL2PolyDataMapper2D&);  // Not implemented.
  void operator=(const vtkOpenGL2PolyDataMapper2D&);  // Not implemented.
};

#endif
