// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkGLTFUtils.h"

#include "vtkResourceStream.h"
#include "vtkURILoader.h"
#include "vtksys/FStream.hxx"
#include "vtksys/RegularExpression.hxx"
#include "vtksys/SystemTools.hxx"

#include <fstream>
#include <iostream>

#define MIN_GLTF_VERSION "2.0"
namespace vtkGLTFUtils
{
//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
bool GetBoolValue(const nlohmann::json& root, const std::string& key, bool& value)
{
  auto it = root.find(key);
  if (it == root.end() || !it.value().is_boolean())
  {
    return false;
  }
  value = it.value();
  return true;
}

//------------------------------------------------------------------------------
bool GetIntValue(const nlohmann::json& root, const std::string& key, int& value)
{
  auto it = root.find(key);
  if (it == root.end() || !it.value().is_number_integer())
  {
    return false;
  }
  value = it.value();
  return true;
}

//------------------------------------------------------------------------------
bool GetUIntValue(const nlohmann::json& root, const std::string& key, unsigned int& value)
{
  auto it = root.find(key);
  if (it == root.end() || !it.value().is_number_unsigned())
  {
    return false;
  }
  value = it.value();
  return true;
}

//------------------------------------------------------------------------------
bool GetDoubleValue(const nlohmann::json& root, const std::string& key, double& value)
{
  auto it = root.find(key);
  if (it == root.end() || !it.value().is_number())
  {
    return false;
  }
  value = it.value();
  return true;
}

//------------------------------------------------------------------------------
bool GetIntArray(const nlohmann::json& root, const std::string& key, std::vector<int>& value)
{
  auto it = root.find(key);
  if (it == root.end() || !it.value().is_array())
  {
    return false;
  }
  value.reserve(it.value().size());
  for (const auto& intValue : it.value())
  {
    if (intValue.empty() && !intValue.is_number_integer())
    {
      value.clear();
      return false;
    }
    value.push_back(intValue);
  }
  if (value.empty())
  {
    value.clear();
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool GetUIntArray(
  const nlohmann::json& root, const std::string& key, std::vector<unsigned int>& value)
{
  auto it = root.find(key);
  if (it == root.end() || !it.value().is_array())
  {
    return false;
  }
  value.reserve(it.value().size());
  for (const auto& uIntValue : it.value())
  {
    if (uIntValue.empty() && !uIntValue.is_number_unsigned())
    {
      value.clear();
      return false;
    }
    value.push_back(uIntValue);
  }
  if (value.empty())
  {
    value.clear();
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool GetFloatArray(const nlohmann::json& root, const std::string& key, std::vector<float>& value)
{
  auto it = root.find(key);
  if (it == root.end() || !it.value().is_array())
  {
    return false;
  }
  value.reserve(it.value().size());
  for (const auto& floatValue : it.value())
  {
    if (floatValue.empty() && !floatValue.is_number())
    {
      value.clear();
      return false;
    }
    value.push_back(floatValue);
  }
  if (value.empty())
  {
    value.clear();
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool GetDoubleArray(const nlohmann::json& root, const std::string& key, std::vector<double>& value)
{
  auto it = root.find(key);
  if (it == root.end() || !it.value().is_array())
  {
    return false;
  }
  value.reserve(it.value().size());
  for (const auto& doubleValue : it.value())
  {
    if (doubleValue.empty() && !doubleValue.is_number())
    {
      value.clear();
      return false;
    }
    value.push_back(doubleValue);
  }
  if (value.empty())
  {
    value.clear();
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool GetStringValue(const nlohmann::json& root, const std::string& key, std::string& value)
{
  auto it = root.find(key);
  if (it == root.end() || !it.value().is_string())
  {
    return false;
  }
  value = it.value();
  return true;
}

//------------------------------------------------------------------------------
bool CheckVersion(const nlohmann::json& glTFAsset)
{
  auto assetMinVersionIt = glTFAsset.find("minVersion");
  auto assetVersionIt = glTFAsset.find("version");

  if (assetMinVersionIt != glTFAsset.end())
  {
    if (assetMinVersionIt.value() != MIN_GLTF_VERSION)
    {
      return false;
    }
  }
  else if (assetVersionIt != glTFAsset.end())
  {
    if (assetVersionIt.value() != MIN_GLTF_VERSION)
    {
      return false;
    }
  }
  else
  {
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
bool GetBinaryBufferFromUri(
  const std::string& uri, vtkURILoader* loader, std::vector<char>& buffer, size_t bufferSize)
{
  auto stream = loader->Load(uri);
  if (!stream)
  {
    return false;
  }

  buffer.resize(bufferSize);
  return stream->Read(buffer.data(), buffer.size()) == buffer.size();
}

//------------------------------------------------------------------------------
bool ExtractGLBFileInformation(vtkResourceStream* stream, uint32_t& version, uint32_t& fileLength,
  uint32_t glbStart, std::vector<vtkGLTFUtils::ChunkInfoType>& chunkInfo)
{
  // Read version
  if (stream->Read(&version, GLBWordSize) != GLBWordSize)
  {
    vtkErrorWithObjectMacro(nullptr, "Truncated glb file");
    return false;
  }

  // Read file length
  if (stream->Read(&fileLength, GLBWordSize) != GLBWordSize)
  {
    vtkErrorWithObjectMacro(nullptr, "Truncated glb file");
    return false;
  }

  // Read chunks until end of file
  while (stream->Tell() - glbStart != fileLength)
  {
    // Read chunk length
    std::uint32_t chunkDataSize;
    if (stream->Read(&chunkDataSize, GLBWordSize) != GLBWordSize)
    {
      vtkErrorWithObjectMacro(nullptr, "Truncated glb file");
      return false;
    }

    // Read chunk type
    std::string chunkType;
    chunkType.resize(GLBWordSize);
    // NOLINTNEXTLINE(readability-container-data-pointer)
    if (stream->Read(&chunkType[0], chunkType.size()) != chunkType.size())
    {
      vtkErrorWithObjectMacro(nullptr, "Truncated glb file");
      return false;
    }

    chunkInfo.emplace_back(chunkType, chunkDataSize);

    // Jump to next chunk
    stream->Seek(chunkDataSize, vtkResourceStream::SeekDirection::Current);
  }

  return true;
}

//------------------------------------------------------------------------------
bool ValidateGLBFile(const std::string& magic, uint32_t version, uint32_t fileLength,
  std::vector<vtkGLTFUtils::ChunkInfoType> chunkInfo)
{
  // Check header
  if (magic != "glTF" || version != vtkGLTFUtils::GLBVersion)
  {
    return false;
  }
  if (chunkInfo.empty())
  {
    return false;
  }
  // Compute sum of chunk sizes and check that first chunk is json
  size_t lengthSum = 0;
  for (size_t chunkNumber = 0; chunkNumber < chunkInfo.size(); chunkNumber++)
  {
    if (chunkNumber == 0)
    {
      if (chunkInfo[chunkNumber].first != "JSON")
      {
        return false;
      }
    }
    lengthSum += chunkInfo[chunkNumber].second;
  }
  // Compute total size
  lengthSum += vtkGLTFUtils::GLBHeaderSize + chunkInfo.size() * vtkGLTFUtils::GLBChunkHeaderSize;
  // Check for inconsistent chunk sizes
  return fileLength == lengthSum;
}
VTK_ABI_NAMESPACE_END
}
