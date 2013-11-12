/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataSetCollection - maintain an unordered list of dataset objects
// .SECTION Description
// vtkDataSetCollection is an object that creates and manipulates lists of
// datasets. See also vtkCollection and subclasses.

#ifndef __vtkDataSetCollection_h
#define __vtkDataSetCollection_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCollection.h"

#include "vtkDataSet.h" // Needed for inline methods.

class VTKCOMMONDATAMODEL_EXPORT vtkDataSetCollection : public vtkCollection
{
public:
  static vtkDataSetCollection *New();
  vtkTypeMacro(vtkDataSetCollection,vtkCollection);

  // Description:
  // Add a dataset to the list.
  void AddItem(vtkDataSet *ds)
    {
      this->vtkCollection::AddItem(ds);
    }

  // Description:
  // Get the next dataset in the list.
  vtkDataSet *GetNextItem() {
    return static_cast<vtkDataSet *>(this->GetNextItemAsObject());};
  vtkDataSet *GetNextDataSet() {
    return static_cast<vtkDataSet *>(this->GetNextItemAsObject());};

  // Description:
  // Get the ith dataset in the list.
  vtkDataSet *GetItem(int i) {
    return static_cast<vtkDataSet *>(this->GetItemAsObject(i));};
  vtkDataSet *GetDataSet(int i) {
    return static_cast<vtkDataSet *>(this->GetItemAsObject(i));};

  //BTX
  // Description:
  // Reentrant safe way to get an object in a collection. Just pass the
  // same cookie back and forth.
  vtkDataSet *GetNextDataSet(vtkCollectionSimpleIterator &cookie) {
    return static_cast<vtkDataSet *>(this->GetNextItemAsObject(cookie));};
  //ETX

protected:
  vtkDataSetCollection() {}
  ~vtkDataSetCollection() {}

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

private:
  vtkDataSetCollection(const vtkDataSetCollection&);  // Not implemented.
  void operator=(const vtkDataSetCollection&);  // Not implemented.
};


#endif
// VTK-HeaderTest-Exclude: vtkDataSetCollection.h
