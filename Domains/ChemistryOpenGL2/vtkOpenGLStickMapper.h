/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLStickMapper - PolyDataMapper using OpenGL to render.
// .SECTION Description
// PolyDataMapper that uses a OpenGL to do the actual rendering.

#ifndef __vtkOpenGLStickMapper_h
#define __vtkOpenGLStickMapper_h

#include "vtkDOmainsChemistryOpenGL2Module.h" // For export macro
#include "vtkOpenGLPolyDataMapper.h"

class VTKDOMAINSCHEMISTRYOPENGL2_EXPORT vtkOpenGLStickMapper : public vtkOpenGLPolyDataMapper
{
public:
  static vtkOpenGLStickMapper* New();
  vtkTypeMacro(vtkOpenGLStickMapper, vtkOpenGLPolyDataMapper)
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Convenience method to set the array to scale with.
  vtkSetStringMacro(ScaleArray);

  // Description:
  // Convenience method to set the array to orient with
  vtkSetStringMacro(OrientationArray);

  // Description:
  // Convenience method to set the array to select with
  vtkSetStringMacro(SelectionIdArray);

protected:
  vtkOpenGLStickMapper();
  ~vtkOpenGLStickMapper();

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
  const char *OrientationArray;
  const char *SelectionIdArray;

  // Description:
  // Update the VBO to contain point based values
  virtual void UpdateVBO(vtkRenderer *ren, vtkActor *act);

  virtual void RenderPieceDraw(vtkRenderer *ren, vtkActor *act);

private:
  vtkOpenGLStickMapper(const vtkOpenGLStickMapper&); // Not implemented.
  void operator=(const vtkOpenGLStickMapper&); // Not implemented.
};

#endif
