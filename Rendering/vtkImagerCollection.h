/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagerCollection.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImagerCollection - a collection of imagers
// .SECTION Description
// Simple collection class for vtkImagers

// .SECTION See Also
// vtkCollection vtkImager

#ifndef __vtkImagerCollection_h
#define __vtkImagerCollection_h

#include "vtkCollection.h"
#include "vtkImager.h"


class VTK_RENDERING_EXPORT vtkImagerCollection : public vtkCollection
{
 public:
  static vtkImagerCollection *New();
  vtkTypeRevisionMacro(vtkImagerCollection,vtkCollection);

  // Description:
  // Standard methods for manipulating the collection.
  void AddItem(vtkImager *a);
  vtkImager *GetNextItem();
  vtkImager *GetLastItem();
  
protected:  
  vtkImagerCollection() {};
  ~vtkImagerCollection() {};

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

private:
  vtkImagerCollection(const vtkImagerCollection&);  // Not implemented.
  void operator=(const vtkImagerCollection&);  // Not implemented.
};

inline void vtkImagerCollection::AddItem(vtkImager *a) 
{
  this->vtkCollection::AddItem((vtkObject *)a);
}

inline vtkImager *vtkImagerCollection::GetNextItem() 
{ 
  return static_cast<vtkImager *>(this->GetNextItemAsObject());
}

inline vtkImager *vtkImagerCollection::GetLastItem() 
{ 
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return static_cast<vtkImager *>(this->Bottom->Item);
    }
}

#endif





