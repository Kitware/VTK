// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGLSLModifierFactory.h"
#include "vtkGLSLModifierBase.h"
#include "vtkLogger.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkGLSLModifierBase* vtkGLSLModifierFactory::CreateAMod(const std::string& modName)
{
  const auto& table = vtkGLSLModifierFactory::GetInstance().ModTable;
  auto it = table.find(modName);
  if (it == table.end())
  {
    vtkLog(ERROR, << "Unable to create GLSLModifier for \'" << modName
                  << "\'. Did you register the mod?");
    return nullptr;
  }
  else
  {
    return it->second.F(it->second.UserData);
  }
}

//------------------------------------------------------------------------------
void vtkGLSLModifierFactory::RegisterAMod(
  const std::string& modName, NewModFunction createFunction, void* userData)
{
  auto& table = vtkGLSLModifierFactory::GetInstance().ModTable;
  ModCreator createData = {};
  createData.UserData = userData;
  createData.F = createFunction;
  table.insert(std::make_pair(modName, createData));
}

//------------------------------------------------------------------------------
vtkGLSLModifierFactory& vtkGLSLModifierFactory::GetInstance()
{
  thread_local vtkGLSLModifierFactory instance;
  return instance;
}

VTK_ABI_NAMESPACE_END
