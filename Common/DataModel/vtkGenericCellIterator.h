// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGenericCellIterator
 * @brief   iterator used to traverse cells
 *
 * This class (and subclasses) are used to iterate over cells. Use it
 * only in conjunction with vtkGenericDataSet (i.e., the adaptor framework).
 *
 * Typical use is:
 * <pre>
 * vtkGenericDataSet *dataset;
 * vtkGenericCellIterator *it = dataset->NewCellIterator(2);
 * for (it->Begin(); !it->IsAtEnd(); it->Next());
 *   {
 *   spec=it->GetCell();
 *   }
 * </pre>
 */

#ifndef vtkGenericCellIterator_h
#define vtkGenericCellIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkGenericAdaptorCell;

class VTKCOMMONDATAMODEL_EXPORT vtkGenericCellIterator : public vtkObject
{
public:
  ///@{
  /**
   * Standard VTK construction and type macros.
   */
  vtkTypeMacro(vtkGenericCellIterator, vtkObject);
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
   * Create an empty cell. The user is responsible for deleting it.
   * \post result_exists: result!=0
   */
  virtual vtkGenericAdaptorCell* NewCell() = 0;

  /**
   * Get the cell at current position. The cell should be instantiated
   * with the NewCell() method.
   * \pre not_at_end: !IsAtEnd()
   * \pre c_exists: c!=0
   * THREAD SAFE
   */
  virtual void GetCell(vtkGenericAdaptorCell* c) = 0;

  /**
   * Get the cell at the current traversal position.
   * NOT THREAD SAFE
   * \pre not_at_end: !IsAtEnd()
   * \post result_exits: result!=0
   */
  virtual vtkGenericAdaptorCell* GetCell() = 0;

  /**
   * Move the iterator to the next position in the list.
   * \pre not_at_end: !IsAtEnd()
   */
  virtual void Next() = 0;

protected:
  vtkGenericCellIterator();
  ~vtkGenericCellIterator() override;

private:
  vtkGenericCellIterator(const vtkGenericCellIterator&) = delete;
  void operator=(const vtkGenericCellIterator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
