// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUShaderDatabase.h"
#include "vtkObjectFactory.h"

#include "ActorColorOptions.h"
#include "ActorRenderOptions.h"
#include "ActorTransform.h"
#include "LineFragmentShader.h"
#include "LineMiterJoinVertexShader.h"
#include "LineRoundJoinVertexShader.h"
#include "SceneLight.h"
#include "SceneTransform.h"
#include "Utilities.h"

#include <unordered_map>

VTK_ABI_NAMESPACE_BEGIN

class vtkWebGPUShaderDatabase::vtkInternals
{
public:
  // Key is a string that describes a virtual path to a *.wgsl file.
  // Value is a string which contains the contents of a webgpu shader.
  std::unordered_map<std::string, std::string> Map;
};

vtkStandardNewMacro(vtkWebGPUShaderDatabase);

//------------------------------------------------------------------------------
vtkWebGPUShaderDatabase::vtkWebGPUShaderDatabase()
  : Internals(new vtkInternals())
{
  this->Internals->Map["VTK/wgsl/ActorColorOptions.wgsl"] = ActorColorOptions;
  this->Internals->Map["VTK/wgsl/ActorRenderOptions.wgsl"] = ActorRenderOptions;
  this->Internals->Map["VTK/wgsl/ActorTransform.wgsl"] = ActorTransform;
  this->Internals->Map["VTK/wgsl/LineFragmentShader.wgsl"] = LineFragmentShader;
  this->Internals->Map["VTK/wgsl/LineMiterJoinVertexShader.wgsl"] = LineMiterJoinVertexShader;
  this->Internals->Map["VTK/wgsl/LineRoundJoinVertexShader.wgsl"] = LineRoundJoinVertexShader;
  this->Internals->Map["VTK/wgsl/SceneLight.wgsl"] = SceneLight;
  this->Internals->Map["VTK/wgsl/SceneTransform.wgsl"] = SceneTransform;
  this->Internals->Map["VTK/wgsl/Utilities.wgsl"] = Utilities;
}

//------------------------------------------------------------------------------
vtkWebGPUShaderDatabase::~vtkWebGPUShaderDatabase() = default;

//------------------------------------------------------------------------------
void vtkWebGPUShaderDatabase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkWebGPUShaderDatabase::AddShaderSource(const std::string& key, const std::string& source)
{
  this->Internals->Map[key] = source;
}

//------------------------------------------------------------------------------
std::string vtkWebGPUShaderDatabase::GetShaderSource(const std::string& key) const
{
  const auto it = this->Internals->Map.find(key);
  if (it != this->Internals->Map.end())
  {
    return it->second;
  }
  else
  {
    return "";
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUShaderDatabase::RemoveShaderSource(const std::string& key)
{
  this->Internals->Map.erase(key);
}

VTK_ABI_NAMESPACE_END
