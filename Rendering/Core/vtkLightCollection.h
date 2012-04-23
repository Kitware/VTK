/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLightCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLightCollection - a list of lights
// .SECTION Description
// vtkLightCollection represents and provides methods to manipulate a list of
// lights (i.e., vtkLight and subclasses). The list is unsorted and duplicate
// entries are not prevented.

// .SECTION see also
// vtkCollection vtkLight

#ifndef __vtkLightCollection_h
#define __vtkLightCollection_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkCollection.h"

class vtkLight;

class VTKRENDERINGCORE_EXPORT vtkLightCollection : public vtkCollection
{
 public:
  static vtkLightCollection *New();
  vtkTypeMacro(vtkLightCollection,vtkCollection);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a light to the list.
  void AddItem(vtkLight *a);

  // Description:
  // Get the next light in the list. NULL is returned when the collection is
  // exhausted.
  vtkLight *GetNextItem();

  //BTX
  // Description:
  // Reentrant safe way to get an object in a collection. Just pass the
  // same cookie back and forth.
  vtkLight *GetNextLight(vtkCollectionSimpleIterator &cookie);
  //ETX

protected:
  vtkLightCollection() {};
  ~vtkLightCollection() {};


private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

private:
  vtkLightCollection(const vtkLightCollection&);  // Not implemented.
  void operator=(const vtkLightCollection&);  // Not implemented.
};


#endif

