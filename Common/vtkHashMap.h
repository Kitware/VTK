/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHashMap.h
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
// .NAME vtkHashMap - a dynamic map data structure
// .SECTION Description
// vtkHashMap is a an array implementation of the map data structure
//
// Map data structure is a one dimensional sequence of pairs
// of key and data. On the higher level, it implements mapping
// from key values to data elements. It can be implemented using
// array of pairs, hash table, or different trees.

// .SECTION See also
// vtkAbstractMap

#include "vtkAbstractMap.h"

#ifndef __vtkHashMap_h
#define __vtkHashMap_h

template<class DataType> class vtkVector;
template<class KeyType, class DataType> class vtkHashMapIterator;

template<class KeyType, class DataType>
class vtkHashMap : public vtkAbstractMap<KeyType,DataType>
{
  friend class vtkHashMapIterator<KeyType,DataType>;
  
public:
  typedef vtkAbstractMap<KeyType,DataType> Superclass;
  typedef vtkHashMapIterator<KeyType,DataType> IteratorType;
  
  // Cannot use this macro because of the comma in the type name.
  // The CPP splits that in two and we ae in trouble.
  //vtkContainerTypeMacro((vtkHashMap<KeyType,DataType>), vtkContainer);

  virtual const char* GetClassName() const {return "vtkHashMap";} 
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkHashMap<KeyType,DataType>* New(); 
  
  // Description:
  // Return an iterator to the list. This iterator is allocated using
  // New, so the developer is responsible for deleating it.
  vtkHashMapIterator<KeyType,DataType>* NewIterator();
  
  // Description:
  // Sets the item with the given key to the given data.  It
  // overwrites the old value if it exists, or inserts a new value
  // otherwise.  It returns VTK_OK if successfull.
  int SetItem(const KeyType& key, const DataType& data);
  
  // Description:
  // Removes the item with the given key from the map.  It returns
  // VTK_OK if successfull.
  int RemoveItem(const KeyType& key);
  
  // Description:
  // Remove all items from the map.
  void RemoveAllItems();
  
  // Description:
  // Get the data asociated with the given key.
  // It returns VTK_OK if successfull.
  int GetItem(const KeyType& key, DataType& data);

  // Description:
  // Return the number of items currently held in this container.
  vtkIdType GetNumberOfItems() const;
  
  // Description:
  // Set/Get the maximum allowed load factor.  If the ratio of number
  // of items to number of buckets exceeds this value, the number of
  // buckets will be increased.  A value of zero indicates that no
  // re-hashing is to occur.
  void SetMaximumLoadFactor(float factor);
  float GetMaximumLoadFactor() const;
  
  // Description:
  // Set/Get the number of buckets currently used in the hash table.
  // Setting the number of buckets will automatically set the
  // MaximumLoadFactor to 0 to prevent resizing.
  void SetNumberOfBuckets(vtkIdType n);
  vtkIdType GetNumberOfBuckets() const;  
  
protected:
  vtkHashMap();
  virtual ~vtkHashMap();
  
  typedef vtkAbstractMapItem<KeyType,DataType> ItemType;
  typedef vtkVector<ItemType> BucketType;
  
  // Check if the load factor is okay.  Increase number of buckets and
  // re-hash if necessary.
  void CheckLoadFactor();
  
  // Change the number of buckets to the given number and re-hash the
  // items.
  void RehashItems(vtkIdType newNumberOfBuckets);

  // Description:
  // Hash a key to give an index in the range [0,nbuckets-1].
  vtkIdType HashKey(const KeyType& key, vtkIdType nbuckets);
  
  float MaximumLoadFactor;
  vtkIdType NumberOfItems;
  vtkIdType NumberOfBuckets;
  BucketType** Buckets;
  
private:
  vtkHashMap(const vtkHashMap<KeyType,DataType>&); // Not implemented
  void operator=(const vtkHashMap<KeyType,DataType>&); // Not implemented
};

static inline unsigned long vtkHashMapHashMethod(int x) 
{ return static_cast<unsigned long>(x); }
static inline unsigned long vtkHashMapHashMethod(const char* s)
{
  unsigned long h = 0;
  for(;*s;++s)
    {
    h = 5*h + *s;
    }
  return h;
}

static inline unsigned long vtkHashMapHashMethod(vtkObjectBase* o) 
{ 
#if defined ( _MSC_VER )
  return PtrToUlong(o);
#else
  return reinterpret_cast<unsigned long>(o); 
#endif
}

#ifdef VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION
#include "vtkHashMap.txx"
#endif 

#endif
