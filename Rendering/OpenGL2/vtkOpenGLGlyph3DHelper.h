/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLGlyph3DHelper - PolyDataMapper using OpenGL to render.
// .SECTION Description
// PolyDataMapper that uses a OpenGL to do the actual rendering.

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

  // Description
  // Fast path for rendering glyphs comprised of only one type of primative
  void GlyphRender(vtkRenderer* ren, vtkActor* actor, vtkIdType numPts,
      std::vector<unsigned char> &colors, std::vector<float> &matrices,
      std::vector<float> &normalMatrices, std::vector<vtkIdType> &pickIds,
      unsigned long pointMTime);

protected:
  vtkOpenGLGlyph3DHelper();
  ~vtkOpenGLGlyph3DHelper();

  // special opengl 32 version that uses instances
#if GL_ES_VERSION_2_0 != 1 || GL_ES_VERSION_3_0 == 1
  void GlyphRenderInstances(vtkRenderer* ren, vtkActor* actor, vtkIdType numPts,
      std::vector<unsigned char> &colors, std::vector<float> &matrices,
      std::vector<float> &normalMatrices,
      unsigned long pointMTime);
#endif

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

  bool UseFastPath;
  bool UsingInstancing;

  float* ModelTransformMatrix;
  float* ModelNormalMatrix;
  unsigned char* ModelColor;

  vtkgl::BufferObject NormalMatrixBuffer;
  vtkgl::BufferObject MatrixBuffer;
  vtkgl::BufferObject ColorBuffer;
  vtkTimeStamp InstanceBuffersLoadTime;


private:
  vtkOpenGLGlyph3DHelper(const vtkOpenGLGlyph3DHelper&); // Not implemented.
  void operator=(const vtkOpenGLGlyph3DHelper&); // Not implemented.
};

#endif
