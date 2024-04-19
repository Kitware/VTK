// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkProcrustesAlignmentFilter
 * @brief   aligns a set of pointsets together
 *
 *
 * vtkProcrustesAlignmentFilter is a filter that takes a set of pointsets
 * (any object derived from vtkPointSet) and aligns them in a least-squares
 * sense to their mutual mean. The algorithm is iterated until convergence,
 * as the mean must be recomputed after each alignment.
 *
 * vtkProcrustesAlignmentFilter requires a vtkMultiBlock input consisting
 * of vtkPointSets as first level children.
 *
 * The default (in vtkLandmarkTransform) is for a similarity alignment.
 * For a rigid-body alignment (to build a 'size-and-shape' model) use:
 *
 *    GetLandmarkTransform()->SetModeToRigidBody().
 *
 * Affine alignments are not normally used but are left in for completeness:
 *
 *    GetLandmarkTransform()->SetModeToAffine().
 *
 * vtkProcrustesAlignmentFilter is an implementation of:
 *
 *    J.C. Gower (1975)
 *    Generalized Procrustes Analysis. Psychometrika, 40:33-51.
 *
 * @warning
 * All of the input pointsets must have the same number of points.
 *
 * @par Thanks:
 * Tim Hutton and Rasmus Paulsen who developed and contributed this class
 *
 * @sa
 * vtkLandmarkTransform
 */

#ifndef vtkProcrustesAlignmentFilter_h
#define vtkProcrustesAlignmentFilter_h

#include "vtkFiltersHybridModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkLandmarkTransform;
class vtkPointSet;
class vtkPoints;

class VTKFILTERSHYBRID_EXPORT vtkProcrustesAlignmentFilter : public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkProcrustesAlignmentFilter, vtkMultiBlockDataSetAlgorithm);

  /**
   * Prints information about the state of the filter.
   */
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates with similarity transform.
   */
  static vtkProcrustesAlignmentFilter* New();

  ///@{
  /**
   * Get the internal landmark transform. Use it to constrain the number of
   * degrees of freedom of the alignment (i.e. rigid body, similarity, etc.).
   * The default is a similarity alignment.
   */
  vtkGetObjectMacro(LandmarkTransform, vtkLandmarkTransform);
  ///@}

  ///@{
  /**
   * Get the estimated mean point cloud
   */
  vtkGetObjectMacro(MeanPoints, vtkPoints);
  ///@}

  ///@{
  /**
   * When on, the initial alignment is to the centroid
   * of the cohort curves.  When off, the alignment is to the
   * centroid of the first input.  Default is off for
   * backward compatibility.
   */
  vtkSetMacro(StartFromCentroid, bool);
  vtkGetMacro(StartFromCentroid, bool);
  vtkBooleanMacro(StartFromCentroid, bool);
  ///@}

  ///@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings. If the desired precision is
   * DEFAULT_PRECISION and any of the inputs are double precision, then the
   * mean points will be double precision. Otherwise, if the desired
   * precision is DEFAULT_PRECISION and all the inputs are single precision,
   * then the mean points will be single precision.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

protected:
  vtkProcrustesAlignmentFilter();
  ~vtkProcrustesAlignmentFilter() override;

  /**
   * Usual data generation method.
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkLandmarkTransform* LandmarkTransform;

  bool StartFromCentroid;

  vtkPoints* MeanPoints;
  int OutputPointsPrecision;

private:
  vtkProcrustesAlignmentFilter(const vtkProcrustesAlignmentFilter&) = delete;
  void operator=(const vtkProcrustesAlignmentFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
