/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Mat4x4.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkMatrix4x4 - represent and manipulate 4x4 matrices
// .SECTION Description
// vtkMatrix4x4 is a class to represent and manipulate 4x4 matrices.

#ifndef __vtkMatrix4x4_hh
#define __vtkMatrix4x4_hh

#include "Object.hh"

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
  void operator= (vtkMatrix4x4& source);
  float *operator[](const unsigned int i) {return &(Element[i][0]);};

  void Invert (vtkMatrix4x4 in,vtkMatrix4x4 & out);
  void Invert (void) { Invert(*this,*this);};

  void Transpose (vtkMatrix4x4 in,vtkMatrix4x4 & out);
  void Transpose (void) { Transpose(*this,*this);};

  void PointMultiply(float in[4], float out[4]);
  void Adjoint (vtkMatrix4x4 & in,vtkMatrix4x4 & out);
  float Determinant (vtkMatrix4x4 & in);
};
#endif
