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
  vtkFieldData();
  ~vtkFieldData();
  static vtkFieldData *New() {return new vtkFieldData;};
  void Initialize();
  int Allocate(const int sz, const int ext=1000);
  const char *GetClassName() {return "vtkFieldData";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // satisfy vtkDataObject API
  vtkFieldData *MakeObject();

  // managing data arrays into the field - most efficient way
  void SetNumberOfArrays(int num);
  void SetArray(int i, vtkDataArray *);
  int GetNumberOfArrays();
  vtkDataArray *GetArray(int i);
  void SetArrayName(int i,char *name);
  char *GetArrayName(int i);

  // managing components/tuples in the field
  int GetNumberOfComponents();
  int GetNumberOfTuples();
  void SetNumberOfTuples(const int number);
  float *GetTuple(const int i);
  void GetTuple(const int i, float tuple[]);
  void SetTuple(const int i, const float tuple[]);
  void InsertTuple(const int i, const float tuple[]);
  int InsertNextTuple(const float tuple[]);
  float GetComponent(const int i, const int j);
  void SetComponent(const int i, const int j, const float c);
  void InsertComponent(const int i, const int j, const float c);

  // Copy a field in various ways
  void DeepCopy(vtkFieldData& da);
  void ShallowCopy(vtkFieldData& da);

  // special operators
  void Squeeze();
  void Reset();

  // Get a field from a list of ids
  void GetField(vtkIdList& ptId, vtkFieldData& f);

protected:
  int NumberOfArrays;
  vtkDataArray **Data;
  char **ArrayNames;

  int TupleSize; //used for type conversion
  float *Tuple;
};

#endif
