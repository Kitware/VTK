/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMatrix4x4.cxx
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
#include <stdlib.h>
#include <math.h>

#include "vtkMatrix4x4.h"
#include "vtkMath.h"

// Useful for viewing a double[16] as a double[4][4]
typedef double (*SqMatPtr)[4];

// Construct a 4x4 identity matrix.
vtkMatrix4x4::vtkMatrix4x4 ()
{
  int i,j;

  for (i = 0; i < 4; i++) 
    {
    for (j = 0; j < 4; j++)
      {
      this->Element[i][j] = 0.0;
      }
    }
  this->Element[0][0] = 
    this->Element[1][1] =
    this->Element[2][2] =
    this->Element[3][3] = 1.0;
}

// Construct a 4x4 matrix with the values which are contained in the
// argument m.
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


void vtkMatrix4x4::Zero()
{
  vtkMatrix4x4::Zero(&this->Element[0][0]);
  this->Modified ();
}

void vtkMatrix4x4::Zero(double Elements[16])
{
  SqMatPtr elem  = (SqMatPtr) Elements;
  int i,j;
  for (i = 0; i < 4; i++)
    {
    for (j = 0; j < 4; j++)
      {
      elem[i][j] = 0.0;
      }
    }
}

// Set all the elements of the matrix to the given value.
void vtkMatrix4x4::operator= (double element)
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


// Multiply this matrix by a point (in homogeneous coordinates). 
// and return the result in result. The in[4] and result[4] 
// arrays must both be allocated but they can be the same array.
void vtkMatrix4x4::MultiplyPoint(float in[4],float result[4])
{
  vtkMatrix4x4::MultiplyPoint(&this->Element[0][0], in, result);
}

void vtkMatrix4x4::MultiplyPoint(double Elements[16], float in[4],
				 float result[4])
{
  SqMatPtr elem = (SqMatPtr) Elements;

  int i;
  double v1 = in[0];
  double v2 = in[1];
  double v3 = in[2];
  double v4 = in[3];
  
  for (i = 0; i < 4; i++)
    {
    result[i] = 
      v1 * elem[i][0] +
      v2 * elem[i][1] +
      v3 * elem[i][2] +
      v4 * elem[i][3];
    }
  
}

// Multiply a point (in homogeneous coordinates) by this matrix,
// and return the result in result. The in[4] and result[4] 
// arrays must both be allocated, but they can be the same array.
void vtkMatrix4x4::PointMultiply(float in[4],float result[4])
{
  vtkMatrix4x4::PointMultiply(&this->Element[0][0], in, result);
}

void vtkMatrix4x4::PointMultiply(double Elements[16], float in[4], 
				 float result[4])
{
  SqMatPtr elem = (SqMatPtr) Elements;

  int i;
  double v1 = in[0];
  double v2 = in[1];
  double v3 = in[2];
  double v4 = in[3];
  
  for (i = 0; i < 4; i++)
    {
    result[i] = 
      v1 * elem[0][i] +
      v2 * elem[1][i] +
      v3 * elem[2][i] +
      v4 * elem[3][i];
    }
  
}


// Multiply this matrix by a point (in homogeneous coordinates). 
// and return the result in result. The in[4] and result[4] 
// arrays must both be allocated but they can be the same array.
void vtkMatrix4x4::MultiplyPoint(double in[4],double result[4])
{
  vtkMatrix4x4::MultiplyPoint(&this->Element[0][0], in, result);
}

void vtkMatrix4x4::MultiplyPoint(double Elements[16], double in[4],
				 double result[4])
{
  SqMatPtr elem = (SqMatPtr) Elements;

  int i;
  double v1 = in[0];
  double v2 = in[1];
  double v3 = in[2];
  double v4 = in[3];
  
  for (i = 0; i < 4; i++)
    {
    result[i] = 
      v1 * elem[i][0] +
      v2 * elem[i][1] +
      v3 * elem[i][2] +
      v4 * elem[i][3];
    }
  
}

// Multiply a point (in homogeneous coordinates) by this matrix,
// and return the result in result. The in[4] and result[4] 
// arrays must both be allocated, but they can be the same array.
void vtkMatrix4x4::PointMultiply(double in[4],double result[4])
{
  vtkMatrix4x4::PointMultiply(&this->Element[0][0], in, result);
}

void vtkMatrix4x4::PointMultiply(double Elements[16], double in[4],
				 double result[4])
{
  SqMatPtr elem = (SqMatPtr) Elements;

  int i;
  double v1 = in[0];
  double v2 = in[1];
  double v3 = in[2];
  double v4 = in[3];
  
  for (i = 0; i < 4; i++)
    {
    result[i] = 
      v1 * elem[0][i] +
      v2 * elem[1][i] +
      v3 * elem[2][i] +
      v4 * elem[3][i];
    }
  
}

// Matrix Inversion (adapted from Richard Carling in "Graphics Gems," 
// Academic Press, 1990).
void vtkMatrix4x4::Invert (vtkMatrix4x4 *in,vtkMatrix4x4 *out)
{
  vtkMatrix4x4::Invert(&in->Element[0][0], &out->Element[0][0]);
}

void vtkMatrix4x4::Invert(double inElements[16], double outElements[16])
{
  SqMatPtr outElem = (SqMatPtr) outElements;

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
  vtkMatrix4x4::Adjoint( inElements, outElements );

  // scale the adjoint matrix to get the inverse
  for (i=0; i<4; i++)
    {
    for(j=0; j<4; j++)
      {
      outElem[i][j] = outElem[i][j] / det;
      }
    }
}

// Compute the determinant of the matrix and return it.
float vtkMatrix4x4::Determinant (vtkMatrix4x4 *in)
{
  return vtkMatrix4x4::Determinant(&in->Element[0][0]);
}

float vtkMatrix4x4::Determinant (double Elements[16])
{
  SqMatPtr elem = (SqMatPtr) Elements;

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

// Compute adjoint of the matrix and put it into out.
void vtkMatrix4x4::Adjoint (vtkMatrix4x4 *in,vtkMatrix4x4 *out)
{
  vtkMatrix4x4::Adjoint(&in->Element[0][0], &out->Element[0][0]);
}

void vtkMatrix4x4::Adjoint(double inElements[16], double outElements[16])
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

// Set the elements of the matrix to the same values as the elements
// of the source Matrix.
void vtkMatrix4x4::DeepCopy(vtkMatrix4x4 *source)
{
  vtkMatrix4x4::DeepCopy(&this->Element[0][0], source);
  this->Modified();
}

void vtkMatrix4x4::DeepCopy(double Elements[16], vtkMatrix4x4 *source)
{
  SqMatPtr elem = (SqMatPtr) Elements;
  int i, j;
  
  for (i = 0; i < 4; ++i)
    {
    for (j = 0; j < 4; ++j)
      {
      elem[i][j] = source->Element[i][j];
      }
    }
}

// Transpose the matrix and put it into out. 
  
void vtkMatrix4x4::Transpose (vtkMatrix4x4 *in,vtkMatrix4x4 *out)
{
  vtkMatrix4x4::Transpose(&in->Element[0][0],&out->Element[0][0]);
}

void vtkMatrix4x4::Transpose (double inElements[16], double outElements[16])
{
  SqMatPtr inElem = (SqMatPtr) inElements;
  SqMatPtr outElem = (SqMatPtr) outElements;
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

