/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVector.h
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
// .NAME vtkVector - a dynamic vector

#ifndef __vtkVector_h
#define __vtkVector_h

#include "vtkAbstractList.h"

template <class DType> class vtkVectorIterator;

template <class DType>
class vtkVector : public vtkAbstractList<DType>
{
  friend class vtkVectorIterator<DType>;

public:
  typedef vtkAbstractList<DType> Superclass;
  typedef vtkVectorIterator<DType> IteratorType;
  
  static vtkVector<DType> *New();
  virtual const char* GetClassName() const { return "vtkVector"; }

  // Description:
  // Return an iterator to the list. This iterator is allocated using
  // New, so the developer is responsible for deleating it.
  vtkVectorIterator<DType> *NewIterator();
  
  // Description:
  // Append an Item to the end of the vector.
  int AppendItem(DType a);
  
  // Description:
  // Insert an Item to the front of the linked list.
  int PrependItem(DType a);
  
  // Description:
  // Insert an Item to the specific location in the vector.
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
  // Remove an Item from the vector
  int RemoveItem(vtkIdType id);
  
  // Description:
  // Return an item that was previously added to this vector. 
  int GetItem(vtkIdType id, DType& ret);
      
  // Description:
  // Return an item that was previously added to this vector. 
  void GetItemNoCheck(vtkIdType id, DType& ret);
      
  // Description:
  // Find an item in the vector. Return one if it was found, zero if it was
  // not found. The location of the item is returned in res.
  int FindItem(DType a, vtkIdType &res);

  // Description:
  // Find an item in the vector using a comparison routine. 
  // Return VTK_OK if it was found, VTK_ERROR if it was
  // not found. The location of the item is returned in res.
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
  vtkIdType GetSize() const { return this->Size; }

  // Description:
  // Removes all items from the container.
  void RemoveAllItems();

  // Description:
  // Set the capacity of the vector.
  // It returns VTK_OK if successfull.
  // If capacity is set, the vector will not resize.
  int SetSize(vtkIdType size);

  // Description:
  // Allow or disallow resizing. If resizing is disallowed, when
  // inserting too many elements, it will return VTK_ERROR.
  // Initially allowed.
  void SetResize(int r) { this->Resize = r; }
  void ResizeOn() { this->SetResize(1); }
  void ResizeOff() { this->SetResize(0); }
  int GetResize() const { return this->Resize; }

  // Description:
  // Display the content of the list.
  void DebugList();

protected:
  vtkVector() {
    this->Array = 0; this->NumberOfItems = 0; this->Size = 0; 
    this->Resize = 1; }
  virtual ~vtkVector();
  vtkIdType NumberOfItems;
  vtkIdType Size;
  int Resize;
  DType *Array;

private:
  vtkVector(const vtkVector<DType>&); // Not implemented
  void operator=(const vtkVector<DType>&); // Not implemented
};

#ifdef VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION
#include "vtkVector.txx"
#endif 

#endif
