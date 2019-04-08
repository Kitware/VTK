/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGLTFUtils.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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

#include "vtk_jsoncpp_fwd.h" // For Json forward declaration

#include <string> // For string
#include <vector> // For vector

namespace vtkGLTFUtils
{
using ChunkInfoType = std::pair<std::string, uint32_t>;
// Binary glTF constants
const uint32_t GLBWordSize = 4;
const uint32_t GLBHeaderSize = 12;
const uint32_t GLBChunkHeaderSize = 8;
const uint32_t GLBVersion = 2;

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
bool ExtractGLBFileInformation(const std::string& fileName, std::string& magic, uint32_t& version,
  uint32_t& fileLength, std::vector<vtkGLTFUtils::ChunkInfoType>& chunkInfo);

/**
 * Get int value from Json variable, with existence and type checks.
 */
bool GetIntValue(const Json::Value& root, int& value);

/**
 * Get int value from Json variable, with existence and type checks.
 */
bool GetUIntValue(const Json::Value& root, unsigned int& value);

/**
 * Get double value from Json variable, with existence and type checks.
 */
bool GetDoubleValue(const Json::Value& root, double& value);

/**
 * Get string value from Json variable, with existence and type checks.
 */
bool GetStringValue(const Json::Value& root, std::string& value);

/**
 * Get bool value from Json variable, with existence and type checks.
 */
bool GetBoolValue(const Json::Value& root, bool& value);

/**
 * Get int array from Json variable, with existence and type checks.
 */
bool GetIntArray(const Json::Value& root, std::vector<int>& value);

/**
 * Get int array from Json variable, with existence and type checks.
 */
bool GetUIntArray(const Json::Value& root, std::vector<unsigned int>& value);

/**
 * Get float array from Json variable, with existence and type checks.
 */
bool GetFloatArray(const Json::Value& root, std::vector<float>& value);

/**
 * Get double array from Json variable, with existence and type checks.
 */
bool GetDoubleArray(const Json::Value& root, std::vector<double>& value);

/**
 * Check document version. Currently supporting glTF 2.0 only.
 */
bool CheckVersion(const Json::Value& glTFAsset);

/**
 * Compute the path to a resource from its path as specified in the glTF file, and the glTF
 * file's path.
 */
std::string GetResourceFullPath(const std::string& resourcePath, const std::string& glTFFilePath);

/**
 * Load binary buffer from uri information. Uri can be a base 64 data-uri or file path.
 */
bool GetBinaryBufferFromUri(const std::string& uri, const std::string& glTFFileName,
  std::vector<char>& buffer, size_t bufferSize);

/**
 * Extract MIME-Type from data-uri
 */
std::string GetDataUriMimeType(const std::string& uri);
}

#endif

// VTK-HeaderTest-Exclude: vtkGLTFUtils.h
