/*=========================================================================

  Program:   Visualization Library
  Module:    VolumeC.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlVolumeCollection - a list of Volumes
// .SECTION Description
// vlVolumeCollection represents and provides methods to manipulate list of
// Volumes (i.e., vlVolume and subclasses). The list is unsorted and duplicate
// entries are not prevented.

#ifndef __vlVolumeC_hh
#define __vlVolumeC_hh

#include "Collect.hh"
#include "Volume.hh"

class vlVolumeCollection : public vlCollection
{
 public:
  char *GetClassName() {return "vlVolumeCollection";};

  void AddItem(vlVolume *a);
  void RemoveItem(vlVolume *a);
  int IsItemPresent(vlVolume *a);
  vlVolume *GetNextItem();
};

// Description:
// Add an Volume to the list.
inline void vlVolumeCollection::AddItem(vlVolume *a) 
{
  this->vlCollection::AddItem((vlObject *)a);
}

// Description:
// Remove an Volume from the list.
inline void vlVolumeCollection::RemoveItem(vlVolume *a) 
{
  this->vlCollection::RemoveItem((vlObject *)a);
}

// Description:
// Determine whether a particular Volume is present. Returns its position
// in the list.
inline int vlVolumeCollection::IsItemPresent(vlVolume *a) 
{
  return this->vlCollection::IsItemPresent((vlObject *)a);
}

// Description:
// Get the next Volume in the list.
inline vlVolume *vlVolumeCollection::GetNextItem() 
{ 
  return (vlVolume *)(this->vlCollection::GetNextItem());
}

#endif





