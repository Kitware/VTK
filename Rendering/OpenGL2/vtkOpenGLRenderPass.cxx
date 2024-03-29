// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenGLRenderPass.h"

#include "vtkInformation.h"
#include "vtkInformationObjectBaseVectorKey.h"
#include "vtkObjectFactory.h"
#include "vtkProp.h"
#include "vtkRenderState.h"

#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkInformationKeyMacro(vtkOpenGLRenderPass, RenderPasses, ObjectBaseVector);

//------------------------------------------------------------------------------
void vtkOpenGLRenderPass::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
bool vtkOpenGLRenderPass::PreReplaceShaderValues(
  std::string&, std::string&, std::string&, vtkAbstractMapper*, vtkProp*)
{
  return true;
}
bool vtkOpenGLRenderPass::PostReplaceShaderValues(
  std::string&, std::string&, std::string&, vtkAbstractMapper*, vtkProp*)
{
  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenGLRenderPass::SetShaderParameters(
  vtkShaderProgram*, vtkAbstractMapper*, vtkProp*, vtkOpenGLVertexArrayObject*)
{
  return true;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkOpenGLRenderPass::GetShaderStageMTime()
{
  return 0;
}

//------------------------------------------------------------------------------
vtkOpenGLRenderPass::vtkOpenGLRenderPass() = default;

//------------------------------------------------------------------------------
vtkOpenGLRenderPass::~vtkOpenGLRenderPass() = default;

//------------------------------------------------------------------------------
void vtkOpenGLRenderPass::PreRender(const vtkRenderState* s)
{
  assert("Render state valid." && s);
  size_t numProps = s->GetPropArrayCount();
  for (size_t i = 0; i < numProps; ++i)
  {
    vtkProp* prop = s->GetPropArray()[i];
    this->PreRenderProp(prop);
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderPass::PreRenderProp(vtkProp* prop)
{
  if (prop)
  {
    vtkInformation* info = prop->GetPropertyKeys();
    if (!info)
    {
      info = vtkInformation::New();
      prop->SetPropertyKeys(info);
      info->FastDelete();
    }
    info->Append(vtkOpenGLRenderPass::RenderPasses(), this);
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderPass::PostRender(const vtkRenderState* s)
{
  assert("Render state valid." && s);
  size_t numProps = s->GetPropArrayCount();
  for (size_t i = 0; i < numProps; ++i)
  {
    vtkProp* prop = s->GetPropArray()[i];
    this->PostRenderProp(prop);
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderPass::PostRenderProp(vtkProp* prop)
{
  if (prop)
  {
    vtkInformation* info = prop->GetPropertyKeys();
    if (info)
    {
      info->Remove(vtkOpenGLRenderPass::RenderPasses(), this);
      if (info->Length(vtkOpenGLRenderPass::RenderPasses()) == 0)
      {
        info->Remove(vtkOpenGLRenderPass::RenderPasses());
      }
    }
  }
}
VTK_ABI_NAMESPACE_END
