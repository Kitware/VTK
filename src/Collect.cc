/*=========================================================================

  Program:   Visualization Library
  Module:    Collect.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <stdlib.h>
#include <iostream.h>
#include <math.h>

#include "Collect.hh"


vlCollection::vlCollection()
{
  this->NumberOfItems = 0;
  this->Top = NULL;
  this->Bottom = NULL;
}

void vlCollection::AddItem(vlObject *a)
{
  vlCollectionElement *elem;

  elem = new vlCollectionElement;
  
  if (!this->Top)
    {
    this->Top = elem;
    }
  else
    {
    this->Bottom->Next = elem;
    }
  this->Bottom = elem;

  elem->Item = a;
  elem->Next = NULL;

  this->NumberOfItems++;
}

void vlCollection::RemoveItem(vlObject *a)
{
  int i;
  vlCollectionElement *elem,*prev;
  
  if (!this->Top) return;

  elem = this->Top;
  prev = NULL;
  for (i = 0; i < this->NumberOfItems; i++)
    {
    if (elem->Item == a)
      {
      if (prev)
	{
	prev->Next = elem->Next;
	}
      else
	{
	this->Top = elem->Next;
	}
      if (!elem->Next)
	{
	this->Bottom = prev;
	}
      
      delete elem;
      this->NumberOfItems--;
      return;
      }
    else
      {
      prev = elem;
      elem = elem->Next;
      }
    }
}

int vlCollection::IsItemPresent(vlObject *a)
{
  int i;
  vlCollectionElement *elem;
  
  if (!this->Top) return 0;

  elem = this->Top;
  for (i = 0; i < this->NumberOfItems; i++)
    {
    if (elem->Item == a)
      {
      return i + 1;
      }
    else
      {
      elem = elem->Next;
      }
    }

  return 0;
}


int vlCollection::GetNumberOfItems()
{
  return this->NumberOfItems;
}

vlObject *vlCollection::GetItem(int num)
{
  int i;
  vlCollectionElement *elem;

  if ((num < 1) || (num > this->NumberOfItems))
    {
    vlErrorMacro(<< ": Requesting illegal index\n");
    return this->Top->Item;
    }

  elem = this->Top;
  for (i = 1; i < num; i++)
    {
    elem = elem->Next;
    }
  
  return (elem->Item);
}

void vlCollection::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlCollection::GetClassName()))
    {
    vlObject::PrintSelf(os,indent);
    
    os << indent << "Number Of Items: " << this->NumberOfItems << "\n";
    }
}








