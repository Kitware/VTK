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
#include "vtkOpenVRModel.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkTextureObject.h"

/*=========================================================================
vtkOpenVRModel
=========================================================================*/
vtkStandardNewMacro(vtkOpenVRModel);

//------------------------------------------------------------------------------
vtkOpenVRModel::vtkOpenVRModel()
{
  this->RawModel = nullptr;
  this->RawTexture = nullptr;
}

//------------------------------------------------------------------------------
void vtkOpenVRModel::FillModelHelper()
{
  this->ModelVBO->Upload(
    this->RawModel->rVertexData, this->RawModel->unVertexCount, vtkOpenGLBufferObject::ArrayBuffer);

  this->ModelHelper.IBO->Upload(this->RawModel->rIndexData, this->RawModel->unTriangleCount * 3,
    vtkOpenGLBufferObject::ElementArrayBuffer);
  this->ModelHelper.IBO->IndexCount = this->RawModel->unTriangleCount * 3;
}

//------------------------------------------------------------------------------
void vtkOpenVRModel::SetPositionAndTCoords()
{
  this->ModelHelper.VAO->Bind();
  vtkShaderProgram* program = this->ModelHelper.Program;
  if (!this->ModelHelper.VAO->AddAttributeArray(program, this->ModelVBO, "position",
        offsetof(vr::RenderModel_Vertex_t, vPosition), sizeof(vr::RenderModel_Vertex_t), VTK_FLOAT,
        3, false))
  {
    vtkErrorMacro(<< "Error setting position in shader VAO.");
  }
  if (!this->ModelHelper.VAO->AddAttributeArray(program, this->ModelVBO, "v2TexCoordsIn",
        offsetof(vr::RenderModel_Vertex_t, rfTextureCoord), sizeof(vr::RenderModel_Vertex_t),
        VTK_FLOAT, 2, false))
  {
    vtkErrorMacro(<< "Error setting tcoords in shader VAO.");
  }
}

//------------------------------------------------------------------------------
void vtkOpenVRModel::CreateTextureObject(vtkOpenGLRenderWindow* win)
{
  this->TextureObject->SetContext(win);
  this->TextureObject->Create2DFromRaw(this->RawTexture->unWidth, this->RawTexture->unHeight, 4,
    VTK_UNSIGNED_CHAR,
    const_cast<void*>(static_cast<const void* const>(this->RawTexture->rubTextureMapData)));
  this->TextureObject->SetWrapS(vtkTextureObject::ClampToEdge);
  this->TextureObject->SetWrapT(vtkTextureObject::ClampToEdge);

  this->TextureObject->SetMinificationFilter(vtkTextureObject::LinearMipmapLinear);
  this->TextureObject->SetGenerateMipmap(true);
}

//------------------------------------------------------------------------------
void vtkOpenVRModel::LoadModelAndTexture(vtkOpenGLRenderWindow* win)
{
  // do we not have the model loaded? Keep trying it is async
  if (!this->RawModel)
  {
    // start loading the model if we didn't find one
    vr::EVRRenderModelError result =
      vr::VRRenderModels()->LoadRenderModel_Async(this->GetName().c_str(), &this->RawModel);
    if (result > vr::EVRRenderModelError::VRRenderModelError_Loading)
    {
      this->FailedToLoad = true;
      if (result != vr::VRRenderModelError_NotEnoughTexCoords)
      {
        vtkErrorMacro(
          "Unable to load render model " << this->GetName() << " with error code " << result);
      }
      return; // move on to the next tracked device
    }
  }

  // if we have the model try loading the texture
  if (this->RawModel && !this->RawTexture)
  {
    if (vr::VRRenderModels()->LoadTexture_Async(this->RawModel->diffuseTextureId,
          &this->RawTexture) > vr::EVRRenderModelError::VRRenderModelError_Loading)
    {
      vtkErrorMacro(<< "Unable to load render texture for render model " << this->GetName());
    }

    // if this call succeeded and we have the texture
    // then build the VTK structures
    if (this->RawTexture)
    {
      if (!this->Build(win))
      {
        vtkErrorMacro("Unable to create GL model from render model " << this->GetName());
      }
      vr::VRRenderModels()->FreeRenderModel(this->RawModel);
      vr::VRRenderModels()->FreeTexture(this->RawTexture);
      this->Loaded = true;
    }
  }
}
