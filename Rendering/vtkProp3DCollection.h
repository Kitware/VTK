/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProp3DCollection.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
#include "vtkProp3D.h"

class VTK_EXPORT vtkProp3DCollection : public vtkPropCollection
{
public:
  static vtkProp3DCollection *New();
  vtkTypeMacro(vtkProp3DCollection,vtkPropCollection);

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
  vtkProp3DCollection(const vtkProp3DCollection&);
  void operator=(const vtkProp3DCollection&);
    

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
  return vtkProp3D::SafeDownCast(this->GetNextItemAsObject());
}

inline vtkProp3D *vtkProp3DCollection::GetLastProp3D() 
{ 
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return vtkProp3D::SafeDownCast(this->Bottom->Item);
    }
}

#endif





