// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenGLBufferObject.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"
#include "vtkUnsignedCharArray.h"

#include <iostream>
#include <string>

// This test emulates a texture buffer. It verifies that the provided
// data has been uploaded as a 2D texture when emulating texture buffers.
int TestTextureBufferEmulation(int /*argc*/, char* /*argv*/[])
{
  bool success = false;
  const int width = 12;
  const int height = 5;

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(width, height);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  iren->Initialize();

  std::vector<unsigned char> values(width * height * 4, 0); // w x h x [rgba]

  auto oglRenWin = vtkOpenGLRenderWindow::SafeDownCast(renWin);
  vtkNew<vtkOpenGLBufferObject> bo;
  bo->SetType(vtkOpenGLBufferObject::ArrayBuffer);
  bo->Upload(values, bo->GetType());
  // should get uploaded as a 2D texture.
  vtkNew<vtkTextureObject> aTexture;
  aTexture->SetRequireTextureInteger(true);
  aTexture->SetContext(oglRenWin);
  success = aTexture->EmulateTextureBufferWith2DTextures(width * height, 4, VTK_UNSIGNED_CHAR, bo);

  using GLUtil = vtkOpenGLRenderUtilities;
  std::string fs = GLUtil::GetFullScreenQuadFragmentShaderTemplate();
  // write code that indexes into a 2D texture.
  vtkShaderProgram::Substitute(fs, "//VTK::FSQ::Decl", "uniform usampler2D aTexture;");
  vtkShaderProgram::Substitute(fs, "//VTK::FSQ::Impl",
    "vec2 pixelCoord = vec2(gl_FragCoord.x - 0.5, gl_FragCoord.y - 0.5);\n"
    "int i = int(pixelCoord.x);\n"
    "int j = int(pixelCoord.y);\n"
    "int idx = i + j * " +
      std::to_string(width) +
      ";\n"
      "gl_FragData[0] = texelFetch(aTexture, ivec2(idx, 0), 0) / 255.0f;\n"
      // gotta use texCoord, so that program is linked
      "gl_FragDepth = texCoord.x;\n");
  vtkShaderProgram* program = oglRenWin->GetShaderCache()->ReadyShaderProgram(
    GLUtil::GetFullScreenQuadVertexShader().c_str(), fs.c_str(), "");

  vtkNew<vtkOpenGLVertexArrayObject> vao;
  GLUtil::PrepFullScreenVAO(oglRenWin, vao.Get(), program);

  renWin->Start();

  aTexture->Activate();
  program->SetUniformi("aTexture", aTexture->GetTextureUnit());
  vao->Bind();

  auto gl = oglRenWin->GetState();
  gl->vtkglDisable(GL_SCISSOR_TEST);
  gl->vtkglDisable(GL_DEPTH_TEST);
  gl->vtkglDisable(GL_BLEND);
  gl->vtkglViewport(0, 0, width, height);
  GLUtil::DrawFullScreenQuad();

  vao->Release();
  aTexture->Deactivate();

  renWin->End();
  renWin->Frame();

  vtkNew<vtkUnsignedCharArray> output;
  renWin->GetRGBACharPixelData(0, 0, width - 1, height - 1, /*front=*/1, output);
  for (vtkIdType i = 0; i < output->GetNumberOfValues(); ++i)
  {
    success &= (values[i] == output->GetValue(i));
  }
  if (!success)
  {
    // if something didn't go right, print all values.
    for (vtkIdType i = 0; i < output->GetNumberOfValues(); ++i)
    {
      std::cout << int(output->GetValue(i)) << ' ';
    }
    std::cout << std::endl;
  }

  return success ? 0 : 1;
}
