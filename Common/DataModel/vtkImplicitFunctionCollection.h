/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitFunctionCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImplicitFunctionCollection - maintain a list of implicit functions
// .SECTION Description
// vtkImplicitFunctionCollection is an object that creates and manipulates
// lists of objects of type vtkImplicitFunction.
// .SECTION See Also
// vtkCollection vtkPlaneCollection

#ifndef __vtkImplicitFunctionCollection_h
#define __vtkImplicitFunctionCollection_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCollection.h"

#include "vtkImplicitFunction.h" // Needed for inline methods

class VTKCOMMONDATAMODEL_EXPORT vtkImplicitFunctionCollection : public vtkCollection
{
public:
  vtkTypeMacro(vtkImplicitFunctionCollection,vtkCollection);
  static vtkImplicitFunctionCollection *New();

  // Description:
  // Add an implicit function to the list.
  void AddItem(vtkImplicitFunction *);

  // Description:
  // Get the next implicit function in the list.
  vtkImplicitFunction *GetNextItem();

  //BTX
  // Description:
  // Reentrant safe way to get an object in a collection. Just pass the
  // same cookie back and forth.
  vtkImplicitFunction *GetNextImplicitFunction(
    vtkCollectionSimpleIterator &cookie)
    {
      return static_cast<vtkImplicitFunction *>(
        this->GetNextItemAsObject(cookie));
    };
  //ETX

protected:
  vtkImplicitFunctionCollection() {}
  ~vtkImplicitFunctionCollection() {}


private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

private:
  vtkImplicitFunctionCollection(const vtkImplicitFunctionCollection&);  // Not implemented.
  void operator=(const vtkImplicitFunctionCollection&);  // Not implemented.
};

inline void vtkImplicitFunctionCollection::AddItem(vtkImplicitFunction *f)
{
  this->vtkCollection::AddItem(f);
}

inline vtkImplicitFunction *vtkImplicitFunctionCollection::GetNextItem()
{
 return static_cast<vtkImplicitFunction *>(this->GetNextItemAsObject());
}

#endif
// VTK-HeaderTest-Exclude: vtkImplicitFunctionCollection.h
