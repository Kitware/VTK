/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssemblyPaths.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkAssemblyPaths - a list of lists of actors representing an assembly hierarchy
// .SECTION Description
// vtkAssemblyPaths represents a hierarchy of assemblies as a sequence of
// paths. Each path is a list of actors, starting from the root of the
// assembly down to the leaf actors. Methods are also provided to manipulate
// the path including propagating transformation matrices and actor properties.

// .SECTION see also
// vtkAssembly vtkActor

#ifndef __vtkAssemblyPaths_h
#define __vtkAssemblyPaths_h

#include "vtkActorCollection.h"
class vtkActor;

class VTK_EXPORT vtkAssemblyPaths : public vtkCollection
{
public:
  static vtkAssemblyPaths *New();
  vtkTypeMacro(vtkAssemblyPaths,vtkCollection);

  // Description:
  // Add a path to the list.
  void AddItem(vtkActorCollection *a);

  // Description:
  // Remove a path from the list.
  void RemoveItem(vtkActorCollection *a);

  // Description:
  // Determine whether a particular path is present. Returns its position
  // in the list.
  int IsItemPresent(vtkActorCollection *a);

  // Description:
  // Get the next path in the list.
  vtkActorCollection *GetNextItem();

protected:
  vtkAssemblyPaths() {};
  ~vtkAssemblyPaths() {};
  vtkAssemblyPaths(const vtkAssemblyPaths&) {};
  void operator=(const vtkAssemblyPaths&) {};
  
private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };
  void RemoveItem(vtkObject *o) { this->vtkCollection::RemoveItem(o); };
  void RemoveItem(int i) { this->vtkCollection::RemoveItem(i); };
  int  IsItemPresent(vtkObject *o) { return this->vtkCollection::IsItemPresent(o);};
};

inline void vtkAssemblyPaths::AddItem(vtkActorCollection *a) 
{
  this->vtkCollection::AddItem((vtkObject *)a);
}

inline void vtkAssemblyPaths::RemoveItem(vtkActorCollection *a) 
{
  this->vtkCollection::RemoveItem((vtkObject *)a);
}

inline int vtkAssemblyPaths::IsItemPresent(vtkActorCollection *a) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)a);
}

inline vtkActorCollection *vtkAssemblyPaths::GetNextItem() 
{ 
  return (vtkActorCollection *)(this->GetNextItemAsObject());
}

#endif
