/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArray.h
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
// .NAME vtkDataArray - abstract superclass for arrays
// .SECTION Description
// vtkDataArray is an abstract superclass for data array objects. This class
// defines an API that all array objects must support. Note that the concrete
// subclasses of this class represent data in native form (char, int, etc.) and
// often have specialized more efficient methods for operating on this data 
// (for example, getting pointers to data or getting/inserting data in native
// form). 
//
// The logical structure of this class is an array of tuples, where each
// tuple is made up of n-components (also called a component group), and n is
// the number of component values in a tuple(n >= 1).  Another view of this
// class is a mxn matrix, where m is the number of tuples, and n is the
// number of components in a tuple. Thus vtkDataArray can be used to
// represent scalars (1-4 components), 3D vectors (3 components), texture
// coordinates (1-3 components), tensors, (9 components) and so on.
// 
// Each data array is required to have a character-string name. The 
// naming of the array occurs automatically when it is instantiated, but 
// you are free to name arrays using the SetName() method. 
// (The array name is used for data manipulation.)
//
// .SECTION See Also
// vtkBitArray vtkCharArray vtkUnsignedCharArray vtkShortArray
// vtkUnsignedShortArray vtkIntArray vtkUnsignedIntArray vtkLongArray
// vtkUnsignedLongArray vtkFloatArray vtkDoubleArray vtkVoidArray

#ifndef __vtkDataArray_h
#define __vtkDataArray_h

#include "vtkObject.h"
#include "vtkLargeInteger.h"

class vtkFloatArray;
class vtkLookupTable;

class VTK_EXPORT vtkDataArray : public vtkObject 
{
public:
  vtkTypeMacro(vtkDataArray,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Allocate memory for this array. Delete old storage only if necessary.
  // Note that ext is no longer used.
  virtual int Allocate(const int sz, const int ext=1000) = 0;
  virtual void Initialize() = 0;

  // Description:
  // Virtual constructor creates an object of the same type as this one.
  // The created object also has the same number of components. You are 
  // responsible for freeing the object.
  virtual vtkDataArray *MakeObject() = 0;

  // Description:
  // Return the underlying data type. An integer indicating data type is 
  // returned as specified in vtkSetGet.h.
  virtual int GetDataType() = 0;

  // Description:
  // Set/Get the dimension (n) of the components. Must be >= 1. Make sure that
  // this is set before allocation.
  vtkSetClampMacro(NumberOfComponents,int,1,VTK_LARGE_INTEGER);
  int GetNumberOfComponents() {return this->NumberOfComponents;};

  // Description:
  // Set the number of tuples (a component group) in the array. Note that 
  // this may allocate space depending on the number of components.
  virtual void SetNumberOfTuples(const int number) = 0;

  // Description:
  // Get the number of tuples (a component group) in the array.
  int GetNumberOfTuples() 
    {return (this->MaxId + 1)/this->NumberOfComponents;}

  // Description:
  // Get the data tuple at ith location. Return it as a pointer to an array.
  // Note: this method is not thread-safe, and the pointer is only valid
  // as long as another method invocation to a vtk object is not performed.
  virtual float *GetTuple(const int i) = 0;

  // Description:
  // Get the data tuple at ith location by filling in a user-provided array,
  // Make sure that your array is large enough to hold the NumberOfComponents
  // amount of data being returned.
  virtual void GetTuple(const int i, float * tuple) = 0;
  virtual void GetTuple(const int i, double * tuple);

  // Description:
  // Set the data tuple at ith location. Note that range checking or
  // memory allocation is not performed; use this method in conjunction
  // with SetNumberOfTuples() to allocate space.
  virtual void SetTuple(const int i, const float * tuple) = 0;
  virtual void SetTuple(const int i, const double * tuple);

  // Description:
  // Insert the data tuple at ith location. Note that memory allocation
  // is performed as necessary to hold the data.
  virtual void InsertTuple(const int i, const float * tuple) = 0;
  virtual void InsertTuple(const int i, const double * tuple);

  // Description:
  // Insert the data tuple at the end of the array and return the location at
  // which the data was inserted. Memory is allocated as necessary to hold
  // the data.
  virtual int InsertNextTuple(const float * tuple) = 0;
  virtual int InsertNextTuple(const double * tuple);

  // Description:
  // Return the data component at the ith tuple and jth component location.
  // Note that i is less than NumberOfTuples and j is less than 
  // NumberOfComponents.
  virtual float GetComponent(const int i, const int j);

  // Description:
  // Return the active data component at the ith tuple location
  // by calling GetComponent(i, this->ActiveComponent).
  virtual float GetComponent(const int i);

  // Description:
  // Set the data component at the ith tuple and jth component location.
  // Note that i is less than NumberOfTuples and j is less than
  //  NumberOfComponents. Make sure enough memory has been allocated 
  // (use SetNumberOfTuples() and SetNumberOfComponents()).
  virtual void SetComponent(const int i, const int j, const float c);

  // Description:
  // Set the active data component at the ith tuple location
  // by calling SetComponent(i, this->ActiveComponent, c)
  virtual void SetComponent(const int i, const float c);

  // Description:
  // Insert the data component at ith tuple and jth component location. 
  // Note that memory allocation is performed as necessary to hold the data.
  virtual void InsertComponent(const int i, const int j, const float c);

  // Description:
  // Get the data as a float array in the range (tupleMin,tupleMax) and
  // (compMin, compMax). The resulting float array consists of all data in
  // the tuple range specified and only the component range specified. This
  // process typically requires casting the data from native form into
  // floating point values. This method is provided as a convenience for data
  // exchange, and is not very fast.
  virtual void GetData(int tupleMin, int tupleMax, int compMin, int compMax, 
		       vtkFloatArray* data);

  // Description:
  // Deep copy of data. Copies data from different data arrays even if
  // they are different types (using floating-point exchange).
  virtual void DeepCopy(vtkDataArray *da);

  // Description:
  // Return a void pointer. For image pipeline interface and other 
  // special pointer manipulation.
  virtual void *GetVoidPointer(const int id) = 0;

  // Description:
  // Free any unnecessary memory.
  virtual void Squeeze() = 0;

  // Description:
  // Resize the array while conserving the data.
  virtual void Resize(int numTuples) = 0;

  // Description:
  // Reset to an empty state, without freeing any memory.
  void Reset() 
    {this->MaxId = -1;}

  // Description:
  // Return the size of the data.
  int GetSize() 
    {return this->Size;}
  
  // Description:
  // What is the maximum id currently in the array.
  int GetMaxId() 
    {return this->MaxId;}

  // Description:
  // This method lets the user specify data to be held by the array.  The 
  // array argument is a pointer to the data.  size is the size of 
  // the array supplied by the user.  Set save to 1 to keep the class
  // from deleting the array when it cleans up or reallocates memory.
  // The class uses the actual array provided; it does not copy the data 
  // from the suppled array.
  virtual void SetVoidArray(void *vtkNotUsed(array),int vtkNotUsed(size),
			    int vtkNotUsed(save)) {};

  // Description:
  // Return the memory in kilobytes consumed by this data array. Used to
  // support streaming and reading/writing data. The value returned is
  // guaranteed to be greater than or equal to the memory required to
  // actually represent the data represented by this object. The 
  // information returned is valid only after the pipeline has 
  // been updated.
  unsigned long GetActualMemorySize();
  
  // Description:
  // Create default lookup table. Generally used to create one when none
  // is available.
  void CreateDefaultLookupTable();

  // Description:
  // Set/get the lookup table associated with this scalar data, if any.
  void SetLookupTable(vtkLookupTable *lut);
  vtkGetObjectMacro(LookupTable,vtkLookupTable);
  
  // Description:
  // Set/get array's name
  void SetName(const char* name);
  const char* GetName();

  // Description:
  // The active component is used when treating an array which
  // has nComponents > 1 as a scalar. Used together with 
  // Set(Get)Component
  void SetActiveComponent(int i);
  vtkGetMacro(ActiveComponent, int);

  // Description:
  // Creates an array for dataType where dataType is one of
  // VTK_BIT, VTK_CHAR, VTK_UNSIGNED_CHAR, VTK_SHORT,
  // VTK_UNSIGNED_SHORT, VTK_INT, VTK_UNSIGNED_INT, VTK_LONG,
  // VTK_UNSIGNED_LONG, VTK_FLOAT, VTK_DOUBLE.
  // Note that the data array returned has be deleted by the
  // user
  
  static vtkDataArray* CreateDataArray(int dataType);

protected:
  // Construct object with default tuple dimension (number of components) of 1.
  vtkDataArray(int numComp=1);
  ~vtkDataArray();
  vtkDataArray(const vtkDataArray&) {};
  void operator=(const vtkDataArray&) {};

  vtkLookupTable *LookupTable;

  int Size;      // allocated size of data
  int MaxId;     // maximum index inserted thus far
  int NumberOfComponents; // the number of components per tuple

  int ActiveComponent;

  static unsigned long ArrayNamePostfix;
  char* Name;
};

#endif
