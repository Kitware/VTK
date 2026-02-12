// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor2DCollection.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkActor2DCollection);

void vtkActor2DCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// Destructor for the vtkActor2DCollection class. This removes all
// objects from the collection.
vtkActor2DCollection::~vtkActor2DCollection()
{
  this->RemoveAllItems();
}

// Sort and then render the collection of 2D actors.
void vtkActor2DCollection::RenderOverlay(vtkViewport* viewport)
{
  if (this->GetNumberOfItems() != 0)
  {
    this->Sort();
    vtkActor2D* tempActor;
    vtkCollectionSimpleIterator adit;
    for (this->InitTraversal(adit); (tempActor = this->GetNextActor2D(adit));)
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
void vtkActor2DCollection::AddItem(vtkActor2D* a)
{
  // Perform binary search to find 1st vtkActor2D with >= layer number
  auto it = std::lower_bound(this->begin(), this->end(), a,
    [](vtkObject* l, vtkObject* r)
    {
      vtkActor2D* actorL = static_cast<vtkActor2D*>(l);
      vtkActor2D* actorR = static_cast<vtkActor2D*>(r);
      return actorL->GetLayerNumber() < actorR->GetLayerNumber();
    });

  // Found, insert before that vtkActor2D
  if (it < this->end())
  {
    this->InsertItem(it - this->begin(), a); // this already registers a
  }
  else
  {
    // End of list found before a larger layer number
    this->vtkCollection::AddItem(a); // this already registers a
  }
}

// Sorts the vtkActor2DCollection by layer number.  Smaller layer
// numbers are first.  Layer numbers can be any integer value.
void vtkActor2DCollection::Sort()
{
  vtkCollection::Sort(
    [](vtkObject* a, vtkObject* b)
    {
      vtkActor2D* actorA = static_cast<vtkActor2D*>(a);
      vtkActor2D* actorB = static_cast<vtkActor2D*>(b);
      return actorA->GetLayerNumber() < actorB->GetLayerNumber();
    });
}
VTK_ABI_NAMESPACE_END
