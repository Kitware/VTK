/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTensor.h
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
// .NAME vtkTensor - supporting class to enable assignment and referencing of tensors
// .SECTION Description
// vtkTensor is a floating point representation of an nxn tensor. vtkTensor 
// provides methods for assignment and reference of tensor components. It 
// does it in such a way as to minimize data copying.
//
// .SECTION Caveats
// vtkTensor performs its operations using pointer reference. You are 
// responsible for supplying data storage (if necessary) if local copies 
// of data are being made.

#ifndef __vtkTensor_h
#define __vtkTensor_h

#include "vtkObject.h"

class VTK_EXPORT vtkTensor : public vtkObject
{
public:
  vtkTensor();
  static vtkTensor *New() {return new vtkTensor;};
  const char *GetClassName() {return "vtkTensor";};

  // Description:
  // Initialize tensor components to 0.0.
  void Initialize();

  // Description:
  // Get the tensor component (i,j).
  float GetComponent(int i, int j) {return this->T[i+3*j];};

  // Description:
  // Set the value of the tensor component (i,j).
  void SetComponent(int i, int j, float v) {this->T[i+3*j] = v;};

  // Description:
  // Add to the value of the tensor component at location (i,j).
  void AddComponent(int i, int j, float v) { this->T[i+3*j] += v;};

  // Description:
  // Return column vector from tensor. (Assumes 2D matrix form and 0-offset.)
  float *GetColumn(int j) { return this->T + 3*j;};

  // Description:
  // Deep copy of one tensor to another tensor.
  void DeepCopy(vtkTensor &t);

  // Description:
  // Provide float * type conversion.
  operator float*() {return this->T;};

  float *T;

protected: 
  float Storage[9];
};

inline void vtkTensor::Initialize()
{
  for (int j=0; j<3; j++)
    {
    for (int i=0; i<3; i++)
      {
      this->T[i+j*3] = 0.0;
      }
    }
}

inline void vtkTensor::DeepCopy(vtkTensor &t)
{
  for (int j=0; j < 3; j++)
    {
    for (int i=0; i < 3; i++)
      {
      this->T[i+3*j] = t.T[i+3*j];
      }
    }
}

#endif
