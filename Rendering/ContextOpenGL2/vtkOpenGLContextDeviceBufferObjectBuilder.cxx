// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenGLContextDeviceBufferObjectBuilder.h"
#include "vtkLogger.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLVertexBufferObjectGroup.h"
#include "vtkRenderTimerLog.h"
#include "vtkRenderWindow.h"

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
void vtkOpenGLContextDeviceBufferObjectBuilder::BuildVBO(vtkOpenGLHelper* cbo,
  vtkDataArray* positions, vtkUnsignedCharArray* colors, vtkFloatArray* tcoords,
  std::size_t cacheIdentifier, vtkRenderWindow* renderWindow)
{
  auto timer = renderWindow->GetRenderTimer();
  VTK_SCOPED_RENDER_EVENT("vtkOpenGLContextDeviceBufferObjectBuilder"
      << "::" << __func__ << "(cacheIdentifier: " << cacheIdentifier
      << ", points:" << positions->GetNumberOfTuples() << "[x" << positions->GetNumberOfComponents()
      << "]colors:" << (colors ? colors->GetNumberOfTuples() : 0) << "[x"
      << (colors ? colors->GetNumberOfComponents() : 0)
      << "], tcoords:" << (tcoords ? tcoords->GetNumberOfTuples() : 0) << "[x2]",
    timer);

  auto vboCache = static_cast<vtkOpenGLRenderWindow*>(renderWindow)->GetVBOCache();
  const auto vboGroupIter =
    this->VBOGroups
      .emplace(cacheIdentifier, vtk::TakeSmartPointer(vtkOpenGLVertexBufferObjectGroup::New()))
      .first;
  auto& vbos = vboGroupIter->second;
  vbos->CacheDataArray("vertexMC", positions, vboCache, VTK_FLOAT);
  if (colors && colors->GetNumberOfTuples() > 0)
  {
    vbos->CacheDataArray("vertexScalar", colors, vboCache, VTK_UNSIGNED_CHAR);
  }
  if (tcoords && tcoords->GetNumberOfTuples() > 0)
  {
    vbos->CacheDataArray("tcoordMC", tcoords, vboCache, VTK_FLOAT);
  }
  // uploads only if array contents are different from the last time they were uploaded.
  vbos->BuildAllVBOs(vboCache);
  // release existing vertex attribute pointers.
  cbo->VAO->ShaderProgramChanged();
  // setup new vertex attributes.
  cbo->VAO->Bind();
  vbos->AddAllAttributesToVAO(cbo->Program, cbo->VAO);
}

//------------------------------------------------------------------------------
void vtkOpenGLContextDeviceBufferObjectBuilder::Erase(
  std::size_t cacheIdentifier, vtkRenderWindow* renderWindow)
{
  auto vboGroupIter = this->VBOGroups.find(cacheIdentifier);
  if (vboGroupIter != this->VBOGroups.end())
  {
    vboGroupIter->second->ReleaseGraphicsResources(renderWindow);
    this->VBOGroups.erase(vboGroupIter);
  }
}

VTK_ABI_NAMESPACE_END
