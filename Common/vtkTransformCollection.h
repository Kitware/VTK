/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformCollection.h
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
  vtkTypeMacro(vtkTransformCollection,vtkCollection);
  static vtkTransformCollection *New();

  // Description:
  // Add a Transform to the list.
  void AddItem(vtkTransform *);

  // Description:
  // Get the next Transform in the list. Return NULL when the end of the
  // list is reached.
  vtkTransform *GetNextItem();

protected:
  vtkTransformCollection() {};
  ~vtkTransformCollection() {};
  vtkTransformCollection(const vtkTransformCollection&);
  void operator=(const vtkTransformCollection&);


private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

};

inline void vtkTransformCollection::AddItem(vtkTransform *t) 
{
  this->vtkCollection::AddItem((vtkObject *)t);
}

inline vtkTransform *vtkTransformCollection::GetNextItem() 
{ 
  return static_cast<vtkTransform *>(this->GetNextItemAsObject());
}

#endif
