/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIdentityTransform.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkIdentityTransform - a transform that doesn't do anything
// .SECTION Description
// vtkIdentityTransform is a transformation which will simply pass coordinate
// data unchanged.  All other transform types can also do this, however,
// the vtkIdentityTransform does so with much greater efficiency.
// .SECTION see also
// vtkLinearTransform


#ifndef __vtkIdentityTransform_h
#define __vtkIdentityTransform_h

#include "vtkCommonTransformsModule.h" // For export macro
#include "vtkLinearTransform.h"

class VTKCOMMONTRANSFORMS_EXPORT vtkIdentityTransform : public vtkLinearTransform
{
public:
  static vtkIdentityTransform *New();

  vtkTypeMacro(vtkIdentityTransform,vtkLinearTransform);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Apply the transformation to a series of points, and append the
  // results to outPts.
  void TransformPoints(vtkPoints *inPts, vtkPoints *outPts);

  // Description:
  // Apply the transformation to a series of normals, and append the
  // results to outNms.
  void TransformNormals(vtkDataArray *inNms, vtkDataArray *outNms);

  // Description:
  // Apply the transformation to a series of vectors, and append the
  // results to outVrs.
  void TransformVectors(vtkDataArray *inVrs, vtkDataArray *outVrs);

  // Description:
  // Apply the transformation to a combination of points, normals
  // and vectors.
  void TransformPointsNormalsVectors(vtkPoints *inPts,
                                     vtkPoints *outPts,
                                     vtkDataArray *inNms,
                                     vtkDataArray *outNms,
                                     vtkDataArray *inVrs,
                                     vtkDataArray *outVrs);

  // Invert the transformation.  This doesn't do anything to the
  // identity transformation.
  void Inverse() {}

  // Description:
  // This will calculate the transformation without calling Update.
  // Meant for use only within other VTK classes.
  void InternalTransformPoint(const float in[3], float out[3]);
  void InternalTransformPoint(const double in[3], double out[3]);

  // Description:
  // This will calculate the transformation without calling Update.
  // Meant for use only within other VTK classes.
  void InternalTransformNormal(const float in[3], float out[3]);
  void InternalTransformNormal(const double in[3], double out[3]);

  // Description:
  // This will calculate the transformation without calling Update.
  // Meant for use only within other VTK classes.
  void InternalTransformVector(const float in[3], float out[3]);
  void InternalTransformVector(const double in[3], double out[3]);

  // Description:
  // This will calculate the transformation as well as its derivative
  // without calling Update.  Meant for use only within other VTK
  // classes.
  void InternalTransformDerivative(const float in[3], float out[3],
                                   float derivative[3][3]);
  void InternalTransformDerivative(const double in[3], double out[3],
                                   double derivative[3][3]);

  // Description:
  // Make a transform of the same type.  This will actually
  // return the same transform.
  vtkAbstractTransform *MakeTransform();

protected:
  vtkIdentityTransform();
  ~vtkIdentityTransform();

  void InternalDeepCopy(vtkAbstractTransform *t);

private:
  vtkIdentityTransform(const vtkIdentityTransform&);  // Not implemented.
  void operator=(const vtkIdentityTransform&);  // Not implemented.
};

#endif





