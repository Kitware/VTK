/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayMap.h
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
// .NAME vtkArrayMap - a dynamic map data structure
// .SECTION Description
// vtkArrayMap is a an array implementation of the map data structure
//
// Map data structure is a one dimensional sequence of pairs
// of key and data. On the higher level, it implements mapping
// from key values to data elements. It can be implemented using
// array of pairs, hash table, or different trees.

// .SECTION See also
// vtkAbstractMap

#include "vtkAbstractMap.h"

#ifndef __vtkArrayMap_h
#define __vtkArrayMap_h

template<class DataType> class vtkVector;
template<class KeyType, class DataType> class vtkArrayMapIterator;

template<class KeyType, class DataType>
class vtkArrayMap : public vtkAbstractMap<KeyType,DataType>
{
  friend class vtkArrayMapIterator<KeyType,DataType>;
  
public:
  typedef vtkAbstractMap<KeyType,DataType> Superclass;
  typedef vtkArrayMapIterator<KeyType,DataType> IteratorType;

  // Cannot use this macro because of the comma in the type name.
  // The CPP splits that in two and we ae in trouble.
  //vtkContainerTypeMacro((vtkArrayMap<KeyType,DataType>), vtkContainer);

  virtual const char *GetClassName() const {return "vtkArrayMap";} 

  static vtkArrayMap<KeyType,DataType> *New(); 

  // Description:
  // Return an iterator to the list. This iterator is allocated using
  // New, so the developer is responsible for deleating it.
  vtkArrayMapIterator<KeyType,DataType> *NewIterator();

  // Description:
  // Sets the item at with specific key to data.
  // It overwrites the old item.
  // It returns VTK_OK if successfull.
  int SetItem(const KeyType& key, const DataType& data);
  
  // Description:
  // Remove an Item with the key from the map.
  // It returns VTK_OK if successfull.
  int RemoveItem(const KeyType& key);
  
  // Description:
  // Remove all items from the map.
  void RemoveAllItems();
  
  // Description:
  // Return the data asociated with the key.
  // It returns VTK_OK if successfull.
  int GetItem(const KeyType& key, DataType& data);
  
  // Description:
  // Return the number of items currently held in this container. This
  // different from GetSize which is provided for some containers. GetSize
  // will return how many items the container can currently hold.
  vtkIdType GetNumberOfItems() const;

  void DebugList();

protected:
  vtkArrayMap() { this->Array = 0; }
  virtual ~vtkArrayMap();

  // Description:
  // Find vtkAbstractMapItem that with specific key
  virtual vtkAbstractMapItem<KeyType,DataType> 
    *FindDataItem(const KeyType key);

  vtkVector< vtkAbstractMapItem<KeyType,DataType>* > *Array;

private:
  vtkArrayMap(const vtkArrayMap<KeyType,DataType>&); // Not implemented
  void operator=(const vtkArrayMap<KeyType,DataType>&); // Not implemented
};

#ifdef VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION
#include "vtkArrayMap.txx"
#endif 

#endif
