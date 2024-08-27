// © Kitware, Inc. See license.md for details.
#include "token/Manager.h"
#include "token/Token.h"

#include <algorithm>
#include <array>
#include <iostream>

token_BEGIN_NAMESPACE

void Manager::printSelf(std::ostream& os, int indent)
{
  LockGuard guard(this->m_writeLock);

  int indent2 = indent + 2;
  int indent3 = indent2 + 2;
  os << std::string(' ', indent) << "Data: " << m_data.size() << " entries\n";
  for (const auto& entry : m_data)
  {
    os << std::string(' ', indent2) << entry.first << ": " << entry.second << '\n';
  }
  os << std::string(' ', indent) << "Sets: " << m_sets.size() << " entries\n";
  for (const auto& entry : m_sets)
  {
    os << std::string(' ', indent2) << entry.first << ": " << entry.second.size() << " entries\n";
    for (const auto& child : entry.second)
    {
      os << std::string(' ', indent3) << child << '\n';
    }
  }
}

Hash Manager::manage(const std::string& ss)
{
  std::pair<Hash, bool> hp;
  {
    LockGuard lock(this->m_writeLock);
    hp = this->computeInternalAndInsert(ss, lock);
  }
  return hp.first;
}

std::size_t Manager::unmanage(Hash hh)
{
  LockGuard writeGuard(this->m_writeLock);
  std::size_t num = this->unmanageInternal(hh, writeGuard);
  return num;
}

const std::string& Manager::value(Hash hh) const
{
  LockGuard guard(this->m_writeLock);

  static const std::string empty;
  auto it = m_data.find(hh);
  if (it == m_data.end())
  {
    static bool once = false;
    if (!once)
    {
      once = true;
      std::cerr << "Hash " << hh << " is missing from manager. Returning empty string.\n";
    }
    return empty;
  }
  return it->second;
}

Hash Manager::find(const std::string& ss) const
{
  std::pair<Hash, bool> hh;
  {
    LockGuard lock(this->m_writeLock);
    hh = this->computeInternal(ss, lock);
  }
  return hh.second ? hh.first : Invalid();
}

Hash Manager::compute(const std::string& ss) const
{
  LockGuard lock(this->m_writeLock);
  return this->computeInternal(ss, lock).first;
}

Hash Manager::insert(const std::string& ss, Hash hh)
{
  LockGuard lock(this->m_writeLock);
  bool didInsert = false;
  // Verify \a h is managed.
  if (m_data.find(hh) == m_data.end())
  {
    return Invalid();
  }
  // Insert \a h into \a ss.
  std::pair<Hash, bool> setHash{ Invalid(), false };
  {
    setHash = this->computeInternalAndInsert(ss, lock);
    didInsert = m_sets[setHash.first].insert(hh).second;
    (void)didInsert;
  }
  return setHash.first;
}

bool Manager::insert(Hash ss, Hash hh)
{
  LockGuard lock(this->m_writeLock);
  bool didInsert = false;
  // Verify \a ss and \a h are managed.
  if (m_data.find(hh) == m_data.end() || m_data.find(ss) == m_data.end())
  {
    return didInsert;
  }
  didInsert = m_sets[ss].insert(hh).second;
  return didInsert;
}

bool Manager::remove(const std::string& ss, Hash hh)
{
  bool didRemove = false;
  LockGuard lock(this->m_writeLock);
  // Verify \a h is managed.
  if (m_data.find(hh) == m_data.end())
  {
    return Invalid();
  }
  // Remove \a hh from \a ss.
  auto setHash = this->computeInternalAndInsert(ss, lock);
  auto it = m_sets.find(setHash.first);
  // Verify \a setHash is managed.
  if (it == m_sets.end())
  {
    return didRemove;
  }
  didRemove = m_sets[setHash.first].erase(hh) > 0;
  if (didRemove)
  {
    if (m_sets[setHash.first].empty())
    {
      m_sets.erase(setHash.first);
    }
  }
  return didRemove;
}

bool Manager::remove(Hash ss, Hash hh)
{
  bool didRemove = false;
  LockGuard lock(this->m_writeLock);
  auto hit = m_data.find(hh);
  auto sit = m_sets.find(ss);
  // Verify \a h is managed and \a ss is a set.
  if (hit == m_data.end() || sit == m_sets.end())
  {
    return false;
  }
  // Remove \a h from \a ss.
  didRemove = m_sets[ss].erase(hh) > 0;
  if (didRemove)
  {
    if (m_sets[ss].empty())
    {
      m_sets.erase(ss);
    }
  }
  return didRemove;
}

bool Manager::contains(const std::string& ss, Hash hh) const
{
  LockGuard lock(this->m_writeLock);
  auto setHash = this->computeInternal(ss, lock);
  auto sit = m_sets.find(setHash.first);
  return (sit != m_sets.end() && sit->second.find(hh) != sit->second.end());
}

bool Manager::contains(Hash ss, Hash hh) const
{
  LockGuard lock(this->m_writeLock);
  if (ss == Invalid())
  {
    auto mit = m_data.find(hh);
    return mit != m_data.end();
  }
  auto sit = m_sets.find(ss);
  return (sit != m_sets.end() && sit->second.find(hh) != sit->second.end());
}

Manager::Visit Manager::visitMembers(Visitor visitor, Hash ss) const
{
  if (!visitor)
  {
    return Manager::Visit::Halt;
  }

  std::unordered_set<Hash> currentSet;
  if (ss == Invalid())
  {
    // Copy keys from m_data and iterate over them all.
    {
      LockGuard guard(this->m_writeLock);
      for (const auto& entry : m_data)
      {
        currentSet.insert(entry.first);
      }
    }
    // Iterate over m_data.
    for (const auto& entry : currentSet)
    {
      if (visitor(entry) == Manager::Visit::Halt)
      {
        return Manager::Visit::Halt;
      }
    }
    return Manager::Visit::Continue;
  }

  // Copy values from m_sets[hh] and iterate over them all.
  {
    LockGuard guard(this->m_writeLock);
    auto sit = m_sets.find(ss);
    if (sit == m_sets.end())
    {
      return Manager::Visit::Continue;
    }
    currentSet = sit->second;
  }
  for (const auto& entry : currentSet)
  {
    if (visitor(entry) == Manager::Visit::Halt)
    {
      return Manager::Visit::Halt;
    }
  }

  return Manager::Visit::Continue;
}

Manager::Visit Manager::visitSets(Visitor visitor) const
{
  if (!visitor)
  {
    return Manager::Visit::Halt;
  }

  std::unordered_set<Hash> setKeys;
  {
    LockGuard guard(this->m_writeLock);
    for (const auto& entry : m_sets)
    {
      setKeys.insert(entry.first);
    }
  }
  // Iterate over the keys.
  for (const auto& key : setKeys)
  {
    if (visitor(key) == Manager::Visit::Halt)
    {
      return Manager::Visit::Halt;
    }
  }

  return Manager::Visit::Continue;
}

void Manager::reset()
{
  LockGuard writeGuard(this->m_writeLock);
  m_data.clear();
  m_sets.clear();
}

std::pair<Hash, bool> Manager::computeInternal(
  const std::string& ss, const LockGuard& proofOfLock) const
{
  (void) proofOfLock;
  /// Same as compute() but does not lock (you must hold m_writeLock upon entry).
  std::pair<Hash, bool> result{ Token::stringHash(ss.data(), ss.size()), false };
  while (true)
  {
    auto it = m_data.find(result.first);
    if (it == m_data.end())
    {
      return result;
    }
    else if (it->second == ss)
    {
      result.second = true;
      return result;
    }
    std::cerr << "String token collision " << ss << " and " << it->second << " both " << it->first << ".\n";
    ++result.first;
  }
}

std::pair<Hash, bool> Manager::computeInternalAndInsert(
  const std::string& ss, const LockGuard& proofOfLock)
{
  std::pair<Hash, bool> result = this->computeInternal(ss, proofOfLock);
  if (result.first != Invalid())
  {
    m_data[result.first] = ss;
    result.second = true;
  }
  return result;
}

std::size_t Manager::unmanageInternal(Hash hh, const LockGuard& proofOfLock)
{
  std::size_t num = 0;
  auto it = m_data.find(hh);
  if (it == m_data.end())
  {
    return num;
  }
  auto members = m_sets.find(hh);
  if (members != m_sets.end())
  {
    // Erase all sets contained in this set recursively.
    for (auto member : members->second)
    {
      num += this->unmanageInternal(member, proofOfLock);
    }
  }
  num += m_data.erase(hh);
  return num;
}

void Manager::addTranslation(Hash source, Hash target)
{
  LockGuard guard(this->m_writeLock);
  m_translation[source] = target;
}

Hash Manager::getTranslation(Hash source) const
{
  LockGuard guard(this->m_writeLock);
  auto it = m_translation.find(source);
  if (it == m_translation.end())
  {
    return source;
  }
  return it->second;
}

std::size_t Manager::resetTranslations()
{
  LockGuard guard(this->m_writeLock);
  std::size_t sz = m_translation.size();
  m_translation.clear();
  return sz;
}

token_CLOSE_NAMESPACE
