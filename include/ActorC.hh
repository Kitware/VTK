/*=========================================================================

  Program:   OSCAR 
  Module:    ActorC.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the vis library

- Ken Martin

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
};

#endif





