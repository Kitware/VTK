

#include "vtkCollection.h"
#include "vtkActor2D.h"
#include "vtkActor2DCollection.h"

vtkActor2DCollection::vtkActor2DCollection()
{

}

vtkActor2DCollection::~vtkActor2DCollection()
{
// Traverse the list 
// Unregister the actors (items)
// Delete the list elements

  vtkCollectionElement* indexElem;
  vtkCollectionElement* delElem;
  vtkActor2D* tempActor;

  delElem = this->Top;

  for (indexElem = this->Top->Next;
         indexElem != NULL;
           indexElem = indexElem->Next)
    {
     tempActor = (vtkActor2D*) delElem->Item;
     tempActor->UnRegister(this);
     delete delElem;
     delElem = indexElem;	
    }  

  // The last item on the list will fall through,
  // so delete it here
  tempActor = (vtkActor2D*) delElem->Item;
  tempActor->UnRegister(this);
  delete delElem;

}

// Description:
// Render the collection of 2D actors.
void vtkActor2DCollection::Render(vtkViewport* viewport)
{
  if (this->NumberOfItems != 0)
    {
    this->Sort();  
    vtkActor2D* tempActor;
    for ( this->InitTraversal(); 
           (tempActor = this->GetNextItem());)
      {
	    // Make sure that the actor is visible before rendering
	    if (tempActor->GetVisibility() == 1) tempActor->Render(viewport);
      }
    }
}

// Description:
// Add an actor to the list.
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

    vtkActor2D* tempActor = (vtkActor2D*) indexElem->Item;
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


void vtkActor2DCollection::Sort()
{
   vtkActor2D* tempActor;
   int index;
   
   vtkDebugMacro(<<"vtkActor2DCollection::Sort");

   int numElems  = this->GetNumberOfItems();

   // Create an array of pointers to actors
   vtkActor2D** actorPtrArr = new vtkActor2D* [numElems];

   vtkDebugMacro(<<"vtkActor2DCollection::Sort - Getting actors from collection");

   // Start at the beginning of the collection
   this->InitTraversal();

   // Fill the actor array with the items in the collection
   for (index = 0; index < numElems; index++)
     {
     actorPtrArr[index] = this->GetNextItem();
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

















