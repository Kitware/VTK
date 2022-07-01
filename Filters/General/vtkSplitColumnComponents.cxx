
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSplitColumnComponents.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSplitColumnComponents.h"

#include "vtkAbstractArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSetAttributes.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include <cmath>
#include <sstream>

vtkStandardNewMacro(vtkSplitColumnComponents);
vtkInformationKeyMacro(vtkSplitColumnComponents, ORIGINAL_ARRAY_NAME, String);
vtkInformationKeyMacro(vtkSplitColumnComponents, ORIGINAL_COMPONENT_NUMBER, Integer);
//------------------------------------------------------------------------------
vtkSplitColumnComponents::vtkSplitColumnComponents()
  : CalculateMagnitudes(true)
  , NamingMode(vtkSplitColumnComponents::NUMBERS_WITH_PARENS)
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
vtkSplitColumnComponents::~vtkSplitColumnComponents() = default;

//------------------------------------------------------------------------------
int vtkSplitColumnComponents::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get input tables
  vtkInformation* table1Info = inputVector[0]->GetInformationObject(0);
  vtkTable* table = vtkTable::SafeDownCast(table1Info->Get(vtkDataObject::DATA_OBJECT()));

  // Get output table
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkTable* output = vtkTable::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataSetAttributes* inputRD = table->GetRowData();
  vtkDataSetAttributes* outputRD = output->GetRowData();

  vtkDataArray* inputGlobalIds = inputRD->GetGlobalIds();

  // Add columns from table, split multiple component columns as necessary
  for (int i = 0; i < table->GetNumberOfColumns(); ++i)
  {
    vtkAbstractArray* col = table->GetColumn(i);
    if (col->GetName() == nullptr)
    {
      vtkWarningMacro("Skipping column with no name!");
      continue;
    }

    int components = col->GetNumberOfComponents();
    if (components == 1)
    {
      output->AddColumn(col);
      vtkDataArray* newCol =
        vtkArrayDownCast<vtkDataArray>(output->GetColumn(output->GetNumberOfColumns() - 1));

      if (col == inputGlobalIds)
      {
        outputRD->SetGlobalIds(newCol);
      }
    }
    else if (components > 1)
    {
      // Split the multicomponent column up into individual columns
      int colSize = col->GetNumberOfTuples();
      for (int j = 0; j < components; ++j)
      {
        const std::string component_label = this->GetComponentLabel(col, j);
        vtkAbstractArray* newCol = vtkAbstractArray::CreateArray(col->GetDataType());
        newCol->SetName(component_label.c_str());
        newCol->SetNumberOfTuples(colSize);
        // pass component name overrides, if provided.
        if (col->HasAComponentName())
        {
          newCol->SetComponentName(0, col->GetComponentName(j));
        }
        // Now copy the components into their new columns
        if (col->IsA("vtkDataArray"))
        {
          // Handle numeric array types
          vtkDataArray* srcDataArray = vtkDataArray::SafeDownCast(col);
          vtkDataArray* dstDataArray = vtkDataArray::SafeDownCast(newCol);
          dstDataArray->CopyComponent(0, srcDataArray, j);
        }
        else if (col->GetDataType() == VTK_STRING)
        {
          vtkStringArray* srcArray = vtkStringArray::SafeDownCast(col);
          vtkStringArray* dstArray = vtkStringArray::SafeDownCast(newCol);
          int numSrcComponents = srcArray->GetNumberOfComponents();
          for (vtkIdType id = 0; id < srcArray->GetNumberOfTuples(); ++id)
          {
            dstArray->SetValue(id, srcArray->GetValue(id * numSrcComponents + j));
          }
        }
        else if (col->GetDataType() == VTK_VARIANT)
        {
          vtkVariantArray* srcArray = vtkVariantArray::SafeDownCast(col);
          vtkVariantArray* dstArray = vtkVariantArray::SafeDownCast(newCol);
          int numSrcComponents = srcArray->GetNumberOfComponents();
          for (vtkIdType id = 0; id < srcArray->GetNumberOfTuples(); ++id)
          {
            dstArray->SetValue(id, srcArray->GetValue(id * numSrcComponents + j));
          }
        }
        else
        {
          vtkErrorMacro("Unsupported array type " << col->GetClassName());
        }
        if (auto info = newCol->GetInformation())
        {
          info->Set(ORIGINAL_ARRAY_NAME(), col->GetName());
          info->Set(ORIGINAL_COMPONENT_NUMBER(), j);
        }
        output->AddColumn(newCol);
        newCol->Delete();
      }
      // Add a magnitude column and calculate values if requested
      if (this->CalculateMagnitudes && col->IsA("vtkDataArray"))
      {
        std::string component_label = this->GetComponentLabel(col, -1 /* for magnitude */);
        vtkAbstractArray* newCol = vtkAbstractArray::CreateArray(col->GetDataType());
        newCol->SetName(component_label.c_str());
        newCol->SetNumberOfTuples(colSize);
        // Now calculate the magnitude column
        vtkDataArray* srcDataArray = vtkDataArray::SafeDownCast(col);
        vtkDataArray* dstDataArray = vtkDataArray::SafeDownCast(newCol);

        const auto srcRange = vtk::DataArrayTupleRange(srcDataArray);
        auto dstRange = vtk::DataArrayValueRange<1>(dstDataArray);
        auto dstIter = dstRange.begin();

        for (const auto tuple : srcRange)
        {
          double mag = 0.0;
          for (const auto component : tuple)
          {
            auto x = static_cast<double>(component);
            mag += x * x;
          }
          (*dstIter) = std::sqrt(mag);

          ++dstIter;
        }

        if (auto info = newCol->GetInformation())
        {
          info->Set(ORIGINAL_ARRAY_NAME(), col->GetName());
          info->Set(ORIGINAL_COMPONENT_NUMBER(), -1); // for magnitude
        }
        output->AddColumn(newCol);
        newCol->Delete();
      }
    }
  }
  return 1;
}

namespace
{
//------------------------------------------------------------------------------
std::string vtkDefaultComponentName(int componentNumber, int componentCount)
{
  if (componentCount <= 1)
  {
    return "";
  }
  else if (componentNumber == -1)
  {
    return "Magnitude";
  }
  else if (componentCount <= 3 && componentNumber < 3)
  {
    const char* titles[] = { "X", "Y", "Z" };
    return titles[componentNumber];
  }
  else if (componentCount == 6)
  {
    const char* titles[] = { "XX", "YY", "ZZ", "XY", "YZ", "XZ" };
    // Assume this is a symmetric matrix.
    return titles[componentNumber];
  }
  else
  {
    std::ostringstream buffer;
    buffer << componentNumber;
    return buffer.str();
  }
}
std::string vtkGetComponentName(vtkAbstractArray* array, int component_no)
{
  const char* name = array->GetComponentName(component_no);
  if (name)
  {
    return name;
  }
  return vtkDefaultComponentName(component_no, array->GetNumberOfComponents());
}
};

//------------------------------------------------------------------------------
std::string vtkSplitColumnComponents::GetComponentLabel(vtkAbstractArray* array, int component_no)
{
  std::ostringstream stream;
  switch (this->NamingMode)
  {
    case NUMBERS_WITH_PARENS:
      stream << array->GetName() << " (";
      if (component_no == -1)
      {
        stream << "Magnitude)";
      }
      else
      {
        stream << component_no << ")";
      }
      break;

    case NUMBERS_WITH_UNDERSCORES:
      stream << array->GetName() << "_";
      if (component_no == -1)
      {
        stream << "Magnitude";
      }
      else
      {
        stream << component_no;
      }
      break;

    case NAMES_WITH_PARENS:
      stream << array->GetName() << " (" << vtkGetComponentName(array, component_no) << ")";
      break;

    case NAMES_WITH_UNDERSCORES:
    default:
      stream << array->GetName() << "_" << vtkGetComponentName(array, component_no);
      break;
  }
  return stream.str();
}

//------------------------------------------------------------------------------
void vtkSplitColumnComponents::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CalculateMagnitudes: " << this->CalculateMagnitudes << endl;
  os << indent << "NamingMode: ";
  switch (this->NamingMode)
  {
    case NAMES_WITH_UNDERSCORES:
      os << "NAMES_WITH_UNDERSCORES" << endl;
      break;
    case NAMES_WITH_PARENS:
      os << "NAMES_WITH_PARENS" << endl;
      break;
    case NUMBERS_WITH_UNDERSCORES:
      os << "NUMBERS_WITH_UNDERSCORES" << endl;
      break;
    case NUMBERS_WITH_PARENS:
      os << "NUMBERS_WITH_PARENS" << endl;
      break;
    default:
      os << "INVALID" << endl;
  }
}
