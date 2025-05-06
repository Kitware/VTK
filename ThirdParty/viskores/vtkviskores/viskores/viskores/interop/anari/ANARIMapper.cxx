//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/interop/anari/ANARIMapper.h>

namespace viskores
{
namespace interop
{
namespace anari
{

ANARIMapper::ANARIMapper(anari_cpp::Device device,
                         const ANARIActor& actor,
                         const std::string& name,
                         const viskores::cont::ColorTable& colorTable)
  : Actor(actor)
  , ColorTable(colorTable)
  , Name(name)
{
  this->Handles = std::make_shared<ANARIHandles>();
  this->Handles->Device = device;
  anari_cpp::retain(device, device);
}

anari_cpp::Device ANARIMapper::GetDevice() const
{
  return this->Handles->Device;
}

const ANARIActor& ANARIMapper::GetActor() const
{
  return this->Actor;
}

const char* ANARIMapper::GetName() const
{
  return this->Name.c_str();
}

void ANARIMapper::SetActor(const ANARIActor& actor)
{
  this->Actor = actor;
}

void ANARIMapper::SetMapFieldAsAttribute(bool enabled)
{
  this->MapFieldAsAttribute = enabled;
}

bool ANARIMapper::GetMapFieldAsAttribute() const
{
  return this->MapFieldAsAttribute;
}

const viskores::cont::ColorTable& ANARIMapper::GetColorTable() const
{
  return this->ColorTable;
}

void ANARIMapper::SetANARIColorMap(anari_cpp::Array1D color,
                                   anari_cpp::Array1D opacity,
                                   bool releaseArrays)
{
  auto d = this->GetDevice();
  if (releaseArrays)
  {
    anari_cpp::release(d, color);
    anari_cpp::release(d, opacity);
  }
}

void ANARIMapper::SetANARIColorMapValueRange(const viskores::Vec2f_32&)
{
  // no-op
}

void ANARIMapper::SetANARIColorMapOpacityScale(viskores::Float32)
{
  // no-op
}

void ANARIMapper::SetName(const char* name)
{
  this->Name = name;
}

void ANARIMapper::SetColorTable(const viskores::cont::ColorTable& colorTable)
{
  this->ColorTable = colorTable;
}

anari_cpp::Geometry ANARIMapper::GetANARIGeometry()
{
  return nullptr;
}

anari_cpp::SpatialField ANARIMapper::GetANARISpatialField()
{
  return nullptr;
}

anari_cpp::Surface ANARIMapper::GetANARISurface()
{
  return nullptr;
}

anari_cpp::Volume ANARIMapper::GetANARIVolume()
{
  return nullptr;
}

anari_cpp::Group ANARIMapper::GetANARIGroup()
{
  if (!this->Handles->Group)
  {
    auto d = this->GetDevice();
    this->Handles->Group = anari_cpp::newObject<anari_cpp::Group>(d);
    this->RefreshGroup();
  }

  return this->Handles->Group;
}

anari_cpp::Instance ANARIMapper::GetANARIInstance()
{
  if (!this->Handles->Instance)
  {
    auto d = this->GetDevice();
    this->Handles->Instance = anari_cpp::newObject<anari_cpp::Instance>(d, "transform");
    auto group = this->GetANARIGroup();
    anari_cpp::setParameter(d, this->Handles->Instance, "group", group);
    anari_cpp::setParameter(d, this->Handles->Instance, "name", MakeObjectName("instance"));
    anari_cpp::commitParameters(d, this->Handles->Instance);
  }

  return this->Handles->Instance;
}

bool ANARIMapper::GroupIsEmpty() const
{
  return !this->Valid;
}

std::string ANARIMapper::MakeObjectName(const char* suffix) const
{
  std::string name = this->GetName();
  name += '.';
  name += suffix;
  return name;
}

void ANARIMapper::RefreshGroup()
{
  if (!this->Handles->Group)
    return;

  auto d = this->GetDevice();

  anari_cpp::unsetParameter(d, this->Handles->Group, "surface");
  anari_cpp::unsetParameter(d, this->Handles->Group, "volume");

  auto surface = this->GetANARISurface();
  auto volume = this->GetANARIVolume();

  if (!this->GroupIsEmpty())
  {
    if (surface)
      anari_cpp::setParameterArray1D(d, this->Handles->Group, "surface", &surface, 1);

    if (volume)
      anari_cpp::setParameterArray1D(d, this->Handles->Group, "volume", &volume, 1);

    anari_cpp::setParameter(d, this->Handles->Group, "name", MakeObjectName("group"));
  }

  anari_cpp::commitParameters(d, this->Handles->Group);
}

viskores::cont::ColorTable& ANARIMapper::GetColorTable()
{
  return this->ColorTable;
}

ANARIMapper::ANARIHandles::~ANARIHandles()
{
  anari_cpp::release(this->Device, this->Group);
  anari_cpp::release(this->Device, this->Instance);
  anari_cpp::release(this->Device, this->Device);
}

} // namespace anari
} // namespace interop
} // namespace viskores
