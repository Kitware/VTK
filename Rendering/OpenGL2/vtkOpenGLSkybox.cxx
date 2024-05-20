// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenGLSkybox.h"

#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLShaderProperty.h"
#include "vtkOpenGLState.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkShaderProgram.h"
#include "vtkTexture.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenGLSkybox);

vtkOpenGLSkybox::vtkOpenGLSkybox()
{
  vtkNew<vtkPolyData> poly;
  vtkNew<vtkPoints> pts;
  pts->SetNumberOfPoints(4);
  pts->SetPoint(0, -1, -1, 0);
  pts->SetPoint(1, 1, -1, 0);
  pts->SetPoint(2, 1, 1, 0);
  pts->SetPoint(3, -1, 1, 0);
  poly->SetPoints(pts);
  vtkNew<vtkCellArray> polys;
  poly->SetPolys(polys);
  polys->InsertNextCell(4);
  polys->InsertCellPoint(0);
  polys->InsertCellPoint(1);
  polys->InsertCellPoint(2);
  polys->InsertCellPoint(3);

  // this->CubeMapper->SetInputConnection(this->Cube->GetOutputPort(0));
  this->CubeMapper->SetInputData(poly);
  this->SetMapper(this->CubeMapper);
  this->OpenGLActor->SetMapper(this->CubeMapper);

  vtkOpenGLShaderProperty* sp =
    vtkOpenGLShaderProperty::SafeDownCast(this->OpenGLActor->GetShaderProperty());
  sp->AddShaderReplacement(vtkShader::Vertex,
    "//VTK::PositionVC::Dec",  // replace
    true,                      // before the standard replacements
    "//VTK::PositionVC::Dec\n" // we still want the default
    "out vec3 TexCoords;\n",
    false // only do it once
  );
  sp->AddShaderReplacement(vtkShader::Vertex,
    "//VTK::PositionVC::Impl", // replace
    true,                      // before the standard replacements
    "  gl_Position = vec4(vertexMC.xy, 1.0, 1.0);\n"
    "  vec4 tmpc = inverse(MCDCMatrix) * gl_Position;\n"
    "  TexCoords = tmpc.xyz/tmpc.w;\n",
    false // only do it once
  );

  this->LastProjection = -1;
  this->LastGammaCorrect = false;

  this->GetProperty()->SetDiffuse(0.0);
  this->GetProperty()->SetAmbient(1.0);
  this->GetProperty()->SetSpecular(0.0);
  this->OpenGLActor->SetProperty(this->GetProperty());
  this->CurrentRenderer = nullptr;
}

vtkOpenGLSkybox::~vtkOpenGLSkybox() = default;

void vtkOpenGLSkybox::SetMapper(vtkMapper* mapper)
{
  this->Superclass::SetMapper(mapper);
  mapper->AddObserver(vtkCommand::UpdateShaderEvent, this, &vtkOpenGLSkybox::UpdateUniforms);
}

void vtkOpenGLSkybox::UpdateUniforms(vtkObject*, unsigned long, void* calldata)
{
  vtkShaderProgram* program = reinterpret_cast<vtkShaderProgram*>(calldata);
  program->SetUniform3f("cameraPos", this->LastCameraPosition);
  float plane[4];
  double norm = vtkMath::Norm(this->FloorPlane, 3);
  plane[0] = this->FloorPlane[0] / norm;
  plane[1] = this->FloorPlane[1] / norm;
  plane[2] = this->FloorPlane[2] / norm;
  plane[3] = this->FloorPlane[3] / norm;
  program->SetUniform4f("floorPlane", plane);
  program->SetUniform3f("floorRight", this->FloorRight);
  program->SetUniform2f("floorTCoordScale", this->FloorTexCoordScale);
  float front[3];
  vtkMath::Cross(plane, this->FloorRight, front);
  program->SetUniform3f("floorFront", front);
  program->SetUniformf(
    "leftEye", (this->CurrentRenderer->GetActiveCamera()->GetLeftEye() ? 1.0 : 0.0));
}

// Actual Skybox render method.
void vtkOpenGLSkybox::Render(vtkRenderer* ren, vtkMapper* mapper)
{
  vtkOpenGLClearErrorMacro();

  if (this->LastProjection != this->Projection || this->LastGammaCorrect != this->GammaCorrect)
  {
    vtkOpenGLShaderProperty* sp =
      vtkOpenGLShaderProperty::SafeDownCast(this->OpenGLActor->GetShaderProperty());

    std::string str = "//VTK::System::Dec\n" // always start with this line
                      "//VTK::Output::Dec\n" // always have this line in your FS
                      "in vec3 TexCoords;\n"
                      "uniform vec3 cameraPos;\n" // wc camera position;
                      "//VTK::Projection::Dec\n"
                      "void main () {\n"
                      "//VTK::Projection::Impl\n"
                      "}\n";

    if (this->Projection == vtkSkybox::Cube)
    {
      vtkShaderProgram::Substitute(str, "//VTK::Projection::Dec",
        "uniform samplerCube actortexture;\n"
        "uniform vec4 floorPlane;\n" // floor plane eqn
        "uniform vec3 floorRight;\n" // floor plane right
        "uniform vec3 floorFront;\n" // floor plane front
      );

      vtkShaderProgram::Substitute(str, "//VTK::Projection::Impl",
        "  vec3 diri = normalize(TexCoords - cameraPos);\n"
        "  vec3 dirv = vec3(dot(diri,floorRight),\n"
        "    dot(diri,floorPlane.xyz),\n"
        "    dot(diri,floorFront));\n"
        "  vec4 color = textureLod(actortexture, dirv, 0.0);\n"
        "//VTK::Gamma::Impl\n");
    }
    if (this->Projection == vtkSkybox::Sphere)
    {
      vtkShaderProgram::Substitute(str, "//VTK::Projection::Dec",
        "uniform sampler2D actortexture;\n"
        "uniform vec4 floorPlane;\n" // floor plane eqn
        "uniform vec3 floorRight;\n" // floor plane right
        "uniform vec3 floorFront;\n" // floor plane front
      );

      vtkShaderProgram::Substitute(str, "//VTK::Projection::Impl",
        "  vec3 diri = normalize(TexCoords - cameraPos);\n"
        "  vec3 dirv = vec3(dot(diri,floorRight),\n"
        "    dot(diri,floorPlane.xyz),\n"
        "    dot(diri,floorFront));\n"
        "  float phix = length(vec2(dirv.x, dirv.z));\n"
        "  vec4 color = textureLod(actortexture, vec2(0.5*atan(dirv.x, "
        "dirv.z)/3.1415927 + 0.5, atan(dirv.y,phix)/3.1415927 + 0.5), 0.0);\n"
        "//VTK::Gamma::Impl\n");
    }
    if (this->Projection == vtkSkybox::StereoSphere)
    {
      vtkShaderProgram::Substitute(str, "//VTK::Projection::Dec",
        "uniform sampler2D actortexture;\n"
        "uniform vec4 floorPlane;\n" // floor plane eqn
        "uniform vec3 floorRight;\n" // floor plane right
        "uniform vec3 floorFront;\n" // floor plane front
        "uniform float leftEye;\n"   // 1.0 for left, 0.0 for right
      );

      vtkShaderProgram::Substitute(str, "//VTK::Projection::Impl",
        "  vec3 diri = normalize(TexCoords - cameraPos);\n"
        "  vec3 dirv = vec3(dot(diri,floorRight),\n"
        "    dot(diri,floorPlane.xyz),\n"
        "    dot(diri,floorFront));\n"
        "  float phix = length(vec2(dirv.x, dirv.z));\n"
        "  vec4 color = textureLod(actortexture, vec2(0.5*atan(dirv.x, dirv.z)/3.1415927 + "
        "0.5, 0.5*atan(dirv.y,phix)/3.1415927 + 0.25 + 0.5*leftEye), 0.0);\n"
        "//VTK::Gamma::Impl\n");
    }
    if (this->Projection == vtkSkybox::Floor)
    {
      vtkShaderProgram::Substitute(str, "//VTK::Projection::Dec",
        "uniform vec4 floorPlane;\n"       // floor plane eqn
        "uniform vec3 floorRight;\n"       // floor plane right
        "uniform vec3 floorFront;\n"       // floor plane front
        "uniform vec2 floorTCoordScale;\n" // floor texture scale
        "uniform mat4 MCDCMatrix;\n"
        "uniform sampler2D actortexture;\n");

      vtkShaderProgram::Substitute(str, "//VTK::Projection::Impl",
        "  vec3 dirv = normalize(TexCoords - cameraPos);\n"
        "  float den = dot(floorPlane.xyz, dirv);\n"
        "  if (abs(den) < 0.0001 ) { discard; } else {\n"
        "    vec3 p0 = -1.0*floorPlane.w*floorPlane.xyz;\n"
        "    vec3 p0l0 = p0 - cameraPos;\n"
        "    float t = dot(p0l0, floorPlane.xyz) / den;\n"
        "    if (t >= 0.0) {\n"
        "      vec3 pos = dirv*t - p0l0;\n"
        "      vec4 color = texture(actortexture, "
        "vec2(dot(floorRight,pos)/floorTCoordScale.x, dot(floorFront, pos)/floorTCoordScale.y));\n"
        "      //VTK::Gamma::Impl\n"
        // The discards cause a discontinuity with mipmapping
        // on the horizon of the floor. So we fade out the floor
        // along the horizon. Specifically starting at when the
        // dot product equals .02 which is at 88.85 degrees and
        // going to zero at 90 degrees.
        "      gl_FragData[0].a *= (50.0*min(0.02, abs(den)));\n"
        "      vec4 tpos = MCDCMatrix*vec4(pos.xyz + 0.01 * p0l0,1.0);\n"
        "      gl_FragDepth = clamp(0.5 + 0.5*tpos.z/tpos.w,0.0,1.0);\n"
        "    } else { discard; }\n"
        "  }\n");
    }

    if (this->GammaCorrect)
    {
      vtkShaderProgram::Substitute(str, "//VTK::Gamma::Impl",
        "gl_FragData[0] = vec4(pow(color.rgb, vec3(1.0 / 2.2)), color.a);\n");
    }
    else
    {
      vtkShaderProgram::Substitute(str, "//VTK::Gamma::Impl", "gl_FragData[0] = color;\n");
    }

    sp->SetFragmentShaderCode(str.c_str());

    this->CubeMapper->Modified();
    mapper->Modified();
    this->LastProjection = this->Projection;
    this->LastGammaCorrect = this->GammaCorrect;
  }

  double* pos = ren->GetActiveCamera()->GetPosition();
  this->LastCameraPosition[0] = pos[0];
  this->LastCameraPosition[1] = pos[1];
  this->LastCameraPosition[2] = pos[2];

  this->CurrentRenderer = ren;

  // get opacity
  static_cast<vtkOpenGLRenderer*>(ren)->GetState()->vtkglDepthMask(GL_TRUE);
  static_cast<vtkOpenGLRenderer*>(ren)->GetState()->vtkglDepthFunc(GL_LEQUAL);

  // send a render to the mapper; update pipeline
  this->Texture->Render(ren);
  this->OpenGLActor->SetTexture(this->GetTexture());
  mapper->Render(ren, this->OpenGLActor);
  this->Texture->PostRender(ren);

  vtkOpenGLCheckErrorMacro("failed after Render");
}

//------------------------------------------------------------------------------
void vtkOpenGLSkybox::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
