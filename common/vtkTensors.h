/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTensors.h
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
// .NAME vtkVectors - represent and manipulate 3x3 tensors
// .SECTION Description
// vtkTensors represents 3x3 tensors. The data model for vtkTensors is an 
// array of 3x3 matrices accessible by (point or cell) id.

#ifndef __vtkTensors_h
#define __vtkTensors_h

#include "vtkAttributeData.h"
#include "vtkTensor.h"

class vtkIdList;
class vtkTensors;

class VTK_EXPORT vtkTensors : public vtkAttributeData
{
public:
  vtkTensors(int dataType=VTK_FLOAT);
  static vtkTensors *New(int dataType=VTK_FLOAT) {return new vtkTensors(dataType);};
  const char *GetClassName() {return "vtkTensors";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // overload vtkAttributeData API
  vtkAttributeData *MakeObject();

  // generic access to tensor data
  int GetNumberOfTensors();
  vtkTensor *GetTensor(int id);
  void GetTensor(int id, vtkTensor& t);
  void SetNumberOfTensors(int number);
  void SetTensor(int id, vtkTensor *t);
  void InsertTensor(int id, vtkTensor *t);
  void InsertTensor(int id, float t11, float t12, float t13, 
                    float t21, float t22, float t23, 
                    float t31, float t32, float t33);
  int InsertNextTensor(vtkTensor *t);
  int InsertNextTensor(float t11, float t12, float t13, 
                       float t21, float t22, float t23, 
                       float t31, float t32, float t33);

  void GetTensors(vtkIdList& ptId, vtkTensors& fv);

protected:
  vtkTensor T;

};

// Description:
// Create a copy of this object.
inline vtkAttributeData *vtkTensors::MakeObject()
{
  return new vtkTensors(this->GetDataType());
}

// Description:
// Return number of tensors in array.
inline int vtkTensors::GetNumberOfTensors()
{
  return this->Data->GetNumberOfTuples();
}

// Description:
// Return a pointer to a float tensor for a specific id.
inline vtkTensor *vtkTensors::GetTensor(int id)
{
  this->T.T = this->Data->GetTuple(id);
  return &(this->T);
}

// Description:
// Specify the number of tensors for this object to hold. Does an
// allocation as well as setting the MaxId ivar. Used in conjunction with
// SetTensor() method for fast insertion.
inline void vtkTensors::SetNumberOfTensors(int number)
{
  this->Data->SetNumberOfComponents(9);
  this->Data->SetNumberOfTuples(number);
}

// These include files are placed here so that if Tensors.h is included 
// all other classes necessary for compilation are also included. 
#include "vtkIdList.h"

#endif

