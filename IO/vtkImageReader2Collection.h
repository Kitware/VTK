/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageReader2Collection.h
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
// .NAME vtkImageReader2Collection - maintain a list of implicit functions
// .SECTION Description
// vtkImageReader2Collection is an object that creates and manipulates
// lists of objects of type vtkImplicitFunction. 
// .SECTION See Also
// vtkCollection vtkPlaneCollection

#ifndef __vtkImageReader2Collection_h
#define __vtkImageReader2Collection_h

#include "vtkCollection.h"
#include "vtkImageReader2.h"

class VTK_IO_EXPORT vtkImageReader2Collection : public vtkCollection
{
public:
  vtkTypeRevisionMacro(vtkImageReader2Collection,vtkCollection);
  static vtkImageReader2Collection *New();

  // Description:
  // Add an implicit function to the list.
  void AddItem(vtkImageReader2 *);

  // Description:
  // Get the next implicit function in the list.
  vtkImageReader2 *GetNextItem();
  
protected:
  vtkImageReader2Collection() {};
  ~vtkImageReader2Collection() {};
  

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

private:
  vtkImageReader2Collection(const vtkImageReader2Collection&);  // Not implemented.
  void operator=(const vtkImageReader2Collection&);  // Not implemented.
};

inline void vtkImageReader2Collection::AddItem(vtkImageReader2 *f) 
{
  this->vtkCollection::AddItem((vtkObject *)f);
}

inline vtkImageReader2 *vtkImageReader2Collection::GetNextItem() 
{ 
  return static_cast<vtkImageReader2*>(this->GetNextItemAsObject());
}

#endif
