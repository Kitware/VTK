/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Collect.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <stdlib.h>
#include <math.h>

#include "Collect.hh"

// Description:
// Construct with empty list.
vtkCollection::vtkCollection()
{
  this->NumberOfItems = 0;
  this->Top = NULL;
  this->Bottom = NULL;
  this->Current = NULL;
}

vtkCollection::~vtkCollection()
{
  this->RemoveAllItems();
}

// Description:
// Add an object to the list. Does not prevent duplicate entries.
void vtkCollection::AddItem(vtkObject *a)
{
  vtkCollectionElement *elem;

  elem = new vtkCollectionElement;
  
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

// Description:
// Remove an object from the list. Removes the first object found, not
// all occurences. If no object found, list is unaffected.
void vtkCollection::RemoveItem(vtkObject *a)
{
  int i;
  vtkCollectionElement *elem,*prev;
  
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

// Description:
// Remove all object from the list.
void vtkCollection::RemoveAllItems()
{
  vtkCollectionElement *p, *next;

  for ( next=p=this->Top; next != NULL; p=next)
    {
    next = p->Next;
    delete p;
    }

  this->NumberOfItems = 0;
  this->Top = this->Bottom = this->Current = NULL;
}

// Description:
// Search for an object and return location in list. If location == 0,
// object was not found.
int vtkCollection::IsItemPresent(vtkObject *a)
{
  int i;
  vtkCollectionElement *elem;
  
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


// Description:
// Return the number of objects in the list.
int vtkCollection::GetNumberOfItems()
{
  return this->NumberOfItems;
}


void vtkCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Number Of Items: " << this->NumberOfItems << "\n";
}
