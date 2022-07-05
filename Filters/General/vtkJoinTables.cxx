/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkJoinTables.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkJoinTables.h"

#include "vtkAbstractArray.h"
#include "vtkAlgorithmOutput.h"
#include "vtkArray.h"
#include "vtkDataArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkType.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

#include <iostream>
#include <map>
#include <string>
#include <utility>

vtkStandardNewMacro(vtkJoinTables);

//------------------------------------------------------------------------------
vtkJoinTables::vtkJoinTables()
{
  this->SetNumberOfInputPorts(2);
}

//------------------------------------------------------------------------------
void vtkJoinTables::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//------------------------------------------------------------------------------
void vtkJoinTables::SetSourceData(vtkTable* source)
{
  this->SetInputData(1, source);
}

//------------------------------------------------------------------------------
bool HasDuplicates(vtkAbstractArray* array)
{
  vtkIdType size = array->GetNumberOfValues();
  for (int i = 1; i < size; i++)
  {
    for (int j = 0; j < i; j++)
    {
      if (array->GetVariantValue(i) == array->GetVariantValue(j))
      {
        return true;
      }
    }
  }
  return false;
}

//------------------------------------------------------------------------------
int vtkJoinTables::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkTable* left = vtkTable::GetData(inputVector[0]);
  vtkTable* right = vtkTable::GetData(inputVector[1]);

  vtkTable* output = vtkTable::GetData(outputVector);

  if (!left || !right || !output)
  {
    vtkErrorMacro("No inputs or output.");
    return 0;
  }

  // If one of the inputs is an empty table, also return an empty table.
  if (left->GetNumberOfColumns() == 0 || right->GetNumberOfColumns() == 0)
  {
    return 1;
  }

  // Assert that LeftKey and RightKey refer to valid existing columns
  vtkAbstractArray* leftKeyCol = left->GetColumnByName(this->LeftKey.c_str());
  if (!leftKeyCol)
  {
    vtkErrorMacro("Left key is invalid");
    return 0;
  }
  vtkAbstractArray* rightKeyCol = right->GetColumnByName(this->RightKey.c_str());
  if (!rightKeyCol)
  {
    vtkErrorMacro("Right key is invalid");
    return 0;
  }

  if (!(leftKeyCol->GetDataType() == rightKeyCol->GetDataType()))
  {
    vtkErrorMacro("Key columns data types do not match : " << leftKeyCol->GetDataType() << " and "
                                                           << rightKeyCol->GetDataType() << ".");
    return 0;
  }

  // Assert that each key column contains unique elements
  if (HasDuplicates(leftKeyCol) || HasDuplicates(rightKeyCol))
  {
    vtkErrorMacro("The key columns must not contain duplicate values.");
    return 0;
  }

  // Core Algorithm
  if (auto leftKeyDA = vtkDataArray::SafeDownCast(leftKeyCol))
  {
    auto rightKeyDA = vtkDataArray::SafeDownCast(rightKeyCol);
    auto maps = Maps<double>();
    this->JoinAlgorithm<vtkDataArray, double>(left, right, output, leftKeyDA, rightKeyDA, &maps);
  }
  else if (auto leftKeySA = vtkStringArray::SafeDownCast(leftKeyCol))
  {
    auto rightKeySA = vtkStringArray::SafeDownCast(rightKeyCol);
    auto maps = Maps<std::string>();
    this->JoinAlgorithm<vtkStringArray, std::string>(
      left, right, output, leftKeySA, rightKeySA, &maps);
  }
  else
  {
    vtkErrorMacro("Unsupported types for the key columns.");
    return 0;
  }
  return 1;
}

//------------------------------------------------------------------------------
int vtkJoinTables::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

//------------------------------------------------------------------------------
void vtkJoinTables::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Left Key Column: " << this->LeftKey << std::endl;
  os << indent << "Right Key Column: " << this->RightKey << std::endl;
  os << indent << "Replacement Value: " << this->ReplacementValue << std::endl;
  os << indent << "Mode: ";
  switch (this->Mode)
  {
    case INTERSECTION:
      os << "Intersection";
      break;
    case UNION:
      os << "Union";
      break;
    case LEFT:
      os << "Left Join";
      break;
    case RIGHT:
      os << "Right Join";
      break;
    default:
      os << "Undefined";
      break;
  }
  os << endl;
}
