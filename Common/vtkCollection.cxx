/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCollection.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <stdlib.h>
#include <math.h>

#include "vtkCollection.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkCollection* vtkCollection::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkCollection");
  if(ret)
    {
    return (vtkCollection*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkCollection;
}




// Construct with empty list.
vtkCollection::vtkCollection()
{
  this->NumberOfItems = 0;
  this->Top = NULL;
  this->Bottom = NULL;
  this->Current = NULL;
}

// Desctructor for the vtkCollection class. This removes all 
// objects from the collection.
vtkCollection::~vtkCollection()
{
  this->RemoveAllItems();
}

// protected function to delete an element. Internal use only.
void vtkCollection::DeleteElement(vtkCollectionElement *e)
{
  if (e->Item != NULL)
    {
    e->Item->UnRegister(this); 
    }
  delete e;
}

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

  a->Register(this);
  elem->Item = a;
  elem->Next = NULL;

  this->Modified();

  this->NumberOfItems++;
}

// Remove an object from the list. Removes the first object found, not
// all occurrences. If no object found, list is unaffected.  See warning
// in description of RemoveItem(int).
void vtkCollection::RemoveItem(vtkObject *a)
{
  int i;
  vtkCollectionElement *elem;
  
  if (!this->Top)
    {
    return;
    }

  elem = this->Top;
  for (i = 0; i < this->NumberOfItems; i++)
    {
    if (elem->Item == a)
      {
      this->RemoveItem(i);
      this->Modified();
      return;
      }
    else
      {
      elem = elem->Next;
      }
    }
}

// Remove all objects from the list.
void vtkCollection::RemoveAllItems()
{
  int i;

  for (i = this->NumberOfItems - 1; i >= 0; i--)
    {
    this->RemoveItem(i);
    }
}

// Search for an object and return location in list. If location == 0,
// object was not found.
int vtkCollection::IsItemPresent(vtkObject *a)
{
  int i;
  vtkCollectionElement *elem;
  
  if (!this->Top)
    {
    return 0;
    }

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


// Get the i'th item in the collection. NULL is returned if i is out
// of range
vtkObject *vtkCollection::GetItemAsObject(int i)
{
  vtkCollectionElement *elem=this->Top;

  if (i < 0)
    {
    return NULL;
    }
  
  while (elem != NULL && i > 0)
    {
    elem = elem->Next;
    i--;
    }
  
  if ( elem != NULL )
    {
    return elem->Item;
    }
  else
    {
    return NULL;
    }
}


// Replace the i'th item in the collection with a
void vtkCollection::ReplaceItem(int i, vtkObject *a)
{
  vtkCollectionElement *elem;

  if( i < 0 || i >= this->NumberOfItems )
    {
    return;
    }
  
  elem = this->Top;
  for (int j = 0; j < i; j++, elem = elem->Next ) 
    {}

  // Take care of reference counting
  if (elem->Item != NULL)
    {
    elem->Item->UnRegister(this);
    }
  a->Register(this);
  
  // j == i
  elem->Item = a;

  this->Modified();
}


// Remove the i'th item in the list.
// Be careful if using this function during traversal of the list using 
// GetNextItemAsObject (or GetNextItem in derived class).  The list WILL
// be shortened if a valid index is given!  If this->Current is equal to the
// element being removed, have it point to then next element in the list.
void vtkCollection::RemoveItem(int i)
{
  vtkCollectionElement *elem,*prev;

  if( i < 0 || i >= this->NumberOfItems )
    {
    return;
    }
  
  this->Modified();

  elem = this->Top;
  prev = NULL;
  for (int j = 0; j < i; j++)
    {
    prev = elem;
    elem = elem->Next;
    }  

  // j == i
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
      
  if ( this->Current == elem )
    {
    this->Current = elem->Next;
    }

  this->DeleteElement(elem);
  this->NumberOfItems--;
}






