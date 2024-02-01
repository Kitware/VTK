// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkOpenGLPointGaussianMapperHelper_h
#define vtkOpenGLPointGaussianMapperHelper_h

#include "vtkOpenGLPolyDataMapper.h"

VTK_ABI_NAMESPACE_BEGIN

class vtkPointGaussianMapper;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLPointGaussianMapperHelper : public vtkOpenGLPolyDataMapper
{
public:
  static vtkOpenGLPointGaussianMapperHelper* New();
  vtkTypeMacro(vtkOpenGLPointGaussianMapperHelper, vtkOpenGLPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkPointGaussianMapper* Owner;

  // set from parent
  float* OpacityTable;  // the table
  double OpacityScale;  // used for quick lookups
  double OpacityOffset; // used for quick lookups
  float* ScaleTable;    // the table
  double ScaleScale;    // used for quick lookups
  double ScaleOffset;   // used for quick lookups

  vtkIdType FlatIndex;

  bool UsingPoints;
  double BoundScale;

  // called by our Owner skips some stuff
  void GaussianRender(vtkRenderer* ren, vtkActor* act);

protected:
  vtkOpenGLPointGaussianMapperHelper();
  ~vtkOpenGLPointGaussianMapperHelper() override;

  // Description:
  // Create the basic shaders before replacement
  void GetShaderTemplate(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer*, vtkActor*) override;

  // Description:
  // Perform string replacements on the shader templates
  void ReplaceShaderColor(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer*, vtkActor*) override;
  void ReplaceShaderPositionVC(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer*, vtkActor*) override;

  // Description:
  // Set the shader parameters related to the Camera
  void SetCameraShaderParameters(vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* act) override;

  // Description:
  // Set the shader parameters related to the actor/mapper
  void SetMapperShaderParameters(vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* act) override;

  // Description:
  // Does the VBO/IBO need to be rebuilt
  bool GetNeedToRebuildBufferObjects(vtkRenderer* ren, vtkActor* act) override;

  // Description:
  // Update the VBO to contain point based values
  void BuildBufferObjects(vtkRenderer* ren, vtkActor* act) override;

  void RenderPieceDraw(vtkRenderer* ren, vtkActor* act) override;

  // Description:
  // Does the shader source need to be recomputed
  bool GetNeedToRebuildShaders(vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* act) override;

private:
  vtkOpenGLPointGaussianMapperHelper(const vtkOpenGLPointGaussianMapperHelper&) = delete;
  void operator=(const vtkOpenGLPointGaussianMapperHelper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
