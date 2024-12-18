// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStreamTracer
 * @brief   Streamline generator
 *
 * vtkStreamTracer is a filter that integrates a vector field to generate
 * streamlines. The integration is performed using a specified integrator,
 * by default Runge-Kutta2.
 *
 * vtkStreamTracer produces polylines as the output, with each cell (i.e.,
 * polyline) representing a streamline. The attribute values associated
 * with each streamline are stored in the cell data, whereas those
 * associated with streamline-points are stored in the point data.
 *
 * vtkStreamTracer supports forward (the default), backward, and combined
 * (i.e., BOTH) integration. The length of a streamline is governed by
 * specifying a maximum value either in physical arc length or in (local)
 * cell length. Otherwise, the integration terminates upon exiting the
 * flow field domain, or if the particle speed is reduced to a value less
 * than a specified terminal speed, or when a maximum number of steps is
 * completed. The specific reason for the termination is stored in a cell
 * array named ReasonForTermination.
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
 * The quality of streamline integration can be controlled by setting the
 * initial integration step (InitialIntegrationStep), particularly for
 * Runge-Kutta2 and Runge-Kutta4 (with a fixed step size), and in the case
 * of Runge-Kutta45 (with an adaptive step size and error control) the
 * minimum integration step, the maximum integration step, and the maximum
 * error. These steps are in either LENGTH_UNIT or CELL_LENGTH_UNIT while
 * the error is in physical arc length. For the former two integrators,
 * there is a trade-off between integration speed and streamline quality.
 *
 * The integration time, vorticity, rotation and angular velocity are stored
 * in point data arrays named "IntegrationTime", "Vorticity", "Rotation" and
 * "AngularVelocity", respectively (vorticity, rotation and angular velocity
 * are computed only when ComputeVorticity is on). All point data attributes
 * in the source dataset are interpolated on the new streamline points.
 *
 * vtkStreamTracer supports integration through any type of dataset. Thus if
 * the dataset contains 2D cells like polygons or triangles, the integration
 * is constrained to lie on the surface defined by 2D cells.
 *
 * The starting point, or the so-called 'seed', of a streamline may be set in
 * two different ways. Starting from global x-y-z "position" allows you to
 * start a single trace at a specified x-y-z coordinate. If you specify a
 * source object, traces will be generated from each point in the source that
 * is inside the dataset. Note that if the integration direction is BOTH,
 * then potentially 2N streamlines will be generated given N seed points.
 *
 * @note This class has been threaded using vtkSMPTools. Each separate
 * streamline (corresponding to the initial seeds) is processed in a
 * separate thread. Consequently, if threading is enabled and many
 * streamlines are generated, significant performance improvement is
 * possible.
 *
 * @note Field data is shallow copied to the output. When the input is a
 * composite data set, field data associated with the root block is shallow-
 * copied to the output vtkPolyData.
 *
 * @sa
 * vtkRibbonFilter vtkRuledSurfaceFilter vtkInitialValueProblemSolver
 * vtkRungeKutta2 vtkRungeKutta4 vtkRungeKutta45 vtkParticleTracerBase
 * vtkParticleTracer vtkParticlePathFilter vtkStreaklineFilter
 * vtkAbstractInterpolatedVelocityField vtkCompositeInterpolatedVelocityField
 * vtkAMRInterpolatedVelocityField vtkSMPTools vtkPStreamTracer
 */

#ifndef vtkStreamTracer_h
#define vtkStreamTracer_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#include "vtkDataSetAttributesFieldList.h" // Needed to identify common data arrays
#include "vtkInitialValueProblemSolver.h"  // Needed for constants

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractInterpolatedVelocityField;
class vtkCompositeDataSet;
class vtkDataArray;
class vtkDataSetAttributes;
class vtkDoubleArray;
class vtkExecutive;
class vtkGenericCell;
class vtkIdList;
class vtkIntArray;
class vtkPoints;

VTK_ABI_NAMESPACE_END
#include <vector> // for std::vector

// Helper struct to convert between different length scales.
VTK_ABI_NAMESPACE_BEGIN
struct VTKFILTERSFLOWPATHS_EXPORT vtkIntervalInformation
{
  double Interval;
  int Unit;

  static double ConvertToLength(double interval, int unit, double cellLength);
  static double ConvertToLength(vtkIntervalInformation& interval, double cellLength);
};

/**
 * Used to specify custom conditions which are evaluated to determine whether
 * a streamline should be terminated.
 *    clientdata is set by the client when setting up the callback.
 *    points is the array of points integrated so far.
 *    velocity velocity vector integrated to produce the streamline.
 *    integrationDirection FORWARD of BACKWARD
 * The function returns true if the streamline should be terminated
 * and false otherwise.
 */
typedef bool (*CustomTerminationCallbackType)(
  void* clientdata, vtkPoints* points, vtkDataArray* velocity, int integrationDirection);

class VTKFILTERSFLOWPATHS_EXPORT vtkStreamTracer : public vtkPolyDataAlgorithm
{
public:
  /**
   * Construct the object to start from position (0,0,0), with forward
   * integration, terminal speed 1.0E-12, vorticity computation on,
   * integration step size 0.5 (in cell length unit), maximum number
   * of steps 2000, using Runge-Kutta2, and maximum propagation 1.0
   * (in arc length unit).
   */
  static vtkStreamTracer* New();

  ///@{
  /**
   * Standard methods to obtain type information and print object state.
   */
  vtkTypeMacro(vtkStreamTracer, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Specify the starting point (seed) of a streamline in the global
   * coordinate system. Search must be performed to find the initial cell
   * from which to start integration.
   */
  vtkSetVector3Macro(StartPosition, double);
  vtkGetVector3Macro(StartPosition, double);
  ///@}

  ///@{
  /**
   * Specify the source object used to generate starting points (seeds).
   * Note that this method does not connect the pipeline. The algorithm will
   * work on the input data as it is without updating the producer of the data.
   * See SetSourceConnection for connecting the pipeline.
   */
  void SetSourceData(vtkDataSet* source);
  vtkDataSet* GetSource();
  ///@}

  /**
   * Specify the source object used to generate starting points (seeds).
   * This method connects to the pipeline: the Source will be updated
   * and the results used as streamline seeds.
   */
  void SetSourceConnection(vtkAlgorithmOutput* algOutput);

  // The previously-supported TIME_UNIT is excluded in this current
  // enumeration definition because the underlying step size is ALWAYS in
  // arc length unit (LENGTH_UNIT) while the 'real' time interval (virtual
  // for steady flows) that a particle actually takes to trave in a single
  // step is obtained by dividing the arc length by the LOCAL speed. The
  // overall elapsed time (i.e., the life span) of the particle is the sum
  // of those individual step-wise time intervals. The arc-length-to-time
  // conversion only occurs for vorticity computation and for generating a
  // point data array named 'IntegrationTime'.
  enum Units
  {
    LENGTH_UNIT = 1,
    CELL_LENGTH_UNIT = 2
  };

  enum Solvers
  {
    RUNGE_KUTTA2,
    RUNGE_KUTTA4,
    RUNGE_KUTTA45,
    NONE,
    UNKNOWN
  };

  enum ReasonForTermination
  {
    OUT_OF_DOMAIN = vtkInitialValueProblemSolver::OUT_OF_DOMAIN,
    NOT_INITIALIZED = vtkInitialValueProblemSolver::NOT_INITIALIZED,
    UNEXPECTED_VALUE = vtkInitialValueProblemSolver::UNEXPECTED_VALUE,
    OUT_OF_LENGTH = 4,
    OUT_OF_STEPS = 5,
    STAGNATION = 6,
    FIXED_REASONS_FOR_TERMINATION_COUNT
  };

  ///@{
  /**
   * Set/get the integrator type to be used for streamline generation.
   * The object passed is not actually used but is cloned with
   * NewInstance in the process of integration  (prototype pattern).
   * The default is Runge-Kutta2. The integrator can also be changed
   * using SetIntegratorType. The recognized solvers are:
   * RUNGE_KUTTA2  = 0
   * RUNGE_KUTTA4  = 1
   * RUNGE_KUTTA45 = 2
   */
  void SetIntegrator(vtkInitialValueProblemSolver*);
  vtkGetObjectMacro(Integrator, vtkInitialValueProblemSolver);
  void SetIntegratorType(int type);
  int GetIntegratorType();
  void SetIntegratorTypeToRungeKutta2() { this->SetIntegratorType(RUNGE_KUTTA2); }
  void SetIntegratorTypeToRungeKutta4() { this->SetIntegratorType(RUNGE_KUTTA4); }
  void SetIntegratorTypeToRungeKutta45() { this->SetIntegratorType(RUNGE_KUTTA45); }
  ///@}

  /**
   * Set the velocity field interpolator type to one that uses a point
   * locator to perform local spatial searching. Typically a point locator is
   * faster than searches with a cell locator, but it may not always find the
   * correct cells enclosing a point. This is particularly true with meshes
   * that are disjoint at seams, or abut meshes in an incompatible manner.
   * By default (and if a InterpolationPrototype is not set), a point locator
   * is used.
   */
  void SetInterpolatorTypeToDataSetPointLocator();

  /**
   * Set the velocity field interpolator type to one that uses a cell locator
   * to perform spatial searching. Using a cell locator should always return
   * the correct results, but it can be much slower that point locator-based
   * searches.
   */
  void SetInterpolatorTypeToCellLocator();

  ///@{
  /**
   * Specify the maximum length of a streamline expressed in LENGTH_UNIT.
   */
  vtkSetMacro(MaximumPropagation, double);
  vtkGetMacro(MaximumPropagation, double);
  ///@}

  /**
   * Specify a uniform integration step unit for MinimumIntegrationStep,
   * InitialIntegrationStep, and MaximumIntegrationStep. NOTE: The valid
   * unit is now limited to only LENGTH_UNIT (1) and CELL_LENGTH_UNIT (2),
   * EXCLUDING the previously-supported TIME_UNIT.
   */
  void SetIntegrationStepUnit(int unit);
  int GetIntegrationStepUnit() { return this->IntegrationStepUnit; }

  ///@{
  /**
   * Specify the Initial step size used for line integration, expressed in:
   * LENGTH_UNIT      = 1
   * CELL_LENGTH_UNIT = 2
   * (either the starting size for an adaptive integrator, e.g., RK45,
   * or the constant / fixed size for non-adaptive ones, i.e., RK2 and RK4)
   */
  vtkSetMacro(InitialIntegrationStep, double);
  vtkGetMacro(InitialIntegrationStep, double);
  ///@}

  ///@{
  /**
   * Specify the Minimum step size used for line integration, expressed in:
   * LENGTH_UNIT      = 1
   * CELL_LENGTH_UNIT = 2
   * (Only valid for an adaptive integrator, e.g., RK45)
   */
  vtkSetMacro(MinimumIntegrationStep, double);
  vtkGetMacro(MinimumIntegrationStep, double);
  ///@}

  ///@{
  /**
   * Specify the Maximum step size used for line integration, expressed in:
   * LENGTH_UNIT      = 1
   * CELL_LENGTH_UNIT = 2
   * (Only valid for an adaptive integrator, e.g., RK45)
   */
  vtkSetMacro(MaximumIntegrationStep, double);
  vtkGetMacro(MaximumIntegrationStep, double);
  ///@}

  ///@{
  /**
   * Specify the maximum error tolerated throughout streamline integration.
   */
  vtkSetMacro(MaximumError, double);
  vtkGetMacro(MaximumError, double);
  ///@}

  ///@{
  /**
   * Specify the maximum number of steps for integrating a streamline. Note
   * that the number of steps generated is always one greater than
   * MaximumNumberOfSteps. So if MaximumNumberOfSteps==0, then only one step
   * will be generated. This is useful for advection situations when the
   * stream tracer is to be propagated just one step at a time (e.g., see
   * vtkStreamSurface which depends on this behavior).
   */
  vtkSetMacro(MaximumNumberOfSteps, vtkIdType);
  vtkGetMacro(MaximumNumberOfSteps, vtkIdType);
  ///@}

  ///@{
  /**
   * Specify the terminal speed value, below which streamline integration is
   * terminated.
   */
  vtkSetMacro(TerminalSpeed, double);
  vtkGetMacro(TerminalSpeed, double);
  ///@}

  ///@{
  /**
   * Specify whether streamlines should be computed on a surface.
   * The input should contains only 2D planar cells for this option to work as expected.
   */
  vtkGetMacro(SurfaceStreamlines, bool);
  vtkSetMacro(SurfaceStreamlines, bool);
  vtkBooleanMacro(SurfaceStreamlines, bool);
  ///@}

  enum
  {
    FORWARD,
    BACKWARD,
    BOTH
  };

  enum
  {
    INTERPOLATOR_WITH_DATASET_POINT_LOCATOR,
    INTERPOLATOR_WITH_CELL_LOCATOR
  };

  ///@{
  /**
   * Specify whether the streamline is integrated in the upstream or
   * downstream direction, or in both directions. (If integrated in both
   * directions, two separate streamlines are generated, both of which which
   * start at the seed point with one traveling in the forward direction, and
   * one in the backward direction.)
   */
  vtkSetClampMacro(IntegrationDirection, int, FORWARD, BOTH);
  vtkGetMacro(IntegrationDirection, int);
  void SetIntegrationDirectionToForward() { this->SetIntegrationDirection(FORWARD); }
  void SetIntegrationDirectionToBackward() { this->SetIntegrationDirection(BACKWARD); }
  void SetIntegrationDirectionToBoth() { this->SetIntegrationDirection(BOTH); }
  ///@}

  ///@{
  /**
   * Turn on/off vorticity computation at streamline points
   * (necessary for generating proper stream-ribbons using the
   * vtkRibbonFilter).
   */
  vtkSetMacro(ComputeVorticity, bool);
  vtkGetMacro(ComputeVorticity, bool);
  ///@}

  ///@{
  /**
   * This can be used to scale the rate with which the streamribbons
   * twist. The default is 1.
   */
  vtkSetMacro(RotationScale, double);
  vtkGetMacro(RotationScale, double);
  ///@}

  /**
   * The object used to interpolate the velocity field during integration is
   * of the same class as this prototype. The performance of streamline
   * generations can be significantly affected by the choice of the
   * interpolator, particularly its use of the locator to use.
   *
   * For non AMR datasets, initialize a vtkCompositeInterpolatedVelocityField
   * and set the FindCellStrategyType.
   */
  void SetInterpolatorPrototype(vtkAbstractInterpolatedVelocityField* ivf);

  /**
   * Set the type of the velocity field interpolator to determine whether
   * INTERPOLATOR_WITH_DATASET_POINT_LOCATOR or INTERPOLATOR_WITH_CELL_LOCATOR
   * is employed for locating cells during streamline integration. The latter
   * (adopting vtkAbstractCellLocator sub-classes such as vtkCellLocator and
   * vtkModifiedBSPTree) is more robust than the former (through vtkDataSet /
   * vtkPointSet::FindCell() coupled with vtkPointLocator). However the former
   * can be much faster and produce adequate results.
   */
  void SetInterpolatorType(int interpType);

  ///@{
  /**
   * Force the filter to run stream tracer advection in serial. This affects
   * the filter only if more than one streamline is to be generated.
   */
  vtkGetMacro(ForceSerialExecution, bool);
  vtkSetMacro(ForceSerialExecution, bool);
  vtkBooleanMacro(ForceSerialExecution, bool);
  ///@}

  /**
   * Adds a custom termination callback.
   * callback is a function provided by the user that says if the streamline
   *         should be terminated.
   * clientdata user specific data passed to the callback.
   * reasonForTermination this value will be set in the ReasonForTermination cell
   *         array if the streamline is terminated by this callback.
   */
  void AddCustomTerminationCallback(
    CustomTerminationCallbackType callback, void* clientdata, int reasonForTermination);

  /** The following methods should not be called by the user. They serve as integration
   * bridges between this vtkStreamTracer class and classes defined and implemented in
   * anonymous namespace. */

  /**
   * Helper method to convert between length scales. Made public so internal threaded
   * classes in anonymous namespace can invoke the method.
   */
  void ConvertIntervals(
    double& step, double& minStep, double& maxStep, int direction, double cellLength);

  ///@{
  /**
   * Helper methods to generate normals on streamlines. Made public so internal threaded
   * classes in anonymous namespace can invoke the methods.
   */
  void GenerateNormals(vtkPolyData* output, double* firstNormal, const char* vecName);
  void CalculateVorticity(
    vtkGenericCell* cell, double pcoords[3], vtkDoubleArray* cellVectors, double vorticity[3]);
  ///@}

  ///@{
  /**
   * If true the filter considers that the whole seed source is available on all ranks.
   * Else the filter will aggregate all seed sources from all ranks and merge their points.
   *
   * This property only makes sense when the filter is parallelized and is a no-op for its
   * sequential version. However, this member function needs to be defined in this class to
   * maintain a uniform interface between vtkStreamTracer and its parallel override class,
   * vtkPStreamTracer.
   * Default is true.
   */
  vtkSetMacro(UseLocalSeedSource, bool);
  vtkGetMacro(UseLocalSeedSource, bool);
  vtkBooleanMacro(UseLocalSeedSource, bool);
  ///@}

protected:
  vtkStreamTracer();
  ~vtkStreamTracer() override;

  // Create a default executive.
  vtkExecutive* CreateDefaultExecutive() override;

  // hide the superclass' AddInput() from the user and the compiler
  void AddInput(vtkDataObject*)
  {
    vtkErrorMacro(<< "AddInput() must be called with a vtkDataSet not a vtkDataObject.");
  }

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

  void Integrate(vtkPointData* inputData, vtkPolyData* output, vtkDataArray* seedSource,
    vtkIdList* seedIds, vtkIntArray* integrationDirections,
    vtkAbstractInterpolatedVelocityField* func, int maxCellSize, int vecType,
    const char* vecFieldName, double& propagation, vtkIdType& numSteps, double& integrationTime,
    std::vector<CustomTerminationCallbackType>& customTerminationCallback,
    std::vector<void*>& customTerminationClientData, std::vector<int>& customReasonForTermination);

  double SimpleIntegrate(double seed[3], double lastPoint[3], double stepSize,
    vtkAbstractInterpolatedVelocityField* func);
  int CheckInputs(vtkAbstractInterpolatedVelocityField*& func, int* maxCellSize);

  bool GenerateNormalsInIntegrate;

  // starting from global x-y-z position
  double StartPosition[3];

  static const double EPSILON;
  double TerminalSpeed;

  // Used by subclasses, leave alone
  double LastUsedStepSize;

  double MaximumPropagation;
  double MinimumIntegrationStep;
  double MaximumIntegrationStep;
  double InitialIntegrationStep;

  int SetupOutput(vtkInformation* inInfo, vtkInformation* outInfo);
  void InitializeSeeds(vtkDataArray*& seeds, vtkIdList*& seedIds,
    vtkIntArray*& integrationDirections, vtkDataSet* source);

  int IntegrationStepUnit;
  int IntegrationDirection;

  // Prototype showing the integrator type to be set by the user.
  vtkInitialValueProblemSolver* Integrator;

  double MaximumError;
  vtkIdType MaximumNumberOfSteps;

  bool ComputeVorticity;
  double RotationScale;

  // Compute streamlines only on surface.
  bool SurfaceStreamlines;

  vtkAbstractInterpolatedVelocityField* InterpolatorPrototype;

  // These are used to manage complex input types such as
  // multiblock / composite datasets. Basically the filter input is
  // converted to a composite dataset, and the point data attributes
  // are intersected to produce a common set of output data arrays.
  vtkCompositeDataSet* InputData;        // convert input data to composite dataset
  vtkDataSetAttributesFieldList InputPD; // intersect attributes of all datasets
  bool
    HasMatchingPointAttributes; // does the point data in the multiblocks have the same attributes?

  // Control execution as serial or threaded
  bool ForceSerialExecution;
  bool SerialExecution; // internal use to combine information

  std::vector<CustomTerminationCallbackType> CustomTerminationCallback;
  std::vector<void*> CustomTerminationClientData;
  std::vector<int> CustomReasonForTermination;

  // Only relevant for this derived parallel version of vtkStreamTracer,
  // but needs to be defined in this class to have a uniform interface
  // between this class and the parallel override vtkPStreamTracer
  bool UseLocalSeedSource;

  friend class PStreamTracerUtils;

private:
  vtkStreamTracer(const vtkStreamTracer&) = delete;
  void operator=(const vtkStreamTracer&) = delete;
  int InterpolatorType = vtkStreamTracer::INTERPOLATOR_WITH_DATASET_POINT_LOCATOR;
};

VTK_ABI_NAMESPACE_END
#endif
