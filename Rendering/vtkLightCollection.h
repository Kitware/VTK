/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLightCollection.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
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

#ifndef __vtkLightC_h
#define __vtkLightC_h

#include "vtkCollection.h"
#include "vtkLight.h"

class VTK_RENDERING_EXPORT vtkLightCollection : public vtkCollection
{
 public:
  static vtkLightCollection *New();
  vtkTypeRevisionMacro(vtkLightCollection,vtkCollection);

  // Description:
  // Add a light to the list.
  void AddItem(vtkLight *a);

  // Description:
  // Get the next light in the list. NULL is returned when the collection is 
  // exhausted.
  vtkLight *GetNextItem();

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

