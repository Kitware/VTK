
// .NAME vtkNewVolumeCollection - a list of new volumes
// .SECTION Description
// vtkNewVolumeCollection represents and provides methods to manipulate a 
// list of volumes (i.e., vtkVolume and subclasses). The list is unsorted 
// and duplicate entries are not prevented.

// .SECTION see also
// vtkCollection vtkVolume

#ifndef __vtkVolumeC_h
#define __vtkVolumeC_h

#include "vtkCollection.h"
#include "vtkVolume.h"

class vtkVolume;

class vtkVolumeCollection : public vtkCollection
{
 public:
  char *GetClassName() {return "vtkVolumeCollection";};

  void AddItem(vtkVolume *a);
  void RemoveItem(vtkVolume *a);
  int IsItemPresent(vtkVolume *a);
  vtkVolume *GetNextItem();
};

// Description:
// Add a volume to the list.
inline void vtkVolumeCollection::AddItem(vtkVolume *a) 
{
  this->vtkCollection::AddItem((vtkObject *)a);
}

// Description:
// Remove a volume from the list.
inline void vtkVolumeCollection::RemoveItem(vtkVolume *a) 
{
  this->vtkCollection::RemoveItem((vtkObject *)a);
}

// Description:
// Determine whether a particular volume is present. Returns its position
// in the list.
inline int vtkVolumeCollection::IsItemPresent(vtkVolume *a) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)a);
}

// Description:
// Get the next volume in the list. Return NULL when the end of the list
// is reached.
inline vtkVolume *vtkVolumeCollection::GetNextItem() 
{ 
  return (vtkVolume *)(this->GetNextItemAsObject());
}

#endif





