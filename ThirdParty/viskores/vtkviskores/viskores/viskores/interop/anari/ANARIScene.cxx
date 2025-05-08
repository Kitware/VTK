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

#include <viskores/interop/anari/ANARIScene.h>
// std
#include <algorithm>

namespace viskores
{
namespace interop
{
namespace anari
{

ANARIScene::ANARIScene(anari_cpp::Device device)
  : Device(device)
{
  anari_cpp::retain(this->Device, this->Device);
}

ANARIScene::~ANARIScene()
{
  anari_cpp::release(this->Device, this->World);
  anari_cpp::release(this->Device, this->Device);
}

viskores::IdComponent ANARIScene::GetNumberOfMappers() const
{
  return static_cast<viskores::IdComponent>(this->Mappers.size());
}

bool ANARIScene::HasMapperWithName(const char* _name) const
{
  std::string name = _name;
  auto itr = std::find_if(this->Mappers.begin(),
                          this->Mappers.end(),
                          [&](auto& m) { return m.Mapper->GetName() == name; });
  return itr != this->Mappers.end();
}

viskores::IdComponent ANARIScene::GetMapperIndexByName(const char* _name)
{
  std::string name = _name;
  auto itr = std::find_if(this->Mappers.begin(),
                          this->Mappers.end(),
                          [&](auto& m) { return m.Mapper->GetName() == name; });
  return static_cast<viskores::IdComponent>(std::distance(this->Mappers.begin(), itr));
}

ANARIMapper& ANARIScene::GetMapper(viskores::IdComponent id)
{
  return *this->Mappers[id].Mapper;
}

ANARIMapper& ANARIScene::GetMapper(const char* _name)
{
  std::string name = _name;
  auto itr = std::find_if(this->Mappers.begin(),
                          this->Mappers.end(),
                          [&](auto& m) { return m.Mapper->GetName() == name; });
  return *itr->Mapper;
}

bool ANARIScene::GetMapperVisible(viskores::IdComponent id) const
{
  return this->Mappers[id].Show;
}

void ANARIScene::SetMapperVisible(viskores::IdComponent id, bool shown)
{
  auto& m = this->Mappers[id];
  if (m.Show != shown)
  {
    m.Show = shown;
    this->UpdateWorld();
  }
}

void ANARIScene::RemoveMapper(viskores::IdComponent id)
{
  this->Mappers.erase(this->Mappers.begin() + id);
  this->UpdateWorld();
}

void ANARIScene::RemoveMapper(const char* name)
{
  std::string n = name;
  this->Mappers.erase(std::remove_if(this->Mappers.begin(),
                                     this->Mappers.end(),
                                     [&](auto& m) { return m.Mapper->GetName() == n; }),
                      this->Mappers.end());
  this->UpdateWorld();
}

void ANARIScene::RemoveAllMappers()
{
  this->Mappers.clear();
  this->UpdateWorld();
}

anari_cpp::Device ANARIScene::GetDevice() const
{
  return this->Device;
}

anari_cpp::World ANARIScene::GetANARIWorld()
{
  if (!this->World)
  {
    auto d = this->GetDevice();
    this->World = anari_cpp::newObject<anari_cpp::World>(d);
    anari_cpp::setParameter(d, this->World, "name", "scene");
    this->UpdateWorld();
  }

  return this->World;
}

void ANARIScene::UpdateWorld()
{
  if (!this->World)
    return; // nobody has asked for the world yet, so don't actually do anything

  auto d = this->GetDevice();

  std::vector<anari_cpp::Instance> instances;

  for (auto& m : this->Mappers)
  {
    auto i = m.Mapper->GetANARIInstance();
    if (i && m.Show)
      instances.push_back(i);
  }

  if (!instances.empty())
  {
    anari_cpp::setAndReleaseParameter(
      d, this->World, "instance", anari_cpp::newArray1D(d, instances.data(), instances.size()));
  }
  else
    anari_cpp::unsetParameter(d, this->World, "instance");

  anari_cpp::commitParameters(d, this->World);
}

} // namespace anari
} // namespace interop
} // namespace viskores
