/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLinkedList.h
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
// .NAME vtkLinkedList - a templated linked list

#ifndef __vtkLinkedList_h
#define __vtkLinkedList_h

#include "vtkAbstractList.h"

template <class DType> class vtkLinkedListNode;
template <class DType> class vtkLinkedListIterator;

template <class DType>
class vtkLinkedList : public vtkAbstractList<DType>
{
  friend class vtkLinkedListIterator<DType>;

public:
  typedef vtkAbstractList<DType> Superclass;
  typedef vtkLinkedListIterator<DType> IteratorType;

  static vtkLinkedList<DType> *New();
  virtual const char* GetClassName() const { return "vtkLinkedList"; }

  // Description:
  // Return an iterator to the list. This iterator is allocated using
  // New, so the developer is responsible for deleating it.
  vtkLinkedListIterator<DType> *NewIterator();
  
  // Description:
  // Append an Item to the end of the linked list.
  int AppendItem(DType a);
  
  // Description:
  // Insert an Item to the front of the linked list.
  int PrependItem(DType a);
  
  // Description:
  // Insert an Item to the specific location in the linked list.
  int InsertItem(vtkIdType loc, DType a);
  
  // Description:
  // Sets the Item at the specific location in the list to a new value.
  // It also checks if the item can be set.
  // It returns VTK_OK if successfull.
  int SetItem(vtkIdType loc, DType a);

  // Description:
  // Sets the Item at the specific location in the list to a new value.
  // This method does not perform any error checking.
  void SetItemNoCheck(vtkIdType loc, DType a);

   // Description:
  // Remove an Item from the linked list
  int RemoveItem(vtkIdType id);
  
  // Description:
  // Return an item that was previously added to this linked list. 
  int GetItem(vtkIdType id, DType& ret);
      
  // Description:
  // Find an item in the linked list. Return VTK_OK if it was found
  // od VTK_ERROR if not found. The location of the item is returned in res.
  int FindItem(DType a, vtkIdType &res);

  // Description:
  // Find an item in the linked list using a comparison routine. 
  // Return VTK_OK if it was found
  // od VTK_ERROR if not found. The location of the item is returned in res.
  int FindItem(DType a, 
               vtkAbstractListCompareFunction(DType, compare), 
               vtkIdType &res);
  
  // Description:
  // Return the number of items currently held in this container. This
  // different from GetSize which is provided for some containers. GetSize
  // will return how many items the container can currently hold.
  vtkIdType GetNumberOfItems() const { return this->NumberOfItems; }
  
  // Description:
  // Returns the number of items the container can currently hold.
  // Since capacity is arbitrary for the linked list, this will 
  // always return the current number of elements.
  vtkIdType GetSize() const { return this->NumberOfItems; }

  // Description:
  // Removes all items from the container.
  void RemoveAllItems();

  // Description:
  // Since linked list does not have the notion of capacity,
  // this method always return VTK_ERROR.
  int SetSize(vtkIdType ) { return VTK_ERROR; }

  // Description:
  // This method dumps debug of the linked list.
  void DebugList();

protected:
  vtkLinkedList() {
    this->Head = 0; this->Tail = 0;
    this->NumberOfItems = 0; 
  }
  virtual ~vtkLinkedList();

  // Description:
  // Find a node with given index.
  vtkLinkedListNode<DType>* FindNode(vtkIdType i);

  vtkIdType NumberOfItems;
  vtkLinkedListNode<DType> *Head;
  vtkLinkedListNode<DType> *Tail;

private:
  vtkLinkedList(const vtkLinkedList<DType>&); // Not implemented
  void operator=(const vtkLinkedList<DType>&); // Not implemented
};

#ifdef VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION
#include "vtkLinkedList.txx"
#endif 

#endif
