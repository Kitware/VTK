/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ActorC.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkActorCollection - a list of actors
// .SECTION Description
// vtkActorCollection represents and provides methods to manipulate list of
// actors (i.e., vtkActor and subclasses). The list is unsorted and duplicate
// entries are not prevented.

#ifndef __vtkActorC_hh
#define __vtkActorC_hh

#include "Collect.hh"
#include "Actor.hh"

class vtkActorCollection : public vtkCollection
{
 public:
  char *GetClassName() {return "vtkActorCollection";};

  void AddItem(vtkActor *a);
  void RemoveItem(vtkActor *a);
  int IsItemPresent(vtkActor *a);
  vtkActor *GetNextItem();
};

// Description:
// Add an actor to the list.
inline void vtkActorCollection::AddItem(vtkActor *a) 
{
  this->vtkCollection::AddItem((vtkObject *)a);
}

// Description:
// Remove an actor from the list.
inline void vtkActorCollection::RemoveItem(vtkActor *a) 
{
  this->vtkCollection::RemoveItem((vtkObject *)a);
}

// Description:
// Determine whether a particular actor is present. Returns its position
// in the list.
inline int vtkActorCollection::IsItemPresent(vtkActor *a) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)a);
}

// Description:
// Get the next actor in the list.
inline vtkActor *vtkActorCollection::GetNextItem() 
{ 
  return (vtkActor *)(this->vtkCollection::GetNextItem());
}

#endif





