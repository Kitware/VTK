// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDGInvokeOperator.h"

#include <cassert>

VTK_ABI_NAMESPACE_BEGIN

namespace
{

template <typename IdType>
struct SimpleInputPoints
{
  struct Iter
  {
    std::size_t Location;
    SimpleInputPoints* Self;

    Iter operator++()
    {
      // Do not increment endlessly.
      if (this->Location == this->Self->Size)
      {
        return *this;
      }

      ++this->Location;
      return *this;
    }
    bool operator!=(const Iter& other)
    {
      assert(this->Self == other.Self);
      return this->Location != other.Location;
    }
    IdType GetCellId() const { return this->Self->CellIds[this->Location]; }
    std::array<double, 3> GetParameter() const
    {
      std::array<double, 3> result{ this->Self->Parameters[3 * this->Location],
        this->Self->Parameters[3 * this->Location + 1],
        this->Self->Parameters[3 * this->Location + 2] };
      return result;
    }
  };

  SimpleInputPoints(std::size_t num, const IdType* cellIds, const double* rst)
    : Size(num)
    , CellIds(cellIds)
    , Parameters(rst)
  {
  }

  Iter Begin() { return Iter{ 0, this }; }
  Iter End() { return Iter{ this->Size, this }; }

  Iter begin() { return this->Begin(); }
  Iter end() { return this->End(); }

  std::size_t Size{ 0 };
  const IdType* CellIds{ nullptr };
  const double* Parameters{ nullptr };
};

} // anonymous namespace

bool vtkDGInvokeOperator::Invoke(const vtkDGOperatorEntry& op,
  const vtkCellAttribute::CellTypeInfo& info, std::size_t num, const vtkIdType* cellIds,
  const double* rst, double* result)
{
  if (!cellIds || !rst || !result)
  {
    return false;
  }
  if (num <= 0)
  {
    return true;
  }

  SimpleInputPoints<vtkIdType> in(num, cellIds, rst);
  return this->InvokeOp(op, info, in.begin(), in.end(), result);
}

VTK_ABI_NAMESPACE_END
