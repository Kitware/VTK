// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOverrideInformationCollection
 * @brief   maintain a list of override information objects
 *
 * vtkOverrideInformationCollection is an object that creates and manipulates
 * lists of objects of type vtkOverrideInformation.
 * @sa
 * vtkCollection
 */

#ifndef vtkOverrideInformationCollection_h
#define vtkOverrideInformationCollection_h

#include "vtkCollection.h"
#include "vtkCommonCoreModule.h" // For export macro

#include "vtkOverrideInformation.h" // Needed for inline methods

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkOverrideInformationCollection : public vtkCollection
{
public:
  vtkTypeMacro(vtkOverrideInformationCollection, vtkCollection);
  static vtkOverrideInformationCollection* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add a OverrideInformation to the list.
   */
  void AddItem(vtkOverrideInformation*);

  /**
   * Get the next OverrideInformation in the list.
   */
  vtkOverrideInformation* GetNextItem();

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkOverrideInformation* GetNextOverrideInformation(vtkCollectionSimpleIterator& cookie)
  {
    return static_cast<vtkOverrideInformation*>(this->GetNextItemAsObject(cookie));
  }

protected:
  vtkOverrideInformationCollection() = default;
  ~vtkOverrideInformationCollection() override = default;

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject* o) { this->vtkCollection::AddItem(o); }

  vtkOverrideInformationCollection(const vtkOverrideInformationCollection&) = delete;
  void operator=(const vtkOverrideInformationCollection&) = delete;
};

inline void vtkOverrideInformationCollection::AddItem(vtkOverrideInformation* f)
{
  this->vtkCollection::AddItem(f);
}

inline vtkOverrideInformation* vtkOverrideInformationCollection::GetNextItem()
{
  return static_cast<vtkOverrideInformation*>(this->GetNextItemAsObject());
}

VTK_ABI_NAMESPACE_END
#endif
