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
}

typedef std::map<std::string, std::vector<double>> NamedVariables;
typedef std::map<std::string, vtkSmartPointer<vtkTexture>> NamedTextures;

class vtkOSPRayMaterialLibraryInternals
{
public:
  vtkOSPRayMaterialLibraryInternals() = default;
  ~vtkOSPRayMaterialLibraryInternals() = default;

  std::set<std::string> NickNames;
  std::map<std::string, std::string> ImplNames;
  std::map<std::string, NamedVariables> VariablesFor;
  std::map<std::string, NamedTextures> TexturesFor;
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkOSPRayMaterialLibrary);

//------------------------------------------------------------------------------
vtkOSPRayMaterialLibrary::vtkOSPRayMaterialLibrary()
{
  this->Internal = new vtkOSPRayMaterialLibraryInternals;
}

//------------------------------------------------------------------------------
vtkOSPRayMaterialLibrary::~vtkOSPRayMaterialLibrary()
{
  delete this->Internal;
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkOSPRayMaterialLibrary::RemoveMaterial(const std::string& nickname)
{
  this->Internal->NickNames.erase(nickname);
  this->Internal->ImplNames.erase(nickname);
  this->Internal->VariablesFor.erase(nickname);
  this->Internal->TexturesFor.erase(nickname);
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkOSPRayMaterialLibrary::RemoveTexture(
  const std::string& nickname, const std::string& texname)
{
  std::string realname = ::FindRealName(this->Internal->ImplNames[nickname], texname);
  this->Internal->TexturesFor[nickname].erase(realname);
}

//------------------------------------------------------------------------------
void vtkOSPRayMaterialLibrary::RemoveAllTextures(const std::string& nickname)
{
  this->Internal->TexturesFor[nickname].clear();
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkOSPRayMaterialLibrary::RemoveShaderVariable(
  const std::string& nickname, const std::string& varname)
{
  std::string realname = ::FindRealName(this->Internal->ImplNames[nickname], varname);
  this->Internal->VariablesFor[nickname].erase(realname);
}

//------------------------------------------------------------------------------
void vtkOSPRayMaterialLibrary::RemoveAllShaderVariables(const std::string& nickname)
{
  this->Internal->VariablesFor[nickname].clear();
}

//------------------------------------------------------------------------------
bool vtkOSPRayMaterialLibrary::ReadFile(const char* filename)
{
  return this->InternalParse(filename, true);
}

//------------------------------------------------------------------------------
bool vtkOSPRayMaterialLibrary::ReadBuffer(const char* filename)
{
  return this->InternalParse(filename, false);
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
bool vtkOSPRayMaterialLibrary::InternalParseJSON(
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

    std::string implname = nextmat["type"].asString();
    // backward compatibility over OSPRay 2.0 name changes
    if (implname == "Alloy")
    {
      implname = "alloy";
    }
    if (implname == "CarPaint")
    {
      implname = "carPaint";
    }
    if (implname == "Glass")
    {
      implname = "glass";
    }
    if (implname == "Metal")
    {
      implname = "metal";
    }
    if (implname == "MetallicPaint")
    {
      implname = "metallicPaint";
    }
    if (implname == "OBJMaterial")
    {
      implname = "obj";
    }
    if (implname == "Principled")
    {
      implname = "principled";
    }
    if (implname == "ThinGlass")
    {
      implname = "thinGlass";
    }
    this->Internal->ImplNames[nickname] = implname;
    if (nextmat.isMember("textures"))
    {
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

//------------------------------------------------------------------------------
bool vtkOSPRayMaterialLibrary::InternalParseMTL(
  const char* filename, bool fromfile, std::istream* doc)
{
  std::string str;
  std::string nickname = "";
  std::string implname = "obj";
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

    // ospray type of the material, if not obj
    lkey = "type ";
    if (tstr.compare(0, lkey.size(), lkey) == 0)
    {
      // this non standard entry is a quick way to break out of
      // objmaterial and use one of the ospray specific materials
      implname = trim(tstr.substr(lkey.size()));
      if (implname == "matte")
      {
        implname = "obj";
      }
      if (implname == "glass")
      {
        implname = "thinGlass";
      }
      // backward compatibility over OSPRay 2.0 name changes
      if (implname == "Alloy")
      {
        implname = "alloy";
      }
      if (implname == "CarPaint")
      {
        implname = "carPaint";
      }
      if (implname == "Glass")
      {
        implname = "glass";
      }
      if (implname == "Metal")
      {
        implname = "metal";
      }
      if (implname == "MetallicPaint")
      {
        implname = "metallicPaint";
      }
      if (implname == "OBJMaterial")
      {
        implname = "obj";
      }
      if (implname == "Principled")
      {
        implname = "principled";
      }
      if (implname == "ThinGlass")
      {
        implname = "thinGlass";
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
      if (!tfname.empty())
      {
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

        this->AddTexture(nickname, key.substr(0, key.size() - 1).c_str(), textr);
      }
    }
  }

  return true;
}

//------------------------------------------------------------------------------
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
void vtkOSPRayMaterialLibrary::Fire()
{
  this->InvokeEvent(vtkCommand::UpdateDataEvent);
}

//------------------------------------------------------------------------------
std::set<std::string> vtkOSPRayMaterialLibrary::GetMaterialNames()
{
  return this->Internal->NickNames;
}

//------------------------------------------------------------------------------
std::string vtkOSPRayMaterialLibrary::LookupImplName(const std::string& nickname)
{
  return this->Internal->ImplNames[nickname];
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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
    { "Luminous",
      {
        { "color", vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB },
        { "intensity", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
        { "transparency", vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT },
      } },
  };
  return dic;
}
