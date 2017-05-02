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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Fast path for rendering glyphs comprised of only one type of primitive
   * Must set this->CurrentInput explicitly before calling.
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
  void ReleaseGraphicsResources(vtkWindow *window) VTK_OVERRIDE;

protected:
  vtkOpenGLGlyph3DHelper();
  ~vtkOpenGLGlyph3DHelper() VTK_OVERRIDE;

  // special opengl 32 version that uses instances
  void GlyphRenderInstances(vtkRenderer* ren, vtkActor* actor, vtkIdType numPts,
      std::vector<unsigned char> &colors, std::vector<float> &matrices,
      std::vector<float> &normalMatrices,
      vtkMTimeType pointMTime);

  /**
   * Create the basic shaders before replacement
   */
  void GetShaderTemplate(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act) VTK_OVERRIDE;

  //@{
  /**
   * Perform string replacments on the shader templates
   */
  void ReplaceShaderPicking(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act) VTK_OVERRIDE;
  void ReplaceShaderColor(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act) VTK_OVERRIDE;
  void ReplaceShaderNormal(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act) VTK_OVERRIDE;
  void ReplaceShaderClip(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act) VTK_OVERRIDE;
  void ReplaceShaderPositionVC(
    std::map<vtkShader::Type, vtkShader *> shaders,
    vtkRenderer *ren, vtkActor *act) VTK_OVERRIDE;
  //@}

  /**
   * Set the shader parameteres related to the actor/mapper
   */
  void SetMapperShaderParameters(
    vtkOpenGLHelper &cellBO, vtkRenderer *ren, vtkActor *act) VTK_OVERRIDE;

  bool UsingInstancing;

  vtkOpenGLBufferObject *NormalMatrixBuffer;
  vtkOpenGLBufferObject *MatrixBuffer;
  vtkOpenGLBufferObject *ColorBuffer;
  vtkTimeStamp InstanceBuffersBuildTime;
  vtkTimeStamp InstanceBuffersLoadTime;


private:
  vtkOpenGLGlyph3DHelper(const vtkOpenGLGlyph3DHelper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLGlyph3DHelper&) VTK_DELETE_FUNCTION;
};

#endif
