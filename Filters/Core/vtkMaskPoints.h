// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMaskPoints
 * @brief   selectively filter points
 *
 * vtkMaskPoints is a filter that passes through points and point attributes
 * from input dataset. (Other geometry is not passed through.) It is
 * possible to mask every nth point, and to specify an initial offset
 * to begin masking from.
 * It is possible to also generate different random selections
 * (jittered strides, real random samples, and spatially stratified
 * random samples) from the input data.
 * The filter can also generate vertices (topological
 * primitives) as well as points. This is useful because vertices are
 * rendered while points are not.
 */

#ifndef vtkMaskPoints_h
#define vtkMaskPoints_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkMaskPoints : public vtkPolyDataAlgorithm
{
public:
  // Method used to pick points
  enum DistributionType
  {
    RANDOMIZED_ID_STRIDES,
    RANDOM_SAMPLING,
    SPATIALLY_STRATIFIED,
    UNIFORM_SPATIAL_BOUNDS,
    UNIFORM_SPATIAL_SURFACE,
    UNIFORM_SPATIAL_VOLUME
  };

  static vtkMaskPoints* New();
  vtkTypeMacro(vtkMaskPoints, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Turn on every nth point (strided sampling), ignored by random modes.
   */
  vtkSetClampMacro(OnRatio, int, 1, VTK_INT_MAX);
  vtkGetMacro(OnRatio, int);
  ///@}

  ///@{
  /**
   * Limit the number of points that can be passed through (i.e.,
   * sets the output sample size).
   */
  vtkSetClampMacro(MaximumNumberOfPoints, vtkIdType, 0, VTK_ID_MAX);
  vtkGetMacro(MaximumNumberOfPoints, vtkIdType);
  ///@}

  ///@{
  /**
   * Start sampling with this point. Ignored by certain random modes.
   */
  vtkSetClampMacro(Offset, vtkIdType, 0, VTK_ID_MAX);
  vtkGetMacro(Offset, vtkIdType);
  ///@}

  ///@{
  /**
   * Special flag causes randomization of point selection.
   */
  vtkSetMacro(RandomMode, bool);
  vtkGetMacro(RandomMode, bool);
  vtkBooleanMacro(RandomMode, bool);
  ///@}

  ///@{
  /**
   * Set/Get Seed used for generating a spatially uniform distributions.
   * default is 1.
   */
  vtkSetMacro(RandomSeed, int);
  vtkGetMacro(RandomSeed, int);
  ///@}

  ///@{
  /**
   * Special mode selector that switches between random mode types.
   * 0 - randomized strides: randomly strides through the data (default);
   * fairly certain that this is not a statistically random sample
   * because the output depends on the order of the input and
   * the input points do not have an equal chance to appear in the output
   * (plus Vitter's incremental random algorithms are more complex
   * than this, while not a proof it is good indication this isn't
   * a statistically random sample - the closest would be algorithm S)
   * 1 - random sample: create a statistically random sample using Vitter's
   * incremental algorithm D without A described in Vitter
   * "Faster Methods for Random Sampling", Communications of the ACM
   * Volume 27, Issue 7, 1984
   * (OnRatio and Offset are ignored) O(sample size)
   * 2 - spatially stratified random sample: create a spatially
   * stratified random sample using the first method described in
   * Woodring et al. "In-situ Sampling of a Large-Scale Particle
   * Simulation for Interactive Visualization and Analysis",
   * Computer Graphics Forum, 2011 (EuroVis 2011).
   * (OnRatio and Offset are ignored) O(N log N)
   * 3 - spatially uniform (bound based): point randomly sampled
   * using a point locator and random positions inside the bounds
   * of the data set.
   * 4 - spatially uniform (surface based): points randomly sampled
   * via an inverse transform on surface area of each cell.
   * Note that 3D cells are ignored.
   * 5 - spatially uniform (volume based): points randomly sampled via an
   * inverse transform on volume area of each cell.
   * Note that 2D cells are ignored.
   */
  vtkSetClampMacro(RandomModeType, int, RANDOMIZED_ID_STRIDES, UNIFORM_SPATIAL_VOLUME);
  vtkGetMacro(RandomModeType, int);
  ///@}

  ///@{
  /**
   * THIS ONLY WORKS WITH THE PARALLEL IMPLEMENTATION vtkPMaskPoints RUNNING
   * IN PARALLEL.
   * NOTHING WILL CHANGE IF THIS IS NOT THE PARALLEL vtkPMaskPoints.
   * Determines whether maximum number of points is taken per processor
   * (default) or if the maximum number of points is proportionally
   * taken across processors (i.e., number of points per
   * processor = points on a processor * maximum number of points /
   * total points across all processors).  In the first case,
   * the total number of points = maximum number of points *
   * number of processors.  In the second case, the total number of
   * points = maximum number of points.
   */
  vtkSetMacro(ProportionalMaximumNumberOfPoints, bool);
  vtkGetMacro(ProportionalMaximumNumberOfPoints, bool);
  vtkBooleanMacro(ProportionalMaximumNumberOfPoints, bool);
  ///@}

  ///@{
  /**
   * Generate output polydata vertices as well as points. A useful
   * convenience method because vertices are drawn (they are topology) while
   * points are not (they are geometry). By default this method is off.
   */
  vtkSetMacro(GenerateVertices, bool);
  vtkGetMacro(GenerateVertices, bool);
  vtkBooleanMacro(GenerateVertices, bool);
  ///@}

  ///@{
  /**
   * When vertex generation is enabled, by default vertices are produced
   * as multi-vertex cells (more than one per cell), if you wish to have
   * a single vertex per cell, enable this flag.
   */
  vtkSetMacro(SingleVertexPerCell, bool);
  vtkGetMacro(SingleVertexPerCell, bool);
  vtkBooleanMacro(SingleVertexPerCell, bool);
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
  vtkMaskPoints();
  ~vtkMaskPoints() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  int OnRatio = 2;         // every OnRatio point is on; all others are off.
  vtkIdType Offset = 0;    // or starting point id.
  bool RandomMode = false; // turn on/off randomization.
  int RandomSeed = 1;
  vtkIdType MaximumNumberOfPoints;
  bool GenerateVertices = false; // generate polydata verts
  bool SingleVertexPerCell = false;
  int RandomModeType = RANDOMIZED_ID_STRIDES;
  bool ProportionalMaximumNumberOfPoints = false;
  int OutputPointsPrecision = DEFAULT_PRECISION;

  virtual void InternalScatter(unsigned long*, unsigned long*, int, int) {}
  virtual void InternalGather(unsigned long*, unsigned long*, int, int) {}
  virtual void InternalBroadcast(double*, int, int) {}
  virtual void InternalGather(double*, double*, int, int) {}
  virtual int InternalGetNumberOfProcesses() { return 1; }
  virtual int InternalGetLocalProcessId() { return 0; }
  virtual void InternalSplitController(int, int) {}
  virtual void InternalResetController() {}
  virtual void InternalBarrier() {}

  unsigned long GetLocalSampleSize(vtkIdType, int);
  double GetLocalAreaFactor(double, int);

private:
  vtkMaskPoints(const vtkMaskPoints&) = delete;
  void operator=(const vtkMaskPoints&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
