/*=========================================================================

  Program:   Visualization Library
  Module:    Stack.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Stack.hh"

// Description:
// Construct with empty stack.
vlStack::vlStack()
{
  this->NumberOfItems = 0;
  this->Top = NULL;
  this->Bottom = NULL;
}

vlStack::~vlStack()
{
  vlStackElement *p;  

  for ( p=this->Top; p != NULL; p = p->Next )
    {
    delete p;
    }
}

// Description:
// Add an object to the top of the stack. Does not prevent duplicate entries.
void vlStack::Push(vlObject *a)
{
  vlStackElement *elem;

  elem = new vlStackElement;
  
  if ( this->Top == NULL )
    this->Bottom = elem;
  else
    elem->Next = this->Top;

  this->Top = elem;
  elem->Item = a;
  this->NumberOfItems++;
}

// Description:
// Remove an object from the top of the list.
vlObject *vlStack::Pop()
{
  vlObject *item;
  vlStackElement *next;
  
  if ( this->Top == NULL ) return NULL;

  item = this->Top->Item;
  next = this->Top->Next;
  delete this->Top;

  if ( this->Top == this->Bottom )
    this->Top = this->Bottom = NULL;
  else
    this->Top = next;

  this->NumberOfItems--;
  return item;
}

// Description:
// Return the number of objects in the stack.
vlObject *vlStack::GetTop()
{
  if ( this->Top != NULL )
    return this->Top->Item;
  else
    return NULL;
}

// Description:
// Return the number of objects in the stack.
int vlStack::GetNumberOfItems()
{
  return this->NumberOfItems;
}

void vlStack::PrintSelf(ostream& os, vlIndent indent)
{
  vlObject::PrintSelf(os,indent);

  os << indent << "Number Of Items: " << this->NumberOfItems << "\n";
}








