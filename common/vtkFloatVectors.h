/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFloatVectors.h
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
// .NAME vtkFloatVectors - floating point representation of 3D vectors
// .SECTION Description
// vtkFloatVectors is a concrete implementation of vtkVectors. Vectors are
// represented using float values.

#ifndef __vtkFloatVectors_h
#define __vtkFloatVectors_h

#include "vtkVectors.h"
#include "vtkFloatArray.h"

class VTK_EXPORT vtkFloatVectors : public vtkVectors
{
public:
  vtkFloatVectors();
  vtkFloatVectors(const vtkFloatVectors& fv);
  vtkFloatVectors(const int sz, const int ext=1000);
  ~vtkFloatVectors();

  int Allocate(const int sz, const int ext=1000) {return this->V->Allocate(3*sz,3*ext);};
  void Initialize() {this->V->Initialize();};
  static vtkFloatVectors *New() {return new vtkFloatVectors;};
  const char *GetClassName() {return "vtkFloatVectors";};

  // vtkVector interface
  vtkVectors *MakeObject(int sze, int ext=1000);
  char *GetDataType() {return "float";};
  int GetNumberOfVectors() {return (V->GetMaxId()+1)/3;};
  void Squeeze() {this->V->Squeeze();};
  float *GetVector(int i) {return this->V->GetPointer(3*i);};
  void GetVector(int i,float v[3]) {this->vtkVectors::GetVector(i,v);};
  void SetNumberOfVectors(int number);
  void SetVector(int i, float v[3]);
  void InsertVector(int i, float v[3]);
  int InsertNextVector(float v[3]);

  // miscellaneous
  float *GetPointer(const int id);
  float *WritePointer(const int id, const int number);
  vtkFloatVectors &operator=(const vtkFloatVectors& fv);
  void operator+=(const vtkFloatVectors& fv){*(this->V) += *(fv.V);};
  void Reset() {this->V->Reset();};

protected:
  vtkFloatArray *V;
};

// Description:
// Get pointer to array of data starting at data position "id".
inline float *vtkFloatVectors::GetPointer(const int id)
{
  return this->V->GetPointer(3*id);
}

// Description:
// Get pointer to data array. Useful for direct writes of data. MaxId is 
// bumped by number (and memory allocated if necessary). Id is the 
// location you wish to write into; number is the number of vectors to 
// write. 
inline float *vtkFloatVectors::WritePointer(const int id, const int number)
{
  return this->V->WritePointer(3*id,3*number);
}

inline void vtkFloatVectors::SetNumberOfVectors(int number)
{
  this->V->SetNumberOfValues(3*number);
}

inline void vtkFloatVectors::SetVector(int id, float v[3]) 
{
  id *= 3;
  this->V->SetValue(id++, v[0]);
  this->V->SetValue(id++, v[1]);
  this->V->SetValue(id,   v[2]);
}

inline void vtkFloatVectors::InsertVector(int i, float v[3]) 
{
  float *ptr = this->V->WritePointer(i*3,3);

  *ptr++ = v[0];
  *ptr++ = v[1];
  *ptr   = v[2];
}

inline int vtkFloatVectors::InsertNextVector(float v[3]) 
{
  int id = this->V->GetMaxId() + 1;
  float *ptr = this->V->WritePointer(id,3);

  *ptr++ = v[0];
  *ptr++ = v[1];
  *ptr   = v[2];

  return (id+2)/3;
}

#endif
