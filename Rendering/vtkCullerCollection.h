/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCullerCollection.h
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
// .NAME vtkCullerCollection - a list of Cullers
// .SECTION Description
// vtkCullerCollection represents and provides methods to manipulate a list
// of Cullers (i.e., vtkCuller and subclasses). The list is unsorted and
// duplicate entries are not prevented.

// .SECTION see also
// vtkCuller vtkCollection 

#ifndef __vtkCullerC_h
#define __vtkCullerC_h

#include "vtkCollection.h"
#include "vtkCuller.h"

class VTK_EXPORT vtkCullerCollection : public vtkCollection
{
 public:
  static vtkCullerCollection *New();
  vtkTypeMacro(vtkCullerCollection,vtkCollection);

  // Description:
  // Add an Culler to the list.
  void AddItem(vtkCuller *a) {
    this->vtkCollection::AddItem((vtkObject *)a);};

  // Description:
  // Get the next Culler in the list.
  vtkCuller *GetNextItem() { 
    return vtkCuller::SafeDownCast(this->GetNextItemAsObject());};
  
  // Description:
  // Get the last Culler in the list.
  vtkCuller *GetLastItem();
  
protected:
  vtkCullerCollection() {};
  ~vtkCullerCollection() {};
  vtkCullerCollection(const vtkCullerCollection&);
  void operator=(const vtkCullerCollection&);
  

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

};


inline vtkCuller *vtkCullerCollection::GetLastItem() 
{ 
  if ( this->Bottom == NULL )
    {
    return NULL;
    }
  else
    {
    return vtkCuller::SafeDownCast(this->Bottom->Item);
    }
}

#endif





