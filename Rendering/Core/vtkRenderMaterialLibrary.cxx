// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkRenderMaterialLibrary.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkTexture.h"

#include <map>
#include <set>
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

struct TextureInfo
{
  std::string Name;
  vtkSmartPointer<vtkTexture> Texture;
  std::string Filename;
};

typedef std::map<std::string, std::vector<double>> NamedVariables;
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
}

//------------------------------------------------------------------------------
void vtkRenderMaterialLibrary::Fire()
{
  this->InvokeEvent(vtkCommand::UpdateDataEvent);
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
void vtkRenderMaterialLibrary::AddShaderVariable(
  const std::string& nickname, const std::string& varname, int numVars, const double* x)
{
  std::vector<double> w;
  w.assign(x, x + numVars);
  NamedVariables& vsForNickname = this->Internal->VariablesFor[nickname];
  vsForNickname[varname] = std::move(w);
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
