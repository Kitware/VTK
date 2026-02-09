// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkFLUENTCFFInternal.h"

//------------------------------------------------------------------------------
bool vtkFLUENTCFFInternal::RemoveTrailingIndex(std::string& fieldName)
{
  if (fieldName.size() > 2 && fieldName[fieldName.size() - 2] == '_' &&
    std::isdigit(fieldName[fieldName.size() - 1]))
  {
    fieldName.erase(fieldName.size() - 2);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkFLUENTCFFInternal::RemoveSuffixIfPresent(std::string& fieldName, const std::string& suffix)
{
  if (fieldName.size() > suffix.size() &&
    fieldName.substr(fieldName.size() - suffix.size(), suffix.size()) == suffix)
  {
    fieldName.erase(fieldName.size() - suffix.size());
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
std::string vtkFLUENTCFFInternal::GetMatchingFieldName(const std::string& strSectionName)
{
  const std::string prefix = strSectionName.substr(0, 3);
  if (prefix != "SV_")
  {
    return strSectionName;
  }
  std::string fieldName = strSectionName.substr(3);

  std::string vector_string;
  if (vtkFLUENTCFFInternal::RemoveSuffixIfPresent(fieldName, "_G"))
  {
    vector_string = " gradient vector";
  }
  else if (vtkFLUENTCFFInternal::RemoveSuffixIfPresent(fieldName, "_RG"))
  {
    vector_string = " RG vector";
  }

  vtkFLUENTCFFInternal::RemoveTrailingIndex(fieldName);
  if (!vtkFLUENTCFFInternal::RemoveSuffixIfPresent(fieldName, "_MEAN"))
  {
    vtkFLUENTCFFInternal::RemoveSuffixIfPresent(fieldName, "_RMS");
  }

  std::string previous_step_string;
  if (vtkFLUENTCFFInternal::RemoveSuffixIfPresent(fieldName, "_M1"))
  {
    previous_step_string = " at previous step";
  }
  else if (vtkFLUENTCFFInternal::RemoveSuffixIfPresent(fieldName, "_M2"))
  {
    previous_step_string = " at second_previous_step";
  }

  if (!vtkFLUENTCFFInternal::FieldsNamesMap.count(fieldName))
  {
    return strSectionName;
  }

  return vtkFLUENTCFFInternal::FieldsNamesMap.at(fieldName) + vector_string + previous_step_string +
    " (" + strSectionName + ")";
}
