
#ifndef __vtkActor2DCollection_h
#define __vtkActor2DCollection_h

#include "vtkCollection.h"
#include "vtkActor2D.h"


class VTK_EXPORT vtkActor2DCollection : public vtkCollection
{
 public:
  static vtkActor2DCollection *New() {return new vtkActor2DCollection;};
  const char *GetClassName() {return "vtkActor2DCollection";};

  void Sort();
  void AddItem(vtkActor2D *a);
  void RemoveItem(vtkActor2D *a);
  int IsItemPresent(vtkActor2D *a);
  vtkActor2D *GetNextItem();
  vtkActor2D *GetLastItem();
};



// Description:
// Remove an actor from the list.
inline void vtkActor2DCollection::RemoveItem(vtkActor2D *a) 
{
  this->vtkCollection::RemoveItem((vtkObject *)a);
  a->UnRegister(this);
}

// Description:
// Determine whether a particular actor is present. Returns its position
// in the list.
inline int vtkActor2DCollection::IsItemPresent(vtkActor2D *a) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)a);
}

// Description:
// Get the next actor in the list.
inline vtkActor2D *vtkActor2DCollection::GetNextItem() 
{ 
  return (vtkActor2D *)(this->GetNextItemAsObject());
}

// Description:
// Get the last actor in the list.
inline vtkActor2D *vtkActor2DCollection::GetLastItem() 
{ 
  if ( this->Bottom == NULL ) return NULL;
  else return (vtkActor2D *)(this->Bottom->Item);
}

#endif





