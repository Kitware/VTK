// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCenterOfMass
 * @brief   Find the center of mass of a set of points.
 *
 * vtkCenterOfMass finds the "center of mass" of a vtkPointSet (vtkPolyData
 * or vtkUnstructuredGrid). Optionally, the user can specify to use the scalars
 * as weights in the computation. If this option, UseScalarsAsWeights, is off,
 * each point contributes equally in the calculation.
 *
 * You must ensure Update() has been called before GetCenter will produce a valid
 * value.
 */

#ifndef vtkCenterOfMass_h
#define vtkCenterOfMass_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkPoints;
class vtkDataArray;

class VTKFILTERSCORE_EXPORT vtkCenterOfMass : public vtkPointSetAlgorithm
{
public:
  static vtkCenterOfMass* New();
  vtkTypeMacro(vtkCenterOfMass, vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the output of the center of mass computation.
   */
  vtkSetVector3Macro(Center, double);
  vtkGetVector3Macro(Center, double);
  ///@}

  ///@{
  /**
   * Set a flag to determine if the points are weighted.
   */
  vtkSetMacro(UseScalarsAsWeights, bool);
  vtkGetMacro(UseScalarsAsWeights, bool);
  ///@}

  /**
   * This function is called by RequestData. It exists so that
   * other classes may use this computation without constructing
   * a vtkCenterOfMass object.  The scalars can be set to nullptr
   * if all points are to be weighted equally.  If scalars are
   * used, it is the caller's responsibility to ensure that the
   * number of scalars matches the number of points, and that
   * the sum of the scalars is a positive value.
   */
  static void ComputeCenterOfMass(vtkPoints* input, vtkDataArray* scalars, double center[3]);

protected:
  vtkCenterOfMass();

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkCenterOfMass(const vtkCenterOfMass&) = delete;
  void operator=(const vtkCenterOfMass&) = delete;

  bool UseScalarsAsWeights;
  double Center[3];
};

VTK_ABI_NAMESPACE_END
#endif
