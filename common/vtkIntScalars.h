/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIntScalars.h
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
// .NAME vtkIntScalars - integer representation of scalar data
// .SECTION Description
// vtkIntScalars is a concrete implementation of vtkScalars. Scalars are
// represented using integer values.

#ifndef __vtkIntScalars_h
#define __vtkIntScalars_h

#include "vtkScalars.h"
#include "vtkShortArray.h"

class vtkIntScalars : public vtkScalars 
{
public:
  vtkIntScalars() {};
  vtkIntScalars(const vtkIntScalars& ss) {this->S = ss.S;};
  vtkIntScalars(const int sz, const int ext=1000):S(sz,ext){};
  int Allocate(const int sz, const int ext=1000) {return this->S.Allocate(sz,ext);};
  void Initialize() {this->S.Initialize();};
  char *GetClassName() {return "vtkIntScalars";};

  // vtkScalar interface
  vtkScalars *MakeObject(int sze, int ext=1000);
  char *GetDataType() {return "int";};
  int GetNumberOfScalars() {return (this->S.GetMaxId()+1);};
  void Squeeze() {this->S.Squeeze();};
  float GetScalar(int i) {return (float)this->S[i];};
  void SetScalar(int i, int s) {this->S[i] = s;};
  void SetScalar(int i, float s) {this->S[i] = (int)s;};
  void InsertScalar(int i, float s) {S.InsertValue(i,(int)s);};
  void InsertScalar(int i, int s) {S.InsertValue(i,s);};
  int InsertNextScalar(int s) {return S.InsertNextValue(s);};
  int InsertNextScalar(float s) {return S.InsertNextValue((int)s);};
  void GetScalars(vtkIdList& ptIds, vtkFloatScalars& fs);
  void GetScalars(int p1, int p2, vtkFloatScalars& fs);

  // miscellaneous
  int *GetPtr(const int id);
  void *GetVoidPtr(const int id);
  int *WritePtr(const int id, const int number);
  void WrotePtr();
  vtkIntScalars &operator=(const vtkIntScalars& is);
  void operator+=(const vtkIntScalars& is) {this->S += is.S;};
  void Reset() {this->S.Reset();};

protected:
  vtkIntArray S;
};

// Description:
// Get pointer to array of data starting at data position "id".
inline int *vtkIntScalars::GetPtr(const int id)
{
  return this->S.GetPtr(id);
}

// Description:
// Get a void pointer to array of data starting at data position "id".
inline void *vtkIntScalars::GetVoidPtr(const int id)
{
  return (void *)(this->S.GetPtr(id));
}

// Description:
// Get pointer to data array. Useful for direct writes of data. MaxId is 
// bumped by number (and memory allocated if necessary). Id is the 
// location you wish to write into; number is the number of scalars to 
// write. Use the method WrotePtr() to mark completion of write.
inline int *vtkIntScalars::WritePtr(const int id, const int number)
{
  return this->S.WritePtr(id,number);
}

// Description:
// Terminate direct write of data. Although dummy routine now, reserved for
// future use.
inline void vtkIntScalars::WrotePtr() {}

#endif
