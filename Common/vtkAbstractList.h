/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractList.h
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
// .NAME vtkAbstractList - a dynamic list data structure
// .SECTION Description
// vtkAbstractList is a an abstract templated superclass of all
// containers that implement list data structure.
//
// List data structure is a one dimensional sequence of elements
// with strict ordering. Every element has an index and each 
// element except the first and the last one, have unique 
// predecessor and successor. Examples of list data structure
// are dynamic array (vector) and linked list.

// .SECTION See also
// vtkContainer vtkAbstractMap

#include "vtkContainer.h"

#ifndef __vtkAbstractList_h
#define __vtkAbstractList_h

// Since some compilers have problems with keyword typename, we have 
// to do this with macros.
#define vtkAbstractListCompareFunction(KeyType, CompareFunction) \
    int (*CompareFunction)(const KeyType&  k1, const KeyType& k2)

template<class DType>
class vtkAbstractList : public vtkContainer
{
public:
  vtkContainerTypeMacro(vtkAbstractList<DType>, vtkContainer);

  // Just to avoid typing over and over, let us define some typedefs.
  // They will not work in subclasses, but this header file will 
  // be more readable.
  typedef vtkAbstractListCompareFunction(DType, CompareFunctionType);

  // Description:
  // Append an Item to the end of the list.
  // It returns VTK_OK if successfull.
  //virtual int AppendItem(DType a) = 0;
  
  // Description:
  // Insert an Item to the front of the list.
  // It returns VTK_OK if successfull.
  //virtual int PrependItem(DType a) = 0;
  
  // Description:
  // Insert an Item to the specific location in the list.
  // It returns VTK_OK if successfull.
  //virtual int InsertItem(vtkIdType loc, DType a) = 0;
  
  // Description:
  // Sets the Item at the specific location in the list to a new value.
  // It also checks if the item can be set.
  // It returns VTK_OK if successfull.
  //virtual int SetItem(vtkIdType loc, DType a) = 0;
  
  // Description:
  // Sets the Item at the specific location in the list to a new value.
  // This method does not perform any error checking.
  //virtual void SetItemNoCheck(vtkIdType loc, DType a) = 0;

  // Description:
  // Remove an Item from the list
  // It returns VTK_OK if successfull.
  //virtual int RemoveItem(vtkIdType id) = 0;
  
  // Description:
  // Return an item that was previously added to this list. 
  // It returns VTK_OK if successfull.
  //virtual int GetItem(vtkIdType id, DType& ret) = 0;
      
  // Description:
  // Find an item in the list. Return one if it was found, zero if it was
  // not found. The location of the item is returned in res.
  // It returns VTK_OK if successfull.
  //virtual int FindItem(DType a, vtkIdType &res) = 0;

  // Description:
  // Find an item in the list using a comparison routine. 
  // Return one if it was found, zero if it was
  // not found. The location of the item is returned in res.
  // It returns VTK_OK if successfull.
  //virtual int FindItem(DType a, CompareFunctionType compare, 
  //                     vtkIdType &res) = 0;

  // Description:
  // Set the capacity of the list.
  // It returns VTK_OK if successfull.
  //virtual int SetSize(vtkIdType size) = 0;
  
  // Description:
  // Return the number of items currently held in this container. This
  // different from GetSize which is provided for some containers. GetSize
  // will return how many items the container can currently hold.
  //virtual vtkIdType GetNumberOfItems() = 0;
  
  // Description:
  // Returns the number of items the container can currently hold.
  // This is the capacity of the container.
  //virtual vtkIdType GetSize() = 0;

protected:
  vtkAbstractList();
};

#ifdef VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION
#include "vtkAbstractList.txx"
#endif 

#endif
