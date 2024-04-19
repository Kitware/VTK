// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkAnimateModes
 * @brief animate mode shapes
 *
 * For certain file formats, like Exodus, simulation codes may use the timesteps
 * and time values to represent quantities other than time. For example, for
 * modal analysis, the natural frequency for each mode may be used as the time
 * value. vtkAnimateModes can be used to reinterpret time as mode shapes.
 * The filter can also animate vibrations for each mode shape (when
 * AnimateVibrations is set to true). In that case, the time requested by the
 * downstream pipeline is used to scale the displacement magnitude
 * for a mode shape in a sinusoidal pattern, `cos(2*pi * requested-time)`.
 *
 * Historically, the VTK's Exodus reader (`vtkExodusIIReader`) had support for
 * this internally. However, when implementation the IOSS-based reader for
 * Exodus files (`vtkIossReader`), it was decided that it's cleaner to leave the
 * mode shape and vibration animation logic independent of the reader and thus
 * make it usable with other file formats too. Hence this filter was created.
 */

#ifndef vtkAnimateModes_h
#define vtkAnimateModes_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

#include <vector> // for std::vector

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkAnimateModes : public vtkPassInputTypeAlgorithm
{
public:
  static vtkAnimateModes* New();
  vtkTypeMacro(vtkAnimateModes, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get whether the filter should animate the vibrations.
   * Defaults to true. When set, the requested time is used compute
   * displacements for the chosen mode shape.
   * Defaults to true.
   */
  vtkSetMacro(AnimateVibrations, bool);
  vtkGetMacro(AnimateVibrations, bool);
  vtkBooleanMacro(AnimateVibrations, bool);
  ///@}

  ///@{
  /**
   * Get the range for available mode shapes in the input. One must call
   * `UpdateInformation` before check the range since the range is determined
   * based on the number of input timesteps. The range is always `[1, <number of
   * timesteps>]`.
   */
  vtkGetVector2Macro(ModeShapesRange, int);
  ///@}

  ///@{
  /**
   * Get/Set the mode shape to animate. Defaults to 1.
   */
  vtkSetClampMacro(ModeShape, int, 1, VTK_INT_MAX);
  vtkGetMacro(ModeShape, int);
  ///@}

  ///@{
  /**
   * Get/Set whether displacements are pre-applied.
   * Default is false.
   */
  vtkSetMacro(DisplacementPreapplied, bool);
  vtkGetMacro(DisplacementPreapplied, bool);
  vtkBooleanMacro(DisplacementPreapplied, bool);
  ///@}

  ///@{
  /**
   * Get/Set a scale factor to apply the displacements.
   * Defaults to 1.
   */
  vtkSetMacro(DisplacementMagnitude, double);
  vtkGetMacro(DisplacementMagnitude, double);
  ///@}

  ///@{
  /**
   * This returns (0, 1.0) as the range that can be used when animating a mode
   * shape.
   */
  vtkGetVector2Macro(TimeRange, double);
  ///@}

protected:
  vtkAnimateModes();
  ~vtkAnimateModes() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkAnimateModes(const vtkAnimateModes&) = delete;
  void operator=(const vtkAnimateModes&) = delete;

  bool AnimateVibrations;
  int ModeShapesRange[2];
  int ModeShape;
  double DisplacementMagnitude;
  bool DisplacementPreapplied;
  std::vector<double> InputTimeSteps;
  double TimeRange[2];
};

VTK_ABI_NAMESPACE_END
#endif
