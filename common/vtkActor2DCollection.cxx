

#include "vtkCollection.h"
#include "vtkActor2D.h"
#include "vtkActor2DCollection.h"

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
  
    //#### Check on where this item is deleted
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
   
   // ### This needs to be checked !!!
   // this->DebugOn();

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
     // vtkDebugMacro(<<"vtkActor2DCollection::Sort - actorPtrArr["<<index<<"] layer: " <<
     //	              actorPtrArr[index]->GetLayerNumber());
     
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
  // keep the links the same, but swap around 
  // the items
 
  //vtkCollectionElement* elem = new vtkCollectionElement; 

  vtkCollectionElement* elem = this->Top;

  elem->Item = actorPtrArr[0];

  // vtkDebugMacro(<<"vtkActor2DCollection::Sort - Layer number: " << actorPtrArr[0]->GetLayerNumber());

  for (i = 1; i < numElems; i++)
    {
    elem = elem->Next;
    elem->Item = actorPtrArr[i];
	// vtkDebugMacro(<<"vtkActor2DCollection::Sort - Layer number: " << actorPtrArr[i]->GetLayerNumber());
    }

  // vtkDebugMacro(<<"vtkActor2DCollection::Sort - Deleting elem.");
  // delete elem;

  vtkDebugMacro(<<"vtkActor2DCollection::Sort - Deleting actorPtrArr.");
  delete[] actorPtrArr;

  vtkDebugMacro(<<"vtkActor2DCollection::Sort - Check list elements.");

  int incr = 0;

  for (this->InitTraversal();
         tempActor = this->GetNextItem();)
    {
			 
			vtkDebugMacro(<<"vtkActor2DCollection::Sort - Actor number: " << incr);
			incr++;
    }


  vtkDebugMacro(<<"vtkActor2DCollection::Sort - Exit.");
  


}




#if 0
  // Sort so that layer number increases as you go down 
  // the list

  // Check for NULL pointer passed to function
  if (a == NULL)
    {
    vtkErrorMacro (<< "vtkActor2DCollection::AddItem - Item to be
                                    added is NULL!");
    return;
    }


  // Check if the current item is NULL 
  if (this->Current == NULL)
    {
    //#### Check on where this item is deleted
    this->Current = new vtkCollectionElement;
    this->Current->Item = a;
    this->Current->Next = NULL;
    this->Bottom = this->Current;
    this->NumberOfItems++;
    return;
    }

  // this->Current should now be the place where we insert 
  // the new item.
  
  // Create a new collection element - check on deletion
  elem = new vtkCollectionElement;
  
  // Set that element's items to be the vtkActor2D object
  elem->Item = a;

  // Check if we're at the end of the list
  if (this->Current->Next != NULL)
    {
    // If we're not at the end of the list, put the next element 
    // after the element we're adding
    elem->Next = this->Current->Next;
    }
  else
    {
    // Otherwise make the element the end of the list
    elem->Next = NULL;
    this->Bottom = elem;
    }

  // Attach the new element to the list
  this->Current->Next = elem;
  
  // Increment the number of items
  this->NumberOfItems++;
  
#endif



    //while (this->Current->Next != NULL)
    // The Current layer isn't larger, so keep moving through the list
    // "while" this->Current = this->Current->Next;


#if 0
  // Need to comment this line out later
  this->vtkCollection::AddItem((vtkObject *)a);
#endif
