/*=========================================================================

  Program:   Visualization Library
  Module:    Mat4x4.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include <stdlib.h>
#include <math.h>

#include "Mat4x4.hh"

vlMatrix4x4::vlMatrix4x4 ()
{
}
void vlMatrix4x4::operator= (float element)
{
  int i,j;

  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) {
      this->Element[i][j] = element;
    }
  }
  this->Modified ();
}
void vlMatrix4x4::Invert (vlMatrix4x4 in,vlMatrix4x4 & out)
{
// Matrix Inversion
// by Richard Carling
// from "Graphics Gems", Academic Press, 1990

#define SMALL_NUMBER	1.e-9

// inverse( original_matrix, inverse_matrix )
// calculate the inverse of a 4x4 matrix

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
  if ( fabs( det ) < SMALL_NUMBER) {
    cerr << "Singular matrix, no inverse! Determinant = ";
    cerr << det;
    det = 0.0;
    return;
  }

  // calculate the adjoint matrix

  this->Adjoint( in, out );

  // scale the adjoint matrix to get the inverse

  for (i=0; i<4; i++)
    for(j=0; j<4; j++)
      out.Element[i][j] = out.Element[i][j] / det;
}
/*
 * double = det2x2( double a, double b, double c, double d )
 * 
 * calculate the determinent of a 2x2 matrix.
 */

static double det2x2( double a, double b, double c, double d)
{
    double ans;
    ans = a * d - b * c;
    return ans;
}

/*
 * double = det3x3(  a1, a2, a3, b1, b2, b3, c1, c2, c3 )
 * 
 * calculate the determinent of a 3x3 matrix
 * in the form
 *
 *     | a1,  b1,  c1 |
 *     | a2,  b2,  c2 |
 *     | a3,  b3,  c3 |
 */

static double det3x3( double a1, double a2, double a3, double b1, double b2, double b3, double c1, double c2, double c3 )
{
    double ans;

    ans = a1 * det2x2( b2, b3, c2, c3 )
        - b1 * det2x2( a2, a3, c2, c3 )
        + c1 * det2x2( a2, a3, b2, b3 );
    return ans;
}

float vlMatrix4x4::Determinant (vlMatrix4x4 & in)
/*
 * double = det4x4( matrix )
 * 
 * calculate the determinent of a 4x4 matrix.
 */
{
  double ans;
  double a1, a2, a3, a4, b1, b2, b3, b4, c1, c2, c3, c4, d1, d2, d3, d4;
  double det3x3();

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

  ans = a1 * det3x3( b2, b3, b4, c2, c3, c4, d2, d3, d4)
      - b1 * det3x3( a2, a3, a4, c2, c3, c4, d2, d3, d4)
      + c1 * det3x3( a2, a3, a4, b2, b3, b4, d2, d3, d4)
      - d1 * det3x3( a2, a3, a4, b2, b3, b4, c2, c3, c4);
  return ans;
}

void vlMatrix4x4::Adjoint (vlMatrix4x4 & in,vlMatrix4x4 & out)
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
///

  double a1, a2, a3, a4, b1, b2, b3, b4;
  double c1, c2, c3, c4, d1, d2, d3, d4;
  double det3x3();

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

  out.Element[0][0]  =   det3x3( b2, b3, b4, c2, c3, c4, d2, d3, d4);
  out.Element[1][0]  = - det3x3( a2, a3, a4, c2, c3, c4, d2, d3, d4);
  out.Element[2][0]  =   det3x3( a2, a3, a4, b2, b3, b4, d2, d3, d4);
  out.Element[3][0]  = - det3x3( a2, a3, a4, b2, b3, b4, c2, c3, c4);
        
  out.Element[0][1]  = - det3x3( b1, b3, b4, c1, c3, c4, d1, d3, d4);
  out.Element[1][1]  =   det3x3( a1, a3, a4, c1, c3, c4, d1, d3, d4);
  out.Element[2][1]  = - det3x3( a1, a3, a4, b1, b3, b4, d1, d3, d4);
  out.Element[3][1]  =   det3x3( a1, a3, a4, b1, b3, b4, c1, c3, c4);
        
  out.Element[0][2]  =   det3x3( b1, b2, b4, c1, c2, c4, d1, d2, d4);
  out.Element[1][2]  = - det3x3( a1, a2, a4, c1, c2, c4, d1, d2, d4);
  out.Element[2][2]  =   det3x3( a1, a2, a4, b1, b2, b4, d1, d2, d4);
  out.Element[3][2]  = - det3x3( a1, a2, a4, b1, b2, b4, c1, c2, c4);
        
  out.Element[0][3]  = - det3x3( b1, b2, b3, c1, c2, c3, d1, d2, d3);
  out.Element[1][3]  =   det3x3( a1, a2, a3, c1, c2, c3, d1, d2, d3);
  out.Element[2][3]  = - det3x3( a1, a2, a3, b1, b2, b3, d1, d2, d3);
  out.Element[3][3]  =   det3x3( a1, a2, a3, b1, b2, b3, c1, c2, c3);
}

void vlMatrix4x4::operator= (vlMatrix4x4& source)
{
  int i, j;

  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) {
      this->Element[i][j] = source.Element[i][j];
    }
  }
}

void vlMatrix4x4::PrintSelf (ostream& os, vlIndent indent)
{
  int i, j;

  if (this->ShouldIPrint (vlMatrix4x4::GetClassName()))
    {
	  vlObject::PrintSelf(os, indent);

	  os << indent << "Elements:\n";
          for (i = 0; i < 4; i++) {
	    cout << indent << indent;
            for (j = 0; j < 4; j++) {
              cout << this->Element[i][j] << " ";
            }
	    cout << "\n";
          }
    }
}

