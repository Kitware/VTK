
#ifndef __vtkImagerCollection_h
#define __vtkImagerCollection_h

#include "vtkCollection.h"
#include "vtkImager.h"


class VTK_EXPORT vtkImagerCollection : public vtkCollection
{
 public:
  static vtkImagerCollection *New() {return new vtkImagerCollection;};
  const char *GetClassName() {return "vtkImagerCollection";};

  void AddItem(vtkImager *a);
  void RemoveItem(vtkImager *a);
  int IsItemPresent(vtkImager *a);
  vtkImager *GetNextItem();
  vtkImager *GetLastItem();
};

// Description:
// Add an actor to the list.
inline void vtkImagerCollection::AddItem(vtkImager *a) 
{
  this->vtkCollection::AddItem((vtkObject *)a);
  // Needed for convenience functions
  // a->Register(this);
}

// Description:
// Remove an actor from the list.
inline void vtkImagerCollection::RemoveItem(vtkImager *a) 
{
  this->vtkCollection::RemoveItem((vtkObject *)a);
  // Needed for convenience functions
  // a->UnRegister(this);
}

// Description:
// Determine whether a particular actor is present. Returns its position
// in the list.
inline int vtkImagerCollection::IsItemPresent(vtkImager *a) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)a);
}

// Description:
// Get the next actor in the list.
inline vtkImager *vtkImagerCollection::GetNextItem() 
{ 
  return (vtkImager *)(this->GetNextItemAsObject());
}

// Description:
// Get the last actor in the list.
inline vtkImager *vtkImagerCollection::GetLastItem() 
{ 
  if ( this->Bottom == NULL ) return NULL;
  else return (vtkImager *)(this->Bottom->Item);
}

#endif





