/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMatrix4x4.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkMatrix4x4 - represent and manipulate 4x4 transformation matrices
// .SECTION Description
// vtkMatrix4x4 is a class to represent and manipulate 4x4 matrices. 
// Specifically, it is designed to work on 4x4 transformation matrices
// found in 3D rendering using homogeneous coordinates [x y z w].

// .SECTION See Also
// vtkTransform

#ifndef __vtkMatrix4x4_h
#define __vtkMatrix4x4_h

#include "vtkObject.h"

class VTK_EXPORT vtkMatrix4x4 : public vtkObject
{
  // Some of the methods in here have a corresponding static (class)
  // method taking a pointer to 16 doubles that constitutes a user
  // supplied matrix. This allows C++ clients to allocate double arrays
  // on the stack and manipulate them using vtkMatrix4x4 methods. 
  // This is an alternative to allowing vtkMatrix4x4 instances to be
  // created on the stack (which is frowned upon) or doing lots of
  // temporary heap allocation within vtkTransform and vtkActor methods,
  // which is inefficient.

 public:
  double Element[4][4];

  // Description:
  // Construct a 4x4 identity matrix.
  static vtkMatrix4x4 *New();

  vtkTypeMacro(vtkMatrix4x4,vtkObject);
  void PrintSelf (vtkOstream& os, vtkIndent indent);
  
  // Description:
  // Set the elements of the matrix to the same values as the elements
  // of the source Matrix.
  void DeepCopy(vtkMatrix4x4 *source);
//BTX
  static void DeepCopy(double Elements[16], vtkMatrix4x4 *source);
//ETX

  // Description:
  // Non-static member function. Assigns *from* elements array
  void DeepCopy(const double Elements[16]);

  // Description:
  // Set all of the elements to zero.
  void Zero();
//BTX
  static void Zero(double Elements[16]);
//ETX  

  // Description:
  // Set equal to Identity matrix
  void Identity();
//BTX
  static void Identity(double Elements[16]);
//ETX  


  // Description:
  // Matrix Inversion (adapted from Richard Carling in "Graphics Gems," 
  // Academic Press, 1990).
  void Invert(vtkMatrix4x4 *in,vtkMatrix4x4 *out);
  void Invert(void) { this->Invert(this,this);};
//BTX
  static void Invert(const double inElements[16], double outElements[16]);
//ETX


  // Description:
  // Transpose the matrix and put it into out. 
  void Transpose(vtkMatrix4x4 *in,vtkMatrix4x4 *out);
  void Transpose(void) { this->Transpose(this,this);};
//BTX
  static void Transpose(const double inElements[16], double outElements[16]);
//ETX

  // Description:
  // Multiply a homogenous coordinate by this matrix, i.e. out = A*in.
  // The in[4] and out[4] can be the same array.
  void MultiplyPoint(const float in[4], float out[4]);
  void MultiplyPoint(const double in[4], double out[4]);

//BTX
  static void MultiplyPoint(const double Elements[16], 
			    const float in[4], float out[4]);
  static void MultiplyPoint(const double Elements[16], 
			    const double in[4], double out[4]);
//ETX

  // Description:
  // For use in Java, Python or Tcl.  The default MultiplyPoint() uses
  // a single-precision point.
  float *MultiplyPoint(const float in[4]) {
    return this->MultiplyFloatPoint(in); };
  float *MultiplyFloatPoint(const float in[4]) {
    this->MultiplyPoint(in,this->FloatPoint); return this->FloatPoint; } 
  double *MultiplyDoublePoint(const double in[4]) {
    this->MultiplyPoint(in,this->DoublePoint); return this->DoublePoint; } 

  // Description:
  // Multiplies matrices a and b and stores the result in c.
  static void Multiply4x4(vtkMatrix4x4 *a, vtkMatrix4x4 *b, vtkMatrix4x4 *c);
//BTX
  static void Multiply4x4(const double a[16], const double b[16], 
			  double c[16]);
//ETX

  // Description:
  // Compute adjoint of the matrix and put it into out.
  void Adjoint(vtkMatrix4x4 *in,vtkMatrix4x4 *out);
//BTX
  static void Adjoint(const double inElements[16], double outElements[16]);
//ETX

  // Description:
  // Compute the determinant of the matrix and return it.
  float Determinant(vtkMatrix4x4 *in);
//BTX
  static float Determinant(const double Elements[16]);
//ETX

  // Description:
  // Sets the element i,j in the matrix.
  void SetElement(int i, int j, double value);
//BTX
  void SetElement(int i, int j, float value);
//ETX
  // Description:
  // Returns the element i,j from the matrix.
  double GetElement(int i, int j) const {return this->Element[i][j];};

  // Description: 
  // For legacy compatibility. Do not use.
  double *operator[](const unsigned int i) {return &(this->Element[i][0]);};
  const double *operator[](unsigned int i) const
    { return &(this->Element[i][0]); }  
  void operator= (double element);
  void Adjoint(vtkMatrix4x4 &in,vtkMatrix4x4 &out){this->Adjoint(&in,&out);}
  float Determinant(vtkMatrix4x4 &in) {return this->Determinant(&in);}
  void Invert(vtkMatrix4x4 &in,vtkMatrix4x4 &out)
    {this->Invert(&in,&out);}
  void Transpose(vtkMatrix4x4 &in,vtkMatrix4x4 &out)
    {this->Transpose(&in,&out);}
  void PointMultiply(const float in[4], float out[4]);
  void PointMultiply(const double in[4], double out[4]);
//BTX
  static void PointMultiply(const double Elements[16], 
			    const float in[4], float out[4]);
  static void PointMultiply(const double Elements[16], 
			    const double in[4], double out[4]);
//ETX


protected:
  vtkMatrix4x4();
  ~vtkMatrix4x4() {};
  vtkMatrix4x4(const vtkMatrix4x4&);
  void operator= (const vtkMatrix4x4& source);
  
  float FloatPoint[4];
  double DoublePoint[4];
};

inline void vtkMatrix4x4::SetElement (int i, int j, float value)
{
  if (this->Element[i][j] != value)
    {
    this->Element[i][j] = value;
    this->Modified();
    }
}
inline void vtkMatrix4x4::SetElement (int i, int j, double value)
{
  if (this->Element[i][j] != value)
    {
    this->Element[i][j] = value;
    this->Modified();
    }
}

#endif

