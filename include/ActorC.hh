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
#ifndef __vlActorC_hh
#define __vlActorC_hh

#include "Collect.hh"
#include "Actor.hh"

class vlActorCollection : public vlCollection
{
 public:
  void AddItem(vlActor *a) {this->vlCollection::AddItem((vlObject *)a);};
  void RemoveItem(vlActor *a) 
    {this->vlCollection::RemoveItem((vlObject *)a);};
  int IsItemPresent(vlActor *a) 
    {return this->vlCollection::IsItemPresent((vlObject *)a);};
  vlActor *GetItem(int num) 
    { return (vlActor *)(this->vlCollection::GetItem(num));};
  char *GetClassName() {return "vlActorCollection";};
};

#endif





