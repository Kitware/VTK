// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "vtkRowQuery.h"

#include "algorithm"
#include "vtkObjectFactory.h"
#include "vtkVariantArray.h"

#include <cctype>

VTK_ABI_NAMESPACE_BEGIN
vtkRowQuery::vtkRowQuery()
{
  this->CaseSensitiveFieldNames = false;
}

vtkRowQuery::~vtkRowQuery() = default;

void vtkRowQuery::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CaseSensitiveFieldNames: " << this->CaseSensitiveFieldNames << endl;
}

int vtkRowQuery::GetFieldIndex(const char* name)
{
  std::string lcSearchName(name);
  std::transform(
    lcSearchName.begin(), lcSearchName.end(), lcSearchName.begin(), (int (*)(int))tolower);

  int index;
  bool found = false;
  for (index = 0; index < this->GetNumberOfFields(); index++)
  {
    if (this->CaseSensitiveFieldNames)
    {
      if (!strcmp(name, this->GetFieldName(index)))
      {
        found = true;
        break;
      }
    }
    else
    {
      std::string fieldName(this->GetFieldName(index));
      std::transform(fieldName.begin(), fieldName.end(), fieldName.begin(), (int (*)(int))tolower);
      if (lcSearchName == fieldName)
      {
        found = true;
        break;
      }
    }
  }
  if (found)
  {
    return index;
  }
  return -1;
}

bool vtkRowQuery::NextRow(vtkVariantArray* rowArray)
{
  if (!this->NextRow())
  {
    return false;
  }
  rowArray->Reset();
  for (int col = 0; col < this->GetNumberOfFields(); col++)
  {
    rowArray->InsertNextValue(this->DataValue(col));
  }
  return true;
}
VTK_ABI_NAMESPACE_END
