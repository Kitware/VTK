// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkEvenlySpacedStreamlines2D
 * @brief   Evenly spaced streamline generator for 2D.
 *
 * vtkEvenlySpacedStreamlines2D is a filter that integrates a 2D
 * vector field to generate evenly-spaced streamlines.
 *
 * We implement
 * the algorithm described in:
 * Jobard, Bruno, and Wilfrid Lefer. "Creating evenly-spaced
 * streamlines of arbitrary density." Visualization in Scientific
 * Computing '97. Springer Vienna, 1997. 43-55.
 * The loop detection is described in:
 * Liu, Zhanping, Robert Moorhead, and Joe Groner.
 * "An advanced evenly-spaced streamline placement algorithm."
 * IEEE Transactions on Visualization and Computer Graphics 12.5 (2006): 965-972.
 *
 * The integration is performed using a specified integrator,
 * by default Runge-Kutta2.
 *
 * vtkEvenlySpacedStreamlines2D produces polylines as the output, with
 * each cell (i.e., polyline) representing a streamline. The attribute
 * values associated with each streamline are stored in the cell data,
 * whereas those associated with streamline-points are stored in the
 * point data.
 *
 * vtkEvenlySpacedStreamlines2D integrates streamlines both forward
 * and backward. The integration for a streamline terminates upon
 * exiting the flow field domain, or if the particle speed is reduced
 * to a value less than a specified terminal speed, if the current
 * streamline gets too close to other streamlines
 * (vtkStreamTracer::FIXED_REASONS_FOR_TERMINATION_COUNT + 1) or if
 * the streamline forms a loop
 * (vtkStreamTracer::FIXED_REASONS_FOR_TERMINATION_COUNT). The
 * specific reason for the termination is stored in a cell array named
 * ReasonForTermination.
 *
 * Note that normalized vectors are adopted in streamline integration,
 * which achieves high numerical accuracy/smoothness of flow lines that is
 * particularly guaranteed for Runge-Kutta45 with adaptive step size and
 * error control). In support of this feature, the underlying step size is
 * ALWAYS in arc length unit (LENGTH_UNIT) while the 'real' time interval
 * (virtual for steady flows) that a particle actually takes to trave in a
 * single step is obtained by dividing the arc length by the LOCAL speed.
 * The overall elapsed time (i.e., the life span) of the particle is the
 * sum of those individual step-wise time intervals.
 *
 * The quality of streamline integration can be controlled by setting
 * the initial integration step (InitialIntegrationStep), particularly
 * for Runge-Kutta2 and Runge-Kutta4 (with a fixed step size). We do
 * not support Runge-Kutta45 (with an adaptive step size and error
 * control) because a requirement of the algorithm is that sample
 * points along a streamline be evenly spaced. These steps are in
 * either LENGTH_UNIT or CELL_LENGTH_UNIT.
 *
 * The integration time, vorticity, rotation and angular velocity are stored
 * in point data arrays named "IntegrationTime", "Vorticity", "Rotation" and
 * "AngularVelocity", respectively (vorticity, rotation and angular velocity
 * are computed only when ComputeVorticity is on). All point data attributes
 * in the source dataset are interpolated on the new streamline points.
 *
 * vtkEvenlySpacedStreamlines2D supports integration through any type of 2D dataset.
 *
 * The starting point, or the so-called 'seed', of the first streamline is set
 * by setting StartPosition
 *
 * @sa
 * vtkStreamTracer vtkRibbonFilter vtkRuledSurfaceFilter vtkInitialValueProblemSolver
 * vtkRungeKutta2 vtkRungeKutta4 vtkRungeKutta45 vtkParticleTracerBase
 * vtkParticleTracer vtkParticlePathFilter vtkStreaklineFilter
 * vtkAbstractInterpolatedVelocityField vtkCompositeInterpolatedVelocityField
 * vtkAMRInterpolatedVelocityField
 */

#ifndef vtkEvenlySpacedStreamlines2D_h
#define vtkEvenlySpacedStreamlines2D_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#include <array>  // for std::array
#include <vector> // for std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractInterpolatedVelocityField;
class vtkCompositeDataSet;
class vtkDataArray;
class vtkDoubleArray;
class vtkExecutive;
class vtkGenericCell;
class vtkIdList;
class vtkInitialValueProblemSolver;
class vtkImageData;
class vtkIntArray;
class vtkPolyDataCollection;
class vtkPoints;
class vtkStreamTracer;

class VTKFILTERSFLOWPATHS_EXPORT vtkEvenlySpacedStreamlines2D : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkEvenlySpacedStreamlines2D, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object to start from position (0,0,0), with forward
   * integration, terminal speed 1.0E-12, vorticity computation on,
   * integration step size 0.5 (in cell length unit), maximum number
   * of steps 2000, using Runge-Kutta2, and maximum propagation 1.0
   * (in arc length unit).
   */
  static vtkEvenlySpacedStreamlines2D* New();

  ///@{
  /**
   * Specify the starting point (seed) of the first streamline in the global
   * coordinate system. Search must be performed to find the initial cell
   * from which to start integration. If the seed is not specified a
   * random position in the input data is chosen.
   */
  vtkSetVector3Macro(StartPosition, double);
  vtkGetVector3Macro(StartPosition, double);
  ///@}

  ///@{
  /**
   * Set/get the integrator type to be used for streamline generation.
   * The object passed is not actually used but is cloned with
   * NewInstance in the process of integration  (prototype pattern).
   * The default is Runge-Kutta2. The integrator can also be changed
   * using SetIntegratorType. The recognized solvers are:
   * RUNGE_KUTTA2  = 0
   * RUNGE_KUTTA4  = 1
   */
  void SetIntegrator(vtkInitialValueProblemSolver*);
  vtkGetObjectMacro(Integrator, vtkInitialValueProblemSolver);
  void SetIntegratorType(int type);
  int GetIntegratorType();
  void SetIntegratorTypeToRungeKutta2();
  void SetIntegratorTypeToRungeKutta4();
  ///@}

  /**
   * Set the velocity field interpolator type to the one involving
   * a dataset point locator.
   */
  void SetInterpolatorTypeToDataSetPointLocator();

  /**
   * Set the velocity field interpolator type to the one involving
   * a cell locator.
   */
  void SetInterpolatorTypeToCellLocator();

  /**
   * Specify a uniform integration step unit for
   * InitialIntegrationStep, and SeparatingDistance. Valid units are
   * LENGTH_UNIT (1) (value is in global coordinates) and CELL_LENGTH_UNIT (2)
   * (the value is in number of cell lengths)
   */
  void SetIntegrationStepUnit(int unit);
  int GetIntegrationStepUnit() { return this->IntegrationStepUnit; }

  ///@{
  /**
   * Specify the maximum number of steps for integrating a streamline.
   */
  vtkSetMacro(MaximumNumberOfSteps, vtkIdType);
  vtkGetMacro(MaximumNumberOfSteps, vtkIdType);
  ///@}

  ///@{
  /**
   * We don't try to eliminate loops with fewer points than this. Default value
   * is 4.
   */
  vtkSetMacro(MinimumNumberOfLoopPoints, vtkIdType);
  vtkGetMacro(MinimumNumberOfLoopPoints, vtkIdType);
  ///@}

  ///@{
  /**
   * Specify the Initial step size used for line integration, expressed in
   * IntegrationStepUnit
   *
   * This is the constant / fixed size for non-adaptive integration
   * methods, i.e., RK2 and RK4
   */
  vtkSetMacro(InitialIntegrationStep, double);
  vtkGetMacro(InitialIntegrationStep, double);
  ///@}

  ///@{
  /**
   * Specify the separation distance between streamlines expressed in
   * IntegrationStepUnit.
   */
  vtkSetMacro(SeparatingDistance, double);
  vtkGetMacro(SeparatingDistance, double);
  ///@}

  ///@{
  /**
   * Streamline integration is stopped if streamlines are closer than
   * SeparatingDistance*SeparatingDistanceRatio to other streamlines.
   */
  vtkSetMacro(SeparatingDistanceRatio, double);
  vtkGetMacro(SeparatingDistanceRatio, double);
  ///@}

  ///@{
  /**
   * Loops are considered closed if the have two points at distance less than this.
   * This is expressed in IntegrationStepUnit.
   */
  vtkSetMacro(ClosedLoopMaximumDistance, double);
  vtkGetMacro(ClosedLoopMaximumDistance, double);
  ///@}

  ///@{
  /**
   * The angle (in radians) between the vector created by p0p1 and the
   * velocity in the point closing the loop. p0 is the current point
   * and p1 is the point before that.  Default value is 20 degrees in radians.
   */
  vtkSetMacro(LoopAngle, double);
  vtkGetMacro(LoopAngle, double);
  ///@}

  ///@{
  /**
   * Specify the terminal speed value, below which integration is terminated.
   */
  vtkSetMacro(TerminalSpeed, double);
  vtkGetMacro(TerminalSpeed, double);
  ///@}

  ///@{
  /**
   * Turn on/off vorticity computation at streamline points
   * (necessary for generating proper stream-ribbons using the
   * vtkRibbonFilter.
   */
  vtkSetMacro(ComputeVorticity, bool);
  vtkGetMacro(ComputeVorticity, bool);
  ///@}

  /**
   * The object used to interpolate the velocity field during
   * integration is of the same class as this prototype.
   */
  void SetInterpolatorPrototype(vtkAbstractInterpolatedVelocityField* ivf);

  /**
   * Set the type of the velocity field interpolator to determine whether
   * INTERPOLATOR_WITH_DATASET_POINT_LOCATOR or INTERPOLATOR_WITH_CELL_LOCATOR
   * is employed for locating cells during streamline integration. The latter
   * (adopting vtkAbstractCellLocator sub-classes such as vtkCellLocator and
   * vtkModifiedBSPTree) is more robust then the former (through vtkDataSet /
   * vtkPointSet::FindCell() coupled with vtkPointLocator).
   */
  void SetInterpolatorType(int interpType);

protected:
  vtkEvenlySpacedStreamlines2D();
  ~vtkEvenlySpacedStreamlines2D() override;

  /**
   * Do we test for separating distance or a ratio of the separating distance.
   */
  enum DistanceType
  {
    DISTANCE,
    DISTANCE_RATIO
  };
  // hide the superclass' AddInput() from the user and the compiler
  void AddInput(vtkDataObject*)
  {
    vtkErrorMacro(<< "AddInput() must be called with a vtkDataSet not a vtkDataObject.");
  }

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

  int SetupOutput(vtkInformation* inInfo, vtkInformation* outInfo);
  int CheckInputs(vtkAbstractInterpolatedVelocityField*& func, int* maxCellSize);
  double ConvertToLength(double interval, int unit, double cellLength);

  static void GetBounds(vtkCompositeDataSet* cds, double bounds[6]);
  void InitializeSuperposedGrid(double* bounds);
  void AddToAllPoints(vtkPolyData* streamline);
  void AddToCurrentPoints(vtkIdType pointId);
  template <typename T>
  void InitializePoints(T& points);
  void InitializeMinPointIds();

  static bool IsStreamlineLooping(
    void* clientdata, vtkPoints* points, vtkDataArray* velocity, int direction);
  static bool IsStreamlineTooCloseToOthers(
    void* clientdata, vtkPoints* points, vtkDataArray* velocity, int direction);
  template <typename CellCheckerType>
  bool ForEachCell(double* point, CellCheckerType checker, vtkPoints* points = nullptr,
    vtkDataArray* velocity = nullptr, int direction = 1);
  template <int distanceType>
  bool IsTooClose(
    double* point, vtkIdType cellId, vtkPoints* points, vtkDataArray* velocity, int direction);
  bool IsLooping(
    double* point, vtkIdType cellId, vtkPoints* points, vtkDataArray* velocity, int direction);
  const char* GetInputArrayToProcessName();
  int ComputeCellLength(double* cellLength);

  // starting from global x-y-z position
  double StartPosition[3];

  double TerminalSpeed;

  double InitialIntegrationStep;
  double SeparatingDistance;
  // SeparatingDistance can be in cell length or arc length. This member
  // stores SeparatingDistance in arc length. It is computed when
  // the filter executes.
  double SeparatingDistanceArcLength;
  double SeparatingDistanceRatio;
  double ClosedLoopMaximumDistance;
  // ClosedLoopMaximumDistance can be in cell length or arc length.
  // This member stores ClosedLoopMaximumDistance in arc length. It is
  // computed when the filter executes.
  double ClosedLoopMaximumDistanceArcLength;
  double LoopAngle;
  int IntegrationStepUnit;

  vtkIdType MaximumNumberOfSteps;
  vtkIdType MinimumNumberOfStreamlinePoints;
  vtkIdType MinimumNumberOfLoopPoints;

  // Prototype showing the integrator type to be set by the user.
  vtkInitialValueProblemSolver* Integrator;

  bool ComputeVorticity;

  vtkAbstractInterpolatedVelocityField* InterpolatorPrototype;

  vtkCompositeDataSet* InputData;
  // grid superposed over InputData. The grid cell height and width is
  // SeparatingDistance
  vtkImageData* SuperposedGrid;
  // AllPoints[i][j] is the point for point j on the streamlines that
  // falls over cell id i in SuperposedGrid. AllPoint[i].size() tell
  // us how many points fall over cell id i.
  std::vector<std::vector<std::array<double, 3>>> AllPoints;

  // CurrentPoints[i][j] is the point id for point j on the current streamline that
  // falls over cell id i in SuperposedGrid. CurrentPoints[i].size() tell us
  // how many points fall over cell id i.
  std::vector<std::vector<vtkIdType>> CurrentPoints;
  // Min and Max point ids stored in a cell of SuperposedGrid
  std::vector<vtkIdType> MinPointIds;
  // The index of the first point for the current
  // direction. Note we integrate streamlines both forward and
  // backward.
  vtkIdType DirectionStart;
  // The previous integration direction.
  int PreviousDirection;

  // queue of streamlines to be processed
  vtkPolyDataCollection* Streamlines;

private:
  vtkEvenlySpacedStreamlines2D(const vtkEvenlySpacedStreamlines2D&) = delete;
  void operator=(const vtkEvenlySpacedStreamlines2D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
