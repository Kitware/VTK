/*=========================================================================

  Program:   Visualization Library
  Module:    ActorC.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlActorCollection - a list of actors
// .SECTION Description
// vlActorCollection represents and provides methods to manipulate list of
// actors (i.e., vlActor and subclasses). The list is unsorted and duplicate
// entries are not prevented.

#ifndef __vlActorC_hh
#define __vlActorC_hh

#include "Collect.hh"
#include "Actor.hh"

class vlActorCollection : public vlCollection
{
 public:
  char *GetClassName() {return "vlActorCollection";};

  void AddItem(vlActor *a);
  void RemoveItem(vlActor *a);
  int IsItemPresent(vlActor *a);
  vlActor *GetNextItem();
};

// Description:
// Add an actor to the list.
inline void vlActorCollection::AddItem(vlActor *a) 
{
  this->vlCollection::AddItem((vlObject *)a);
}

// Description:
// Remove an actor from the list.
inline void vlActorCollection::RemoveItem(vlActor *a) 
{
  this->vlCollection::RemoveItem((vlObject *)a);
}

// Description:
// Determine whether a particular actor is present. Returns its position
// in the list.
inline int vlActorCollection::IsItemPresent(vlActor *a) 
{
  return this->vlCollection::IsItemPresent((vlObject *)a);
}

// Description:
// Get the next actor in the list.
inline vlActor *vlActorCollection::GetNextItem() 
{ 
  return (vlActor *)(this->vlCollection::GetNextItem());
}

#endif





