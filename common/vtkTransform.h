/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransform.h
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

// .NAME vtkTransform - a general matrix transformation class
// .SECTION Description
// vtkTransform maintains a stack of 4x4 transformation matrices.  A
// variety of methods are provided to manipulate the translation,
// scale, and rotation components of the matrix.  Methods operate on
// the matrix at the top of the stack. Many objects, such as vtkActor and
// vtkCamera, use this class for performing their matrix operations.
// It is very important to realize that this class performs all of
// its operations in a right handed coordinate system with right
// handed rotations. Some other graphics libraries use left handed 
// coordinate systems and rotations.

// .SECTION Caveats
// By default the initial matrix is the identity matrix.
// .EXAMPLE XFormSph.cc
// .SECTION see also
// vtkMatrix4x4 vtkTransformCollection vtkTransformFilter
// vtkTransformPolyFilter

#ifndef __vtkTransform_h
#define __vtkTransform_h

#include "vtkObject.h"
#include "vtkMatrix4x4.h"
#include "vtkPoints.h"
#include "vtkNormals.h"
#include "vtkVectors.h"

class vtkTransform : public vtkObject
{
 public:
  vtkTransform ();
  vtkTransform (const vtkTransform& t);
  ~vtkTransform ();
  char *GetClassName () {return "vtkTransform";};
  void PrintSelf (ostream& os, vtkIndent indent);
  vtkTransform &operator=(const vtkTransform &t);

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
  void Transpose();
  void GetTranspose (vtkMatrix4x4& transpose);
  void Inverse();
  void GetInverse(vtkMatrix4x4& inverse);
  float *GetOrientation();
  void GetOrientation(float& rx, float& ry, float& rz);
  float *GetOrientationWXYZ();  
  float *GetPosition();
  void GetPosition (float& x, float& y, float& z);
  float *GetScale();
  void GetScale (float& sx, float& sy, float& sz);
  void SetMatrix(vtkMatrix4x4& m);
  vtkMatrix4x4& GetMatrix();
  void GetMatrix (vtkMatrix4x4& m);
  void Concatenate (vtkMatrix4x4 & matrix);
  void Multiply4x4 ( vtkMatrix4x4 & a, vtkMatrix4x4 & b, vtkMatrix4x4 & c);
  void PointMultiply (float in[4],float out[4]);
  void MultiplyPoint (float in[4],float out[4]);
  void MultiplyPoints(vtkPoints *inPts, vtkPoints *outPts);
  void MultiplyVectors(vtkVectors *inVectors, vtkVectors *outVectors);
  void MultiplyNormals(vtkNormals *inNormals, vtkNormals *outNormals);
  vtkSetVector4Macro(Point,float);
  float *GetPoint();
  void GetPoint(float p[4]);

 private:
  int PreMultiplyFlag;
  int StackSize;
  vtkMatrix4x4 **Stack;
  vtkMatrix4x4 **StackBottom;
  float Point[4];
  float Orientation[3];

};

inline void vtkTransform::PointMultiply (float in[4],float out[4]) 
{
  this->Stack[0]->PointMultiply(in,out);
}

inline void vtkTransform::MultiplyPoint (float in[4],float out[4]) 
{
  this->Stack[0]->MultiplyPoint(in,out);
}

#endif
