/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProp3DCollection.h
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
// .NAME vtkProp3DCollection - a list of 3D props
// .SECTION Description
// vtkProp3DCollection represents and provides methods to manipulate a list of
// 3D props (i.e., vtkProp3D and subclasses). The list is unsorted and 
// duplicate entries are not prevented.

// .SECTION see also
// vtkProp3D vtkCollection 

#ifndef __vtkProp3DCollection_h
#define __vtkProp3DCollection_h

#include "vtkPropCollection.h"
class vtkProp3D;

class VTK_EXPORT vtkProp3DCollection : public vtkPropCollection
{
public:
  static vtkProp3DCollection *New();
  const char *GetClassName() {return "vtkProp3DCollection";};

  // Description:
  // Add an actor to the list.
  void AddItem(vtkProp3D *p);

  // Description:
  // Get the next actor in the list.
  vtkProp3D *GetNextProp3D();

  // Description:
  // Get the last actor in the list.
  vtkProp3D *GetLastProp3D();

protected:
  vtkProp3DCollection() {};
  ~vtkProp3DCollection() {};
  vtkProp3DCollection(const vtkProp3DCollection&) {};
  void operator=(const vtkProp3DCollection&) {};
    

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };
  void AddItem(vtkProp *o) { this->vtkPropCollection::AddItem(o); };

};

inline void vtkProp3DCollection::AddItem(vtkProp3D *a) 
{
  this->vtkCollection::AddItem((vtkObject *)a);
}

inline vtkProp3D *vtkProp3DCollection::GetNextProp3D() 
{ 
  return (vtkProp3D *)(this->GetNextItemAsObject());
}

inline vtkProp3D *vtkProp3DCollection::GetLastProp3D() 
{ 
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return (vtkProp3D *)(this->Bottom->Item);
    }
}

#endif





