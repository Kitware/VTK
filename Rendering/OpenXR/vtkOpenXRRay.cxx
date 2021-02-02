/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenVRModel.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenXRRay.h"

#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLHelper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkRendererCollection.h"
#include "vtkShaderProgram.h"

#include "vtkOpenXR.h"

/*=========================================================================
vtkOpenXRRay
=========================================================================*/
vtkStandardNewMacro(vtkOpenXRRay);

//------------------------------------------------------------------------------
void vtkOpenXRRay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Loaded " << (this->Loaded ? "On\n" : "Off\n");
}

//------------------------------------------------------------------------------
void vtkOpenXRRay::ReleaseGraphicsResources(vtkRenderWindow* win)
{
  this->RayVBO->ReleaseGraphicsResources();
  this->RayHelper.ReleaseGraphicsResources(win);
}

//------------------------------------------------------------------------------
bool vtkOpenXRRay::Build(vtkOpenGLRenderWindow* win)
{
  // Ray geometry
  float vert[] = { 0, 0, 0, 0, 0, -1 };

  this->RayVBO->Upload(vert, 2 * 3, vtkOpenGLBufferObject::ArrayBuffer);

  this->RayHelper.Program = win->GetShaderCache()->ReadyShaderProgram(

    // vertex shader
    "//VTK::System::Dec\n"
    "uniform mat4 matrix;\n"
    "uniform float scale;\n"
    "in vec3 position;\n"
    "void main()\n"
    "{\n"
    " gl_Position =  matrix * vec4(scale * position, 1.0);\n"
    "}\n",

    // fragment shader
    "//VTK::System::Dec\n"
    "//VTK::Output::Dec\n"
    "uniform vec3 color;\n"
    "void main()\n"
    "{\n"
    "   gl_FragData[0] = vec4(color, 1.0);\n"
    "}\n",

    // geom shader
    "");
  vtkShaderProgram* program = this->RayHelper.Program;
  this->RayHelper.VAO->Bind();
  if (!this->RayHelper.VAO->AddAttributeArray(
        program, this->RayVBO, "position", 0, 3 * sizeof(float), VTK_FLOAT, 3, false))
  {
    vtkErrorMacro(<< "Error setting position in shader VAO.");
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkOpenXRRay::Render(vtkOpenGLRenderWindow* win, vtkMatrix4x4* poseMatrix)
{
  // Load ray
  if (!this->Loaded)
  {
    if (!this->Build(win))
    {
      vtkErrorMacro("Unable to build controller ray ");
    }
    this->Loaded = true;
  }

  // Render ray
  win->GetState()->vtkglDepthMask(GL_TRUE);
  win->GetShaderCache()->ReadyShaderProgram(this->RayHelper.Program);
  this->RayHelper.VAO->Bind();

  vtkRenderer* ren = static_cast<vtkRenderer*>(win->GetRenderers()->GetItemAsObject(0));
  if (!ren)
  {
    vtkErrorMacro("Unable get renderer");
    return;
  }

  double unitV[4] = { 0, 0, 0, 1 };
  double scaleFactor = vtkMath::Norm(poseMatrix->MultiplyDoublePoint(unitV));

  this->RayHelper.Program->SetUniformf("scale", this->Length / scaleFactor);
  this->RayHelper.Program->SetUniform3f("color", this->Color);

  this->RayHelper.Program->SetUniformMatrix("matrix", poseMatrix);

  glDrawArrays(GL_LINES, 0, 6);
}
