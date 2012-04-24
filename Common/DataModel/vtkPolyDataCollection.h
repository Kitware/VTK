/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPolyDataCollection - maintain a list of polygonal data objects
// .SECTION Description
// vtkPolyDataCollection is an object that creates and manipulates lists of
// datasets of type vtkPolyData.

// .SECTION See Also
// vtkDataSetCollection vtkCollection

#ifndef __vtkPolyDataCollection_h
#define __vtkPolyDataCollection_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCollection.h"

#include "vtkPolyData.h" // Needed for static cast

class VTKCOMMONDATAMODEL_EXPORT vtkPolyDataCollection : public vtkCollection
{
public:
  static vtkPolyDataCollection *New();
  vtkTypeMacro(vtkPolyDataCollection,vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a poly data to the list.
  void AddItem(vtkPolyData *pd)
    {
      this->vtkCollection::AddItem(pd);
    }

  // Description:
  // Get the next poly data in the list.
  vtkPolyData *GetNextItem() {
    return static_cast<vtkPolyData *>(this->GetNextItemAsObject());};

  //BTX
  // Description:
  // Reentrant safe way to get an object in a collection. Just pass the
  // same cookie back and forth.
  vtkPolyData *GetNextPolyData(vtkCollectionSimpleIterator &cookie) {
    return static_cast<vtkPolyData *>(this->GetNextItemAsObject(cookie));};
  //ETX

protected:
  vtkPolyDataCollection() {};
  ~vtkPolyDataCollection() {};

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

private:
  vtkPolyDataCollection(const vtkPolyDataCollection&);  // Not implemented.
  void operator=(const vtkPolyDataCollection&);  // Not implemented.
};


#endif
