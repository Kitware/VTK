/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRendererCollection.h
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
// .NAME vtkRendererCollection - a list of renderers
// .SECTION Description
// vtkRendererCollection represents and provides methods to manipulate a list 
// of renderers (i.e., vtkRenderer and subclasses). The list is unsorted and 
// duplicate entries are not prevented.

// .SECTION see also
// vtkRenderer vtkCollection

#ifndef __vtkRendererCollection_h
#define __vtkRendererCollection_h

#include "vtkCollection.h"
#include "vtkRenderer.h"

class VTK_RENDERING_EXPORT vtkRendererCollection : public vtkCollection
{
 public:
  static vtkRendererCollection *New();
  vtkTypeMacro(vtkRendererCollection,vtkCollection);

  // Description:
  // Add a Renderer to the list.
  void AddItem(vtkRenderer *a) {
    this->vtkCollection::AddItem((vtkObject *)a);};
  
  // Description:
  // Get the next Renderer in the list. Return NULL when at the end of the 
  // list.
  vtkRenderer *GetNextItem() {
    return static_cast<vtkRenderer *>(this->GetNextItemAsObject());};

  // Description:
  // Forward the Render() method to each renderer in the list.
  void Render();
  void RenderOverlay();

protected:  
  vtkRendererCollection() {};
  ~vtkRendererCollection() {};

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

private:
  vtkRendererCollection(const vtkRendererCollection&);  // Not implemented.
  void operator=(const vtkRendererCollection&);  // Not implemented.
};


#endif
