/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFieldData.h
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
// .NAME vtkFieldData - represent and manipulate fields of data
// .SECTION Description
// vtkFieldData represents and manipulates fields of data. The model of a field
// is a m x n matrix of data values, where m is the number of tuples, and n
// is the number of components. (A tuple is a row of n components in the
// matrix.) The field is assumed to be composed of a set of one or more data
// arrays, where the data in the arrays are of different types (e.g., int,
// float, char, etc.), and there may be variable numbers of components in
// each array. Note that each data array is assumed to be "m" in length
// (i.e., number of tuples), which typically corresponds to the number of
// points or cells in a dataset. Also, each data array must have a
// character-string name. (This is used to manipulate data.)
//
// There are two ways of manipulating and interfacing to fields. You can do
// it generically by manipulating components/tuples via a float-type data
// exchange, or you can do it by grabbing the arrays and manipulating them
// directly. The former is simpler but performs type conversion, which is bad
// if your data has non-castable types like (void) pointers, or you lose
// information as a result of the cast. The, more efficient method means
// managing each array in the field.  Using this method you can create
// faster, more efficient algorithms that do not lose information.

// .SECTION See Also
// vtkDataArray vtkAttribueData vtkPointData vtkCellData

#ifndef __vtkFieldData_h
#define __vtkFieldData_h

#include "vtkDataArray.h"
#include "vtkIdList.h"

class VTK_EXPORT vtkFieldData : public vtkObject
{
public:
  static vtkFieldData *New();

  vtkTypeMacro(vtkFieldData,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Release all data but do not delete object.
  virtual void Initialize();

  // Description:
  // Allocate data for each array.
  // Note that ext is no longer used.
  int Allocate(const vtkIdType sz, const vtkIdType ext=1000);

  // Description:
  // Virtual constructor creates a field with the same number of data 
  // arrays and types of data arrays, but the arrays contain nothing.
  vtkFieldData *MakeObject();

  // Description:
  // AllocateOfArrays actually sets the number of
  // vtkDataArray pointers in the vtkFieldData object, not the
  // number of used pointers (arrays). Adding more arrays will
  // cause the object to dynamically adjust the number of pointers
  // if it needs to extend. Although AllocateArrays can
  // be used if the number of arrays which will be added is
  // known, it can be omitted with a small computation cost.
  void AllocateArrays(int num);

#ifndef VTK_REMOVE_LEGACY_CODE
  void SetNumberOfArrays(int num)
    {
      vtkWarningMacro("This method will be deprecated. Please use "
		      "AllocateArrays() instead.");
      this->AllocateArrays(num);
    }
#endif

  // Description:
  // Get the number of arrays of data available.
  // This does not include NULL array pointers therefore after
  // fd->AllocateArray(n); nArrays = GetNumberOfArrays()
  // nArrays is not necessarily equal to n.
  int GetNumberOfArrays();

  // Description:
  // Add an array to the array list. If an array with the same name
  // already exists - then the added array will replace it.
  int AddArray(vtkDataArray *array);

  // Description:
  // Remove an array (with the given name) from the list of arrays.
  virtual void RemoveArray(const char *name)
    {
      int i;
      this->GetArray(name, i);
      this->RemoveArray(i);
    }

  // Description:
  // Return the ith array in the field. A NULL is returned if the
  // index i is out of range.
  vtkDataArray *GetArray(int i);

  // Description:
  // Return the array with the name given. Returns NULL is array not found.
  // Also returns index of array if found, -1 otherwise
  vtkDataArray *GetArray(const char *arrayName, int &index);

  // Return the array with the name given. Returns NULL is array not found.
  vtkDataArray *GetArray(const char *arrayName);

  // Description:
  // Get the name of ith array.
  // Note that this is equivalent to:
  // GetArray(i)->GetName() if ith array pointer is not NULL
  const char* GetArrayName(int i)
    {
      vtkDataArray* da;
      if ((da=this->GetArray(i)))
	{
	return da->GetName();
	}
      else
	{
	return 0;
	}
    }

  // Description:
  // Copy a field by creating new data arrays (i.e., duplicate storage).
  virtual void DeepCopy(vtkFieldData *da);

  // Description:
  // Copy a field by reference counting the data arrays.
  virtual void ShallowCopy(vtkFieldData *da);

  // Description:
  // Squeezes each data array in the field (Squeeze() reclaims unused memory.)
  void Squeeze();

  // Description:
  // Resets each data array in the field (Reset() does not release memory but
  // it makes the arrays look like they are empty.)
  void Reset();

  // Description:
  // Return the memory in kilobytes consumed by this field data. Used to
  // support streaming and reading/writing data. The value returned is
  // guaranteed to be greater than or equal to the memory required to
  // actually represent the data represented by this object.
  virtual unsigned long GetActualMemorySize();

  // Description:
  // Check object's components for modified times.
  unsigned long int GetMTime();
  
  // Description:
  // Get a field from a list of ids. Supplied field f should have same types 
  // and number of data arrays as this one (i.e., like MakeObject() returns).
  // This method should not be used if the instance is from a
  // subclass of vtkFieldData (vtkPointData or vtkCellData).
  // This is because in those cases, the attribute data is 
  // stored with the other fields and will cause the method
  // to behave in an unexpected way.
  void GetField(vtkIdList *ptId, vtkFieldData *f);

  // Description:
  // Return the array containing the ith component of the field. The
  // return value is an integer number n 0<=n<this->NumberOfArrays. Also,
  // an integer value is returned indicating the component in the array
  // is returned. Method returns -1 if specified component is not
  // in the field.
  // This method should not be used if the instance is from a
  // subclass of vtkFieldData (vtkPointData or vtkCellData).
  // This is because in those cases, the attribute data is 
  // stored with the other fields and will cause the method
  // to behave in an unexpected way.
  int GetArrayContainingComponent(int i, int& arrayComp);

  // Description:
  // Get the number of components in the field. This is determined by adding
  // up the components in each non-NULL array.
  // This method should not be used if the instance is from a
  // subclass of vtkFieldData (vtkPointData or vtkCellData).
  // This is because in those cases, the attribute data is 
  // stored with the other fields and will cause the method
  // to behave in an unexpected way.
  int GetNumberOfComponents();

  // Description:
  // Get the number of tuples in the field. Note: some fields have arrays with
  // different numbers of tuples; this method returns the number of tuples in
  // the first array. Mixed-length arrays may have to be treated specially.
  // This method should not be used if the instance is from a
  // subclass of vtkFieldData (vtkPointData or vtkCellData).
  // This is because in those cases, the attribute data is 
  // stored with the other fields and will cause the method
  // to behave in an unexpected way.
  vtkIdType GetNumberOfTuples();

  // Description:
  // Set the number of tuples for each data array in the field.
  // This method should not be used if the instance is from a
  // subclass of vtkFieldData (vtkPointData or vtkCellData).
  // This is because in those cases, the attribute data is 
  // stored with the other fields and will cause the method
  // to behave in an unexpected way.
  void SetNumberOfTuples(const vtkIdType number);

  // Description:
  // Return a tuple consisting of a concatenation of all data from all
  // the different arrays. Note that everything is converted to and from
  // float values.
  // This method should not be used if the instance is from a
  // subclass of vtkFieldData (vtkPointData or vtkCellData).
  // This is because in those cases, the attribute data is 
  // stored with the other fields and will cause the method
  // to behave in an unexpected way.
  float *GetTuple(const vtkIdType i);

  // Description:
  // Copy the ith tuple value into a user provided tuple array. Make
  // sure that you've allocated enough space for the copy.
  // This method should not be used if the instance is from a
  // subclass of vtkFieldData (vtkPointData or vtkCellData).
  // This is because in those cases, the attribute data is 
  // stored with the other fields and will cause the method
  // to behave in an unexpected way.
  void GetTuple(const vtkIdType i, float * tuple);

  // Description:
  // Set the tuple value at the ith location. Set operations
  // mean that no range checking is performed, so they're faster.
  // This method should not be used if the instance is from a
  // subclass of vtkFieldData (vtkPointData or vtkCellData).
  // This is because in those cases, the attribute data is 
  // stored with the other fields and will cause the method
  // to behave in an unexpected way.
  void SetTuple(const vtkIdType i, const float * tuple);

  // Description:
  // Insert the tuple value at the ith location. Range checking is
  // performed and memory allocates as necessary.
  // This method should not be used if the instance is from a
  // subclass of vtkFieldData (vtkPointData or vtkCellData).
  // This is because in those cases, the attribute data is 
  // stored with the other fields and will cause the method
  // to behave in an unexpected way.
  void InsertTuple(const vtkIdType i, const float * tuple);

  // Description:
  // Insert the tuple value at the end of the tuple matrix. Range
  // checking is performed and memory is allocated as necessary.
  // This method should not be used if the instance is from a
  // subclass of vtkFieldData (vtkPointData or vtkCellData).
  // This is because in those cases, the attribute data is 
  // stored with the other fields and will cause the method
  // to behave in an unexpected way.
  vtkIdType InsertNextTuple(const float * tuple);

  // Description:
  // Get the component value at the ith tuple (or row) and jth component (or
  // column).
  // This method should not be used if the instance is from a
  // subclass of vtkFieldData (vtkPointData or vtkCellData).
  // This is because in those cases, the attribute data is 
  // stored with the other fields and will cause the method
  // to behave in an unexpected way.
  float GetComponent(const vtkIdType i, const int j);

  // Description:
  // Set the component value at the ith tuple (or row) and jth component (or
  // column).  Range checking is not performed, so set the object up properly
  // before invoking.
  // This method should not be used if the instance is from a
  // subclass of vtkFieldData (vtkPointData or vtkCellData).
  // This is because in those cases, the attribute data is 
  // stored with the other fields and will cause the method
  // to behave in an unexpected way.
  void SetComponent(const vtkIdType i, const int j, const float c);
  
  // Description:
  // Insert the component value at the ith tuple (or row) and jth component
  // (or column).  Range checking is performed and memory allocated as
  // necessary o hold data.
  // This method should not be used if the instance is from a
  // subclass of vtkFieldData (vtkPointData or vtkCellData).
  // This is because in those cases, the attribute data is 
  // stored with the other fields and will cause the method
  // to behave in an unexpected way.
  void InsertComponent(const vtkIdType i, const int j, const float c);

protected:

  vtkFieldData();
  ~vtkFieldData();
  vtkFieldData(const vtkFieldData&);
  void operator=(const vtkFieldData&);

  int NumberOfArrays;
  int NumberOfActiveArrays;
  vtkDataArray **Data;

  int TupleSize; //used for type conversion
  float *Tuple;

  // Description:
  // Set an array to define the field.
  void SetArray(int i, vtkDataArray *array);

  virtual void RemoveArray(int index);

//BTX

public:

  class VTK_EXPORT BasicIterator
  {
  public:
    BasicIterator();
    BasicIterator(const BasicIterator& source);
    BasicIterator(const int* list, unsigned int listSize);
    BasicIterator& operator=(const BasicIterator& source);
    virtual ~BasicIterator();

    int GetListSize() const
      {
	return this->ListSize;
      }
    int GetCurrentIndex()
      {
	return this->List[this->Position];
      }
    int BeginIndex()
      {
	this->Position = -1;
	return this->NextIndex();
      }
    int End() const
      {
	return (this->Position >= this->ListSize);
      }
    int NextIndex()
      {
	this->Position++;
	return (this->End() ? -1 : this->List[this->Position]);
      }
    
  protected:
    int IsInList(int index);

    int* List;
    int ListSize;
    int Position;
  };

  class VTK_EXPORT Iterator : public BasicIterator
  {
  public:

    Iterator();
    Iterator(const Iterator& source);
    Iterator& operator=(const Iterator& source);
    virtual ~Iterator();
    Iterator(vtkFieldData* dsa, const int* list=0, 
	     unsigned int listSize=0);

    vtkDataArray* Begin()
      {
	this->Position = -1;
	return this->Next();
      }

    vtkDataArray* Next()
      {
	this->Position++;
	return (this->End() ? 0 : 
		Fields->GetArray(this->List[this->Position]));
      }

    void DetachFieldData();

  protected:
    vtkFieldData* Fields;
    int Detached;
  };

//ETX

};


#endif
