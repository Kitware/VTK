/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFloatPoints.h
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
// .NAME vtkFloatPoints - floating point representation of 3D points
// .SECTION Description
// vtkFloatPoints is a concrete implementation of vtkPoints. Points are
// represented using float values.

#ifndef __vtkFloatPoints_h
#define __vtkFloatPoints_h

#include "vtkPoints.h"
#include "vtkFloatArray.h"

class vtkFloatPoints : public vtkPoints
{
public:
  vtkFloatPoints() {};
  vtkFloatPoints(const vtkFloatPoints& fp) {this->P = fp.P;};
  vtkFloatPoints(const int sz, const int ext=1000):P(3*sz,3*ext){};
  int Allocate(const int sz, const int ext=1000) {return this->P.Allocate(3*sz,3*ext);};
  void Initialize() {this->P.Initialize();};
  char *GetClassName() {return "vtkFloatPoints";};

  // vtkPoint interface
  vtkPoints *MakeObject(int sze, int ext=1000);
  char *GetDataType() {return "float";};
  int GetNumberOfPoints() {return (P.GetMaxId()+1)/3;};
  void Squeeze() {this->P.Squeeze();};
  float *GetPoint(int id) {return this->P.GetPtr(3*id);};
  void GetPoint(int id, float x[3]);
  void SetPoint(int id, float x[3]);
  void InsertPoint(int id, float x[3]);
  int InsertNextPoint(float x[3]);
  void GetPoints(vtkIdList& ptId, vtkFloatPoints& fp);

  // miscellaneous
  float *GetPtr(const int id);
  float *WritePtr(const int id, const int number);
  void WrotePtr();
  vtkFloatPoints &operator=(const vtkFloatPoints& fp);
  void operator+=(const vtkFloatPoints& fp) {this->P += fp.P;};
  void Reset() {this->P.Reset();};

protected:
  vtkFloatArray P;
};

// Description:
// Get pointer to array of data starting at data position "id".
inline float *vtkFloatPoints::GetPtr(const int id)
{
  return this->P.GetPtr(id);
}

// Description:
// Get pointer to data array. Useful for direct writes of data. MaxId is 
// bumped by number (and memory allocated if necessary). Id is the 
// location you wish to write into; number is the number of scalars to 
// write. Use the method WrotePtr() to mark completion of write.
inline float *vtkFloatPoints::WritePtr(const int id, const int number)
{
  return this->P.WritePtr(id,3*number);
}

// Description:
// Terminate direct write of data. Although dummy routine now, reserved for
// future use.
inline void vtkFloatPoints::WrotePtr() {}

inline void vtkFloatPoints::GetPoint(int id, float x[3])
{
  float *p=this->P.GetPtr(3*id); 
  x[0] = p[0]; x[1] = p[1]; x[2] = p[2];
}

inline void vtkFloatPoints::SetPoint(int id, float x[3]) 
{
  float *ptr = this->P.WritePtr(id*3,3);

  *ptr++ = x[0];
  *ptr++ = x[1];
  *ptr   = x[2];
}

inline void vtkFloatPoints::InsertPoint(int id, float x[3])
{
  float *ptr = this->P.WritePtr(id*3,3);

  *ptr++ = x[0];
  *ptr++ = x[1];
  *ptr   = x[2];
}

inline int vtkFloatPoints::InsertNextPoint(float x[3])
{
  int id = this->P.GetMaxId() + 1;
  float *ptr = this->P.WritePtr(id,3);

  *ptr++ = x[0];
  *ptr++ = x[1];
  *ptr   = x[2];

  return (id+2)/3;
}

#endif
