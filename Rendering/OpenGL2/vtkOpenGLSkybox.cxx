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
#include "vtkObjectFactory.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkOpenGLError.h"
#include "vtkRenderWindow.h"

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

  this->CubeMapper->AddShaderReplacement(
    vtkShader::Vertex,
    "//VTK::PositionVC::Dec", // replace
    true, // before the standard replacements
    "//VTK::PositionVC::Dec\n" // we still want the default
    "varying vec3 TexCoords;\n",
    false // only do it once
    );
  this->CubeMapper->AddShaderReplacement(
    vtkShader::Vertex,
    "//VTK::PositionVC::Impl", // replace
    true, // before the standard replacements
    "  gl_Position = vec4(vertexMC.xy, 1.0, 1.0);\n"
    "  TexCoords.xyz = normalize(inverse(MCDCMatrix) * gl_Position).xyz;\n",
    false // only do it once
    );

  this->LastProjection = -1;

  this->GetProperty()->SetDiffuse(0.0);
  this->GetProperty()->SetAmbient(1.0);
  this->GetProperty()->SetSpecular(0.0);
  this->OpenGLActor->SetProperty(this->GetProperty());
}

vtkOpenGLSkybox::~vtkOpenGLSkybox()
{
}


// Actual Skybox render method.
void vtkOpenGLSkybox::Render(vtkRenderer *ren, vtkMapper *mapper)
{
  vtkOpenGLClearErrorMacro();

  if (this->LastProjection != this->Projection)
  {
    if (this->Projection == vtkSkybox::Cube)
    {
      // Replace VTK fragment shader
      this->CubeMapper->SetFragmentShaderCode(
        "//VTK::System::Dec\n"  // always start with this line
        "//VTK::Output::Dec\n"  // always have this line in your FS
        "varying vec3 TexCoords;\n"
        "uniform samplerCube texture_0;\n" // texture_0 is the first texture
        "void main () {\n"
        "  gl_FragData[0] = texture(texture_0, TexCoords);\n"
        "}\n"
        );
    }
    if (this->Projection == vtkSkybox::Sphere)
    {
      // Replace VTK fragment shader
      this->CubeMapper->SetFragmentShaderCode(
        "//VTK::System::Dec\n"  // always start with this line
        "//VTK::Output::Dec\n"  // always have this line in your FS
        "varying vec3 TexCoords;\n"
        "uniform sampler2D texture_0;\n" // texture_0 is the first texture
        "void main () {\n"
        "  float phix = length(vec2(TexCoords.x, TexCoords.z));\n"
        "  gl_FragData[0] = texture(texture_0, vec2(0.5*atan(TexCoords.z, TexCoords.x)/3.1415927 + 0.5, atan(TexCoords.y,phix)/3.1415927 + 0.5));\n"
        "}\n"
        );
    }
    this->CubeMapper->Modified();
    this->LastProjection = this->Projection;
  }

  this->OpenGLActor->SetPosition(ren->GetActiveCamera()->GetPosition());

  // get opacity
  glDepthMask(GL_TRUE);

  // send a render to the mapper; update pipeline
  mapper->Render(ren, this->OpenGLActor);

  vtkOpenGLCheckErrorMacro("failed after Render");
}

//----------------------------------------------------------------------------
void vtkOpenGLSkybox::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
