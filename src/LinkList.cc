/*=========================================================================

  Program:   Visualization Library
  Module:    LinkList.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "LinkList.hh"

vlLinkList::vlLinkList(const int sz, const int ext)
{
  this->Size = sz;
  this->Array = new vlLink[sz];
  this->Extend = ext;
  this->MaxId = -1;
}

vlLinkList::~vlLinkList()
{
  delete [] this->Array;
}

//
// Add a link to structure
//
void vlLinkList::AllocateLinks()
{
  for (int i=0; i<=this->MaxId; i++)
    {
    this->Array[i].cells = new int[this->Array[i].ncells];
    }
}

void vlLinkList::Squeeze()
{
  this->Resize (this->MaxId+1);
}

void vlLinkList::Reset()
{
  this->MaxId = -1;
}
//
// Private function does "reallocate"
//
vlLink *vlLinkList::Resize(const int sz)
{
  int i;
  vlLink *newArray;
  int newSize;

  if ( sz >= this->Size ) newSize = this->Size + 
    this->Extend*(((sz-this->Size)/this->Extend)+1);
  else newSize = sz;

  if ( (newArray = new vlLink[newSize]) == 0 )
    {
    vlErrorMacro("Cannot allocate memory\n");
    return 0;
    }

  for (i=0; i<sz && i<this->Size; i++)
    newArray[i] = this->Array[i];

  this->Size = newSize;
  delete [] this->Array;
  this->Array = newArray;

  return this->Array;
}
