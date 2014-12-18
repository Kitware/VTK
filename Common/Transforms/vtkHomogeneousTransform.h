/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHomogeneousTransform.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHomogeneousTransform - superclass for homogeneous transformations
// .SECTION Description
// vtkHomogeneousTransform provides a generic interface for homogeneous
// transformations, i.e. transformations which can be represented by
// multiplying a 4x4 matrix with a homogeneous coordinate.
// .SECTION see also
// vtkPerspectiveTransform vtkLinearTransform vtkIdentityTransform


#ifndef vtkHomogeneousTransform_h
#define vtkHomogeneousTransform_h

#include "vtkCommonTransformsModule.h" // For export macro
#include "vtkAbstractTransform.h"

class vtkMatrix4x4;

class VTKCOMMONTRANSFORMS_EXPORT vtkHomogeneousTransform : public vtkAbstractTransform
{
public:

  vtkTypeMacro(vtkHomogeneousTransform,vtkAbstractTransform);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Apply the transformation to a series of points, and append the
  // results to outPts.
  void TransformPoints(vtkPoints *inPts, vtkPoints *outPts);

  // Description:
  // Apply the transformation to a combination of points, normals
  // and vectors.
  virtual void TransformPointsNormalsVectors(vtkPoints *inPts,
                                             vtkPoints *outPts,
                                             vtkDataArray *inNms,
                                             vtkDataArray *outNms,
                                             vtkDataArray *inVrs,
                                             vtkDataArray *outVrs);

  // Description:
  // Get a copy of the internal transformation matrix.  The
  // transform is Updated first, to guarantee that the matrix
  // is valid.
  void GetMatrix(vtkMatrix4x4 *m);

  // Description:
  // Get a pointer to an internal vtkMatrix4x4 that represents
  // the transformation.  An Update() is called on the transform
  // to ensure that the matrix is up-to-date when you get it.
  // You should not store the matrix pointer anywhere because it
  // might become stale.
  vtkMatrix4x4 *GetMatrix() { this->Update(); return this->Matrix; };

  // Description:
  // Just like GetInverse(), but includes typecast to vtkHomogeneousTransform.
  vtkHomogeneousTransform *GetHomogeneousInverse()
    {
      return static_cast<vtkHomogeneousTransform *>(this->GetInverse());
    }

  // Description:
  // This will calculate the transformation without calling Update.
  // Meant for use only within other VTK classes.
  void InternalTransformPoint(const float in[3], float out[3]);
  void InternalTransformPoint(const double in[3], double out[3]);

  // Description:
  // This will calculate the transformation as well as its derivative
  // without calling Update.  Meant for use only within other VTK
  // classes.
  void InternalTransformDerivative(const float in[3], float out[3],
                                   float derivative[3][3]);
  void InternalTransformDerivative(const double in[3], double out[3],
                                   double derivative[3][3]);

protected:
  vtkHomogeneousTransform();
  ~vtkHomogeneousTransform();

  void InternalDeepCopy(vtkAbstractTransform *transform);

  vtkMatrix4x4 *Matrix;

private:
  vtkHomogeneousTransform(const vtkHomogeneousTransform&);  // Not implemented.
  void operator=(const vtkHomogeneousTransform&);  // Not implemented.
};

#endif





