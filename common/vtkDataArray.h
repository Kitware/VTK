/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArray.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .SECTION See Also
// vtkBitArray vtkCharArray vtkUnsignedCharArray vtkShortArray
// vtkUnsignedShortArray vtkIntArray vtkUnsignedIntArray vtkLongArray
// vtkUnsignedLongArray vtkFloatArray vtkDoubleArray vtkVoidArray

#ifndef __vtkDataArray_h
#define __vtkDataArray_h

#include "vtkObject.h"

class vtkFloatArray;

class VTK_EXPORT vtkDataArray : public vtkObject 
{
public:
  // Description:
  // Construct object with default tuple dimension (number of components) of 1.
  vtkDataArray(int numComp=1);

  virtual int Allocate(const int sz, const int ext=1000) = 0;
  virtual void Initialize() = 0;
  const char *GetClassName() {return "vtkDataArray";};
  void PrintSelf(ostream& os, vtkIndent indent);

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
  int GetNumberOfTuples() {return (this->MaxId + 1)/this->NumberOfComponents;};

  // Description:
  // Get the data tuple at ith location. Return it as a pointer to an array.
  // Note: this method is not thread-safe, and the pointer is only valid
  // as long as another method incovation to a vtk object is not performed.
  virtual float *GetTuple(const int i) = 0;

  // Description:
  // Get the data tuple at ith location by filling in a user-provided array,
  // Make sure that your array is large enough to hold the NumberOfComponents
  // amount of data being returned.
  virtual void GetTuple(const int i, float * tuple) = 0;

  // Description:
  // Set the data tuple at ith location. Note that range checking or
  // memory allocation is not performed; use this method in conjunction
  // with SetNumberOfTuples() to allocate space.
  virtual void SetTuple(const int i, const float * tuple) = 0;

  // Description:
  // Insert the data tuple at ith location. Note that memory allocation
  // is performed as necessary to hold the data.
  virtual void InsertTuple(const int i, const float * tuple) = 0;

  // Description:
  // Insert the data tuple at the end of the array and return the location at
  // which the data was inserted. Memory is allocated as necessary to hold
  // the data.
  virtual int InsertNextTuple(const float * tuple) = 0;

  // Description:
  // Return the data component at the ith tuple and jth component location.
  // Note that i<NumberOfTuples and j<NumberOfComponents.
  virtual float GetComponent(const int i, const int j);

  // Description:
  // Set the data component at the ith tuple and jth component location.
  // Note that i<NumberOfTuples and j<NumberOfComponents. Make sure enough
  // memory has been allocated (use SetNumberOfTuples() and 
  // SetNumberOfComponents()).
  virtual void SetComponent(const int i, const int j, const float c);

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
		       vtkFloatArray &data);

  // Description:
  // Deep copy of data. Copies data from different data arrays even if they are 
  // different types (using floating-point exchange).
  virtual void DeepCopy(vtkDataArray& da);

  // Description:
  // Return a void pointer. For image pipeline interface and other 
  // special pointer manipulation.
  virtual void *GetVoidPointer(const int id) = 0;

  // Description:
  // Free any unneccesary memory.
  virtual void Squeeze() = 0;
  
  // Description:
  // Reset to an empty state, without freeing any memory.
  void Reset() {this->MaxId = -1;}

  // Description:
  // Return the size of the data.
  int GetSize() {return this->Size;}
  
  // Description:
  // What is the maximum id currently in the array.
  int GetMaxId() {return this->MaxId;}

  // Description:
  // By how many elements should the array increase when more memory is 
  // required.
  int GetExtend() {return this->Extend;}

protected:
  int Size;      // allocated size of data
  int MaxId;     // maximum index inserted thus far
  int Extend;    // grow array by this amount
  int NumberOfComponents; // the number of components per tuple
};

#endif
