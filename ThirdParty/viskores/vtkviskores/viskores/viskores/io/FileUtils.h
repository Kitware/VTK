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
#ifndef viskores_io_FileUtils_h
#define viskores_io_FileUtils_h

#include <viskores/io/viskores_io_export.h>

#include <string>

namespace viskores
{
namespace io
{

/// \brief Checks if a provided string ends with a specific substring.
VISKORES_IO_EXPORT bool EndsWith(const std::string& value, const std::string& ending);

/// \brief Returns the filename component of a filePath string.
/// Mimics the functionality of the c++17 filesystem::path filename function
VISKORES_IO_EXPORT std::string Filename(const std::string& filePath);

/// \brief Returns the directory component of a filePath string.
/// Mimics the functionality of the c++17 filesystem::path parent_path function
VISKORES_IO_EXPORT std::string ParentPath(const std::string& filePath);

/// \brief Creates all the directories found in a given filePath component
/// if they don't exist. Only returns true if directories are actually created.
VISKORES_IO_EXPORT bool CreateDirectoriesFromFilePath(const std::string& filePath);

/// \brief Merges two filepath strings together using the correct system filepath seperator
/// EX: MergePaths("path/to/merge", "some/filename.txt") = "path/to/merge/some/filename.txt"
/// EX: MergePaths("path/to/merge/", "/some/filename.txt") = "path/to/merge/some/filename.txt"
VISKORES_IO_EXPORT std::string MergePaths(const std::string& filePathPrefix,
                                          const std::string& filePathSuffix);

/// \brief Takes the supplied prefix and prepends it to the filename for the provided filePath
/// EX: PrefixStringToFilename("/some/path/to/filename.txt", "prefix-") = "/some/path/to/prefix-filename.txt"
VISKORES_IO_EXPORT std::string PrefixStringToFilename(const std::string& filePath,
                                                      const std::string& prefix);

/// \brief determine which path separator to use for windows given a provided path
/// Should return one of either '\\' or '/' depending on what the provided path uses.
/// If no seperator is found it will just return '\\'
VISKORES_IO_EXPORT char GetWindowsPathSeperator(const std::string& filePath);

} // namespace viskores::io
} // namespace viskores

#endif //viskores_io_FileUtils_h
