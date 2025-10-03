// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkBuffer.h"
#include "vtkGenericDataArray.h"

/**
 * This class is used in some unit tests to setup a mock data array which derives
 * vtkGenericDataArray
 */
template <typename ValueT>
class MockDataArray : public vtkGenericDataArray<MockDataArray<ValueT>, ValueT>
{
  using GenericDataArrayType = vtkGenericDataArray<MockDataArray<ValueT>, ValueT>;

public:
  vtkTemplateTypeMacro(MockDataArray<ValueT>, GenericDataArrayType);
  using ValueType = typename Superclass::ValueType;
  static MockDataArray* New() { VTK_STANDARD_NEW_BODY(MockDataArray<ValueT>); }
  void* GetVoidPointer(vtkIdType idx) override { return this->Buffer->GetBuffer() + idx; }
  ValueType GetValue(vtkIdType valueIdx) const { return this->Buffer->GetBuffer()[valueIdx]; }
  void SetValue(vtkIdType valueIdx, ValueType value)
  {
    this->Buffer->GetBuffer()[valueIdx] = value;
  }
  void GetTypedTuple(vtkIdType tupleIdx, ValueType* tuple) const
  {
    const vtkIdType valueIdx = tupleIdx * this->NumberOfComponents;
    std::copy(this->Buffer->GetBuffer() + valueIdx,
      this->Buffer->GetBuffer() + valueIdx + this->NumberOfComponents, tuple);
  }
  void SetTypedTuple(vtkIdType tupleIdx, const ValueType* tuple)
  {
    const vtkIdType valueIdx = tupleIdx * this->NumberOfComponents;
    std::copy(tuple, tuple + this->NumberOfComponents, this->Buffer->GetBuffer() + valueIdx);
  }
  ValueType GetTypedComponent(vtkIdType tupleIdx, int compIdx) const
  {
    return this->Buffer->GetBuffer()[this->NumberOfComponents * tupleIdx + compIdx];
  }
  void SetTypedComponent(vtkIdType tupleIdx, int compIdx, ValueType value)
  {
    const vtkIdType valueIdx = tupleIdx * this->NumberOfComponents + compIdx;
    this->SetValue(valueIdx, value);
  }

protected:
  vtkNew<vtkBuffer<ValueT>> Buffer;
  bool AllocateTuples(vtkIdType numTuples)
  {
    vtkIdType numValues = numTuples * this->GetNumberOfComponents();
    if (this->Buffer->Allocate(numValues))
    {
      this->Size = this->Buffer->GetSize();
      return true;
    }
    return false;
  }
  bool ReallocateTuples(vtkIdType numTuples)
  {
    if (this->Buffer->Reallocate(numTuples * this->GetNumberOfComponents()))
    {
      this->Size = this->Buffer->GetSize();
      return true;
    }
    return false;
  }
  friend class vtkGenericDataArray<MockDataArray<ValueT>, ValueT>;
};
