/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagerCollection.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Matt Turek who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkImagerCollection - a collection of imagers
// .SECTION Description
// Simple collection class for vtkImagers

// .SECTION See Also
// vtkCollection vtkImager

#ifndef __vtkImagerCollection_h
#define __vtkImagerCollection_h

#include "vtkCollection.h"
#include "vtkImager.h"


class VTK_EXPORT vtkImagerCollection : public vtkCollection
{
 public:
  static vtkImagerCollection *New() {return new vtkImagerCollection;};
  const char *GetClassName() {return "vtkImagerCollection";};

  // Description:
  // Standard methods for manipulating the collection.
  void AddItem(vtkImager *a);
  void RemoveItem(vtkImager *a);
  int IsItemPresent(vtkImager *a);
  vtkImager *GetNextItem();
  vtkImager *GetLastItem();
};

inline void vtkImagerCollection::AddItem(vtkImager *a) 
{
  this->vtkCollection::AddItem((vtkObject *)a);
  // Needed for convenience functions
  // a->Register(this);
}

inline void vtkImagerCollection::RemoveItem(vtkImager *a) 
{
  this->vtkCollection::RemoveItem((vtkObject *)a);
  // Needed for convenience functions
  // a->UnRegister(this);
}

inline int vtkImagerCollection::IsItemPresent(vtkImager *a) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)a);
}

inline vtkImager *vtkImagerCollection::GetNextItem() 
{ 
  return (vtkImager *)(this->GetNextItemAsObject());
}

inline vtkImager *vtkImagerCollection::GetLastItem() 
{ 
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return (vtkImager *)(this->Bottom->Item);
    }
}

#endif





