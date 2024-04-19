// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenGLContextDevice3D.h"

#include "vtkBrush.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLContextDevice2D.h"
#include "vtkOpenGLContextDeviceBufferObjectBuilder.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLHelper.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkPen.h"
#include "vtkRenderTimerLog.h"
#include "vtkShaderProgram.h"
#include "vtkTransform.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLContextDevice3D::Private
{
public:
  Private() = default;

  ~Private() = default;

  void Transpose(double* in, double* transposed)
  {
    transposed[0] = in[0];
    transposed[1] = in[4];
    transposed[2] = in[8];
    transposed[3] = in[12];

    transposed[4] = in[1];
    transposed[5] = in[5];
    transposed[6] = in[9];
    transposed[7] = in[13];

    transposed[8] = in[2];
    transposed[9] = in[6];
    transposed[10] = in[10];
    transposed[11] = in[14];

    transposed[12] = in[3];
    transposed[13] = in[7];
    transposed[14] = in[11];
    transposed[15] = in[15];
  }

  void SetLineType(int type)
  {
    if (type == vtkPen::SOLID_LINE || type == vtkPen::NO_PEN)
    {
      return;
    }
    vtkGenericWarningMacro(<< "Line Stipples are no longer supported");
  }

  vtkVector2i Dim;
  vtkVector2i Offset;

  vtkOpenGLContextDeviceBufferObjectBuilder BufferObjectBuilder;
};

vtkStandardNewMacro(vtkOpenGLContextDevice3D);

vtkOpenGLContextDevice3D::vtkOpenGLContextDevice3D()
  : Storage(new Private)
{
  this->ModelMatrix = vtkTransform::New();
  this->ModelMatrix->Identity();
  this->VBO = new vtkOpenGLHelper;
  this->VCBO = new vtkOpenGLHelper;
  this->ClippingPlaneStates.resize(6, false);
  this->ClippingPlaneValues.resize(24);
}

vtkOpenGLContextDevice3D::~vtkOpenGLContextDevice3D()
{
  delete this->VBO;
  this->VBO = nullptr;
  delete this->VCBO;
  this->VCBO = nullptr;

  this->ModelMatrix->Delete();
  delete Storage;
}

void vtkOpenGLContextDevice3D::Initialize(vtkRenderer* ren, vtkOpenGLContextDevice2D* dev)
{
  this->Device2D = dev;
  this->Renderer = ren;
  this->RenderWindow = vtkOpenGLRenderWindow::SafeDownCast(ren->GetVTKWindow());
}
//------------------------------------------------------------------------------
void vtkOpenGLContextDevice3D::Begin(vtkViewport* vtkNotUsed(viewport))
{
  this->ModelMatrix->Identity();
  for (int i = 0; i < 6; i++)
  {
    this->ClippingPlaneStates[i] = false;
  }
}

void vtkOpenGLContextDevice3D::SetMatrices(vtkShaderProgram* prog)
{
  vtkOpenGLState* ostate = this->RenderWindow->GetState();
  ostate->vtkglDisable(GL_SCISSOR_TEST);
  prog->SetUniformMatrix("WCDCMatrix", this->Device2D->GetProjectionMatrix());

  vtkMatrix4x4* mvm = this->Device2D->GetModelMatrix();
  vtkMatrix4x4* tmp = vtkMatrix4x4::New();
  vtkMatrix4x4::Multiply4x4(mvm, this->ModelMatrix->GetMatrix(), tmp);

  prog->SetUniformMatrix("MCWCMatrix",
    //    this->ModelMatrix->GetMatrix());
    tmp);
  tmp->Delete();

  // add all the clipping planes
  int numClipPlanes = 0;
  float planeEquations[6][4];
  for (int i = 0; i < 6; i++)
  {
    if (this->ClippingPlaneStates[i])
    {
      planeEquations[numClipPlanes][0] = this->ClippingPlaneValues[i * 4];
      planeEquations[numClipPlanes][1] = this->ClippingPlaneValues[i * 4 + 1];
      planeEquations[numClipPlanes][2] = this->ClippingPlaneValues[i * 4 + 2];
      planeEquations[numClipPlanes][3] = this->ClippingPlaneValues[i * 4 + 3];
      numClipPlanes++;
    }
  }
  prog->SetUniformi("numClipPlanes", numClipPlanes);
  prog->SetUniform4fv("clipPlanes", 6, planeEquations);
}

void vtkOpenGLContextDevice3D::BuildVBO(vtkOpenGLHelper* cellBO, const float* f, int nv,
  const unsigned char* colors, int nc, float* tcoords)
{
  // build up temporary vtkDataArrays without copying the data.
  vtkNew<vtkFloatArray> positionsArray;
  vtkNew<vtkUnsignedCharArray> colorsArray;
  vtkNew<vtkFloatArray> tcoordsArray;

  positionsArray->SetNumberOfComponents(3);
  positionsArray->SetNumberOfTuples(nv);
  std::copy(f, f + nv * 3, positionsArray->Begin());

  colorsArray->SetNumberOfComponents(nc);
  colorsArray->SetNumberOfTuples(nv);
  std::copy(colors, colors + nv * nc, colorsArray->Begin());

  tcoordsArray->SetNumberOfComponents(2);
  tcoordsArray->SetArray(tcoords, nv * 2, 1); // do not take ownership of 'tcoords'

  // use 'anonymous' cache identifier because of raw typed array pointers.
  this->Storage->BufferObjectBuilder.BuildVBO(
    cellBO, positionsArray, colorsArray, tcoordsArray, /*cacheIdentifier=*/0, this->RenderWindow);
}

void vtkOpenGLContextDevice3D::ReadyVBOProgram()
{
  if (!this->VBO->Program)
  {
    this->VBO->Program = this->RenderWindow->GetShaderCache()->ReadyShaderProgram(
      // vertex shader
      "//VTK::System::Dec\n"
      "in vec3 vertexMC;\n"
      "uniform mat4 WCDCMatrix;\n"
      "uniform mat4 MCWCMatrix;\n"
      "uniform int numClipPlanes;\n"
      "uniform vec4 clipPlanes[6];\n"
      "out float clipDistances[6];\n"
      "void main() {\n"
      "vec4 vertex = vec4(vertexMC.xyz, 1.0);\n"
      "for (int planeNum = 0; planeNum < numClipPlanes; planeNum++)\n"
      "  {\n"
      "  clipDistances[planeNum] = dot(clipPlanes[planeNum], vertex*MCWCMatrix);\n"
      "  }\n"
      "gl_Position = vertex*MCWCMatrix*WCDCMatrix; }\n",
      // fragment shader
      "//VTK::System::Dec\n"
      "//VTK::Output::Dec\n"
      "uniform vec4 vertexColor;\n"
      "uniform int numClipPlanes;\n"
      "in float clipDistances[6];\n"
      "void main() { \n"
      "  for (int planeNum = 0; planeNum < numClipPlanes; planeNum++)\n"
      "    {\n"
      "    if (clipDistances[planeNum] < 0.0) discard;\n"
      "    }\n"
      "  gl_FragData[0] = vertexColor; }",
      // geometry shader
      "");
  }
  else
  {
    this->RenderWindow->GetShaderCache()->ReadyShaderProgram(this->VBO->Program);
  }
}

void vtkOpenGLContextDevice3D::ReadyVCBOProgram()
{
  if (!this->VCBO->Program)
  {
    this->VCBO->Program = this->RenderWindow->GetShaderCache()->ReadyShaderProgram(
      // vertex shader
      "//VTK::System::Dec\n"
      "in vec3 vertexMC;\n"
      "in vec4 vertexScalar;\n"
      "uniform mat4 WCDCMatrix;\n"
      "uniform mat4 MCWCMatrix;\n"
      "out vec4 vertexColor;\n"
      "uniform int hasOpacity;\n"
      "uniform int numClipPlanes;\n"
      "uniform vec4 clipPlanes[6];\n"
      "out float clipDistances[6];\n"
      "void main() {\n"
      "vec4 vertex = vec4(vertexMC.xyz, 1.0);\n"
      "vertexColor = vertexScalar;\n"
      "for (int planeNum = 0; planeNum < numClipPlanes; planeNum++)\n"
      "  {\n"
      "  clipDistances[planeNum] = dot(clipPlanes[planeNum], vertex*MCWCMatrix);\n"
      "  }\n"
      "gl_Position = vertex*MCWCMatrix*WCDCMatrix; \n"
      "if (hasOpacity == 0) { vertexColor.a = 1.0f; }\n"
      "}\n",
      // fragment shader
      "//VTK::System::Dec\n"
      "//VTK::Output::Dec\n"
      "in vec4 vertexColor;\n"
      "uniform int numClipPlanes;\n"
      "in float clipDistances[6];\n"
      "void main() { \n"
      "  for (int planeNum = 0; planeNum < numClipPlanes; planeNum++)\n"
      "    {\n"
      "    if (clipDistances[planeNum] < 0.0) discard;\n"
      "    }\n"
      "  gl_FragData[0] = vertexColor; }",
      // geometry shader
      "");
  }
  else
  {
    this->RenderWindow->GetShaderCache()->ReadyShaderProgram(this->VCBO->Program);
  }
}

bool vtkOpenGLContextDevice3D::HaveWideLines()
{
  if (this->Pen->GetWidth() > 1.0)
  {
    // we have wide lines, but the OpenGL implementation may
    // actually support them, check the range to see if we
    // really need have to implement our own wide lines
    return !(this->RenderWindow &&
      this->RenderWindow->GetMaximumHardwareLineWidth() >= this->Pen->GetWidth());
  }
  return false;
}

void vtkOpenGLContextDevice3D::DrawPoly(
  const float* verts, int n, const unsigned char* colors, int nc)
{
  assert("verts must be non-null" && verts != nullptr);
  assert("n must be greater than 0" && n > 0);

  if (this->Pen->GetLineType() == vtkPen::NO_PEN)
  {
    return;
  }

  vtkOpenGLClearErrorMacro();

  this->EnableDepthBuffer();

  this->Storage->SetLineType(this->Pen->GetLineType());

  vtkOpenGLHelper* cbo = nullptr;
  if (colors)
  {
    this->ReadyVCBOProgram();
    cbo = this->VCBO;
    if (!cbo->Program)
    {
      return;
    }
  }
  else
  {
    this->ReadyVBOProgram();
    cbo = this->VBO;
    if (!cbo->Program)
    {
      return;
    }
    if (this->HaveWideLines())
    {
      vtkWarningMacro(
        << "a line width has been requested that is larger than your system supports");
    }
    else
    {
      this->RenderWindow->GetState()->vtkglLineWidth(this->Pen->GetWidth());
    }
    cbo->Program->SetUniform4uc("vertexColor", this->Pen->GetColor());
  }

  this->BuildVBO(cbo, verts, n, colors, nc, nullptr);
  this->SetMatrices(cbo->Program);

  auto timer = this->RenderWindow->GetRenderTimer();
  VTK_SCOPED_RENDER_EVENT(this->GetClassNameInternal()
      << "::" << __func__ << "|glDrawArrays(cacheIdentifier: "
      << "null"
      << ",mode:GL_LINE_STRIP,n:" << n,
    timer);
  glDrawArrays(GL_LINE_STRIP, 0, n);

  // free everything
  cbo->ReleaseGraphicsResources(this->RenderWindow);
  this->RenderWindow->GetState()->vtkglLineWidth(1.0);

  this->DisableDepthBuffer();

  vtkOpenGLCheckErrorMacro("failed after DrawPoly");
}

//------------------------------------------------------------------------------
void vtkOpenGLContextDevice3D::DrawLines(
  const float* verts, int n, const unsigned char* colors, int nc)
{
  assert("verts must be non-null" && verts != nullptr);
  assert("n must be greater than 0" && n > 0);

  if (this->Pen->GetLineType() == vtkPen::NO_PEN)
  {
    return;
  }

  vtkOpenGLClearErrorMacro();

  this->EnableDepthBuffer();

  this->Storage->SetLineType(this->Pen->GetLineType());

  if (this->Pen->GetWidth() > 1.0)
  {
    vtkErrorMacro(<< "lines wider than 1.0 are not supported\n");
  }
  this->RenderWindow->GetState()->vtkglLineWidth(this->Pen->GetWidth());

  vtkOpenGLHelper* cbo = nullptr;
  if (colors)
  {
    this->ReadyVCBOProgram();
    cbo = this->VCBO;
    if (!cbo->Program)
    {
      return;
    }
  }
  else
  {
    this->ReadyVBOProgram();
    cbo = this->VBO;
    if (!cbo->Program)
    {
      return;
    }
    cbo->Program->SetUniform4uc("vertexColor", this->Pen->GetColor());
  }

  this->BuildVBO(cbo, verts, n, colors, nc, nullptr);
  this->SetMatrices(cbo->Program);

  auto timer = this->RenderWindow->GetRenderTimer();
  VTK_SCOPED_RENDER_EVENT(this->GetClassNameInternal()
      << "::" << __func__ << "|glDrawArrays(cacheIdentifier: "
      << "null"
      << ",mode:GL_LINES,n:" << n,
    timer);
  glDrawArrays(GL_LINES, 0, n);

  // free everything
  cbo->ReleaseGraphicsResources(this->RenderWindow);
  this->RenderWindow->GetState()->vtkglLineWidth(1.0);

  this->DisableDepthBuffer();

  vtkOpenGLCheckErrorMacro("failed after DrawLines");
}

void vtkOpenGLContextDevice3D::DrawPoints(
  const float* verts, int n, const unsigned char* colors, int nc)
{
  // build up temporary vtkDataArrays without copying the data.
  vtkNew<vtkFloatArray> positionsArray;
  vtkNew<vtkUnsignedCharArray> colorsArray;

  positionsArray->SetNumberOfComponents(3);
  positionsArray->SetNumberOfTuples(n);
  std::copy(verts, verts + n * 3, positionsArray->Begin());

  if (colors != nullptr)
  {
    vtkErrorMacro(<< "Here");
    colorsArray->SetNumberOfComponents(nc);
    colorsArray->SetNumberOfTuples(n);
    std::copy(colors, colors + n * nc, colorsArray->Begin());
  }

  this->DrawPoints(positionsArray, colorsArray, /*cacheIdentifier=*/0);
}

void vtkOpenGLContextDevice3D::DrawPoints(
  vtkDataArray* positions, vtkUnsignedCharArray* colors, std::uintptr_t cacheIdentifier)
{
  assert("positions must be non-null" && positions != nullptr);
  assert("number of positions must be greater than 0" && positions->GetNumberOfTuples() > 0);

  vtkOpenGLClearErrorMacro();

  this->EnableDepthBuffer();

  this->RenderWindow->GetState()->vtkglPointSize(this->Pen->GetWidth());

  vtkOpenGLHelper* cbo = nullptr;
  if (colors && colors->GetNumberOfTuples() > 0)
  {
    this->ReadyVCBOProgram();
    cbo = this->VCBO;
    if (!cbo->Program)
    {
      return;
    }
    const int hasOpacity = (colors->GetNumberOfComponents() == 4 ? 1 : 0);
    cbo->Program->SetUniform1iv("hasOpacity", 1, &hasOpacity);
  }
  else
  {
    this->ReadyVBOProgram();
    cbo = this->VBO;
    if (!cbo->Program)
    {
      return;
    }
    cbo->Program->SetUniform4uc("vertexColor", this->Pen->GetColor());
  }

  this->Storage->BufferObjectBuilder.BuildVBO(
    cbo, positions, colors, nullptr, cacheIdentifier, this->RenderWindow);
  this->SetMatrices(cbo->Program);

  auto timer = this->RenderWindow->GetRenderTimer();
  VTK_SCOPED_RENDER_EVENT(this->GetClassNameInternal()
      << "::" << __func__ << "|glDrawArrays(cacheIdentifier: " << cacheIdentifier
      << ",mode:GL_POINTS,n:" << positions->GetNumberOfTuples(),
    timer);
  glDrawArrays(GL_POINTS, 0, positions->GetNumberOfTuples());

  this->DisableDepthBuffer();

  vtkOpenGLCheckErrorMacro("failed DrawPoints");
}

void vtkOpenGLContextDevice3D::DrawTriangleMesh(
  const float* mesh, int n, const unsigned char* colors, int nc)
{
  // build up temporary vtkDataArrays without copying the data.
  vtkNew<vtkFloatArray> positionsArray;
  vtkNew<vtkUnsignedCharArray> colorsArray;

  positionsArray->SetNumberOfComponents(3);
  positionsArray->SetNumberOfTuples(n);
  std::copy(mesh, mesh + n * 3, positionsArray->Begin());

  if (colors != nullptr)
  {
    colorsArray->SetNumberOfComponents(nc);
    colorsArray->SetNumberOfTuples(n);
    std::copy(colors, colors + n * nc, colorsArray->Begin());
  }

  this->DrawTriangleMesh(positionsArray, colorsArray, /*cacheIdentifier=*/0);
}

void vtkOpenGLContextDevice3D::DrawTriangleMesh(
  vtkDataArray* positions, vtkUnsignedCharArray* colors, std::uintptr_t cacheIdentifier)
{
  assert("positions must be non-null" && positions != nullptr);
  assert("number of positions must be greater than 0" && positions->GetNumberOfTuples() > 0);

  vtkOpenGLClearErrorMacro();

  this->EnableDepthBuffer();

  vtkOpenGLHelper* cbo = nullptr;
  if (colors && colors->GetNumberOfTuples() > 0)
  {
    this->ReadyVCBOProgram();
    cbo = this->VCBO;
    if (!cbo->Program)
    {
      return;
    }
    const int hasOpacity = (colors->GetNumberOfComponents() == 4 ? 1 : 0);
    cbo->Program->SetUniform1iv("hasOpacity", 1, &hasOpacity);
  }
  else
  {
    this->ReadyVBOProgram();
    cbo = this->VBO;
    if (!cbo->Program)
    {
      return;
    }
    cbo->Program->SetUniform4uc("vertexColor", this->Pen->GetColor());
  }

  this->Storage->BufferObjectBuilder.BuildVBO(
    cbo, positions, colors, nullptr, cacheIdentifier, this->RenderWindow);
  this->SetMatrices(cbo->Program);

  auto timer = this->RenderWindow->GetRenderTimer();
  VTK_SCOPED_RENDER_EVENT(this->GetClassNameInternal()
      << "::" << __func__ << "|glDrawArrays(cacheIdentifier: " << cacheIdentifier
      << ",mode:GL_TRIANGLES,n:" << positions->GetNumberOfTuples(),
    timer);
  glDrawArrays(GL_TRIANGLES, 0, positions->GetNumberOfTuples());

  this->DisableDepthBuffer();

  vtkOpenGLCheckErrorMacro("failed after DrawTriangleMesh");
}

void vtkOpenGLContextDevice3D::ApplyPen(vtkPen* pen)
{
  this->Pen->DeepCopy(pen);
}

void vtkOpenGLContextDevice3D::ApplyBrush(vtkBrush* brush)
{
  this->Brush->DeepCopy(brush);
}

//------------------------------------------------------------------------------
void vtkOpenGLContextDevice3D::PushMatrix()
{
  this->ModelMatrix->Push();
}

//------------------------------------------------------------------------------
void vtkOpenGLContextDevice3D::PopMatrix()
{
  this->ModelMatrix->Pop();
}

void vtkOpenGLContextDevice3D::SetMatrix(vtkMatrix4x4* m)
{
  this->ModelMatrix->SetMatrix(m);
}

void vtkOpenGLContextDevice3D::GetMatrix(vtkMatrix4x4* m)
{
  m->DeepCopy(this->ModelMatrix->GetMatrix());
}

void vtkOpenGLContextDevice3D::MultiplyMatrix(vtkMatrix4x4* m)
{
  this->ModelMatrix->Concatenate(m);
}

void vtkOpenGLContextDevice3D::SetClipping(const vtkRecti& rect)
{
  // Check the bounds, and clamp if necessary
  GLint vp[4] = { this->Storage->Offset.GetX(), this->Storage->Offset.GetY(),
    this->Storage->Dim.GetX(), this->Storage->Dim.GetY() };

  if (rect.GetX() > 0 && rect.GetX() < vp[2])
  {
    vp[0] += rect.GetX();
  }
  if (rect.GetY() > 0 && rect.GetY() < vp[3])
  {
    vp[1] += rect.GetY();
  }
  if (rect.GetWidth() > 0 && rect.GetWidth() < vp[2])
  {
    vp[2] = rect.GetWidth();
  }
  if (rect.GetHeight() > 0 && rect.GetHeight() < vp[3])
  {
    vp[3] = rect.GetHeight();
  }

  vtkOpenGLState* ostate = this->RenderWindow->GetState();
  ostate->vtkglScissor(vp[0], vp[1], vp[2], vp[3]);
}

void vtkOpenGLContextDevice3D::EnableClipping(bool enable)
{
  vtkOpenGLState* ostate = this->RenderWindow->GetState();
  ostate->SetEnumState(GL_SCISSOR_TEST, enable);
}

void vtkOpenGLContextDevice3D::EnableClippingPlane(int i, double* planeEquation)
{
  if (i >= 6)
  {
    vtkOpenGLCheckErrorMacro("only 6 ClippingPlane allowed");
    return;
  }
  this->ClippingPlaneStates[i] = true;
  this->ClippingPlaneValues[i * 4] = planeEquation[0];
  this->ClippingPlaneValues[i * 4 + 1] = planeEquation[1];
  this->ClippingPlaneValues[i * 4 + 2] = planeEquation[2];
  this->ClippingPlaneValues[i * 4 + 3] = planeEquation[3];
}

void vtkOpenGLContextDevice3D::DisableClippingPlane(int i)
{
  if (i >= 6)
  {
    vtkOpenGLCheckErrorMacro("only 6 ClippingPlane allowed");
    return;
  }
  this->ClippingPlaneStates[i] = false;
}

void vtkOpenGLContextDevice3D::EnableDepthBuffer()
{
  vtkOpenGLState* ostate = this->RenderWindow->GetState();
  ostate->vtkglEnable(GL_DEPTH_TEST);
}

void vtkOpenGLContextDevice3D::DisableDepthBuffer()
{
  vtkOpenGLState* ostate = this->RenderWindow->GetState();
  ostate->vtkglDisable(GL_DEPTH_TEST);
}

void vtkOpenGLContextDevice3D::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkOpenGLContextDevice3D::ReleaseCache(std::uintptr_t cacheIdentifier)
{
  this->Storage->BufferObjectBuilder.Erase(cacheIdentifier, this->RenderWindow);
}
VTK_ABI_NAMESPACE_END
