/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParticleTracerBase.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkSmartPointer.h" // For protected ivars.
#include "vtkPolyDataAlgorithm.h"

#include <vector> // STL Header
#include <list>   // STL Header

class vtkAbstractInterpolatedVelocityField;
class vtkAbstractParticleWriter;
class vtkCellArray;
class vtkCharArray;
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
class vtkPointData;
class vtkPoints;
class vtkPolyData;
class vtkTemporalInterpolatedVelocityField;

namespace vtkParticleTracerBaseNamespace
{
  typedef struct { double x[4]; } Position;
  typedef struct {
    // These are used during iteration
    Position      CurrentPosition;
    int           CachedDataSetId[2];
    vtkIdType     CachedCellId[2];
    int           LocationState;
    // These are computed scalars we might display
    int           SourceID;
    int           TimeStepAge; // amount of time steps the particle has advanced
    int           InjectedPointId;
    int           InjectedStepId;  // time step the particle was injected
    int           UniqueParticleId;
    double        SimulationTime;
    // These are useful to track for debugging etc
    int           ErrorCode;
    float         age;
    // these are needed across time steps to compute vorticity
    float         rotation;
    float         angularVel;
    float         time;
    float         speed;
    // once the partice is added, PointId is valid and is the tuple location
    // in ProtoPD.
    vtkIdType     PointId;
    // if PointId is negative then in parallel this particle was just
    // received and we need to get the tuple value from vtkPParticleTracerBase::Tail.
    vtkIdType     TailPointId;
  } ParticleInformation;

  typedef std::vector<ParticleInformation>  ParticleVector;
  typedef ParticleVector::iterator             ParticleIterator;
  typedef std::list<ParticleInformation>    ParticleDataList;
  typedef ParticleDataList::iterator           ParticleListIterator;
};

class VTKFILTERSFLOWPATHS_EXPORT vtkParticleTracerBase : public vtkPolyDataAlgorithm
{
public:
  enum Solvers
  {
    RUNGE_KUTTA2,
    RUNGE_KUTTA4,
    RUNGE_KUTTA45,
    NONE,
    UNKNOWN
  };

  vtkTypeMacro(vtkParticleTracerBase,vtkPolyDataAlgorithm)
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  void PrintParticleHistories();

  //@{
  /**
   * Turn on/off vorticity computation at streamline points
   * (necessary for generating proper stream-ribbons using the
   * vtkRibbonFilter.
   */
  vtkGetMacro(ComputeVorticity, bool);
  void SetComputeVorticity(bool);
  //@}

  //@{
  /**
   * Specify the terminal speed value, below which integration is terminated.
   */
  vtkGetMacro(TerminalSpeed, double);
  void SetTerminalSpeed(double);
  //@}

  //@{
  /**
   * This can be used to scale the rate with which the streamribbons
   * twist. The default is 1.
   */
  vtkGetMacro(RotationScale, double);
  void SetRotationScale(double);
  //@}

  //@{
  /**
   * To get around problems with the Paraview Animation controls
   * we can just animate the time step and ignore the TIME_ requests
   */
  vtkSetMacro(IgnorePipelineTime, int);
  vtkGetMacro(IgnorePipelineTime, int);
  vtkBooleanMacro(IgnorePipelineTime, int);
  //@}

  //@{
  /**
   * When animating particles, it is nice to inject new ones every Nth step
   * to produce a continuous flow. Setting ForceReinjectionEveryNSteps to a
   * non zero value will cause the particle source to reinject particles
   * every Nth step even if it is otherwise unchanged.
   * Note that if the particle source is also animated, this flag will be
   * redundant as the particles will be reinjected whenever the source changes
   * anyway
   */
  vtkGetMacro(ForceReinjectionEveryNSteps,int);
  void SetForceReinjectionEveryNSteps(int);
  //@}

  //@{
  /**
   * Setting TerminationTime to a positive value will cause particles
   * to terminate when the time is reached. Use a vlue of zero to
   * diable termination. The units of time should be consistent with the
   * primary time variable.
   */
  void SetTerminationTime(double t);
  vtkGetMacro(TerminationTime,double);
  //@}

  void SetIntegrator(vtkInitialValueProblemSolver *);
  vtkGetObjectMacro ( Integrator, vtkInitialValueProblemSolver );

  void SetIntegratorType(int type);
  int GetIntegratorType();

  //@{
  /**
   * Set the time value for particle tracing to begin. The units of time should
   * be consistent with the primary time variable.
   */
  vtkGetMacro(StartTime, double);
  void SetStartTime(double t);
  //@}

  //@{
  /**
   * if StaticSeeds is set and the mesh is static,
   * then every time particles are injected we can re-use the same
   * injection information. We classify particles according to
   * processor just once before start.
   * If StaticSeeds is set and a moving seed source is specified
   * the motion will be ignored and results will not be as expected.
   * The default is that StaticSeeds is 0.
   */
  vtkSetMacro(StaticSeeds,int);
  vtkGetMacro(StaticSeeds,int);
  //@}

  //@{
  /**
   * if StaticMesh is set, many optimizations for cell caching
   * can be assumed. if StaticMesh is not set, the algorithm
   * will attempt to find out if optimizations can be used, but
   * setting it to true will force all optimizations.
   * Do not Set StaticMesh to true if a dynamic mesh is being used
   * as this will invalidate all results.
   * The default is that StaticMesh is 0.
   */
  vtkSetMacro(StaticMesh,int);
  vtkGetMacro(StaticMesh,int);
  //@}

  //@{
  /**
   * Set/Get the Writer associated with this Particle Tracer
   * Ideally a parallel IO capable vtkH5PartWriter should be used
   * which will collect particles from all parallel processes
   * and write them to a single HDF5 file.
   */
  virtual void SetParticleWriter(vtkAbstractParticleWriter *pw);
  vtkGetObjectMacro(ParticleWriter, vtkAbstractParticleWriter);
  //@}

  //@{
  /**
   * Set/Get the filename to be used with the particle writer when
   * dumping particles to disk
   */
  vtkSetStringMacro(ParticleFileName);
  vtkGetStringMacro(ParticleFileName);
  //@}

  //@{
  /**
   * Set/Get the filename to be used with the particle writer when
   * dumping particles to disk
   */
  vtkSetMacro(EnableParticleWriting,int);
  vtkGetMacro(EnableParticleWriting,int);
  vtkBooleanMacro(EnableParticleWriting,int);
  //@}

  //@{
  /**
   * Set/Get the flag to disable cache
   * This is off by default and turned on in special circumstances
   * such as in a coprocessing workflow
   */
  vtkSetMacro(DisableResetCache,int);
  vtkGetMacro(DisableResetCache,int);
  vtkBooleanMacro(DisableResetCache,int);
  //@}

  //@{
  /**
   * Provide support for multiple seed sources
   */
  void AddSourceConnection(vtkAlgorithmOutput* input);
  void RemoveAllSources();
  //@}

 protected:
  vtkSmartPointer<vtkPolyData> Output; //managed by child classes
  //@{
  /**
   * ProtoPD is used just to keep track of the input array names and number of components
   * for copy allocating from other vtkPointDatas where the data is really stored
   */
  vtkSmartPointer<vtkPointData> ProtoPD;
  vtkIdType UniqueIdCounter;// global Id counter used to give particles a stamp
  vtkParticleTracerBaseNamespace::ParticleDataList  ParticleHistories;
  vtkSmartPointer<vtkPointData>     ParticlePointData; //the current particle point data consistent
                                                       //with particle history
  //Everything related to time
  int IgnorePipelineTime; //whether to use the pipeline time for termination
  int DisableResetCache; //whether to enable ResetCache() method
  //@}

  vtkParticleTracerBase();
  ~vtkParticleTracerBase() VTK_OVERRIDE;

  //
  // Make sure the pipeline knows what type we expect as input
  //
  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  //
  // The usual suspects
  //
  int ProcessRequest(vtkInformation* request,
                     vtkInformationVector** inputVector,
                     vtkInformationVector* outputVector) VTK_OVERRIDE;

  //
  // Store any information we need in the output and fetch what we can
  // from the input
  //
  int RequestInformation(vtkInformation* request,
                         vtkInformationVector** inputVector,
                         vtkInformationVector* outputVector) VTK_OVERRIDE;

  //
  // Compute input time steps given the output step
  //
  int RequestUpdateExtent(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector) VTK_OVERRIDE;

  //
  // what the pipeline calls for each time step
  //
  int RequestData(vtkInformation* request,
                  vtkInformationVector** inputVector,
                  vtkInformationVector* outputVector) VTK_OVERRIDE;

  //
  // these routines are internally called to actually generate the output
  //
  virtual int ProcessInput(vtkInformationVector** inputVector);

  // This is the main part of the algorithm:
  //  * move all the particles one step
  //  * Reinject particles (by adding them to this->ParticleHistories)
  //    either at the beginning or at the end of each step (modulo this->ForceReinjectionEveryNSteps)
  //  * Output a polydata representing the moved particles
  // Note that if the starting and the ending time coincide, the polydata is still valid.
  virtual vtkPolyData* Execute(vtkInformationVector** inputVector);

  // the RequestData will call these methods in turn
  virtual void Initialize(){} //the first iteration
  virtual int OutputParticles(vtkPolyData* poly)=0; //every iteration
  virtual void Finalize(){} //the last iteration

  /**
   * Method to get the data set seed sources.
   * For in situ we want to override how the seed sources are made available.
   */
  virtual std::vector<vtkDataSet*> GetSeedSources(vtkInformationVector* inputVector, int timeStep);

  //
  // Initialization of input (vector-field) geometry
  //
  int InitializeInterpolator();
  int UpdateDataCache(vtkDataObject *td);

  /**
   * inside our data. Add good ones to passed list and set count to the
   * number that passed
   */
  void TestParticles(
    vtkParticleTracerBaseNamespace::ParticleVector &candidates,
    vtkParticleTracerBaseNamespace::ParticleVector &passed,
    int &count);

  void TestParticles(
    vtkParticleTracerBaseNamespace::ParticleVector &candidates, std::vector<int> &passed);

  /**
   * all the injection/seed points according to which processor
   * they belong to. This saves us retesting at every injection time
   * providing 1) The volumes are static, 2) the seed points are static
   * If either are non static, then this step is skipped.
   */
  virtual void AssignSeedsToProcessors(
    double time, vtkDataSet *source, int sourceID, int ptId,
    vtkParticleTracerBaseNamespace::ParticleVector &localSeedPoints,
    int &localAssignedCount);

  /**
   * give each one a uniqu ID. We need to use MPI to find out
   * who is using which numbers.
   */
  virtual void AssignUniqueIds(
    vtkParticleTracerBaseNamespace::ParticleVector &localSeedPoints);

  /**
   * and sending between processors, into a list, which is used as the master
   * list on this processor
   */
  void UpdateParticleList(
    vtkParticleTracerBaseNamespace::ParticleVector &candidates);

  /**
   * this is used during classification of seed points and also between iterations
   * of the main loop as particles leave each processor domain. Returns true
   * if particles moved between processes and false otherwise.
   */
  virtual bool UpdateParticleListFromOtherProcesses(){return false;}

  /**
   * particle between the two times supplied.
   */
  void IntegrateParticle(
    vtkParticleTracerBaseNamespace::ParticleListIterator &it,
    double currenttime, double terminationtime,
    vtkInitialValueProblemSolver* integrator);

  // if the particle is added to send list, then returns value is 1,
  // if it is kept on this process after a retry return value is 0
  virtual bool SendParticleToAnotherProcess(
    vtkParticleTracerBaseNamespace::ParticleInformation &,
    vtkParticleTracerBaseNamespace::ParticleInformation &, vtkPointData*)
  {
    return true;
  }

  /**
   * This is an old routine kept for possible future use.
   * In dynamic meshes, particles might leave the domain and need to be extrapolated across
   * a gap between the meshes before they re-renter another domain
   * dodgy rotating meshes need special care....
   */
  bool ComputeDomainExitLocation(
    double pos[4], double p2[4], double intersection[4],
    vtkGenericCell *cell);

  //
  // Scalar arrays that are generated as each particle is updated
  //
  void CreateProtoPD(vtkDataObject* input);

  vtkFloatArray*    GetParticleAge(vtkPointData*);
  vtkIntArray*      GetParticleIds(vtkPointData*);
  vtkCharArray*     GetParticleSourceIds(vtkPointData*);
  vtkIntArray*      GetInjectedPointIds(vtkPointData*);
  vtkIntArray*      GetInjectedStepIds(vtkPointData*);
  vtkIntArray*      GetErrorCodeArr(vtkPointData*);
  vtkFloatArray*    GetParticleVorticity(vtkPointData*);
  vtkFloatArray*    GetParticleRotation(vtkPointData*);
  vtkFloatArray*    GetParticleAngularVel(vtkPointData*);

  // utility function we use to test if a point is inside any of our local datasets
  bool InsideBounds(double point[]);

  void CalculateVorticity( vtkGenericCell* cell, double pcoords[3],
                           vtkDoubleArray* cellVectors, double vorticity[3] );

  //------------------------------------------------------


  double GetCacheDataTime(int i);
  double GetCacheDataTime();

  virtual void ResetCache();
  void AddParticle(vtkParticleTracerBaseNamespace::ParticleInformation &info, double* velocity);

  //@{
  /**
   * Methods that check that the input arrays are ordered the
   * same on all data sets. This needs to be true for all
   * blocks in a composite data set as well as across all processes.
   */
  virtual bool IsPointDataValid(vtkDataObject* input);
  bool IsPointDataValid(vtkCompositeDataSet* input, std::vector<std::string>& arrayNames);
  void GetPointDataArrayNames(vtkDataSet* input, std::vector<std::string>& names);
  //@}

  vtkGetMacro(ReinjectionCounter, int);
  vtkGetMacro(CurrentTimeValue, double);

  /**
   * Methods to append values to existing point data arrays that may
   * only be desired on specific concrete derived classes.
   */
  virtual void InitializeExtraPointDataArrays(vtkPointData* vtkNotUsed(outputPD)) {}

  virtual void AppendToExtraPointDataArrays(vtkParticleTracerBaseNamespace::ParticleInformation &) {}

  vtkTemporalInterpolatedVelocityField* GetInterpolator();

  /**
   * For restarts of particle paths, we add in the ability to add in
   * particles from a previous computation that we will still advect.
   */
  virtual void AddRestartSeeds(vtkInformationVector** /*inputVector*/) {}

 private:
  /**
   * Hide this because we require a new interpolator type
   */
  void SetInterpolatorPrototype(vtkAbstractInterpolatedVelocityField*) {}

  /**
   * When particles leave the domain, they must be collected
   * and sent to the other processes for possible continuation.
   * These routines manage the collection and sending after each main iteration.
   * RetryWithPush adds a small push to a particle along it's current velocity
   * vector, this helps get over cracks in dynamic/rotating meshes. This is a
   * first order integration though so it may introduce a bit extra error compared
   * to the integrator that is used.
   */
  bool RetryWithPush(
    vtkParticleTracerBaseNamespace::ParticleInformation &info, double* point1,double delT, int subSteps);

  bool SetTerminationTimeNoModify(double t);

  //Parameters of tracing
  vtkInitialValueProblemSolver* Integrator;
  double IntegrationStep;
  double MaximumError;
  bool ComputeVorticity;
  double RotationScale;
  double TerminalSpeed;

  // A counter to keep track of how many times we reinjected
  int ReinjectionCounter;

  // Important for Caching of Cells/Ids/Weights etc
  int           AllFixedGeometry;
  int           StaticMesh;
  int           StaticSeeds;

  std::vector<double>  InputTimeValues;
  double StartTime;
  double TerminationTime;
  double CurrentTimeValue;

  int  StartTimeStep; //InputTimeValues[StartTimeStep] <= StartTime <= InputTimeValues[StartTimeStep+1]
  int  CurrentTimeStep;
  int  TerminationTimeStep; //computed from start time
  bool FirstIteration;

  //Innjection parameters
  int           ForceReinjectionEveryNSteps;
  vtkTimeStamp  ParticleInjectionTime;
  bool          HasCache;

  // Particle writing to disk
  vtkAbstractParticleWriter *ParticleWriter;
  char                      *ParticleFileName;
  int                        EnableParticleWriting;


  // The main lists which are held during operation- between time step updates
  vtkParticleTracerBaseNamespace::ParticleVector    LocalSeeds;

  // The velocity interpolator
  vtkSmartPointer<vtkTemporalInterpolatedVelocityField>  Interpolator;
  vtkAbstractInterpolatedVelocityField * InterpolatorPrototype;

  // Data for time step CurrentTimeStep-1 and CurrentTimeStep
  vtkSmartPointer<vtkMultiBlockDataSet> CachedData[2];

  // Cache bounds info for each dataset we will use repeatedly
  typedef struct {
    double b[6];
  } bounds;
  std::vector<bounds> CachedBounds[2];

  // temporary variables used by Exeucte(), for convenience only

  vtkSmartPointer<vtkPoints> OutputCoordinates;
  vtkSmartPointer<vtkFloatArray>    ParticleAge;
  vtkSmartPointer<vtkIntArray>      ParticleIds;
  vtkSmartPointer<vtkCharArray>     ParticleSourceIds;
  vtkSmartPointer<vtkIntArray>      InjectedPointIds;
  vtkSmartPointer<vtkIntArray>      InjectedStepIds;
  vtkSmartPointer<vtkIntArray>      ErrorCode;
  vtkSmartPointer<vtkFloatArray>    ParticleVorticity;
  vtkSmartPointer<vtkFloatArray>    ParticleRotation;
  vtkSmartPointer<vtkFloatArray>    ParticleAngularVel;
  vtkSmartPointer<vtkDoubleArray>   CellVectors;
  vtkSmartPointer<vtkPointData>     OutputPointData;
  vtkSmartPointer<vtkDataSet>       DataReferenceT[2];
  vtkSmartPointer<vtkCellArray>     ParticleCells;

  vtkParticleTracerBase(const vtkParticleTracerBase&) VTK_DELETE_FUNCTION;
  void operator=(const vtkParticleTracerBase&) VTK_DELETE_FUNCTION;
  vtkTimeStamp ExecuteTime;

  unsigned int NumberOfParticles();

  friend class ParticlePathFilterInternal;
  friend class StreaklineFilterInternal;

  static const double Epsilon;
};

#endif
