/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMatrix4x4.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <stdlib.h>
#include <math.h>

#include "vtkMatrix4x4.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"


//----------------------------------------------------------------------------
vtkMatrix4x4* vtkMatrix4x4::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMatrix4x4");
  if(ret)
    {
    return (vtkMatrix4x4*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMatrix4x4;
}

// Useful for viewing a double[16] as a double[4][4]
typedef double (*SqMatPtr)[4];

//----------------------------------------------------------------------------
void vtkMatrix4x4::Zero(double Elements[16])
{
  SqMatPtr elem  = (SqMatPtr)Elements;
  int i,j;
  for (i = 0; i < 4; i++)
    {
    for (j = 0; j < 4; j++)
      {
      elem[i][j] = 0.0;
      }
    }
}

//----------------------------------------------------------------------------
void vtkMatrix4x4::Identity(double Elements[16])
{
  Elements[0] = Elements[5] = Elements[10] = Elements[15] = 1.0;
  Elements[1] = Elements[2] = Elements[3] = Elements[4] = 
    Elements[6] = Elements[7] = Elements[8] = Elements[9] = 
    Elements[11] = Elements[12] = Elements[13] = Elements[14] = 0.0;
}

//----------------------------------------------------------------------------
template<class T1, class T2, class T3>
static inline void vtkMatrixMultiplyPoint(T1 elem[16], T2 in[4], T3 out[4])
{
  T3 v1 = in[0];
  T3 v2 = in[1];
  T3 v3 = in[2];
  T3 v4 = in[3];

  out[0] = v1*elem[0]  + v2*elem[1]  + v3*elem[2]  + v4*elem[3];
  out[1] = v1*elem[4]  + v2*elem[5]  + v3*elem[6]  + v4*elem[7];
  out[2] = v1*elem[8]  + v2*elem[9]  + v3*elem[10] + v4*elem[11];
  out[3] = v1*elem[12] + v2*elem[13] + v3*elem[14] + v4*elem[15];
}  

//----------------------------------------------------------------------------
// Multiply this matrix by a point (in homogeneous coordinates). 
// and return the result in result. The in[4] and result[4] 
// arrays must both be allocated but they can be the same array.
void vtkMatrix4x4::MultiplyPoint(const double Elements[16], 
				 const float in[4], float result[4])
{
  vtkMatrixMultiplyPoint(Elements,in,result);
}

//----------------------------------------------------------------------------
void vtkMatrix4x4::MultiplyPoint(const double Elements[16], 
				 const double in[4], double result[4])
{
  vtkMatrixMultiplyPoint(Elements,in,result);
}

//----------------------------------------------------------------------------
void vtkMatrix4x4::PointMultiply(const double Elements[16], 
				 const float in[4], float result[4])
{
  double newElements[16];
  vtkMatrix4x4::Transpose(Elements,newElements);
  vtkMatrix4x4::MultiplyPoint(newElements,in,result);
}

//----------------------------------------------------------------------------
void vtkMatrix4x4::PointMultiply(const double in[4],double result[4])
{
  vtkMatrix4x4::PointMultiply(&this->Element[0][0], in, result);
  VTK_LEGACY_METHOD(PointMultiply,"3.2");
}

//----------------------------------------------------------------------------
void vtkMatrix4x4::PointMultiply(const double Elements[16], 
				 const double in[4], double result[4])
{
  double newElements[16];
  vtkMatrix4x4::Transpose(Elements,newElements);
  vtkMatrix4x4::MultiplyPoint(newElements,in,result);
}

//----------------------------------------------------------------------------
// Multiplies matrices a and b and stores the result in c.
void vtkMatrix4x4::Multiply4x4(const double a[16], const double b[16], 
			       double c[16])
{
  SqMatPtr aMat = (SqMatPtr) a;
  SqMatPtr bMat = (SqMatPtr) b;
  SqMatPtr cMat = (SqMatPtr) c;
  int i, k;
  double Accum[4][4];

  for (i = 0; i < 4; i++) 
    {
    for (k = 0; k < 4; k++) 
      {
      Accum[i][k] = aMat[i][0] * bMat[0][k] +
                    aMat[i][1] * bMat[1][k] +
                    aMat[i][2] * bMat[2][k] +
                    aMat[i][3] * bMat[3][k];
      }
    }

  // Copy to final dest
  for (i = 0; i < 4; i++)
    {
    cMat[i][0] = Accum[i][0];
    cMat[i][1] = Accum[i][1];
    cMat[i][2] = Accum[i][2];
    cMat[i][3] = Accum[i][3];
    }

}

//----------------------------------------------------------------------------
// Matrix Inversion (adapted from Richard Carling in "Graphics Gems," 
// Academic Press, 1990).
void vtkMatrix4x4::Invert(const double inElements[16], 
			  double outElements[16])
{
  SqMatPtr outElem = (SqMatPtr)outElements;

  // inverse( original_matrix, inverse_matrix )
  // calculate the inverse of a 4x4 matrix
  //
  //     -1     
  //     A  = ___1__ adjoint A
  //         det A
  //
  
  int i, j;
  double det;

  // calculate the 4x4 determinent
  // if the determinent is zero, 
  // then the inverse matrix is not unique.

  det = vtkMatrix4x4::Determinant(inElements);
  if ( det == 0.0 ) 
    {
    //vtkErrorMacro(<< "Singular matrix, no inverse!" );
    return;
    }

  // calculate the adjoint matrix
  vtkMatrix4x4::Adjoint(inElements, outElements );

  // scale the adjoint matrix to get the inverse
  for (i=0; i<4; i++)
    {
    for(j=0; j<4; j++)
      {
      outElem[i][j] = outElem[i][j] / det;
      }
    }
}

//----------------------------------------------------------------------------
double vtkMatrix4x4::Determinant(const double Elements[16])
{
  SqMatPtr elem = (SqMatPtr)Elements;

  double a1, a2, a3, a4, b1, b2, b3, b4, c1, c2, c3, c4, d1, d2, d3, d4;

  // assign to individual variable names to aid selecting
  //  correct elements

  a1 = elem[0][0]; b1 = elem[0][1]; 
  c1 = elem[0][2]; d1 = elem[0][3];

  a2 = elem[1][0]; b2 = elem[1][1]; 
  c2 = elem[1][2]; d2 = elem[1][3];

  a3 = elem[2][0]; b3 = elem[2][1]; 
  c3 = elem[2][2]; d3 = elem[2][3];

  a4 = elem[3][0]; b4 = elem[3][1]; 
  c4 = elem[3][2]; d4 = elem[3][3];

  return a1 * vtkMath::Determinant3x3( b2, b3, b4, c2, c3, c4, d2, d3, d4)
       - b1 * vtkMath::Determinant3x3( a2, a3, a4, c2, c3, c4, d2, d3, d4)
       + c1 * vtkMath::Determinant3x3( a2, a3, a4, b2, b3, b4, d2, d3, d4)
       - d1 * vtkMath::Determinant3x3( a2, a3, a4, b2, b3, b4, c2, c3, c4);
}

//----------------------------------------------------------------------------
void vtkMatrix4x4::Adjoint(const double inElements[16], double outElements[16])
{
  SqMatPtr inElem = (SqMatPtr) inElements;
  SqMatPtr outElem = (SqMatPtr) outElements;

  // 
  //   adjoint( original_matrix, inverse_matrix )
  // 
  //     calculate the adjoint of a 4x4 matrix
  //
  //      Let  a   denote the minor determinant of matrix A obtained by
  //           ij
  //
  //      deleting the ith row and jth column from A.
  //
  //                    i+j
  //     Let  b   = (-1)    a
  //          ij            ji
  //
  //    The matrix B = (b  ) is the adjoint of A
  //                     ij
  //
  double a1, a2, a3, a4, b1, b2, b3, b4;
  double c1, c2, c3, c4, d1, d2, d3, d4;

  // assign to individual variable names to aid
  // selecting correct values

  a1 = inElem[0][0]; b1 = inElem[0][1]; 
  c1 = inElem[0][2]; d1 = inElem[0][3];

  a2 = inElem[1][0]; b2 = inElem[1][1]; 
  c2 = inElem[1][2]; d2 = inElem[1][3];

  a3 = inElem[2][0]; b3 = inElem[2][1];
  c3 = inElem[2][2]; d3 = inElem[2][3];

  a4 = inElem[3][0]; b4 = inElem[3][1]; 
  c4 = inElem[3][2]; d4 = inElem[3][3];


  // row column labeling reversed since we transpose rows & columns

  outElem[0][0]  =   
    vtkMath::Determinant3x3( b2, b3, b4, c2, c3, c4, d2, d3, d4);
  outElem[1][0]  = 
    - vtkMath::Determinant3x3( a2, a3, a4, c2, c3, c4, d2, d3, d4);
  outElem[2][0]  =   
    vtkMath::Determinant3x3( a2, a3, a4, b2, b3, b4, d2, d3, d4);
  outElem[3][0]  = 
    - vtkMath::Determinant3x3( a2, a3, a4, b2, b3, b4, c2, c3, c4);

  outElem[0][1]  = 
    - vtkMath::Determinant3x3( b1, b3, b4, c1, c3, c4, d1, d3, d4);
  outElem[1][1]  =   
    vtkMath::Determinant3x3( a1, a3, a4, c1, c3, c4, d1, d3, d4);
  outElem[2][1]  = 
    - vtkMath::Determinant3x3( a1, a3, a4, b1, b3, b4, d1, d3, d4);
  outElem[3][1]  =   
    vtkMath::Determinant3x3( a1, a3, a4, b1, b3, b4, c1, c3, c4);
        
  outElem[0][2]  =   
    vtkMath::Determinant3x3( b1, b2, b4, c1, c2, c4, d1, d2, d4);
  outElem[1][2]  = 
    - vtkMath::Determinant3x3( a1, a2, a4, c1, c2, c4, d1, d2, d4);
  outElem[2][2]  =   
    vtkMath::Determinant3x3( a1, a2, a4, b1, b2, b4, d1, d2, d4);
  outElem[3][2]  = 
    - vtkMath::Determinant3x3( a1, a2, a4, b1, b2, b4, c1, c2, c4);
        
  outElem[0][3]  = 
    - vtkMath::Determinant3x3( b1, b2, b3, c1, c2, c3, d1, d2, d3);
  outElem[1][3]  =   
    vtkMath::Determinant3x3( a1, a2, a3, c1, c2, c3, d1, d2, d3);
  outElem[2][3]  = 
    - vtkMath::Determinant3x3( a1, a2, a3, b1, b2, b3, d1, d2, d3);
  outElem[3][3]  =   
    vtkMath::Determinant3x3( a1, a2, a3, b1, b2, b3, c1, c2, c3);
}

//----------------------------------------------------------------------------
void vtkMatrix4x4::DeepCopy(double Elements[16], const double newElements[16])
{
  for (int i = 0; i < 16; i++)
    {
    Elements[i] = newElements[i];
    }
}

//----------------------------------------------------------------------------
// Transpose the matrix and put it into out.   
void vtkMatrix4x4::Transpose(const double inElements[16], 
			      double outElements[16])
{
  SqMatPtr inElem = (SqMatPtr)inElements;
  SqMatPtr outElem = (SqMatPtr)outElements;
  int i, j;
  double temp;

  for (i=0; i<4; i++)
    {
    for(j=i; j<4; j++)
      {
      temp = inElem[i][j];
      outElem[i][j] = inElem[j][i];
      outElem[j][i] = temp;
      }
    }
}

//----------------------------------------------------------------------------
void vtkMatrix4x4::PrintSelf(ostream& os, vtkIndent indent)
{
  int i, j;

  vtkObject::PrintSelf(os, indent);

  os << indent << "Elements:\n";
  for (i = 0; i < 4; i++) 
    {
    os << indent << indent;
    for (j = 0; j < 4; j++) 
      {
      os << this->Element[i][j] << " ";
      }
    os << "\n";
    }
}

//----------------------------------------------------------------------------
// Set all the elements of the matrix to the given value.  
// This is a legacy method -- do not use
void vtkMatrix4x4::operator=(double element)
{
  int i,j;

  for (i = 0; i < 4; i++)
    {
    for (j = 0; j < 4; j++)
      {
      this->Element[i][j] = element;
      }
    }
  this->Modified ();
}

