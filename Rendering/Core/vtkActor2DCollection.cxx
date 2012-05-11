/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkActor2DCollection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor2DCollection.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkActor2DCollection);

// protected function to delete an element. Internal use only.
void vtkActor2DCollection::DeleteElement(vtkCollectionElement *e)
{
  vtkCollection::DeleteElement(e);
}

// Desctructor for the vtkActor2DCollection class. This removes all
// objects from the collection.
vtkActor2DCollection::~vtkActor2DCollection()
{
  this->RemoveAllItems();
}

// Sort and then render the collection of 2D actors.
void vtkActor2DCollection::RenderOverlay(vtkViewport* viewport)
{
  if (this->NumberOfItems != 0)
    {
    this->Sort();
    vtkActor2D* tempActor;
    vtkCollectionSimpleIterator adit;
    for ( this->InitTraversal(adit);
          (tempActor = this->GetNextActor2D(adit));)
      {
      // Make sure that the actor is visible before rendering
      if (tempActor->GetVisibility() == 1)
        {
        tempActor->RenderOverlay(viewport);
        }
      }
    }
}

// Add an actor to the list.  The new actor is
// inserted in the list according to it's layer
// number.
void vtkActor2DCollection::AddItem(vtkActor2D *a)
{
  vtkCollectionElement* indexElem;
  vtkCollectionElement* elem = new vtkCollectionElement;

  // Check if the top item is NULL
  if (this->Top == NULL)
    {
    vtkDebugMacro(<<"vtkActor2DCollection::AddItem - Adding item to top of the list");

    this->Top = elem;
    elem->Item = a;
    elem->Next = NULL;
    this->Bottom = elem;
    this->NumberOfItems++;
    a->Register(this);
    return;
    }

  for (indexElem = this->Top;
         indexElem != NULL;
           indexElem = indexElem->Next)
    {

    vtkActor2D* tempActor = static_cast<vtkActor2D*>(indexElem->Item);
    if (a->GetLayerNumber() < tempActor->GetLayerNumber())
      {
      // The indexElem item's layer number is larger, so swap
      // the new item and the indexElem item.
      vtkDebugMacro(<<"vtkActor2DCollection::AddItem - Inserting item");
      elem->Item = indexElem->Item;
      elem->Next = indexElem->Next;
      indexElem->Item = a;
      indexElem->Next = elem;
      this->NumberOfItems++;
      a->Register(this);
      return;
      }

    }

  //End of list found before a larger layer number
  vtkDebugMacro(<<"vtkActor2DCollection::AddItem - Adding item to end of the list");
  elem->Item = a;
  elem->Next = NULL;
  this->Bottom->Next = elem;
  this->Bottom = elem;
  this->NumberOfItems++;
  a->Register(this);

}

// Sorts the vtkActor2DCollection by layer number.  Smaller layer
// numbers are first.  Layer numbers can be any integer value.
void vtkActor2DCollection::Sort()
{
   int index;

   vtkDebugMacro(<<"vtkActor2DCollection::Sort");

   int numElems  = this->GetNumberOfItems();

   // Create an array of pointers to actors
   vtkActor2D** actorPtrArr = new vtkActor2D* [numElems];

   vtkDebugMacro(<<"vtkActor2DCollection::Sort - Getting actors from collection");

   // Start at the beginning of the collection
   vtkCollectionSimpleIterator ait;
   this->InitTraversal(ait);


   // Fill the actor array with the items in the collection
   for (index = 0; index < numElems; index++)
     {
     actorPtrArr[index] = this->GetNextActor2D(ait);
     }

  vtkDebugMacro(<<"vtkActor2DCollection::Sort - Starting selection sort");
   // Start the sorting - selection sort
  int i, j, min;
  vtkActor2D* t;

  for (i = 0; i < numElems - 1; i++)
    {
    min = i;
    for (j = i + 1; j < numElems ; j++)
      {
      if(actorPtrArr[j]->GetLayerNumber() < actorPtrArr[min]->GetLayerNumber())
        {
        min = j;
        }
      }
    t = actorPtrArr[min];
    actorPtrArr[min] = actorPtrArr[i];
    actorPtrArr[i] = t;
    }

   vtkDebugMacro(<<"vtkActor2DCollection::Sort - Selection sort done.");

   for (index = 0; index < numElems; index++)
     {
     vtkDebugMacro(<<"vtkActor2DCollection::Sort - actorPtrArr["<<index<<"] layer: " <<
     actorPtrArr[index]->GetLayerNumber());
     }

  vtkDebugMacro(<<"vtkActor2DCollection::Sort - Rearraging the linked list.");
  // Now move the items around in the linked list -
  // keep the links the same, but swap around the items

  vtkCollectionElement* elem = this->Top;
  elem->Item = actorPtrArr[0];

  for (i = 1; i < numElems; i++)
    {
    elem = elem->Next;
    elem->Item = actorPtrArr[i];
    }

  delete[] actorPtrArr;
}

















