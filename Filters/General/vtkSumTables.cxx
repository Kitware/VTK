// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSumTables.h"

#include "vtkAbstractArray.h"
#include "vtkAlgorithmOutput.h"
#include "vtkArray.h"
#include "vtkDataArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkSMPTools.h"
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

VTK_ABI_NAMESPACE_BEGIN

namespace
{

bool buildColumnCorrespondences(vtkTable* aa, vtkTable* bb,
  std::map<vtkDataArray*, vtkDataArray*>& columnPairs, bool allowAbstractColumns)
{
  columnPairs.clear();
  vtkIdType numCols = aa->GetNumberOfColumns();
  if (numCols != bb->GetNumberOfColumns())
  {
    return false;
  }
  vtkIdType numRows = aa->GetNumberOfRows();
  for (vtkIdType col = 0; col < numCols; ++col)
  {
    auto* colAA = vtkDataArray::SafeDownCast(aa->GetColumn(col));
    if (!colAA)
    {
      if (allowAbstractColumns)
      {
        continue;
      }
      else
      {
        return false;
      }
    }
    auto* colBB = vtkDataArray::SafeDownCast(bb->GetColumnByName(colAA->GetName()));
    if (!colBB)
    {
      if (allowAbstractColumns)
      {
        continue;
      }
      else
      {
        return false;
      }
    }
    else
    {
      if (colAA->GetNumberOfComponents() != colBB->GetNumberOfComponents() ||
        colBB->GetNumberOfTuples() != numRows)
      {
        return false;
      }
    }
    columnPairs[colAA] = colBB;
  }
  return true;
}

template <typename T>
struct TupleFetcher
{
  TupleFetcher(vtkDataArray* array)
    : Array(array)
  {
    this->Tuple.resize(array ? array->GetNumberOfComponents() : 0);
  }

  T* GetData() { return this->Tuple.data(); }

  void Fetch(vtkIdType idx);
  void Update(vtkIdType idx);

  template <typename Other>
  TupleFetcher<T>& operator+=(const Other& other)
  {
    std::size_t count = this->Tuple.size();
    assert(count == other.Tuple.size());
    for (std::size_t ii = 0; ii < count; ++ii)
    {
      this->Tuple[ii] += static_cast<T>(other.Tuple[ii]);
    }
    return *this;
  }

  vtkDataArray* Array{ nullptr };
  std::vector<T> Tuple;
};

template <>
void TupleFetcher<vtkTypeInt64>::Fetch(vtkIdType idx)
{
  this->Array->GetIntegerTuple(idx, this->Tuple.data());
}

template <>
void TupleFetcher<vtkTypeUInt64>::Fetch(vtkIdType idx)
{
  this->Array->GetUnsignedTuple(idx, this->Tuple.data());
}

template <>
void TupleFetcher<vtkTypeInt64>::Update(vtkIdType idx)
{
  this->Array->SetIntegerTuple(idx, this->Tuple.data());
}

template <>
void TupleFetcher<vtkTypeUInt64>::Update(vtkIdType idx)
{
  this->Array->SetUnsignedTuple(idx, this->Tuple.data());
}

struct SumArrayWorker
{
  void Prepare(vtkDataArray* aa, vtkDataArray* bb)
  {
    this->Result = aa;
    this->Summand = bb;
  }

  template <typename IntTypeA, typename IntTypeB>
  void SumIntegral(vtkIdType begin, vtkIdType end)
  {
    TupleFetcher<IntTypeA> tupleA(this->Result);
    TupleFetcher<IntTypeB> tupleB(this->Summand);
    for (vtkIdType ii = begin; ii < end; ++ii)
    {
      tupleA.Fetch(ii);
      tupleB.Fetch(ii);
      tupleA += tupleB;
      tupleA.Update(ii);
    }
  }

  void SumFloatingPoint(vtkIdType begin, vtkIdType end)
  {
    std::vector<double> tupleA;
    std::vector<double> tupleB;
    int numComp = this->Result->GetNumberOfComponents();
    tupleA.resize(numComp);
    tupleB.resize(numComp);
    for (vtkIdType ii = begin; ii < end; ++ii)
    {
      this->Result->GetTuple(ii, tupleA.data());
      this->Summand->GetTuple(ii, tupleB.data());
      for (int jj = 0; jj < numComp; ++jj)
      {
        tupleA[jj] += tupleB[jj];
      }
      this->Result->SetTuple(ii, tupleA.data());
    }
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    if (this->Result->IsIntegral() && this->Summand->IsIntegral())
    {
      if (IsSigned(this->Result->GetDataType()))
      {
        if (IsSigned(this->Summand->GetDataType()))
        {
          this->SumIntegral<vtkTypeInt64, vtkTypeInt64>(begin, end);
        }
        else
        {
          this->SumIntegral<vtkTypeInt64, vtkTypeUInt64>(begin, end);
        }
      }
      else
      {
        if (IsSigned(this->Summand->GetDataType()))
        {
          this->SumIntegral<vtkTypeUInt64, vtkTypeInt64>(begin, end);
        }
        else
        {
          this->SumIntegral<vtkTypeUInt64, vtkTypeUInt64>(begin, end);
        }
      }
    }
    else
    {
      this->SumFloatingPoint(begin, end);
    }
  }

  vtkDataArray* Result;
  vtkDataArray* Summand;
};

} // anonymous namespace

vtkStandardNewMacro(vtkSumTables);

vtkSumTables::vtkSumTables()
{
  this->SetNumberOfInputPorts(2);
}

void vtkSumTables::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkSumTables::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

void vtkSumTables::SetSourceData(vtkTable* source)
{
  this->SetInputData(1, source);
}

bool vtkSumTables::SumTables(vtkTable* aa, vtkTable* bb, bool checkOnly, bool allowAbstractColumns)
{
  if (!aa || !bb)
  {
    vtkGenericWarningMacro("Null input table(s).");
    return false;
  }
  std::map<vtkDataArray*, vtkDataArray*> columnPairs;
  vtkIdType numRows = aa->GetNumberOfRows();
  if (numRows != bb->GetNumberOfRows())
  {
    return false;
  }
  bool status = buildColumnCorrespondences(aa, bb, columnPairs, allowAbstractColumns);
  if (!status || checkOnly)
  {
    return status;
  }

  SumArrayWorker worker;
  for (const auto& entry : columnPairs)
  {
    worker.Prepare(entry.first, entry.second);
    vtkSMPTools::For(0, numRows, worker);
  }
  return true;
}

int vtkSumTables::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkTable* left = vtkTable::GetData(inputVector[0]);
  vtkTable* right = vtkTable::GetData(inputVector[1]);

  vtkTable* output = vtkTable::GetData(outputVector);

  if (!left || !right || !output)
  {
    vtkErrorMacro("No inputs or no output.");
    return 0;
  }

  output->DeepCopy(left);
  int status = vtkSumTables::SumTables(output, right) ? 1 : 0;
  return status;
}

int vtkSumTables::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

VTK_ABI_NAMESPACE_END
