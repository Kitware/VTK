/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSliceCollection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkImageSliceCollection.h"
#include "vtkObjectFactory.h"
#include "vtkImageSlice.h"
#include "vtkImageProperty.h"

vtkStandardNewMacro(vtkImageSliceCollection);

//----------------------------------------------------------------------------
// protected function to delete an element. Internal use only.
void vtkImageSliceCollection::DeleteElement(vtkCollectionElement *e)
{
  vtkCollection::DeleteElement(e);
}

//----------------------------------------------------------------------------
// Desctructor for the vtkImageSliceCollection class. This removes all
// objects from the collection.
vtkImageSliceCollection::~vtkImageSliceCollection()
{
  this->RemoveAllItems();
}

//----------------------------------------------------------------------------
// Add an image to the list.  The new image is inserted in the
// list according to it's layer number.
void vtkImageSliceCollection::AddItem(vtkImageSlice *a)
{
  vtkCollectionElement* elem = new vtkCollectionElement;
  elem->Item = a;

  // Find insertion location according to the layer number
  vtkCollectionElement *prevElem = 0;
  int layerNumber = a->GetProperty()->GetLayerNumber();
  for (vtkCollectionElement *indexElem = this->Top;
       indexElem != 0;
       indexElem = indexElem->Next)
  {
    vtkImageSlice* tempImage = static_cast<vtkImageSlice*>(indexElem->Item);
    if (layerNumber < tempImage->GetProperty()->GetLayerNumber())
    {
      break;
    }
    prevElem = indexElem;
  }

  // Insert the new element into the linked list
  if (prevElem == 0)
  {
    elem->Next = this->Top;
    this->Top = elem;
  }
  else
  {
    elem->Next = prevElem->Next;
    prevElem->Next = elem;
  }

  // Check if this is the new bottom
  if (elem->Next == 0)
  {
    this->Bottom = elem;
  }

  this->NumberOfItems++;
  a->Register(this);
}

//----------------------------------------------------------------------------
// small helper struct
class vtkImageSliceLayerPair
{
public:
  vtkImageSlice *image;
  int layer;
};

//----------------------------------------------------------------------------
// Sorts the vtkImageSliceCollection by layer number.  Smaller layer
// numbers are first.  Layer numbers can be any integer value.
void vtkImageSliceCollection::Sort()
{
  // Create a temporary array of pointers to images
  int numElems = this->GetNumberOfItems();
  vtkImageSliceLayerPair defaultLayerArray[8];
  vtkImageSliceLayerPair *layerArray = defaultLayerArray;
  if (numElems > 8)
  {
    layerArray = new vtkImageSliceLayerPair [numElems];
  }

  // Start at the beginning of the collection
  vtkCollectionSimpleIterator ait;
  this->InitTraversal(ait);

  // Fill the image array with the items in the collection
  for (int ii = 0; ii < numElems; ii++)
  {
    vtkImageSlice *image = this->GetNextImage(ait);
    layerArray[ii].image = image;
    layerArray[ii].layer = image->GetProperty()->GetLayerNumber();
  }

  // The collection will be small (often with n=2) so do a brute-force
  // selection sort, which also keeps items with the same layer number
  // in the same order as before the sort.
  for (int i = 0; i < numElems - 1; i++)
  {
    int imin = i;
    int lmin = layerArray[imin].layer;
    int j = i + 1;

    do
    {
      int l = layerArray[j].layer;
      if (l < lmin)
      {
        imin = j;
        lmin = l;
      }
    }
    while (++j < numElems);

    vtkImageSliceLayerPair t = layerArray[imin];
    layerArray[imin] = layerArray[i];
    layerArray[i] = t;
  }

  // Now move the items around in the linked list -
  // keep the links the same, but swap around the items
  vtkCollectionElement* elem = this->Top;
  for (int jj = 0; jj < numElems; jj++)
  {
    elem->Item = layerArray[jj].image;
    elem = elem->Next;
  }

  if (layerArray != defaultLayerArray)
  {
    delete [] layerArray;
  }
}
