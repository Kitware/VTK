/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFloatTensors.h
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
// .NAME vtkFloatTensors - floating point representation of tensor data
// .SECTION Description
// vtkFloatTensors is a concrete implementation of vtkTensors. Tensor values
// are represented using float values.

#ifndef __vtkFloatTensors_h
#define __vtkFloatTensors_h

#include "vtkTensors.h"
#include "vtkFloatArray.h"

class VTK_EXPORT vtkFloatTensors : public vtkTensors
{
public:
  vtkFloatTensors();
  vtkFloatTensors(const vtkFloatTensors& ft);
  vtkFloatTensors(int sz, int d=3, int ext=1000);
  ~vtkFloatTensors();

  int Allocate(const int sz, const int dim=3, const int ext=1000);
  void Initialize() {this->T->Initialize();};
  static vtkFloatTensors *New() {return new vtkFloatTensors;};
  char *GetClassName() {return "vtkFloatTensors";};

  // vtkTensors interface
  vtkTensors *MakeObject(int sze, int d=3, int ext=1000);
  char *GetDataType() {return "float";};
  int GetNumberOfTensors();
  void Squeeze() {this->T->Squeeze();};
  vtkTensor *GetTensor(int i);
  void GetTensor(int i,vtkTensor &t) {this->vtkTensors::GetTensor(i,t);};
  void SetNumberOfTensors(int number);
  void SetTensor(int i, vtkTensor *t);
  void InsertTensor(int i, vtkTensor *t);
  int InsertNextTensor(vtkTensor *t);

  // miscellaneous
  float *GetPtr(const int id);
  float *WritePtr(const int id, const int number);
  vtkFloatTensors &operator=(const vtkFloatTensors& ft);
  void operator+=(const vtkFloatTensors& ft) {*(this->T) += *(ft.T);};
  void Reset() {this->T->Reset();};

protected:
  vtkFloatArray *T;
};

// Description:
// Get pointer to array of data starting at data position "id".
inline float *vtkFloatTensors::GetPtr(const int id)
{
  return this->T->GetPtr(id);
}

// Description:
// Get pointer to data array. Useful for direct writes of data. MaxId is 
// bumped by number (and memory allocated if necessary). Id is the 
// location you wish to write into; number is the number of tensors to 
// write. 
// Make sure the dimension of the tensor is set prior to issuing this call.
inline float *vtkFloatTensors::WritePtr(const int id, const int number)
{
  return this->T->WritePtr(id,this->Dimension*this->Dimension*number);
}

inline int vtkFloatTensors::Allocate(const int sz, const int dim,const int ext) 
{
  return this->T->Allocate(dim*dim*sz,dim*dim*ext);
}

inline int vtkFloatTensors::GetNumberOfTensors() 
{
  return (this->T->GetMaxId()+1)/(this->Dimension*this->Dimension);
}

inline void vtkFloatTensors::SetNumberOfTensors(int number)
{
  this->T->SetNumberOfValues(this->Dimension*this->Dimension*number);
}


#endif
