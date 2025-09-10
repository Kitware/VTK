// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDateToNumeric.h"

#include "vtkCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkStringScanner.h"

#include <ctime>
#include <iostream>
#include <sstream>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkDateToNumeric);
//------------------------------------------------------------------------------
vtkDateToNumeric::vtkDateToNumeric()
  : DateFormat(nullptr)
{
}

//------------------------------------------------------------------------------
vtkDateToNumeric::~vtkDateToNumeric() = default;

namespace
{
//------------------------------------------------------------------------------
bool is_not_strftime_format(const std::string& format)
{
  return format.size() >= 5 && format[0] == '{' && format[1] == ':' && format.back() == '}';
}
}

//------------------------------------------------------------------------------
void vtkDateToNumeric::SetDateFormat(const char* formatArg)
{
  std::string format = formatArg ? formatArg : "";
  if (!::is_not_strftime_format(format))
  {
    format = "{:" + format + '}';
  }
  const char* formatStr = format.c_str();
  vtkSetStringBodyMacro(DateFormat, formatStr);
}

//------------------------------------------------------------------------------
int vtkDateToNumeric::FillInputPortInformation(int, vtkInformation* info)
{
  // Skip composite data sets so that executives will treat this as a simple filter
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGenericDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

//------------------------------------------------------------------------------
int vtkDateToNumeric::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto input = vtkDataObject::GetData(inputVector[0], 0);
  auto output = vtkDataObject::GetData(outputVector, 0);
  output->ShallowCopy(input);

  std::vector<std::string> formats;
  if (this->DateFormat)
  {
    formats.emplace_back(this->DateFormat);
  }
  // default formats
  formats.emplace_back("{:%Y-%m-%d %H:%M:%S}");
  formats.emplace_back("{:%d/%m/%Y %H:%M:%S}");

  // now filter arrays for each of the associations.
  for (int association = 0; association < vtkDataObject::NUMBER_OF_ASSOCIATIONS; ++association)
  {
    if (this->CheckAbort())
    {
      break;
    }
    if (association == vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS)
    {
      continue;
    }

    auto inFD = input->GetAttributesAsFieldData(association);
    auto outFD = output->GetAttributesAsFieldData(association);
    if (!inFD || !outFD)
    {
      continue;
    }

    auto inDSA = vtkDataSetAttributes::SafeDownCast(inFD);
    auto outDSA = vtkDataSetAttributes::SafeDownCast(outFD);

    for (int idx = 0, max = inFD->GetNumberOfArrays(); idx < max; ++idx)
    {
      vtkStringArray* inarray = vtkStringArray::SafeDownCast(inFD->GetAbstractArray(idx));
      if (inarray && inarray->GetName())
      {
        // look at the first value to see if it is a date we can parse
        std::string useFormat;
        for (auto& format : formats)
        {
          const std::string& inval = inarray->GetValue(0);
          auto result = vtk::scan<std::tm>(inval, format);
          if (result)
          {
            useFormat = format;
            break;
          }
        }
        if (!useFormat.empty())
        {
          vtkNew<vtkDoubleArray> newArray;
          std::string newName = inarray->GetName();
          newName += "_numeric";
          newArray->SetName(newName.c_str());
          newArray->Allocate(inarray->GetNumberOfValues());
          for (vtkIdType i = 0; i < inarray->GetNumberOfValues(); ++i)
          {
            const std::string& inval = inarray->GetValue(i);
            auto result = vtk::scan<std::tm>(inval, useFormat);
            if (result)
            {
              auto etime = std::mktime(&result->value());
              newArray->InsertNextValue(etime);
            }
            else
            {
              newArray->InsertNextValue(0.0);
            }
          }
          outFD->AddArray(newArray);

          // preserve attribute type flags.
          for (int attr = 0; inDSA && outDSA && (attr < vtkDataSetAttributes::NUM_ATTRIBUTES);
               ++attr)
          {
            if (inDSA->GetAbstractAttribute(attr) == inarray)
            {
              outDSA->SetAttribute(newArray, attr);
            }
          }
        }
      }
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkDateToNumeric::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DateFormat: " << (this->DateFormat ? this->DateFormat : "(none)");
}
VTK_ABI_NAMESPACE_END
