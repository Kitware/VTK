// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOSPRayMaterialLibrary.h"

#include "vtkJPEGReader.h"
#include "vtkObjectFactory.h"
#include "vtkPNGReader.h"
#include "vtkTexture.h"
#include "vtkXMLImageDataReader.h"

#include "vtk_jsoncpp.h"
#include "vtksys/SystemTools.hxx"

#include <string>

VTK_ABI_NAMESPACE_BEGIN

// OSPRay parameter name aliases: maps from alias to canonical name per material type
const std::map<std::string, std::map<std::string, std::string>> Aliases = {
  { "obj",
    { { "colorMap", "map_kd" }, { "map_Kd", "map_kd" }, { "map_Ks", "map_ks" },
      { "map_Ns", "map_ns" }, { "map_Bump", "map_bump" }, { "normalMap", "map_bump" },
      { "BumpMap", "map_bump" }, { "color", "kd" }, { "Kd", "kd" }, { "alpha", "d" },
      { "Ks", "ks" }, { "ns", "Ns" }, { "tf", "Tf" } } },
  { "thinGlass", { { "color", "attenuationColor" }, { "transmission", "attenuationColor" } } },
  { "metallicPaint", { { "color", "baseColor" } } },
  { "glass",
    { { "etaInside", "eta" }, { "etaOutside", "eta" },
      { "attenuationColorOutside", "attenuationColor" } } },
  { "principled", {} }, { "carPaint", {} }, { "metal", {} }, { "alloy", {} }, { "luminous", {} }
};

std::string FindRealName(const std::string& materialType, const std::string& alias)
{
  auto matAliasesIt = ::Aliases.find(materialType);
  if (matAliasesIt != ::Aliases.end())
  {
    auto realNameIt = matAliasesIt->second.find(alias);
    if (realNameIt != matAliasesIt->second.end())
    {
      return realNameIt->second;
    }
  }
  // correct texture new names
  size_t len = alias.length();
  if (len > 3)
  {
    std::string suffix = alias.substr(len - 3, 3);
    if (suffix == "Map")
    {
      std::string correctName = "map_";
      correctName += alias.substr(0, len - 3);
      return correctName;
    }
  }
  return alias;
}

// backward compatibility over OSPRay 2.0 name changes
void BackwardCompatibilityName(std::string& implname)
{
  implname[0] = std::tolower(implname[0]);
  if (implname == "oBJMaterial")
  {
    implname = "obj";
  }
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkOSPRayMaterialLibrary);

//------------------------------------------------------------------------------
void vtkOSPRayMaterialLibrary::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkOSPRayMaterialLibrary::AddMaterial(const std::string& nickname, const std::string& implname)
{
  auto& dic = vtkOSPRayMaterialLibrary::GetParametersDictionary();

  std::string resolvedImplName = implname;
  ::BackwardCompatibilityName(resolvedImplName);

  if (dic.find(resolvedImplName) != dic.end())
  {
    this->Superclass::AddMaterial(nickname, resolvedImplName);
  }
  else
  {
    vtkGenericWarningMacro(
      "Unknown material type \"" << implname << "\" for material named \"" << nickname << "\"");
  }
}

//------------------------------------------------------------------------------
void vtkOSPRayMaterialLibrary::AddTexture(const std::string& nickname, const std::string& varname,
  vtkTexture* tex, const std::string& texname, const std::string& filename)
{
  std::string implname = this->InternalGetImplName(nickname);
  std::string realname = ::FindRealName(implname, varname);

  auto& dic = vtkOSPRayMaterialLibrary::GetParametersDictionary();
  auto dicIt = dic.find(implname);
  if (dicIt != dic.end())
  {
    auto& params = dicIt->second;
    if (params.find(realname) != params.end())
    {
      this->Superclass::AddTexture(nickname, realname, tex, texname, filename);
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
    this->Superclass::AddTexture(nickname, realname, tex, texname, filename);
  }
}

//------------------------------------------------------------------------------
void vtkOSPRayMaterialLibrary::AddShaderVariable(
  const std::string& nickname, const std::string& varname, int numVars, const double* x)
{
  std::string implname = this->InternalGetImplName(nickname);
  std::string realname = ::FindRealName(implname, varname);

  auto& dic = vtkOSPRayMaterialLibrary::GetParametersDictionary();
  auto dicIt = dic.find(implname);
  if (dicIt != dic.end())
  {
    auto& params = dicIt->second;
    if (params.find(realname) != params.end())
    {
      this->Superclass::AddShaderVariable(nickname, realname, numVars, x);
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
    this->Superclass::AddShaderVariable(nickname, realname, numVars, x);
  }
}

//------------------------------------------------------------------------------
bool vtkOSPRayMaterialLibrary::InternalParseJSON(
  const char* filename, bool fromfile, std::istream* doc)
{
  // Parse JSON with backward compatibility for OSPRay material type names
  // We do custom parsing to apply BackwardCompatibilityName transformation

  Json::Value root;
  std::string errs;
  Json::CharReaderBuilder jreader;
  bool ok = Json::parseFromStream(jreader, *doc, &root, &errs);
  if (!ok)
  {
    vtkErrorMacro("JSON parsing error: " << errs);
    return false;
  }

  std::string parentDir = vtksys::SystemTools::GetParentDirectory(filename);

  if (!root.isMember("family"))
  {
    vtkErrorMacro("Not a materials file. Must have \"family\"=\"...\" entry.");
    return false;
  }

  const char* acceptedFamily = this->GetAcceptedFamilyName();
  if (acceptedFamily != nullptr)
  {
    const auto& family = root["family"];
    if (family.asString() != acceptedFamily)
    {
      vtkErrorMacro("Unsupported materials file. Family is not \"" << acceptedFamily << "\" (got \""
                                                                   << family.asString() << "\")");
      return false;
    }
  }

  if (!root.isMember("version"))
  {
    vtkErrorMacro("Not a materials file. Must have \"version\"=\"...\" entry.");
    return false;
  }
  if (!root.isMember("materials"))
  {
    vtkErrorMacro("Not a materials file. Must have \"materials\"={...} entry.");
    return false;
  }

  const auto& materials = root["materials"];
  std::vector<std::string> ikeys = materials.getMemberNames();
  for (size_t i = 0; i < ikeys.size(); ++i)
  {
    const std::string& nickname = ikeys[i];
    const auto& nextmat = materials[nickname];
    if (!nextmat.isMember("type"))
    {
      vtkErrorMacro(
        "Invalid material " << nickname << " must have \"type\"=\"...\" entry, ignoring.");
      continue;
    }

    std::string implname = nextmat["type"].asString();
    // Apply backward compatibility transformation
    ::BackwardCompatibilityName(implname);

    // Use AddMaterial which validates against OSPRay dictionary
    this->AddMaterial(nickname, implname);

    if (nextmat.isMember("textures"))
    {
      const auto& textures = nextmat["textures"];
      for (const std::string& vname : textures.getMemberNames())
      {
        const auto& nexttext = textures[vname];
        vtkNew<vtkTexture> textr;
        std::string textureName, textureFilename;
        if (!this->ReadTextureFileOrData(
              nexttext.asString(), fromfile, parentDir, textr, textureName, textureFilename))
        {
          continue;
        }
        this->AddTexture(nickname, vname, textr, textureName, textureFilename);
      }
    }
    if (nextmat.isMember("doubles"))
    {
      const auto& doubles = nextmat["doubles"];
      for (const std::string& vname : doubles.getMemberNames())
      {
        const auto& nexttext = doubles[vname];
        std::vector<double> vals(nexttext.size());
        for (size_t k = 0; k < nexttext.size(); ++k)
        {
          const auto& nv = nexttext[static_cast<int>(k)];
          vals[k] = nv.asDouble();
        }
        this->AddShaderVariable(nickname, vname, nexttext.size(), vals.data());
      }
    }
  }

  return true;
}

//------------------------------------------------------------------------------
std::vector<double> vtkOSPRayMaterialLibrary::GetDoubleShaderVariable(
  const std::string& nickname, const std::string& varname)
{
  std::string implname = this->InternalGetImplName(nickname);
  std::string realname = ::FindRealName(implname, varname);
  return this->Superclass::GetDoubleShaderVariable(nickname, realname);
}

//------------------------------------------------------------------------------
const TextureInfo* vtkOSPRayMaterialLibrary::GetTextureInfo(
  const std::string& nickname, const std::string& varname)
{
  std::string implname = this->InternalGetImplName(nickname);
  std::string realname = ::FindRealName(implname, varname);
  return this->Superclass::GetTextureInfo(nickname, realname);
}

//------------------------------------------------------------------------------
const std::map<std::string, vtkOSPRayMaterialLibrary::ParametersMap>&
vtkOSPRayMaterialLibrary::GetParametersDictionary()
{
  // This is the material dictionary from OSPRay 1.8
  // If attribute name changes with new OSPRay version, keep old name aliases support in functions
  // vtkOSPRayMaterialLibrary::AddShaderVariable and vtkOSPRayMaterialLibrary::AddTexture
  static std::map<std::string, vtkOSPRayMaterialLibrary::ParametersMap> dic = {
    { "obj",
      {
        { "ka", vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "kd", vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "ks", vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "ns", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "d", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "tf", vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "map_bump", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_bump.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_bump.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_bump.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_bump.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_kd", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_kd.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_kd.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_kd.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_kd.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_ks", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_ks.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_ks.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_ks.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_ks.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_ns", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_ns.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_ns.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_ns.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_ns.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_d", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_d.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_d.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_d.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_d.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
      } },
    { "principled",
      {
        { "baseColor", vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "edgeColor", vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "metallic", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "diffuse", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "specular", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "ior", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "transmission", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "transmissionColor", vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "transmissionDepth", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "roughness", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "anisotropy", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "rotation", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "normal", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "baseNormal", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "thin", vtkOSPRayMaterialLibrary::ParameterType::BOOLEAN },
        { "thickness", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "backlight", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coat", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "coatIor", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatColor", vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "coatThickness", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatRoughness", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "coatNormal", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "sheen", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "sheenColor", vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "sheenTint", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "sheenRoughness", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "opacity", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "map_baseColor", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_baseColor.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_baseColor.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_baseColor.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_baseColor.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_edgeColor", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_edgeColor.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_edgeColor.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_edgeColor.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_edgeColor.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_metallic", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_metallic.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_metallic.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_metallic.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_metallic.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_diffuse", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_diffuse.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_diffuse.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_diffuse.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_diffuse.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_specular", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_specular.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_specular.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_specular.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_specular.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_ior", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_ior.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_ior.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_ior.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_ior.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_transmission", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_transmission.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_transmission.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_transmission.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_transmission.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_transmissionColor", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_transmissionColor.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_transmissionColor.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_transmissionColor.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_transmissionColor.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_transmissionDepth", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_transmissionDepth.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_transmissionDepth.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_transmissionDepth.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_transmissionDepth.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_roughness", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_roughness.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_roughness.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_roughness.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_roughness.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_anisotropy", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_anisotropy.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_anisotropy.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_anisotropy.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_anisotropy.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_rotation", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_rotation.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_rotation.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_rotation.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_rotation.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_normal", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_normal.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_normal.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_normal.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_normal.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_baseNormal", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_baseNormal.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_baseNormal.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_baseNormal.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_baseNormal.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_thin", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_thin.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_thin.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_thin.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_thin.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_thickness", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_thickness.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_thickness.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_thickness.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_thickness.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_backlight", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_backlight.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_backlight.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_backlight.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_backlight.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_coat", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_coat.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_coat.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_coat.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_coat.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_coatIor", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_coatIor.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_coatIor.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_coatIor.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_coatIor.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_coatColor", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_coatColor.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_coatColor.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_coatColor.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_coatColor.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_coatThickness", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_coatThickness.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_coatThickness.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_coatThickness.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_coatThickness.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_coatRoughness", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_coatRoughness.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_coatRoughness.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_coatRoughness.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_coatRoughness.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_coatNormal", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_coatNormal.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_coatNormal.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_coatNormal.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_coatNormal.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_sheen", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_sheen.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_sheen.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_sheen.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_sheen.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_sheenColor", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_sheenColor.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_sheenColor.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_sheenColor.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_sheenColor.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_sheenTint", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_sheenTint.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_sheenTint.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_sheenTint.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_sheenTint.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_sheenRoughness", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_sheenRoughness.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_sheenRoughness.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_sheenRoughness.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_sheenRoughness.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_opacity", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_opacity.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_opacity.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_opacity.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_opacity.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
      } },
    { "carPaint",
      {
        { "baseColor", vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "roughness", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "normal", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "flakeDensity", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "flakeScale", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "flakeSpread", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "flakeJitter", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "flakeRoughness", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "coat", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "coatIor", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatColor", vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "coatThickness", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatRoughness", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "coatNormal", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "flipflopColor", vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "flipflopFalloff", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_baseColor", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_baseColor.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_baseColor.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_baseColor.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_baseColor.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_roughness", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_roughness.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_roughness.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_roughness.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_roughness.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_normal", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_normal.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_normal.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_normal.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_normal.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_flakeDensity", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_flakeDensity.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_flakeDensity.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_flakeDensity.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_flakeDensity.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_flakeScale", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_flakeScale.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_flakeScale.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_flakeScale.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_flakeScale.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_flakeSpread", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_flakeSpread.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_flakeSpread.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_flakeSpread.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_flakeSpread.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_flakeJitter", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_flakeJitter.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_flakeJitter.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_flakeJitter.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_flakeJitter.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_flakeRoughness", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_flakeRoughness.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_flakeRoughness.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_flakeRoughness.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_flakeRoughness.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_coat", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_coat.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_coat.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_coat.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_coat.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_coatIor", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_coatIor.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_coatIor.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_coatIor.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_coatIor.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_coatColor", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_coatColor.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_coatColor.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_coatColor.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_coatColor.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_coatThickness", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_coatThickness.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_coatThickness.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_coatThickness.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_coatThickness.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_coatRoughness", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_coatRoughness.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_coatRoughness.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_coatRoughness.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_coatRoughness.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_coatNormal", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_coatNormal.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_coatNormal.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_coatNormal.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_coatNormal.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_flipflopColor", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_flipflopColor.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_flipflopColor.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_flipflopColor.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_flipflopColor.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_flipflopFalloff", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_flipflopFalloff.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_flipflopFalloff.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_flipflopFalloff.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_flipflopFalloff.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
      } },
    { "metal",
      {
        { "ior", vtkOSPRayMaterialLibrary::ParameterType::FLOAT_DATA },
        { "eta", vtkOSPRayMaterialLibrary::ParameterType::VEC3 },
        { "k", vtkOSPRayMaterialLibrary::ParameterType::VEC3 },
        { "roughness", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "map_roughness", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_roughness.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_roughness.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_roughness.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_roughness.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
      } },
    { "alloy",
      {
        { "color", vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "edgeColor", vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "roughness", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "map_color", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_color.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_color.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_color.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_color.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_edgeColor", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_edgeColor.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_edgeColor.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_edgeColor.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_edgeColor.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_roughness", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_roughness.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_roughness.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_roughness.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_roughness.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
      } },
    { "glass",
      {
        { "eta", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "attenuationColor", vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "attenuationDistance", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
      } },
    { "thinGlass",
      {
        { "eta", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "attenuationColor", vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "attenuationDistance", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "thickness", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_attenuationColor", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_attenuationColor.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_attenuationColor.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_attenuationColor.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_attenuationColor.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
      } },
    { "metallicPaint",
      {
        { "baseColor", vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "flakeAmount", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "flakeColor", vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "flakeSpread", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "eta", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_baseColor", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_baseColor.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_baseColor.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_baseColor.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_baseColor.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
      } },
    { "luminous",
      {
        { "color", vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "intensity", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "transparency", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
      } },
  };
  return dic;
}

//------------------------------------------------------------------------------
bool vtkOSPRayMaterialLibrary::ReadTextureFileOrData(const std::string& texFilenameOrData,
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
  else if (fromfile)
  {
    textureFilename = texFilenameOrData;
    // try the texFilenameOrData as an absolute path
    if (!vtksys::SystemTools::FileExists(textureFilename.c_str(), true))
    {
      // Not found, try as a relative path from the current directory
      textureFilename = parentDir + "/" + texFilenameOrData;
      if (!vtksys::SystemTools::FileExists(textureFilename.c_str(), true))
      {
        vtkWarningMacro("No such texture file " << texFilenameOrData << "(absolute path), nor "
                                                << textureFilename << "(relative path) skipping");
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
