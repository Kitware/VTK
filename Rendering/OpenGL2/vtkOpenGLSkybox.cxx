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


#include "vtkMapper.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkOpenGLError.h"
#include "vtkRenderWindow.h"
#include "vtkTransform.h"

#include <cmath>

vtkStandardNewMacro(vtkOpenGLSkybox);


vtkOpenGLSkybox::vtkOpenGLSkybox()
{
  this->MCWCMatrix = vtkMatrix4x4::New();
  this->NormalMatrix = vtkMatrix3x3::New();
  this->NormalTransform = vtkTransform::New();

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

  this->GetProperty()->SetDiffuse(0.0);
  this->GetProperty()->SetAmbient(1.0);
  this->GetProperty()->SetSpecular(0.0);
}

vtkOpenGLSkybox::~vtkOpenGLSkybox()
{
  this->MCWCMatrix->Delete();
  this->NormalMatrix->Delete();
  this->NormalTransform->Delete();
}


// Actual Skybox render method.
void vtkOpenGLSkybox::Render(vtkRenderer *ren, vtkMapper *mapper)
{
  vtkOpenGLClearErrorMacro();

  this->SetPosition(ren->GetActiveCamera()->GetPosition());

  // get opacity
  glDepthMask(GL_TRUE);

  // send a render to the mapper; update pipeline
  mapper->Render(ren, this);

  vtkOpenGLCheckErrorMacro("failed after Render");
}

//----------------------------------------------------------------------------
void vtkOpenGLSkybox::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

void vtkOpenGLSkybox::GetKeyMatrices(vtkMatrix4x4 *&mcwc, vtkMatrix3x3 *&normMat)
{
  // has the Skybox changed?
  if (this->GetMTime() > this->KeyMatrixTime)
  {
    this->ComputeMatrix();
    this->MCWCMatrix->DeepCopy(this->Matrix);
    this->MCWCMatrix->Transpose();

    if (this->GetIsIdentity())
    {
      this->NormalMatrix->Identity();
    }
    else
    {
      this->NormalTransform->SetMatrix(this->Matrix);
      vtkMatrix4x4 *mat4 = this->NormalTransform->GetMatrix();
      for(int i = 0; i < 3; ++i)
      {
        for (int j = 0; j < 3; ++j)
        {
          this->NormalMatrix->SetElement(i, j, mat4->GetElement(i, j));
        }
      }
    }
    this->NormalMatrix->Invert();
    this->KeyMatrixTime.Modified();
  }

  mcwc = this->MCWCMatrix;
  normMat = this->NormalMatrix;
}
