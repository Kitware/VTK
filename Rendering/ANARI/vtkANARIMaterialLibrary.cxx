// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkANARIMaterialLibrary.h"

#include "vtkObjectFactory.h"
#include "vtkTexture.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkANARIMaterialLibrary);

//------------------------------------------------------------------------------
void vtkANARIMaterialLibrary::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkANARIMaterialLibrary::vtkANARIMaterialLibrary() = default;

//------------------------------------------------------------------------------
vtkANARIMaterialLibrary::~vtkANARIMaterialLibrary() = default;

//------------------------------------------------------------------------------
void vtkANARIMaterialLibrary::AddMaterial(const std::string& nickname, const std::string& implname)
{
  auto& dic = this->GetParametersDictionary();

  if (dic.find(implname) != dic.end())
  {
    this->Superclass::AddMaterial(nickname, implname);
  }
  else
  {
    vtkGenericWarningMacro(
      "Unknown material type \"" << implname << "\" for material named \"" << nickname << "\"");
  }
}

//------------------------------------------------------------------------------
void vtkANARIMaterialLibrary::AddTexture(const std::string& nickname, const std::string& varname,
  vtkTexture* tex, const std::string& texname, const std::string& filename)
{
  std::string implname = this->InternalGetImplName(nickname);

  auto& dic = this->GetParametersDictionary();
  auto dicIt = dic.find(implname);
  if (dicIt != dic.end())
  {
    auto& params = dicIt->second;
    if (params.find(varname) != params.end())
    {
      this->Superclass::AddTexture(nickname, varname, tex, texname, filename);
    }
    else
    {
      vtkGenericWarningMacro(
        "Unknown parameter \"" << varname << "\" for type \"" << implname << "\"");
    }
  }
  else
  {
    // If material type not in dictionary, store without validation
    this->Superclass::AddTexture(nickname, varname, tex, texname, filename);
  }
}

//------------------------------------------------------------------------------
void vtkANARIMaterialLibrary::AddShaderVariable(
  const std::string& nickname, const std::string& varname, int numVars, const double* x)
{
  std::string implname = this->InternalGetImplName(nickname);

  auto& dic = this->GetParametersDictionary();
  auto dicIt = dic.find(implname);
  if (dicIt != dic.end())
  {
    auto& params = dicIt->second;
    if (params.find(varname) != params.end())
    {
      this->Superclass::AddShaderVariable(nickname, varname, numVars, x);
    }
    else
    {
      vtkGenericWarningMacro(
        "Unknown parameter \"" << varname << "\" for type \"" << implname << "\"");
    }
  }
  else
  {
    // If material type not in dictionary, store without validation
    this->Superclass::AddShaderVariable(nickname, varname, numVars, x);
  }
}

//------------------------------------------------------------------------------
const std::map<std::string, vtkANARIMaterialLibrary::ParametersMap>&
vtkANARIMaterialLibrary::GetParametersDictionary()
{
  // ANARI material dictionary based on supported material types
  // ANARI supports physicallyBased and matte materials via KHR extensions
  static std::map<std::string, vtkANARIMaterialLibrary::ParametersMap> dic = {
    { "principled",
      {
        { "baseColor", vtkANARIMaterialLibrary::ParameterType::COLOR_RGB },
        { "metallic", vtkANARIMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "roughness", vtkANARIMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "ior", vtkANARIMaterialLibrary::ParameterType::FLOAT },
        { "transmission", vtkANARIMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "map_baseColor", vtkANARIMaterialLibrary::ParameterType::TEXTURE },
        { "map_metallic", vtkANARIMaterialLibrary::ParameterType::TEXTURE },
        { "map_roughness", vtkANARIMaterialLibrary::ParameterType::TEXTURE },
      } },
    { "obj",
      {
        { "kd", vtkANARIMaterialLibrary::ParameterType::COLOR_RGB },
        { "ks", vtkANARIMaterialLibrary::ParameterType::COLOR_RGB },
        { "ns", vtkANARIMaterialLibrary::ParameterType::FLOAT },
        { "d", vtkANARIMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "map_kd", vtkANARIMaterialLibrary::ParameterType::TEXTURE },
        { "map_ks", vtkANARIMaterialLibrary::ParameterType::TEXTURE },
        { "map_bump", vtkANARIMaterialLibrary::ParameterType::TEXTURE },
      } },
    { "glass",
      {
        { "eta", vtkANARIMaterialLibrary::ParameterType::FLOAT },
        { "attenuationColor", vtkANARIMaterialLibrary::ParameterType::COLOR_RGB },
        { "attenuationDistance", vtkANARIMaterialLibrary::ParameterType::FLOAT },
      } },
    { "thinGlass",
      {
        { "eta", vtkANARIMaterialLibrary::ParameterType::FLOAT },
        { "attenuationColor", vtkANARIMaterialLibrary::ParameterType::COLOR_RGB },
        { "attenuationDistance", vtkANARIMaterialLibrary::ParameterType::FLOAT },
        { "thickness", vtkANARIMaterialLibrary::ParameterType::FLOAT },
      } },
    { "metal",
      {
        { "eta", vtkANARIMaterialLibrary::ParameterType::VEC3 },
        { "k", vtkANARIMaterialLibrary::ParameterType::VEC3 },
        { "roughness", vtkANARIMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "map_roughness", vtkANARIMaterialLibrary::ParameterType::TEXTURE },
      } },
    { "alloy",
      {
        { "color", vtkANARIMaterialLibrary::ParameterType::COLOR_RGB },
        { "edgeColor", vtkANARIMaterialLibrary::ParameterType::COLOR_RGB },
        { "roughness", vtkANARIMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "map_color", vtkANARIMaterialLibrary::ParameterType::TEXTURE },
      } },
    { "matte",
      {
        { "color", vtkANARIMaterialLibrary::ParameterType::COLOR_RGB },
        { "map_color", vtkANARIMaterialLibrary::ParameterType::TEXTURE },
      } },
  };
  return dic;
}

VTK_ABI_NAMESPACE_END
