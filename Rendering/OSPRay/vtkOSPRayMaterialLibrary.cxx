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

  //todo: this reader is a lot more fragile then I'ld like, need to make it robust
  std::istream *doc;
  if (fromfile)
  {
    doc = new std::ifstream(filename, std::ifstream::binary);
  } else {
    doc = new std::istringstream(filename);
  }

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
        for (size_t i = 0; i < vvals.size(); i++)
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
