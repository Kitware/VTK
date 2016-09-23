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
/**
 * @class   vtkImageReader2Collection
 * @brief   maintain a list of image readers
 *
 * vtkImageReader2Collection is an object that creates and manipulates
 * lists of objects of type vtkImageReader2 and its subclasses.
 * @sa
 * vtkCollection vtkPlaneCollection
*/

#ifndef vtkImageReader2Collection_h
#define vtkImageReader2Collection_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkCollection.h"

class vtkImageReader2;

class VTKIOIMAGE_EXPORT vtkImageReader2Collection : public vtkCollection
{
public:
  vtkTypeMacro(vtkImageReader2Collection,vtkCollection);
  static vtkImageReader2Collection *New();
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Add an image reader to the list.
   */
  void AddItem(vtkImageReader2 *);

  /**
   * Get the next image reader in the list.
   */
  vtkImageReader2 *GetNextItem();

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  vtkImageReader2 *GetNextImageReader2(vtkCollectionSimpleIterator &cookie);

protected:
  vtkImageReader2Collection() {}
  ~vtkImageReader2Collection() {}


private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

private:
  vtkImageReader2Collection(const vtkImageReader2Collection&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageReader2Collection&) VTK_DELETE_FUNCTION;
};

#endif
