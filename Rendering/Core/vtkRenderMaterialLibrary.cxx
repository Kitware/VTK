// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkRenderMaterialLibrary.h"

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
std::set<std::string> vtkRenderMaterialLibrary::GetMaterialNames()
{
  return this->Internal->NickNames;
}

//------------------------------------------------------------------------------
std::string vtkRenderMaterialLibrary::LookupImplName(const std::string& nickname)
{
  return this->Internal->ImplNames[nickname];
}

VTK_ABI_NAMESPACE_END
