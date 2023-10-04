// © Kitware, Inc. See license.md for details.
#include "token/Token.h"
#include "token/Manager.h"

#include <cstring>
#include <exception>
#include <mutex>
#include <thread>

token_BEGIN_NAMESPACE

static std::mutex s_managerLock;
std::shared_ptr<Manager> Token::m_manager;

Token::Token(const char* data, std::size_t size)
{
  if (!data)
  {
    m_id = token_NAMESPACE::Invalid();
  }
  else
  {
    if (size == std::string::npos)
    {
      size = std::strlen(data);
    }
    m_id = Token::getManagerInternal()->manage(std::string(data, size));
  }
}

Token::Token(const std::string& data)
{
  m_id = Token::getManagerInternal()->manage(data);
}

const std::string& Token::data() const
{
  return Token::getManagerInternal()->value(m_id);
}

bool Token::hasData() const
{
  return Token::getManagerInternal()->contains(m_id);
}

Manager* Token::getManager()
{
  return Token::getManagerInternal();
}

Manager* Token::getManagerInternal()
{
  if (!Token::m_manager)
  {
    std::lock_guard<std::mutex> lock(s_managerLock);
    if (!Token::m_manager)
    {
      Token::m_manager = std::make_shared<Manager>();
    }
  }
  return Token::m_manager.get();
}

bool Token::operator==(const Token& other) const
{
  return m_id == other.m_id;
}

bool Token::operator!=(const Token& other) const
{
  return m_id != other.m_id;
}

bool Token::operator<(const Token& other) const
{
  return this->data() < other.data();
}

bool Token::operator>(const Token& other) const
{
  return this->data() > other.data();
}

bool Token::operator<=(const Token& other) const
{
  return this->data() <= other.data();
}

bool Token::operator>=(const Token& other) const
{
  return this->data() >= other.data();
}

bool operator==(const std::string& a, const Token& b)
{
  return a == b.data();
}
bool operator!=(const std::string& a, const Token& b)
{
  return a != b.data();
}
bool operator>(const std::string& a, const Token& b)
{
  return a > b.data();
}
bool operator<(const std::string& a, const Token& b)
{
  return a < b.data();
}
bool operator>=(const std::string& a, const Token& b)
{
  return a >= b.data();
}
bool operator<=(const std::string& a, const Token& b)
{
  return a <= b.data();
}

bool operator==(const Token& a, const std::string& b)
{
  return a.data() == b;
}
bool operator!=(const Token& a, const std::string& b)
{
  return a.data() != b;
}
bool operator>(const Token& a, const std::string& b)
{
  return a.data() > b;
}
bool operator<(const Token& a, const std::string& b)
{
  return a.data() < b;
}
bool operator>=(const Token& a, const std::string& b)
{
  return a.data() >= b;
}
bool operator<=(const Token& a, const std::string& b)
{
  return a.data() <= b;
}

bool operator==(const char* a, const Token& b)
{
  return std::string(a) == b.data();
}
bool operator!=(const char* a, const Token& b)
{
  return std::string(a) != b.data();
}
bool operator>(const char* a, const Token& b)
{
  return std::string(a) > b.data();
}
bool operator<(const char* a, const Token& b)
{
  return std::string(a) < b.data();
}
bool operator>=(const char* a, const Token& b)
{
  return std::string(a) >= b.data();
}
bool operator<=(const char* a, const Token& b)
{
  return std::string(a) <= b.data();
}

bool operator==(const Token& a, const char* b)
{
  return a.data() == std::string(b);
}
bool operator!=(const Token& a, const char* b)
{
  return a.data() != std::string(b);
}
bool operator>(const Token& a, const char* b)
{
  return a.data() > std::string(b);
}
bool operator<(const Token& a, const char* b)
{
  return a.data() < std::string(b);
}
bool operator>=(const Token& a, const char* b)
{
  return a.data() >= std::string(b);
}
bool operator<=(const Token& a, const char* b)
{
  return a.data() <= std::string(b);
}

token_CLOSE_NAMESPACE
