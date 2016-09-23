/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLStickMapper
 * @brief   use imposters to draw cylinders
 *
 * PolyDataMapper that uses imposters to draw cylinders/sticks
 * for ball/stick style molecular rendering. Supports picking.
*/

#ifndef vtkOpenGLStickMapper_h
#define vtkOpenGLStickMapper_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkOpenGLPolyDataMapper.h"

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLStickMapper : public vtkOpenGLPolyDataMapper
{
public:
  static vtkOpenGLStickMapper* New();
  vtkTypeMacro(vtkOpenGLStickMapper, vtkOpenGLPolyDataMapper)
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Convenience method to set the array to scale with.
   */
  vtkSetStringMacro(ScaleArray);
  //@}

  //@{
  /**
   * Convenience method to set the array to orient with
   */
  vtkSetStringMacro(OrientationArray);
  //@}

  //@{
  /**
   * Convenience method to set the array to select with
   */
  vtkSetStringMacro(SelectionIdArray);
  //@}

protected:
  vtkOpenGLStickMapper();
  ~vtkOpenGLStickMapper();

  /**
   * Create the basic shaders before replacement
   */
  virtual void GetShaderTemplate(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act);

  /**
   * Perform string replacments on the shader templates
   */
  virtual void ReplaceShaderValues(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act);

  /**
   * Set the shader parameters related to the Camera
   */
  virtual void SetCameraShaderParameters(vtkOpenGLHelper &cellBO, vtkRenderer *ren, vtkActor *act);

  /**
   * Set the shader parameters related to the actor/mapper
   */
  virtual void SetMapperShaderParameters(vtkOpenGLHelper &cellBO, vtkRenderer *ren, vtkActor *act);

  const char *ScaleArray;
  const char *OrientationArray;
  const char *SelectionIdArray;

  /**
   * Does the VBO/IBO need to be rebuilt
   */
  virtual bool GetNeedToRebuildBufferObjects(vtkRenderer *ren, vtkActor *act);

  /**
   * Update the VBO to contain point based values
   */
  virtual void BuildBufferObjects(vtkRenderer *ren, vtkActor *act);

  virtual void RenderPieceDraw(vtkRenderer *ren, vtkActor *act);

private:
  vtkOpenGLStickMapper(const vtkOpenGLStickMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLStickMapper&) VTK_DELETE_FUNCTION;
};

#endif
