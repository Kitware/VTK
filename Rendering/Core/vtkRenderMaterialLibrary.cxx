// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkRenderMaterialLibrary.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStringScanner.h"
#include "vtkTexture.h"

#include "vtk_jsoncpp.h"
#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

#include <cctype>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include <sys/types.h>

VTK_ABI_NAMESPACE_BEGIN

struct TextureInfo
{
  std::string Name;
  vtkSmartPointer<vtkTexture> Texture;
  std::string Filename;
};

typedef std::map<std::string, std::vector<double>> NamedVariables;
// Map ShaderVariableName -> TextureInfo
typedef std::map<std::string, TextureInfo> NamedTextures;

class vtkRenderMaterialLibraryInternals
{
public:
  vtkRenderMaterialLibraryInternals() = default;
  ~vtkRenderMaterialLibraryInternals() = default;

  std::set<std::string> NickNames;
  std::map<std::string, std::string> ImplNames;
  std::map<std::string, NamedVariables> VariablesFor;
  std::map<std::string, NamedTextures> TexturesFor;
};

//------------------------------------------------------------------------------
namespace
{
std::string FilePathToTextureName(const std::string& path)
{
  std::string res = vtksys::SystemTools::GetFilenameName(path);
  std::size_t dot = res.find_last_of('.');
  return (dot == std::string::npos) ? res : std::string(res.begin(), res.begin() + dot);
}

static std::string trim(std::string s)
{
  size_t start = 0;
  while ((start < s.length()) && (isspace(s[start])))
  {
    start++;
  }
  size_t end = s.length();
  while ((end > start) && (isspace(s[end - 1])))
  {
    end--;
  }
  return s.substr(start, end - start);
}
} // namespace

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkRenderMaterialLibrary);

//------------------------------------------------------------------------------
vtkRenderMaterialLibrary::vtkRenderMaterialLibrary()
{
  this->Internal = new vtkRenderMaterialLibraryInternals;
}

//------------------------------------------------------------------------------
vtkRenderMaterialLibrary::~vtkRenderMaterialLibrary()
{
  delete this->Internal;
}

//------------------------------------------------------------------------------
void vtkRenderMaterialLibrary::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Materials:\n";
  for (auto mat : this->Internal->NickNames)
  {
    os << indent << "  - " << mat << "( " << this->Internal->ImplNames[mat] << " )" << endl;
    for (auto v : this->Internal->VariablesFor[mat])
    {
      os << indent << "    - " << v.first << endl;
    }
  }
}

//------------------------------------------------------------------------------
void vtkRenderMaterialLibrary::AddMaterial(const std::string& nickname, const std::string& implname)
{
  this->Internal->NickNames.insert(nickname);
  this->Internal->ImplNames[nickname] = implname;
}

//------------------------------------------------------------------------------
void vtkRenderMaterialLibrary::RemoveMaterial(const std::string& nickname)
{
  this->Internal->NickNames.erase(nickname);
  this->Internal->ImplNames.erase(nickname);
  this->Internal->VariablesFor.erase(nickname);
  this->Internal->TexturesFor.erase(nickname);
}

//------------------------------------------------------------------------------
void vtkRenderMaterialLibrary::AddTexture(const std::string& nickname, const std::string& varname,
  vtkTexture* tex, const std::string& texname, const std::string& filename)
{
  NamedTextures& tsForNickname = this->Internal->TexturesFor[nickname];
  tsForNickname[varname] = { texname, tex, filename };
}

//------------------------------------------------------------------------------
void vtkRenderMaterialLibrary::RemoveTexture(
  const std::string& nickname, const std::string& varname)
{
  this->Internal->TexturesFor[nickname].erase(varname);
}

//------------------------------------------------------------------------------
void vtkRenderMaterialLibrary::RemoveAllTextures(const std::string& nickname)
{
  this->Internal->TexturesFor[nickname].clear();
}

//------------------------------------------------------------------------------
void vtkRenderMaterialLibrary::AddShaderVariable(
  const std::string& nickname, const std::string& varname, int numVars, const double* x)
{
  std::vector<double> w;
  w.assign(x, x + numVars);
  NamedVariables& vsForNickname = this->Internal->VariablesFor[nickname];
  vsForNickname[varname] = std::move(w);
}

//------------------------------------------------------------------------------
void vtkRenderMaterialLibrary::RemoveShaderVariable(
  const std::string& nickname, const std::string& varname)
{
  this->Internal->VariablesFor[nickname].erase(varname);
}

//------------------------------------------------------------------------------
void vtkRenderMaterialLibrary::RemoveAllShaderVariables(const std::string& nickname)
{
  this->Internal->VariablesFor[nickname].clear();
}

//------------------------------------------------------------------------------
bool vtkRenderMaterialLibrary::ReadFile(const char* filename)
{
  return this->InternalParse(filename, true);
}

//------------------------------------------------------------------------------
bool vtkRenderMaterialLibrary::ReadBuffer(const char* filename)
{
  return this->InternalParse(filename, false);
}

//------------------------------------------------------------------------------
bool vtkRenderMaterialLibrary::InternalParse(const char* filename, bool fromfile)
{
  if (!filename)
  {
    return false;
  }
  if (fromfile && !vtksys::SystemTools::FileExists(filename, true))
  {
    return false;
  }

  std::istream* doc;
  if (fromfile)
  {
    doc = new vtksys::ifstream(filename, std::ios::binary);
  }
  else
  {
    doc = new std::istringstream(filename);
  }
  bool retOK = false;
  if (std::string(filename).rfind(".mtl") != std::string::npos)
  {
    retOK = this->InternalParseMTL(filename, fromfile, doc);
  }
  else
  {
    retOK = this->InternalParseJSON(filename, fromfile, doc);
  }
  delete doc;
  return retOK;
}

//------------------------------------------------------------------------------
bool vtkRenderMaterialLibrary::InternalParseJSON(
  const char* filename, bool fromfile, std::istream* doc)
{
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
    const Json::Value family = root["family"];
    if (family.asString() != acceptedFamily)
    {
      vtkErrorMacro("Unsupported materials file. Family is not \"" << acceptedFamily << "\" (got \""
                                                                   << family.asString() << "\").");
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

  const Json::Value materials = root["materials"];
  std::vector<std::string> ikeys = materials.getMemberNames();
  for (size_t i = 0; i < ikeys.size(); ++i)
  {
    const std::string& nickname = ikeys[i];
    const Json::Value nextmat = materials[nickname];
    if (!nextmat.isMember("type"))
    {
      vtkErrorMacro(
        "Invalid material " << nickname << " must have \"type\"=\"...\" entry, ignoring.");
      continue;
    }

    // keep a record so others know this material is available
    this->Internal->NickNames.insert(nickname);

    std::string implname = nextmat["type"].asString();
    this->Internal->ImplNames[nickname] = implname;
    if (nextmat.isMember("textures"))
    {
      const Json::Value textures = nextmat["textures"];
      for (const std::string& vname : textures.getMemberNames())
      {
        const Json::Value nexttext = textures[vname];
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
      const Json::Value doubles = nextmat["doubles"];
      for (const std::string& vname : doubles.getMemberNames())
      {
        const Json::Value nexttext = doubles[vname];
        std::vector<double> vals(nexttext.size());
        for (size_t k = 0; k < nexttext.size(); ++k)
        {
          const Json::Value nv = nexttext[static_cast<int>(k)];
          vals[k] = nv.asDouble();
        }
        this->AddShaderVariable(nickname, vname, nexttext.size(), vals.data());
      }
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkRenderMaterialLibrary::InternalParseMTL(
  const char* filename, bool fromfile, std::istream* doc)
{
  std::string str;
  std::string nickname = "";
  std::string parentDir = vtksys::SystemTools::GetParentDirectory(filename);

  const std::vector<std::string> singles{ "d ", "Ks ", "alpha ", "roughness ", "eta ",
    "thickness " };
  const std::vector<std::string> triples{ "Ka ", "color ", "Kd ", "Ks " };
  const std::vector<std::string> textures{ "map_d ", "map_Kd ", "map_kd ", "colorMap ", "map_Ks ",
    "map_ks ", "map_Ns ", "map_ns ", "map_Bump", "map_bump", "normalMap", "bumpMap" };

  while (getline(*doc, str))
  {
    std::string tstr = trim(str);
    std::string lkey;

    // a new material
    lkey = "newmtl ";
    if (tstr.compare(0, lkey.size(), lkey) == 0)
    {
      nickname = trim(tstr.substr(lkey.size()));
      this->Internal->NickNames.insert(nickname);
      this->Internal->ImplNames[nickname] = "obj";
    }

    // type of the material, if not obj
    lkey = "type ";
    if (tstr.compare(0, lkey.size(), lkey) == 0)
    {
      std::string implname = trim(tstr.substr(lkey.size()));
      this->Internal->ImplNames[nickname] = implname;
    }

    // grab all the single valued settings we see
    std::vector<std::string>::const_iterator sit1 = singles.begin();
    while (sit1 != singles.end())
    {
      std::string key = *sit1;
      ++sit1;
      if (tstr.compare(0, key.size(), key) == 0)
      {
        std::string v = tstr.substr(key.size());
        double dv = 0.;
        auto result = vtk::from_chars(v, dv);
        if (result.ec == std::errc())
        {
          double vals[1] = { dv };
          this->AddShaderVariable(nickname, key.substr(0, key.size() - 1).c_str(), 1, vals);
        }
      }
    }

    // grab all the triple valued settings we see
    std::vector<std::string>::const_iterator sit3 = triples.begin();
    while (sit3 != triples.end())
    {
      std::string key = *sit3;
      ++sit3;
      if (tstr.compare(0, key.size(), key) == 0)
      {
        std::string vs = tstr.substr(key.size());
        size_t loc1 = vs.find(" ");
        size_t loc2 = vs.find(" ", loc1 + 1);
        std::string v1 = vs.substr(0, loc1);
        std::string v2 = vs.substr(loc1 + 1, loc2);
        std::string v3 = vs.substr(loc2 + 1);
        double d1 = 0;
        double d2 = 0;
        double d3 = 0;
        auto result1 = vtk::from_chars(v1, d1);
        auto result2 = vtk::from_chars(v2, d2);
        auto result3 = vtk::from_chars(v3, d3);
        if (result1.ec == std::errc() && result2.ec == std::errc() && result3.ec == std::errc())
        {
          double vals[3] = { d1, d2, d3 };
          this->AddShaderVariable(nickname, key.substr(0, key.size() - 1).c_str(), 3, vals);
        }
      }
    }

    // grab all the textures we see
    std::vector<std::string>::const_iterator tit = textures.begin();
    while (tit != textures.end())
    {
      std::string key = *tit;
      ++tit;

      std::string tfname = "";
      if (tstr.compare(0, key.size(), key) == 0)
      {
        tfname = trim(tstr.substr(key.size()));
      }
      if (!tfname.empty())
      {
        vtkNew<vtkTexture> textr;
        std::string textureName, textureFilename;
        if (!this->ReadTextureFileOrData(
              tfname, fromfile, parentDir, textr, textureName, textureFilename))
        {
          continue;
        }
        this->AddTexture(
          nickname, key.substr(0, key.size() - 1).c_str(), textr, textureName, textureFilename);
      }
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkRenderMaterialLibrary::ReadTextureFileOrData(const std::string& texFilenameOrData,
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
    vtkErrorMacro(
      "XML data loading requires IOImage module support. Subclass must implement this.");
    return false;
  }
  else if (fromfile)
  {
    textureFilename = texFilenameOrData;
    vtkErrorMacro(
      "File texture loading requires IOImage module support. Subclass must implement this.");
    return false;
  }
  else
  {
    vtkErrorMacro(
      "Unable to read the texture as XML data nor a file for texture " << texFilenameOrData);
    return false;
  }
}

//------------------------------------------------------------------------------
const char* vtkRenderMaterialLibrary::WriteBuffer(bool writeImageInline)
{
  Json::Value root;
  root["family"] = this->GetFamilyName();
  root["version"] = "0.0";
  Json::Value materials;

  std::set<std::string>::iterator it = this->Internal->NickNames.begin();
  while (it != this->Internal->NickNames.end())
  {
    std::string nickname = *it;
    Json::Value jnickname;
    std::string implname = this->LookupImplName(nickname);
    jnickname["type"] = implname;

    if (this->Internal->VariablesFor.find(nickname) != this->Internal->VariablesFor.end())
    {
      Json::Value variables;
      NamedVariables::iterator vit = this->Internal->VariablesFor[nickname].begin();
      while (vit != this->Internal->VariablesFor[nickname].end())
      {
        std::string vname = vit->first;
        std::vector<double> vvals = vit->second;
        Json::Value jvvals;
        for (size_t i = 0; i < vvals.size(); ++i)
        {
          jvvals.append(vvals[i]);
        }
        variables[vname] = jvvals;
        ++vit;
      }

      jnickname["doubles"] = variables;
    }

    if (this->Internal->TexturesFor.find(nickname) != this->Internal->TexturesFor.end())
    {
      Json::Value textures;
      NamedTextures::iterator vit = this->Internal->TexturesFor[nickname].begin();
      while (vit != this->Internal->TexturesFor[nickname].end())
      {
        std::string vname = vit->first;
        const TextureInfo& texInfo = vit->second;
        const std::string& filename = texInfo.Filename;

        std::string os = filename;
        if (os.empty() && (writeImageInline))
        {
          vtkWarningMacro("Unable to serialize texture data inline. Subclass must implement inline "
                          "serialization.");
        }

        Json::Value jvvals = os;
        textures[vname] = jvvals;
        ++vit;
      }

      jnickname["textures"] = textures;
    }

    materials[nickname] = jnickname;
    ++it;
  }

  root["materials"] = materials;

  Json::StreamWriterBuilder builder;
  builder["commentStyle"] = "None";
  builder["indentation"] = "   ";
  std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
  std::ostringstream result;
  writer->write(root, &result);

  std::string rstring = result.str();
  if (!rstring.empty())
  {
    char* buf = new char[rstring.size() + 1];
    memcpy(buf, rstring.c_str(), rstring.size());
    buf[rstring.size()] = 0;
    return buf;
  }
  return nullptr;
}

//------------------------------------------------------------------------------
void vtkRenderMaterialLibrary::WriteFile(const std::string& filename, bool writeImageInline)
{
  const char* rchar = this->WriteBuffer(writeImageInline);
  std::string rstring = rchar;
  delete[] rchar;

  if (!rstring.empty())
  {
    vtksys::ofstream fileStream(filename.c_str(), std::ios::out | std::ios::trunc);
    fileStream << rstring;
    fileStream.close();
  }
}

//------------------------------------------------------------------------------
void vtkRenderMaterialLibrary::Fire()
{
  this->InvokeEvent(vtkCommand::UpdateDataEvent);
}

//------------------------------------------------------------------------------
std::set<std::string> vtkRenderMaterialLibrary::GetMaterialNames()
{
  return this->Internal->NickNames;
}

//------------------------------------------------------------------------------
std::string vtkRenderMaterialLibrary::LookupImplName(const std::string& nickname)
{
  return this->Internal->ImplNames[nickname];
}

//------------------------------------------------------------------------------
const TextureInfo* vtkRenderMaterialLibrary::GetTextureInfo(
  const std::string& nickname, const std::string& varname)
{
  if (this->Internal->TexturesFor.find(nickname) != this->Internal->TexturesFor.end())
  {
    NamedTextures& tsForNickname = this->Internal->TexturesFor[nickname];
    auto it = tsForNickname.find(varname);
    if (it != tsForNickname.end())
    {
      return &it->second;
    }
  }
  return nullptr;
}

//------------------------------------------------------------------------------
vtkTexture* vtkRenderMaterialLibrary::GetTexture(
  const std::string& nickname, const std::string& varname)
{
  if (const TextureInfo* texInfo = this->GetTextureInfo(nickname, varname))
  {
    return texInfo->Texture;
  }
  return nullptr;
}

//------------------------------------------------------------------------------
std::string vtkRenderMaterialLibrary::GetTextureName(
  const std::string& nickname, const std::string& varname)
{
  if (const TextureInfo* texInfo = this->GetTextureInfo(nickname, varname))
  {
    return texInfo->Name;
  }
  return "";
}

//------------------------------------------------------------------------------
std::string vtkRenderMaterialLibrary::GetTextureFilename(
  const std::string& nickname, const std::string& varname)
{
  if (const TextureInfo* texInfo = this->GetTextureInfo(nickname, varname))
  {
    return texInfo->Filename;
  }
  return "";
}

//------------------------------------------------------------------------------
std::vector<double> vtkRenderMaterialLibrary::GetDoubleShaderVariable(
  const std::string& nickname, const std::string& varname)
{
  if (this->Internal->VariablesFor.find(nickname) != this->Internal->VariablesFor.end())
  {
    NamedVariables vsForNickname = this->Internal->VariablesFor[nickname];
    auto it = vsForNickname.find(varname);
    if (it != vsForNickname.end())
    {
      return it->second;
    }
  }
  return std::vector<double>();
}

//------------------------------------------------------------------------------
std::vector<std::string> vtkRenderMaterialLibrary::GetDoubleShaderVariableList(
  const std::string& nickname)
{
  std::vector<std::string> variableNames;
  if (this->Internal->VariablesFor.find(nickname) != this->Internal->VariablesFor.end())
  {
    for (auto& v : this->Internal->VariablesFor[nickname])
    {
      variableNames.push_back(v.first);
    }
  }
  return variableNames;
}

//------------------------------------------------------------------------------
std::vector<std::string> vtkRenderMaterialLibrary::GetTextureList(const std::string& nickname)
{
  std::vector<std::string> texNames;
  if (this->Internal->TexturesFor.find(nickname) != this->Internal->TexturesFor.end())
  {
    for (auto& v : this->Internal->TexturesFor[nickname])
    {
      texNames.push_back(v.first);
    }
  }
  return texNames;
}

//------------------------------------------------------------------------------
const std::map<std::string, vtkRenderMaterialLibrary::ParametersMap>&
vtkRenderMaterialLibrary::GetParametersDictionary()
{
  static std::map<std::string, vtkRenderMaterialLibrary::ParametersMap> empty;
  return empty;
}

//------------------------------------------------------------------------------
std::string vtkRenderMaterialLibrary::InternalGetImplName(const std::string& nickname) const
{
  auto it = this->Internal->ImplNames.find(nickname);
  if (it != this->Internal->ImplNames.end())
  {
    return it->second;
  }
  return "";
}

VTK_ABI_NAMESPACE_END
