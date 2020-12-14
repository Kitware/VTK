/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParallelVectors.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkParallelVectors
 * @brief   Compute polylines corresponding to locations where two vector fields
 *          are parallel
 *
 * vtkParallelVectors accepts a 3D dataset with two nodal 3-vector fields and
 * generates polylines along the paths where the vector fields are parallel.
 * This filter is an implementation of the concepts outlined in the following
 * article:
 *
 * R. Peikert and M. Roth, "The "Parallel Vectors" operator-a vector field
 * visualization primitive," Proceedings Visualization '99 (Cat. No.99CB37067),
 * San Francisco, CA, USA, 1999, pp. 263-532.
 *
 * @sa
 * vtkVortexCore
 */

#ifndef vtkParallelVectors_h
#define vtkParallelVectors_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#include "vtkNew.h" // for vtkNew

class VTKFILTERSFLOWPATHS_EXPORT vtkParallelVectors : public vtkPolyDataAlgorithm
{
public:
  static vtkParallelVectors* New();
  vtkTypeMacro(vtkParallelVectors, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/Get the name of first vector field.
   */
  vtkSetStringMacro(FirstVectorFieldName);
  vtkGetStringMacro(FirstVectorFieldName);
  //@}

  //@{
  /**
   * Set/Get the name of second vector field.
   */
  vtkSetStringMacro(SecondVectorFieldName);
  vtkGetStringMacro(SecondVectorFieldName);
  //@}

protected:
  vtkParallelVectors();
  ~vtkParallelVectors() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

  virtual void Prefilter(vtkInformation*, vtkInformationVector**, vtkInformationVector*) {}
  virtual void Postfilter(vtkInformation*, vtkInformationVector**, vtkInformationVector*) {}

  virtual bool AcceptSurfaceTriangle(const vtkIdType surfaceSimplexIndices[3]);

  /**
   * Computes additional criteria to determine if a point should be added to a vortex core.
   * Criteria are returned in the criteria parameter.
   */
  virtual bool ComputeAdditionalCriteria(
    const vtkIdType surfaceSimplexIndices[3], double s, double t);

  /**
   * Contains the name of the first vector field to compare.
   */
  char* FirstVectorFieldName;

  /**
   * Contains the name of the second vector field to compare.
   */
  char* SecondVectorFieldName;

  /**
   * Map from unique points inserted in the data set to valid point ids.
   * This is potentially needed to update auxilliary arrays in subclasses.
   * Auxilliary arrays may be built in overridden versions of ComputeAdditionalCriteria(),
   * where only the current index of the valid point is known. However, points
   * are inserted to be unique, so at the end of the algorithm, the point data arrays
   * do not match one-for-one the points. We need to create new
   * auxilliary arrays with point data that matches the points in the output. This
   * map is used for that purpose.
   */
  vtkNew<vtkIdTypeArray> UniquePointIdToValidId;

private:
  vtkParallelVectors(const vtkParallelVectors&) = delete;
  void operator=(const vtkParallelVectors&) = delete;
};

#endif
