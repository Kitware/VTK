/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkObjectFactoryCollection.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to William A. Hoffman who developed this class


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
// .NAME vtkObjectFactoryCollection - maintain a list of object factories
// .SECTION Description
// vtkObjectFactoryCollection is an object that creates and manipulates lists
// of object of type vtkObjectFactory.

// .SECTION see also
// vtkCollection vtkObjectFactory

#ifndef __vtkObjectFactoryCollection_h
#define __vtkObjectFactoryCollection_h

#include "vtkCollection.h"
#include "vtkObjectFactory.h"
#include "vtkDebugLeaks.h"

class VTK_EXPORT vtkObjectFactoryCollection : public vtkCollection
{
public:
  vtkTypeMacro(vtkObjectFactoryCollection,vtkCollection);
  static vtkObjectFactoryCollection *New() 
    {
#ifdef VTK_DEBUG_LEAKS
    vtkDebugLeaks::ConstructClass("vtkObjectFactoryCollection");
#endif    
return new vtkObjectFactoryCollection;};

  // Description:
  // Add an ObjectFactory from the list.
  void AddItem(vtkObjectFactory *t) { this->vtkCollection::AddItem((vtkObject *)t); }
  
  // Description:
  // Get the next ObjectFactory in the list. Return NULL when the end of the
  // list is reached.
  vtkObjectFactory *GetNextItem() { return static_cast<vtkObjectFactory *>(this->GetNextItemAsObject());}

protected:
  vtkObjectFactoryCollection() {};
  ~vtkObjectFactoryCollection() {};
  vtkObjectFactoryCollection(const vtkObjectFactoryCollection&);
  void operator=(const vtkObjectFactoryCollection&);


private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(vtkObject *o) { this->vtkCollection::AddItem(o); };

};


#endif
