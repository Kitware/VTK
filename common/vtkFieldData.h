/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFieldData.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
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
// points or cells in a dataset.
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
  void Initialize();

  // Description:
  // Allocate data for each array.
  int Allocate(const int sz, const int ext=1000);

  // Description:
  // Virtual constructor creates a field with the same number of data 
  // arrays and types of data arrays, but the arrays contain nothing.
  vtkFieldData *MakeObject();

  // Description:
  // Set the number of arrays used to define the field.
  void SetNumberOfArrays(int num);

  // Description:
  // Set an array to define the field.
  void SetArray(int i, vtkDataArray *);

  // Description:
  // Get the number of arrays of data available.
  int GetNumberOfArrays();

  // Description:
  // Return the ith array in the field. A NULL is returned if the
  // index i is out of range.
  vtkDataArray *GetArray(int i);

  // Description:
  // Add an array to the end of the array list, return the new array index
  int AddArray(vtkDataArray *array);

  // Description:
  // Add an array to the end of the array list, and set the name
  // return the new array index
  int AddArray(vtkDataArray *array, char *name);

  // Description:
  // Add an array to the end of the array list, and set the name
  // return the new array index. if array with given name
  // already exists - overwrites it
  int AddReplaceArray(vtkDataArray *array, char *name);

  // Description:
  // Add an array to the end of the array list, and set the name
  // return the new array index. returns -1 if array with given name
  // already exists and does not overwrite it
  int AddNoReplaceArray(vtkDataArray *array, char *name);

  // Description:
  // Return the array containing the ith component of the field. The
  // return value is an integer number n 0<=n<this->NumberOfArrays. Also,
  // an integer value is returned indicating the component in the array
  // is returned. Method returns -1 if specified component is not
  // in the field.
  int GetArrayContainingComponent(int i, int& arrayComp);

  // Description:
  // Return the array with the name given. Returns NULL is array not found.
  vtkDataArray *GetArray(char *arrayName);

  // Description:
  // Return the array with the name given. Returns NULL is array not found.
  // Also returns index of array if found, -1 otherwise
  vtkDataArray *GetArray(char *arrayName, int &index);

  // Description:
  // Set/Get the name for an array of data.
  void SetArrayName(int i,char *name);
  char *GetArrayName(int i);

  // Description:
  // Get the number of components in the field. This is determined by adding
  // up the components in each non-NULL array.
  int GetNumberOfComponents();

  // Description:
  // Get the number of tuples in the field. Note: some fields have arrays with
  // different numbers of tuples; this method returns the number of tuples in
  // the first array. Mixed-length arrays may have to be treated specially.
  int GetNumberOfTuples();

  // Description:
  // Set the number of tuples for each data array in the field.
  void SetNumberOfTuples(const int number);

  // Description:
  // Return a tuple consisting of a concatenation of all data from all
  // the different arrays. Note that everything is converted to and from
  // float values.
  float *GetTuple(const int i);

  // Description:
  // Copy the ith tuple value into a user provided tuple array. Make
  // sure that you've allocated enough space for the copy.
  void GetTuple(const int i, float * tuple);

  // Description:
  // Set the tuple value at the ith location. Set operations
  // mean that no range checking is performed, so they're faster.
  void SetTuple(const int i, const float * tuple);

  // Description:
  // Insert the tuple value at the ith location. Range checking is
  // performed and memory allocates as necessary.
  void InsertTuple(const int i, const float * tuple);

  // Description:
  // Insert the tuple value at the end of the tuple matrix. Range
  // checking is performed and memory is allocated as necessary.
  int InsertNextTuple(const float * tuple);

  // Description:
  // Get the component value at the ith tuple (or row) and jth component (or
  // column).
  float GetComponent(const int i, const int j);

  // Description:
  // Set the component value at the ith tuple (or row) and jth component (or
  // column).  Range checking is not performed, so set the object up properly
  // before invoking.
  void SetComponent(const int i, const int j, const float c);
  
  // Description:
  // Insert the component value at the ith tuple (or row) and jth component
  // (or column).  Range checking is performed and memory allocated as
  // necessary o hold data.
  void InsertComponent(const int i, const int j, const float c);

  // Description:
  // Copy a field by creating new data arrays (i.e., duplicate storage).
  void DeepCopy(vtkFieldData *da);

  // Description:
  // Copy a field by reference counting the data arrays.
  void ShallowCopy(vtkFieldData *da);

  // Description:
  // Squeezes each data array in the field (Squeeze() reclaims unused memory.)
  void Squeeze();

  // Description:
  // Resets each data array in the field (Reset() does not release memory but
  // it makes the arrays look like they are empty.)
  void Reset();

  // Description:
  // Get a field from a list of ids. Supplied field f should have same types 
  // and number of data arrays as this one (i.e., like MakeObject() returns).
  void GetField(vtkIdList *ptId, vtkFieldData *f);

  // Description:
  // For legacy compatibility. Do not use.
  void DeepCopy(vtkFieldData &da) {this->DeepCopy(&da);}
  void ShallowCopy(vtkFieldData &da) {this->ShallowCopy(&da);}
  void GetField(vtkIdList& ptId, vtkFieldData& f) {this->GetField(&ptId, &f);}
  
  // Description:
  // Return the memory in kilobytes consumed by this field data. Used to
  // support streaming and reading/writing data. The value returned is
  // guaranteed to be greater than or equal to the memory required to
  // actually represent the data represented by this object.
  virtual unsigned long GetActualMemorySize();
  
protected:
  vtkFieldData();
  ~vtkFieldData();
  vtkFieldData(const vtkFieldData&) {};
  void operator=(const vtkFieldData&) {};

  int NumberOfArrays;
  vtkDataArray **Data;
  char **ArrayNames;

  int TupleSize; //used for type conversion
  float *Tuple;
};

#endif
