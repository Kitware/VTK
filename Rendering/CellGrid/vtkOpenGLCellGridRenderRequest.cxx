// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenGLCellGridRenderRequest.h"

#include "vtkActor.h"
#include "vtkCellGridResponders.h"
#include "vtkCellMetadata.h"
#include "vtkDGHex.h"
#include "vtkDGOpenGLRenderer.h"
#include "vtkDGTet.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLCellGridMapper.h"
#include "vtkRenderer.h"
#include "vtkWindow.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkOpenGLCellGridRenderRequest);

vtkOpenGLCellGridRenderRequest::vtkOpenGLCellGridRenderRequest()
{
  // Plugins are expected to register responders, but for the base functionality provided
  // by VTK itself, we use this object to register responders at construction.
  // Since the vtkOpenGLCellGridMapper owns an instance of this request, the registration
  // is guaranteed to occur in time for the first render of cell types supported by VTK.
  static bool initialized = false;
  if (!initialized)
  {
    vtkNew<vtkDGOpenGLRenderer> dgResponder;
    auto* responders = vtkCellMetadata::GetResponders();
    responders->RegisterQueryResponder<vtkDGHex, vtkOpenGLCellGridRenderRequest>(
      dgResponder.GetPointer());
    responders->RegisterQueryResponder<vtkDGTet, vtkOpenGLCellGridRenderRequest>(
      dgResponder.GetPointer());
    initialized = true;
  }
}

vtkOpenGLCellGridRenderRequest::~vtkOpenGLCellGridRenderRequest()
{
  this->State.clear();
  this->SetActor(nullptr);
  this->SetMapper(nullptr);
  this->SetRenderer(nullptr);
  this->SetWindow(nullptr);
}

void vtkOpenGLCellGridRenderRequest::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Actor: " << this->Actor << "\n";
  os << indent << "Renderer: " << this->Renderer << "\n";
  os << indent << "Mapper: " << this->Mapper << "\n";
  os << indent << "Window: " << this->Window << "\n";
  os << indent << "IsReleasingResources: " << (this->IsReleasingResources ? "True" : "False")
     << "\n";
  os << indent << "State: (" << this->State.size() << " entries)\n";
  vtkIndent i2 = indent.GetNextIndent();
  for (const auto& entry : this->State)
  {
    os << i2 << entry.first.Data() << ": " << entry.second.get() << "\n";
  }
}

void vtkOpenGLCellGridRenderRequest::Initialize() {}

void vtkOpenGLCellGridRenderRequest::Finalize()
{
  // Always reset the request after releasing resources (but
  // never assume we're going to release resources after a
  // render pass).
  if (this->IsReleasingResources)
  {
    this->SetIsReleasingResources(false);
  }
}

void vtkOpenGLCellGridRenderRequest::SetMapper(vtkOpenGLCellGridMapper* mapper)
{
  if (this->Mapper == mapper)
  {
    return;
  }
  this->Mapper = mapper;
  this->Modified();
}

void vtkOpenGLCellGridRenderRequest::SetActor(vtkActor* actor)
{
  if (this->Actor == actor)
  {
    return;
  }
  this->Actor = actor;
  this->Modified();
}

void vtkOpenGLCellGridRenderRequest::SetRenderer(vtkRenderer* renderer)
{
  if (this->Renderer == renderer)
  {
    return;
  }
  this->Renderer = renderer;
  this->Modified();
}

void vtkOpenGLCellGridRenderRequest::SetWindow(vtkWindow* window)
{
  if (this->Window == window)
  {
    return;
  }
  this->Window = window;
  this->Modified();
}

VTK_ABI_NAMESPACE_END
