/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractMap.h
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
// .NAME vtkAbstractMap - a dynamic map data structure
// .SECTION Description
// vtkAbstractMap is a an abstract templated superclass of all
// containers that implement map data structure.
//
// Map data structure is a one dimensional sequence of pairs
// of key and data. On the higher level, it implements mapping
// from key values to data elements. It can be implemented using
// array of pairs, hash table, or different trees.

// .SECTION See also
// vtkContainer

#include "vtkContainer.h"

#ifndef __vtkAbstractMap_h
#define __vtkAbstractMap_h

template<class KeyType, class DataType>
class vtkAbstractMap : public vtkContainer
{
public:
  // Cannot use this macro because of the comma in the type name.
  // The CPP splits that in two and we ae in trouble.
  //vtkContainerTypeMacro((vtkAbstractMap<KeyType,DataType>), vtkContainer);

  typedef vtkContainer Superclass; 
  virtual const char *GetClassName() 
    {return "vtkAbstractMap<KeyType,DataType>";} 
  static int IsTypeOf(const char *type) 
  { 
    if ( !strcmp("vtkAbstractMap<KeyType,DataType>",type) ) 
      { 
      return 1;
      }
    return vtkContainer::IsTypeOf(type);
  }
  virtual int IsA(const char *type)
  {
    return this->vtkAbstractMap<KeyType,DataType>::IsTypeOf(type);
  }
  static vtkAbstractMap<KeyType,DataType>* SafeDownCast(vtkObject *o)
  {
    if ( o && o->IsA("vtkAbstractMap<KeyType,DataType>") )
      {
      return (vtkAbstractMap<KeyType,DataType> *)o;
      }
    return NULL;
  }

  // Description:
  // Sets the Item at the specific location in the list to a new value.
  // It also checks if the item can be set.
  // It returns VTK_OK if successfull.
  virtual int SetItem(KeyType key, DataType data) = 0;
  
  // Description:
  // Sets the Item at the specific location in the list to a new value.
  // This method does not perform any error checking.
  virtual void SetItemNoCheck(KeyType key, DataType data) = 0;

  // Description:
  // Remove an Item from the list
  // It returns VTK_OK if successfull.
  virtual int RemoveItem(KeyType key) = 0;
  
  // Description:
  // Return an item that was previously added to this list. 
  // It returns VTK_OK if successfull.
  virtual int GetItem(KeyType key, DataType& data) = 0;
  
  // Description:
  // Return the number of items currently held in this container. This
  // different from GetSize which is provided for some containers. GetSize
  // will return how many items the container can currently hold.
  virtual vtkIdType GetNumberOfItems() = 0;
};

#endif
