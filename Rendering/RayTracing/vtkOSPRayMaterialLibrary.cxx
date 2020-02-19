/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayMaterialLibrary.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOSPRayMaterialLibrary.h"

#include "vtkActor.h"
#include "vtkCommand.h"
#include "vtkImageData.h"
#include "vtkJPEGReader.h"
#include "vtkOSPRayMaterialHelpers.h"
#include "vtkObjectFactory.h"
#include "vtkPNGReader.h"
#include "vtkSmartPointer.h"
#include "vtkTexture.h"
#include "vtkXMLImageDataReader.h"
#include "vtkXMLImageDataWriter.h"
#include "vtk_jsoncpp.h"
#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

#include <fstream>
#include <string>
#include <vector>

#include <sys/types.h>

namespace
{
const std::map<std::string, std::map<std::string, std::string> > Aliases = {
  { "OBJMaterial",
    { { "colorMap", "map_Kd" }, { "map_kd", "map_Kd" }, { "map_ks", "map_Ks" },
      { "map_ns", "map_Ns" }, { "map_bump", "map_Bump" }, { "normalMap", "map_Bump" },
      { "BumpMap", "map_Bump" }, { "color", "Kd" }, { "kd", "Kd" }, { "alpha", "d" },
      { "ks", "Ks" }, { "ns", "Ns" }, { "tf", "Tf" } } },
  { "ThinGlass", { { "color", "attenuationColor" }, { "transmission", "attenuationColor" } } },
  { "MetallicPaint", { { "color", "baseColor" } } },
  { "Glass",
    { { "etaInside", "eta" }, { "etaOutside", "eta" },
      { "attenuationColorOutside", "attenuationColor" } } },
  { "Principled", {} }, { "CarPaint", {} }, { "Metal", {} }, { "Alloy", {} }, { "Luminous", {} }
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
  return alias;
}
}

typedef std::map<std::string, std::vector<double> > NamedVariables;
typedef std::map<std::string, vtkSmartPointer<vtkTexture> > NamedTextures;

class vtkOSPRayMaterialLibraryInternals
{
public:
  vtkOSPRayMaterialLibraryInternals() {}
  ~vtkOSPRayMaterialLibraryInternals() {}

  std::set<std::string> NickNames;
  std::map<std::string, std::string> ImplNames;
  std::map<std::string, NamedVariables> VariablesFor;
  std::map<std::string, NamedTextures> TexturesFor;
};

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOSPRayMaterialLibrary);

// ----------------------------------------------------------------------------
vtkOSPRayMaterialLibrary::vtkOSPRayMaterialLibrary()
{
  this->Internal = new vtkOSPRayMaterialLibraryInternals;
}

// ----------------------------------------------------------------------------
vtkOSPRayMaterialLibrary::~vtkOSPRayMaterialLibrary()
{
  delete this->Internal;
}

// ----------------------------------------------------------------------------
void vtkOSPRayMaterialLibrary::PrintSelf(ostream& os, vtkIndent indent)
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

// ----------------------------------------------------------------------------
void vtkOSPRayMaterialLibrary::AddMaterial(const std::string& nickname, const std::string& implname)
{
  auto& dic = vtkOSPRayMaterialLibrary::GetParametersDictionary();

  if (dic.find(implname) != dic.end())
  {
    this->Internal->NickNames.insert(nickname);
    this->Internal->ImplNames[nickname] = implname;
  }
  else
  {
    vtkGenericWarningMacro(
      "Unknown material type \"" << implname << "\" for material named \"" << nickname << "\"");
  }
}

// ----------------------------------------------------------------------------
void vtkOSPRayMaterialLibrary::RemoveMaterial(const std::string& nickname)
{
  this->Internal->NickNames.erase(nickname);
  this->Internal->ImplNames.erase(nickname);
  this->Internal->VariablesFor.erase(nickname);
  this->Internal->TexturesFor.erase(nickname);
}

// ----------------------------------------------------------------------------
void vtkOSPRayMaterialLibrary::AddTexture(
  const std::string& nickname, const std::string& texname, vtkTexture* tex)
{
  std::string realname = ::FindRealName(this->Internal->ImplNames[nickname], texname);

  auto& dic = vtkOSPRayMaterialLibrary::GetParametersDictionary();
  auto& params = dic.at(this->Internal->ImplNames[nickname]);
  if (params.find(realname) != params.end())
  {
    NamedTextures& tsForNickname = this->Internal->TexturesFor[nickname];
    tsForNickname[realname] = tex;
  }
  else
  {
    vtkGenericWarningMacro("Unknown parameter \"" << texname << "\" for type \""
                                                  << this->Internal->ImplNames[nickname] << "\"");
  }
}

// ----------------------------------------------------------------------------
void vtkOSPRayMaterialLibrary::RemoveTexture(
  const std::string& nickname, const std::string& texname)
{
  std::string realname = ::FindRealName(this->Internal->ImplNames[nickname], texname);
  this->Internal->TexturesFor[nickname].erase(realname);
}

// ----------------------------------------------------------------------------
void vtkOSPRayMaterialLibrary::RemoveAllTextures(const std::string& nickname)
{
  this->Internal->TexturesFor[nickname].clear();
}

// ----------------------------------------------------------------------------
void vtkOSPRayMaterialLibrary::AddShaderVariable(
  const std::string& nickname, const std::string& varname, int numVars, const double* x)
{
  std::vector<double> w;
  w.assign(x, x + numVars);

  std::string realname = ::FindRealName(this->Internal->ImplNames[nickname], varname);

  auto& dic = vtkOSPRayMaterialLibrary::GetParametersDictionary();
  auto& params = dic.at(this->Internal->ImplNames[nickname]);
  if (params.find(realname) != params.end())
  {
    NamedVariables& vsForNickname = this->Internal->VariablesFor[nickname];
    vsForNickname[realname] = std::move(w);
  }
  else
  {
    vtkGenericWarningMacro("Unknown parameter \"" << varname << "\" for type \""
                                                  << this->Internal->ImplNames[nickname] << "\"");
  }
}

// ----------------------------------------------------------------------------
void vtkOSPRayMaterialLibrary::RemoveShaderVariable(
  const std::string& nickname, const std::string& varname)
{
  std::string realname = ::FindRealName(this->Internal->ImplNames[nickname], varname);
  this->Internal->VariablesFor[nickname].erase(realname);
}

// ----------------------------------------------------------------------------
void vtkOSPRayMaterialLibrary::RemoveAllShaderVariables(const std::string& nickname)
{
  this->Internal->VariablesFor[nickname].clear();
}

// ----------------------------------------------------------------------------
bool vtkOSPRayMaterialLibrary::ReadFile(const char* filename)
{
  return this->InternalParse(filename, true);
}

// ----------------------------------------------------------------------------
bool vtkOSPRayMaterialLibrary::ReadBuffer(const char* filename)
{
  return this->InternalParse(filename, false);
}

// ----------------------------------------------------------------------------
bool vtkOSPRayMaterialLibrary::InternalParse(const char* filename, bool fromfile)
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

// ----------------------------------------------------------------------------
bool vtkOSPRayMaterialLibrary::InternalParseJSON(
  const char* filename, bool fromfile, std::istream* doc)
{
  Json::Value root;
  std::string errs;
  Json::CharReaderBuilder jreader;
  bool ok = Json::parseFromStream(jreader, *doc, &root, &errs);
  if (!ok)
  {
    return false;
  }
  if (!root.isMember("family"))
  {
    vtkErrorMacro("Not a materials file. Must have \"family\"=\"...\" entry.");
    return false;
  }
  const Json::Value family = root["family"];
  if (family.asString() != "OSPRay")
  {
    vtkErrorMacro("Unsupported materials file. Family is not \"OSPRay\".");
    return false;
  }
  if (!root.isMember("version"))
  {
    vtkErrorMacro("Not a materials file. Must have \"version\"=\"...\" entry.");
    return false;
  }
  const Json::Value version = root["version"];
  if (version.asString() != "0.0")
  {
    vtkErrorMacro("Unsupported materials file. Version is not \"0.0\".");
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

    const std::string& implname = nextmat["type"].asString();
    this->Internal->ImplNames[nickname] = implname;
    if (nextmat.isMember("textures"))
    {
      std::string parentDir = vtksys::SystemTools::GetParentDirectory(filename);
      const Json::Value textures = nextmat["textures"];
      for (const std::string& tname : textures.getMemberNames())
      {
        const Json::Value nexttext = textures[tname];
        const char* tfname = nexttext.asCString();
        vtkSmartPointer<vtkTexture> textr = vtkSmartPointer<vtkTexture>::New();
        vtkSmartPointer<vtkJPEGReader> jpgReader = vtkSmartPointer<vtkJPEGReader>::New();
        vtkSmartPointer<vtkPNGReader> pngReader = vtkSmartPointer<vtkPNGReader>::New();
        if (fromfile)
        {
          std::string tfullname = parentDir + "/" + tfname;
          if (!vtksys::SystemTools::FileExists(tfullname.c_str(), true))
          {
            cerr << "No such texture file " << tfullname << " skipping" << endl;
            continue;
          }
          if (tfullname.substr(tfullname.length() - 3) == "png")
          {
            pngReader->SetFileName(tfullname.c_str());
            pngReader->Update();
            textr->SetInputConnection(pngReader->GetOutputPort(0));
          }
          else
          {
            jpgReader->SetFileName(tfullname.c_str());
            jpgReader->Update();
            textr->SetInputConnection(jpgReader->GetOutputPort(0));
          }
        }
        else
        {
          vtkSmartPointer<vtkXMLImageDataReader> reader =
            vtkSmartPointer<vtkXMLImageDataReader>::New();
          reader->ReadFromInputStringOn();
          reader->SetInputString(tfname);
          textr->SetInputConnection(reader->GetOutputPort(0));
        }
        textr->Update();
        this->AddTexture(nickname, tname, textr);
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

namespace
{
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
}

// ----------------------------------------------------------------------------
bool vtkOSPRayMaterialLibrary::InternalParseMTL(
  const char* filename, bool fromfile, std::istream* doc)
{
  std::string str;
  std::string nickname = "";
  std::string implname = "OBJMaterial";

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
      this->Internal->ImplNames[nickname] = "OBJMaterial";
    }

    // ospray type of the material, if not obj
    lkey = "type ";
    if (tstr.compare(0, lkey.size(), lkey) == 0)
    {
      // this non standard entry is a quick way to break out of
      // objmaterial and use one of the ospray specific materials
      implname = trim(tstr.substr(lkey.size()));
      if (implname == "matte")
      {
        implname = "OBJMaterial";
      }
      if (implname == "glass")
      {
        implname = "ThinGlass";
      }
      if (implname == "metal")
      {
        implname = "Metal";
      }
      if (implname == "metallicPaint")
      {
        implname = "MetallicPaint";
      }
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
        bool OK = false;
        try
        {
          dv = std::stod(v);
          OK = true;
        }
        catch (const std::invalid_argument&)
        {
        }
        catch (const std::out_of_range&)
        {
        }
        if (OK)
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
        bool OK = false;
        try
        {
          d1 = std::stod(v1);
          d2 = std::stod(v2);
          d3 = std::stod(v3);
          OK = true;
        }
        catch (const std::invalid_argument&)
        {
        }
        catch (const std::out_of_range&)
        {
        }
        if (OK)
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
      if (tfname != "")
      {
        vtkSmartPointer<vtkTexture> textr = vtkSmartPointer<vtkTexture>::New();
        vtkSmartPointer<vtkJPEGReader> jpgReader = vtkSmartPointer<vtkJPEGReader>::New();
        vtkSmartPointer<vtkPNGReader> pngReader = vtkSmartPointer<vtkPNGReader>::New();
        if (fromfile)
        {
          std::string parentDir = vtksys::SystemTools::GetParentDirectory(filename);
          std::string tfullname = parentDir + "/" + tfname;
          if (!vtksys::SystemTools::FileExists(tfullname.c_str(), true))
          {
            cerr << "No such texture file " << tfullname << " skipping" << endl;
            continue;
          }
          if (tfullname.substr(tfullname.length() - 3) == "png")
          {
            pngReader->SetFileName(tfullname.c_str());
            pngReader->Update();
            textr->SetInputConnection(pngReader->GetOutputPort(0));
          }
          else
          {
            jpgReader->SetFileName(tfullname.c_str());
            jpgReader->Update();
            textr->SetInputConnection(jpgReader->GetOutputPort(0));
          }
        }
        else
        {
          vtkSmartPointer<vtkXMLImageDataReader> reader =
            vtkSmartPointer<vtkXMLImageDataReader>::New();
          reader->ReadFromInputStringOn();
          reader->SetInputString(tfname);
          textr->SetInputConnection(reader->GetOutputPort(0));
        }
        textr->Update();

        this->AddTexture(nickname, key.substr(0, key.size() - 1).c_str(), textr);
      }
    }
  }

  return true;
}

// ----------------------------------------------------------------------------
const char* vtkOSPRayMaterialLibrary::WriteBuffer()
{
  Json::Value root;
  root["family"] = "OSPRay";
  root["version"] = "0.0";
  Json::Value materials;

  vtkSmartPointer<vtkXMLImageDataWriter> idwriter = vtkSmartPointer<vtkXMLImageDataWriter>::New();
  idwriter->WriteToOutputStringOn();

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
        vtkSmartPointer<vtkTexture> vvals = vit->second;
        idwriter->SetInputData(vvals->GetInput());
        idwriter->Write();
        std::string os = idwriter->GetOutputString();
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

  if (rstring.size())
  {
    char* buf = new char[rstring.size() + 1];
    memcpy(buf, rstring.c_str(), rstring.size());
    buf[rstring.size()] = 0;
    return buf;
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
void vtkOSPRayMaterialLibrary::Fire()
{
  this->InvokeEvent(vtkCommand::UpdateDataEvent);
}

//-----------------------------------------------------------------------------
std::set<std::string> vtkOSPRayMaterialLibrary::GetMaterialNames()
{
  return this->Internal->NickNames;
}

//-----------------------------------------------------------------------------
std::string vtkOSPRayMaterialLibrary::LookupImplName(const std::string& nickname)
{
  return this->Internal->ImplNames[nickname];
}

//-----------------------------------------------------------------------------
vtkTexture* vtkOSPRayMaterialLibrary::GetTexture(
  const std::string& nickname, const std::string& texturename)
{
  NamedTextures tsForNickname;
  if (this->Internal->TexturesFor.find(nickname) != this->Internal->TexturesFor.end())
  {
    tsForNickname = this->Internal->TexturesFor[nickname];
    std::string realname = ::FindRealName(this->Internal->ImplNames[nickname], texturename);
    return tsForNickname[realname];
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
std::vector<double> vtkOSPRayMaterialLibrary::GetDoubleShaderVariable(
  const std::string& nickname, const std::string& varname)
{
  if (this->Internal->VariablesFor.find(nickname) != this->Internal->VariablesFor.end())
  {
    NamedVariables vsForNickname = this->Internal->VariablesFor[nickname];
    std::string realname = ::FindRealName(this->Internal->ImplNames[nickname], varname);
    return vsForNickname[realname];
  }
  return std::vector<double>();
}

//-----------------------------------------------------------------------------
std::vector<std::string> vtkOSPRayMaterialLibrary::GetDoubleShaderVariableList(
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

//-----------------------------------------------------------------------------
std::vector<std::string> vtkOSPRayMaterialLibrary::GetTextureList(const std::string& nickname)
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

//-----------------------------------------------------------------------------
const std::map<std::string, vtkOSPRayMaterialLibrary::ParametersMap>&
vtkOSPRayMaterialLibrary::GetParametersDictionary()
{
  // This is the material dictionary from OSPRay 1.8
  // If attribute name changes with new OSPRay version, keep old name aliases support in functions
  // vtkOSPRayMaterialLibrary::AddShaderVariable and vtkOSPRayMaterialLibrary::AddTexture
  static std::map<std::string, vtkOSPRayMaterialLibrary::ParametersMap> dic = {
    { "OBJMaterial",
      {
        { "Ka", vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "Kd", vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "Ks", vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "Ns", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "d", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "Tf", vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "map_Bump", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_Bump.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_Bump.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_Bump.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_Bump.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_Kd", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_Kd.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_Kd.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_Kd.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_Kd.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_Ks", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_Ks.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_Ks.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_Ks.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_Ks.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_Ns", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_Ns.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_Ns.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_Ns.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_Ns.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_d", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "map_d.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "map_d.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "map_d.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "map_d.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
      } },
    { "Principled",
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
        { "baseColorMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "baseColorMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "baseColorMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "baseColorMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "baseColorMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "edgeColorMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "edgeColorMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "edgeColorMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "edgeColorMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "edgeColorMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "metallicMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "metallicMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "metallicMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "metallicMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "metallicMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "diffuseMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "diffuseMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "diffuseMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "diffuseMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "diffuseMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "specularMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "specularMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "specularMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "specularMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "specularMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "iorMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "iorMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "iorMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "iorMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "iorMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "transmissionMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "transmissionMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "transmissionMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "transmissionMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "transmissionMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "transmissionColorMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "transmissionColorMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "transmissionColorMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "transmissionColorMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "transmissionColorMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "transmissionDepthMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "transmissionDepthMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "transmissionDepthMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "transmissionDepthMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "transmissionDepthMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "roughnessMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "roughnessMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "roughnessMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "roughnessMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "roughnessMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "anisotropyMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "anisotropyMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "anisotropyMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "anisotropyMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "anisotropyMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "rotationMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "rotationMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "rotationMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "rotationMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "rotationMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "normalMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "normalMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "normalMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "normalMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "normalMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "baseNormalMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "baseNormalMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "baseNormalMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "baseNormalMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "baseNormalMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "thinMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "thinMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "thinMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "thinMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "thinMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "thicknessMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "thicknessMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "thicknessMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "thicknessMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "thicknessMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "backlightMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "backlightMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "backlightMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "backlightMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "backlightMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "coatMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "coatMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatIorMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "coatIorMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "coatIorMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatIorMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatIorMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatColorMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "coatColorMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "coatColorMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatColorMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatColorMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatThicknessMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "coatThicknessMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "coatThicknessMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatThicknessMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatThicknessMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatRoughnessMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "coatRoughnessMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "coatRoughnessMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatRoughnessMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatRoughnessMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatNormalMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "coatNormalMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "coatNormalMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatNormalMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatNormalMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "sheenMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "sheenMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "sheenMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "sheenMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "sheenMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "sheenColorMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "sheenColorMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "sheenColorMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "sheenColorMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "sheenColorMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "sheenTintMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "sheenTintMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "sheenTintMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "sheenTintMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "sheenTintMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "sheenRoughnessMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "sheenRoughnessMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "sheenRoughnessMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "sheenRoughnessMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "sheenRoughnessMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "opacityMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "opacityMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "opacityMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "opacityMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "opacityMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
      } },
    { "CarPaint",
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
        { "baseColorMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "baseColorMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "baseColorMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "baseColorMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "baseColorMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "roughnessMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "roughnessMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "roughnessMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "roughnessMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "roughnessMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "normalMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "normalMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "normalMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "normalMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "normalMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "flakeDensityMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "flakeDensityMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "flakeDensityMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "flakeDensityMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "flakeDensityMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "flakeScaleMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "flakeScaleMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "flakeScaleMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "flakeScaleMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "flakeScaleMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "flakeSpreadMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "flakeSpreadMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "flakeSpreadMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "flakeSpreadMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "flakeSpreadMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "flakeJitterMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "flakeJitterMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "flakeJitterMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "flakeJitterMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "flakeJitterMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "flakeRoughnessMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "flakeRoughnessMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "flakeRoughnessMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "flakeRoughnessMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "flakeRoughnessMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "coatMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "coatMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatIorMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "coatIorMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "coatIorMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatIorMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatIorMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatColorMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "coatColorMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "coatColorMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatColorMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatColorMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatThicknessMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "coatThicknessMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "coatThicknessMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatThicknessMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatThicknessMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatRoughnessMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "coatRoughnessMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "coatRoughnessMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatRoughnessMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatRoughnessMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatNormalMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "coatNormalMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "coatNormalMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "coatNormalMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "coatNormalMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "flipflopColorMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "flipflopColorMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "flipflopColorMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "flipflopColorMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "flipflopColorMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "flipflopFalloffMap", vtkOSPRayMaterialLibrary::ParameterType::TEXTURE },
        { "flipflopFalloffMap.transform", vtkOSPRayMaterialLibrary::ParameterType::VEC4 },
        { "flipflopFalloffMap.rotation", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "flipflopFalloffMap.scale", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
        { "flipflopFalloffMap.translation", vtkOSPRayMaterialLibrary::ParameterType::VEC2 },
      } },
    { "Metal",
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
    { "Alloy",
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
    { "Glass",
      {
        { "eta", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
        { "attenuationColor", vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "attenuationDistance", vtkOSPRayMaterialLibrary::ParameterType::FLOAT },
      } },
    { "ThinGlass",
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
    { "MetallicPaint",
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
    { "Luminous",
      {
        { "color", vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "intensity", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "transparency", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
      } },
  };
  return dic;
}
