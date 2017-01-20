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
 * @class   vtkOpenGLSphereMapper
 * @brief   draw spheres using imposters
 *
 * An OpenGL mapper that uses imposters to draw spheres. Supports
 * transparency and picking as well.
*/

#ifndef vtkOpenGLSphereMapper_h
#define vtkOpenGLSphereMapper_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkOpenGLPolyDataMapper.h"

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLSphereMapper : public vtkOpenGLPolyDataMapper
{
public:
  static vtkOpenGLSphereMapper* New();
  vtkTypeMacro(vtkOpenGLSphereMapper, vtkOpenGLPolyDataMapper)
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Convenience method to set the array to scale with.
   */
  vtkSetStringMacro(ScaleArray);
  //@}

  //@{
  /**
   * This value will be used for the radius is the scale
   * array is not provided.
   */
   vtkSetMacro(Radius, float);
   vtkGetMacro(Radius, float);

  /**
   * This calls RenderPiece (twice when transparent)
   */
  void Render(vtkRenderer *ren, vtkActor *act) VTK_OVERRIDE;

protected:
  vtkOpenGLSphereMapper();
  ~vtkOpenGLSphereMapper() VTK_OVERRIDE;

  /**
   * Create the basic shaders before replacement
   */
  void GetShaderTemplate(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act) VTK_OVERRIDE;

  /**
   * Perform string replacments on the shader templates
   */
  void ReplaceShaderValues(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act) VTK_OVERRIDE;

  /**
   * Set the shader parameters related to the Camera
   */
  void SetCameraShaderParameters(vtkOpenGLHelper &cellBO, vtkRenderer *ren, vtkActor *act) VTK_OVERRIDE;

  /**
   * Set the shader parameters related to the actor/mapper
   */
  void SetMapperShaderParameters(vtkOpenGLHelper &cellBO, vtkRenderer *ren, vtkActor *act) VTK_OVERRIDE;

  const char *ScaleArray;

  /**
   * Does the VBO/IBO need to be rebuilt
   */
  bool GetNeedToRebuildBufferObjects(vtkRenderer *ren, vtkActor *act) VTK_OVERRIDE;

  /**
   * Update the VBO to contain point based values
   */
  void BuildBufferObjects(vtkRenderer *ren, vtkActor *act) VTK_OVERRIDE;

  void RenderPieceDraw(vtkRenderer *ren, vtkActor *act) VTK_OVERRIDE;

  virtual void CreateVBO(
    float * points, vtkIdType numPts,
    unsigned char *colors, int colorComponents,
    vtkIdType nc,
    float *sizes, vtkIdType ns, vtkRenderer *ren);

  // used for transparency
  bool Invert;
  float Radius;

private:
  vtkOpenGLSphereMapper(const vtkOpenGLSphereMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLSphereMapper&) VTK_DELETE_FUNCTION;
};

#endif
