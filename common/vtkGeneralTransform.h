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
// vtkGeneralTransform provides a generic interface for linear, 
// perspective, and nonlinear warp transformations.
// .SECTION see also
// vtkTransform


#ifndef __vtkGeneralTransform_h
#define __vtkGeneralTransform_h

#include "vtkObject.h"
#include "vtkPoints.h"
#include "vtkNormals.h"
#include "vtkVectors.h"

#define VTK_PERSPECTIVE_TRANSFORM         0x00000001
#define VTK_LINEAR_TRANSFORM              0x00000003
#define VTK_SIMILARITY_TRANSFORM          0x00000007
#define VTK_RIGIDBODY_TRANSFORM           0x0000000f

#define VTK_MATRIX4X4_TRANSFORM           0x00000101
#define VTK_MATRIX4X3_TRANSFORM           0x00000203
#define VTK_QUATERNION_TRANSFORM          0x00000407

#define VTK_GRID_TRANSFORM                0x00010000
#define VTK_THINPLATESPLINE_TRANSFORM     0x00020000
#define VTK_CUSTOM_TRANSFORM              0x00040000

#define VTK_INVERSE_TRANSFORM             0x01000000
#define VTK_CONCATENATION_TRANSFORM       0x02000000

class vtkGeneralTransformInverse;

class VTK_EXPORT vtkGeneralTransform : public vtkObject
{
public:

  vtkTypeMacro(vtkGeneralTransform,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the type of this transform.
  vtkGetMacro(TransformType,int);

  // Description:
  // Apply the transformation to an (x,y,z) coordinate.
  // Use this if you are programming in python, tcl or Java.
  float *TransformFloatPoint(float x, float y, float z);
  double *TransformDoublePoint(double x, double y, double z);

  // Description:
  // Apply the transformation to a coordinate.  You can use the same 
  // array to store both the input and output point.
  virtual void TransformPoint(const float in[3], float out[3]) = 0;
  virtual void TransformPoint(const double in[3], double out[3]);

  // Description:
  // Apply the transformation to a series of points, and append the
  // results to outPts.  
  virtual void TransformPoints(vtkPoints *inPts, vtkPoints *outPts);

  // Description:
  // Apply the transformation to a series of normals, and append the
  // results to outNms.  The outPts must have been calculated beforehand.
  // The inPts and outPts are required in order for nonlinear transformations
  // to be properly supported.
  virtual void TransformNormals(vtkPoints *inPts, vtkPoints *outPts, 
				vtkNormals *inNms, vtkNormals *outNms);

  // Description:
  // Apply the transformation to a series of vectors, and append the
  // results to outVrs.  The outPts must have been calculated beforehand.
  // The inPts and outPts are required in order for nonlinear transformations
  // to be properly supported.
  virtual void TransformVectors(vtkPoints *inPts, vtkPoints *outPts, 
				vtkVectors *inVrs, vtkVectors *outVrs);

  // Description:
  // Create an identity transformation.
  virtual void Identity() {};

  // Description:
  // Invert the transformation.
  virtual void Inverse() {};

  // Description:
  // Get the inverse of this transform.  If you modify this transform,
  // the returned inverse transform will automatically update.
  virtual vtkGeneralTransform *GetInverse();

  // Description:
  // Make another transform of the same type.
  virtual vtkGeneralTransform *MakeTransform() { return NULL; };

  // Description:
  // Copy this transform from another of the same type.
  virtual void DeepCopy(vtkGeneralTransform *) {};

  // Description:
  // Update the transform to account for any changes which
  // have been made.  This is called automatically when
  // TransformPoint etc. is called.
  virtual void Update();

  // Description:
  // Needs a special UnRegister() implementation to avoid
  // circular references.
  void UnRegister(vtkObject * o);

protected:
  vtkGeneralTransform() { this->AutoUpdate = 1; 
                          this->MyInverse = NULL; 
                          this->InUnRegister = 0; };

  ~vtkGeneralTransform();
  vtkGeneralTransform(const vtkGeneralTransform&) {};
  void operator=(const vtkGeneralTransform&) {};

//BTX
  vtkSetMacro(AutoUpdate,int);
  vtkBooleanMacro(AutoUpdate,int);
  vtkGetMacro(AutoUpdate,int);
//ETX

  int TransformType;
  int AutoUpdate;
  vtkGeneralTransformInverse *MyInverse;
  
  float InternalFloatPoint[3];
  double InternalDoublePoint[3];

  int InUnRegister;
};

#endif





