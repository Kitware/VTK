// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkInformationIterator
 * @brief   Iterates over keys of an information object
 *
 * vtkInformationIterator can be used to iterate over the keys of an
 * information object. The corresponding values can then be directly
 * obtained from the information object using the keys.
 *
 * @sa
 * vtkInformation vtkInformationKey
 */

#ifndef vtkInformationIterator_h
#define vtkInformationIterator_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkInformation;
class vtkInformationKey;
class vtkInformationIteratorInternals;

class VTKCOMMONCORE_EXPORT vtkInformationIterator : public vtkObject
{
public:
  static vtkInformationIterator* New();
  vtkTypeMacro(vtkInformationIterator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the information to iterator over.
   */
  void SetInformation(vtkInformation*);
  vtkGetObjectMacro(Information, vtkInformation);
  ///@}

  /**
   * Set the function to iterate over. The iterator
   * will not hold a reference to the information object.
   * Can be used to optimize certain places by avoiding
   * garbage collection.
   */
  void SetInformationWeak(vtkInformation*);

  /**
   * Move the iterator to the beginning of the collection.
   */
  void InitTraversal() { this->GoToFirstItem(); }

  /**
   * Move the iterator to the beginning of the collection.
   */
  virtual void GoToFirstItem();

  /**
   * Move the iterator to the next item in the collection.
   */
  virtual void GoToNextItem();

  /**
   * Test whether the iterator is currently pointing to a valid
   * item. Returns 1 for yes, 0 for no.
   */
  virtual int IsDoneWithTraversal();

  /**
   * Get the current item. Valid only when IsDoneWithTraversal()
   * returns 1.
   */
  virtual vtkInformationKey* GetCurrentKey();

protected:
  vtkInformationIterator();
  ~vtkInformationIterator() override;

  vtkInformation* Information;
  vtkInformationIteratorInternals* Internal;

  bool ReferenceIsWeak;

private:
  vtkInformationIterator(const vtkInformationIterator&) = delete;
  void operator=(const vtkInformationIterator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
