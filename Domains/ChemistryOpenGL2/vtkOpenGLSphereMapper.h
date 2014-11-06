/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLSphereMapper - draw spheres using imposters
// .SECTION Description
// An OpenGL mapper that uses imposters to draw spheres. Supports
// transparency and picking as well.

#ifndef __vtkOpenGLSphereMapper_h
#define __vtkOpenGLSphereMapper_h

#include "vtkDomainsChemistryOpenGL2Module.h" // For export macro
#include "vtkOpenGLPolyDataMapper.h"

class VTKDOMAINSCHEMISTRYOPENGL2_EXPORT vtkOpenGLSphereMapper : public vtkOpenGLPolyDataMapper
{
public:
  static vtkOpenGLSphereMapper* New();
  vtkTypeMacro(vtkOpenGLSphereMapper, vtkOpenGLPolyDataMapper)
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Convenience method to set the array to scale with.
  vtkSetStringMacro(ScaleArray);

  // Description:
  // This calls RenderPiece (twice when transparent)
  virtual void Render(vtkRenderer *ren, vtkActor *act);

protected:
  vtkOpenGLSphereMapper();
  ~vtkOpenGLSphereMapper();

  // Description:
  // Create the basic shaders before replacement
  virtual void GetShaderTemplate(std::string &VertexCode,
                           std::string &fragmentCode,
                           std::string &geometryCode,
                           int lightComplexity,
                           vtkRenderer *ren, vtkActor *act);

  // Description:
  // Perform string replacments on the shader templates
  virtual void ReplaceShaderValues(std::string &VertexCode,
                           std::string &fragmentCode,
                           std::string &geometryCode,
                           int lightComplexity,
                           vtkRenderer *ren, vtkActor *act);

  // Description:
  // Set the shader parameteres related to the Camera
  virtual void SetCameraShaderParameters(vtkgl::CellBO &cellBO, vtkRenderer *ren, vtkActor *act);

  // Description:
  // Set the shader parameteres related to the property
  virtual void SetPropertyShaderParameters(vtkgl::CellBO &cellBO, vtkRenderer *ren, vtkActor *act);

  // Description:
  // Set the shader parameteres related to the actor/mapper
  virtual void SetMapperShaderParameters(vtkgl::CellBO &cellBO, vtkRenderer *ren, vtkActor *act);

  const char *ScaleArray;

  // Description:
  // Update the VBO to contain point based values
  virtual void UpdateVBO(vtkRenderer *ren, vtkActor *act);

  virtual void RenderPieceDraw(vtkRenderer *ren, vtkActor *act);

  // used for transparency
  bool Invert;

private:
  vtkOpenGLSphereMapper(const vtkOpenGLSphereMapper&); // Not implemented.
  void operator=(const vtkOpenGLSphereMapper&); // Not implemented.
};

#endif
