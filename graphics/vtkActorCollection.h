/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkActorCollection.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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

#include "vtkPropCollection.h"
class vtkActor;

class VTK_EXPORT vtkActorCollection : public vtkPropCollection
{
public:
  static vtkActorCollection *New();
  const char *GetClassName() {return "vtkActorCollection";};

  // Description:
  // Add an actor to the list.
  void AddItem(vtkActor *a);

  // Description:
  // Get the next actor in the list.
  vtkActor *GetNextActor();

  // Description:
  // Get the last actor in the list.
  vtkActor *GetLastActor();

  // Description:
  // Access routines that are provided for compatibility with previous
  // version of VTK.  Please use the GetNextActor(), GetLastActor() variants
  // where possible.
  vtkActor *GetNextItem();
  vtkActor *GetLastItem();

protected:
  vtkActorCollection() {};
  ~vtkActorCollection() {};
  vtkActorCollection(const vtkActorCollection&) {};
  void operator=(const vtkActorCollection&) {};
    

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };
  void AddItem(vtkProp *o) { this->vtkPropCollection::AddItem(o); };

};

inline void vtkActorCollection::AddItem(vtkActor *a) 
{
  this->vtkCollection::AddItem((vtkObject *)a);
}

inline vtkActor *vtkActorCollection::GetNextActor() 
{ 
  return (vtkActor *)(this->GetNextItemAsObject());
}

inline vtkActor *vtkActorCollection::GetLastActor() 
{ 
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return (vtkActor *)(this->Bottom->Item);
    }
}

inline vtkActor *vtkActorCollection::GetNextItem() 
{ 
  return this->GetNextActor();
}

inline vtkActor *vtkActorCollection::GetLastItem() 
{
  return this->GetLastActor();
}

#endif





