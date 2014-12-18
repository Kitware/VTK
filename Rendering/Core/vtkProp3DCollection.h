/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProp3DCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkProp3DCollection - a list of 3D props
// .SECTION Description
// vtkProp3DCollection represents and provides methods to manipulate a list of
// 3D props (i.e., vtkProp3D and subclasses). The list is unsorted and
// duplicate entries are not prevented.

// .SECTION see also
// vtkProp3D vtkCollection

#ifndef vtkProp3DCollection_h
#define vtkProp3DCollection_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkPropCollection.h"
#include "vtkProp3D.h" // Needed for inline methods

class VTKRENDERINGCORE_EXPORT vtkProp3DCollection : public vtkPropCollection
{
public:
  static vtkProp3DCollection *New();
  vtkTypeMacro(vtkProp3DCollection,vtkPropCollection);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add an actor to the list.
  void AddItem(vtkProp3D *p);

  // Description:
  // Get the next actor in the list.
  vtkProp3D *GetNextProp3D();

  // Description:
  // Get the last actor in the list.
  vtkProp3D *GetLastProp3D();

  //BTX
  // Description:
  // Reentrant safe way to get an object in a collection. Just pass the
  // same cookie back and forth.
  vtkProp3D *GetNextProp3D(vtkCollectionSimpleIterator &cookie)
    {
      return static_cast<vtkProp3D *>(this->GetNextItemAsObject(cookie));
    }
  //ETX

protected:
  vtkProp3DCollection() {}
  ~vtkProp3DCollection() {}


private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o)
    {
      this->vtkCollection::AddItem(o);
    }
  void AddItem(vtkProp *o)
    {
      this->vtkPropCollection::AddItem(o);
    }

private:
  vtkProp3DCollection(const vtkProp3DCollection&);  // Not implemented.
  void operator=(const vtkProp3DCollection&);  // Not implemented.
};

inline void vtkProp3DCollection::AddItem(vtkProp3D *a)
{
  this->vtkCollection::AddItem(a);
}

inline vtkProp3D *vtkProp3DCollection::GetNextProp3D()
{
  return static_cast<vtkProp3D *>(this->GetNextItemAsObject());
}

inline vtkProp3D *vtkProp3DCollection::GetLastProp3D()
{
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return static_cast<vtkProp3D *>(this->Bottom->Item);
    }
}

#endif





