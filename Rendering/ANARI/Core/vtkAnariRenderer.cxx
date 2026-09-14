// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAnariRenderer.h"

#include "vtkObjectFactory.h"

#include <anari/anari_cpp/ext/std.h>

VTK_ABI_NAMESPACE_BEGIN

using namespace anari::std_types;

// ----------------------------------------------------------------------------
class vtkAnariRendererInternals : public vtkObject
{
public:
  static vtkAnariRendererInternals* New();
  vtkTypeMacro(vtkAnariRendererInternals, vtkObject);

  vtkAnariRendererInternals() = default;
  ~vtkAnariRendererInternals() override;

  void CleanupAnariObjects();

  /**
   * Set a parameter on the underlying anari::Renderer
   */
  template <typename T>
  void SetRendererParameter(const char* p, const T& v);

  anari::Device AnariDevice{ nullptr };
  anari::Renderer AnariRenderer{ nullptr };
  std::string AnariRendererSubtype;
};

// ----------------------------------------------------------------------------
vtkAnariRendererInternals::~vtkAnariRendererInternals()
{
  CleanupAnariObjects();
}

// ----------------------------------------------------------------------------
void vtkAnariRendererInternals::CleanupAnariObjects()
{
  if (this->AnariDevice)
  {
    anari::release(this->AnariDevice, this->AnariRenderer);
    this->AnariRenderer = nullptr;
    this->AnariRendererSubtype.clear();
    anari::release(this->AnariDevice, this->AnariDevice);
    this->AnariDevice = nullptr;
  }
}

//------------------------------------------------------------------------------
template <typename T>
void vtkAnariRendererInternals::SetRendererParameter(const char* p, const T& v)
{
  if (!this->AnariDevice || !this->AnariRenderer)
  {
    vtkWarningMacro(<< "ANARI device not yet set, ignoring renderer parameter '" << p << "'");
    return;
  }

  anari::setParameter(this->AnariDevice, this->AnariRenderer, p, v);
  anari::commitParameters(this->AnariDevice, this->AnariRenderer);
}

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAnariRendererInternals);

//============================================================================

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAnariRenderer);

//----------------------------------------------------------------------------
void vtkAnariRenderer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkAnariRenderer::SetAnariDevice(vtkAnariDevice* dev)
{
  vtkDebugMacro(<< " setting AnariDevice  to " << dev);
  if (this->AnariDevice == dev)
  {
    return;
  }

  if (this->AnariDevice)
  {
    this->Internal->CleanupAnariObjects();
  }

  this->AnariDevice = dev;
  if (auto devHandle = this->AnariDevice->GetHandle())
  {
    this->Internal->AnariDevice = devHandle;
    anari::retain(devHandle, devHandle);
    this->SetSubtype();
  }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkAnariRenderer::SetSubtype(const char* subtype)
{
  if (this->Internal->AnariRendererSubtype == subtype)
  {
    return;
  }

  if (!this->Internal->AnariDevice)
  {
    return;
  }

  if (this->Internal->AnariRenderer)
  {
    anari::release(this->Internal->AnariDevice, this->Internal->AnariRenderer);
    this->Internal->AnariRenderer = nullptr;
    this->Internal->AnariRendererSubtype.clear();
  }

  anari::Renderer renderer =
    anari::newObject<anari::Renderer>(this->Internal->AnariDevice, subtype);
  if (!renderer)
  {
    vtkDebugMacro(<< "[ANARI] unable to create '" << subtype << "' renderer.\n");
    return;
  }

  this->Internal->AnariRenderer = renderer;
  this->Internal->AnariRendererSubtype = subtype;
}

const char* vtkAnariRenderer::GetSubtype() const
{
  return this->Internal->AnariRendererSubtype.c_str();
}

//----------------------------------------------------------------------------
void vtkAnariRenderer::SetParameterb(const char* param, bool b)
{
  this->Internal->SetRendererParameter(param, b);
}

//----------------------------------------------------------------------------
void vtkAnariRenderer::SetParameteri(const char* param, int x)
{
  this->Internal->SetRendererParameter(param, x);
}

//----------------------------------------------------------------------------
void vtkAnariRenderer::SetParameter2i(const char* param, int x, int y)
{
  this->Internal->SetRendererParameter(param, ivec2{ x, y });
}

//----------------------------------------------------------------------------
void vtkAnariRenderer::SetParameter3i(const char* param, int x, int y, int z)
{
  this->Internal->SetRendererParameter(param, ivec3{ x, y, z });
}

//----------------------------------------------------------------------------
void vtkAnariRenderer::SetParameter4i(const char* param, int x, int y, int z, int w)
{
  this->Internal->SetRendererParameter(param, ivec4{ x, y, z, w });
}

//----------------------------------------------------------------------------
void vtkAnariRenderer::SetParameterf(const char* param, float x)
{
  this->Internal->SetRendererParameter(param, x);
}

//----------------------------------------------------------------------------
void vtkAnariRenderer::SetParameter2f(const char* param, float x, float y)
{
  this->Internal->SetRendererParameter(param, vec2{ x, y });
}

//----------------------------------------------------------------------------
void vtkAnariRenderer::SetParameter3f(const char* param, float x, float y, float z)
{
  this->Internal->SetRendererParameter(param, vec3{ x, y, z });
}

//----------------------------------------------------------------------------
void vtkAnariRenderer::SetParameter4f(const char* param, float x, float y, float z, float w)
{
  this->Internal->SetRendererParameter(param, vec4{ x, y, z, w });
}

// ----------------------------------------------------------------------------
anari::Renderer vtkAnariRenderer::GetHandle() const
{
  return this->Internal->AnariRenderer;
}

// ----------------------------------------------------------------------------
vtkAnariRenderer::vtkAnariRenderer()
{
  this->Internal = vtkAnariRendererInternals::New();
}

// ----------------------------------------------------------------------------
vtkAnariRenderer::~vtkAnariRenderer()
{
  this->Internal->Delete();
  this->Internal = nullptr;
  if (this->AnariDevice)
  {
    this->AnariDevice = nullptr;
  }
}

// ----------------------------------------------------------------------------
std::vector<std::pair<std::string, int>> vtkAnariRenderer::GetRendererParameters() const
{
  std::vector<std::pair<std::string, int>> rendererParams;
  if (!this->GetHandle() || !this->Internal->AnariDevice)
  {
    return rendererParams;
  }

  const ANARIParameter* rParams =
    reinterpret_cast<const ANARIParameter*>(anariGetObjectInfo(this->Internal->AnariDevice,
      ANARI_RENDERER, this->GetSubtype(), "parameter", ANARI_PARAMETER_LIST));
  if (!rParams)
  {
    vtkWarningMacro(<< "Renderer has no parameters."
                    << "Device: " << this->AnariDevice->GetAnariDeviceName()
                    << " Library: " << this->AnariDevice->GetAnariLibraryName()
                    << " Renderer SubType: " << this->GetSubtype());
  }

  for (const ANARIParameter* rpam = rParams; rpam->name != nullptr; ++rpam)
  {
    rendererParams.emplace_back(rpam->name, rpam->type);
  }

  return rendererParams;
}

// ----------------------------------------------------------------------------
std::string vtkAnariRenderer::GetRendererParameterDescription(
  std::pair<std::string, int> rparam) const
{
  if (!this->GetHandle() || !this->Internal->AnariDevice)
  {
    return std::string();
  }

  return std::string(
    reinterpret_cast<const char*>(anariGetParameterInfo(this->Internal->AnariDevice, ANARI_RENDERER,
      this->GetSubtype(), rparam.first.c_str(), rparam.second, "description", ANARI_STRING)));
}

// ----------------------------------------------------------------------------
bool vtkAnariRenderer::IsRendererParameterRequired(std::pair<std::string, int> rparam) const
{
  if (!this->GetHandle() || !this->Internal->AnariDevice)
  {
    return false;
  }
  const int* required =
    reinterpret_cast<const int*>(anariGetParameterInfo(this->Internal->AnariDevice, ANARI_RENDERER,
      this->GetSubtype(), rparam.first.c_str(), rparam.second, "required", ANARI_BOOL));

  return (required && *required);
}

// ----------------------------------------------------------------------------
const void* vtkAnariRenderer::GetRendererParameterDefault(std::pair<std::string, int> rparam)
{
  if (!this->GetHandle() || !this->Internal->AnariDevice)
  {
    return nullptr;
  }
  return anariGetParameterInfo(this->Internal->AnariDevice, ANARI_RENDERER, this->GetSubtype(),
    rparam.first.c_str(), rparam.second, "default", rparam.second);
}

// ----------------------------------------------------------------------------
const void* vtkAnariRenderer::GetRendererParameterMinimum(std::pair<std::string, int> rparam)
{
  if (!this->GetHandle() || !this->Internal->AnariDevice)
  {
    return nullptr;
  }
  return anariGetParameterInfo(this->Internal->AnariDevice, ANARI_RENDERER, this->GetSubtype(),
    rparam.first.c_str(), rparam.second, "minimum", rparam.second);
}

// ----------------------------------------------------------------------------
const void* vtkAnariRenderer::GetRendererParameterMaximum(std::pair<std::string, int> rparam)
{
  if (!this->GetHandle() || !this->Internal->AnariDevice)
  {
    return nullptr;
  }
  return anariGetParameterInfo(this->Internal->AnariDevice, ANARI_RENDERER, this->GetSubtype(),
    rparam.first.c_str(), rparam.second, "maximum", rparam.second);
}

// ----------------------------------------------------------------------------
const void* vtkAnariRenderer::GetRendererParameterValue(std::pair<std::string, int> rparam)
{
  if (!this->GetHandle() || !this->Internal->AnariDevice)
  {
    return nullptr;
  }
  return anariGetParameterInfo(this->Internal->AnariDevice, ANARI_RENDERER, this->GetSubtype(),
    rparam.first.c_str(), rparam.second, "value", rparam.second);
}

VTK_ABI_NAMESPACE_END
