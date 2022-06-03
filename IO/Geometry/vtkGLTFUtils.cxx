/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGLTFUtils.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGLTFUtils.h"

#include "vtkBase64Utilities.h"
#include "vtksys/FStream.hxx"
#include "vtksys/RegularExpression.hxx"
#include "vtksys/SystemTools.hxx"

#include <fstream>
#include <iostream>

#define MIN_GLTF_VERSION "2.0"
//------------------------------------------------------------------------------
bool vtkGLTFUtils::GetBoolValue(const nlohmann::json& root, const std::string& key, bool& value)
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
bool vtkGLTFUtils::GetIntValue(const nlohmann::json& root, const std::string& key, int& value)
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
bool vtkGLTFUtils::GetUIntValue(
  const nlohmann::json& root, const std::string& key, unsigned int& value)
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
bool vtkGLTFUtils::GetDoubleValue(const nlohmann::json& root, const std::string& key, double& value)
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
bool vtkGLTFUtils::GetIntArray(
  const nlohmann::json& root, const std::string& key, std::vector<int>& value)
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
bool vtkGLTFUtils::GetUIntArray(
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
bool vtkGLTFUtils::GetFloatArray(
  const nlohmann::json& root, const std::string& key, std::vector<float>& value)
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
bool vtkGLTFUtils::GetDoubleArray(
  const nlohmann::json& root, const std::string& key, std::vector<double>& value)
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
bool vtkGLTFUtils::GetStringValue(
  const nlohmann::json& root, const std::string& key, std::string& value)
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
bool vtkGLTFUtils::CheckVersion(const nlohmann::json& glTFAsset)
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
std::string vtkGLTFUtils::GetResourceFullPath(
  const std::string& resourcePath, const std::string& glTFFilePath)
{
  // Check for relative path
  if (!vtksys::SystemTools::FileIsFullPath(resourcePath.c_str()))
  {
    // Append relative path to base dir
    std::string baseDirPath = vtksys::SystemTools::GetParentDirectory(glTFFilePath);
    return vtksys::SystemTools::CollapseFullPath(resourcePath, baseDirPath);
  }
  return resourcePath;
}

//------------------------------------------------------------------------------
std::string vtkGLTFUtils::GetDataUriMimeType(const std::string& uri)
{
  vtksys::RegularExpression regex("^data:.*;");
  if (regex.find(uri))
  {
    // Remove preceding 'data:' and trailing semicolon
    size_t start = regex.start(0) + 5;
    size_t end = regex.end(0) - 1;
    return uri.substr(start, end - start);
  }
  return std::string();
}

//------------------------------------------------------------------------------
bool vtkGLTFUtils::GetBinaryBufferFromUri(const std::string& uri, const std::string& glTFFilePath,
  std::vector<char>& buffer, size_t bufferSize)
{
  // Check for data-uri
  if (vtksys::SystemTools::StringStartsWith(uri, "data:"))
  {
    // Extract base64 buffer
    std::vector<std::string> tokens;
    vtksys::SystemTools::Split(uri, tokens, ',');
    std::string base64Buffer = *(tokens.end() - 1); // Last token contains the base64 data
    buffer.resize(bufferSize);
    vtkBase64Utilities::DecodeSafely(reinterpret_cast<const unsigned char*>(base64Buffer.c_str()),
      base64Buffer.size(), reinterpret_cast<unsigned char*>(buffer.data()), bufferSize);
  }
  // Load buffer from file
  else
  {
    vtksys::ifstream fin;

    std::string bufferPath = GetResourceFullPath(uri, glTFFilePath);

    // Open file
    fin.open(bufferPath.c_str(), ios::binary);
    if (!fin.is_open())
    {
      return false;
    }
    // Check file length
    unsigned int len = vtksys::SystemTools::FileLength(bufferPath);
    if (len != bufferSize)
    {
      fin.close();
      return false;
    }
    // Load data
    buffer.resize(bufferSize);
    fin.read(buffer.data(), bufferSize);
    fin.close();
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkGLTFUtils::ExtractGLBFileInformation(const std::string& fileName, std::string& magic,
  uint32_t& version, uint32_t& fileLength, std::vector<vtkGLTFUtils::ChunkInfoType>& chunkInfo)
{
  vtksys::ifstream fin;
  fin.open(fileName.c_str(), std::ios::binary | std::ios::in);
  if (!fin.is_open())
  {
    return false;
  }

  // Load glb header
  // Read first word ("magic")
  char magicBuffer[vtkGLTFUtils::GLBWordSize];
  fin.read(magicBuffer, vtkGLTFUtils::GLBWordSize);
  magic = std::string(magicBuffer, magicBuffer + vtkGLTFUtils::GLBWordSize);
  // Read version
  fin.read(reinterpret_cast<char*>(&version), vtkGLTFUtils::GLBWordSize);
  // Read file length
  fin.read(reinterpret_cast<char*>(&fileLength), vtkGLTFUtils::GLBWordSize);
  // Check equality between extracted and actual file lengths
  fin.seekg(0, std::ios::end);
  if (fin.tellg() != std::streampos(fileLength))
  {
    return false;
  }
  fin.seekg(vtkGLTFUtils::GLBHeaderSize);
  // Read chunks until end of file
  while (fin.tellg() < fileLength)
  {
    // Read chunk length
    uint32_t chunkDataSize;
    fin.read(reinterpret_cast<char*>(&chunkDataSize), vtkGLTFUtils::GLBWordSize);

    // Read chunk type
    char chunkTypeBuffer[vtkGLTFUtils::GLBWordSize];
    fin.read(chunkTypeBuffer, vtkGLTFUtils::GLBWordSize);
    std::string chunkType(chunkTypeBuffer, chunkTypeBuffer + vtkGLTFUtils::GLBWordSize);
    chunkInfo.emplace_back(chunkType, chunkDataSize);
    // Jump to next chunk
    fin.seekg(chunkDataSize, std::ios::cur);
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkGLTFUtils::ValidateGLBFile(const std::string& magic, uint32_t version, uint32_t fileLength,
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
