/*=========================================================================

  Program:   Visualization Library
  Module:    Mat4x4.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#ifndef __vlMatrix4x4_hh
#define __vlMatrix4x4_hh

#include "Object.hh"

class vlMatrix4x4 : public vlObject
{
 public:
  float Element[4][4];
  //  A 4 x 4 matrix.
  vlMatrix4x4 ();
  vlMatrix4x4 (const vlMatrix4x4& m);
  void operator= (float element);
  void operator= (vlMatrix4x4& source);
  float *operator[](const unsigned int i) {return &(Element[i][0]);};

  //  Calculate the inverse of in and
  //  return it in out.
  void Invert (vlMatrix4x4 in,vlMatrix4x4 & out);
  void Invert (void) { Invert(*this,*this);};

  //  Calculate the transpose of in and
  //  return it in out.
  void Transpose (vlMatrix4x4 in,vlMatrix4x4 & out);
  void Transpose (void) { Transpose(*this,*this);};

  void VectorMultiply(float in[4], float out[4]);
  void Adjoint (vlMatrix4x4 & in,vlMatrix4x4 & out);
  float Determinant (vlMatrix4x4 & in);
  char *GetClassName () {return "vlMatrix4x4";};
  void PrintSelf (ostream& os, vlIndent indent);
};
#endif
