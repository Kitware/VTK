/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataCollection.h
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
// .NAME vtkPolyDataCollection - maintain a list of polygonal data objects
// .SECTION Description
// vtkPolyDataCollection is an object that creates and manipulates lists of
// datasets of type vtkPolyData. 
// .SECTION See Also
// vtkDataSetCollection vtkCollection

#ifndef __vtkPolyDataCollection_h
#define __vtkPolyDataCollection_h

#include "vtkCollection.h"
#include "vtkPolyData.h"

class VTK_EXPORT vtkPolyDataCollection : public vtkCollection
{
public:
  static vtkPolyDataCollection *New() {return new vtkPolyDataCollection;};
  const char *GetClassName() {return "vtkPolyDataCollection";};

  void AddItem(vtkPolyData *);
  void RemoveItem(vtkPolyData *);
  int IsItemPresent(vtkPolyData *);
  vtkPolyData *GetNextItem();
};

// Description:
// Add a poly data to the list.
inline void vtkPolyDataCollection::AddItem(vtkPolyData *pd) 
{
  this->vtkCollection::AddItem((vtkObject *)pd);
}

// Description:
// Remove an poly data from the list.
inline void vtkPolyDataCollection::RemoveItem(vtkPolyData *pd) 
{
  this->vtkCollection::RemoveItem((vtkObject *)pd);
}

// Description:
// Determine whether a particular poly data is present. Returns its position
// in the list.
inline int vtkPolyDataCollection::IsItemPresent(vtkPolyData *pd) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)pd);
}

// Description:
// Get the next poly data in the list.
inline vtkPolyData *vtkPolyDataCollection::GetNextItem() 
{ 
  return (vtkPolyData *)(this->GetNextItemAsObject());
}

#endif
