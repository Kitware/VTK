//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/ExternalDataRegistry.h>

#include <mutex>
#include <unordered_map>

namespace fides
{
namespace io
{

class ExternalDataRegistry::InternalImpl
{
public:
  mutable std::mutex RegistryMutex;
  std::unordered_map<std::string, std::shared_ptr<fides::DataContainer>> ObjectMap;
  uint64_t ObjectCount = 0;
};

ExternalDataRegistry::ExternalDataRegistry()
  : Internals(std::make_unique<InternalImpl>())
{
}

ExternalDataRegistry::~ExternalDataRegistry() = default;

ExternalDataRegistry& ExternalDataRegistry::Instance()
{
  static ExternalDataRegistry registry;
  return registry;
}

std::string ExternalDataRegistry::Register(std::shared_ptr<fides::DataContainer> data)
{
  std::lock_guard<std::mutex> guard(this->Internals->RegistryMutex);
  const std::string token = "ext-" + std::to_string(this->Internals->ObjectCount);
  this->Internals->ObjectCount += 1;
  this->Internals->ObjectMap[token] = std::move(data);
  return token;
}

std::shared_ptr<fides::DataContainer> ExternalDataRegistry::Get(const std::string& token) const
{
  std::lock_guard<std::mutex> guard(this->Internals->RegistryMutex);
  auto it = this->Internals->ObjectMap.find(token);
  return it == this->Internals->ObjectMap.end() ? nullptr : it->second;
}

void ExternalDataRegistry::Unregister(const std::string& token)
{
  std::lock_guard<std::mutex> guard(this->Internals->RegistryMutex);
  this->Internals->ObjectMap.erase(token);
}

}
}
