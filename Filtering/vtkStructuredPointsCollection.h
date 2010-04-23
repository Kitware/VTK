/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkStructuredPointsCollection - maintain a list of structured points data objects
// .SECTION Description
// vtkStructuredPointsCollection is an object that creates and manipulates
// lists of structured points datasets. See also vtkCollection and
// subclasses.

#ifndef __vtkStructuredPointsCollection_h
#define __vtkStructuredPointsCollection_h

#include "vtkCollection.h"
#include "vtkStructuredPoints.h" // Needed for static cast

class VTK_FILTERING_EXPORT vtkStructuredPointsCollection : public vtkCollection
{
public:
  static vtkStructuredPointsCollection *New();
  vtkTypeMacro(vtkStructuredPointsCollection,vtkCollection);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a pointer to a vtkStructuredPoints to the list.
  void AddItem(vtkStructuredPoints *ds)
    {
      this->vtkCollection::AddItem(ds);
    }
  
  // Description:
  // Get the next item in the collection. NULL is returned if the collection
  // is exhausted.
  vtkStructuredPoints *GetNextItem() {
    return static_cast<vtkStructuredPoints *>(this->GetNextItemAsObject());};

    //BTX
  // Description: 
  // Reentrant safe way to get an object in a collection. Just pass the
  // same cookie back and forth. 
  vtkStructuredPoints *GetNextStructuredPoints(
    vtkCollectionSimpleIterator &cookie) {
    return static_cast<vtkStructuredPoints *>(
      this->GetNextItemAsObject(cookie));};
  //ETX

protected:
  vtkStructuredPointsCollection() {};
  ~vtkStructuredPointsCollection() {};
  
  

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

private:
  vtkStructuredPointsCollection(const vtkStructuredPointsCollection&);  // Not implemented.
  void operator=(const vtkStructuredPointsCollection&);  // Not implemented.
};


#endif
