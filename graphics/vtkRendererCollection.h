/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRendererCollection.h
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
// .NAME vtkRendererCollection - a list of renderers
// .SECTION Description
// vtkRendererCollection represents and provides methods to manipulate a list 
// of renderers (i.e., vtkRenderer and subclasses). The list is unsorted and 
// duplicate entries are not prevented.

// .SECTION see also
// vtkRenderer vtkCollection

#ifndef __vtkRendererCollection_hh
#define __vtkRendererCollection_hh

#include "vtkCollection.hh"
#include "vtkRenderer.hh"

class vtkRendererCollection : public vtkCollection
{
 public:
  char *GetClassName() {return "vtkRendererCollection";};

  void AddItem(vtkRenderer *a);
  void RemoveItem(vtkRenderer *a);
  int IsItemPresent(vtkRenderer *a);
  vtkRenderer *GetNextItem();
  void Render();
};

// Description:
// Add a renderer to the list.
inline void vtkRendererCollection::AddItem(vtkRenderer *a) 
{
  this->vtkCollection::AddItem((vtkObject *)a);
}

// Description:
// Remove a renderer from the list.
inline void vtkRendererCollection::RemoveItem(vtkRenderer *a) 
{
  this->vtkCollection::RemoveItem((vtkObject *)a);
}

// Description:
// Determine whether a particular renderer is present. Returns its position
// in the list.
inline int vtkRendererCollection::IsItemPresent(vtkRenderer *a) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)a);
}

// Description:
// Get the next renderer in the list. Return NULL when at the end of the list.
inline vtkRenderer *vtkRendererCollection::GetNextItem() 
{
  return (vtkRenderer *)(this->GetNextItemAsObject());
}

#endif
