/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMatrix4x4.cxx
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
#include <stdlib.h>
#include <math.h>

#include "vtkMatrix4x4.h"
#include "vtkMath.h"

// Description:
// Construct a 4x4 identity matrix.
vtkMatrix4x4::vtkMatrix4x4 ()
{
  int i,j;

  for (i = 0; i < 4; i++) 
    {
    for (j = 0; j < 4; j++)
      {
      if ( i == j ) this->Element[i][j] = 1.0;
      else this->Element[i][j] = 0.0;
      }
    }
}

vtkMatrix4x4::vtkMatrix4x4(const vtkMatrix4x4& m)
{
  int i,j;

  for (i = 0; i < 4; i++) 
    {
    for (j = 0; j < 4; j++)
      {
      this->Element[i][j] = m.Element[i][j];
      }
    }
}

// Description:
// Set all the elements of the matrix to the given value.
void vtkMatrix4x4::operator= (float element)
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

// Description:
// Multiply this matrix by a point (in homogeneous coordinates). 
// and return the result in result. The in[4] and result[4] 
// arrays must both be allocated but they can be the same array.
void vtkMatrix4x4::MultiplyPoint(float in[4],float result[4])
{
  int i;
  float v1 = in[0];
  float v2 = in[1];
  float v3 = in[2];
  float v4 = in[3];
  
  for (i = 0; i < 4; i++)
    {
    result[i] = 
      v1 * this->Element[i][0] +
      v2 * this->Element[i][1] +
      v3 * this->Element[i][2] +
      v4 * this->Element[i][3];
    }
  
}

// Description:
// Multiply a point (in homogeneous coordinates) by this matrix,
// and return the result in result. The in[4] and result[4] 
// arrays must both be allocated, but they can be the same array.
void vtkMatrix4x4::PointMultiply(float in[4],float result[4])
{
  int i;
  float v1 = in[0];
  float v2 = in[1];
  float v3 = in[2];
  float v4 = in[3];
  
  for (i = 0; i < 4; i++)
    {
    result[i] = 
      v1 * this->Element[0][i] +
      v2 * this->Element[1][i] +
      v3 * this->Element[2][i] +
      v4 * this->Element[3][i];
    }
  
}

// Description:
// Matrix Inversion (adapted from Richard Carling in "Graphics Gems," 
// Academic Press, 1990).
void vtkMatrix4x4::Invert (vtkMatrix4x4 in,vtkMatrix4x4 & out)
{

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

  det = in.Determinant(in);
  if ( det == 0.0 ) 
    {
    vtkErrorMacro(<< "Singular matrix, no inverse!" );
    return;
    }

  // calculate the adjoint matrix
  this->Adjoint( in, out );

  // scale the adjoint matrix to get the inverse
  for (i=0; i<4; i++)
    for(j=0; j<4; j++)
      out.Element[i][j] = out.Element[i][j] / det;
}

// Description:
// Compute the determinant of the matrix and return it.
float vtkMatrix4x4::Determinant (vtkMatrix4x4 & in)
{
  double a1, a2, a3, a4, b1, b2, b3, b4, c1, c2, c3, c4, d1, d2, d3, d4;

  // assign to individual variable names to aid selecting
  //  correct elements

  a1 = in.Element[0][0]; b1 = in.Element[0][1]; 
  c1 = in.Element[0][2]; d1 = in.Element[0][3];

  a2 = in.Element[1][0]; b2 = in.Element[1][1]; 
  c2 = in.Element[1][2]; d2 = in.Element[1][3];

  a3 = in.Element[2][0]; b3 = in.Element[2][1]; 
  c3 = in.Element[2][2]; d3 = in.Element[2][3];

  a4 = in.Element[3][0]; b4 = in.Element[3][1]; 
  c4 = in.Element[3][2]; d4 = in.Element[3][3];

  return a1 * vtkMath::Determinant3x3( b2, b3, b4, c2, c3, c4, d2, d3, d4)
       - b1 * vtkMath::Determinant3x3( a2, a3, a4, c2, c3, c4, d2, d3, d4)
       + c1 * vtkMath::Determinant3x3( a2, a3, a4, b2, b3, b4, d2, d3, d4)
       - d1 * vtkMath::Determinant3x3( a2, a3, a4, b2, b3, b4, c2, c3, c4);
}

// Description:
// Compute adjoint of the matrix and put it into out.
void vtkMatrix4x4::Adjoint (vtkMatrix4x4 & in,vtkMatrix4x4 & out)
{
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

  a1 = in.Element[0][0]; b1 = in.Element[0][1]; 
  c1 = in.Element[0][2]; d1 = in.Element[0][3];

  a2 = in.Element[1][0]; b2 = in.Element[1][1]; 
  c2 = in.Element[1][2]; d2 = in.Element[1][3];

  a3 = in.Element[2][0]; b3 = in.Element[2][1];
  c3 = in.Element[2][2]; d3 = in.Element[2][3];

  a4 = in.Element[3][0]; b4 = in.Element[3][1]; 
  c4 = in.Element[3][2]; d4 = in.Element[3][3];


  // row column labeling reversed since we transpose rows & columns

  out.Element[0][0]  =   
    vtkMath::Determinant3x3( b2, b3, b4, c2, c3, c4, d2, d3, d4);
  out.Element[1][0]  = 
    - vtkMath::Determinant3x3( a2, a3, a4, c2, c3, c4, d2, d3, d4);
  out.Element[2][0]  =   
    vtkMath::Determinant3x3( a2, a3, a4, b2, b3, b4, d2, d3, d4);
  out.Element[3][0]  = 
    - vtkMath::Determinant3x3( a2, a3, a4, b2, b3, b4, c2, c3, c4);

  out.Element[0][1]  = 
    - vtkMath::Determinant3x3( b1, b3, b4, c1, c3, c4, d1, d3, d4);
  out.Element[1][1]  =   
    vtkMath::Determinant3x3( a1, a3, a4, c1, c3, c4, d1, d3, d4);
  out.Element[2][1]  = 
    - vtkMath::Determinant3x3( a1, a3, a4, b1, b3, b4, d1, d3, d4);
  out.Element[3][1]  =   
    vtkMath::Determinant3x3( a1, a3, a4, b1, b3, b4, c1, c3, c4);
        
  out.Element[0][2]  =   
    vtkMath::Determinant3x3( b1, b2, b4, c1, c2, c4, d1, d2, d4);
  out.Element[1][2]  = 
    - vtkMath::Determinant3x3( a1, a2, a4, c1, c2, c4, d1, d2, d4);
  out.Element[2][2]  =   
    vtkMath::Determinant3x3( a1, a2, a4, b1, b2, b4, d1, d2, d4);
  out.Element[3][2]  = 
    - vtkMath::Determinant3x3( a1, a2, a4, b1, b2, b4, c1, c2, c4);
        
  out.Element[0][3]  = 
    - vtkMath::Determinant3x3( b1, b2, b3, c1, c2, c3, d1, d2, d3);
  out.Element[1][3]  =   
    vtkMath::Determinant3x3( a1, a2, a3, c1, c2, c3, d1, d2, d3);
  out.Element[2][3]  = 
    - vtkMath::Determinant3x3( a1, a2, a3, b1, b2, b3, d1, d2, d3);
  out.Element[3][3]  =   
    vtkMath::Determinant3x3( a1, a2, a3, b1, b2, b3, c1, c2, c3);
}

vtkMatrix4x4& vtkMatrix4x4::operator= (const vtkMatrix4x4& source)
{
  int i, j;

  for (i = 0; i < 4; i++) 
    {
    for (j = 0; j < 4; j++) 
      {
      this->Element[i][j] = source.Element[i][j];
      }
    }
  return *this;
}

// Description:
// Transpose the matrix and put it into out. 
void vtkMatrix4x4::Transpose (vtkMatrix4x4 in,vtkMatrix4x4 & out)
{
  int i, j;
  float temp;

  for (i=0; i<4; i++)
    {
    for(j=i; j<4; j++)
      {
      temp = in.Element[i][j];
      out.Element[i][j] = in.Element[j][i];
      out.Element[j][i] = temp;
      }
    }
}

void vtkMatrix4x4::PrintSelf (ostream& os, vtkIndent indent)
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

