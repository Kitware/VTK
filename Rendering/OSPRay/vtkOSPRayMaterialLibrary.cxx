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

#include "vtkActor.h"
#include "vtkCommand.h"
#include "vtkImageData.h"
#include "vtkJPEGReader.h"
#include "vtkPNGReader.h"
#include "vtkObjectFactory.h"
#include "vtkOSPRayMaterialLibrary.h"
#include "vtkSmartPointer.h"
#include "vtkTexture.h"
#include "vtkXMLImageDataReader.h"
#include "vtkXMLImageDataWriter.h"
#include "vtk_jsoncpp.h"
#include <vtksys/SystemTools.hxx>

#include <fstream>
#include <vector>
#include <string>

#include <sys/types.h>

typedef std::map<std::string, std::vector<double> > NamedVariables;
typedef std::map<std::string, vtkSmartPointer<vtkTexture> > NamedTextures;

class vtkOSPRayMaterialLibraryInternals
{
public:
  vtkOSPRayMaterialLibraryInternals()
  {
  }
  ~vtkOSPRayMaterialLibraryInternals()
  {
  }

  std::set<std::string> NickNames;
  std::map<std::string, std::string > ImplNames;
  std::map<std::string, NamedVariables > VariablesFor;
  std::map<std::string, NamedTextures > TexturesFor;
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
  this->Superclass::PrintSelf(os,indent);
}

// ----------------------------------------------------------------------------
void vtkOSPRayMaterialLibrary::AddMaterial(const std::string& nickname, const std::string& implname)
{
  this->Internal->NickNames.insert(nickname);
  this->Internal->ImplNames[nickname] = implname;
}

// ----------------------------------------------------------------------------
void vtkOSPRayMaterialLibrary::AddTexture(const std::string& nickname, const std::string& texname, vtkTexture* tex)
{
  NamedTextures &tsForNickname = this->Internal->TexturesFor[nickname];
  tsForNickname[texname] = tex;
}

// ----------------------------------------------------------------------------
void vtkOSPRayMaterialLibrary::AddShaderVariable(const std::string& nickname, const std::string& varname, int numVars, double *x)
{
  std::vector<double> w;
  w.assign(x, x+numVars);

  NamedVariables &vsForNickname = this->Internal->VariablesFor[nickname];
  vsForNickname[varname] = w;
}

// ----------------------------------------------------------------------------
bool vtkOSPRayMaterialLibrary::ReadFile
 (const char *filename)
{
  return this->InternalParse(filename, true);
}

// ----------------------------------------------------------------------------
bool vtkOSPRayMaterialLibrary::ReadBuffer
  (const char *filename)
{
  return this->InternalParse(filename, false);
}

// ----------------------------------------------------------------------------
bool vtkOSPRayMaterialLibrary::InternalParse
  (const char *filename, bool fromfile)
{
  if (!filename)
  {
    return false;
  }
  if (fromfile && !vtksys::SystemTools::FileExists(filename, true))
  {
    return false;
  }

  std::istream *doc;
  if (fromfile)
  {
    doc = new std::ifstream(filename, std::ifstream::binary);
  } else {
    doc = new std::istringstream(filename);
  }
  if (std::string(filename).rfind(".mtl") != std::string::npos)
    {
    return this->InternalParseMTL(filename, fromfile, doc);
    }
  return this->InternalParseJSON(filename, fromfile, doc);
}

// ----------------------------------------------------------------------------
bool vtkOSPRayMaterialLibrary::InternalParseJSON
  (const char *filename, bool fromfile, std::istream *doc)
{
  //todo: this reader is a lot more fragile then I'ld like, need to make it robust
  Json::Value root;
  *doc >> root;
  delete doc;

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
  for (size_t i=0; i<ikeys.size(); ++i )
  {
    const std::string& nickname = ikeys[i];
    const Json::Value nextmat = materials[nickname];
    if (!nextmat.isMember("type"))
    {
      vtkErrorMacro("Invalid material " <<  nickname << " must have \"type\"=\"...\" entry, ignoring.");
      continue;
    }

    // keep a record so others know this material is available
    this->Internal->NickNames.insert(nickname);

    const std::string& implname = nextmat["type"].asString();
    this->Internal->ImplNames[nickname] = implname;
    if (nextmat.isMember("textures"))
    {
      std::string parentDir
        = vtksys::SystemTools::GetParentDirectory(filename);
      const Json::Value textures = nextmat["textures"];
      std::vector<std::string> jkeys = textures.getMemberNames();
      for (size_t j=0; j<jkeys.size(); ++j )
      {
        const std::string &tname = jkeys[j];
        const Json::Value nexttext = textures[tname];
        const char *tfname = nexttext.asCString();
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
          if (tfullname.substr( tfullname.length() - 3 ) == "png")
          {
            pngReader->SetFileName(tfullname.c_str());
            pngReader->Update();
            textr->SetInputConnection(pngReader->GetOutputPort(0));
          } else {
            jpgReader->SetFileName(tfullname.c_str());
            jpgReader->Update();
            textr->SetInputConnection(jpgReader->GetOutputPort(0));
          }
        } else {
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
      std::vector<std::string> jkeys = doubles.getMemberNames();
      for (size_t j=0; j<jkeys.size(); ++j )
      {
        const std::string &vname = jkeys[j];
        const Json::Value nexttext = doubles[vname];
        double *vals = new double[nexttext.size()];
        for (size_t k=0; k < nexttext.size(); ++k)
        {
          const Json::Value nv = nexttext[static_cast<int>(k)];
          vals[k] = nv.asDouble();
        }
        this->AddShaderVariable(nickname, vname, nexttext.size(), vals);
        delete[] vals;
      }
    }
  }

  return true;
}

namespace {
  static std::string trim(std::string s)
  {
    size_t start = 0;
    while ((start < s.length()) && (isspace(s[start])))
    {
      start++;
    }
    size_t end = s.length();
    while ((end > start) && (isspace(s[end-1])))
    {
      end--;
    }
    return s.substr(start, end-start);
  }
}

// ----------------------------------------------------------------------------
bool vtkOSPRayMaterialLibrary::InternalParseMTL
  (const char *filename, bool fromfile, std::istream *doc)
{
  std::string str;
  std::string nickname = "";
  std::string implname = "OBJMaterial";

  const std::vector<std::string> singles
    {"d ", "Ks ", "alpha ", "roughness ", "eta ", "thickness "};
  const std::vector<std::string> triples
    {"Ka ", "color ", "Kd ", "Ks "};
  const std::vector<std::string> textures
    {"map_d ",
     "map_Kd ", "map_kd ", "colorMap ",
     "map_Ks ", "map_ks ",
     "map_Ns ", "map_ns ", "map_Bump", "map_bump", "normalMap", "bumpMap"};

  while(getline(*doc, str))
  {
    std::string tstr = trim(str);
    std::string lkey;

    //a new material
    lkey = "newmtl ";
    if (tstr.compare(0, lkey.size(), lkey) == 0)
    {
      nickname = trim(tstr.substr(lkey.size()));
      this->Internal->NickNames.insert(nickname);
      this->Internal->ImplNames[nickname] = "OBJMaterial";
    }

    //ospray type of the material, if not obj
    lkey = "type ";
    if (tstr.compare(0, lkey.size(), lkey) == 0)
    {
      //this non standard entry is a quick way to break out of
      //objmaterial and use one of the ospray specific materials
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
      if (implname == "glass")
      {
        implname = "ThinGlass";
      }
      if (implname == "metallicPaint")
      {
        implname = "MetallicPaint";
      }
      this->Internal->ImplNames[nickname] = implname;
    }

    //grab all the single valued settings we see
    std::vector<std::string>::const_iterator sit1 = singles.begin();
    while (sit1 != singles.end())
    {
      std::string key = *sit1;
      ++sit1;
      if (tstr.compare(0, key.size(), key) == 0)
      {
        std::string v = tstr.substr(key.size());
        double dv;
        bool OK = false;
        try
        {
          dv = std::stod(v);
          OK = true;
        }
        catch (const std::invalid_argument&) {}
        catch (const std::out_of_range&) {}
        if (OK)
        {
          double vals[1] = {dv};
          this->AddShaderVariable(nickname, key.substr(0,key.size()-1).c_str(), 1, vals);
        }
      }
    }

    //grab all the triple valued settings we see
    std::vector<std::string>::const_iterator sit3 = triples.begin();
    while (sit3 != triples.end())
    {
      std::string key = *sit3;
      ++sit3;
      if (tstr.compare(0, key.size(), key) == 0)
      {
        std::string vs = tstr.substr(key.size());
        size_t loc1 = vs.find(" ");
        size_t loc2 = vs.find(" ", loc1);
        std::string v1 = vs.substr(0,loc1);
        std::string v2 = vs.substr(loc1+1,loc2);
        std::string v3 = vs.substr(loc2+1);
        double d1, d2, d3;
        bool OK = false;
        try
        {
          d1 = std::stod(v1);
          d2 = std::stod(v1);
          d3 = std::stod(v1);
          OK = true;
        }
        catch (const std::invalid_argument&) {}
        catch (const std::out_of_range&) {}
        if (OK)
        {
          double vals[3] = {d1, d2, d3};
          this->AddShaderVariable(nickname, key.substr(0,key.size()-1).c_str(), 3, vals);
        }
      }
    }

    //grab all the textures we see
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
          std::string parentDir
            = vtksys::SystemTools::GetParentDirectory(filename);
          std::string tfullname = parentDir + "/" + tfname;
          if (!vtksys::SystemTools::FileExists(tfullname.c_str(), true))
          {
            cerr << "No such texture file " << tfullname << " skipping" << endl;
            continue;
          }
          if (tfullname.substr( tfullname.length() - 3 ) == "png")
          {
            pngReader->SetFileName(tfullname.c_str());
            pngReader->Update();
            textr->SetInputConnection(pngReader->GetOutputPort(0));
          } else {
            jpgReader->SetFileName(tfullname.c_str());
            jpgReader->Update();
            textr->SetInputConnection(jpgReader->GetOutputPort(0));
          }
        } else {
          vtkSmartPointer<vtkXMLImageDataReader> reader =
            vtkSmartPointer<vtkXMLImageDataReader>::New();
          reader->ReadFromInputStringOn();
          reader->SetInputString(tfname);
          textr->SetInputConnection(reader->GetOutputPort(0));
        }
        textr->Update();

        this->AddTexture(nickname, key.substr(0,key.size()-1).c_str(), textr);
      }
    }
  }

  return true;
}

// ----------------------------------------------------------------------------
const char * vtkOSPRayMaterialLibrary::WriteBuffer()
{
  Json::Value root;
  root["family"] = "OSPRay";
  root["version"] = "0.0";
  Json::Value materials;

  vtkSmartPointer<vtkXMLImageDataWriter> idwriter =
    vtkSmartPointer<vtkXMLImageDataWriter>::New();
  idwriter->WriteToOutputStringOn();

  std::set<std::string>::iterator it = this->Internal->NickNames.begin();
  while (it != this->Internal->NickNames.end())
  {
    std::string nickname = *it;
    Json::Value jnickname;
    std::string implname = this->LookupImplName(nickname);
    jnickname["type"] = implname;

    if (this->Internal->VariablesFor.find(nickname)
        != this->Internal->VariablesFor.end())
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

    if (this->Internal->TexturesFor.find(nickname)
        != this->Internal->TexturesFor.end())
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
  Json::FastWriter fast;
  std::string sFast = fast.write(root);

  char *buf = new char[sFast.length()+1];
  memcpy(buf, sFast.c_str(), sFast.length());
  buf[sFast.length()] =0;
  return buf;
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
vtkTexture* vtkOSPRayMaterialLibrary::GetTexture(const std::string& nickname, const std::string& texturename)
{
  NamedTextures tsForNickname;
  if (this->Internal->TexturesFor.find(nickname) != this->Internal->TexturesFor.end())
  {
    tsForNickname = this->Internal->TexturesFor[nickname];
    return tsForNickname[texturename];
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
std::vector<double> vtkOSPRayMaterialLibrary::GetDoubleShaderVariable(const std::string& nickname, const std::string& varname)
{
  if (this->Internal->VariablesFor.find(nickname) != this->Internal->VariablesFor.end())
  {
    NamedVariables vsForNickname = this->Internal->VariablesFor[nickname];
    return vsForNickname[varname];
  }
  return std::vector<double>();
}
