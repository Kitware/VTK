// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkStringToken.h"
#include "vtkStringManager.h"

#include <cstring>
#include <exception>
#include <mutex>
#include <thread>

VTK_ABI_NAMESPACE_BEGIN

static std::mutex s_managerLock;
vtkSmartPointer<vtkStringManager> vtkStringToken::Manager;

vtkStringToken::vtkStringToken(const char* data, std::size_t size)
{
  if (!data)
  {
    this->Id = vtkStringManager::Invalid;
  }
  else
  {
    if (size == std::string::npos)
    {
      size = std::strlen(data);
    }
    this->Id = vtkStringToken::GetManagerInternal()->Manage(std::string(data, size));
  }
}

vtkStringToken::vtkStringToken(const std::string& data)
{
  this->Id = vtkStringToken::GetManagerInternal()->Manage(data);
}

const std::string& vtkStringToken::Data() const
{
  return vtkStringToken::GetManagerInternal()->Value(this->Id);
}

const vtkStringManager* vtkStringToken::GetManager()
{
  return vtkStringToken::GetManagerInternal();
}

vtkStringManager* vtkStringToken::GetManagerInternal()
{
  if (!vtkStringToken::Manager)
  {
    std::lock_guard<std::mutex> lock(s_managerLock);
    if (!vtkStringToken::Manager)
    {
      vtkStringToken::Manager = vtkSmartPointer<vtkStringManager>::New();
    }
  }
  return vtkStringToken::Manager.GetPointer();
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
