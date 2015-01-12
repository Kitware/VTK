/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssemblyPaths.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAssemblyPaths - a list of lists of props representing an assembly hierarchy
// .SECTION Description
// vtkAssemblyPaths represents an assembly hierarchy as a list of
// vtkAssemblyPath. Each path represents the complete path from the
// top level assembly (if any) down to the leaf prop.

// .SECTION see also
// vtkAssemblyPath vtkAssemblyNode vtkPicker vtkAssembly vtkProp

#ifndef vtkAssemblyPaths_h
#define vtkAssemblyPaths_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkCollection.h"

#include "vtkAssemblyPath.h" // Needed for inline methods

class vtkProp;

class VTKRENDERINGCORE_EXPORT vtkAssemblyPaths : public vtkCollection
{
public:
  static vtkAssemblyPaths *New();
  vtkTypeMacro(vtkAssemblyPaths, vtkCollection);

  // Description:
  // Add a path to the list.
  void AddItem(vtkAssemblyPath *p);

  // Description:
  // Remove a path from the list.
  void RemoveItem(vtkAssemblyPath *p);

  // Description:
  // Determine whether a particular path is present. Returns its position
  // in the list.
  int IsItemPresent(vtkAssemblyPath *p);

  // Description:
  // Get the next path in the list.
  vtkAssemblyPath *GetNextItem();

  // Description:
  // Override the standard GetMTime() to check for the modified times
  // of the paths.
  virtual unsigned long GetMTime();

  //BTX
  // Description:
  // Reentrant safe way to get an object in a collection. Just pass the
  // same cookie back and forth.
  vtkAssemblyPath *GetNextPath(vtkCollectionSimpleIterator &cookie)
    { return static_cast<vtkAssemblyPath *>(this->GetNextItemAsObject(cookie)); }
  //ETX

protected:
  vtkAssemblyPaths() {}
  ~vtkAssemblyPaths() {}

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o)
    { this->vtkCollection::AddItem(o); }
  void RemoveItem(vtkObject *o)
    { this->vtkCollection::RemoveItem(o); }
  void RemoveItem(int i)
    { this->vtkCollection::RemoveItem(i); }
  int  IsItemPresent(vtkObject *o)
    { return this->vtkCollection::IsItemPresent(o); }
private:
  vtkAssemblyPaths(const vtkAssemblyPaths&);  // Not implemented.
  void operator=(const vtkAssemblyPaths&);  // Not implemented.
};

inline void vtkAssemblyPaths::AddItem(vtkAssemblyPath *p)
{
  this->vtkCollection::AddItem(p);
}

inline void vtkAssemblyPaths::RemoveItem(vtkAssemblyPath *p)
{
  this->vtkCollection::RemoveItem(p);
}

inline int vtkAssemblyPaths::IsItemPresent(vtkAssemblyPath *p)
{
  return this->vtkCollection::IsItemPresent(p);
}

inline vtkAssemblyPath *vtkAssemblyPaths::GetNextItem()
{
  return static_cast<vtkAssemblyPath *>(this->GetNextItemAsObject());
}

#endif
// VTK-HeaderTest-Exclude: vtkAssemblyPaths.h
