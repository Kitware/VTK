//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================
#include "token/json/jsonManager.h"

#include <iostream>
#include <thread>

token_BEGIN_NAMESPACE

void to_json(json& j, const std::shared_ptr<Manager>& m)
{
  json::object_t members;
  json::object_t sets;

  // Record the size (in bytes) of hashes on this platform.
  // This will be 4 or 8 (32 or 64 bits).
  j["hash_size"] = sizeof(std::size_t);

  m->visitMembers([&members, &m](Hash h) {
    // We store the "inverse" of the string-manager's map because
    // JSON keys must be strings and we would rather avoid the overhead
    // and potential ambiguities of conversion where possible, although
    // the sets below do some conversion.
    members[m->value(h)] = h;
    return Manager::Visit::Continue;
  });
  j["members"] = members;

  m->visitSets([&sets, &m](Hash setHash) {
    json::array_t children;
    m->visitMembers(
      [&children](Hash h) {
        children.push_back(h);
        return Manager::Visit::Continue;
      },
      setHash);
    sets[std::to_string(setHash)] = children;
    return Manager::Visit::Continue;
  });
  if (!sets.empty())
  {
    j["sets"] = sets;
  }
}

void from_json(const json& j, std::shared_ptr<Manager>& m)
{
  if (!m.get() || j.is_null())
  {
    m = std::shared_ptr<token_NAMESPACE::Manager>(new token_NAMESPACE::Manager);
  }
  // Warn if we have 64-bit hash codes on a 32-bit machine:
  auto sit = j.find("hash_size");
  if (sit != j.end())
  {
    auto jsonSize = sit->get<std::size_t>();
    if (jsonSize > sizeof(std::size_t))
    {
      std::cerr
        << "Deserializing " << (jsonSize * 8) << "-bit hash codes " "on a "
        << (sizeof(std::size_t) * 8) << "-bit platform "
        "will likely not work. Expect downstream failures.\n";
    }
  }
  // NB: We do not call m->reset() here since people may
  // wish to deserialize multiple files to the same manager.
  auto mit = j.find("members");
  if (mit != j.end())
  {
    for (const auto& element : mit->items())
    {
      Hash oldHash = element.value().get<Hash>();
      Hash newHash = m->manage(element.key());
      if (newHash != oldHash)
      {
        m->addTranslation(oldHash, newHash);
      }
    }
    sit = j.find("sets");
    if (sit != j.end())
    {
      for (const auto& element : sit->items())
      {
        Hash oldSetHash = static_cast<Hash>(stoull(element.key(), nullptr));
        Hash newSetHash = m->getTranslation(oldSetHash);
        auto oldMembers = element.value().get<std::unordered_set<Hash>>();
        for (const auto& oldMember : oldMembers)
        {
          m->insert(newSetHash, m->getTranslation(oldMember));
        }
      }
    }
  }
}

token_CLOSE_NAMESPACE
