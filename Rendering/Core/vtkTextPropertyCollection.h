/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextPropertyCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTextPropertyCollection - a list of vtkTextProperty objects.
// .SECTION Description
// vtkTextPropertyCollection represents and provides methods to manipulate a
// list of TextProperty objects. The list is unsorted and
// duplicate entries are not prevented.
// .SECTION see also
// vtkTextProperty vtkCollection

#ifndef vtkTextPropertyCollection_h
#define vtkTextPropertyCollection_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkCollection.h"
#include "vtkTextProperty.h" // for inline functions

class VTKRENDERINGCORE_EXPORT vtkTextPropertyCollection : public vtkCollection
{
 public:
  static vtkTextPropertyCollection *New();
  vtkTypeMacro(vtkTextPropertyCollection, vtkCollection)
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a vtkTextProperty to the list.
  void AddItem(vtkTextProperty *a);

  // Description:
  // Get the next vtkTextProperty in the list.
  vtkTextProperty *GetNextItem();

  // Description:
  // Get the vtkTextProperty at the specified index.
  vtkTextProperty *GetItem(int idx);

  // Description:
  // Get the last TextProperty in the list.
  vtkTextProperty *GetLastItem();

  // Description:
  // Reentrant safe way to get an object in a collection. Just pass the
  // same cookie back and forth.
  vtkTextProperty *GetNextTextProperty(vtkCollectionSimpleIterator &cookie);

protected:
  vtkTextPropertyCollection();
  ~vtkTextPropertyCollection();

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o);

private:
  vtkTextPropertyCollection(const vtkTextPropertyCollection&);  // Not implemented.
  void operator=(const vtkTextPropertyCollection&);  // Not implemented.
};

inline void vtkTextPropertyCollection::AddItem(vtkTextProperty *a)
{
  this->vtkCollection::AddItem(a);
}

inline vtkTextProperty *vtkTextPropertyCollection::GetNextItem()
{
  return static_cast<vtkTextProperty *>(this->GetNextItemAsObject());
}

inline vtkTextProperty *vtkTextPropertyCollection::GetItem(int idx)
{
  return static_cast<vtkTextProperty *>(this->GetItemAsObject(idx));
}

inline vtkTextProperty *vtkTextPropertyCollection::GetLastItem()
{
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return static_cast<vtkTextProperty *>(this->Bottom->Item);
    }
}

inline vtkTextProperty *
vtkTextPropertyCollection::GetNextTextProperty(vtkCollectionSimpleIterator &it)
{
  return static_cast<vtkTextProperty *>(this->GetNextItemAsObject(it));
}

inline void vtkTextPropertyCollection::AddItem(vtkObject *o)
{
  this->vtkCollection::AddItem(o);
}

#endif
