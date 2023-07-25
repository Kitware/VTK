// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenXRModel.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkTextureObject.h"

#include <thread>

/*=========================================================================
vtkOpenXRModel
=========================================================================*/
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenXRModel);

//------------------------------------------------------------------------------
void vtkOpenXRModel::FillModelHelper()
{
  this->ModelVBO->Upload(this->ModelVBOData, vtkOpenGLBufferObject::ArrayBuffer);
  this->ModelHelper.IBO->Upload(this->ModelIBOData, vtkOpenGLBufferObject::ElementArrayBuffer);
  this->ModelHelper.IBO->IndexCount = this->ModelIBOData.size();
}

//------------------------------------------------------------------------------
void vtkOpenXRModel::SetPositionAndTCoords()
{
  this->ModelHelper.VAO->Bind();
  vtkShaderProgram* program = this->ModelHelper.Program;
  if (!this->ModelHelper.VAO->AddAttributeArray(
        program, this->ModelVBO, "position", 0, 5 * sizeof(float), VTK_FLOAT, 3, false))
  {
    vtkErrorMacro(<< "Error setting position in shader VAO.");
  }
  if (!this->ModelHelper.VAO->AddAttributeArray(program, this->ModelVBO, "v2TexCoordsIn",
        3 * sizeof(float), 5 * sizeof(float), VTK_FLOAT, 2, false))
  {
    vtkErrorMacro(<< "Error setting tcoords in shader VAO.");
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRModel::CreateTextureObject(vtkOpenGLRenderWindow* win)
{
  this->TextureObject->SetContext(win);
  this->TextureObject->Create2DFromRaw(16, 16, 4, VTK_UNSIGNED_CHAR,
    const_cast<void*>(static_cast<const void*>(this->TextureData.data())));
  this->TextureObject->SetWrapS(vtkTextureObject::ClampToEdge);
  this->TextureObject->SetWrapT(vtkTextureObject::ClampToEdge);

  this->TextureObject->SetMinificationFilter(vtkTextureObject::LinearMipmapLinear);
  this->TextureObject->SetGenerateMipmap(true);
}

void vtkOpenXRModel::AsyncLoad()
{
  // for now use a pyramid for a controller
  for (int k = 0; k < 2; k++)
  {
    for (int j = 0; j < 2; j++)
    {
      for (int i = 0; i < 2; i++)
      {
        // 5 cm x 10 cm controller
        this->ModelVBOData.push_back(i * 0.05);
        this->ModelVBOData.push_back(j * 0.05);
        this->ModelVBOData.push_back(k * 0.1);
        // tcoords
        this->ModelVBOData.push_back(0.0);
        this->ModelVBOData.push_back(0.0);
      }
    }
  }

  this->ModelIBOData.push_back(0);
  this->ModelIBOData.push_back(4);
  this->ModelIBOData.push_back(5);

  this->ModelIBOData.push_back(0);
  this->ModelIBOData.push_back(4);
  this->ModelIBOData.push_back(6);

  this->ModelIBOData.push_back(4);
  this->ModelIBOData.push_back(5);
  this->ModelIBOData.push_back(6);

  for (int i = 0; i < 16 * 16; i++)
  {
    this->TextureData.push_back(128);
    this->TextureData.push_back(255);
    this->TextureData.push_back(128);
    this->TextureData.push_back(255);
  }

  this->ModelLoaded = true;
  this->ModelLoading = false;
}

//------------------------------------------------------------------------------
void vtkOpenXRModel::LoadModelAndTexture(vtkOpenGLRenderWindow* win)
{
  // if we do not have the model loaded and haven't initiated loading
  if (!this->ModelLoaded && !this->ModelLoading)
  {
    this->ModelLoading = true;
    // loading the models can be slow so do it in a separate thread
    std::thread loader([this] { this->AsyncLoad(); });
    loader.detach();
  }

  if (this->ModelLoaded && !this->Loaded)
  {
    if (!this->Build(win))
    {
      vtkErrorMacro("Unable to create GL model from render model " << this->GetName());
    }
    this->Loaded = true;
  }
}
VTK_ABI_NAMESPACE_END
