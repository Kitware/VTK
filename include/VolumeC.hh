/*=========================================================================

  Program:   Visualization Toolkit
  Module:    VolumeC.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkVolumeCollection - a list of Volumes
// .SECTION Description
// vtkVolumeCollection represents and provides methods to manipulate list of
// Volumes (i.e., vtkVolume and subclasses). The list is unsorted and duplicate
// entries are not prevented.

#ifndef __vtkVolumeC_hh
#define __vtkVolumeC_hh

#include "Collect.hh"
#include "Volume.hh"

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
// Add an Volume to the list.
inline void vtkVolumeCollection::AddItem(vtkVolume *a) 
{
  this->vtkCollection::AddItem((vtkObject *)a);
}

// Description:
// Remove an Volume from the list.
inline void vtkVolumeCollection::RemoveItem(vtkVolume *a) 
{
  this->vtkCollection::RemoveItem((vtkObject *)a);
}

// Description:
// Determine whether a particular Volume is present. Returns its position
// in the list.
inline int vtkVolumeCollection::IsItemPresent(vtkVolume *a) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)a);
}

// Description:
// Get the next Volume in the list.
inline vtkVolume *vtkVolumeCollection::GetNextItem() 
{ 
  return (vtkVolume *)(this->vtkCollection::GetNextItem());
}

#endif





