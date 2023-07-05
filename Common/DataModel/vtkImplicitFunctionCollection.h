// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImplicitFunctionCollection
 * @brief   maintain a list of implicit functions
 *
 * vtkImplicitFunctionCollection is an object that creates and manipulates
 * lists of objects of type vtkImplicitFunction.
 * @sa
 * vtkCollection vtkPlaneCollection
 */

#ifndef vtkImplicitFunctionCollection_h
#define vtkImplicitFunctionCollection_h

#include "vtkCollection.h"
#include "vtkCommonDataModelModule.h" // For export macro

#include "vtkImplicitFunction.h" // Needed for inline methods

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT vtkImplicitFunctionCollection : public vtkCollection
{
public:
  vtkTypeMacro(vtkImplicitFunctionCollection, vtkCollection);
  static vtkImplicitFunctionCollection* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add an implicit function to the list.
   */
  void AddItem(vtkImplicitFunction*);

  /**
   * Get the next implicit function in the list.
   */
  vtkImplicitFunction* GetNextItem();

  ///@{
  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkImplicitFunction* GetNextImplicitFunction(vtkCollectionSimpleIterator& cookie)
  {
    return static_cast<vtkImplicitFunction*>(this->GetNextItemAsObject(cookie));
  }
  ///@}

protected:
  vtkImplicitFunctionCollection() = default;
  ~vtkImplicitFunctionCollection() override = default;

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject* o) { this->vtkCollection::AddItem(o); }

  vtkImplicitFunctionCollection(const vtkImplicitFunctionCollection&) = delete;
  void operator=(const vtkImplicitFunctionCollection&) = delete;
};

inline void vtkImplicitFunctionCollection::AddItem(vtkImplicitFunction* f)
{
  this->vtkCollection::AddItem(f);
}

inline vtkImplicitFunction* vtkImplicitFunctionCollection::GetNextItem()
{
  return static_cast<vtkImplicitFunction*>(this->GetNextItemAsObject());
}

VTK_ABI_NAMESPACE_END
#endif
