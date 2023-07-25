// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkStringManager.h"

#include "vtkObjectFactory.h"

#include <algorithm>
#include <array>
#include <iostream>

VTK_ABI_NAMESPACE_BEGIN

constexpr vtkStringManager::Hash vtkStringManager::Invalid;

vtkStandardNewMacro(vtkStringManager);

void vtkStringManager::PrintSelf(ostream& os, vtkIndent indent)
{
  LockGuard guard(this->WriteLock);

  this->Superclass::PrintSelf(os, indent);
  vtkIndent i2 = indent.GetNextIndent();
  vtkIndent i3 = i2.GetNextIndent();
  os << indent << "Data: " << this->Data.size() << " entries\n";
  for (const auto& entry : this->Data)
  {
    os << i2 << entry.first << ": " << entry.second << '\n';
  }
  os << indent << "Sets: " << this->Sets.size() << " entries\n";
  for (const auto& entry : this->Sets)
  {
    os << i2 << entry.first << ": " << entry.second.size() << " entries\n";
    for (const auto& child : entry.second)
    {
      os << i3 << child << '\n';
    }
  }
}

vtkStringManager::Hash vtkStringManager::Manage(const std::string& ss)
{
  std::pair<Hash, bool> hp;
  {
    LockGuard lock(this->WriteLock);
    hp = this->ComputeInternalAndInsert(ss, lock);
  }
  return hp.first;
}

std::size_t vtkStringManager::Unmanage(Hash hh)
{
  LockGuard writeGuard(this->WriteLock);
  std::size_t num = this->UnmanageInternal(hh, writeGuard);
  return num;
}

const std::string& vtkStringManager::Value(Hash hh) const
{
  LockGuard guard(this->WriteLock);

  static const std::string empty;
  auto it = this->Data.find(hh);
  if (it == this->Data.end())
  {
    static bool once = false;
    if (!once)
    {
      once = true;
      vtkWarningMacro("Hash " << hh << " is missing from manager. Returning empty string.");
    }
    return empty;
  }
  return it->second;
}

vtkStringManager::Hash vtkStringManager::Find(const std::string& ss) const
{
  std::pair<Hash, bool> hh;
  {
    LockGuard lock(this->WriteLock);
    hh = this->ComputeInternal(ss, lock);
  }
  return hh.second ? hh.first : Invalid;
}

vtkStringManager::Hash vtkStringManager::Compute(const std::string& ss) const
{
  LockGuard lock(this->WriteLock);
  return this->ComputeInternal(ss, lock).first;
}

vtkStringManager::Hash vtkStringManager::Insert(const std::string& ss, Hash hh)
{
  LockGuard lock(this->WriteLock);
  bool didInsert = false;
  // Verify \a h is managed.
  if (this->Data.find(hh) == this->Data.end())
  {
    return Invalid;
  }
  // Insert \a h into \a ss.
  std::pair<Hash, bool> setHash{ Invalid, false };
  {
    setHash = this->ComputeInternalAndInsert(ss, lock);
    didInsert = this->Sets[setHash.first].insert(hh).second;
    (void)didInsert;
  }
  return setHash.first;
}

bool vtkStringManager::Insert(Hash ss, Hash hh)
{
  LockGuard lock(this->WriteLock);
  bool didInsert = false;
  // Verify \a ss and \a h are managed.
  if (this->Data.find(hh) == this->Data.end() || this->Data.find(ss) == this->Data.end())
  {
    return didInsert;
  }
  didInsert = this->Sets[ss].insert(hh).second;
  return didInsert;
}

bool vtkStringManager::Remove(const std::string& ss, Hash hh)
{
  bool didRemove = false;
  LockGuard lock(this->WriteLock);
  // Verify \a h is managed.
  if (this->Data.find(hh) == this->Data.end())
  {
    return Invalid;
  }
  // Remove \a hh from \a ss.
  auto setHash = this->ComputeInternalAndInsert(ss, lock);
  auto it = this->Sets.find(setHash.first);
  // Verify \a setHash is managed.
  if (it == this->Sets.end())
  {
    return didRemove;
  }
  didRemove = this->Sets[setHash.first].erase(hh) > 0;
  if (didRemove)
  {
    if (this->Sets[setHash.first].empty())
    {
      this->Sets.erase(setHash.first);
    }
  }
  return didRemove;
}

bool vtkStringManager::Remove(Hash ss, Hash hh)
{
  bool didRemove = false;
  LockGuard lock(this->WriteLock);
  auto hit = this->Data.find(hh);
  auto sit = this->Sets.find(ss);
  // Verify \a h is managed and \a ss is a set.
  if (hit == this->Data.end() || sit == this->Sets.end())
  {
    return false;
  }
  // Remove \a h from \a ss.
  didRemove = this->Sets[ss].erase(hh) > 0;
  if (didRemove)
  {
    if (this->Sets[ss].empty())
    {
      this->Sets.erase(ss);
    }
  }
  return didRemove;
}

bool vtkStringManager::Contains(const std::string& ss, Hash hh) const
{
  LockGuard lock(this->WriteLock);
  auto setHash = this->ComputeInternal(ss, lock);
  auto sit = this->Sets.find(setHash.first);
  return (sit != this->Sets.end() && sit->second.find(hh) != sit->second.end());
}

bool vtkStringManager::Contains(Hash ss, Hash hh) const
{
  LockGuard lock(this->WriteLock);
  if (ss == Invalid)
  {
    auto mit = this->Data.find(hh);
    return mit != this->Data.end();
  }
  auto sit = this->Sets.find(ss);
  return (sit != this->Sets.end() && sit->second.find(hh) != sit->second.end());
}

vtkStringManager::Visit vtkStringManager::VisitMembers(Visitor visitor, Hash ss) const
{
  if (!visitor)
  {
    return vtkStringManager::Visit::Halt;
  }

  std::unordered_set<Hash> currentSet;
  if (ss == Invalid)
  {
    // Copy keys from this->Data and iterate over them all.
    {
      LockGuard guard(this->WriteLock);
      for (const auto& entry : this->Data)
      {
        currentSet.insert(entry.first);
      }
    }
    // Iterate over this->Data.
    for (const auto& entry : currentSet)
    {
      if (visitor(entry) == vtkStringManager::Visit::Halt)
      {
        return vtkStringManager::Visit::Halt;
      }
    }
    return vtkStringManager::Visit::Continue;
  }

  // Copy values from this->Sets[hh] and iterate over them all.
  {
    LockGuard guard(this->WriteLock);
    auto sit = this->Sets.find(ss);
    if (sit == this->Sets.end())
    {
      return vtkStringManager::Visit::Continue;
    }
    currentSet = sit->second;
  }
  for (const auto& entry : currentSet)
  {
    if (visitor(entry) == vtkStringManager::Visit::Halt)
    {
      return vtkStringManager::Visit::Halt;
    }
  }

  return vtkStringManager::Visit::Continue;
}

vtkStringManager::Visit vtkStringManager::VisitSets(Visitor visitor) const
{
  if (!visitor)
  {
    return vtkStringManager::Visit::Halt;
  }

  std::unordered_set<Hash> setKeys;
  {
    LockGuard guard(this->WriteLock);
    for (const auto& entry : this->Sets)
    {
      setKeys.insert(entry.first);
    }
  }
  // Iterate over the keys.
  for (const auto& key : setKeys)
  {
    if (visitor(key) == vtkStringManager::Visit::Halt)
    {
      return vtkStringManager::Visit::Halt;
    }
  }

  return vtkStringManager::Visit::Continue;
}

void vtkStringManager::Reset()
{
  LockGuard writeGuard(this->WriteLock);
  this->Data.clear();
  this->Sets.clear();
}

std::pair<vtkStringManager::Hash, bool> vtkStringManager::ComputeInternal(
  const std::string& ss, const LockGuard& vtkNotUsed(proofOfLock)) const
{
  /// Same as compute() but does not lock (you must hold WriteLock upon entry).
  std::pair<Hash, bool> result{ vtkStringToken::StringHash(ss.data(), ss.size()), false };
  while (true)
  {
    auto it = this->Data.find(result.first);
    if (it == this->Data.end())
    {
      return result;
    }
    else if (it->second == ss)
    {
      result.second = true;
      return result;
    }
    vtkWarningMacro(
      "String token collision " << ss << " and " << it->second << " both " << it->first << ".");
    ++result.first;
  }
}

std::pair<vtkStringManager::Hash, bool> vtkStringManager::ComputeInternalAndInsert(
  const std::string& ss, const LockGuard& proofOfLock)
{
  std::pair<Hash, bool> result = this->ComputeInternal(ss, proofOfLock);
  if (result.first != Invalid)
  {
    this->Data[result.first] = ss;
    result.second = true;
  }
  return result;
}

std::size_t vtkStringManager::UnmanageInternal(Hash hh, const LockGuard& proofOfLock)
{
  std::size_t num = 0;
  auto it = this->Data.find(hh);
  if (it == this->Data.end())
  {
    return num;
  }
  auto members = this->Sets.find(hh);
  if (members != this->Sets.end())
  {
    // Erase all sets contained in this set recursively.
    for (auto member : members->second)
    {
      num += this->UnmanageInternal(member, proofOfLock);
    }
  }
  num += this->Data.erase(hh);
  return num;
}

VTK_ABI_NAMESPACE_END
