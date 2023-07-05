// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef OMFHelpers_h
#define OMFHelpers_h

#include "vtkABINamespace.h"

#include "vtk_jsoncpp_fwd.h" // For Json fwd declaration

#include <string>
#include <vector>

namespace omf
{
namespace helper
{
VTK_ABI_NAMESPACE_BEGIN

/**
 * print names of members of root. Just used for debugging
 */
void PrintMemberNames(const Json::Value& root);

/**
 * read a 3d point stored directly in JSON
 */
bool GetPointFromJSON(const Json::Value& pointJSON, double point[3]);

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

VTK_ABI_NAMESPACE_END
} // end namespace helper
} // end namespace omf

#endif // OMFHelpers_h
