/*=========================================================================

  Program:   Visualization Library
  Module:    Trans.hh
  Language:  C++
  Date:      5/8/94
  Version:   1.6

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#ifndef __vlTransform_hh
#define __vlTransform_hh

#include "Object.hh"
#include "Mat4x4.hh"

// .NAME vlTransform - a general matrix transformation class
// .LIBRARY common
// .HEADER Visualization Library
// .INCLUDE common/Trans.hh
// .FILE Trans.hh
// .FILE Trans.cc

class vlTransform : public vlObject
{
 private:
  int PreMultiplyFlag;
  int StackSize;
  vlMatrix4x4 ** Stack;
  vlMatrix4x4 ** StackBottom;
  float Vector[4];
  float Orientation[3];
 public:
  vlTransform ();
  vlTransform (const vlTransform& t);
  ~vlTransform ();
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
  vlMatrix4x4 & GetMatrix ();
  void GetMatrix (vlMatrix4x4 & ctm);
  char *GetClassName () {return "vlTransform";};
  void PrintSelf (ostream& os, vlIndent indent);
  void Concatenate (vlMatrix4x4 & matrix);
  void Multiply4x4 ( vlMatrix4x4 & a, vlMatrix4x4 & b, vlMatrix4x4 & c);
  void VectorMultiply (float in[4],float out[4]) 
     {this->Stack[0]->VectorMultiply(in,out);};
  vlSetVector4Macro(Vector,float);
  float *GetVector();
};

#endif
