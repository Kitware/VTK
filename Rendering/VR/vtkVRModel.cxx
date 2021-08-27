/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkVRModel.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVRModel.h"

#include "vtkInteractorObserver.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLHelper.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLState.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRendererCollection.h"
#include "vtkShaderProgram.h"
#include "vtkTextureObject.h"
#include "vtkVRRay.h"

#include "vtkVRCamera.h"
#include "vtk_glew.h"

//------------------------------------------------------------------------------
vtkVRModel::vtkVRModel()
{
  this->Loaded = false;
  this->ModelVBO = vtkOpenGLVertexBufferObject::New();
  this->FailedToLoad = false;
}

//------------------------------------------------------------------------------
vtkVRModel::~vtkVRModel()
{
  this->ModelVBO->Delete();
  this->ModelVBO = nullptr;
}

//------------------------------------------------------------------------------
void vtkVRModel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ModelName: " << this->ModelName << "\n";
  os << indent << "Visibility: " << this->Visibility << "\n";
  os << indent << "Loaded " << (this->Loaded ? "On\n" : "Off\n");
  os << indent << "FailedToLoad: " << this->FailedToLoad << "\n";

  this->ModelVBO->PrintSelf(os, indent);
  this->TextureObject->PrintSelf(os, indent);
  this->PoseMatrix->PrintSelf(os, indent);
  this->Ray->PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkVRModel::ReleaseGraphicsResources(vtkWindow* win)
{
  this->ModelVBO->ReleaseGraphicsResources();
  this->ModelHelper.ReleaseGraphicsResources(win);
  this->TextureObject->ReleaseGraphicsResources(win);
}

//------------------------------------------------------------------------------
bool vtkVRModel::Build(vtkOpenGLRenderWindow* win)
{
  this->FillModelHelper();

  this->ModelHelper.Program = win->GetShaderCache()->ReadyShaderProgram(

    // vertex shader -- use normals?? yes?
    "//VTK::System::Dec\n"
    "uniform mat4 matrix;\n"
    "in vec4 position;\n"
    //    "attribute vec3 v3NormalIn;\n"
    "in vec2 v2TexCoordsIn;\n"
    "out vec2 v2TexCoord;\n"
    "void main()\n"
    "{\n"
    " v2TexCoord = v2TexCoordsIn;\n"
    " gl_Position = matrix * vec4(position.xyz, 1);\n"
    "}\n",

    // fragment shader
    "//VTK::System::Dec\n"
    "//VTK::Output::Dec\n"
    "uniform sampler2D diffuse;\n"
    "in vec2 v2TexCoord;\n"
    "out vec4 outputColor;\n"
    "void main()\n"
    "{\n"
    "  gl_FragData[0] = texture(diffuse, v2TexCoord);\n"
    "}\n",

    // geom shader
    "");

  this->SetPositionAndTCoords();

  // create and populate the texture
  this->CreateTextureObject(win);

  return true;
}

//------------------------------------------------------------------------------
void vtkVRModel::Render(vtkOpenGLRenderWindow* win, const float poseInTrackingCoordinates[][4])
{
  if (this->FailedToLoad)
  {
    return;
  }

  this->LoadModelAndTexture(win);

  if (this->Loaded)
  {
    // render the model
    win->GetState()->vtkglDepthMask(GL_TRUE);
    win->GetShaderCache()->ReadyShaderProgram(this->ModelHelper.Program);
    this->ModelHelper.VAO->Bind();
    this->ModelHelper.IBO->Bind();

    this->TextureObject->Activate();
    this->ModelHelper.Program->SetUniformi("diffuse", this->TextureObject->GetTextureUnit());

    vtkRenderer* ren = static_cast<vtkRenderer*>(win->GetRenderers()->GetItemAsObject(0));
    if (ren)
    {
      vtkVRCamera* cam = static_cast<vtkVRCamera*>(ren->GetActiveCamera());

      double elems[16];
      for (int j = 0; j < 3; j++)
      {
        for (int i = 0; i < 4; i++)
        {
          elems[j + i * 4] = poseInTrackingCoordinates[j][i];
        }
      }
      elems[3] = 0.0;
      elems[7] = 0.0;
      elems[11] = 0.0;
      elems[15] = 1.0;

      vtkMatrix4x4* tcdc;
      cam->GetTrackingToDCMatrix(tcdc);

      vtkMatrix4x4::Multiply4x4(
        elems, (double*)(tcdc->Element), (double*)(this->PoseMatrix->Element));

      this->ModelHelper.Program->SetUniformMatrix("matrix", this->PoseMatrix);
    }

    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(this->ModelHelper.IBO->IndexCount),
      GL_UNSIGNED_SHORT, nullptr);
    this->TextureObject->Deactivate();

    // Draw ray
    if (this->Ray->GetShow())
    {
      this->Ray->Render(win, this->PoseMatrix);
    }
  }
}

//------------------------------------------------------------------------------
void vtkVRModel::SetShowRay(bool v)
{
  this->Ray->SetShow(v);
}

//------------------------------------------------------------------------------
void vtkVRModel::SetRayLength(double length)
{
  this->Ray->SetLength(length);
}

//------------------------------------------------------------------------------
void vtkVRModel::SetRayColor(double r, double g, double b)
{
  float color[] = { static_cast<float>(r), static_cast<float>(g), static_cast<float>(b) };
  this->Ray->SetColor(color);
}
