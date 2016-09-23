/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkColorMaterialHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkColorMaterialHelper.h"

#include "vtkShaderProgram2.h"
#include "vtkObjectFactory.h"
#include "vtkShader2.h"
#include "vtkShader2Collection.h"
#include "vtkUniformVariables.h"
#include "vtkgl.h"

extern const char * vtkColorMaterialHelper_vs;

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkColorMaterialHelper);

//----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkColorMaterialHelper, Shader, vtkShaderProgram2);

//----------------------------------------------------------------------------
vtkColorMaterialHelper::vtkColorMaterialHelper()
{
  this->Shader = 0;
}

//----------------------------------------------------------------------------
vtkColorMaterialHelper::~vtkColorMaterialHelper()
{
  this->SetShader(0);
}

//----------------------------------------------------------------------------
void vtkColorMaterialHelper::Initialize(vtkShaderProgram2* pgm)
{
  if (this->Shader != pgm)
  {
    this->SetShader(pgm);
    if (pgm)
    {
      vtkShader2 *s=vtkShader2::New();
      s->SetSourceCode(vtkColorMaterialHelper_vs);
      s->SetType(VTK_SHADER_TYPE_VERTEX);
      s->SetContext(pgm->GetContext());
      pgm->GetShaders()->AddItem(s);
      s->Delete();
    }
  }
}

//----------------------------------------------------------------------------
void vtkColorMaterialHelper::PrepareForRendering()
{
  #ifndef NDEBUG
  if (!this->Shader)
  {
    vtkErrorMacro("Please Initialize() before calling PrepareForRendering().");
    return ;
  }
  #endif

  this->Mode = vtkColorMaterialHelper::DISABLED;
  if (glIsEnabled(GL_COLOR_MATERIAL))
  {
    GLint colorMaterialParameter;
    glGetIntegerv(GL_COLOR_MATERIAL_PARAMETER, &colorMaterialParameter);
    switch (colorMaterialParameter)
    {
    case GL_AMBIENT:
      this->Mode = vtkColorMaterialHelper::AMBIENT;
      break;

    case GL_DIFFUSE:
      this->Mode = vtkColorMaterialHelper::DIFFUSE;
      break;

    case GL_SPECULAR:
      this->Mode = vtkColorMaterialHelper::SPECULAR;
      break;

    case GL_AMBIENT_AND_DIFFUSE:
      this->Mode = vtkColorMaterialHelper::AMBIENT_AND_DIFFUSE;
      break;

    case GL_EMISSION:
      this->Mode = vtkColorMaterialHelper::EMISSION;
      break;
    }
  }
}

//----------------------------------------------------------------------------
void vtkColorMaterialHelper::Render()
{
  #ifndef NDEBUG
  if (!this->Shader)
  {
    vtkErrorMacro("Please Initialize() before calling Render().");
    return;
  }
  #endif

  int value=this->Mode;
  this->Shader->GetUniformVariables()->SetUniformi("vtkColorMaterialHelper_Mode",1,&value);
}

//----------------------------------------------------------------------------
void vtkColorMaterialHelper::SetUniformVariables()
{
  this->PrepareForRendering(); // iniitialize this with gl state
  this->Render();              // send as uniforms
}

//----------------------------------------------------------------------------
void vtkColorMaterialHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Shader: " << this->Shader << endl;
}
