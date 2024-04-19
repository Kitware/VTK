// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGridRenderRequest.h"

#include "vtkActor.h"
#include "vtkCellGridResponders.h"
#include "vtkCellMetadata.h"
// #include "vtkDGHex.h"
// #include "vtkDGOpenGLRenderer.h"
// #include "vtkDGQuad.h"
// #include "vtkDGRenderResponder.h"
// #include "vtkDGTet.h"
// #include "vtkDGTri.h"
#include "vtkCellGridMapper.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkWindow.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkCellGridRenderRequest);

vtkCellGridRenderRequest::~vtkCellGridRenderRequest()
{
  // std::cout << " !!! Destruct vtkCellGridRenderRequest, clearing this->State" << std::endl;
  this->State.clear();
  this->SetActor(nullptr);
  this->SetMapper(nullptr);
  this->SetRenderer(nullptr);
  this->SetWindow(nullptr);
}

void vtkCellGridRenderRequest::PrintSelf(std::ostream& os, vtkIndent indent)
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

void vtkCellGridRenderRequest::Initialize() {}

void vtkCellGridRenderRequest::Finalize()
{
  // Always reset the request after releasing resources (but
  // never assume we're going to release resources after a
  // render pass).
  if (this->IsReleasingResources)
  {
    this->SetIsReleasingResources(false);
  }
}

void vtkCellGridRenderRequest::SetMapper(vtkCellGridMapper* mapper)
{
  if (this->Mapper == mapper)
  {
    return;
  }
  this->Mapper = mapper;
  this->Modified();
}

void vtkCellGridRenderRequest::SetActor(vtkActor* actor)
{
  if (this->Actor == actor)
  {
    return;
  }
  this->Actor = actor;
  this->Modified();
}

void vtkCellGridRenderRequest::SetRenderer(vtkRenderer* renderer)
{
  if (this->Renderer == renderer)
  {
    return;
  }
  this->Renderer = renderer;
  this->Modified();
}

void vtkCellGridRenderRequest::SetWindow(vtkWindow* window)
{
  if (this->Window == window)
  {
    return;
  }
  this->Window = window;
  this->Modified();
}

VTK_ABI_NAMESPACE_END
