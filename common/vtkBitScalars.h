/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBitScalars.h
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
// .NAME vtkBitScalars - packed bit (0/1) representation of scalar data
// .SECTION Description
// vtkBitScalars is a concrete implementation of vtkScalars. Scalars are
// represented using a packed bit array. Only possible scalar values are
// (0/1).

#ifndef __vtkBitScalars_h
#define __vtkBitScalars_h

#include "vtkScalars.h"
#include "vtkBitArray.h"

class VTK_EXPORT vtkBitScalars : public vtkScalars 
{
public:
  vtkBitScalars();
  vtkBitScalars(const vtkBitScalars& cs);
  vtkBitScalars(const int sz, const int ext=1000);
  ~vtkBitScalars();

  int Allocate(const int sz, const int ext=1000) {return this->S->Allocate(sz,ext);};
  void Initialize() {this->S->Initialize();};
  static vtkBitScalars *New() {return new vtkBitScalars;};
  const char *GetClassName() {return "vtkBitScalars";};

  // vtkScalar interface
  vtkScalars *MakeObject(int sze, int ext=1000);
  char *GetDataType() {return "bit";};
  int GetNumberOfScalars() {return (this->S->GetMaxId()+1);};
  void Squeeze() {this->S->Squeeze();};
  float GetScalar(int i) {return (float)this->S->GetValue(i);};
  void SetNumberOfScalars(int number);
  void SetScalar(int i, int s) {this->S->SetValue(i,s);};
  void SetScalar(int i, float s) {this->S->SetValue(i,(int)s);};
  void InsertScalar(int i, float s) {S->InsertValue(i,(int)s);};
  void InsertScalar(int i, int s) {S->InsertValue(i,s);};
  int InsertNextScalar(int s) {return S->InsertNextValue(s);};
  int InsertNextScalar(float s) {return S->InsertNextValue((int)s);};
  void GetScalars(vtkIdList& ptIds, vtkFloatScalars& fs);

  // miscellaneous
  unsigned char *GetPointer(const int id);
  unsigned char *WritePointer(const int id, const int number);
  vtkBitScalars &operator=(const vtkBitScalars& cs);
  void operator+=(const vtkBitScalars& cs) {*(this->S) += *(cs.S);};
  void Reset() {this->S->Reset();};

protected:
  vtkBitArray *S;
};

inline void vtkBitScalars::SetNumberOfScalars(int number)
{
  this->S->SetNumberOfValues(number);
}

// Description:
// Get pointer to array of data starting at data position "id".
inline unsigned char *vtkBitScalars::GetPointer(const int id)
{
  return this->S->GetPointer(id);
}

// Description:
// Get pointer to data array. Useful for direct writes of data. MaxId is 
// bumped by number (and memory allocated if necessary). Id is the 
// location you wish to write into; number is the number of scalars to 
// write. 
inline unsigned char *vtkBitScalars::WritePointer(const int id, const int number)
{
  return this->S->WritePointer(id,number);
}

#endif
