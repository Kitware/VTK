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

#include "Actor.hh"

class vlActorListElement
{
 public:
  vlActor *Actor;
  vlActorListElement *Next;

};

class vlActorCollection : public vlObject
{
 public:
  int NumberOfItems;

 private:
  vlActorListElement *Top;
  vlActorListElement *Bottom;

 public:
  vlActorCollection();
  void AddMember(vlActor *);
  int  GetNumberOfMembers();
  vlActor *GetMember(int num);
  void PrintSelf(ostream& os, vlIndent indent);
  char *GetClassName() {return "vlActorCollection";};
};

#endif





