/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMatrix4x4.hh
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
// .NAME vtkMatrix4x4 - represent and manipulate 4x4 transformation matrices
// .SECTION Description
// vtkMatrix4x4 is a class to represent and manipulate 4x4 matrices. 
// Specifically, it is designed to work on 4x4 transformation matrices
// found in 3D rendering using homogeneous coordinates [x y z w].

// .SECTION See Also
// vtkTransform

#ifndef __vtkMatrix4x4_hh
#define __vtkMatrix4x4_hh

#include "vtkObject.hh"

class vtkMatrix4x4 : public vtkObject
{
 public:
  float Element[4][4];
  //  A 4 x 4 matrix.
  vtkMatrix4x4 ();
  vtkMatrix4x4 (const vtkMatrix4x4& m);
  char *GetClassName () {return "vtkMatrix4x4";};
  void PrintSelf (ostream& os, vtkIndent indent);

  void operator= (float element);
  vtkMatrix4x4& operator= (const vtkMatrix4x4& source);
  float *operator[](const unsigned int i) {return &(Element[i][0]);};

  void Invert (vtkMatrix4x4 in,vtkMatrix4x4 & out);
  void Invert (void) { Invert(*this,*this);};

  void Transpose (vtkMatrix4x4 in,vtkMatrix4x4 & out);
  void Transpose (void) { Transpose(*this,*this);};

  void MultiplyPoint(float in[4], float out[4]);
  void PointMultiply(float in[4], float out[4]);
  void Adjoint (vtkMatrix4x4 & in,vtkMatrix4x4 & out);
  float Determinant (vtkMatrix4x4 & in);
  void SetElement(int i, int j, float value);
  float GetElement(int i, int j);
};

// Description:
// Sets the element i,j in the matrix.
inline void vtkMatrix4x4::SetElement (int i, int j, float value)
{
  if (this->Element[i][j] != value)
    {
    this->Element[i][j] = value;
    this->Modified ();
    }
}

// Description:
// Returns the element i,j from the matrix.
inline float vtkMatrix4x4::GetElement (int i, int j)
{
  return this->Element[i][j];
}
#endif
