// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPropCollection
 * @brief   an ordered list of Props
 *
 * vtkPropCollection represents and provides methods to manipulate a list of
 * Props (i.e., vtkProp and subclasses). The list is ordered and duplicate
 * entries are not prevented.
 *
 * @sa
 * vtkProp vtkCollection
 */

#ifndef vtkPropCollection_h
#define vtkPropCollection_h

#include "vtkCollection.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO

#include "vtkProp.h" // Needed for inline methods

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkPropCollection : public vtkCollection
{
public:
  static vtkPropCollection* New();
  vtkTypeMacro(vtkPropCollection, vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add a Prop to the bottom of the list.
   */
  void AddItem(vtkProp* a);

  /**
   * Get the next Prop in the list.
   */
  vtkProp* GetNextProp();

  /**
   * Get the last Prop in the list.
   */
  vtkProp* GetLastProp();

  /**
   * Get the number of paths contained in this list. (Recall that a
   * vtkProp can consist of multiple parts.) Used in picking and other
   * activities to get the parts of composite entities like vtkAssembly
   * or vtkPropAssembly.
   */
  int GetNumberOfPaths();

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkProp* GetNextProp(vtkCollectionSimpleIterator& cookie)
  {
    return static_cast<vtkProp*>(this->GetNextItemAsObject(cookie));
  }

protected:
  vtkPropCollection() = default;
  ~vtkPropCollection() override = default;

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject* o) { this->vtkCollection::AddItem(o); }

  vtkPropCollection(const vtkPropCollection&) = delete;
  void operator=(const vtkPropCollection&) = delete;
};

inline void vtkPropCollection::AddItem(vtkProp* a)
{
  this->vtkCollection::AddItem(a);
}

inline vtkProp* vtkPropCollection::GetNextProp()
{
  return static_cast<vtkProp*>(this->GetNextItemAsObject());
}

inline vtkProp* vtkPropCollection::GetLastProp()
{
  if (this->Bottom == nullptr)
  {
    return nullptr;
  }
  else
  {
    return static_cast<vtkProp*>(this->Bottom->Item);
  }
}

VTK_ABI_NAMESPACE_END
#endif
