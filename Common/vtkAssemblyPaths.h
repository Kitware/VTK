/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssemblyPaths.h
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
// .NAME vtkAssemblyPaths - a list of lists of props representing an assembly hierarchy
// .SECTION Description
// vtkAssemblyPaths represents an assembly hierarchy as a list of 
// vtkAssemblyPath. Each path represents the complete path from the
// top level assembly (if any) down to the leaf prop.

// .SECTION see also
// vtkAssemblyPath vtkAssemblyNode vtkPicker vtkAssembly vtkProp

#ifndef __vtkAssemblyPaths_h
#define __vtkAssemblyPaths_h

#include "vtkCollection.h"
#include "vtkAssemblyPath.h"

class vtkProp;

class VTK_COMMON_EXPORT vtkAssemblyPaths : public vtkCollection
{
public:
  static vtkAssemblyPaths *New();
  vtkTypeMacro(vtkAssemblyPaths,vtkCollection);

  // Description:
  // Add a path to the list.
  void AddItem(vtkAssemblyPath *p);

  // Description:
  // Remove a path from the list.
  void RemoveItem(vtkAssemblyPath *p);

  // Description:
  // Determine whether a particular path is present. Returns its position
  // in the list.
  int IsItemPresent(vtkAssemblyPath *p);

  // Description:
  // Get the next path in the list.
  vtkAssemblyPath *GetNextItem();

  // Description:
  // Override the standard GetMTime() to check for the modified times
  // of the paths.
  virtual unsigned long GetMTime();

protected:
  vtkAssemblyPaths() {};
  ~vtkAssemblyPaths() {};
  vtkAssemblyPaths(const vtkAssemblyPaths&);
  void operator=(const vtkAssemblyPaths&);
  
private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };
  void RemoveItem(vtkObject *o) { this->vtkCollection::RemoveItem(o); };
  void RemoveItem(int i) { this->vtkCollection::RemoveItem(i); };
  int  IsItemPresent(vtkObject *o) 
    { return this->vtkCollection::IsItemPresent(o);};
};

inline void vtkAssemblyPaths::AddItem(vtkAssemblyPath *p) 
{
  this->vtkCollection::AddItem((vtkObject *)p);
}

inline void vtkAssemblyPaths::RemoveItem(vtkAssemblyPath *p) 
{
  this->vtkCollection::RemoveItem((vtkObject *)p);
}

inline int vtkAssemblyPaths::IsItemPresent(vtkAssemblyPath *p) 
{
  return this->vtkCollection::IsItemPresent((vtkObject *)p);
}

inline vtkAssemblyPath *vtkAssemblyPaths::GetNextItem() 
{ 
  return (vtkAssemblyPath *)(this->GetNextItemAsObject());
}

#endif
