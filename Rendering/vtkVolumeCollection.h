/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeCollection.h
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
// .NAME vtkVolumeCollection - a list of volumes
// .SECTION Description
// vtkVolumeCollection represents and provides methods to manipulate a 
// list of volumes (i.e., vtkVolume and subclasses). The list is unsorted 
// and duplicate entries are not prevented.

// .SECTION see also
// vtkCollection vtkVolume

#ifndef __vtkVolumeC_h
#define __vtkVolumeC_h

#include "vtkPropCollection.h"
#include "vtkVolume.h"

class VTK_RENDERING_EXPORT vtkVolumeCollection : public vtkPropCollection
{
 public:
  static vtkVolumeCollection *New();
  vtkTypeMacro(vtkVolumeCollection,vtkPropCollection);

  // Description:
  // Add a Volume to the list.
  void AddItem(vtkVolume *a) {
    this->vtkCollection::AddItem((vtkObject *)a);};
    
  // Description:
  // Get the next Volume in the list. Return NULL when at the end of the 
  // list.
  vtkVolume *GetNextVolume() {
      return static_cast<vtkVolume *>(this->GetNextItemAsObject());};


  // Description:
  // Access routine provided for compatibility with previous
  // versions of VTK.  Please use the GetNextVolume() variant
  // where possible.
  vtkVolume *GetNextItem() { return this->GetNextVolume(); };

protected:
  vtkVolumeCollection() {};
  ~vtkVolumeCollection() {};
  vtkVolumeCollection(const vtkVolumeCollection&);
  void operator=(const vtkVolumeCollection&);

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };
  void AddItem(vtkProp *o) { this->vtkPropCollection::AddItem(o); };

};


#endif





