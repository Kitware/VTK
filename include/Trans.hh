/*=========================================================================

  Program:   Visualization Library
  Module:    Trans.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#ifndef __vlTransform_hh
#define __vlTransform_hh

#include "Object.hh"
#include "Mat4x4.hh"

class vlTransform : public vlObject
// SYNOPSIS
//  A general matrix transformation class.
//  Maintains a stack of transformation matrices
//  that can be modified, pushed and popped.
//  
//  
//  
//  
{
 private:
  int PreMultiplyFlag;
  //  Specifies how a matrix will be concatenated with the current
  //  transformation matrix.
  //  If 1, the matrix will be take effect before the
  //  current transformation matrix.
  //  If 0, the matrix will take effect after the
  //  current transformation matrix.
  int StackSize;
  //  The number of transformations on the stack.
  vlMatrix4x4 ** Stack;
  //  The transformation stack. Each entry contains
  //  a pointer to a vlMatrix4x4. The top matrix on
  //  the stack is the current transformation matrix.
  vlMatrix4x4 ** StackBottom;
  //  The bottom of the stack.
  float Vector[4];
  // a temp vector used in operations
  float Orientation[3];
 public:
  vlTransform ();
  //  Constructs a transform. Sets the following defaults:
  //  preMultiplyFlag = 1
  //  stackSize = 10
  //  creates an identity matrix as the top matrix on the stack
  //  
  ~vlTransform ();
  //  Delete any matrices on the stack.
  void Identity ();
  //  Places an identity matrix on the top of the stack.
  void Pop ();
  //  Deletes the transformation on the top of the
  //  stack and sets the top to the next transformation
  //  on the stack.
  void PostMultiply ();
  //  Sets the internal state of the transform to
  //  post multiply. All matrix subsequent matrix
  //  opeartions will occur after those already represented
  //  in the current transformation matrix.
  void PreMultiply ();
  //  Sets the internal state of the transform to
  //  pre multiply. All matrix subsequent matrix
  //  opeartions will occur before those already represented
  //  in the current transformation matrix.
  void Push ();
  //  Pushes the current transformation matrix onto the
  //  transformation stack.
  void RotateX ( float angle);
  //  Creates an x rotation matrix and concatenates it with 
  //  the current transformation matrix.
  void RotateY ( float angle);
  //  Creates a y rotation matrix and concatenates it with 
  //  the current transformation matrix.
  void RotateZ (float angle);
  //  Creates a z rotation matrix and concatenates it with 
  //  the current transformation matrix.
  void RotateWXYZ ( float angle, float x, float y, float z);
  //  Creates a matrix that rotates angle degrees about an axis
  //  through the origin and x, y, z. Then concatenates
  //  this matrix with the current transformation matrix.
  void Scale ( float x, float y, float z);
  void Translate ( float x, float y, float z);
  void Transpose ();
  //  Transpose the current transformation matrix.
  void GetTranspose (vlMatrix4x4& (transpose));
  //  Returns the transpose of the current transformation
  //  matrix.
  void Inverse ();
  //  Inverts the current transformation matrix.
  void GetInverse ( vlMatrix4x4& inverse);
  //  Returns the inverse of the current transformation
  //  matrix.
  float *GetOrientation();
  //  Returns the equivalent x, y, z rotations that
  //  will reproduce the orientaion of the current
  //  tranformation matrix if it was created with:
  //  Identity ();
  //  RotateZ (z);
  //  RotateX (x);
  //  RotateY (y);
  void GetPosition (float & x,float & y,float & z);
  //  Returns the position entry of the current
  //  transformation matrix.
  void GetScale ( float & x, float & y, float & z);
  //  Returns the x, y, z scale of the current transformation
  //  matrix.
  vlMatrix4x4 & GetMatrix ();
  //  Returns the current transformation matrix.
  void GetMatrix (vlMatrix4x4 & ctm);
  //  Returns the current transformation matrix.
  char *GetClassName () {return "vlTransform";};
  void PrintSelf (ostream& os, vlIndent indent);
  void Concatenate (vlMatrix4x4 & matrix);
  //  Concatenates matrix with the current transformation
  //  matrix. If the PreMultiply flag is 1, allpies the
  //  matrix before the current transformation matrix.
  //  Otherwise, applies it before. The resulting
  //  matrix becomes the current transformation matrix.
  //  
  //  
  void Multiply4x4 ( vlMatrix4x4 & a, vlMatrix4x4 & b, vlMatrix4x4 & c);
  //  Multiplies two 4 x 4 matrices and produces a
  //  4 x 4 matrix. The output matrix can be the
  //  same as either of the two input matrices.
  void VectorMultiply (float in[4],float out[4]) 
     {this->Stack[0]->VectorMultiply(in,out);};
  //  Multiplies two 4 x 4 matrices and produces a
  //  4 x 4 matrix. The output matrix can be the
  //  same as either of the two input matrices.
  vlSetVector4Macro(Vector,float);
  float *GetVector();
};

#endif
