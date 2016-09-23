/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLRenderPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLRenderPass.h"

#include "vtkInformation.h"
#include "vtkInformationObjectBaseVectorKey.h"
#include "vtkObjectFactory.h"
#include "vtkProp.h"
#include "vtkRenderState.h"

#include <cassert>

vtkInformationKeyMacro(vtkOpenGLRenderPass, RenderPasses, ObjectBaseVector)

//------------------------------------------------------------------------------
void vtkOpenGLRenderPass::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
bool vtkOpenGLRenderPass::ReplaceShaderValues(std::string &, std::string &,
                                              std::string &,
                                              vtkAbstractMapper *, vtkProp *)
{
  return true;
}

//------------------------------------------------------------------------------
bool vtkOpenGLRenderPass::SetShaderParameters(vtkShaderProgram *,
                                              vtkAbstractMapper *, vtkProp *)
{
  return true;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkOpenGLRenderPass::GetShaderStageMTime()
{
  return 0;
}

//------------------------------------------------------------------------------
vtkOpenGLRenderPass::vtkOpenGLRenderPass()
{
}

//------------------------------------------------------------------------------
vtkOpenGLRenderPass::~vtkOpenGLRenderPass()
{
}

//------------------------------------------------------------------------------
void vtkOpenGLRenderPass::PreRender(const vtkRenderState *s)
{
  assert("Render state valid." && s);
  size_t numProps = s->GetPropArrayCount();
  for (size_t i = 0; i < numProps; ++i)
  {
    vtkProp *prop = s->GetPropArray()[i];
    vtkInformation *info = prop->GetPropertyKeys();
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
void vtkOpenGLRenderPass::PostRender(const vtkRenderState *s)
{
  assert("Render state valid." && s);
  size_t numProps = s->GetPropArrayCount();
  for (size_t i = 0; i < numProps; ++i)
  {
    vtkProp *prop = s->GetPropArray()[i];
    vtkInformation *info = prop->GetPropertyKeys();
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
