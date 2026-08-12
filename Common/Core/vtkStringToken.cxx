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

std::ostream& operator<<(std::ostream& os, const vtkStringToken& t)
{
  os << "token(" << std::hex << t.GetId() << std::dec;
  if (t.HasData())
  {
    os << "(" << t.Data().size() << "," << t.Data() << ")";
  }
  os << ")";

  return os;
}

std::istream& operator>>(std::istream& is, vtkStringToken& tt)
{
  vtkStringToken::Hash hh = 0;
  static std::array<char, 6> keyword{ 't', 'o', 'k', 'e', 'n', '(' };
  // Require a fixed preamble of "token(".
  for (std::size_t ii = 0; ii < keyword.size(); ++ii)
  {
    if (is.peek() == keyword[ii])
    {
      is.get();
    }
    else
    {
      for (; ii > 0; --ii)
      {
        is.unget();
      }
      is.setstate(std::ios_base::failbit);
      return is;
    }
  }
  // Require 8 hex digits;
  std::size_t numUnget = keyword.size();
  for (int ii = 0; ii < 8; ++ii)
  {
    int digit = is.peek();
    if (digit >= '0' && digit <= '9')
    {
      hh = 16 * hh + (digit - '0');
      is.get();
    }
    else if (digit >= 'a' && digit <= 'f')
    {
      hh = 16 * hh + (digit - 'a' + 10);
      is.get();
    }
    else if (digit == ')')
    {
      // Proper end of token
      is.get();
      tt = vtkStringToken(hh);
      return is;
    }
    else if (digit == '(')
    {
      // Token includes its string generator; read the length and string,
      // inserting it into the token manager if needed.
      is.get();
      std::size_t dataLength;
      is >> dataLength;
      if (is.good())
      {
        std::string data;
        data.reserve(dataLength);
        bool ok = true;
        for (std::size_t jj = 0; jj < dataLength; ++jj)
        {
          int cc = is.get();
          if (is.good())
          {
            data.push_back(static_cast<char>(cc));
          }
          else
          {
            numUnget += ii + jj;
            ok = false;
            break;
          }
        }
        if (ok)
        {
          if (is.peek() == ')')
          {
            vtkStringToken dummy(data);
            tt = vtkStringToken(hh);
            return is;
          }
          else
          {
            numUnget += ii + dataLength;
          }
        }
      }
      else
      {
        numUnget += ii;
        break;
      }
    }
    else
    {
      numUnget = +ii;
      break;
    }
  }
  // We have failed.
  // Unget the digits we have processed so far (including the preamble string), then fail:
  for (; numUnget > 0; --numUnget)
  {
    is.unget();
  }
  is.setstate(std::ios_base::failbit);
  return is;
}

VTK_ABI_NAMESPACE_END
