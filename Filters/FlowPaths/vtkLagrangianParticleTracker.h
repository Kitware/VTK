// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLagrangianParticleTracker
 * @brief   Filter to inject and track particles in a flow
 *
 *
 *
 * Introduce LagrangianParticleTracker
 *
 * This is a very flexible and adaptive filter to inject and track particles in a
 * flow. It takes three inputs :
 * * port 0 : Flow Input, a volumic dataset containing data to integrate with,
 *     any kind of data object, support distributed input.
 * * port 1 : Seed (source) Input, a dataset containing point to generate particles
 *     with, any kind of data object, support distributed input. Only first leaf
 *     of composite dataset is used.
 * * port 2 : Optional Surface Input, containing dataset to interact with, any
 *     kind of data object, support distributed input.
 *
 * It has two outputs :
 * * port 0 : ParticlePaths : a multipiece of polyData (one per thread) of polyLines showing the
 * paths of particles in the flow
 * * port 1 : ParticleInteractions : empty if no surface input, contains a
 *     a multiblock with as many children as the number of threads, each children containing a
 * multiblock with the same structure as the surfaces. The leafs of these structures contain a
 * polydata of vertices corresponding to the interactions. with the same composite layout of surface
 * input if any, showing all interactions between particles and the surface input.
 *
 * It has a parallel implementation which streams particle between domains.
 *
 * The most important parameters of this filter is it's integrationModel.
 * Only one integration model implementation exist currently in ParaView
 * ,vtkLagrangianMatidaIntegrationModel but the design enables plugin developers
 * to expand this tracker by creating new models.
 * A model can define  :
 * * The number of integration variable and new user defined integration variable
 * * the way the particle are integrated
 * * the way particles intersect and interact with the surface
 * * the way freeFlight termination is handled
 * * PreProcess and PostProcess methods
 * * Manual Integration, Manual partichle shifting
 * see vtkLagrangianBasicIntegrationModel and vtkLagrangianMatidaIntegrationModel
 * for more information
 *
 * It also let the user choose the Locator to use when integrating in the flow,
 * as well as the Integrator to use. Integration steps are also highly configurable,
 * step, step min and step max are passed down to the integrator (hence min and max
 * does not matter with a non adaptive integrator like RK4/5)
 *
 *  An IntegrationModel is a very specific vtkFunctionSet with a lot of features
 * allowing inherited classes
 * to concentrate on the mathematical part of the code.
 *  a Particle is basically a class wrapper around three table containing variables
 * about the particle at previous, current and next position.
 *  The particle is passed to the integrator, which use the integration model to
 * integrate the particle in the flow.
 *
 * It has other features also, including :
 *  * Adaptative Step Reintegration, to retry the step with different time step
 *      when the next position is too far
 *  * Different kind of cell length computation, including a divergence theorem
 *      based computation
 *  * Optional lines rendering controlled by a threshold
 *  * Ghost cell support
 *  * Non planar quad interaction support
 *  * Built-in support for surface interaction including, terminate, bounce,
 *      break-up and pass-through surface
 * The serial and parallel filters are fully tested.
 *
 * @sa
 * vtkLagrangianMatidaIntegrationModel vtkLagrangianParticle
 * vtkLagrangianBasicIntegrationModel
 */

#ifndef vtkLagrangianParticleTracker_h
#define vtkLagrangianParticleTracker_h

#include "vtkBoundingBox.h" // For cached bounds
#include "vtkDataObjectAlgorithm.h"
#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkSmartPointer.h"           // For smart pointer

#include <atomic> // for atomic
#include <mutex>  // for mutexes
#include <queue>  // for particle queue

VTK_ABI_NAMESPACE_BEGIN
class vtkBoundingBox;
class vtkCellArray;
class vtkDataSet;
class vtkDoubleArray;
class vtkIdList;
class vtkInformation;
class vtkInitialValueProblemSolver;
class vtkLagrangianBasicIntegrationModel;
class vtkLagrangianParticle;
class vtkMultiBlockDataSet;
class vtkMultiPieceDataSet;
class vtkPointData;
class vtkPoints;
class vtkPolyData;
class vtkPolyLine;
struct IntegratingFunctor;
struct vtkLagrangianThreadedData;

class VTKFILTERSFLOWPATHS_EXPORT vtkLagrangianParticleTracker : public vtkDataObjectAlgorithm
{
public:
  vtkTypeMacro(vtkLagrangianParticleTracker, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkLagrangianParticleTracker* New();

  typedef enum CellLengthComputation
  {
    STEP_CUR_CELL_LENGTH = 1,
    STEP_CUR_CELL_VEL_DIR = 3,
    STEP_CUR_CELL_DIV_THEO = 5
  } CellLengthComputation;

  ///@{
  /**
   * Set/Get the integration model.
   * Default is vtkLagrangianMatidaIntegrationModel
   */
  void SetIntegrationModel(vtkLagrangianBasicIntegrationModel* integrationModel);
  vtkLagrangianBasicIntegrationModel* GetIntegrationModel();
  ///@}

  ///@{
  /**
   * Set/Get the integrator.
   * Default is vtkRungeKutta2
   */
  void SetIntegrator(vtkInitialValueProblemSolver* integrator);
  vtkInitialValueProblemSolver* GetIntegrator();
  ///@}

  ///@{
  /**
   * Set/Get whether or not to use PolyVertex cell type
   * for the interaction output
   * Default is false
   */
  vtkSetMacro(GeneratePolyVertexInteractionOutput, bool);
  vtkGetMacro(GeneratePolyVertexInteractionOutput, bool);
  ///@}

  ///@{
  /**
   * Set/Get the cell length computation mode.
   * Available modes are :
   * - STEP_CUR_CELL_LENGTH :
   * Compute cell length using getLength method
   * on the current cell the particle is in
   * - STEP_CUR_CELL_VEL_DIR :
   * Compute cell length using the particle velocity
   * and the edges of the last cell the particle was in.
   * - STEP_CUR_CELL_DIV_THEO :
   * Compute cell length using the particle velocity
   * and the divergence theorem.
   */
  vtkSetMacro(CellLengthComputationMode, int);
  vtkGetMacro(CellLengthComputationMode, int);
  ///@}

  ///@{
  /**
   * Set/Get the integration step factor. Default is 1.0.
   */
  vtkSetMacro(StepFactor, double);
  vtkGetMacro(StepFactor, double);
  ///@}

  ///@{
  /**
   * Set/Get the integration step factor min. Default is 0.5.
   */
  vtkSetMacro(StepFactorMin, double);
  vtkGetMacro(StepFactorMin, double);
  ///@}

  ///@{
  /**
   * Set/Get the integration step factor max. Default is 1.5.
   */
  vtkSetMacro(StepFactorMax, double);
  vtkGetMacro(StepFactorMax, double);
  ///@}

  ///@{
  /**
   * Set/Get the maximum number of steps. -1 means no limit. Default is 100.
   */
  vtkSetMacro(MaximumNumberOfSteps, int);
  vtkGetMacro(MaximumNumberOfSteps, int);
  ///@}

  ///@{
  /**
   * Set/Get the maximum integration time. A negative value means no limit.
   * Default is -1.
   */
  vtkSetMacro(MaximumIntegrationTime, double);
  vtkGetMacro(MaximumIntegrationTime, double);
  ///@}

  ///@{
  /**
   * Set/Get the Adaptive Step Reintegration feature.
   * it checks the step size after the integration
   * and if it is too big will retry with a smaller step
   * Default is false.
   */
  vtkSetMacro(AdaptiveStepReintegration, bool);
  vtkGetMacro(AdaptiveStepReintegration, bool);
  vtkBooleanMacro(AdaptiveStepReintegration, bool);
  ///@}

  ///@{
  /**
   * Set/Get the generation of the particle path output,
   * Default is true.
   */
  vtkSetMacro(GenerateParticlePathsOutput, bool);
  vtkGetMacro(GenerateParticlePathsOutput, bool);
  vtkBooleanMacro(GenerateParticlePathsOutput, bool);
  ///@}

  ///@{
  /**
   * Set/Get the flag to force the use of manual shift when using
   * distributed computing
   * Default is false.
   */
  vtkSetMacro(ForcePManualShift, bool);
  vtkGetMacro(ForcePManualShift, bool);
  vtkBooleanMacro(ForcePManualShift, bool);
  ///@}

  ///@{
  /**
   * Specify the source object used to generate particle initial position (seeds).
   * Note that this method does not connect the pipeline. The algorithm will
   * work on the input data as it is without updating the producer of the data.
   * See SetSourceConnection for connecting the pipeline.
   */
  void SetSourceData(vtkDataObject* source);
  vtkDataObject* GetSource();
  ///@}

  /**
   * Specify the source object used to generate particle initial position (seeds).
   */
  void SetSourceConnection(vtkAlgorithmOutput* algOutput);

  ///@{
  /**
   * Specify the source object used to compute surface interaction with
   * Note that this method does not connect the pipeline. The algorithm will
   * work on the input data as it is without updating the producer of the data.
   * See SetSurfaceConnection for connecting the pipeline.
   */
  void SetSurfaceData(vtkDataObject* source);
  vtkDataObject* GetSurface();
  ///@}

  /**
   * Specify the object used to compute surface interaction with.
   */
  void SetSurfaceConnection(vtkAlgorithmOutput* algOutput);

  /**
   * Declare input port type
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Declare output port type
   */
  int FillOutputPortInformation(int port, vtkInformation* info) override;

  /**
   * Create outputs objects.
   */
  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Process inputs to integrate particle and generate output.
   */
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Get the tracker modified time taking into account the integration model
   * and the integrator.
   */
  vtkMTimeType GetMTime() override;

  /**
   * Get an unique id for a particle
   * This method is thread safe
   */
  virtual vtkIdType GetNewParticleId();

protected:
  vtkLagrangianParticleTracker();
  ~vtkLagrangianParticleTracker() override;

  virtual bool InitializeFlow(vtkDataObject* flow, vtkBoundingBox* bounds);
  virtual bool InitializeParticles(const vtkBoundingBox* bounds, vtkDataSet* seeds,
    std::queue<vtkLagrangianParticle*>& particles, vtkPointData* seedData);
  virtual void GenerateParticles(const vtkBoundingBox* bounds, vtkDataSet* seeds,
    vtkDataArray* initialVelocities, vtkDataArray* initialIntegrationTimes, vtkPointData* seedData,
    int nVar, std::queue<vtkLagrangianParticle*>& particles);
  virtual bool UpdateSurfaceCacheIfNeeded(vtkDataObject*& surfaces);
  virtual void InitializeSurface(vtkDataObject*& surfaces);

  /**
   * This method is thread safe
   */
  virtual bool InitializePathsOutput(
    vtkPointData* seedData, vtkIdType numberOfSeeds, vtkPolyData*& particlePathsOutput);

  /**
   * This method is thread safe
   */
  virtual bool InitializeInteractionOutput(
    vtkPointData* seedData, vtkDataObject* surfaces, vtkDataObject*& interractionOutput);

  virtual bool FinalizeOutputs(vtkPolyData* particlePathsOutput, vtkDataObject* interactionOutput);

  static void InsertPolyVertexCell(vtkPolyData* polydata);
  static void InsertVertexCells(vtkPolyData* polydata);

  virtual void GetParticleFeed(std::queue<vtkLagrangianParticle*>& particleQueue);

  /**
   * This method is thread safe
   */
  virtual int Integrate(vtkInitialValueProblemSolver* integrator, vtkLagrangianParticle*,
    std::queue<vtkLagrangianParticle*>&, vtkPolyData* particlePathsOutput,
    vtkPolyLine* particlePath, vtkDataObject* interactionOutput);

  /**
   * This method is thread safe
   */
  void InsertPathOutputPoint(vtkLagrangianParticle* particle, vtkPolyData* particlePathsOutput,
    vtkIdList* particlePathPointId, bool prev = false);

  /**
   * This method is thread safe
   */
  void InsertInteractionOutputPoint(vtkLagrangianParticle* particle,
    unsigned int interactedSurfaceFlatIndex, vtkDataObject* interactionOutput);

  /**
   * Computes the cell length for the next step using the method set by
   * CellLengthComputationMode. Returns -1 if particle is out the of domain.
   */
  double ComputeCellLength(vtkLagrangianParticle* particle);

  /**
   * This method is thread safe
   */
  bool ComputeNextStep(vtkInitialValueProblemSolver* integrator, double* xprev, double* xnext,
    double t, double& delT, double& delTActual, double minStep, double maxStep, double cellLength,
    int& integrationRes, vtkLagrangianParticle* particle);

  /**
   * This method is thread safe
   * Call the ParticleAboutToBeDeleted model method and delete the particle
   */
  virtual void DeleteParticle(vtkLagrangianParticle* particle);

  vtkSmartPointer<vtkLagrangianBasicIntegrationModel> IntegrationModel;
  vtkSmartPointer<vtkInitialValueProblemSolver> Integrator;

  int CellLengthComputationMode;
  double StepFactor;
  double StepFactorMin;
  double StepFactorMax;
  int MaximumNumberOfSteps;
  double MaximumIntegrationTime;
  bool AdaptiveStepReintegration;
  bool GenerateParticlePathsOutput = true;
  bool GeneratePolyVertexInteractionOutput;
  bool ForcePManualShift = false;
  std::atomic<vtkIdType> ParticleCounter;
  std::atomic<vtkIdType> IntegratedParticleCounter;
  vtkIdType IntegratedParticleCounterIncrement;
  vtkPointData* SeedData;

  // internal parameters use for step computation
  double MinimumVelocityMagnitude;
  double MinimumReductionFactor;

  // Cache related parameters
  vtkDataObject* FlowCache;
  vtkMTimeType FlowTime;
  vtkBoundingBox FlowBoundsCache;
  bool FlowCacheInvalid = true;
  vtkDataObject* SurfacesCache;
  vtkMTimeType SurfacesTime;
  bool SurfaceCacheInvalid = true;

  std::mutex ProgressMutex;
  friend struct IntegratingFunctor;

  vtkLagrangianThreadedData* SerialThreadedData;

private:
  vtkLagrangianParticleTracker(const vtkLagrangianParticleTracker&) = delete;
  void operator=(const vtkLagrangianParticleTracker&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
