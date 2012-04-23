/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageReader2Collection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageReader2Collection - maintain a list of image readers
// .SECTION Description
// vtkImageReader2Collection is an object that creates and manipulates
// lists of objects of type vtkImageReader2 and its subclasses.
// .SECTION See Also
// vtkCollection vtkPlaneCollection

#ifndef __vtkImageReader2Collection_h
#define __vtkImageReader2Collection_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkCollection.h"

class vtkImageReader2;

class VTKIOIMAGE_EXPORT vtkImageReader2Collection : public vtkCollection
{
public:
  vtkTypeMacro(vtkImageReader2Collection,vtkCollection);
  static vtkImageReader2Collection *New();
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add an image reader to the list.
  void AddItem(vtkImageReader2 *);

  // Description:
  // Get the next image reader in the list.
  vtkImageReader2 *GetNextItem();

  //BTX
  // Description:
  // Reentrant safe way to get an object in a collection. Just pass the
  // same cookie back and forth.
  vtkImageReader2 *GetNextImageReader2(vtkCollectionSimpleIterator &cookie);
  //ETX

protected:
  vtkImageReader2Collection() {};
  ~vtkImageReader2Collection() {};


private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

private:
  vtkImageReader2Collection(const vtkImageReader2Collection&);  // Not implemented.
  void operator=(const vtkImageReader2Collection&);  // Not implemented.
};

#endif
