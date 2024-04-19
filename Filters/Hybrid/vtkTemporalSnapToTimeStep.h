// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTemporalSnapToTimeStep
 * @brief   modify the time range/steps of temporal data
 *
 * vtkTemporalSnapToTimeStep  modify the time range or time steps of
 * the data without changing the data itself. The data is not resampled
 * by this filter, only the information accompanying the data is modified.
 *
 * @par Thanks:
 * John Bidiscombe of CSCS - Swiss National Supercomputing Centre
 * for creating and contributing this class.
 * For related material, please refer to :
 * John Biddiscombe, Berk Geveci, Ken Martin, Kenneth Moreland, David Thompson,
 * "Time Dependent Processing in a Parallel Pipeline Architecture",
 * IEEE Visualization 2007.
 */

#ifndef vtkTemporalSnapToTimeStep_h
#define vtkTemporalSnapToTimeStep_h

#include "vtkFiltersHybridModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

#include <vector> // used because I am a bad boy. So there.

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSHYBRID_EXPORT vtkTemporalSnapToTimeStep : public vtkPassInputTypeAlgorithm
{
public:
  static vtkTemporalSnapToTimeStep* New();
  vtkTypeMacro(vtkTemporalSnapToTimeStep, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum
  {
    VTK_SNAP_NEAREST = 0,
    VTK_SNAP_NEXTBELOW_OR_EQUAL,
    VTK_SNAP_NEXTABOVE_OR_EQUAL
  };

  vtkSetMacro(SnapMode, int);
  vtkGetMacro(SnapMode, int);
  void SetSnapModeToNearest() { this->SetSnapMode(VTK_SNAP_NEAREST); }
  void SetSnapModeToNextBelowOrEqual() { this->SetSnapMode(VTK_SNAP_NEXTBELOW_OR_EQUAL); }
  void SetSnapModeToNextAboveOrEqual() { this->SetSnapMode(VTK_SNAP_NEXTABOVE_OR_EQUAL); }

protected:
  vtkTemporalSnapToTimeStep();
  ~vtkTemporalSnapToTimeStep() override;

  /**
   * see vtkAlgorithm for details
   */
  vtkTypeBool ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  std::vector<double> InputTimeValues;
  vtkTypeBool HasDiscrete;
  int SnapMode;

private:
  vtkTemporalSnapToTimeStep(const vtkTemporalSnapToTimeStep&) = delete;
  void operator=(const vtkTemporalSnapToTimeStep&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
