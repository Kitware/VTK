/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCollection.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include <stdlib.h>
#include <math.h>

#include "vtkCollection.hh"

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
// all occurrences. If no object found, list is unaffected.
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
// Remove all objects from the list.
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
