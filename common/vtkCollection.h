/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCollection.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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

class VTK_EXPORT vtkCollection : public vtkObject
{
public:
  // Description:
  // Construct with empty list.
  vtkCollection();

  // Description:
  // Desctructor for the vtkCollection class. This removes all 
  // objects from the collection.
  virtual ~vtkCollection();

  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkCollection *New() {return new vtkCollection;};
  const char *GetClassName() {return "vtkCollection";};

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
  virtual void RemoveItem(int i);  

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
  virtual void DeleteElement(vtkCollectionElement *); 
  int NumberOfItems;
  vtkCollectionElement *Top;
  vtkCollectionElement *Bottom;
  vtkCollectionElement *Current;

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





