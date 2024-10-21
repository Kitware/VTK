// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAnariRendererManager.h"
#include "vtkAnariProfiling.h"

#include "vtkLogger.h"
#include "vtkObjectFactory.h"

#include <anari/anari_cpp/ext/std.h>

VTK_ABI_NAMESPACE_BEGIN

using namespace anari::std_types;

// ----------------------------------------------------------------------------
class vtkAnariRendererManagerInternals : public vtkObject
{
public:
  static vtkAnariRendererManagerInternals* New();
  vtkTypeMacro(vtkAnariRendererManagerInternals, vtkObject);

  vtkAnariRendererManagerInternals() = default;
  ~vtkAnariRendererManagerInternals();

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
vtkAnariRendererManagerInternals::~vtkAnariRendererManagerInternals()
{
  CleanupAnariObjects();
  anari::release(this->AnariDevice, this->AnariDevice);
  this->AnariDevice = nullptr;
}

// ----------------------------------------------------------------------------
void vtkAnariRendererManagerInternals::CleanupAnariObjects()
{
  if (this->AnariDevice)
  {
    anari::release(this->AnariDevice, this->AnariRenderer);
    this->AnariRenderer = nullptr;
    this->AnariRendererSubtype.clear();
  }
}

//------------------------------------------------------------------------------
template <typename T>
void vtkAnariRendererManagerInternals::SetRendererParameter(const char* p, const T& v)
{
  if (!this->AnariDevice || !this->AnariRenderer)
  {
    return;
  }

  anari::setParameter(this->AnariDevice, this->AnariRenderer, p, v);
  anari::commitParameters(this->AnariDevice, this->AnariRenderer);
}

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAnariRendererManagerInternals);

//============================================================================

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkAnariRendererManager);

//----------------------------------------------------------------------------
void vtkAnariRendererManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkAnariRendererManager::SetAnariDevice(anari::Device d)
{
  if (d == GetAnariDevice())
  {
    return;
  }

  this->Internal->CleanupAnariObjects();

  if (this->Internal->AnariDevice)
  {
    anari::release(this->Internal->AnariDevice, this->Internal->AnariDevice);
  }

  this->Internal->AnariDevice = d;
  anari::retain(d, d);
  this->SetAnariRendererSubtype();
}

anari::Device vtkAnariRendererManager::GetAnariDevice() const
{
  return this->Internal->AnariDevice;
}

//----------------------------------------------------------------------------
void vtkAnariRendererManager::SetAnariRendererSubtype(const char* subtype)
{
  if (this->Internal->AnariRendererSubtype == subtype)
  {
    return;
  }

  if (!this->Internal->AnariDevice)
  {
    return;
  }

  this->Internal->CleanupAnariObjects();

  anari::Renderer renderer = anari::newObject<anari::Renderer>(this->GetAnariDevice(), subtype);
  if (!renderer)
  {
    vtkDebugMacro(<< "[ANARI] unable to create '" << subtype << "' renderer.\n");
    return;
  }

  this->Internal->AnariRenderer = renderer;
  this->Internal->AnariRendererSubtype = subtype;
}

const char* vtkAnariRendererManager::GetAnariRendererSubtype() const
{
  return this->Internal->AnariRendererSubtype.c_str();
}

//----------------------------------------------------------------------------
void vtkAnariRendererManager::SetAnariRendererParameter(const char* param, bool b)
{
  this->Internal->SetRendererParameter(param, b);
}

//----------------------------------------------------------------------------
void vtkAnariRendererManager::SetAnariRendererParameter(const char* param, int x)
{
  this->Internal->SetRendererParameter(param, x);
}

//----------------------------------------------------------------------------
void vtkAnariRendererManager::SetAnariRendererParameter(const char* param, int x, int y)
{
  this->Internal->SetRendererParameter(param, ivec2{ x, y });
}

//----------------------------------------------------------------------------
void vtkAnariRendererManager::SetAnariRendererParameter(const char* param, int x, int y, int z)
{
  this->Internal->SetRendererParameter(param, ivec3{ x, y, z });
}

//----------------------------------------------------------------------------
void vtkAnariRendererManager::SetAnariRendererParameter(
  const char* param, int x, int y, int z, int w)
{
  this->Internal->SetRendererParameter(param, ivec4{ x, y, z, w });
}

//----------------------------------------------------------------------------
void vtkAnariRendererManager::SetAnariRendererParameter(const char* param, float x)
{
  this->Internal->SetRendererParameter(param, x);
}

//----------------------------------------------------------------------------
void vtkAnariRendererManager::SetAnariRendererParameter(const char* param, float x, float y)
{
  this->Internal->SetRendererParameter(param, vec2{ x, y });
}

//----------------------------------------------------------------------------
void vtkAnariRendererManager::SetAnariRendererParameter(
  const char* param, float x, float y, float z)
{
  this->Internal->SetRendererParameter(param, vec3{ x, y, z });
}

//----------------------------------------------------------------------------
void vtkAnariRendererManager::SetAnariRendererParameter(
  const char* param, float x, float y, float z, float w)
{
  this->Internal->SetRendererParameter(param, vec4{ x, y, z, w });
}

// ----------------------------------------------------------------------------
anari::Renderer vtkAnariRendererManager::GetAnariRenderer() const
{
  return this->Internal->AnariRenderer;
}

// ----------------------------------------------------------------------------
vtkAnariRendererManager::vtkAnariRendererManager()
{
  this->Internal = vtkAnariRendererManagerInternals::New();
}

// ----------------------------------------------------------------------------
vtkAnariRendererManager::~vtkAnariRendererManager()
{
  this->Internal->Delete();
  this->Internal = nullptr;
}

VTK_ABI_NAMESPACE_END
