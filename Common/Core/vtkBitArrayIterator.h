// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBitArrayIterator
 * @brief   Iterator for vtkBitArray.
 * This iterator iterates over a vtkBitArray. It uses the double interface
 * to get/set bit values.
 */

#ifndef vtkBitArrayIterator_h
#define vtkBitArrayIterator_h

#include "vtkArrayIterator.h"
#include "vtkBitArray.h"         // For vtkBitArray
#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDeprecation.h"      // For VTK_DEPRECATED_IN_9_7_0
#include "vtkObjectFactory.h"    // For VTK_STANDARD_NEW_BODY

VTK_ABI_NAMESPACE_BEGIN
class VTK_DEPRECATED_IN_9_7_0("Use vtkArrayDispatch") VTKCOMMONCORE_EXPORT vtkBitArrayIterator
  : public vtkArrayIterator
{
public:
  static vtkBitArrayIterator* New() { VTK_STANDARD_NEW_BODY(vtkBitArrayIterator); }
  vtkTypeMacro(vtkBitArrayIterator, vtkArrayIterator);
  void PrintSelf(ostream& os, vtkIndent indent) override
  {
    this->Superclass::PrintSelf(os, indent);
  }

  /**
   * Set the array this iterator will iterate over.
   * After Initialize() has been called, the iterator is valid
   * so long as the Array has not been modified
   * (except using the iterator itself).
   * If the array is modified, the iterator must be re-initialized.
   */
  void Initialize(vtkAbstractArray* array) override
  {
    vtkBitArray* b = vtkArrayDownCast<vtkBitArray>(array);
    if (!b && array)
    {
      vtkErrorMacro("vtkBitArrayIterator can iterate only over vtkBitArray.");
      return;
    }
    this->SetArray(b);
  }

  /**
   * Get the array.
   */
  vtkAbstractArray* GetArray() { return this->Array; }

  /**
   * Must be called only after Initialize.
   */
  int* GetTuple(vtkIdType id)
  {
    if (!this->Array)
    {
      return nullptr;
    }

    vtkIdType numComps = this->Array->GetNumberOfComponents();
    if (this->TupleSize < numComps)
    {
      this->TupleSize = static_cast<int>(numComps);
      delete[] this->Tuple;
      this->Tuple = new int[this->TupleSize];
    }
    vtkIdType loc = id * numComps;
    for (int j = 0; j < numComps; j++)
    {
      this->Tuple[j] = this->Array->GetValue(loc + j);
    }
    return this->Tuple;
  }

  /**
   * Must be called only after Initialize.
   */
  int GetValue(vtkIdType id)
  {
    if (this->Array)
    {
      return this->Array->GetValue(id);
    }
    vtkErrorMacro("Array Iterator not initialized.");
    return 0;
  }

  /**
   * Must be called only after Initialize.
   */
  vtkIdType GetNumberOfTuples() const
  {
    if (this->Array)
    {
      return this->Array->GetNumberOfTuples();
    }
    return 0;
  }

  /**
   * Must be called only after Initialize.
   */
  vtkIdType GetNumberOfValues() const
  {
    if (this->Array)
    {
      return this->Array->GetNumberOfTuples() * this->Array->GetNumberOfComponents();
    }
    return 0;
  }

  /**
   * Must be called only after Initialize.
   */
  int GetNumberOfComponents() const
  {
    if (this->Array)
    {
      return this->Array->GetNumberOfComponents();
    }
    return 0;
  }

  /**
   * Get the data type from the underlying array.
   */
  int GetDataType() const override
  {
    if (this->Array)
    {
      return this->Array->GetDataType();
    }
    return 0;
  }

  /**
   * Get the data type size from the underlying array.
   */
  int GetDataTypeSize() const
  {
    if (this->Array)
    {
      return this->Array->GetDataTypeSize();
    }
    return 0;
  }

  /**
   * Sets the value at the index. This does not verify if the index is valid.
   * The caller must ensure that id is less than the maximum number of values.
   */
  void SetValue(vtkIdType id, int value)
  {
    if (this->Array)
    {
      this->Array->SetValue(id, value);
    }
  }

  /**
   * Data type of a value.
   */
  typedef int ValueType;

protected:
  vtkBitArrayIterator()
  {
    this->Array = nullptr;
    this->Tuple = nullptr;
    this->TupleSize = 0;
  }
  ~vtkBitArrayIterator() override
  {
    this->SetArray(nullptr);
    delete[] this->Tuple;
  }

  int* Tuple;
  int TupleSize;
  void SetArray(vtkBitArray* b) { vtkSetObjectBodyMacro(Array, vtkBitArray, b); }
  vtkBitArray* Array;

private:
  vtkBitArrayIterator(const vtkBitArrayIterator&) = delete;
  void operator=(const vtkBitArrayIterator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
