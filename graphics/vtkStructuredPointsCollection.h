/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsCollection.h
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
// .NAME vtkStructuredPointsCollection - maintain a list of structured points data objects
// .SECTION Description
// vtkStructuredPointsCollection is an object that creates and manipulates lists of
// structured points datasets. See also vtkCollection and subclasses.

#ifndef __vtkStructuredPointsCollection_h
#define __vtkStructuredPointsCollection_h

#include "vtkCollection.h"
#include "vtkStructuredPoints.h"

class vtkStructuredPointsCollection : public vtkCollection
{
public:
  char *GetClassName() {return "vtkStructuredPointsCollection";};

  void AddItem(vtkStructuredPoints *);
  void RemoveItem(vtkStructuredPoints *);
  int IsItemPresent(vtkStructuredPoints *);
  vtkStructuredPoints *GetNextItem();
};

// Description:
// Add a pointer to a vtkStructuredPoints to the list.
inline void vtkStructuredPointsCollection::AddItem(vtkStructuredPoints *ds) 
{
  this->vtkCollection::AddItem((vtkObject *)ds);
}

// Description:
// Remove a pointer to a vtkStructuredPoints from the list.
inline void vtkStructuredPointsCollection::RemoveItem(vtkStructuredPoints *ds) 
{
  this->vtkCollection::RemoveItem((vtkObject *)ds);
}

// Description:
// Determine whether a particular pointer to a vtk StructuredPoints is in
// the list.
// Returns its position in the list.
inline int vtkStructuredPointsCollection::IsItemPresent(vtkStructuredPoints *ds) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)ds);
}


// Description:
// Get the next item in the collection. NULL is returned if the collection
// is exhausted.
inline vtkStructuredPoints *vtkStructuredPointsCollection::GetNextItem()
{
 return (vtkStructuredPoints *)(this->GetNextItemAsObject());
}

#endif
