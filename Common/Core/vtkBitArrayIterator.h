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
#include "vtkCommonCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkBitArray;
class VTKCOMMONCORE_EXPORT vtkBitArrayIterator : public vtkArrayIterator
{
public:
  static vtkBitArrayIterator* New();
  vtkTypeMacro(vtkBitArrayIterator, vtkArrayIterator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the array this iterator will iterate over.
   * After Initialize() has been called, the iterator is valid
   * so long as the Array has not been modified
   * (except using the iterator itself).
   * If the array is modified, the iterator must be re-initialized.
   */
  void Initialize(vtkAbstractArray* array) override;

  /**
   * Get the array.
   */
  vtkAbstractArray* GetArray();

  /**
   * Must be called only after Initialize.
   */
  int* GetTuple(vtkIdType id);

  /**
   * Must be called only after Initialize.
   */
  int GetValue(vtkIdType id);

  /**
   * Must be called only after Initialize.
   */
  vtkIdType GetNumberOfTuples() const;

  /**
   * Must be called only after Initialize.
   */
  vtkIdType GetNumberOfValues() const;

  /**
   * Must be called only after Initialize.
   */
  int GetNumberOfComponents() const;

  /**
   * Get the data type from the underlying array.
   */
  int GetDataType() const override;

  /**
   * Get the data type size from the underlying array.
   */
  int GetDataTypeSize() const;

  /**
   * Sets the value at the index. This does not verify if the index is valid.
   * The caller must ensure that id is less than the maximum number of values.
   */
  void SetValue(vtkIdType id, int value);

  /**
   * Data type of a value.
   */
  typedef int ValueType;

protected:
  vtkBitArrayIterator();
  ~vtkBitArrayIterator() override;

  int* Tuple;
  int TupleSize;
  void SetArray(vtkBitArray* b);
  vtkBitArray* Array;

private:
  vtkBitArrayIterator(const vtkBitArrayIterator&) = delete;
  void operator=(const vtkBitArrayIterator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
