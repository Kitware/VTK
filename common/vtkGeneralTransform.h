/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeneralTransform.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen.

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
  // TransformPoint etc. is called unless the AutoUpdate
  // flag is off.
  virtual void Update();

  // Description:
  // Automatically update the transform any time that
  // TransformPoint, TransformPoints, etc. are called.
  // This is on by default, you can turn it off before
  // transforming a bunch of points to improve efficiency.
  // But make sure that you turn it back on again later!
  vtkSetMacro(AutoUpdate,int);
  vtkBooleanMacro(AutoUpdate,int);
  vtkGetMacro(AutoUpdate,int);

  // Description:
  // Needs a special UnRegister() implementation to avoid
  // circular references.
  void UnRegister(vtkObject * o);

protected:
  vtkGeneralTransform() { this->AutoUpdate = 1; 
                          this->MyInverse = NULL; };

  ~vtkGeneralTransform() {};
  vtkGeneralTransform(const vtkGeneralTransform&) {};
  void operator=(const vtkGeneralTransform&) {};

  int TransformType;
  int AutoUpdate;
  vtkGeneralTransformInverse *MyInverse;
  
  float InternalFloatPoint[3];
  double InternalDoublePoint[3];
};

#endif





