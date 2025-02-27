// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkParticleTracerBase
 * @brief   A particle tracer for vector fields
 *
 * vtkParticleTracerBase is the base class for filters that advect particles
 * in a vector field. Note that the input vtkPointData structure must
 * be identical on all datasets.
 *
 * @sa
 * vtkRibbonFilter vtkRuledSurfaceFilter vtkInitialValueProblemSolver
 * vtkRungeKutta2 vtkRungeKutta4 vtkRungeKutta45 vtkStreamTracer
 */

#ifndef vtkParticleTracerBase_h
#define vtkParticleTracerBase_h

#include "vtkDeprecation.h"            // For VTK_DEPRECATED_IN_9_4_0
#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"
#include "vtkSmartPointer.h"      // For vtkSmartPointer
#include "vtkTemporalAlgorithm.h" // For vtkTemporalAlgorithm
#include "vtkWeakPointer.h"       // For vtkWeakPointer

#include <list>          // STL Header
#include <mutex>         // STL Header
#include <numeric>       // STL Header
#include <unordered_map> // STL Header
#include <vector>        // STL Header

#ifndef __VTK_WRAP__
#define vtkPolyDataAlgorithm vtkTemporalAlgorithm<vtkPolyDataAlgorithm>
#endif

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractParticleWriter;
class vtkCompositeDataSet;
class vtkDataArray;
class vtkDataSet;
class vtkDoubleArray;
class vtkFloatArray;
class vtkGenericCell;
class vtkInitialValueProblemSolver;
class vtkIntArray;
class vtkMultiBlockDataSet;
class vtkMultiProcessController;
class vtkPartitionedDataSet;
class vtkPointData;
class vtkPoints;
class vtkPolyData;
class vtkSignedCharArray;
class vtkTemporalInterpolatedVelocityField;
VTK_ABI_NAMESPACE_END

namespace vtkParticleTracerBaseNamespace
{
VTK_ABI_NAMESPACE_BEGIN
struct Position_t
{
  double x[4];
};
using Position = struct Position_t;

struct ParticleInformation_t
{
  // These are used during iteration
  Position CurrentPosition;
  int CachedDataSetId[2];
  vtkIdType CachedCellId[2];
  int LocationState;
  // These are computed scalars we might display
  int SourceID;
  int TimeStepAge; // amount of time steps the particle has advanced
  int InjectedPointId;
  int InjectedStepId; // time step the particle was injected
  double SimulationTime;
  // These are useful to track for debugging etc
  int ErrorCode;
  float age;
  // these are needed across time steps to compute vorticity
  float rotation;
  float angularVel;
  float time;
  float speed;
  // once the particle is added, PointId is valid and is the tuple location
  // in ProtoPD.
  vtkIdType PointId;

  double velocity[3];
};
using ParticleInformation = struct ParticleInformation_t;

typedef std::vector<ParticleInformation> ParticleVector;
typedef ParticleVector::iterator ParticleIterator;
typedef std::list<ParticleInformation> ParticleDataList;
typedef ParticleDataList::iterator ParticleListIterator;
struct ParticleTracerFunctor;
VTK_ABI_NAMESPACE_END
}

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSFLOWPATHS_EXPORT vtkParticleTracerBase : public vtkPolyDataAlgorithm
{
public:
  friend struct vtkParticleTracerBaseNamespace::ParticleTracerFunctor;
  enum Solvers
  {
    RUNGE_KUTTA2,
    RUNGE_KUTTA4,
    RUNGE_KUTTA45,
    NONE,
    UNKNOWN
  };

  vtkTypeMacro(vtkParticleTracerBase, vtkPolyDataAlgorithm);
  void PrintParticleHistories();

#ifndef __VTK_WRAP__
#undef vtkPolyDataAlgorithm
#endif
  ///@{
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

#if defined(__VTK_WRAP__) || defined(__WRAP_GCCXML)
  vtkCreateWrappedTemporalAlgorithmInterface();
#endif

  ///@{
  /**
   * Get/Set the controller to use. By default
   * vtkMultiProcessController::GlobalController will be used.
   */
  virtual void SetController(vtkMultiProcessController*);
  virtual vtkMultiProcessController* GetController();
  ///@}

  ///@{
  /**
   * Turn on/off vorticity computation at streamline points
   * (necessary for generating proper stream-ribbons using the
   * vtkRibbonFilter.
   */
  vtkGetMacro(ComputeVorticity, bool);
  void SetComputeVorticity(bool);
  ///@}

  ///@{
  /**
   * Specify the terminal speed value, below which integration is terminated.
   */
  vtkGetMacro(TerminalSpeed, double);
  void SetTerminalSpeed(double);
  ///@}

  ///@{
  /**
   * This can be used to scale the rate with which the streamribbons
   * twist. The default is 1.
   */
  vtkGetMacro(RotationScale, double);
  void SetRotationScale(double);
  ///@}

  ///@{
  /**
   * To get around problems with the Paraview Animation controls
   * we can just animate the time step and ignore the TIME_ requests
   */
  vtkSetMacro(IgnorePipelineTime, vtkTypeBool);
  vtkGetMacro(IgnorePipelineTime, vtkTypeBool);
  vtkBooleanMacro(IgnorePipelineTime, vtkTypeBool);
  ///@}

  ///@{
  /**
   * When animating particles, it is nice to inject new ones every Nth step
   * to produce a continuous flow. Setting ForceReinjectionEveryNSteps to a
   * non zero value will cause the particle source to reinject particles
   * every Nth step even if it is otherwise unchanged.
   * Note that if the particle source is also animated, this flag will be
   * redundant as the particles will be reinjected whenever the source changes
   * anyway
   */
  vtkGetMacro(ForceReinjectionEveryNSteps, int);
  void SetForceReinjectionEveryNSteps(int);
  ///@}

  ///@{
  /**
   * @deprecated This does not do anything. Someone wanting to amend the time series
   * early should select a subset of the time steps
   * by propagating upstream the information key `TIME_STEPS`
   * in `vtkAlgorithm::RequestUpdateExtent`.
   */
  VTK_DEPRECATED_IN_9_4_0(
    "Please edit the TIME_STEPS information key in vtkAlgorithm::RequestInformation() instead")
  virtual void SetTerminationTime(double) {}
  VTK_DEPRECATED_IN_9_4_0(
    "Please edit the TIME_STEPS information key in vtkAlgorithm::RequestInformation() instead")
  virtual double GetTerminationTime() { return std::numeric_limits<double>::quiet_NaN(); }
  VTK_DEPRECATED_IN_9_4_0(
    "Please edit the TIME_STEPS information key in vtkAlgorithm::RequestInformation() instead")
  virtual void SetStartTime(double) {}
  VTK_DEPRECATED_IN_9_4_0(
    "Please edit the TIME_STEPS information key in vtkAlgorithm::RequestInformation() instead")
  virtual double GetStartTime() { return std::numeric_limits<double>::quiet_NaN(); }
  ///@}

  ///@{
  /**
   * @deprecated Caching is now automated.
   */
  VTK_DEPRECATED_IN_9_4_0("Caching is now automated")
  virtual void SetDisableResetCache(bool) {}
  VTK_DEPRECATED_IN_9_4_0("Caching is now automated")
  virtual bool GetDisableResetCache() { return false; }
  VTK_DEPRECATED_IN_9_4_0("Caching is now automated")
  virtual void DisableResetCacheOn() {}
  VTK_DEPRECATED_IN_9_4_0("Caching is now automated")
  virtual void DisableResetCacheOff() {}
  ///@}

  void SetIntegrator(vtkInitialValueProblemSolver*);
  vtkGetObjectMacro(Integrator, vtkInitialValueProblemSolver);

  void SetIntegratorType(int type);
  int GetIntegratorType();

  ///@{
  /**
   * if StaticSeeds is set and the mesh is static,
   * then every time particles are injected we can reuse the same
   * injection information. We classify particles according to
   * processor just once before start.
   * If StaticSeeds is set and a moving seed source is specified
   * the motion will be ignored and results will not be as expected.
   * The default is that StaticSeeds is 0.
   */
  vtkSetMacro(StaticSeeds, vtkTypeBool);
  vtkGetMacro(StaticSeeds, vtkTypeBool);
  ///@}

  /**
   * Types of Variance of Mesh over time
   */
  enum MeshOverTimeTypes
  {
    DIFFERENT = 0,
    STATIC = 1,
    LINEAR_TRANSFORMATION = 2,
    SAME_TOPOLOGY = 3
  };

  ///@{
  /*
   * Set/Get the type of variance of the mesh over time.
   *
   * DIFFERENT = 0,
   * STATIC = 1,
   * LINEAR_TRANSFORMATION = 2
   * SAME_TOPOLOGY = 3
   */
  virtual void SetMeshOverTime(int meshOverTime);
  virtual int GetMeshOverTimeMinValue() { return DIFFERENT; }
  virtual int GetMeshOverTimeMaxValue() { return SAME_TOPOLOGY; }
  void SetMeshOverTimeToDifferent() { this->SetMeshOverTime(DIFFERENT); }
  void SetMeshOverTimeToStatic() { this->SetMeshOverTime(STATIC); }
  void SetMeshOverTimeToLinearTransformation() { this->SetMeshOverTime(LINEAR_TRANSFORMATION); }
  void SetMeshOverTimeToSameTopology() { this->SetMeshOverTime(SAME_TOPOLOGY); }
  vtkGetMacro(MeshOverTime, int);
  ///@}

  enum
  {
    INTERPOLATOR_WITH_DATASET_POINT_LOCATOR,
    INTERPOLATOR_WITH_CELL_LOCATOR
  };

  /**
   * Set the type of the velocity field interpolator to determine whether
   * INTERPOLATOR_WITH_DATASET_POINT_LOCATOR or INTERPOLATOR_WITH_CELL_LOCATOR
   * is employed for locating cells during streamline integration. The latter
   * (adopting vtkAbstractCellLocator sub-classes such as vtkCellLocator and
   * vtkModifiedBSPTree) is more robust than the former (through vtkDataSet /
   * vtkPointSet::FindCell() coupled with vtkPointLocator). However the former
   * can be much faster and produce adequate results.
   *
   * Default is INTERPOLATOR_WITH_CELL_LOCATOR (to maintain backwards compatibility).
   */
  void SetInterpolatorType(int interpolatorType);

  /**
   * Set the velocity field interpolator type to one that uses a point
   * locator to perform local spatial searching. Typically a point locator is
   * faster than searches with a cell locator, but it may not always find the
   * correct cells enclosing a point. This is particularly true with meshes
   * that are disjoint at seams, or abut meshes in an incompatible manner.
   */
  void SetInterpolatorTypeToDataSetPointLocator();

  /**
   * Set the velocity field interpolator type to one that uses a cell locator
   * to perform spatial searching. Using a cell locator should always return
   * the correct results, but it can be much slower that point locator-based
   * searches. * By default a cell locator is used.
   */
  void SetInterpolatorTypeToCellLocator();

  ///@{
  /**
   * Set/Get the Writer associated with this Particle Tracer
   * Ideally a parallel IO capable vtkH5PartWriter should be used
   * which will collect particles from all parallel processes
   * and write them to a single HDF5 file.
   */
  virtual void SetParticleWriter(vtkAbstractParticleWriter* pw);
  vtkGetObjectMacro(ParticleWriter, vtkAbstractParticleWriter);
  ///@}

  ///@{
  /**
   * Set/Get the filename to be used with the particle writer when
   * dumping particles to disk
   */
  vtkSetFilePathMacro(ParticleFileName);
  vtkGetFilePathMacro(ParticleFileName);
  ///@}

  ///@{
  /**
   * Set/Get the filename to be used with the particle writer when
   * dumping particles to disk
   */
  vtkSetMacro(EnableParticleWriting, vtkTypeBool);
  vtkGetMacro(EnableParticleWriting, vtkTypeBool);
  vtkBooleanMacro(EnableParticleWriting, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Provide support for multiple seed sources
   */
  void AddSourceConnection(vtkAlgorithmOutput* input);
  void RemoveAllSources();
  ///@}

  ///@{
  /**
   * Force the filter to run particle tracer in serial. This affects
   * the filter only if more than 100 particles is to be generated.
   */
  vtkGetMacro(ForceSerialExecution, bool);
  vtkSetMacro(ForceSerialExecution, bool);
  vtkBooleanMacro(ForceSerialExecution, bool);
  ///@}
protected:
  ///@{
  /**
   * ProtoPD is used just to keep track of the input array names and number of components
   * for copy allocating from other vtkPointDatas where the data is really stored
   */
  vtkSmartPointer<vtkPointData> ProtoPD;
  vtkParticleTracerBaseNamespace::ParticleDataList ParticleHistories;
  vtkTypeBool IgnorePipelineTime; // whether to use the pipeline time for termination
  ///@}

  // Control execution as serial or threaded
  bool ForceSerialExecution;

  void EnqueueParticleToAnotherProcess(vtkParticleTracerBaseNamespace::ParticleInformation&);

  vtkParticleTracerBase();
  ~vtkParticleTracerBase() override;

  //
  // Make sure the pipeline knows what type we expect as input
  //
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Resets internal cache for a clean start.
   */
  int Initialize(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Moves the particles one time step further.
   * When this routine has finished, `OutputPointData`, `OutputCoordinates` and `ParticleHistories`
   * represent the location / point data / meta data of all particles present in the local rank.
   * `MPIRecvList` represents, at this stage, the list of particles that were received during
   * this time step.
   */
  int Execute(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Generates an output using the data provided after `Execute` was ran.
   */
  int Finalize(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Method to get the data set seed sources.
   * For in situ we want to override how the seed sources are made available.
   */
  virtual std::vector<vtkDataSet*> GetSeedSources(vtkInformationVector* inputVector);

  // Initialization of input (vector-field) geometry
  int InitializeInterpolator();

  /**
   * All ranks have the same representation of the seeds. They are gathered to all processes in the
   * same order.
   */
  vtkSmartPointer<vtkDataSet> Seeds;

  /**
   * inside our data. Add good ones to passed list and set count to the
   * number that passed
   */
  void TestParticles(vtkParticleTracerBaseNamespace::ParticleVector& candidates,
    vtkParticleTracerBaseNamespace::ParticleVector& passed);

  void TestParticles(
    vtkParticleTracerBaseNamespace::ParticleVector& candidates, std::vector<int>& passed);

  /**
   * all the injection/seed points according to which processor
   * they belong to. This saves us retesting at every injection time
   * providing 1) The volumes are static, 2) the seed points are static
   * If either are non static, then this step is skipped.
   */
  virtual void AssignSeedsToProcessors(double time, vtkDataSet* source,
    vtkParticleTracerBaseNamespace::ParticleVector& localSeedPoints);

  /**
   * this is used during classification of seed points and also between iterations
   * of the main loop as particles leave each processor domain. Returns
   * true if particles were migrated to any new process.
   */
  bool SendReceiveParticles(std::vector<vtkIdType>&);

  /**
   * and sending between processors, into a list, which is used as the master
   * list on this processor
   */
  void UpdateParticleList(vtkParticleTracerBaseNamespace::ParticleVector& candidates);

  /**
   * this is used during classification of seed points and also between iterations
   * of the main loop as particles leave each processor domain. Returns true
   * if particles moved between processes and false otherwise.
   */
  virtual bool UpdateParticleListFromOtherProcesses();

  /**
   * particle between the two times supplied.
   */
  void IntegrateParticle(vtkParticleTracerBaseNamespace::ParticleListIterator& it,
    double currentTime, double targetTime, vtkInitialValueProblemSolver* integrator,
    vtkTemporalInterpolatedVelocityField* interpolator, vtkDoubleArray* cellVectors,
    std::atomic<vtkIdType>& particleCount, std::mutex& eraseMutex, bool sequential);

  /**
   * This is an old routine kept for possible future use.
   * In dynamic meshes, particles might leave the domain and need to be extrapolated across
   * a gap between the meshes before they re-renter another domain
   * dodgy rotating meshes need special care....
   */
  bool ComputeDomainExitLocation(
    double pos[4], double p2[4], double intersection[4], vtkGenericCell* cell);

  //
  // Scalar arrays that are generated as each particle is updated
  //
  void CreateProtoPD(vtkDataObject* input);

  vtkFloatArray* GetParticleAge(vtkPointData*);
  vtkIntArray* GetParticleIds(vtkPointData*);
  vtkSignedCharArray* GetParticleSourceIds(vtkPointData*);
  vtkIntArray* GetInjectedPointIds(vtkPointData*);
  vtkIntArray* GetInjectedStepIds(vtkPointData*);
  vtkIntArray* GetErrorCodeArr(vtkPointData*);
  vtkFloatArray* GetParticleVorticity(vtkPointData*);
  vtkFloatArray* GetParticleRotation(vtkPointData*);
  vtkFloatArray* GetParticleAngularVel(vtkPointData*);

  // utility function we use to test if a point is inside any of our local datasets
  bool InsideBounds(double point[]);

  void CalculateVorticity(
    vtkGenericCell* cell, double pcoords[3], vtkDoubleArray* cellVectors, double vorticity[3]);

  //------------------------------------------------------

  void SetParticle(vtkParticleTracerBaseNamespace::ParticleInformation& info,
    vtkTemporalInterpolatedVelocityField* interpolator, vtkDoubleArray* cellVectors);

  std::unordered_map<vtkIdType, int> InjectedPointIdToProcessId;

  ///@{
  /**
   * Methods that check that the input arrays are ordered the
   * same on all data sets. This needs to be true for all
   * blocks in a composite data set as well as across all processes.
   */
  virtual bool IsPointDataValid(vtkDataObject* input);
  bool IsPointDataValid(vtkCompositeDataSet* input, std::vector<std::string>& arrayNames);
  void GetPointDataArrayNames(vtkDataSet* input, std::vector<std::string>& names);
  ///@}

  vtkGetMacro(ReinjectionCounter, int);

  void ResizeArrays(vtkIdType numTuples);

  /**
   * Methods to append values to existing point data arrays that may
   * only be desired on specific concrete derived classes.
   */
  virtual void InitializeExtraPointDataArrays(vtkPointData* vtkNotUsed(outputPD)) {}

  virtual void SetToExtraPointDataArrays(
    vtkIdType, vtkParticleTracerBaseNamespace::ParticleInformation&)
  {
  }

  vtkTemporalInterpolatedVelocityField* GetInterpolator();

  /**
   * For restarts of particle paths, we add in the ability to add in
   * particles from a previous computation that we will still advect.
   */
  virtual void AddRestartSeeds(vtkInformationVector** /*inputVector*/) {}

  /**
   * When particles leave the domain, they must be collected
   * and sent to the other processes for possible continuation.
   * These routines manage the collection and sending after each main iteration.
   * RetryWithPush adds a small push to a particle along it's current velocity
   * vector, this helps get over cracks in dynamic/rotating meshes. This is a
   * first order integration though so it may introduce a bit extra error compared
   * to the integrator that is used.
   */
  bool RetryWithPush(vtkParticleTracerBaseNamespace::ParticleInformation& info, double* point1,
    double delT, int subSteps, vtkTemporalInterpolatedVelocityField* interpolator);

  bool SetTerminationTimeNoModify(double t);

  double CachedTimeStep;

  // Parameters of tracing
  vtkInitialValueProblemSolver* Integrator;
  double IntegrationStep;
  double MaximumError;
  bool ComputeVorticity;
  double RotationScale;
  double TerminalSpeed;

  // A counter to keep track of how many times we reinjected
  int ReinjectionCounter;

  // Important for Caching of Cells/Ids/Weights etc
  vtkTypeBool AllFixedGeometry;
  int MeshOverTime;
  vtkTypeBool StaticSeeds;

  // Innjection parameters
  int ForceReinjectionEveryNSteps;
  vtkTimeStamp ParticleInjectionTime;
  bool HasCache;

  // Particle writing to disk
  vtkAbstractParticleWriter* ParticleWriter;
  char* ParticleFileName;
  vtkTypeBool EnableParticleWriting;

  vtkParticleTracerBaseNamespace::ParticleVector LocalSeeds;

  // The velocity interpolator
  vtkSmartPointer<vtkTemporalInterpolatedVelocityField> Interpolator;

  // MPI controller needed when running in parallel
  vtkSmartPointer<vtkMultiProcessController> Controller;

  /**
   * Storage of the particles we want to send to another rank.
   * This storage is deleted upon exiting `Execute`.
   */
  vtkParticleTracerBaseNamespace::ParticleVector MPISendList;

  /**
   * Storage of the particles we received. After `Execute` terminates, it stores
   * the set of particles that were not present at the beginning of the execution.
   * The received particles are indexed using `InjectedPointId`, which
   * is a unique identifier. There are no 2 particles across ranks that have the same
   * `InjectedPointId`.
   */
  std::unordered_map<vtkIdType, vtkParticleTracerBaseNamespace::ParticleInformation> MPIRecvList;

  // Cache bounds info for each dataset we will use repeatedly
  struct bounds_t
  {
    double b[6];
  };
  using bounds = struct bounds_t;
  std::vector<bounds> CachedBounds[2];

  // variables used by Execute() to produce output

  vtkSmartPointer<vtkDataSet> DataReferenceT[2];

  vtkNew<vtkPoints> OutputCoordinates;
  vtkNew<vtkIdTypeArray> ParticleCellsConnectivity;

  vtkNew<vtkFloatArray> ParticleAge;
  vtkNew<vtkIntArray> ParticleIds;
  vtkNew<vtkSignedCharArray> ParticleSourceIds;
  vtkNew<vtkIdTypeArray> InjectedPointIds;
  vtkNew<vtkIntArray> InjectedStepIds;
  vtkNew<vtkIntArray> ErrorCodeArray;
  vtkNew<vtkFloatArray> ParticleVorticity;
  vtkNew<vtkFloatArray> ParticleRotation;
  vtkNew<vtkFloatArray> ParticleAngularVel;
  vtkNew<vtkPointData> OutputPointData;

  // temp array
  vtkNew<vtkDoubleArray> CellVectors;

  vtkParticleTracerBase(const vtkParticleTracerBase&) = delete;
  void operator=(const vtkParticleTracerBase&) = delete;
  vtkTimeStamp ExecuteTime;

  unsigned int NumberOfParticles();

  friend class ParticlePathFilterInternal;
  friend class StreaklineFilterInternal;

  static const double Epsilon;

private:
  // Internal method to initialize CachedData[1] using the provided input
  void InitializeNextCachedData(vtkDataObject* input);

  // Data for time step CurrentTimeStep-1 and CurrentTimeStep
  vtkSmartPointer<vtkPartitionedDataSet> CachedData[2];
};

VTK_ABI_NAMESPACE_END
#endif
