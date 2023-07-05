// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWebGPUCamera.h"

#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkWebGPURenderer.h"

#include <cstdint> // for uint32_t

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebGPUCamera);

//------------------------------------------------------------------------------
vtkWebGPUCamera::vtkWebGPUCamera() = default;

//------------------------------------------------------------------------------
vtkWebGPUCamera::~vtkWebGPUCamera() = default;

//------------------------------------------------------------------------------
void vtkWebGPUCamera::Render(vtkRenderer* renderer)
{
  this->CacheSceneTransforms(renderer);
}

//------------------------------------------------------------------------------
void vtkWebGPUCamera::CacheSceneTransforms(vtkRenderer* renderer)
{
  // has the camera changed?
  if (renderer != this->LastRenderer || this->MTime > this->KeyMatrixTime ||
    renderer->GetMTime() > this->KeyMatrixTime)
  {
    vtkMatrix4x4* view = this->GetModelViewTransformMatrix();
    SceneTransforms& st = this->CachedSceneTransforms;

    for (int i = 0; i < 4; ++i)
    {
      for (int j = 0; j < 4; ++j)
      {
        st.ViewMatrix[i][j] = view->GetElement(j, i);
      }
    }
    // since directx, vulkan and metal expect z-coordinate to lie in [0, 1] instead of [-1, 1],
    // webgpu culls fragments that have a z outside of [0, 1]; even for opengl backend.
    vtkMatrix4x4* projection =
      this->GetProjectionTransformMatrix(renderer->GetTiledAspectRatio(), 0, 1);
    for (int i = 0; i < 4; ++i)
    {
      for (int j = 0; j < 4; ++j)
      {
        // transpose because, shader will interpret it in a column-major order.
        st.ProjectionMatrix[i][j] = projection->GetElement(j, i);
      }
    }
    // normal matrix
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        // need to transpose.
        // but do not transpose, shader will interpret it in a column-major order.
        this->NormalMatrix->SetElement(i, j, view->GetElement(i, j));
      }
    }
    this->NormalMatrix->Invert();
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        // transpose because, shader will interpret it in a column-major order.
        st.NormalMatrix[i][j] = this->NormalMatrix->GetElement(i, j);
      }
    }
    projection->Invert();
    for (int i = 0; i < 4; ++i)
    {
      for (int j = 0; j < 4; ++j)
      {
        // transpose because, shader will interpret it in a column-major order.
        st.InvertedProjectionMatrix[i][j] = projection->GetElement(j, i);
      }
    }
    int lowerLeft[2];
    int width, height;
    renderer->GetTiledSizeAndOrigin(&width, &height, &lowerLeft[0], &lowerLeft[1]);
    st.Viewport[0] = lowerLeft[0];
    st.Viewport[1] = lowerLeft[1];
    st.Viewport[2] = width;
    st.Viewport[3] = height;

    this->KeyMatrixTime.Modified();
    this->LastRenderer = renderer;
  }
}
//------------------------------------------------------------------------------
void vtkWebGPUCamera::UpdateViewport(vtkRenderer* renderer)
{
  int lowerLeft[2];
  int width, height;
  renderer->GetTiledSizeAndOrigin(&width, &height, &lowerLeft[0], &lowerLeft[1]);
  auto rpassEncoder = reinterpret_cast<vtkWebGPURenderer*>(renderer)->GetRenderPassEncoder();

  // Set viewport frustum
  rpassEncoder.SetViewport(
    lowerLeft[0], lowerLeft[1], static_cast<float>(width), static_cast<float>(height), 0.0, 1.0);
  if (this->UseScissor)
  {
    // Set scissor rectangle
    rpassEncoder.SetScissorRect(static_cast<uint32_t>(this->ScissorRect.GetLeft()),
      static_cast<uint32_t>(this->ScissorRect.GetBottom()),
      static_cast<uint32_t>(this->ScissorRect.GetWidth()),
      static_cast<uint32_t>(this->ScissorRect.GetWidth()));
    this->UseScissor = false;
  }
  else
  {
    rpassEncoder.SetScissorRect(
      0u, 0u, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUCamera::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

VTK_ABI_NAMESPACE_END
