/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFloatScalars.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkFloatScalars - floating point representation of scalar data
// .SECTION Description
// vtkFloatScalars is a concrete implementation of vtkScalars. Scalars are
// represented using float values.

#ifndef __vtkFloatScalars_h
#define __vtkFloatScalars_h

#include "vtkScalars.h"
#include "vtkFloatArray.h"

class VTK_EXPORT vtkFloatScalars : public vtkScalars 
{
public:
  vtkFloatScalars();
  vtkFloatScalars(const vtkFloatScalars& fs);
  vtkFloatScalars(const int sz, const int ext=1000);
  ~vtkFloatScalars();

  int Allocate(const int sz, const int ext=1000) {return this->S->Allocate(sz,ext);};
  void Initialize() {this->S->Initialize();};
  static vtkFloatScalars *New() {return new vtkFloatScalars;};
  char *GetClassName() {return "vtkFloatScalars";};

  // vtkScalar interface
  vtkScalars *MakeObject(int sze, int ext=1000);
  char *GetDataType() {return "float";};
  int GetNumberOfScalars() {return (this->S->GetMaxId()+1);};
  void Squeeze() {this->S->Squeeze();};
  float GetScalar(int i) {return this->S->GetValue(i);};
  void SetNumberOfScalars(int number);
  void SetScalar(int i, float s) {this->S->SetValue(i,s);};
  void InsertScalar(int i, float s) {S->InsertValue(i,s);};
  int InsertNextScalar(float s) {return S->InsertNextValue(s);};
  void GetScalars(vtkIdList& ptIds, vtkFloatScalars& fs);
  void GetScalars(int p1, int p2, vtkFloatScalars& fs);

  // miscellaneous
  float *GetPointer(const int id);
  void *GetVoidPtr(const int id);
  float *WritePointer(const int id, const int number);
  vtkFloatScalars &operator=(const vtkFloatScalars& fs);
  void operator+=(const vtkFloatScalars& fs) {*(this->S) += *(fs.S);};
  void Reset() {this->S->Reset();};

  // Used by vtkImageToStructuredPoints (Proper length array is up to user!)
  vtkSetReferenceCountedObjectMacro(S, vtkFloatArray);
  vtkGetObjectMacro(S, vtkFloatArray);

protected:
  vtkFloatArray *S;
};

inline void vtkFloatScalars::SetNumberOfScalars(int number)
{
  this->S->SetNumberOfValues(number);
}

// Description:
// Get pointer to array of data starting at data position "id".
inline float *vtkFloatScalars::GetPointer(const int id)
{
  return this->S->GetPointer(id);
}

// Description:
// Get pointer to array of data starting at data position "id" and return as
// a void pointer.
inline void *vtkFloatScalars::GetVoidPtr(const int id)
{
  return (void *)(this->S->GetPointer(id));
}

// Description:
// Get pointer to data array. Useful for direct writes of data. MaxId is 
// bumped by number (and memory allocated if necessary). Id is the 
// location you wish to write into; number is the number of scalars to 
// write. 
inline float *vtkFloatScalars::WritePointer(const int id, const int number)
{
  return this->S->WritePointer(id,number);
}

#endif
