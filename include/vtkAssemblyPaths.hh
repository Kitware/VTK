/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssemblyPaths.hh
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
// .NAME vtkAssemblyPaths - a list of lists of actors representing an assembly hierarchy
// .SECTION Description
// vtkAssemblyPaths represents a hierarchy of assemblies as a sequence of
// paths. Each path is a list of actors, starting from the root of the
// assembly down to the leaf actors. Methods are also provided to manipulate
// the path including propagating transformation matrices and actor properties.

// .SECTION see also
// vtkAssembly vtkActor vtkAssemblyCollection 

#ifndef __vtkAssemblyPaths_hh
#define __vtkAssemblyPaths_hh

#include "vtkActorCollection.hh"
class vtkActor;

class vtkAssemblyPaths : public vtkCollection
{
 public:
  char *GetClassName() {return "vtkAssemblyPaths";};

  void AddItem(vtkActorCollection *a);
  void RemoveItem(vtkActorCollection *a);
  int IsItemPresent(vtkActorCollection *a);
  vtkActorCollection *GetNextItem();
};

// Description:
// Add a path to the list.
inline void vtkAssemblyPaths::AddItem(vtkActorCollection *a) 
{
  this->vtkCollection::AddItem((vtkObject *)a);
}

// Description:
// Remove a path from the list.
inline void vtkAssemblyPaths::RemoveItem(vtkActorCollection *a) 
{
  this->vtkCollection::RemoveItem((vtkObject *)a);
}

// Description:
// Determine whether a particular path is present. Returns its position
// in the list.
inline int vtkAssemblyPaths::IsItemPresent(vtkActorCollection *a) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)a);
}

// Description:
// Get the next path in the list.
inline vtkActorCollection *vtkAssemblyPaths::GetNextItem() 
{ 
  return (vtkActorCollection *)(this->GetNextItemAsObject());
}

#endif
