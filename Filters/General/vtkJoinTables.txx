// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
template <typename KeyColType, typename KeyValues>
KeyValues GetCell(KeyColType*, int);

//------------------------------------------------------------------------------
template <typename KeyColType, typename KeyValues>
void InsertNext(KeyColType*, KeyValues);

//------------------------------------------------------------------------------
template <>
inline void InsertNext<vtkDataArray, double>(vtkDataArray* col, double val)
{
  col->InsertNextTuple(&val);
}

//------------------------------------------------------------------------------
template <>
inline void InsertNext<vtkStringArray, std::string>(vtkStringArray* col, std::string val)
{
  col->InsertNextValue(val);
}

//------------------------------------------------------------------------------
template <>
inline double GetCell<vtkDataArray, double>(vtkDataArray* col, int index)
{
  return col->GetTuple(index)[0];
}

//------------------------------------------------------------------------------
template <>
inline std::string GetCell<vtkStringArray, std::string>(vtkStringArray* col, int index)
{
  return (col->GetValue(index));
}

//------------------------------------------------------------------------------
template <typename ColType, typename KeyColType, typename KeyValues>
void vtkJoinTables::MergeColumn(ColType* outputColumn, ColType* Column, KeyColType* outputKeyCol,
  const char* name, std::map<KeyValues, int> map)
{

  outputColumn->SetName(name);
  outputColumn->SetNumberOfComponents(Column->GetNumberOfComponents());
  // Loop over numeric tuples
  for (int i = 0; i < outputKeyCol->GetNumberOfValues(); i++)
  {
    KeyValues id = GetCell<KeyColType, KeyValues>(outputKeyCol, i);
    if (auto numericColumn = vtkDataArray::SafeDownCast(Column))
    {
      auto outputNumericColumn = vtkDataArray::SafeDownCast(outputColumn);
      if (map.count(id))
      {
        double* val = numericColumn->GetTuple(map.find(id)->second);
        outputNumericColumn->InsertNextTuple(val);
      }
      else
      {
        double val = this->ReplacementValue;
        outputNumericColumn->InsertNextTuple(&val);
      }
    }
    else if (auto stringColumn = vtkStringArray::SafeDownCast(Column))
    {
      auto outputStringColumn = vtkStringArray::SafeDownCast(outputColumn);
      if (map.count(id))
      {
        auto val = stringColumn->GetValue(map.find(id)->second);
        outputStringColumn->InsertNextValue(val);
      }
      else
      {
        std::string val;
        outputStringColumn->InsertNextValue(val);
      }
    }
    else
    {
      vtkWarningMacro("Key column type is unsupported.");
    }
  }
}

//------------------------------------------------------------------------------
template <typename KeyColType, typename KeyValues>
void vtkJoinTables::JoinAlgorithm(vtkTable* left, vtkTable* right, vtkTable* output,
  KeyColType* leftKeyCol, KeyColType* rightKeyCol, Maps<KeyValues>* maps)
{
  /*
   * 1- First column of the output table will be the key. Start by filling this column with adequate
   *    values according to the join method.
   */
  vtkIdType leftSize = leftKeyCol->GetNumberOfTuples();
  vtkIdType rightSize = rightKeyCol->GetNumberOfTuples();
  vtkSmartPointer<KeyColType> outputKeyCol = leftKeyCol->NewInstance();
  outputKeyCol->SetName(this->LeftKey.c_str());

  switch (this->Mode)
  {
    case INTERSECTION:
    {
      for (int i = 0; i < leftSize; i++)
      {
        for (int j = 0; j < rightSize; j++)
        {
          if (GetCell<KeyColType, KeyValues>(leftKeyCol, i) ==
            GetCell<KeyColType, KeyValues>(rightKeyCol, j))
          {
            auto val = GetCell<KeyColType, KeyValues>(leftKeyCol, i);
            InsertNext<KeyColType, KeyValues>(outputKeyCol, val);
            maps->left.insert(std::make_pair(val, i));
            maps->right.insert(std::make_pair(val, j));
            break;
          }
        }
      }
      break;
    }

    case UNION:
    {
      for (int i = 0; i < rightSize; i++)
      {
        auto val = GetCell<KeyColType, KeyValues>(rightKeyCol, i);
        maps->right.insert(std::make_pair(val, i));
        bool existsInLeftKeyCol = false;
        for (int j = 0; j < leftSize; j++)
        {
          if (i == 0)
          {
            auto val0 = GetCell<KeyColType, KeyValues>(leftKeyCol, j);
            InsertNext<KeyColType, KeyValues>(outputKeyCol, val0);
            maps->left.insert(std::make_pair(val0, j));
          }

          if (!existsInLeftKeyCol &&
            GetCell<KeyColType, KeyValues>(leftKeyCol, j) ==
              GetCell<KeyColType, KeyValues>(rightKeyCol, i))
          {
            // Flag the value from the right table when it's a duplicate from the
            // left table
            existsInLeftKeyCol = true;
          }
        }
        if (!existsInLeftKeyCol)
        {
          // Insert only the values of right that do not exist in left
          InsertNext<KeyColType, KeyValues>(outputKeyCol, val);
        }
      }
      break;
    }

    case LEFT:
    {
      for (int i = 0; i < leftSize; i++)
      {
        auto val = GetCell<KeyColType, KeyValues>(leftKeyCol, i);
        InsertNext<KeyColType, KeyValues>(outputKeyCol, val);
        maps->left.insert(std::make_pair(val, i));
      }
      for (int j = 0; j < rightSize; j++)
      {
        auto val = GetCell<KeyColType, KeyValues>(rightKeyCol, j);
        maps->right.insert(std::make_pair(val, j));
      }
      break;
    }

    case RIGHT:
    {
      for (int i = 0; i < leftSize; i++)
      {
        auto val = GetCell<KeyColType, KeyValues>(leftKeyCol, i);
        maps->left.insert(std::make_pair(val, i));
      }
      for (int j = 0; j < rightSize; j++)
      {
        auto val = GetCell<KeyColType, KeyValues>(rightKeyCol, j);
        InsertNext<KeyColType, KeyValues>(outputKeyCol, val);
        maps->right.insert(std::make_pair(val, j));
      }
      break;
    }
    default:
      vtkErrorMacro("Unknown Mode");
  }
  output->AddColumn(outputKeyCol);

  /*
   * 2- Now fill the rest of the columns. Add the columns from the left table, and then the
   *    columns from the right table.
   */

  // left table and LeftKey
  for (int lc = 0; lc < left->GetNumberOfColumns(); lc++)
  {
    if (std::string(left->GetColumnName(lc)) == this->LeftKey)
    {
      // do not copy the original key column
      continue;
    }
    vtkAbstractArray* currentColumn = left->GetColumn(lc);
    if (auto numericColumn = vtkDataArray::SafeDownCast(currentColumn))
    {
      vtkDataArray* outputColumn = numericColumn->NewInstance();
      this->MergeColumn<vtkDataArray, KeyColType, KeyValues>(
        outputColumn, numericColumn, outputKeyCol, left->GetColumnName(lc), maps->left);
      output->AddColumn(outputColumn);
      outputColumn->Delete();
    }
    else if (auto stringColumn = vtkStringArray::SafeDownCast(currentColumn))
    {
      vtkStringArray* outputColumn = stringColumn->NewInstance();
      this->MergeColumn<vtkStringArray, KeyColType, KeyValues>(
        outputColumn, stringColumn, outputKeyCol, left->GetColumnName(lc), maps->left);
      output->AddColumn(outputColumn);
      outputColumn->Delete();
    }
  }

  // right table and RightKey
  for (int lc = 0; lc < right->GetNumberOfColumns(); lc++)
  {
    if (std::string(right->GetColumnName(lc)) == this->RightKey)
    {
      // do not copy the original key column
      continue;
    }
    auto currentColumn = right->GetColumn(lc);
    if (output->GetColumnByName(right->GetColumnName(lc)))
    {
      // We have a duplicate column name. Add suffixes.
      output->GetColumnByName(right->GetColumnName(lc))
        ->SetName((std::string(right->GetColumnName(lc)) + "_0").c_str());
      currentColumn->SetName((std::string(right->GetColumnName(lc)) + "_1").c_str());
    }
    else
    {
      currentColumn->SetName(right->GetColumnName(lc));
    }

    if (auto numericColumn = vtkDataArray::SafeDownCast(currentColumn))
    {
      vtkDataArray* outputColumn = numericColumn->NewInstance();
      this->MergeColumn<vtkDataArray, KeyColType, KeyValues>(
        outputColumn, numericColumn, outputKeyCol, right->GetColumnName(lc), maps->right);
      output->AddColumn(outputColumn);
      outputColumn->Delete();
    }
    else if (auto stringColumn = vtkStringArray::SafeDownCast(currentColumn))
    {
      vtkStringArray* outputColumn = stringColumn->NewInstance();
      this->MergeColumn<vtkStringArray, KeyColType, KeyValues>(
        outputColumn, stringColumn, outputKeyCol, right->GetColumnName(lc), maps->right);
      output->AddColumn(outputColumn);
      outputColumn->Delete();
    }
  }
  outputKeyCol->Delete();
}
VTK_ABI_NAMESPACE_END
