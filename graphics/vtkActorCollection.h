/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkActorCollection.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkActorCollection - a list of actors
// .SECTION Description
// vtkActorCollection represents and provides methods to manipulate a list of
// actors (i.e., vtkActor and subclasses). The list is unsorted and duplicate
// entries are not prevented.

// .SECTION see also
// vtkActor vtkCollection 

#ifndef __vtkActorC_h
#define __vtkActorC_h

#include "vtkCollection.h"
class vtkActor;

class VTK_EXPORT vtkActorCollection : public vtkCollection
{
 public:
  vtkActorCollection *New() {return new vtkActorCollection;};
  char *GetClassName() {return "vtkActorCollection";};

  void AddItem(vtkActor *a);
  void RemoveItem(vtkActor *a);
  int IsItemPresent(vtkActor *a);
  vtkActor *GetNextItem();
  vtkActor *GetLastItem();
};

// Description:
// Add an actor to the list.
inline void vtkActorCollection::AddItem(vtkActor *a) 
{
  this->vtkCollection::AddItem((vtkObject *)a);
}

// Description:
// Remove an actor from the list.
inline void vtkActorCollection::RemoveItem(vtkActor *a) 
{
  this->vtkCollection::RemoveItem((vtkObject *)a);
}

// Description:
// Determine whether a particular actor is present. Returns its position
// in the list.
inline int vtkActorCollection::IsItemPresent(vtkActor *a) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)a);
}

// Description:
// Get the next actor in the list.
inline vtkActor *vtkActorCollection::GetNextItem() 
{ 
  return (vtkActor *)(this->GetNextItemAsObject());
}

// Description:
// Get the last actor in the list.
inline vtkActor *vtkActorCollection::GetLastItem() 
{ 
  if ( this->Bottom == NULL ) return NULL;
  else return (vtkActor *)(this->Bottom->Item);
}

#endif





