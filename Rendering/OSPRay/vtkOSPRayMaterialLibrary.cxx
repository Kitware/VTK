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
#include "vtk_jsoncpp.h"
#include <vtksys/SystemTools.hxx>

#include <fstream>
#include <vector>
#include <string>

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

//----------------------------------------------------------------------------
vtkOSPRayMaterialLibrary* vtkOSPRayMaterialLibrary::New()
{
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkOSPRayMaterialLibrary");
  if (ret)
  {
    return static_cast<vtkOSPRayMaterialLibrary*>(ret);
  }
  vtkOSPRayMaterialLibrary* o = new vtkOSPRayMaterialLibrary;
  o->InitializeObjectBase();
  return o;
}

//----------------------------------------------------------------------------
vtkOSPRayMaterialLibrary* vtkOSPRayMaterialLibrary::GetInstance()
{
  static vtkSmartPointer<vtkOSPRayMaterialLibrary> Singleton;
  if (Singleton.GetPointer() == nullptr)
  {
    Singleton.TakeReference(vtkOSPRayMaterialLibrary::New());
  }
  return Singleton.GetPointer();
}

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
void vtkOSPRayMaterialLibrary::AddMaterial(std::string nickname, std::string implname)
{
  this->Internal->NickNames.insert(nickname);
  this->Internal->ImplNames[nickname] = implname;
}

// ----------------------------------------------------------------------------
void vtkOSPRayMaterialLibrary::AddTexture(std::string nickname, std::string texname, vtkTexture* tex)
{
  NamedTextures &tsForNickname = this->Internal->TexturesFor[nickname];
  tsForNickname[texname] = tex;
}

// ----------------------------------------------------------------------------
void vtkOSPRayMaterialLibrary::AddShaderVariable(std::string nickname, std::string varname, int numVars, double *x)
{
  std::vector<double> w;
  w.assign(x, x+numVars);

  NamedVariables &vsForNickname = this->Internal->VariablesFor[nickname];
  vsForNickname[varname] = w;
}

// ----------------------------------------------------------------------------
void vtkOSPRayMaterialLibrary::ReadFile
 (const char *filename)
{
  //todo: this reader is a lot more fragile then I'ld like, need to make it robust
  std::ifstream doc(filename, std::ifstream::binary);
  Json::Value root;
  doc >> root;

  if (!root.isMember("family"))
  {
    vtkErrorMacro("Not a materials file. Must have \"family\"=\"...\" entry.");
    return;
  }
  const Json::Value family = root["family"];
  if (family.asString() != "OSPRay")
  {
    vtkErrorMacro("Unsupported materials file. Family is not \"OSPRay\".");
    return;
  }
  if (!root.isMember("version"))
  {
    vtkErrorMacro("Not a materials file. Must have \"version\"=\"...\" entry.");
    return;
  }
  const Json::Value version = root["version"];
  if (version.asString() != "0.0")
  {
    vtkErrorMacro("Unsupported materials file. Version is not \"0.0\".");
    return;
   }
  if (!root.isMember("materials"))
  {
    vtkErrorMacro("Not a materials file. Must have \"materials\"={...} entry.");
    return;
  }

  const Json::Value materials = root["materials"];
  std::vector<std::string> ikeys = materials.getMemberNames();
  for (int i=0; i<ikeys.size(); ++i )
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
      for (int j=0; j<jkeys.size(); ++j )
      {
        const std::string &tname = jkeys[j];
        const Json::Value nexttext = textures[tname];
        vtkSmartPointer<vtkTexture> textr = vtkSmartPointer<vtkTexture>::New();
        vtkSmartPointer<vtkJPEGReader> jpgReader = vtkSmartPointer<vtkJPEGReader>::New();
        vtkSmartPointer<vtkPNGReader> pngReader = vtkSmartPointer<vtkPNGReader>::New();
        const char *tfname = nexttext.asCString();
        std::string tfullname = parentDir + "/" + tfname;
        if (tfullname.substr( tfullname.length() - 3 ) == "png")
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
        this->AddTexture(nickname, tname, textr);
      }
    }
    if (nextmat.isMember("doubles"))
    {
      const Json::Value doubles = nextmat["doubles"];
      std::vector<std::string> jkeys = doubles.getMemberNames();
      for (int j=0; j<jkeys.size(); ++j )
      {
        const std::string &vname = jkeys[j];
        const Json::Value nexttext = doubles[vname];
        double *vals = new double[nexttext.size()];
        for (int k=0; k < nexttext.size(); ++k)
        {
          const Json::Value nv = nexttext[k];
          vals[k] = nv.asDouble();
        }
        this->AddShaderVariable(nickname, vname, nexttext.size(), vals);
        delete[] vals;
      }
    }
  }
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
std::string vtkOSPRayMaterialLibrary::LookupImplName(std::string nickname)
{
  return this->Internal->ImplNames[nickname];
}

//-----------------------------------------------------------------------------
vtkTexture* vtkOSPRayMaterialLibrary::GetTexture(std::string nickname, std::string texturename)
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
std::vector<double> vtkOSPRayMaterialLibrary::GetDoubleShaderVariable(std::string nickname, std::string varname)
{
  if (this->Internal->VariablesFor.find(nickname) != this->Internal->VariablesFor.end())
  {
    NamedVariables vsForNickname = this->Internal->VariablesFor[nickname];
    return vsForNickname[varname];
  }
  return std::vector<double>();
}
