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
#include <viskores/io/FileUtils.h>

#include <viskores/cont/ErrorBadValue.h>

#include <algorithm>
#include <cctype>
#include <errno.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#endif

#ifdef __APPLE__
#include <AvailabilityMacros.h>
#endif

// std::filesystem::absolute (and several other std::filesystem operations) are
// marked unavailable when targeting macOS deployment targets older than 10.15,
// so a manual implementation is used in that case instead.
#if defined(__APPLE__) && MAC_OS_X_VERSION_MIN_REQUIRED < 101500
#define VISKORES_FILESYSTEM_ABSOLUTE_UNAVAILABLE
#include <limits.h>
#include <unistd.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#else
#include <filesystem>
#endif

namespace viskores
{
namespace io
{

bool EndsWith(const std::string& value, const std::string& ending)
{
  if (ending.size() > value.size())
  {
    return false;
  }
  return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

std::string Filename(const std::string& filePath)
{
  // TODO (nadavi): Once we get c++17 installed uncomment this
  // std::filesystem::path path(filePath);
  // return path.filename();

#ifdef _WIN32
  auto lastSlashPos = filePath.rfind(GetWindowsPathSeperator(filePath));
#else
  auto lastSlashPos = filePath.rfind('/');
#endif
  if (lastSlashPos != std::string::npos)
  {
    return filePath.substr(lastSlashPos + 1);
  }
  return filePath;
}

std::string ParentPath(const std::string& filePath)
{
  // TODO (nadavi): Once we get c++17 installed uncomment this
  // std::filesystem::path path(filePath);
  // return path.parent_path();

#ifdef _WIN32
  auto lastSlashPos = filePath.rfind(GetWindowsPathSeperator(filePath));
#else
  auto lastSlashPos = filePath.rfind('/');
#endif
  if (lastSlashPos != std::string::npos)
  {
    return filePath.substr(0, lastSlashPos);
  }
  return "";
}

bool CreateDirectoriesFromFilePath(const std::string& filePath)
{
  // TODO (nadavi): Once we get c++17 installed uncomment this
  // auto dir = ParentPath(filePath);
  // if (dir.empty())
  // {
  //   return false;
  // }
  // return std::filesystem::create_directories(dir);

  auto dir = ParentPath(filePath);
  if (dir.empty())
  {
    return false;
  }

#ifdef _WIN32
  auto ret = _mkdir(dir.c_str());
#else
  mode_t mode = 0755;
  auto ret = mkdir(dir.c_str(), mode);
#endif

  if (ret == 0)
  {
    return true;
  }

  switch (errno)
  {
    case ENOENT:
    {
      if (!CreateDirectoriesFromFilePath(dir))
      {
        return false;
      }
      return CreateDirectoriesFromFilePath(filePath);
    }
    case EEXIST:
      return false;
    default:
      return false;
  }
}

bool IsAbsolutePath(const std::string& filePath)
{
  if (filePath.empty())
    return false;

#ifdef _WIN32
  if (filePath[0] == '/' || filePath[0] == '\\')
    return true;

  if (filePath.size() < 3)
    return false;

  if (!std::isalpha(static_cast<unsigned char>(filePath[0])) || filePath[1] != ':')
    return false;

  return filePath[2] == '/' || filePath[2] == '\\';
#else
  return filePath[0] == '/';
#endif
}

std::string MakeAbsolutePath(const std::string& filePath)
{
  if (IsAbsolutePath(filePath))
    return filePath;

#ifdef VISKORES_FILESYSTEM_ABSOLUTE_UNAVAILABLE
  char buffer[PATH_MAX];
  if (getcwd(buffer, PATH_MAX) == nullptr)
  {
    throw viskores::cont::ErrorBadValue("Unable to determine the current working directory.");
  }

  return MergePaths(std::string(buffer), filePath);
#else
  return std::filesystem::absolute(filePath).string();
#endif
}

std::string MergePaths(const std::string& filePathPrefix, const std::string& filePathSuffix)
{
  // TODO (nadavi): Once we get c++17 installed uncomment this
  // NOTE: This function may not directly mimic the c++ filesystem '/' behavior
  //       the edge case tests will probably need to be updated when switching.
  // std::filesystem::path prefix(filePathPrefix);
  // std::filesystem::path suffix(filePathSuffix);
  // std::filesystem::path fullPath = prefix / suffix;
  // return fullPath.string();

  auto prefix = filePathPrefix;
  auto suffix = filePathSuffix;
  char prefixPathSeperator = '/';
  char suffixPathSeperator = '/';

  if (prefix.empty() && suffix.empty())
  {
    return "";
  }
  else if (prefix.empty())
  {
    return suffix;
  }
  else if (suffix.empty())
  {
    return prefix;
  }

#ifdef _WIN32
  prefixPathSeperator = GetWindowsPathSeperator(prefix);
  suffixPathSeperator = suffix[0] == '/' || suffix[0] == '\\' ? suffix[0] : prefixPathSeperator;
#endif

  if (prefix[prefix.length() - 1] == prefixPathSeperator)
  {
    prefix = prefix.substr(0, prefix.length() - 1);
  }
  if (suffix[0] == suffixPathSeperator)
  {
    suffix = suffix.substr(1, suffix.length());
  }
  return prefix + prefixPathSeperator + suffix;
}

std::string PrefixStringToFilename(const std::string& filePath, const std::string& prefix)
{
  auto parentPath = ParentPath(filePath);
  auto filename = Filename(filePath);
  filename = prefix + filename;
  return MergePaths(parentPath, filename);
}

char GetWindowsPathSeperator(const std::string& filePath)
{
  auto slashType = filePath.rfind('/');
  if (slashType == std::string::npos)
  {
    return '\\';
  }
  return '/';
}

} // namespace viskores::io
} // namespace viskores
