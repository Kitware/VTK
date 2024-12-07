// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkStringToken.h"

#include <cstring>
#include <exception>
#include <mutex>
#include <thread>

#include <token/Hash.h>
#include <token/Manager.h>

VTK_ABI_NAMESPACE_BEGIN

vtkStringToken::vtkStringToken(const char* data, std::size_t size)
{
  if (!data)
  {
    this->Id = token_NAMESPACE::Invalid();
  }
  else
  {
    if (size == std::string::npos)
    {
      size = std::strlen(data);
    }
    this->Id = token_NAMESPACE::Token::getManager()->manage(std::string(data, size));
  }
}

vtkStringToken::vtkStringToken(const std::string& data)
{
  this->Id = token_NAMESPACE::Token::getManager()->manage(data);
}

const std::string& vtkStringToken::Data() const
{
  return token_NAMESPACE::Token::getManager()->value(this->Id);
}

bool vtkStringToken::IsValid() const
{
  return this->Id != token_NAMESPACE::Invalid();
}

bool vtkStringToken::HasData() const
{
  return token_NAMESPACE::Token::getManager()->contains(this->Id);
}

vtkStringToken::Hash vtkStringToken::InvalidHash()
{
  return token_NAMESPACE::Invalid();
}

bool vtkStringToken::AddChild(vtkStringToken member)
{
  if (!this->IsValid() || !member.IsValid())
  {
    return false;
  }

  bool result = token_NAMESPACE::Token::getManager()->insert(this->GetId(), member.GetId());
  return result;
}

bool vtkStringToken::RemoveChild(vtkStringToken member)
{
  if (!this->IsValid() || !member.IsValid())
  {
    return false;
  }

  auto result = token_NAMESPACE::Token::getManager()->remove(this->GetId(), member.GetId());
  return result;
}

std::unordered_set<vtkStringToken> vtkStringToken::Children(bool recursive)
{
  std::unordered_set<vtkStringToken> result;
  auto* manager = token_NAMESPACE::Token::getManager();
  token_NAMESPACE::Manager::Visitor visitor = [&result, &manager, &visitor, recursive](
                                                token_NAMESPACE::Hash member)
  {
    if (recursive && result.find(member) == result.end())
    {
      manager->visitMembers(visitor, member);
    }
    result.insert(vtkStringToken(member));
    return token_NAMESPACE::Manager::Visit::Continue;
  };
  manager->visitMembers(visitor, this->GetId());
  return result;
}

std::unordered_set<vtkStringToken> vtkStringToken::AllGroups()
{
  std::unordered_set<vtkStringToken> result;
  auto* manager = token_NAMESPACE::Token::getManager();
  token_NAMESPACE::Manager::Visitor visitor = [&result](token_NAMESPACE::Hash member)
  {
    result.insert(vtkStringToken(member));
    return token_NAMESPACE::Manager::Visit::Continue;
  };
  manager->visitSets(visitor);
  return result;
}

bool vtkStringToken::operator==(const vtkStringToken& other) const
{
  return this->Id == other.Id;
}

bool vtkStringToken::operator!=(const vtkStringToken& other) const
{
  return this->Id != other.Id;
}

bool vtkStringToken::operator<(const vtkStringToken& other) const
{
  return this->Data() < other.Data();
}

bool vtkStringToken::operator>(const vtkStringToken& other) const
{
  return this->Data() > other.Data();
}

bool vtkStringToken::operator<=(const vtkStringToken& other) const
{
  return this->Data() <= other.Data();
}

bool vtkStringToken::operator>=(const vtkStringToken& other) const
{
  return this->Data() >= other.Data();
}

bool operator==(const std::string& a, const vtkStringToken& b)
{
  return a == b.Data();
}
bool operator!=(const std::string& a, const vtkStringToken& b)
{
  return a != b.Data();
}
bool operator>(const std::string& a, const vtkStringToken& b)
{
  return a > b.Data();
}
bool operator<(const std::string& a, const vtkStringToken& b)
{
  return a < b.Data();
}
bool operator>=(const std::string& a, const vtkStringToken& b)
{
  return a >= b.Data();
}
bool operator<=(const std::string& a, const vtkStringToken& b)
{
  return a <= b.Data();
}

bool operator==(const vtkStringToken& a, const std::string& b)
{
  return a.Data() == b;
}
bool operator!=(const vtkStringToken& a, const std::string& b)
{
  return a.Data() != b;
}
bool operator>(const vtkStringToken& a, const std::string& b)
{
  return a.Data() > b;
}
bool operator<(const vtkStringToken& a, const std::string& b)
{
  return a.Data() < b;
}
bool operator>=(const vtkStringToken& a, const std::string& b)
{
  return a.Data() >= b;
}
bool operator<=(const vtkStringToken& a, const std::string& b)
{
  return a.Data() <= b;
}

bool operator==(const char* a, const vtkStringToken& b)
{
  return std::string(a) == b.Data();
}
bool operator!=(const char* a, const vtkStringToken& b)
{
  return std::string(a) != b.Data();
}
bool operator>(const char* a, const vtkStringToken& b)
{
  return std::string(a) > b.Data();
}
bool operator<(const char* a, const vtkStringToken& b)
{
  return std::string(a) < b.Data();
}
bool operator>=(const char* a, const vtkStringToken& b)
{
  return std::string(a) >= b.Data();
}
bool operator<=(const char* a, const vtkStringToken& b)
{
  return std::string(a) <= b.Data();
}

bool operator==(const vtkStringToken& a, const char* b)
{
  return a.Data() == std::string(b);
}
bool operator!=(const vtkStringToken& a, const char* b)
{
  return a.Data() != std::string(b);
}
bool operator>(const vtkStringToken& a, const char* b)
{
  return a.Data() > std::string(b);
}
bool operator<(const vtkStringToken& a, const char* b)
{
  return a.Data() < std::string(b);
}
bool operator>=(const vtkStringToken& a, const char* b)
{
  return a.Data() >= std::string(b);
}
bool operator<=(const vtkStringToken& a, const char* b)
{
  return a.Data() <= std::string(b);
}

VTK_ABI_NAMESPACE_END
