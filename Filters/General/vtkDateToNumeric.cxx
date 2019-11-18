/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDateToNumeric.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDateToNumeric.h"

#include "vtkCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

// old versions of gcc are missing some pices of c++11 such as std::get_time
// so use

#if (defined(__GNUC__) && (__GNUC__ < 5)) || defined(ANDROID)
#define USE_STRPTIME
#include "time.h"
#endif

vtkStandardNewMacro(vtkDateToNumeric);
//----------------------------------------------------------------------------
vtkDateToNumeric::vtkDateToNumeric()
  : DateFormat(nullptr)
{
}

//----------------------------------------------------------------------------
vtkDateToNumeric::~vtkDateToNumeric() {}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
int vtkDateToNumeric::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto input = vtkDataObject::GetData(inputVector[0], 0);
  auto output = vtkDataObject::GetData(outputVector, 0);
  output->ShallowCopy(input);

  std::vector<std::string> formats;
  if (this->DateFormat)
  {
    formats.push_back(this->DateFormat);
  }
  // default formats
  formats.push_back("%Y-%m-%d %H:%M:%S");
  formats.push_back("%d/%m/%Y %H:%M:%S");

  // now filter arrays for each of the associations.
  for (int association = 0; association < vtkDataObject::NUMBER_OF_ASSOCIATIONS; ++association)
  {
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
        auto inval = inarray->GetValue(0);
        std::string useFormat;
        for (auto& format : formats)
        {
#ifdef USE_STRPTIME
          struct tm atime;
          auto result = strptime(inval.c_str(), format.c_str(), &atime);
          if (result)
          {
            useFormat = format;
            break;
          }
#else
          std::tm tm = {};
          std::stringstream ss(inval);
          ss >> std::get_time(&tm, format.c_str());
          if (!ss.fail())
          {
            useFormat = format;
            break;
          }
#endif
        }
        if (useFormat.size())
        {
          vtkNew<vtkDoubleArray> newArray;
          std::string newName = inarray->GetName();
          newName += "_numeric";
          newArray->SetName(newName.c_str());
          newArray->Allocate(inarray->GetNumberOfValues());
          for (vtkIdType i = 0; i < inarray->GetNumberOfValues(); ++i)
          {
            inval = inarray->GetValue(i);
#ifdef USE_STRPTIME
            struct tm atime;
            auto result = strptime(inval.c_str(), useFormat.c_str(), &atime);
            if (result)
            {
              auto etime = mktime(&atime);
              newArray->InsertNextValue(etime);
            }
#else
            std::tm tm = {};
            std::stringstream ss(inval);
            ss >> std::get_time(&tm, useFormat.c_str());
            if (!ss.fail())
            {
              auto etime = std::mktime(&tm);
              newArray->InsertNextValue(etime);
            }
#endif
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

//----------------------------------------------------------------------------
void vtkDateToNumeric::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DateFormat: " << (this->DateFormat ? this->DateFormat : "(none)");
}
