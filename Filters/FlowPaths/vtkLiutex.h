// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLiutex
 * @brief   Compute the Liutex vortex vector and Omega-Liutex from a velocity field
 *
 * Liutex (formerly Rortex) measures the local rigid rotation of a fluid. Its
 * direction r is the real eigenvector of the velocity gradient tensor J, and its
 * magnitude is
 *
 *   R = omega . r - sqrt((omega . r)^2 - 4 lambda_ci^2)
 *
 * where omega is the vorticity, r is oriented so that omega . r > 0, and
 * lambda_ci is the imaginary part of the complex conjugate eigenvalue pair of J.
 * Where J has three real eigenvalues there is no local rotation and the Liutex
 * vector is zero. For rigid rotation Liutex equals the vorticity; for pure shear
 * it is zero.
 *
 * The input array is either a 3-component velocity, whose gradient is computed
 * with vtkGradientFilter, or a 9-component velocity gradient with the layout
 * vtkGradientFilter produces (component 3 * i + j holds du_i/dx_j). Supplying
 * the gradient from the solver avoids recomputing it and is usually more
 * accurate. Output arrays are added to the same attributes (points or cells) as
 * the input array, with the precision of the gradient.
 *
 * Optionally the filter computes Omega-Liutex, a normalized measure in [0, 1):
 *
 *   Omega_R = beta^2 / (beta^2 + alpha^2 + lambda_cr^2 + lambda_r^2 / 2 + epsilon)
 *
 * with beta = (omega . r) / 2, alpha^2 = beta^2 - lambda_ci^2, lambda_cr the real
 * part of the complex eigenvalue pair and lambda_r the real eigenvalue. Unless
 * OmegaLiutexEpsilon is set, epsilon is OmegaLiutexB0 times the maximum of
 * beta^2 - alpha^2 over the whole input: every block of a composite dataset and,
 * when a global vtkMultiProcessController is set, every process.
 *
 * Omega-Liutex measures rotation relative to shear and stretching, not its
 * strength, so weak rotation can score as high as strong rotation. Where a
 * strong shear meets a slight opposing strain it rises just above 0.5, and the
 * usual 0.52 isosurface then encloses large regions with little rotation. In
 * shear-dominated flows, contour at about 0.6, raise OmegaLiutexB0, or combine
 * Omega-Liutex with a threshold on the Liutex magnitude.
 *
 * References:
 * - C. Liu, Y. Gao, S. Tian, X. Dong, "Rortex: A new vortex vector definition and
 *   vorticity tensor and vector decompositions", Physics of Fluids 30, 035103 (2018).
 * - Y. Wang, Y. Gao, J. Liu, C. Liu, "Explicit formula for the Liutex vector and
 *   physical meaning of vorticity based on the Liutex-Shear decomposition",
 *   Journal of Hydrodynamics 31, 464-474 (2019).
 * - X. Dong, Y. Gao, C. Liu, "New normalized Rortex/vortex identification method",
 *   Physics of Fluids 31, 011701 (2019).
 *
 * @sa
 * vtkLiutexCoreLines vtkGradientFilter vtkVortexCore
 */

#ifndef vtkLiutex_h
#define vtkLiutex_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

#include <string> // For std::string

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;

class VTKFILTERSFLOWPATHS_EXPORT vtkLiutex : public vtkPassInputTypeAlgorithm
{
public:
  static vtkLiutex* New();
  vtkTypeMacro(vtkLiutex, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Add the Liutex vector (3 components) to the output. The default is on.
   */
  vtkSetMacro(ComputeLiutexVector, bool);
  vtkGetMacro(ComputeLiutexVector, bool);
  vtkBooleanMacro(ComputeLiutexVector, bool);
  ///@}

  ///@{
  /**
   * Add the Liutex magnitude R to the output. The default is on.
   */
  vtkSetMacro(ComputeLiutexMagnitude, bool);
  vtkGetMacro(ComputeLiutexMagnitude, bool);
  vtkBooleanMacro(ComputeLiutexMagnitude, bool);
  ///@}

  ///@{
  /**
   * Add Omega-Liutex to the output. The default is off.
   */
  vtkSetMacro(ComputeOmegaLiutex, bool);
  vtkGetMacro(ComputeOmegaLiutex, bool);
  vtkBooleanMacro(ComputeOmegaLiutex, bool);
  ///@}

  ///@{
  /**
   * Names of the output arrays. The defaults are "Liutex", "LiutexMagnitude"
   * and "OmegaLiutex".
   */
  vtkSetMacro(LiutexVectorArrayName, std::string);
  vtkGetMacro(LiutexVectorArrayName, std::string);
  vtkSetMacro(LiutexMagnitudeArrayName, std::string);
  vtkGetMacro(LiutexMagnitudeArrayName, std::string);
  vtkSetMacro(OmegaLiutexArrayName, std::string);
  vtkGetMacro(OmegaLiutexArrayName, std::string);
  ///@}

  ///@{
  /**
   * Scale factor b0 for the automatic Omega-Liutex epsilon,
   * epsilon = b0 * max(beta^2 - alpha^2). The default is 0.001.
   */
  vtkSetClampMacro(OmegaLiutexB0, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(OmegaLiutexB0, double);
  ///@}

  ///@{
  /**
   * A fixed Omega-Liutex epsilon. A negative value, the default, selects the
   * automatic epsilon described by OmegaLiutexB0.
   */
  vtkSetMacro(OmegaLiutexEpsilon, double);
  vtkGetMacro(OmegaLiutexEpsilon, double);
  ///@}

  ///@{
  /**
   * Passed to vtkGradientFilter when the input array is a velocity. The
   * default is off.
   */
  vtkSetMacro(FasterApproximation, bool);
  vtkGetMacro(FasterApproximation, bool);
  vtkBooleanMacro(FasterApproximation, bool);
  ///@}

protected:
  vtkLiutex();
  ~vtkLiutex() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkLiutex(const vtkLiutex&) = delete;
  void operator=(const vtkLiutex&) = delete;

  struct BlockState;
  // Adds the requested arrays to dataSet, which shares its arrays with the input.
  bool ProcessDataSet(vtkDataSet* dataSet, BlockState& state, double& maxLambdaCi2);

  bool ComputeLiutexVector = true;
  bool ComputeLiutexMagnitude = true;
  bool ComputeOmegaLiutex = false;
  std::string LiutexVectorArrayName = "Liutex";
  std::string LiutexMagnitudeArrayName = "LiutexMagnitude";
  std::string OmegaLiutexArrayName = "OmegaLiutex";
  double OmegaLiutexB0 = 0.001;
  double OmegaLiutexEpsilon = -1.0;
  bool FasterApproximation = false;
};

VTK_ABI_NAMESPACE_END
#endif
