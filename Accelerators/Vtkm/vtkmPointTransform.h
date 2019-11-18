/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkmPointTransform
 * @brief transform points via vtkm PointTransform filter
 *
 * vtkmPointTransform is a filter to transform point coordinates. For now it
 * does not support transforming associated point normals and vectors, as well
 * as cell normals and vectors with the point coordinates.
 */

#ifndef vtkmPointTransform_h
#define vtkmPointTransform_h

#include "vtkAcceleratorsVTKmModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

class vtkHomogeneousTransform;

class VTKACCELERATORSVTKM_EXPORT vtkmPointTransform : public vtkPointSetAlgorithm
{
public:
  vtkTypeMacro(vtkmPointTransform, vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkmPointTransform* New();

  //@{
  /**
   * Specify the transform object used to transform the points
   */
  void SetTransform(vtkHomogeneousTransform* tf);
  vtkGetObjectMacro(Transform, vtkHomogeneousTransform);
  //@}

  int FillInputPortInformation(int port, vtkInformation* info) override;

protected:
  vtkmPointTransform();
  ~vtkmPointTransform() override;
  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  vtkHomogeneousTransform* Transform;

private:
  vtkmPointTransform(const vtkmPointTransform&) = delete;
  void operator=(const vtkmPointTransform&) = delete;
};

#endif
// VTK-HeaderTest-Exclude: vtkmPointTransform.h
