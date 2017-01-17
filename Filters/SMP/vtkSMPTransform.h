/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPTransform.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMPTransform
 * @brief   Transform that uses the SMP framework
 *
 * Just like its parent, vtkTransform, vtkSMPTransform calculates and
 * manages transforms. Its main difference is that it performs various
 * transform operations over a set of points in parallel using the SMP
 * framework.
 * @sa
 * vtkTransform
*/

#ifndef vtkSMPTransform_h
#define vtkSMPTransform_h

#include "vtkFiltersSMPModule.h" // For export macro
#include "vtkTransform.h"

class VTKFILTERSSMP_EXPORT vtkSMPTransform : public vtkTransform
{
 public:
  static vtkSMPTransform *New();
  vtkTypeMacro(vtkSMPTransform, vtkTransform);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Apply the transformation to a series of points, and append the
   * results to outPts.
   */
  void TransformPoints(vtkPoints *inPts, vtkPoints *outPts) VTK_OVERRIDE;

  /**
   * Apply the transformation to a series of normals, and append the
   * results to outNms.
   */
  void TransformNormals(vtkDataArray *inNms, vtkDataArray *outNms) VTK_OVERRIDE;

  /**
   * Apply the transformation to a series of vectors, and append the
   * results to outVrs.
   */
  void TransformVectors(vtkDataArray *inVrs, vtkDataArray *outVrs) VTK_OVERRIDE;

  /**
   * Apply the transformation to a combination of points, normals
   * and vectors.
   */
  void TransformPointsNormalsVectors(vtkPoints *inPts,
                                     vtkPoints *outPts,
                                     vtkDataArray *inNms,
                                     vtkDataArray *outNms,
                                     vtkDataArray *inVrs,
                                     vtkDataArray *outVrs) VTK_OVERRIDE;

protected:
  vtkSMPTransform () {}
  ~vtkSMPTransform () VTK_OVERRIDE {}

private:
  vtkSMPTransform (const vtkSMPTransform&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMPTransform&) VTK_DELETE_FUNCTION;
};

#endif
