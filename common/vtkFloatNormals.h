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

class vtkFloatNormals : public vtkNormals
{
public:
  vtkFloatNormals() {};
  vtkFloatNormals(const vtkFloatNormals& fn) {this->N = fn.N;};
  vtkFloatNormals(const int sz, const int ext=1000):N(3*sz,3*ext){};
  int Allocate(const int sz, const int ext=1000) {return this->N.Allocate(3*sz,3*ext);};
  void Initialize() {this->N.Initialize();};
  char *GetClassName() {return "vtkFloatNormals";};

  // vtkNormal interface
  vtkNormals *MakeObject(int sze, int ext=1000);
  char *GetDataType() {return "float";};
  int GetNumberOfNormals() {return (N.GetMaxId()+1)/3;};
  void Squeeze() {this->N.Squeeze();};
  float *GetNormal(int i) {return this->N.GetPtr(3*i);};
  void GetNormal(int i,float n[3]) {this->vtkNormals::GetNormal(i,n);};
  void SetNormal(int i, float n[3]);
  void InsertNormal(int i, float n[3]);
  int InsertNextNormal(float n[3]);

  // miscellaneous
  float *GetPtr(const int id);
  float *WritePtr(const int id, const int number);
  void WrotePtr();
  vtkFloatNormals &operator=(const vtkFloatNormals& fn);
  void operator+=(const vtkFloatNormals& fn);
  void Reset() {this->N.Reset();};

protected:
  vtkFloatArray N;
};

// Description:
// Get pointer to array of data starting at data position "id".
inline float *vtkFloatNormals::GetPtr(const int id)
{
  return this->N.GetPtr(id);
}

// Description:
// Get pointer to data array. Useful for direct writes of data. MaxId is 
// bumped by number (and memory allocated if necessary). Id is the 
// location you wish to write into; number is the number of normals to 
// write. Use the method WrotePtr() to mark completion of write.
inline float *vtkFloatNormals::WritePtr(const int id, const int number)
{
  return this->N.WritePtr(id,3*number);
}

// Description:
// Terminate direct write of data. Although dummy routine now, reserved for
// future use.
inline void vtkFloatNormals::WrotePtr() {}


inline void vtkFloatNormals::SetNormal(int i, float n[3]) 
{
  i*=3; 
  this->N[i]=n[0]; 
  this->N[i+1]=n[1]; 
  this->N[i+2]=n[2];
}

inline void vtkFloatNormals::InsertNormal(int i, float n[3]) 
{
  this->N.InsertValue(3*i+2, n[2]);
  this->N[3*i] =  n[0];
  this->N[3*i+1] =  n[1];
}

inline int vtkFloatNormals::InsertNextNormal(float n[3]) 
{
  int id = this->N.GetMaxId() + 3;
  this->N.InsertValue(id,n[2]);
  this->N[id-2] = n[0];
  this->N[id-1] = n[1];
  return id/3;
}

#endif
