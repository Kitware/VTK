/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCollection.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// vtkActorCollection vtkAssemblyPaths vtkDataSetCollection vtkImplicitFunctionCollection
// vtkLightCollection vtkPolyDataCollection vtkRenderWindowCollection vtkRendererCollection
// vtkStructuredPointsCollection vtkTransformCollection vtkVolumeCollection

#ifndef __vtkCollection_hh
#define __vtkCollection_hh

#include "vtkObject.hh"

//BTX - begin tcl exclude
class vtkCollectionElement //;prevents pick-up by man page generator
{
 public:
  vtkCollectionElement():Item(NULL),Next(NULL) {};
  vtkObject *Item;
  vtkCollectionElement *Next;
};
//ETX end tcl exclude

class vtkCollection : public vtkObject
{
public:
  vtkCollection();
  virtual ~vtkCollection();
  void PrintSelf(ostream& os, vtkIndent indent);
  char *GetClassName() {return "vtkCollection";};

  void AddItem(vtkObject *);
  void RemoveItem(vtkObject *);
  void RemoveAllItems();
  int  IsItemPresent(vtkObject *);
  int  GetNumberOfItems();
  void InitTraversal();
  vtkObject *GetNextItemAsObject();  

protected:
  int NumberOfItems;
  vtkCollectionElement *Top;
  vtkCollectionElement *Bottom;
  vtkCollectionElement *Current;

};

// Description:
// Initialize the traversal of the collection. This means the data pointer
// is set at the beginning of the list.
inline void vtkCollection::InitTraversal()
{
  this->Current = this->Top;
}

// Description:
// Get the next item in the collection. NULL is returned if the collection
// is exhausted.
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





