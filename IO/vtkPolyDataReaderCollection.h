/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataReaderCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPolyDataReaderCollection - maintain a list of image readers 
// .SECTION Description
// vtkPolyDataReaderCollection is an object that creates and manipulates
// lists of objects of type vtkAbstractPolyDataReader and its subclasses. 
// .SECTION See Also
// vtkCollection vtkPlaneCollection

#ifndef __vtkPolyDataReaderCollection_h
#define __vtkPolyDataReaderCollection_h

#include "vtkCollection.h"

class vtkAbstractPolyDataReader;

class VTK_IO_EXPORT vtkPolyDataReaderCollection : public vtkCollection
{
public:
  vtkTypeRevisionMacro(vtkPolyDataReaderCollection,vtkCollection);
  static vtkPolyDataReaderCollection *New();
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add an image reader to the list.
  void AddItem(vtkAbstractPolyDataReader *);

  // Description:
  // Get the next image reader in the list.
  vtkAbstractPolyDataReader *GetNextItem();
  
  //BTX
  // Description: 
  // Reentrant safe way to get an object in a collection. Just pass the
  // same cookie back and forth. 
  vtkAbstractPolyDataReader *GetNextPolyDataReader(vtkCollectionSimpleIterator &cookie);
  //ETX

protected:
  vtkPolyDataReaderCollection() {};
  ~vtkPolyDataReaderCollection() {};
  

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

private:
  vtkPolyDataReaderCollection(const vtkPolyDataReaderCollection&);  // Not implemented.
  void operator=(const vtkPolyDataReaderCollection&);  // Not implemented.
};

#endif
