/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Stack.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Stack.hh"

// Description:
// Construct with empty stack.
vtkStack::vtkStack()
{
  this->NumberOfItems = 0;
  this->Top = NULL;
  this->Bottom = NULL;
}

vtkStack::~vtkStack()
{
  vtkStackElement *p;  

  for ( p=this->Top; p != NULL; p = p->Next )
    {
    delete p;
    }
}

// Description:
// Add an object to the top of the stack. Does not prevent duplicate entries.
void vtkStack::Push(vtkObject *a)
{
  vtkStackElement *elem;

  elem = new vtkStackElement;
  
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
vtkObject *vtkStack::Pop()
{
  vtkObject *item;
  vtkStackElement *next;
  
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
vtkObject *vtkStack::GetTop()
{
  if ( this->Top != NULL )
    return this->Top->Item;
  else
    return NULL;
}

// Description:
// Return the number of objects in the stack.
int vtkStack::GetNumberOfItems()
{
  return this->NumberOfItems;
}

void vtkStack::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Number Of Items: " << this->NumberOfItems << "\n";
}








