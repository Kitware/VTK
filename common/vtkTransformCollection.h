/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformCollection.h
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
// .NAME vtkTransformCollection - maintain a list of transforms
// .SECTION Description
// vtkTransformCollection is an object that creates and manipulates lists of
// objects of type vtkTransform.
// .SECTION see also
// vtkCollection vtkTransform

#ifndef __vtkTransformCollection_h
#define __vtkTransformCollection_h

#include "vtkCollection.h"
#include "vtkTransform.h"

class VTK_EXPORT vtkTransformCollection : public vtkCollection
{
public:
  char *GetClassName() {return "vtkTransformCollection";};
  static vtkTransformCollection *New() {return new vtkTransformCollection;};

  void AddItem(vtkTransform *);
  void RemoveItem(vtkTransform *);
  int IsItemPresent(vtkTransform *);
  vtkTransform *GetNextItem();
};

// Description:
// Add a Transform to the list.
inline void vtkTransformCollection::AddItem(vtkTransform *t) 
{
  this->vtkCollection::AddItem((vtkObject *)t);
}

// Description:
// Remove a Transform from the list.
inline void vtkTransformCollection::RemoveItem(vtkTransform *t) 
{
  this->vtkCollection::RemoveItem((vtkObject *)t);
}

// Description:
// Determine whether a particular Transform is present. Returns its position
// in the list.
inline int vtkTransformCollection::IsItemPresent(vtkTransform *t) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)t);
}

// Description:
// Get the next Transform in the list. Return NULL when the end of the
// list is reached.
inline vtkTransform *vtkTransformCollection::GetNextItem() 
{ 
  return (vtkTransform *)(this->GetNextItemAsObject());
}

#endif
