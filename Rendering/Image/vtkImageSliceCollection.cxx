// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkImageSliceCollection.h"
#include "vtkImageProperty.h"
#include "vtkImageSlice.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImageSliceCollection);

//------------------------------------------------------------------------------
void vtkImageSliceCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
// Destructor for the vtkImageSliceCollection class. This removes all
// objects from the collection.
vtkImageSliceCollection::~vtkImageSliceCollection()
{
  this->RemoveAllItems();
}

//------------------------------------------------------------------------------
// Add an image to the list.  The new image is inserted in the
// list according to it's layer number.
void vtkImageSliceCollection::AddItem(vtkImageSlice* a)
{
  // Perform binary search to find 1st vtkActor2D with >= layer number
  auto it = std::lower_bound(this->begin(), this->end(), a,
    [](vtkObject* l, vtkObject* r)
    {
      vtkImageSlice* actorL = static_cast<vtkImageSlice*>(l);
      vtkImageSlice* actorR = static_cast<vtkImageSlice*>(r);
      return actorL->GetProperty()->GetLayerNumber() < actorR->GetProperty()->GetLayerNumber();
    });

  // Found, insert before that vtkImageSlice
  if (it < this->end())
  {
    this->InsertItem(it - this->begin(), a); // this already registers a
  }
  else
  {
    this->vtkCollection::AddItem(a); // this already registers a
  }
}

//------------------------------------------------------------------------------
// small helper struct
class vtkImageSliceLayerPair
{
public:
  vtkImageSlice* image;
  int layer;
};

//------------------------------------------------------------------------------
// Sorts the vtkImageSliceCollection by layer number.  Smaller layer
// numbers are first.  Layer numbers can be any integer value.
void vtkImageSliceCollection::Sort()
{
  vtkCollection::Sort(
    [](vtkObject* a, vtkObject* b)
    {
      vtkImageSlice* sliceA = static_cast<vtkImageSlice*>(a);
      vtkImageSlice* sliceB = static_cast<vtkImageSlice*>(b);
      return sliceA->GetProperty()->GetLayerNumber() < sliceB->GetProperty()->GetLayerNumber();
    });
}
VTK_ABI_NAMESPACE_END
