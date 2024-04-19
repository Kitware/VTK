// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGenericPointIterator
 * @brief   iterator used to traverse points
 *
 * This class (and subclasses) are used to iterate over points. Use it
 * only in conjunction with vtkGenericDataSet (i.e., the adaptor framework).
 *
 * Typical use is:
 * <pre>
 * vtkGenericDataSet *dataset;
 * vtkGenericPointIterator *it = dataset->NewPointIterator();
 * for (it->Begin(); !it->IsAtEnd(); it->Next());
 *   {
 *   x=it->GetPosition();
 *   }
 * </pre>
 */

#ifndef vtkGenericPointIterator_h
#define vtkGenericPointIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT vtkGenericPointIterator : public vtkObject
{
public:
  ///@{
  /**
   * Standard VTK construction and type macros.
   */
  vtkTypeMacro(vtkGenericPointIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Move iterator to first position if any (loop initialization).
   */
  virtual void Begin() = 0;

  /**
   * Is the iterator at the end of traversal?
   */
  virtual vtkTypeBool IsAtEnd() = 0;

  /**
   * Move the iterator to the next position in the list.
   * \pre not_off: !IsAtEnd()
   */
  virtual void Next() = 0;

  /**
   * Get the coordinates of the point at the current iterator position.
   * \pre not_off: !IsAtEnd()
   * \post result_exists: result!=0
   */
  virtual double* GetPosition() = 0;

  /**
   * Get the coordinates of the point at the current iterator position.
   * \pre not_off: !IsAtEnd()
   * \pre x_exists: x!=0
   */
  virtual void GetPosition(double x[3]) = 0;

  /**
   * Return the unique identifier for the point, could be non-contiguous.
   * \pre not_off: !IsAtEnd()
   */
  virtual vtkIdType GetId() = 0;

protected:
  ///@{
  /**
   * Destructor.
   */
  vtkGenericPointIterator();
  ~vtkGenericPointIterator() override;
  ///@}

private:
  vtkGenericPointIterator(const vtkGenericPointIterator&) = delete;
  void operator=(const vtkGenericPointIterator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
