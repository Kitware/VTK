/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFieldData.h
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
// There are two ways of manipulating and interfacing to fields. You can do it
// generically by manipulating components/tuples via a float-type data exchange,
// or you can do it by grabbing the arrays and manipulating them directly. The
// former is simpler but performs type conversion, which is bad if your data has
// non-castable types like (void) pointers, or you lose information as a result 
// of the cast. The, more efficient method means managing each array in the field.
// Using this method you can create faster, more efficient algorithms that do not
// lose information.

// .SECTION See Also
// vtkDataArray vtkAttribueData vtkPointData vtkCellData

#ifndef __vtkFieldData_h
#define __vtkFieldData_h

#include "vtkDataArray.h"
#include "vtkIdList.h"

class VTK_EXPORT vtkFieldData : public vtkReferenceCount
{
public:

// Description:
// Construct object with no data initially.
  vtkFieldData();

  ~vtkFieldData();
  static vtkFieldData *New() {return new vtkFieldData;};

// Description:
// Release all data but do not delete object.
  void Initialize();


// Description:
// Allocate data for each array.
  int Allocate(const int sz, const int ext=1000);

  const char *GetClassName() {return "vtkFieldData";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // satisfy vtkDataObject API

// Description:
// Virtual constructor creates a field with the same number of data arrays and
// types of data arrays, but the arrays contain nothing.
  vtkFieldData *MakeObject();


  // managing data arrays into the field - most efficient way

// Description:
// Set the number of arrays used to define the field.
  void SetNumberOfArrays(int num);


// Description:
// Set an array to define the field.
  void SetArray(int i, vtkDataArray *);


// Description:
// 
  int GetNumberOfArrays();


// Description:
// Return the ith array in the field. A NULL is returned if the index i is out
// if range.
  vtkDataArray *GetArray(int i);

  void SetArrayName(int i,char *name);
  char *GetArrayName(int i);

  // managing components/tuples in the field

// Description:
// Get the number of components in the field. This is determined by adding
// up the components in each non-NULL array.
  int GetNumberOfComponents();


// Description:
// Get the number of tuples in the field.
  int GetNumberOfTuples();


// Description:
// Set the number of tuples for each data array in the field.
  void SetNumberOfTuples(const int number);


// Description:
// Return a tuple consisting of a concatentation of all data from all
// the different arrays. Note that everything is converted to and from
// float values.
  float *GetTuple(const int i);


// Description:
// Copy the ith tuple value into a user provided tuple array. Make
// sure that you've allocated enough space for the copy.
  void GetTuple(const int i, float * tuple);


// Description:
// Set the tuple value at the ith location. Set operations
// mean that no range chaecking is performed, so they're faster.
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
// Get the component value at the ith tuple (or row) and jth component (or column).
  float GetComponent(const int i, const int j);


// Description:
// Set the component value at the ith tuple (or row) and jth component (or column).
// Range checking is not performed, so set the object up properly before invoking.
  void SetComponent(const int i, const int j, const float c);


// Description:
// Insert the component value at the ith tuple (or row) and jth component (or column).
// Range checking is performed and memory allocated as necessary o hold data.
  void InsertComponent(const int i, const int j, const float c);


  // Copy a field in various ways

// Description:
// Copy a field by creating new data arrays (i.e., duplicate storage).
  void DeepCopy(vtkFieldData& da);


// Description:
// Copy a field by reference counting the data arrays.
  void ShallowCopy(vtkFieldData& da);


  // special operators

// Description:
// Squeezes each data array in the field (Squeeze() reclaims unused memory.)
  void Squeeze();


// Description:
// Resets each data array in the field (Reset() does not release memory but
// it makes the arrays look like they are empty.)
  void Reset();


  // Get a field from a list of ids

// Description:
// Get a field from a list of ids. Supplied field f should have same types 
// and number of data arrays as this one (i.e., like MakeObject() returns).
  void GetField(vtkIdList& ptId, vtkFieldData& f);


protected:
  int NumberOfArrays;
  vtkDataArray **Data;
  char **ArrayNames;

  int TupleSize; //used for type conversion
  float *Tuple;
};

#endif
