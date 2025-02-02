// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCallbackCommand.h"
#include "vtkGLSLModCamera.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkShaderProgram.h"
#include "vtkSmartPointer.h"

#include "vtkOpenGLError.h"

#include <cstdlib>

namespace
{
const char* vss = R"(//VTK::System::Dec

void main()
{
  int pointIds[6] = int[](0, 1, 2, 0, 2, 3);
  vec4 points[4];
  points[0] = vec4(-0.75, -0.75, 0, 1);
  points[1] = vec4(0.75, -0.75, 0, 1);
  points[2] = vec4(0.75, 0.75, 0, 1);
  points[3] = vec4(-0.75, 0.75, 0, 1);

  int pointId = pointIds[gl_VertexID % 6];
  gl_Position = points[pointId];
}
)";
const char* tcss = R"(//VTK::System::Dec

layout(vertices = 3) out;

void main()
{
  if (gl_InvocationID == 0)
  {
    gl_TessLevelOuter[0] = 4.0;
    gl_TessLevelOuter[1] = 4.0;
    gl_TessLevelOuter[2] = 4.0;
    gl_TessLevelInner[0] = 8.0;
  }
  gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
)";

const char* tess = R"(//VTK::System::Dec

layout(triangles, equal_spacing, ccw) in;

out vec3 positionES;
out vec3 patchDistanceES;

vec4 interpolate(vec4 v0, vec4 v1, vec4 v2)
{
  return vec4(gl_TessCoord.x) * v0 + vec4(gl_TessCoord.y) * v1 + vec4(gl_TessCoord.z) * v2;
}

void main()
{
  vec3 position = interpolate(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position).xyz;
  gl_Position = vec4(position, 1.0);
  positionES = position;
  patchDistanceES = gl_TessCoord;
}
)";

const char* gss = R"(//VTK::System::Dec

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 positionES[3];
in vec3 patchDistanceES[3];

// Camera prop
//VTK::Camera::Dec

out vec3 faceNormalGS;
out vec3 patchDistanceGS;
out vec3 triDistanceGS;

void main()
{
  vec3 A = positionES[2] - positionES[0];
  vec3 B = positionES[1] - positionES[0];
  faceNormalGS = normalMatrix * normalize(cross(A, B));

  patchDistanceGS = patchDistanceES[0];
  triDistanceGS = vec3(1, 0, 0);
  gl_Position = MCDCMatrix * gl_in[0].gl_Position;
  EmitVertex();

  patchDistanceGS = patchDistanceES[1];
  triDistanceGS = vec3(0, 1, 0);
  gl_Position = MCDCMatrix * gl_in[1].gl_Position;
  EmitVertex();

  patchDistanceGS = patchDistanceES[2];
  triDistanceGS = vec3(0, 0, 1);
  gl_Position = MCDCMatrix * gl_in[2].gl_Position;
  EmitVertex();

  EndPrimitive();
}

)";

const char* fss = R"(//VTK::System::Dec

in vec3 faceNormalGS;
in vec3 triDistanceGS;
in vec3 patchDistanceGS;

//VTK::Output::Dec

uniform float LightIntensity;
uniform vec3 LightPosition;
uniform vec3 DiffuseMaterial;
uniform vec3 AmbientMaterial;

float amplify(float d, float scale, float offset)
{
    d = scale * d + offset;
    d = clamp(d, 0, 1);
    d = 1 - exp2(-2*d*d);
    return d;
}

void main()
{
  vec3 N = normalize(faceNormalGS);
  vec3 L = LightPosition;
  float df = abs(dot(N, L)) * LightIntensity;
  vec3 color = AmbientMaterial + df * DiffuseMaterial;

  float d1 = min(min(triDistanceGS.x, triDistanceGS.y), triDistanceGS.z);
  float d2 = min(min(patchDistanceGS.x, patchDistanceGS.y), patchDistanceGS.z);
  color = amplify(d1, 40, -0.5) * amplify(d2, 60, -0.5) * color;

  gl_FragData[0] = vec4(color, 1.0);
}
)";

const float ambient[3] = { 0.04, 0.04, 0.04 };
const float diffuse[3] = { 1.0, 0.388, 0.27 };
const float lightPosition[3] = { 0.25, 0.25, 1. };
const float lightIntensity = 1.5;

class TesselationDrawCommand : public vtkCallbackCommand
{
public:
  static TesselationDrawCommand* New() { return new TesselationDrawCommand; }
  vtkTypeMacro(TesselationDrawCommand, vtkCallbackCommand);

  void Execute(vtkObject* caller, unsigned long eventId, void*) override
  {
    if (eventId != vtkCommand::EndEvent)
    {
      return;
    }
    auto* renderer = vtkOpenGLRenderer::SafeDownCast(vtkRenderer::SafeDownCast(caller));
    if (!renderer)
    {
      return;
    }
    auto* renWin = renderer->GetRenderWindow();
    auto* oglRenWin = vtkOpenGLRenderWindow::SafeDownCast(renWin);
    if (!oglRenWin)
    {
      std::cout << "ERROR: render window is not a vtkOpenGLRenderWindow!\n";
      return;
    }
    vtkOpenGLCheckErrors("Before binding vao");
    this->VAO->Bind();

    vtkOpenGLCheckErrors("Before binding shader");
    auto* shaderCache = oglRenWin->GetShaderCache();
    const bool lastSyncGLSLVersion = shaderCache->GetSyncGLSLShaderVersion();
    shaderCache->SyncGLSLShaderVersionOn();
    if (this->Program == nullptr)
    {
      std::string vss_init = vss;
      std::string gss_init = gss;
      std::string fss_init = fss;
      std::string tcss_init = tcss;
      std::string tess_init = tess;
      this->CameraMod->ReplaceShaderValues(
        renderer, vss_init, gss_init, fss_init, tcss_init, tess_init, nullptr, nullptr);
      this->Program = vtk::MakeSmartPointer(shaderCache->ReadyShaderProgram(vss_init.c_str(),
        fss_init.c_str(), gss_init.c_str(), tcss_init.c_str(), tess_init.c_str()));
    }
    else
    {
      shaderCache->ReadyShaderProgram(this->Program);
    }
    shaderCache->SetSyncGLSLShaderVersion(lastSyncGLSLVersion);
    if (this->Program == nullptr || !this->Program->isBound())
    {
      std::cout << "ERROR: shader program is not bound!\n";
      return;
    }

    vtkOpenGLCheckErrors("Before glPatchParameteri");
#ifdef GL_PATCH_VERTICES
    glPatchParameteri(GL_PATCH_VERTICES, 3);
#endif

    this->Program->SetUniform3f("AmbientMaterial", ambient);
    this->Program->SetUniform3f("DiffuseMaterial", diffuse);
    this->Program->SetUniform3f("LightPosition", lightPosition);
    this->Program->SetUniformf("LightIntensity", lightIntensity);
    renderer->ResetCameraClippingRange(-1, 1, -1, 1, -1, 1);
    this->CameraMod->SetShaderParameters(renderer, this->Program, nullptr, this->PlaceholderActor);
    vtkOpenGLCheckErrors("Before draw");
#ifdef GL_PATCHES
    glDrawArrays(GL_PATCHES, 0, 6);
#endif
    vtkOpenGLCheckErrors("After draw");
  }

protected:
  TesselationDrawCommand() = default;
  ~TesselationDrawCommand() override = default;

  vtkNew<vtkOpenGLVertexArrayObject> VAO;
  vtkNew<vtkGLSLModCamera> CameraMod;
  vtkNew<vtkActor> PlaceholderActor;
  vtkShaderProgram* Program = nullptr;

private:
  TesselationDrawCommand(const TesselationDrawCommand&) = delete;
  void operator=(const TesselationDrawCommand&) = delete;
};
}

int TestTessellationShader(int argc, char* argv[])
{
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderWindowInteractor> iren;
  renWin->SetInteractor(iren);
  iren->Initialize();
  if (!vtkShader::IsTessellationShaderSupported())
  {
    std::cerr << "Tessellation shaders are not supported on this system, skipping the test.\n";
    return EXIT_SUCCESS;
  }

  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer);
  renderer->SetBackground(0.75, 0.75, 0.75);
  renderer->ResetCameraScreenSpace(-0.75, 0.75, -0.75, 0.75, -1, 1, /*offsetRatio=*/1.0);

  vtkNew<TesselationDrawCommand> cmd;
  renderer->AddObserver(vtkCommand::EndEvent, cmd);

  renWin->SetSize(400, 400);
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return retVal ? EXIT_SUCCESS : EXIT_FAILURE;
}
