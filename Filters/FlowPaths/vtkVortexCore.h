/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVortexCore.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVortexCore
 * @brief   Compute vortex core lines using the parallel vectors method
 *
 * vtkVortexCore computes vortex core lines using the parallel vectors method,
 * as described in
 *
 * Roth, Martin and Ronald Peikert. “A higher-order method for finding vortex
 * core lines.” Proceedings Visualization '98 (Cat. No.98CB36276) (1998):
 * 143-150.
 *
 * By default, the "Higher-Order" approach of computing the parallel vector
 * lines between the flow field's velocity and jerk is disabled in favor of
 * computing the parallel vector lines between the velocity and acceleration,
 * as suggested in
 *
 * Haimes, Robert and David N. Kenwright. “On the velocity gradient tensor and
 * fluid feature extraction.” (1999).
 *
 * To further discriminate against spurious vortex cores, at each potential point
 * value the Q-criterion, delta-criterion, and lambda_2-criterion as defined in
 *
 * Haller, G. (2005). An objective definition of a vortex. Journal of Fluid
 * Mechanics, 525, 1-26.
 *
 * are checked. Addtitionally, the lambda_ci criterion as defined in
 *
 * Chakraborty, P., Balachandar, S., & Adran, R. (2005). On the relationships
 * between local vortex identification schemes. Journal of Fluid Mechanics, 535,
 * 189-214.
 *
 * is computed. The Q-criterion and delta-criterion are used to prefilter cells
 * prior to the execution of the parallel lines algorithm, and all criteria
 * values are stored as point values on the output
 * polylines.
 *
 * @sa
 * vtkParallelVectors
 */

#ifndef vtkVortexCore_h
#define vtkVortexCore_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSFLOWPATHS_EXPORT vtkVortexCore : public vtkPolyDataAlgorithm
{
public:
  static vtkVortexCore* New();
  vtkTypeMacro(vtkVortexCore, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Use the flow field's jerk instead of acceleration as the second vector field
   * during the parallel vector operation. Disabled by default.
   */
  vtkSetMacro(HigherOrderMethod, vtkTypeBool);
  vtkGetMacro(HigherOrderMethod, vtkTypeBool);
  vtkBooleanMacro(HigherOrderMethod, vtkTypeBool);
  //@}

protected:
  vtkVortexCore();
  ~vtkVortexCore() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

  vtkTypeBool HigherOrderMethod;

private:
  vtkVortexCore(const vtkVortexCore&) = delete;
  void operator=(const vtkVortexCore&) = delete;
};

#endif
