/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeneralTransform.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkGeneralTransform - superclass for geometric transformations
// .SECTION Description
// vtkGeneralTransform is the superclass for all VTK geometric 
// transformations.  The VTK transform heirarchy is split into two
// major branches: warp transformations and perspective (including linear)
// transformations.  The latter can be represented in terms of a 4x4
// transformation matrix, the former cannot.  
// <p>Transformations can be pipelined through two mechanisms:  
// <p>1) GetInverse() returns the pipelined
// inverse of a transformation i.e. if you modify the original transform,
// any transform previously returned by the GetInverse() method will
// automatically update itself according to the change.
// <p>2) you can do pipelined concatenation of transformations through 
// either the Concatenate() method or by directly using the
// vtkGeneralTransformConcatenation class.
// .SECTION see also
// vtkWarpTransform vtkPerspectiveTransform vtkLinearTransform 
// vtkIdentityTransform vtkGeneralTransformConcatenation 
// vtkTransformPolyDataFilter vtkImageReslice


#ifndef __vtkGeneralTransform_h
#define __vtkGeneralTransform_h

#include "vtkObject.h"
#include "vtkPoints.h"
#include "vtkNormals.h"
#include "vtkVectors.h"
#include "vtkMutexLock.h"

//BTX
//forward declaration of a helper class
class VTK_EXPORT vtkSimpleTransformConcatenation;
//ETX

class VTK_EXPORT vtkGeneralTransform : public vtkObject
{
public:

  vtkTypeMacro(vtkGeneralTransform,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Apply the transformation to a coordinate.  You can use the same 
  // array to store both the input and output point.
  void TransformPoint(const float in[3], float out[3]) {
    this->Update(); this->InternalTransformPoint(in,out); };

  // Description:
  // Apply the transformation to a double-precision coordinate.  
  // You can use the same array to store both the input and output point.
  void TransformPoint(const double in[3], double out[3]) {
    this->Update(); this->InternalTransformPoint(in,out); };

  // Description:
  // Synonymous with TransformFloatPoint(x,y,z).
  // Use this if you are programming in python, tcl or Java.
  float *TransformPoint(float x, float y, float z) {
    return this->TransformFloatPoint(x,y,z); }
  float *TransformPoint(const float point[3]) {
    return this->TransformPoint(point[0],point[1],point[2]); };

  // Description:
  // Apply the transformation to an (x,y,z) coordinate.
  // Use this if you are programming in python, tcl or Java.
  float *TransformFloatPoint(float x, float y, float z);
  float *TransformFloatPoint(const float point[3]) {
    return this->TransformFloatPoint(point[0],point[1],point[2]); };

  // Description:
  // Apply the transformation to a double-precision (x,y,z) coordinate.
  // Use this if you are programming in python, tcl or Java.
  double *TransformDoublePoint(double x, double y, double z);
  double *TransformDoublePoint(const double point[3]) {
    return this->TransformDoublePoint(point[0],point[1],point[2]); };

  // Description:
  // Apply the transformation to a series of points, and append the
  // results to outPts.  
  virtual void TransformPoints(vtkPoints *inPts, vtkPoints *outPts);

  // Description:
  // Apply the transformation to a combination of points, normals
  // and vectors.  
  virtual void TransformPointsNormalsVectors(vtkPoints *inPts, 
					     vtkPoints *outPts, 
					     vtkNormals *inNms, 
					     vtkNormals *outNms,
					     vtkVectors *inVrs, 
					     vtkVectors *outVrs);

  // Description:
  // Get the inverse of this transform.  If you modify this transform,
  // the returned inverse transform will automatically update.
  vtkGeneralTransform *GetInverse();

  // Description:
  // Set a transformation that this transform will be the inverse of.
  // This transform will automatically update to agree with the 
  // inverse transform that you set.
  void SetInverse(vtkGeneralTransform *transform);

  // Description:
  // Invert the transformation.
  virtual void Inverse() = 0;

  // Description:
  // Make this transform into an identity transformation.
  virtual void Identity() = 0;

  // Description:
  // Copy this transform from another of the same type.
  void DeepCopy(vtkGeneralTransform *);

  // Description:
  // Update the transform to account for any changes which
  // have been made.  You do not have to call this method 
  // yourself, it is called automatically whenever the
  // transform needs an update.
  void Update();

  // Description:
  // This will calculate the transformation without calling Update.
  // Meant for use only within other VTK classes.
  virtual void InternalTransformPoint(const float in[3], float out[3]) = 0;
  virtual void InternalTransformPoint(const double in[3], double out[3]) = 0;

  // Description:
  // This will transform a point and, at the same time, calculate a
  // 3x3 Jacobian matrix that provides the partial derivatives of the
  // transformation at that point.  This method does not call Update.
  // Meant for use only within other VTK classes.
  virtual void InternalTransformDerivative(const float in[3], float out[3],
					   float derivative[3][3]) = 0;
  virtual void InternalTransformDerivative(const double in[3], double out[3],
					   double derivative[3][3]) = 0;


  // Description:
  // Perform a DeepCopy without type checking, quite risky!
  virtual void InternalDeepCopy(vtkGeneralTransform *) = 0;

  // Description:
  // Make another transform of the same type.
  virtual vtkGeneralTransform *MakeTransform() = 0;

  // Description:
  // Override GetMTime necessary because of inverse transforms.
  unsigned long GetMTime();

  // Description:
  // Needs a special UnRegister() implementation to avoid
  // circular references.
  void UnRegister(vtkObject *O);

protected:
  vtkGeneralTransform();
  ~vtkGeneralTransform();
  vtkGeneralTransform(const vtkGeneralTransform&) {};
  void operator=(const vtkGeneralTransform&) {};

  // Description:
  // Perform updates that are necessary for a specific transform type.
  virtual void InternalUpdate() {};

//BTX
  // The Concatenation is only for use within the vtkGeneralTransform class
  // and within the vtk*Concatenation classes.
//ETX
  vtkSimpleTransformConcatenation *Concatenation;

  float InternalFloatPoint[3];
  double InternalDoublePoint[3];

private:
  
//BTX
  // We need to record the time of the last update, and we also need
  // to do mutex locking so updates don't collide.  These are private
  // because Update() is not virtual.
  // If DependsOnInverse is set, then this transform object will
  // check its inverse on every update, and update itself accordingly
  // if necessary.
//ETX
  vtkTimeStamp UpdateTime;
  vtkSimpleMutexLock UpdateMutex;
  int DependsOnInverse;

//BTX
  // MyInverse is a transform which is the inverse of this one.
//ETX
  vtkGeneralTransform *MyInverse;

  int InUnRegister;
};

//BTX
  //-------------------------------------------------------------------------
  // A helper class (not derived from vtkObject) to make it more efficient
  // to derive vtk*TransformConcatenation classes
class VTK_EXPORT vtkSimpleTransformConcatenation
{
public:
  static vtkSimpleTransformConcatenation *New(
				    vtkGeneralTransform *transform) {
    return new vtkSimpleTransformConcatenation(transform); };
  void Delete() { delete this; };

    // add a transform to the list according to Pre/PostMultiply semantics
  void Concatenate(vtkGeneralTransform *transform);
  void Concatenate(vtkGeneralTransform *t1,
		   vtkGeneralTransform *t2,
		   vtkGeneralTransform *t3,
		   vtkGeneralTransform *t4);
    
  // inverse simply sets the inverse flag
  void Inverse();
    
  // identity simply clears the transform list
  void Identity();
    
  // copy the list
  void DeepCopy(vtkSimpleTransformConcatenation *transform);
    
  // the number of stored transforms
  int GetNumberOfTransforms() { return this->NumberOfTransforms; };
    
  // get one of the transforms
  vtkGeneralTransform *GetTransform(int i);
    
  // switch between pre- and post-multiply mode
  void PreMultiply() { 
    this->PreMultiplyFlag = 1; this->Transform->Modified(); };
  void PostMultiply() { 
    this->PreMultiplyFlag = 0; this->Transform->Modified(); };
    
  // determine which mode we are in
  int GetPreMultiplyFlag() { return this->PreMultiplyFlag; }; 

  // get maximum MTime of all transforms
  unsigned long GetMaxMTime();
    
  // print relevant information
  void PrintSelf(ostream& os, vtkIndent indent);
    
protected:
  vtkSimpleTransformConcatenation(vtkGeneralTransform *transform);
  vtkSimpleTransformConcatenation() {}; // should never be called
  ~vtkSimpleTransformConcatenation();  
    
  // the transform that owns us
  vtkGeneralTransform *Transform;
    
  int InverseFlag;
  int PreMultiplyFlag;
    
  int NumberOfTransforms;
  int MaxNumberOfTransforms;
  vtkGeneralTransform **TransformList;
  vtkGeneralTransform **InverseList;
};
//ETX

#endif





