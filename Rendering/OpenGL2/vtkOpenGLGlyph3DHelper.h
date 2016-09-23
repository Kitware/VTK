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
 * @class   vtkOpenGLGlyph3DHelper
 * @brief   PolyDataMapper using OpenGL to render.
 *
 * PolyDataMapper that uses a OpenGL to do the actual rendering.
*/

#ifndef vtkOpenGLGlyph3DHelper_h
#define vtkOpenGLGlyph3DHelper_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkOpenGLPolyDataMapper.h"

class vtkBitArray;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLGlyph3DHelper : public vtkOpenGLPolyDataMapper
{
public:
  static vtkOpenGLGlyph3DHelper* New();
  vtkTypeMacro(vtkOpenGLGlyph3DHelper, vtkOpenGLPolyDataMapper)
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetModelTransform(float *matrix)
  {
    this->ModelTransformMatrix = matrix;
  }

  void SetModelNormalTransform(float *matrix)
  {
    this->ModelNormalMatrix = matrix;
  }

  void SetModelColor(unsigned char *color)
  {
    this->ModelColor = color;
  }

  void SetUseFastPath(bool fastpath)
  {
    this->UseFastPath = fastpath;
    this->UsingInstancing = false;
  }

  /**
   * Fast path for rendering glyphs comprised of only one type of primative
   */
  void GlyphRender(vtkRenderer* ren, vtkActor* actor, vtkIdType numPts,
      std::vector<unsigned char> &colors, std::vector<float> &matrices,
      std::vector<float> &normalMatrices, std::vector<vtkIdType> &pickIds,
      vtkMTimeType pointMTime);

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(vtkWindow *window);

protected:
  vtkOpenGLGlyph3DHelper();
  ~vtkOpenGLGlyph3DHelper();

  // special opengl 32 version that uses instances
#if GL_ES_VERSION_2_0 != 1 || GL_ES_VERSION_3_0 == 1
  void GlyphRenderInstances(vtkRenderer* ren, vtkActor* actor, vtkIdType numPts,
      std::vector<unsigned char> &colors, std::vector<float> &matrices,
      std::vector<float> &normalMatrices,
      vtkMTimeType pointMTime);
#endif

  /**
   * Create the basic shaders before replacement
   */
  virtual void GetShaderTemplate(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act);

  //@{
  /**
   * Perform string replacments on the shader templates
   */
  virtual void ReplaceShaderPicking(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act);
  virtual void ReplaceShaderColor(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act);
  virtual void ReplaceShaderNormal(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act);
  virtual void ReplaceShaderClip(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act);
  virtual void ReplaceShaderPositionVC(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act);
  //@}

  /**
   * Set the shader parameteres related to the Camera
   */
  virtual void SetCameraShaderParameters(
    vtkOpenGLHelper &cellBO, vtkRenderer *ren, vtkActor *act);

  /**
   * Set the shader parameteres related to the property
   */
  virtual void SetPropertyShaderParameters(
    vtkOpenGLHelper &cellBO, vtkRenderer *ren, vtkActor *act);

  /**
   * Set the shader parameteres related to the actor/mapper
   */
  virtual void SetMapperShaderParameters(
    vtkOpenGLHelper &cellBO, vtkRenderer *ren, vtkActor *act);

  bool UseFastPath;
  bool UsingInstancing;

  float* ModelTransformMatrix;
  float* ModelNormalMatrix;
  unsigned char* ModelColor;

  vtkOpenGLBufferObject *NormalMatrixBuffer;
  vtkOpenGLBufferObject *MatrixBuffer;
  vtkOpenGLBufferObject *ColorBuffer;
  vtkTimeStamp InstanceBuffersLoadTime;


private:
  vtkOpenGLGlyph3DHelper(const vtkOpenGLGlyph3DHelper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLGlyph3DHelper&) VTK_DELETE_FUNCTION;
};

#endif
