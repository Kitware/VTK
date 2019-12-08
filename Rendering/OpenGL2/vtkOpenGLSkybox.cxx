/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLSkybox.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include <cmath>

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

  this->CubeMapper->AddObserver(
    vtkCommand::UpdateShaderEvent, this, &vtkOpenGLSkybox::UpdateUniforms);

  this->LastProjection = -1;

  this->GetProperty()->SetDiffuse(0.0);
  this->GetProperty()->SetAmbient(1.0);
  this->GetProperty()->SetSpecular(0.0);
  this->OpenGLActor->SetProperty(this->GetProperty());
  this->CurrentRenderer = nullptr;
}

vtkOpenGLSkybox::~vtkOpenGLSkybox() = default;

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

  if (this->LastProjection != this->Projection)
  {
    vtkOpenGLShaderProperty* sp =
      vtkOpenGLShaderProperty::SafeDownCast(this->OpenGLActor->GetShaderProperty());
    if (this->Projection == vtkSkybox::Cube)
    {
      // Replace VTK fragment shader
      sp->SetFragmentShaderCode(
        "//VTK::System::Dec\n" // always start with this line
        "//VTK::Output::Dec\n" // always have this line in your FS
        "in vec3 TexCoords;\n"
        "uniform vec3 cameraPos;\n" // wc camera position
        "uniform samplerCube actortexture;\n"
        "void main () {\n"
        "  gl_FragData[0] = texture(actortexture, normalize(TexCoords - cameraPos));\n"
        "}\n");
    }
    if (this->Projection == vtkSkybox::Sphere)
    {
      // Replace VTK fragment shader
      sp->SetFragmentShaderCode("//VTK::System::Dec\n" // always start with this line
                                "//VTK::Output::Dec\n" // always have this line in your FS
                                "in vec3 TexCoords;\n"
                                "uniform vec3 cameraPos;\n" // wc camera position
                                "uniform sampler2D actortexture;\n"
                                "uniform vec4 floorPlane;\n" // floor plane eqn
                                "uniform vec3 floorRight;\n" // floor plane right
                                "uniform vec3 floorFront;\n" // floor plane front
                                "void main () {\n"
                                "  vec3 diri = normalize(TexCoords - cameraPos);\n"
                                "  vec3 dirv = vec3(dot(diri,floorRight),\n"
                                "    dot(diri,floorPlane.xyz),\n"
                                "    dot(diri,floorFront));\n"
                                "  float phix = length(vec2(dirv.x, dirv.z));\n"
                                "  gl_FragData[0] = texture(actortexture, vec2(0.5*atan(dirv.x, "
                                "dirv.z)/3.1415927 + 0.5, atan(dirv.y,phix)/3.1415927 + 0.5));\n"
                                "}\n");
    }
    if (this->Projection == vtkSkybox::StereoSphere)
    {
      // Replace VTK fragment shader
      sp->SetFragmentShaderCode(
        "//VTK::System::Dec\n" // always start with this line
        "//VTK::Output::Dec\n" // always have this line in your FS
        "in vec3 TexCoords;\n"
        "uniform vec3 cameraPos;\n" // wc camera position
        "uniform sampler2D actortexture;\n"
        "uniform vec4 floorPlane;\n" // floor plane eqn
        "uniform vec3 floorRight;\n" // floor plane right
        "uniform vec3 floorFront;\n" // floor plane front
        "uniform float leftEye;\n"   // 1.0 for left, 0.0 for right
        "void main () {\n"
        "  vec3 diri = normalize(TexCoords - cameraPos);\n"
        "  vec3 dirv = vec3(dot(diri,floorRight),\n"
        "    dot(diri,floorPlane.xyz),\n"
        "    dot(diri,floorFront));\n"
        "  float phix = length(vec2(dirv.x, dirv.z));\n"
        "  gl_FragData[0] = texture(actortexture, vec2(0.5*atan(dirv.x, dirv.z)/3.1415927 + 0.5, "
        "0.5*atan(dirv.y,phix)/3.1415927 + 0.25 + 0.5*leftEye));\n"
        "}\n");
    }
    if (this->Projection == vtkSkybox::Floor)
    {
      // Replace VTK fragment shader
      sp->SetFragmentShaderCode("//VTK::System::Dec\n" // always start with this line
                                "//VTK::Output::Dec\n" // always have this line in your FS
                                "in vec3 TexCoords;\n"
                                "uniform vec3 cameraPos;\n"  // wc camera position
                                "uniform vec4 floorPlane;\n" // floor plane eqn
                                "uniform vec3 floorRight;\n" // floor plane right
                                "uniform vec3 floorFront;\n" // floor plane front
                                "uniform mat4 MCDCMatrix;\n"
                                "uniform sampler2D actortexture;\n"
                                "void main () {\n"
                                "  vec3 dirv = normalize(TexCoords - cameraPos);\n"
                                "  float den = dot(floorPlane.xyz, dirv);\n"
                                "  if (abs(den) < 0.0001 ) { discard; } else {\n"
                                "    vec3 p0 = -1.0*floorPlane.w*floorPlane.xyz;\n"
                                "    vec3 p0l0 = p0 - cameraPos;\n"
                                "    float t = dot(p0l0, floorPlane.xyz) / den;\n"
                                "    if (t >= 0.0) {\n"
                                "      vec3 pos = dirv*t - p0l0;\n"
                                "      gl_FragData[0] = texture(actortexture, "
                                "vec2(dot(floorRight,pos), dot(floorFront, pos)));\n"
                                // The discards cause a discontinuity with mipmapping
                                // on the horizon of the floor. So we fade out the floor
                                // along the horizon. Specifically starting at when the
                                // dot product equals .02 which is at 88.85 degrees and
                                // going to zero at 90 degrees.
                                "      gl_FragData[0].a *= (50.0*min(0.02, abs(den)));\n"
                                "      vec4 tpos = MCDCMatrix*vec4(pos.xyz,1.0);\n"
                                "      gl_FragDepth = clamp(0.5 + 0.5*tpos.z/tpos.w,0.0,1.0);\n"
                                "    } else { discard; }\n"
                                "  }\n"
                                "}\n");
    }
    this->CubeMapper->Modified();
    this->LastProjection = this->Projection;
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

//----------------------------------------------------------------------------
void vtkOpenGLSkybox::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
