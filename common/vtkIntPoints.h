/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIntPoints.h
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
// .NAME vtkIntPoints - integer representation of 3D points
// .SECTION Description
// vtkIntPoints is a concrete implementation of vtkPoints. Points are 
// represented using integer values.

#ifndef __vtkIntPoints_h
#define __vtkIntPoints_h

#include "vtkPoints.h"
#include "vtkIntArray.h"

class VTK_EXPORT vtkIntPoints : public vtkPoints
{
public:
  vtkIntPoints();
  ~vtkIntPoints();
  vtkIntPoints(const vtkIntPoints& fp);
  vtkIntPoints(const int sz, const int ext=1000);
  int Allocate(const int sz, const int ext=1000) {return this->P->Allocate(3*sz,3*ext);};
  void Initialize() {this->P->Initialize();};
  static vtkIntPoints *New() {return new vtkIntPoints;};
  char *GetClassName() {return "vtkIntPoints";};

  // vtkPoint interface
  vtkPoints *MakeObject(int sze, int ext=1000);
  char *GetDataType() {return "int";};
  int GetNumberOfPoints() {return (this->P->GetMaxId()+1)/3;};
  void Squeeze() {this->P->Squeeze();};
  float *GetPoint(int i);
  void GetPoint(int id, float x[3]);
  void SetNumberOfPoints(int number);
  void SetPoint(int id, int x[3]);
  void SetPoint(int id, float x[3]);
  void InsertPoint(int i, int x[3]);
  void InsertPoint(int i, float x[3]);
  int InsertNextPoint(int x[3]);
  int InsertNextPoint(float x[3]);
  void GetPoints(vtkIdList& ptId, vtkFloatPoints& fp);

  // miscellaneous
  int *GetPointer(const int id);
  int *WritePointer(const int id, const int number);
  vtkIntPoints &operator=(const vtkIntPoints& fp);
  void operator+=(const vtkIntPoints& fp) {*(this->P) += *(fp.P);};
  void Reset() {this->P->Reset();};

protected:
  vtkIntArray *P;
};

// Description:
// Get pointer to array of data starting at data position "id".
inline int *vtkIntPoints::GetPointer(const int id)
{
  return this->P->GetPointer(3*id);
}

// Description:
// Get pointer to data array. Useful for direct writes of data. MaxId is 
// bumped by number (and memory allocated if necessary). Id is the 
// location you wish to write into; number is the number of points to 
// write. 
inline int *vtkIntPoints::WritePointer(const int id, const int number)
{
  return this->P->WritePointer(3*id,3*number);
}

inline void vtkIntPoints::GetPoint(int id, float x[3])
{
  int *p=this->P->GetPointer(3*id); 
  x[0] = (float)p[0]; x[1] = (float)p[1]; x[2] = (float)p[2];
}

inline void vtkIntPoints::SetNumberOfPoints(int number)
{
  this->P->SetNumberOfValues(3*number);
}

inline void vtkIntPoints::SetPoint(int id, float x[3]) 
{
  id *= 3;
  this->P->SetValue(id++, (int)x[0]);
  this->P->SetValue(id++, (int)x[1]);
  this->P->SetValue(id,   (int)x[2]);
}

inline void vtkIntPoints::SetPoint(int id, int x[3]) 
{
  id *= 3;
  this->P->SetValue(id++, x[0]);
  this->P->SetValue(id++, x[1]);
  this->P->SetValue(id,   x[2]);
}

inline void vtkIntPoints::InsertPoint(int i, int x[3]) 
{
  int *ptr = this->P->WritePointer(i*3,3);

  *ptr++ = x[0];
  *ptr++ = x[1];
  *ptr   = x[2];
}

inline void vtkIntPoints::InsertPoint(int i, float x[3]) 
{
  int *ptr = this->P->WritePointer(i*3,3);

  *ptr++ = (int) x[0];
  *ptr++ = (int) x[1];
  *ptr   = (int) x[2];
}

inline int vtkIntPoints::InsertNextPoint(int x[3]) 
{
  int id = this->P->GetMaxId() + 1;
  int *ptr = this->P->WritePointer(id,3);

  *ptr++ = x[0];
  *ptr++ = x[1];
  *ptr   = x[2];

  return (id+2)/3;
}

inline int vtkIntPoints::InsertNextPoint(float x[3]) 
{
  int id = this->P->GetMaxId() + 1;
  int *ptr = this->P->WritePointer(id,3);

  *ptr++ = (int) x[0];
  *ptr++ = (int) x[1];
  *ptr   = (int) x[2];

  return (id+2)/3;
}

#endif
