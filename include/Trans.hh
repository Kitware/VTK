/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Trans.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/

// .NAME vtkTransform - a general matrix transformation class
// .SECTION Description
// vtkTransform maintains a stack of 4x4 transformation matrices.  A
// variety of methods are provided to manipulate the translation,
// scale, and rotation components of the matrix.  Methods operate on
// the transformation at the top of the stack.
// .SECTION Caveats
// By default the initial matrix is the identity matrix.
// .EXAMPLE XFormSph.cc

#ifndef __vtkTransform_hh
#define __vtkTransform_hh

#include "Object.hh"
#include "Mat4x4.hh"
#include "Points.hh"
#include "Normals.hh"
#include "Vectors.hh"

class vtkTransform : public vtkObject
{
 public:
  vtkTransform ();
  vtkTransform (const vtkTransform& t);
  ~vtkTransform ();
  char *GetClassName () {return "vtkTransform";};
  void PrintSelf (ostream& os, vtkIndent indent);
  void Identity ();
  void Pop ();
  void PostMultiply ();
  void PreMultiply ();
  void Push ();
  void RotateX ( float angle);
  void RotateY ( float angle);
  void RotateZ (float angle);
  void RotateWXYZ ( float angle, float x, float y, float z);
  void Scale ( float x, float y, float z);
  void Translate ( float x, float y, float z);
  void Transpose ();
  void GetTranspose (vtkMatrix4x4& transpose);
  void Inverse ();
  void GetInverse ( vtkMatrix4x4& inverse);
  float *GetOrientation();
  void GetPosition (float & x,float & y,float & z);
  void GetScale ( float & x, float & y, float & z);
  void SetMatrix(vtkMatrix4x4& m);
  vtkMatrix4x4& GetMatrix();
  void GetMatrix (vtkMatrix4x4& m);
  void Concatenate (vtkMatrix4x4 & matrix);
  void Multiply4x4 ( vtkMatrix4x4 & a, vtkMatrix4x4 & b, vtkMatrix4x4 & c);
  void PointMultiply (float in[4],float out[4]);
  void MultiplyPoints(vtkPoints *inPts, vtkPoints *outPts);
  void MultiplyVectors(vtkVectors *inVectors, vtkVectors *outVectors);
  void MultiplyNormals(vtkNormals *inNormals, vtkNormals *outNormals);
  vtkSetVector4Macro(Point,float);
  float *GetPoint();
  void GetPoint(float p[3]);

 private:
  int PreMultiplyFlag;
  int StackSize;
  vtkMatrix4x4 ** Stack;
  vtkMatrix4x4 ** StackBottom;
  float Point[4];
  float Orientation[3];

};

inline void vtkTransform::PointMultiply (float in[4],float out[4]) 
{
  this->Stack[0]->PointMultiply(in,out);
}

#endif
