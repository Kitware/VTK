// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHedgeHog
 * @brief   create oriented lines from vector data
 *
 * vtkHedgeHog creates oriented lines from the input data set. Line
 * length is controlled by vector (or normal) magnitude times scale
 * factor. If VectorMode is UseNormal, normals determine the orientation
 * of the lines. Lines are colored by scalar data, if available.
 */

#ifndef vtkHedgeHog_h
#define vtkHedgeHog_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#define VTK_USE_VECTOR 0
#define VTK_USE_NORMAL 1

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkHedgeHog : public vtkPolyDataAlgorithm
{
public:
  static vtkHedgeHog* New();
  vtkTypeMacro(vtkHedgeHog, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set scale factor to control size of oriented lines.
   */
  vtkSetMacro(ScaleFactor, double);
  vtkGetMacro(ScaleFactor, double);
  ///@}

  ///@{
  /**
   * Specify whether to use vector or normal to perform vector operations.
   */
  vtkSetMacro(VectorMode, int);
  vtkGetMacro(VectorMode, int);
  void SetVectorModeToUseVector() { this->SetVectorMode(VTK_USE_VECTOR); }
  void SetVectorModeToUseNormal() { this->SetVectorMode(VTK_USE_NORMAL); }
  const char* GetVectorModeAsString();
  ///@}

  ///@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

protected:
  vtkHedgeHog();
  ~vtkHedgeHog() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;
  double ScaleFactor;
  int VectorMode; // Orient/scale via normal or via vector data
  int OutputPointsPrecision;

private:
  vtkHedgeHog(const vtkHedgeHog&) = delete;
  void operator=(const vtkHedgeHog&) = delete;
};

/**
 * Return the vector mode as a character string.
 */
inline const char* vtkHedgeHog::GetVectorModeAsString()
{
  if (this->VectorMode == VTK_USE_VECTOR)
  {
    return "UseVector";
  }
  else if (this->VectorMode == VTK_USE_NORMAL)
  {
    return "UseNormal";
  }
  else
  {
    return "Unknown";
  }
}
VTK_ABI_NAMESPACE_END
#endif
