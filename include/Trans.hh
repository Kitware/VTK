/*=========================================================================

  Program:   Visualization Library
  Module:    Trans.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/

// .NAME vlTransform - a general matrix transformation class
// .SECTION Description
// vlTransform maintains a stack of 4x4 transformation matrices.  A
// variety of methods are provided to manipulate the translation,
// scale, and rotation components of the matrix.  Methods operate on
// the transformation at the top of the stack.
// .SECTION Caveats
// By default the initial matrix is the identity matrix.
// .EXAMPLE XFormSph.cc

#ifndef __vlTransform_hh
#define __vlTransform_hh

#include "Object.hh"
#include "Mat4x4.hh"
#include "Points.hh"
#include "Normals.hh"
#include "Vectors.hh"

class vlTransform : public vlObject
{
 public:
  vlTransform ();
  vlTransform (const vlTransform& t);
  ~vlTransform ();
  char *GetClassName () {return "vlTransform";};
  void PrintSelf (ostream& os, vlIndent indent);
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
  void GetTranspose (vlMatrix4x4& (transpose));
  void Inverse ();
  void GetInverse ( vlMatrix4x4& inverse);
  float *GetOrientation();
  void GetPosition (float & x,float & y,float & z);
  void GetScale ( float & x, float & y, float & z);
  void SetMatrix(vlMatrix4x4& m);
  vlMatrix4x4& GetMatrix();
  void GetMatrix (vlMatrix4x4& m);
  void Concatenate (vlMatrix4x4 & matrix);
  void Multiply4x4 ( vlMatrix4x4 & a, vlMatrix4x4 & b, vlMatrix4x4 & c);
  void PointMultiply (float in[4],float out[4]);
  void MultiplyPoints(vlPoints *inPts, vlPoints *outPts);
  void MultiplyVectors(vlVectors *inVectors, vlVectors *outVectors);
  void MultiplyNormals(vlNormals *inNormals, vlNormals *outNormals);
  vlSetVector4Macro(Point,float);
  float *GetPoint();
  void GetPoint(float p[3]);

 private:
  int PreMultiplyFlag;
  int StackSize;
  vlMatrix4x4 ** Stack;
  vlMatrix4x4 ** StackBottom;
  float Point[4];
  float Orientation[3];

};

inline void vlTransform::PointMultiply (float in[4],float out[4]) 
{
  this->Stack[0]->PointMultiply(in,out);
}

#endif
