/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkActor2DCollection.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Matt Turek who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkActor2DCollection
// .SECTION Description
// vtkActor2DCollection is a subclass of vtkCollection.  vtkActor2DCollection
// maintains a collection of vtkActor2D objects that is sorted by layer
// number, with lower layer numbers at the start of the list.  This allows
// the vtkActor2D objects to be rendered in the correct order. 

// .SECTION See Also
// vtkActor2D vtkCollection

#ifndef __vtkActor2DCollection_h
#define __vtkActor2DCollection_h

#include "vtkPropCollection.h"
#include "vtkActor2D.h"
class vtkViewport;

class VTK_COMMON_EXPORT vtkActor2DCollection : public vtkPropCollection
{
 public:
  // Description:
  // Desctructor for the vtkActor2DCollection class. This removes all 
  // objects from the collection.
  static vtkActor2DCollection *New();

  vtkTypeMacro(vtkActor2DCollection,vtkPropCollection);

  // Description:
  // Sorts the vtkActor2DCollection by layer number.  Smaller layer
  // numbers are first.  Layer numbers can be any integer value.
  void Sort();
  
  // Description:
  // Add an actor to the list.  The new actor is inserted in the list
  // according to it's layer number.
  void AddItem(vtkActor2D *a);

  // Description:
  // Standard Collection methods
  int IsItemPresent(vtkActor2D *a);
  vtkActor2D *GetNextActor2D();
  vtkActor2D *GetLastActor2D();

  // Description:
 // Access routines that are provided for compatibility with previous
 // version of VTK.  Please use the GetNextActor2D(), GetLastActor2D()
 // variants where possible.
 vtkActor2D *GetNextItem();
 vtkActor2D *GetLastItem();

    
  // Description:
  // Sort and then render the collection of 2D actors.  
  void RenderOverlay(vtkViewport* viewport);


protected:
  vtkActor2DCollection() {};
  ~vtkActor2DCollection();
  vtkActor2DCollection(const vtkActor2DCollection&);
  void operator=(const vtkActor2DCollection&);

  virtual void DeleteElement(vtkCollectionElement *); 

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };
  void AddItem(vtkProp *o) { this->vtkPropCollection::AddItem(o); };
  int IsItemPresent(vtkObject *o) { return this->vtkCollection::IsItemPresent(o); };

};

inline int vtkActor2DCollection::IsItemPresent(vtkActor2D *a) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)a);
}

inline vtkActor2D *vtkActor2DCollection::GetNextActor2D() 
{ 
  return static_cast<vtkActor2D *>(this->GetNextItemAsObject());
}

inline vtkActor2D *vtkActor2DCollection::GetLastActor2D() 
{ 
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return static_cast<vtkActor2D *>(this->Bottom->Item);
    }
}

inline vtkActor2D *vtkActor2DCollection::GetNextItem() 
{ 
  return this->GetNextActor2D();
}

inline vtkActor2D *vtkActor2DCollection::GetLastItem() 
{
  return this->GetLastActor2D();
}

#endif





