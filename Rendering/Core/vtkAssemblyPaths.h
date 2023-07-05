// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAssemblyPaths
 * @brief   a list of lists of props representing an assembly hierarchy
 *
 * vtkAssemblyPaths represents an assembly hierarchy as a list of
 * vtkAssemblyPath. Each path represents the complete path from the
 * top level assembly (if any) down to the leaf prop.
 *
 * @sa
 * vtkAssemblyPath vtkAssemblyNode vtkPicker vtkAssembly vtkProp
 */

#ifndef vtkAssemblyPaths_h
#define vtkAssemblyPaths_h

#include "vtkCollection.h"
#include "vtkRenderingCoreModule.h" // For export macro

#include "vtkAssemblyPath.h" // Needed for inline methods

VTK_ABI_NAMESPACE_BEGIN
class vtkProp;

class VTKRENDERINGCORE_EXPORT vtkAssemblyPaths : public vtkCollection
{
public:
  static vtkAssemblyPaths* New();
  vtkTypeMacro(vtkAssemblyPaths, vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add a path to the list.
   */
  void AddItem(vtkAssemblyPath* p);

  /**
   * Remove a path from the list.
   */
  void RemoveItem(vtkAssemblyPath* p);

  /**
   * Determine whether a particular path is present. If the return value is
   * 0, the object was not found. If the object was found, the location is
   * the return value-1.
   */
  int IsItemPresent(vtkAssemblyPath* p);

  /**
   * Get the next path in the list.
   */
  vtkAssemblyPath* GetNextItem();

  /**
   * Override the standard GetMTime() to check for the modified times
   * of the paths.
   */
  vtkMTimeType GetMTime() override;

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkAssemblyPath* GetNextPath(vtkCollectionSimpleIterator& cookie)
  {
    return static_cast<vtkAssemblyPath*>(this->GetNextItemAsObject(cookie));
  }

protected:
  vtkAssemblyPaths() = default;
  ~vtkAssemblyPaths() override = default;

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject* o) { this->vtkCollection::AddItem(o); }
  void RemoveItem(vtkObject* o) { this->vtkCollection::RemoveItem(o); }
  void RemoveItem(int i) { this->vtkCollection::RemoveItem(i); }
  int IsItemPresent(vtkObject* o) { return this->vtkCollection::IsItemPresent(o); }

  vtkAssemblyPaths(const vtkAssemblyPaths&) = delete;
  void operator=(const vtkAssemblyPaths&) = delete;
};

inline void vtkAssemblyPaths::AddItem(vtkAssemblyPath* p)
{
  this->vtkCollection::AddItem(p);
}

inline void vtkAssemblyPaths::RemoveItem(vtkAssemblyPath* p)
{
  this->vtkCollection::RemoveItem(p);
}

inline int vtkAssemblyPaths::IsItemPresent(vtkAssemblyPath* p)
{
  return this->vtkCollection::IsItemPresent(p);
}

inline vtkAssemblyPath* vtkAssemblyPaths::GetNextItem()
{
  return static_cast<vtkAssemblyPath*>(this->GetNextItemAsObject());
}

VTK_ABI_NAMESPACE_END
#endif
