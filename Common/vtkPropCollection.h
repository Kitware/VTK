/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPropCollection.h
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
// .NAME vtkPropCollection - a list of Props
// .SECTION Description
// vtkPropCollection represents and provides methods to manipulate a list of
// Props (i.e., vtkProp and subclasses). The list is unsorted and duplicate
// entries are not prevented.

// .SECTION see also
// vtkProp vtkCollection 

#ifndef __vtkPropC_h
#define __vtkPropC_h

#include "vtkCollection.h"
#include "vtkProp.h"

class VTK_COMMON_EXPORT vtkPropCollection : public vtkCollection
{
 public:
  static vtkPropCollection *New();
  vtkTypeRevisionMacro(vtkPropCollection,vtkCollection);

  // Description:
  // Add an Prop to the list.
  void AddItem(vtkProp *a);

  // Description:
  // Get the next Prop in the list.
  vtkProp *GetNextProp();

  // Description:
  // Get the last Prop in the list.
  vtkProp *GetLastProp();
  
  // Description:
  // Get the number of paths contained in this list. (Recall that a
  // vtkProp can consist of multiple parts.) Used in picking and other
  // activities to get the parts of composite entities like vtkAssembly
  // or vtkPropAssembly.
  int GetNumberOfPaths();
  
protected:
  vtkPropCollection() {};
  ~vtkPropCollection() {};
  

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

private:
  vtkPropCollection(const vtkPropCollection&);  // Not implemented.
  void operator=(const vtkPropCollection&);  // Not implemented.
};

inline void vtkPropCollection::AddItem(vtkProp *a) 
{
  this->vtkCollection::AddItem((vtkObject *)a);
}

inline vtkProp *vtkPropCollection::GetNextProp() 
{ 
  return static_cast<vtkProp *>(this->GetNextItemAsObject());
}

inline vtkProp *vtkPropCollection::GetLastProp() 
{ 
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return static_cast<vtkProp *>(this->Bottom->Item);
    }
}

#endif





