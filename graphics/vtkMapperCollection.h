/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapperCollection.h
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
// .NAME vtkMapperCollection - a list of mappers
// .SECTION Description
// vtkMapperCollection represents and provides methods to manipulate a list of
// mappers (i.e., vtkMapper and subclasses). The list is unsorted and duplicate
// entries are not prevented.

// .SECTION see also
// vtkMapper vtkCollection 

#ifndef __vtkMapperC_h
#define __vtkMapperC_h

#include "vtkCollection.h"
class vtkMapper;

class VTK_EXPORT vtkMapperCollection : public vtkCollection
{
 public:
  static vtkMapperCollection *New() {return new vtkMapperCollection;};
  const char *GetClassName() {return "vtkMapperCollection";};

  void AddItem(vtkMapper *a);
  void RemoveItem(vtkMapper *a);
  int IsItemPresent(vtkMapper *a);
  vtkMapper *GetNextItem();
  vtkMapper *GetLastItem();
};

// Description:
// Add an mapper to the list.
inline void vtkMapperCollection::AddItem(vtkMapper *a) 
{
  this->vtkCollection::AddItem((vtkObject *)a);
}

// Description:
// Remove an mapper from the list.
inline void vtkMapperCollection::RemoveItem(vtkMapper *a) 
{
  this->vtkCollection::RemoveItem((vtkObject *)a);
}

// Description:
// Determine whether a particular mapper is present. Returns its position
// in the list.
inline int vtkMapperCollection::IsItemPresent(vtkMapper *a) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)a);
}

// Description:
// Get the next mapper in the list.
inline vtkMapper *vtkMapperCollection::GetNextItem() 
{ 
  return (vtkMapper *)(this->GetNextItemAsObject());
}

// Description:
// Get the last mapper in the list.
inline vtkMapper *vtkMapperCollection::GetLastItem() 
{ 
  if ( this->Bottom == NULL ) return NULL;
  else return (vtkMapper *)(this->Bottom->Item);
}

#endif





