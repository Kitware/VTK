/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFloatNormals.h
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
// .NAME vtkFloatNormals - floating point representation of 3D normals
// .SECTION Description
// vtkFloatNormals is a concrete implementation of vtkNormals. Normals are
// represented using float values.

#ifndef __vtkFloatNormals_h
#define __vtkFloatNormals_h

#include "vtkNormals.h"
#include "vtkFloatArray.h"

class VTK_EXPORT vtkFloatNormals : public vtkNormals
{
public:
  vtkFloatNormals();
  vtkFloatNormals(const vtkFloatNormals& fn);
  vtkFloatNormals(const int sz, const int ext=1000);
  ~vtkFloatNormals();

  int Allocate(const int sz, const int ext=1000) {return this->N->Allocate(3*sz,3*ext);};
  void Initialize() {this->N->Initialize();};
  static vtkFloatNormals *New() {return new vtkFloatNormals;};
  char *GetClassName() {return "vtkFloatNormals";};

  // vtkNormal interface
  vtkNormals *MakeObject(int sze, int ext=1000);
  char *GetDataType() {return "float";};
  int GetNumberOfNormals() {return (N->GetMaxId()+1)/3;};
  void Squeeze() {this->N->Squeeze();};
  float *GetNormal(int i) {return this->N->GetPointer(3*i);};
  void GetNormal(int i,float n[3]) {this->vtkNormals::GetNormal(i,n);};
  void SetNumberOfNormals(int number);
  void SetNormal(int id, float n[3]);
  void InsertNormal(int i, float n[3]);
  int InsertNextNormal(float n[3]);

  // miscellaneous
  float *GetPointer(const int id);
  float *WritePointer(const int id, const int number);
  vtkFloatNormals &operator=(const vtkFloatNormals& fn);
  void operator+=(const vtkFloatNormals& fn);
  void Reset() {this->N->Reset();};

protected:
  vtkFloatArray *N;
};

// Description:
// Get pointer to array of data starting at data position "id".
inline float *vtkFloatNormals::GetPointer(const int id)
{
  return this->N->GetPointer(3*id);
}

// Description:
// Get pointer to data array. Useful for direct writes of data. MaxId is 
// bumped by number (and memory allocated if necessary). Id is the 
// location you wish to write into; number is the number of normals to 
// write. 
inline float *vtkFloatNormals::WritePointer(const int id, const int number)
{
  return this->N->WritePointer(3*id,3*number);
}

inline void vtkFloatNormals::SetNumberOfNormals(int number)
{
  this->N->SetNumberOfValues(3*number);
}

inline void vtkFloatNormals::SetNormal(int id, float n[3]) 
{
  id *= 3;
  this->N->SetValue(id++, n[0]);
  this->N->SetValue(id++, n[1]);
  this->N->SetValue(id,   n[2]);
}

inline void vtkFloatNormals::InsertNormal(int i, float n[3]) 
{
  float *ptr = this->N->WritePointer(i*3,3);

  *ptr++ = n[0];
  *ptr++ = n[1];
  *ptr   = n[2];
}

inline int vtkFloatNormals::InsertNextNormal(float n[3]) 
{
  int id = this->N->GetMaxId() + 1;
  float *ptr = this->N->WritePointer(id,3);

  *ptr++ = n[0];
  *ptr++ = n[1];
  *ptr   = n[2];

  return (id+2)/3;
}

#endif
