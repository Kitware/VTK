/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTemporalStreamTracer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTemporalStreamTracer - A Parallel Particle tracer for unsteady vector fields
// .SECTION Description
// vtkTemporalStreamTracer is a filter that integrates a vector field to generate
//
//
// .SECTION See Also
// vtkRibbonFilter vtkRuledSurfaceFilter vtkInitialValueProblemSolver
// vtkRungeKutta2 vtkRungeKutta4 vtkRungeKutta45 vtkStreamTracer

#ifndef __vtkTemporalStreamTracer_h
#define __vtkTemporalStreamTracer_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkSmartPointer.h" // For protected ivars.
#include "vtkStreamTracer.h"

//BTX
#include <vector> // STL Header
#include <list>   // STL Header
//ETX

class vtkMultiProcessController;

class vtkMultiBlockDataSet;
class vtkDataArray;
class vtkDoubleArray;
class vtkGenericCell;
class vtkIntArray;
class vtkTemporalInterpolatedVelocityField;
class vtkPoints;
class vtkCellArray;
class vtkDoubleArray;
class vtkFloatArray;
class vtkIntArray;
class vtkCharArray;
class vtkAbstractParticleWriter;

//BTX
namespace vtkTemporalStreamTracerNamespace
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
    int           TimeStepAge;
    int           InjectedPointId;
    int           InjectedStepId;
    int           UniqueParticleId;
    // These are useful to track for debugging etc
    int           ErrorCode;
    float         age;
    // these are needed across time steps to compute vorticity
    float         rotation;
    float         angularVel;
    float         time;
    float         speed;
  } ParticleInformation;

  typedef std::vector<ParticleInformation>  ParticleVector;
  typedef ParticleVector::iterator             ParticleIterator;
  typedef std::list<ParticleInformation>    ParticleDataList;
  typedef ParticleDataList::iterator           ParticleListIterator;
};
//ETX

class VTKFILTERSFLOWPATHS_EXPORT vtkTemporalStreamTracer : public vtkStreamTracer
{
public:

    vtkTypeMacro(vtkTemporalStreamTracer,vtkStreamTracer);
    void PrintSelf(ostream& os, vtkIndent indent);

    // Description:
    // Construct object using 2nd order Runge Kutta
    static vtkTemporalStreamTracer *New();

    // Description:
    // Set/Get the TimeStep. This is the primary means of advancing
    // the particles. The TimeStep should be animated and this will drive
    // the pipeline forcing timesteps to be fetched from upstream.
    vtkSetMacro(TimeStep,unsigned int);
    vtkGetMacro(TimeStep,unsigned int);

    // Description:
    // To get around problems with the Paraview Animation controls
    // we can just animate the time step and ignore the TIME_ requests
    vtkSetMacro(IgnorePipelineTime, int);
    vtkGetMacro(IgnorePipelineTime, int);
    vtkBooleanMacro(IgnorePipelineTime, int);

    // Description:
    // If the data source does not have the correct time values
    // present on each time step - setting this value to non unity can
    // be used to adjust the time step size from 1s pre step to
    // 1x_TimeStepResolution : Not functional in this version.
    // Broke it @todo, put back time scaling
    vtkSetMacro(TimeStepResolution,double);
    vtkGetMacro(TimeStepResolution,double);

    // Description:
    // When animating particles, it is nice to inject new ones every Nth step
    // to produce a continuous flow. Setting ForceReinjectionEveryNSteps to a
    // non zero value will cause the particle source to reinject particles
    // every Nth step even if it is otherwise unchanged.
    // Note that if the particle source is also animated, this flag will be
    // redundant as the particles will be reinjected whenever the source changes
    // anyway
    vtkSetMacro(ForceReinjectionEveryNSteps,int);
    vtkGetMacro(ForceReinjectionEveryNSteps,int);

//BTX
  enum Units
  {
    TERMINATION_TIME_UNIT,
    TERMINATION_STEP_UNIT
  };
//ETX

    // Description:
    // Setting TerminationTime to a positive value will cause particles
    // to terminate when the time is reached. Use a vlue of zero to
    // diable termination. The units of time should be consistent with the
    // primary time variable.
    vtkSetMacro(TerminationTime,double);
    vtkGetMacro(TerminationTime,double);

    // Description:
    // The units of TerminationTime may be actual 'Time' units as described
    // by the data, or just TimeSteps of iteration.
    vtkSetMacro(TerminationTimeUnit,int);
    vtkGetMacro(TerminationTimeUnit,int);
    void SetTerminationTimeUnitToTimeUnit()
    {this->SetTerminationTimeUnit(TERMINATION_TIME_UNIT);};
    void SetTerminationTimeUnitToStepUnit()
    {this->SetTerminationTimeUnit(TERMINATION_STEP_UNIT);};

    // Description:
    // if StaticSeeds is set and the mesh is static,
    // then every time particles are injected we can re-use the same
    // injection information. We classify particles according to
    // processor just once before start.
    // If StaticSeeds is set and a moving seed source is specified
    // the motion will be ignored and results will not be as expected.
    vtkSetMacro(StaticSeeds,int);
    vtkGetMacro(StaticSeeds,int);
    vtkBooleanMacro(StaticSeeds,int);

    // Description:
    // if StaticMesh is set, many optimizations for cell caching
    // can be assumed. if StaticMesh is not set, the algorithm
    // will attempt to find out if optimizations can be used, but
    // setting it to true will force all optimizations.
    // Do not Set StaticMesh to true if a dynamic mesh is being used
    // as this will invalidate all results.
    vtkSetMacro(StaticMesh,int);
    vtkGetMacro(StaticMesh,int);
    vtkBooleanMacro(StaticMesh,int);

    // Description:
    // Set/Get the Writer associated with this Particle Tracer
    // Ideally a parallel IO capable vtkH5PartWriter should be used
    // which will collect particles from all parallel processes
    // and write them to a single HDF5 file.
    virtual void SetParticleWriter(vtkAbstractParticleWriter *pw);
    vtkGetObjectMacro(ParticleWriter, vtkAbstractParticleWriter);

    // Description:
    // Set/Get the filename to be used with the particle writer when
    // dumping particles to disk
    vtkSetStringMacro(ParticleFileName);
    vtkGetStringMacro(ParticleFileName);

    // Description:
    // Set/Get the filename to be used with the particle writer when
    // dumping particles to disk
    vtkSetMacro(EnableParticleWriting,int);
    vtkGetMacro(EnableParticleWriting,int);
    vtkBooleanMacro(EnableParticleWriting,int);

    // Description:
    // Provide support for multiple see sources
    void AddSourceConnection(vtkAlgorithmOutput* input);
    void RemoveAllSources();

  protected:

     vtkTemporalStreamTracer();
    ~vtkTemporalStreamTracer();

    //
    // Make sure the pipeline knows what type we expect as input
    //
    virtual int FillInputPortInformation(int port, vtkInformation* info);

    //
    // The usual suspects
    //
    virtual int ProcessRequest(vtkInformation* request,
                               vtkInformationVector** inputVector,
                               vtkInformationVector* outputVector);

    //
    // Store any information we need in the output and fetch what we can
    // from the input
    //
    virtual int RequestInformation(vtkInformation* request,
                                  vtkInformationVector** inputVector,
                                  vtkInformationVector* outputVector);

    //
    // Compute input time steps given the output step
    //
    virtual int RequestUpdateExtent(vtkInformation* request,
                                    vtkInformationVector** inputVector,
                                    vtkInformationVector* outputVector);

    //
    // what the pipeline calls for each time step
    //
    virtual int RequestData(vtkInformation* request,
                            vtkInformationVector** inputVector,
                            vtkInformationVector* outputVector);

    //
    // these routines are internally called to actually generate the output
    //
    virtual int ProcessInput(vtkInformationVector** inputVector);

    virtual int GenerateOutput(vtkInformationVector** inputVector,
                               vtkInformationVector* outputVector);

    //
    // Initialization of input (vector-field) geometry
    //
    int InitializeInterpolator();
    int SetTemporalInput(vtkDataObject *td, int index);

//BTX
//

    // Description : Test the list of particles to see if they are
    // inside our data. Add good ones to passed list and set count to the
    // number that passed
    void TestParticles(
      vtkTemporalStreamTracerNamespace::ParticleVector &candidates,
      vtkTemporalStreamTracerNamespace::ParticleVector &passed,
      int &count);

    // Description : Before starting the particle trace, classify
    // all the injection/seed points according to which processor
    // they belong to. This saves us retesting at every injection time
    // providing 1) The volumes are static, 2) the seed points are static
    // If either are non static, then this step is skipped.
    virtual void AssignSeedsToProcessors(
      vtkDataSet *source, int sourceID, int ptId,
      vtkTemporalStreamTracerNamespace::ParticleVector &LocalSeedPoints,
      int &LocalAssignedCount);

    // Description : once seeds have been assigned to a process, we
    // give each one a uniqu ID. We need to use MPI to find out
    // who is using which numbers.
    virtual void AssignUniqueIds(
      vtkTemporalStreamTracerNamespace::ParticleVector &LocalSeedPoints);

    // Description : copy list of particles from a vector used for testing particles
    // and sending between processors, into a list, which is used as the master
    // list on this processor
    void UpdateParticleList(
      vtkTemporalStreamTracerNamespace::ParticleVector &candidates);

    // Description : Perform a GatherV operation on a vector of particles
    // this is used during classification of seed points and also between iterations
    // of the main loop as particles leave each processor domain
    virtual void TransmitReceiveParticles(
      vtkTemporalStreamTracerNamespace::ParticleVector &outofdomain,
      vtkTemporalStreamTracerNamespace::ParticleVector &received,
      bool removeself);

    // Description : The main loop performing Runge-Kutta integration of a single
    // particle between the two times supplied.
    void IntegrateParticle(
      vtkTemporalStreamTracerNamespace::ParticleListIterator &it,
      double currenttime, double terminationtime,
      vtkInitialValueProblemSolver* integrator);

    // Description : When particle leave the domain, they must be collected
    // and sent to the other processors for possible continuation.
    // These routines manage the collection and sending after each main iteration.
    // RetryWithPush adds a small pusj to aparticle along it's current velocity
    // vector, this helps get over cracks in dynamic/rotating meshes
    bool RetryWithPush(
      vtkTemporalStreamTracerNamespace::ParticleInformation &info,
      double velocity[3], double delT);

    // if the particle is added to send list, then returns value is 1,
    // if it is kept on this process after a retry return value is 0
    bool SendParticleToAnotherProcess(
      vtkTemporalStreamTracerNamespace::ParticleInformation &info,
      double point1[4], double delT);

    void AddParticleToMPISendList(
      vtkTemporalStreamTracerNamespace::ParticleInformation &info);

    // Description : This is an old routine kept for possible future use.
    // In dnamic meshes, particles might leave the domain and need to be extrapolated across
    // a gap between the meshes before they re-renter another domain
    // dodgy rotating meshes need special care....
    bool ComputeDomainExitLocation(
      double pos[4], double p2[4], double intersection[4],
      vtkGenericCell *cell);

//
//ETX
//
    //Track internally which round of RequestData it is--between 0 and 2
    int           RequestIndex;

    // Track which process we are
    int           UpdatePiece;
    int           UpdateNumPieces;

    // Important for Caching of Cells/Ids/Weights etc
    int           AllFixedGeometry;
    int           StaticMesh;
    int           StaticSeeds;

    // Support 'pipeline' time or manual SetTimeStep
    unsigned int  TimeStep;
    unsigned int  ActualTimeStep;
    int           IgnorePipelineTime;
    unsigned int  NumberOfInputTimeSteps;
//BTX
    std::vector<double>  InputTimeValues;
    std::vector<double>  OutputTimeValues;
//ETX

    // more time management
    double        EarliestTime;
    double        CurrentTimeSteps[2];
    double        TimeStepResolution;

    // Particle termination after time
    double        TerminationTime;
    int           TerminationTimeUnit;

    // Particle injection+Reinjection
    int           ForceReinjectionEveryNSteps;
    bool          ReinjectionFlag;
    int           ReinjectionCounter;
    vtkTimeStamp  ParticleInjectionTime;

    // Particle writing to disk
    vtkAbstractParticleWriter *ParticleWriter;
    char                      *ParticleFileName;
    int                        EnableParticleWriting;

//BTX
    // The main lists which are held during operation- between time step updates
    unsigned int                                        NumberOfParticles;
    vtkTemporalStreamTracerNamespace::ParticleDataList  ParticleHistories;
    vtkTemporalStreamTracerNamespace::ParticleVector    LocalSeeds;
//ETX

//BTX
    //
    // Scalar arrays that are generated as each particle is updated
    //
    vtkSmartPointer<vtkFloatArray>    ParticleAge;
    vtkSmartPointer<vtkIntArray>      ParticleIds;
    vtkSmartPointer<vtkCharArray>     ParticleSourceIds;
    vtkSmartPointer<vtkIntArray>      InjectedPointIds;
    vtkSmartPointer<vtkIntArray>      InjectedStepIds;
    vtkSmartPointer<vtkIntArray>      ErrorCode;
    vtkSmartPointer<vtkFloatArray>    ParticleVorticity;
    vtkSmartPointer<vtkFloatArray>    ParticleRotation;
    vtkSmartPointer<vtkFloatArray>    ParticleAngularVel;
    vtkSmartPointer<vtkDoubleArray>   cellVectors;
    vtkSmartPointer<vtkPointData>     OutputPointData;
    int                               InterpolationCount;

    // The output geometry
    vtkSmartPointer<vtkCellArray>     ParticleCells;
    vtkSmartPointer<vtkPoints>        OutputCoordinates;

    // List used for transmitting between processors during parallel operation
    vtkTemporalStreamTracerNamespace::ParticleVector MPISendList;

    // The velocity interpolator
    vtkSmartPointer<vtkTemporalInterpolatedVelocityField>  Interpolator;

    // The input datasets which are stored by time step 0 and 1
    vtkSmartPointer<vtkMultiBlockDataSet> InputDataT[2];
    vtkSmartPointer<vtkDataSet>           DataReferenceT[2];

    // Cache bounds info for each dataset we will use repeatedly
    typedef struct {
      double b[6];
    } bounds;
    std::vector<bounds> CachedBounds[2];

    // utility function we use to test if a point is inside any of our local datasets
    bool InsideBounds(double point[]);

//ETX

  // global Id counter used to give particles a stamp
  vtkIdType UniqueIdCounter;
  vtkIdType UniqueIdCounterMPI;
  // for debugging only;
  int substeps;

private:
  // Description:
  // Hide this because we require a new interpolator type
  void SetInterpolatorPrototype(vtkAbstractInterpolatedVelocityField*) {};

private:
  vtkTemporalStreamTracer(const vtkTemporalStreamTracer&);  // Not implemented.
  void operator=(const vtkTemporalStreamTracer&);  // Not implemented.
};

#endif
