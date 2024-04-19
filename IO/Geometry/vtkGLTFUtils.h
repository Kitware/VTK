// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @brief   Helper functions for glTF parsing and validation.
 *
 * vtkGLTFUtils is a helper namespace that contains various functions to help
 * with the parsing and validation of JSON-formatted glTF files.
 * More specifically, these functions add existence and type verifications
 * before extracting Json values.
 * Another function helps check the document's version against supported glTF versions
 */

#ifndef vtkGLTFUtils_h
#define vtkGLTFUtils_h

#include "vtkABINamespace.h"

#include <vtk_nlohmannjson.h>
#include VTK_NLOHMANN_JSON(json.hpp)

#include <string> // For string
#include <vector> // For vector

VTK_ABI_NAMESPACE_BEGIN
class vtkResourceStream;
class vtkURILoader;
VTK_ABI_NAMESPACE_END

namespace vtkGLTFUtils
{
VTK_ABI_NAMESPACE_BEGIN
using ChunkInfoType = std::pair<std::string, uint32_t>;
// Binary glTF constants
static constexpr uint32_t GLBWordSize = 4;
static constexpr uint32_t GLBHeaderSize = 12;
static constexpr uint32_t GLBChunkHeaderSize = 8;
static constexpr uint32_t GLBVersion = 2;

/**
 * Checks various binary glTF elements for validity.
 * Checks: version, file length, JSON chunk presence as first chunk, chunk type name size, chunk
 * data size
 */
bool ValidateGLBFile(const std::string& magic, uint32_t version, uint32_t fileLength,
  std::vector<vtkGLTFUtils::ChunkInfoType> chunkInfo);

/**
 * Extract all header information from a binary glTF file
 */
bool ExtractGLBFileInformation(vtkResourceStream* stream, uint32_t& version, uint32_t& fileLength,
  uint32_t glbStart, std::vector<vtkGLTFUtils::ChunkInfoType>& chunkInfo);

/**
 * Get int value from Json variable, with existence and type checks.
 */
bool GetIntValue(const nlohmann::json& root, const std::string& key, int& value);

/**
 * Get int value from Json variable, with existence and type checks.
 */
bool GetUIntValue(const nlohmann::json& root, const std::string& key, unsigned int& value);

/**
 * Get double value from Json variable, with existence and type checks.
 */
bool GetDoubleValue(const nlohmann::json& root, const std::string& key, double& value);

/**
 * Get string value from Json variable, with existence and type checks.
 */
bool GetStringValue(const nlohmann::json& root, const std::string& key, std::string& value);

/**
 * Get bool value from Json variable, with existence and type checks.
 */
bool GetBoolValue(const nlohmann::json& root, const std::string& key, bool& value);

/**
 * Get int array from Json variable, with existence and type checks.
 */
bool GetIntArray(const nlohmann::json& root, const std::string& key, std::vector<int>& value);

/**
 * Get int array from Json variable, with existence and type checks.
 */
bool GetUIntArray(
  const nlohmann::json& root, const std::string& key, std::vector<unsigned int>& value);

/**
 * Get float array from Json variable, with existence and type checks.
 */
bool GetFloatArray(const nlohmann::json& root, const std::string& key, std::vector<float>& value);

/**
 * Get double array from Json variable, with existence and type checks.
 */
bool GetDoubleArray(const nlohmann::json& root, const std::string& key, std::vector<double>& value);

/**
 * Check document version. Currently supporting glTF 2.0 only.
 */
bool CheckVersion(const nlohmann::json& glTFAsset);

/**
 * Load binary buffer from uri information. Uri can be a base 64 data-uri or file path.
 */
bool GetBinaryBufferFromUri(
  const std::string& uri, vtkURILoader* loader, std::vector<char>& buffer, size_t bufferSize);

VTK_ABI_NAMESPACE_END
}

#endif

// VTK-HeaderTest-Exclude: vtkGLTFUtils.h
