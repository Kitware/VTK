/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPropCollection.h
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
// .NAME vtkPropCollection - a list of Props
// .SECTION Description
// vtkPropCollection represents and provides methods to manipulate a list of
// Props (i.e., vtkProp and subclasses). The list is unsorted and duplicate
// entries are not prevented.

// .SECTION see also
// vtkProp vtkCollection 

#ifndef __vtkPropC_h
#define __vtkPropC_h

#include "vtkCollection.h"
class vtkProp;

class VTK_EXPORT vtkPropCollection : public vtkCollection
{
 public:
  static vtkPropCollection *New() {return new vtkPropCollection;};
  const char *GetClassName() {return "vtkPropCollection";};

  // Description:
  // Add an Prop to the list.
  void AddItem(vtkProp *a);

  // Description:
  // Remove an Prop from the list.
  void RemoveItem(vtkProp *a);

  // Description:
  // Determine whether a particular Prop is present. Returns its position
  // in the list.
  int IsItemPresent(vtkProp *a);

  // Description:
  // Get the next Prop in the list.
  vtkProp *GetNextItem();

  // Description:
  // Get the last Prop in the list.
  vtkProp *GetLastItem();
};

inline void vtkPropCollection::AddItem(vtkProp *a) 
{
  this->vtkCollection::AddItem((vtkObject *)a);
}

inline void vtkPropCollection::RemoveItem(vtkProp *a) 
{
  this->vtkCollection::RemoveItem((vtkObject *)a);
}

inline int vtkPropCollection::IsItemPresent(vtkProp *a) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)a);
}

inline vtkProp *vtkPropCollection::GetNextItem() 
{ 
  return (vtkProp *)(this->GetNextItemAsObject());
}

inline vtkProp *vtkPropCollection::GetLastItem() 
{ 
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return (vtkProp *)(this->Bottom->Item);
    }
}

#endif





