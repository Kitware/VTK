/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCollection.h
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
// .NAME vtkCollection - create and manipulate unsorted lists of objects
// .SECTION Description
// vtkCollection is a general object for creating and manipulating lists
// of objects. The lists are unsorted and allow duplicate entries. 
// vtkCollection also serves as a base class for lists of specific types 
// of objects.

// .SECTION See Also
// vtkActorCollection vtkAssemblyPaths vtkDataSetCollection
// vtkImplicitFunctionCollection vtkLightCollection vtkPolyDataCollection
// vtkRenderWindowCollection vtkRendererCollection
// vtkStructuredPointsCollection vtkTransformCollection vtkVolumeCollection

#ifndef __vtkCollection_h
#define __vtkCollection_h

#include "vtkObject.h"

//BTX - begin tcl exclude
class vtkCollectionElement //;prevents pick-up by man page generator
{
 public:
  vtkCollectionElement():Item(NULL),Next(NULL) {};
  vtkObject *Item;
  vtkCollectionElement *Next;
};
//ETX end tcl exclude

class VTK_COMMON_EXPORT vtkCollection : public vtkObject
{
public:
  vtkTypeMacro(vtkCollection,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with empty list.
  static vtkCollection *New();

  // Description:
  // Add an object to the list. Does not prevent duplicate entries.
  void AddItem(vtkObject *);

  // Description:
  // Replace the i'th item in the collection with a
  void ReplaceItem(int i, vtkObject *);

  // Description:
  // Remove the i'th item in the list.
  // Be careful if using this function during traversal of the list using 
  // GetNextItemAsObject (or GetNextItem in derived class).  The list WILL
  // be shortened if a valid index is given!  If this->Current is equal to the
  // element being removed, have it point to then next element in the list.
  void RemoveItem(int i);  

  // Description:
  // Remove an object from the list. Removes the first object found, not
  // all occurrences. If no object found, list is unaffected.  See warning
  // in description of RemoveItem(int).
  void RemoveItem(vtkObject *);

  // Description:
  // Remove all objects from the list.
  void RemoveAllItems();

  // Description:
  // Search for an object and return location in list. If location == 0,
  // object was not found.
  int  IsItemPresent(vtkObject *);

  // Description:
  // Return the number of objects in the list.
  int  GetNumberOfItems();

  // Description:
  // Initialize the traversal of the collection. This means the data pointer
  // is set at the beginning of the list.
  void InitTraversal() { this->Current = this->Top;};

  // Description:
  // Get the next item in the collection. NULL is returned if the collection
  // is exhausted.
  vtkObject *GetNextItemAsObject();  

  // Description:
  // Get the i'th item in the collection. NULL is returned if i is out
  // of range
  vtkObject *GetItemAsObject(int i);

protected:
  vtkCollection();
  ~vtkCollection();

  virtual void DeleteElement(vtkCollectionElement *); 
  int NumberOfItems;
  vtkCollectionElement *Top;
  vtkCollectionElement *Bottom;
  vtkCollectionElement *Current;

private:
  vtkCollection(const vtkCollection&);
  void operator=(const vtkCollection&);
};


inline vtkObject *vtkCollection::GetNextItemAsObject()
{
  vtkCollectionElement *elem=this->Current;

  if ( elem != NULL )
    {
    this->Current = elem->Next;
    return elem->Item;
    }
  else
    {
    return NULL;
    }
}

#endif





