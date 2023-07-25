// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWarpLens
 * @brief   deform geometry by applying lens distortion
 *
 * vtkWarpLens is a filter that modifies point coordinates by moving
 * in accord with a lens distortion model.
 */

#ifndef vtkWarpLens_h
#define vtkWarpLens_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkWarpLens : public vtkPointSetAlgorithm
{
public:
  static vtkWarpLens* New();
  vtkTypeMacro(vtkWarpLens, vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify second order symmetric radial lens distortion parameter.
   * This is obsoleted by newer instance variables.
   */
  void SetKappa(double kappa);
  double GetKappa();
  ///@}

  ///@{
  /**
   * Specify the center of radial distortion in pixels.
   * This is obsoleted by newer instance variables.
   */
  void SetCenter(double centerX, double centerY);
  double* GetCenter() VTK_SIZEHINT(2);
  ///@}

  ///@{
  /**
   * Specify the calibrated principal point of the camera/lens
   */
  vtkSetVector2Macro(PrincipalPoint, double);
  vtkGetVectorMacro(PrincipalPoint, double, 2);
  ///@}

  ///@{
  /**
   * Specify the symmetric radial distortion parameters for the lens
   */
  vtkSetMacro(K1, double);
  vtkGetMacro(K1, double);
  vtkSetMacro(K2, double);
  vtkGetMacro(K2, double);
  ///@}

  ///@{
  /**
   * Specify the decentering distortion parameters for the lens
   */
  vtkSetMacro(P1, double);
  vtkGetMacro(P1, double);
  vtkSetMacro(P2, double);
  vtkGetMacro(P2, double);
  ///@}

  ///@{
  /**
   * Specify the imager format width / height in mm
   */
  vtkSetMacro(FormatWidth, double);
  vtkGetMacro(FormatWidth, double);
  vtkSetMacro(FormatHeight, double);
  vtkGetMacro(FormatHeight, double);
  ///@}

  ///@{
  /**
   * Specify the image width / height in pixels
   */
  vtkSetMacro(ImageWidth, int);
  vtkGetMacro(ImageWidth, int);
  vtkSetMacro(ImageHeight, int);
  vtkGetMacro(ImageHeight, int);
  ///@}

  int FillInputPortInformation(int port, vtkInformation* info) override;

protected:
  vtkWarpLens();
  ~vtkWarpLens() override = default;

  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  double PrincipalPoint[2]; // The calibrated principal point of camera/lens in mm
  double K1;                // Symmetric radial distortion parameters
  double K2;
  double P1; // Decentering distortion parameters
  double P2;
  double FormatWidth;  // imager format width in mm
  double FormatHeight; // imager format height in mm
  int ImageWidth;      // image width in pixels
  int ImageHeight;     // image height in pixels
private:
  vtkWarpLens(const vtkWarpLens&) = delete;
  void operator=(const vtkWarpLens&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
