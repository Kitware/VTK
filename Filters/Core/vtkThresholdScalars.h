// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkThresholdScalars
 * @brief   label scalars according to one or more threshold intervals
 *
 * vtkThresholdScalars creates a new scalar field that represents a
 * segmentation of input scalar values into labeled threshold intervals.
 * In other words, this filter can be used to convert a continuous scalar
 * field into one or more labeled regions, each region identified by a
 * labeled threshold segment. In combination with other filters, the
 * segmented scalars can then be used to extract points, or perform
 * various operations such as performing a Voronoi tessellation.
 *
 * This filter operates on any input dataset type, but requires an input
 * point scalar array on which to operate. In addition, one or more
 * disjoint threshold intervals [s0,s1,labelId) should be defined,
 * with an optional "background" label used to mark points whose
 * scalar values are not contained in any threshold interval.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 */

#ifndef vtkThresholdScalars_h
#define vtkThresholdScalars_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkIntervalSet; // Support PIMPLd set of intervals

class VTKFILTERSCORE_EXPORT vtkThresholdScalars : public vtkDataSetAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for class instantiation, obtaining type information,
   * and printing the object.
   */
  vtkTypeMacro(vtkThresholdScalars, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkThresholdScalars* New();
  ///@}

  ///@{
  /**
   * Methods to create and manage threshold intervals. Intervals [s0,s1,labelId)
   * should be disjoint; an input scalar s is in an interval when s0<=s<s1. If s
   * is not in any interval, then it is assigned the background label. Intervals
   * do not have to be defined in any particular order. The AddInterval() method
   * returns an id that can be used to subsequently delete the interval using
   * RemoveInterval().
   */
  vtkIdType AddInterval(double s0, double s1, int labelId);
  void RemoveInterval(vtkIdType intervalId);
  void RemoveAllIntervals();
  ///@}

  ///@{
  /**
   * Set the background label value. This label is used when an input
   * scalar value does not fall within any threshold interval.
   */
  vtkSetMacro(BackgroundLabel, int);
  vtkGetMacro(BackgroundLabel, int);
  ///@}

protected:
  vtkThresholdScalars();
  ~vtkThresholdScalars() override;

  // PIMPLd set of intervals
  vtkIntervalSet* Intervals;

  // Scalars that don't fall in any interval are assigned
  // the background label.
  int BackgroundLabel;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkThresholdScalars(const vtkThresholdScalars&) = delete;
  void operator=(const vtkThresholdScalars&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
