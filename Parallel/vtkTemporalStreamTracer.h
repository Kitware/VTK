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

#include "vtkSmartPointer.h" // For protected ivars.
#include "vtkStreamTracer.h"

//BTX
#include <vtkstd/vector> // I'll remove these soon.
#include <vtkstd/list> // I'll remove these soon.
//ETX

class vtkMultiProcessController;

class vtkCompositeDataSet;
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
class vtkAbstractParticleWriter;

//BTX
namespace vtkTemporalStreamTracerNamespace
{
  typedef struct { double x[4]; } Position;
  typedef struct {
    // These are used during iteration
    int           Counter;
    int           Index;
    bool          Wrap;
    Position      CurrentPosition;
    int           CachedDataSet[2];
    vtkIdType     CachedCellId[2];
    // These are computed scalars we might display
    int           SourceID;
    int           InjectedPointId;      
    float         UniqueParticleId;
    // these are needed across time steps to compute vorticity
    float         rotation;
    float         angularVel;
    float         time;
  } ParticleInformation;

  typedef vtkstd::vector<ParticleInformation>  ParticleList;
  typedef vtkstd::list<ParticleInformation>    ParticleDataList;
  typedef ParticleDataList::iterator           ParticleIterator;

  class vtkTemporalStreamTracerInternals {
  public:
  };
};

//ETX

class VTK_PARALLEL_EXPORT vtkTemporalStreamTracer : public vtkStreamTracer
{
public:

    vtkTypeRevisionMacro(vtkTemporalStreamTracer,vtkStreamTracer);
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

    // Description:
    // Specify an alternative Geometry object as the source of particles
    // This method exists so that in the ParaView GUI we can either
    // select a widget as source, or by using this input, a dataset generated
    // during the session.
    // Old style. Do not use.
    void SetSource2(vtkDataSet *source);
    vtkDataSet *GetSource2();

    // Description:
    // Specify an alternative Geometry object as the source of particles
    // This method exists so that in the ParaView GUI we can either
    // select a widget as source, or by using this input, a dataset generated
    // during the session.
    // New Style. Do use.
    void SetSource2Connection(vtkAlgorithmOutput* algOutput);

    // Description:
    vtkSetMacro(EnableSource1,int);
    vtkGetMacro(EnableSource1,int);
    vtkBooleanMacro(EnableSource1,int);

    // Description:
    vtkSetMacro(EnableSource2,int);
    vtkGetMacro(EnableSource2,int);
    vtkBooleanMacro(EnableSource2,int);

    // Description:
    // Set/Get the controller used when sending particles between processes
    // The controller must be an instance of vtkMPIController.
    // If VTK was compiled without VTK_USE_MPI on, then the Controller is simply
    // ignored.
    virtual void SetController(vtkMultiProcessController* controller);
    vtkGetObjectMacro(Controller, vtkMultiProcessController);

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
    // Generate output
    //
    virtual int RequestData(vtkInformation* request,
                            vtkInformationVector** inputVector,
                            vtkInformationVector* outputVector);


    int InitializeInterpolator();
    int SetupInputs(vtkInformation* inInfo, vtkInformation* outInfo);

//
//BTX
//
    // Description : Tests points to see if they are inside this region
    // Pass in either a source object or an input list, one
    // parameter should be valid, the other NULL
    void InjectSeeds(vtkDataSet *source, int sourceID, int injectionID, 
      vtkTemporalStreamTracerNamespace::ParticleList *inputlist,
      vtkTemporalStreamTracerNamespace::ParticleList &candidates, vtkTemporalStreamTracerNamespace::ParticleList *outofdomain);

    void UpdateSeeds(vtkTemporalStreamTracerNamespace::ParticleList &candidates);

    void TransmitReceiveParticles(
      vtkTemporalStreamTracerNamespace::ParticleList &outofdomain, vtkTemporalStreamTracerNamespace::ParticleList &received, bool removeself);

    void IntegrateParticle(
      vtkTemporalStreamTracerNamespace::ParticleIterator &it, 
      double currenttime, double terminationtime,
      vtkInitialValueProblemSolver* integrator);

    void GenerateOutputLines(vtkPolyData *output);
    bool DoParticleSendTasks(vtkTemporalStreamTracerNamespace::ParticleInformation &info, double point1[4], double velocity[3], double delT);
    bool DoParticleSendTasks(vtkTemporalStreamTracerNamespace::ParticleInformation &info, double point1[4], double delT);
    bool ComputeDomainExitLocation(
      double pos[4], double p2[4], double intersection[4],
      vtkGenericCell *cell);
    void AddParticleToMPISendList(vtkTemporalStreamTracerNamespace::ParticleInformation &info);
//
//ETX
//

    // Mostly useful for debugging parallel operation
    int           UpdatePiece;
    int           UpdateNumPieces;

    // Turn on/off sources
    int           EnableSource1;      
    int           EnableSource2;

    // Important for Caching of Cells/Ids/Weights etc
    int           AllFixedGeometry;
    int           NoFixedGeometry;

    // internal data variables
    int           MaxCellSize;

    // Support pipeline time 
    unsigned int  TimeStep;
    unsigned int  ActualTimeStep;
    unsigned int  NumberOfInputTimeSteps;
//BTX
    vtkstd::vector<double>  InputTimeValues;
    vtkstd::vector<double>  OutputTimeValues;
//ETX

    double        EarliestTime;
    double        CurrentTimeSteps[2];
    double        TimeStepResolution;
    int           ForceReinjectionEveryNSteps;
    bool          ReinjectionFlag;  
    int           ReinjectionCounter;
    int           IgnorePipelineTime;
    //
    vtkAbstractParticleWriter *ParticleWriter;
    char                      *ParticleFileName; 
    int                        EnableParticleWriting;
    //
    vtkTimeStamp  ParticleInjectionTime;
    vtkTimeStamp  SeedInjectionTime;

//BTX
    unsigned int                                        NumberOfParticles;
    vtkTemporalStreamTracerNamespace::ParticleDataList  ParticleHistories;
//ETX

//BTX
    vtkSmartPointer<vtkPoints>        OutputCoordinates;
    vtkSmartPointer<vtkCellArray>     ParticleCells;
    //
    // Scalar arrays that are generated as each particle is updated
    //
    vtkSmartPointer<vtkIntArray>      ParticleIds;
    vtkSmartPointer<vtkIntArray>      ParticleSourceIds;
    vtkSmartPointer<vtkIntArray>      InjectedPointIds;
    vtkSmartPointer<vtkDoubleArray>   cellVectors;
    vtkSmartPointer<vtkFloatArray>    ParticleTime;
    vtkSmartPointer<vtkFloatArray>    ParticleVorticity;
    vtkSmartPointer<vtkFloatArray>    ParticleRotation;
    vtkSmartPointer<vtkFloatArray>    ParticleAngularVel;
    vtkSmartPointer<vtkPointData>     OutputPointData;
    vtkSmartPointer<vtkPointData>     OutputPointDataT1;
    vtkSmartPointer<vtkPointData>     OutputPointDataT2;
    //
    vtkTemporalStreamTracerNamespace::ParticleList MPISendList;
    //
    vtkSmartPointer<vtkTemporalInterpolatedVelocityField>  Interpolator;
    vtkCompositeDataSet                                   *InputDataT[2];
    vtkDataSet                                            *DataReferenceT[2];

    // info about each dataset we will use repeatedly
    typedef struct {
      double b[6];
    } bounds;
    vtkstd::vector<bounds> CachedBounds[2];
    vtkstd::vector<bool>   GeometryFixed[2];

    bool InsideBounds(double point[]);

    //
//ETX

  vtkMultiProcessController* Controller;
  static vtkIdType UniqueIdCounter;

private:
  // Description:
  // Hide this because we require a new interpolator type
  void SetInterpolatorPrototype(vtkInterpolatedVelocityField*) {};

private:
  vtkTemporalStreamTracer(const vtkTemporalStreamTracer&);  // Not implemented.
  void operator=(const vtkTemporalStreamTracer&);  // Not implemented.
};


#endif


