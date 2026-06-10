// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkANARIMaterialLibrary.h"

#include "vtkJPEGReader.h"
#include "vtkObjectFactory.h"
#include "vtkPNGReader.h"
#include "vtkTexture.h"
#include "vtkXMLImageDataReader.h"

#include "vtksys/SystemTools.hxx"

#include <iostream>
#include <string>

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
namespace
{
bool CaseInsensitiveFind(
  const std::map<std::string, vtkANARIMaterialLibrary::ParameterType>& params,
  const std::string& varname)
{
  for (const auto& param : params)
  {
    if (param.first.size() == varname.size() &&
      std::equal(param.first.begin(), param.first.end(), varname.begin(),
        [](unsigned char a, unsigned char b) { return std::tolower(a) == std::tolower(b); }))
    {
      return true;
    }
  }
  return false;
}
} // namespace

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
    if (CaseInsensitiveFind(params, varname))
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
    if (CaseInsensitiveFind(params, varname))
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

//------------------------------------------------------------------------------
bool vtkANARIMaterialLibrary::ReadTextureFileOrData(const std::string& texFilenameOrData,
  bool fromfile, const std::string& parentDir, vtkTexture* textr, std::string& textureName,
  std::string& textureFilename)
{
  if (!textr)
  {
    vtkErrorMacro("You must initialize the resulting texture before calling ReadTextureFileOrData");
    return false;
  }

  textureName = "unnamedTexture";
  textureFilename = "";
  if (texFilenameOrData.rfind("<?xml", 0) == 0)
  {
    // The data starts with an xml tag, so try to read it with a XMLImageDataReader
    textureName = "rawDataTexture";
    vtkNew<vtkXMLImageDataReader> reader;
    reader->ReadFromInputStringOn();
    reader->SetInputString(texFilenameOrData);
    textr->SetInputConnection(reader->GetOutputPort(0));
  }
  else if (fromfile || !texFilenameOrData.empty())
  {
    // Try to load as a file path (either explicitly or as a fallback when deserializing)
    textureFilename = texFilenameOrData;
    // try the texFilenameOrData as an absolute path
    if (!vtksys::SystemTools::FileExists(textureFilename.c_str(), true))
    {
      // Not found, try as a relative path from the current directory or parent directory
      std::string relPath =
        parentDir.empty() ? texFilenameOrData : parentDir + "/" + texFilenameOrData;
      if (vtksys::SystemTools::FileExists(relPath.c_str(), true))
      {
        textureFilename = relPath;
      }
      else if (!vtksys::SystemTools::FileExists(textureFilename.c_str(), true))
      {
        vtkWarningMacro("No such texture file " << texFilenameOrData << " (absolute path), nor "
                                                << relPath << " (relative path) skipping");
        return false;
      }
    }
    textureName = this->FilePathToTextureName(textureFilename);
    if (textureFilename.substr(textureFilename.length() - 3) == "png")
    {
      vtkNew<vtkPNGReader> pngReader;
      pngReader->SetFileName(textureFilename.c_str());
      pngReader->Update();
      textr->SetInputConnection(pngReader->GetOutputPort(0));
    }
    else
    {
      vtkNew<vtkJPEGReader> jpgReader;
      jpgReader->SetFileName(textureFilename.c_str());
      jpgReader->Update();
      textr->SetInputConnection(jpgReader->GetOutputPort(0));
    }
  }
  else
  {
    vtkErrorMacro(
      "Unable to read the texture as XML data nor a file for texture " << texFilenameOrData);
    return false;
  }
  textr->Update();
  return true;
}

VTK_ABI_NAMESPACE_END
