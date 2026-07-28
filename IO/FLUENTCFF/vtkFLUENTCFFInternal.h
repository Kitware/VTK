// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @namespace vtkFLUENTCFFInternal
 * @brief internal utilities for vtkFLUENTCFFReader
 *
 * Utility methods used by vtkFLUENTCFFReader to retrieve the matching array name.
 */
#ifndef vtkFLUENTCFFUtilities_h
#define vtkFLUENTCFFUtilities_h

#include <string>
#include <unordered_map>

namespace vtkFLUENTCFFInternal
{
/**
 * Retrieve the correct field name. If there is not match, returns the same string passed as an
 * argument
 */
std::string GetMatchingFieldName(const std::string& strSectionName);

/**
 * Remove the trailing index of the string if it ends by "_1" or any other digit
 */
bool RemoveTrailingIndex(std::string& fieldName);

/**
 * Remove the suffix passed in argument if it is prensent in the field name
 */
bool RemoveSuffixIfPresent(std::string& fieldName, const std::string& suffix);

const std::unordered_map<std::string, std::string>& FieldsNamesMap();

}

#endif
