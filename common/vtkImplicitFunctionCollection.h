/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitFunctionCollection.h
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
// .NAME vtkImplicitFunctionCollection - maintain a list of implicit functions
// .SECTION Description
// vtkImplicitFunctionCollection is an object that creates and manipulates
// lists of objects of type vtkImplicitFunction. 
// .SECTION See Also
// vtkCollection

#ifndef __vtkImplicitFunctionCollection_h
#define __vtkImplicitFunctionCollection_h

#include "vtkCollection.h"
#include "vtkImplicitFunction.h"

class VTK_EXPORT vtkImplicitFunctionCollection : public vtkCollection
{
public:
  char *GetClassName() {return "vtkImplicitFunctionCollection";};
  static vtkImplicitFunctionCollection *New() 
    {return new vtkImplicitFunctionCollection;};

  void AddItem(vtkImplicitFunction *);
  void RemoveItem(vtkImplicitFunction *);
  int IsItemPresent(vtkImplicitFunction *);
  vtkImplicitFunction *GetNextItem();
};

// Description:
// Add an implicit function to the list.
inline void vtkImplicitFunctionCollection::AddItem(vtkImplicitFunction *f) 
{
  this->vtkCollection::AddItem((vtkObject *)f);
}

// Description:
// Remove an implicit function from the list.
inline void vtkImplicitFunctionCollection::RemoveItem(vtkImplicitFunction *f) 
{
  this->vtkCollection::RemoveItem((vtkObject *)f);
}

// Description:
// Determine whether a particular implicit function is present. Returns its position
// in the list.
inline int vtkImplicitFunctionCollection::IsItemPresent(vtkImplicitFunction *f) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)f);
}

// Description:
// Get the next implicit function in the list.
inline vtkImplicitFunction *vtkImplicitFunctionCollection::GetNextItem() 
{ 
  return (vtkImplicitFunction *)(this->GetNextItemAsObject());
}

#endif
