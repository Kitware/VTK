/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangianParticleTracker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
 * * port 0 : ParticlePaths : a polyData of polyLines showing the paths of
 *     particles in the flow
 * * port 1 : ParticleInteractions : empty if no surface input, contains a
 *     polydata of vertex
 * with the same composite layout of surface input if any, showing all
 *     interactions between particles and the surface input
 *
 * It has a parallel implementation which streams particle between domains.
 *
 * The most important parameters of this filter is it's integrationModel.
 * Only one integration model implementation exist currently in ParaView
 * ,vtkLagrangianMatidaIntegrationModel but the design enables plugin developpers
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
 *  * Optional lines rendering controlled by a treshold
 *  * Ghost cell support
 *  * Non planar quad interactionn support
 *  * Built-in support for surface interaction including, terminate, bounce,
 *      break-up and pass-trough surface
 * The serial and parallel filters are fully tested.
 *
 * @sa
 * vtkLagrangianMatidaIntegrationModel vtkLagrangianParticle
 * vtkLagrangianBasicIntegrationModel
*/

#ifndef vtkLagrangianParticleTracker_h
#define vtkLagrangianParticleTracker_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkDataObjectAlgorithm.h"

#include <queue> // for particle queue

class vtkBoundingBox;
class vtkCellArray;
class vtkDataSet;
class vtkDoubleArray;
class vtkIdList;
class vtkInformation;
class vtkInitialValueProblemSolver;
class vtkLagrangianBasicIntegrationModel;
class vtkLagrangianParticle;
class vtkPointData;
class vtkPoints;
class vtkPolyData;

class VTKFILTERSFLOWPATHS_EXPORT vtkLagrangianParticleTracker :
  public vtkDataObjectAlgorithm
{
public:

  vtkTypeMacro(vtkLagrangianParticleTracker, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkLagrangianParticleTracker *New();

  typedef enum CellLengthComputation{
    STEP_LAST_CELL_LENGTH = 0,
    STEP_CUR_CELL_LENGTH = 1,
    STEP_LAST_CELL_VEL_DIR = 2,
    STEP_CUR_CELL_VEL_DIR = 3,
    STEP_LAST_CELL_DIV_THEO = 4,
    STEP_CUR_CELL_DIV_THEO = 5
  } CellLengthComputation;

  //@{
  /**
   * Set/Get the integration model.
   */
  void SetIntegrationModel(vtkLagrangianBasicIntegrationModel* integrationModel);
  vtkGetObjectMacro(IntegrationModel, vtkLagrangianBasicIntegrationModel);
  //@}

  //@{
  /**
   * Set/Get the integrator.
   */
  void SetIntegrator(vtkInitialValueProblemSolver* integrator);
  vtkGetObjectMacro(Integrator, vtkInitialValueProblemSolver);
  //@}

  //@{
  /**
   * Set/Get the cell length computation mode.
   * Available modes are :
   * - STEP_LAST_CELL_LENGTH :
   * Compute cell length using getLength method
   * on the last cell the particle was in
   * - STEP_CUR_CELL_LENGTH :
   * Compute cell length using getLength method
   * on the current cell the particle is in
   * - STEP_LAST_CELL_VEL_DIR :
   * Compute cell length using the particle velocity
   * and the edges of the last cell the particle was in.
   * - STEP_CUR_CELL_VEL_DIR :
   * Compute cell length using the particle velocity
   * and the edges of the last cell the particle was in.
   * - STEP_LAST_CELL_DIV_THEO :
   * Compute cell length using the particle velocity
   * and the divergence theorem, not supported
   * with vtkVoxel, fallback to STEP_LAST_CELL_LENGTH
   * - STEP_CUR_CELL_DIV_THEO :
   * Compute cell length using the particle velocity
   * and the divergence theorem, not supported
   * with vtkVoxel, fallback to STEP_CUR_CELL_LENGTH
   */
  vtkSetMacro(CellLengthComputationMode, int);
  vtkGetMacro(CellLengthComputationMode, int);
  //@}

  //@{
  /**
   * Set/Get the integration step factor.
   */
  vtkSetMacro(StepFactor, double);
  vtkGetMacro(StepFactor, double);
  //@}

  //@{
  /**
   * Set/Get the integration step factor min.
   */
  vtkSetMacro(StepFactorMin, double);
  vtkGetMacro(StepFactorMin, double);
  //@}

  //@{
  /**
   * Set/Get the integration step factor max.
   */
  vtkSetMacro(StepFactorMax, double);
  vtkGetMacro(StepFactorMax, double);
  //@}

  //@{
  /**
   * Set/Get the maximum number of steps.
   */
  vtkSetMacro(MaximumNumberOfSteps, int);
  vtkGetMacro(MaximumNumberOfSteps, int);
  //@}

  //@{
  /**
   * Set/Get the Adaptive Step Reintegration feature.
   * it checks the step size after the integration
   * and if it is too big will retry with a smaller step
   */
  vtkSetMacro(AdaptiveStepReintegration, bool);
  vtkGetMacro(AdaptiveStepReintegration, bool);
  vtkBooleanMacro(AdaptiveStepReintegration, bool);
  //@}

  //@{
  /**
   * Set/Get the Optional Paths Rendering feature
   */
  vtkSetMacro(UseParticlePathsRenderingThreshold, bool);
  vtkGetMacro(UseParticlePathsRenderingThreshold, bool);
  vtkBooleanMacro(UseParticlePathsRenderingThreshold, bool);
  //@}

  //@{
  /**
   * Set/Get the Optional Paths Rendering feature
   */
  vtkSetMacro(ParticlePathsRenderingPointsThreshold, int);
  vtkGetMacro(ParticlePathsRenderingPointsThreshold, int);
  //@}

  //@{
  /**
   * Set/Get the Creation of particle initially outside of the domain
   */
  vtkSetMacro(CreateOutOfDomainParticle, bool);
  vtkGetMacro(CreateOutOfDomainParticle, bool);
  vtkBooleanMacro(CreateOutOfDomainParticle, bool);
  //@}

  //@{
  /**
   * Specify the source object used to generate particle initial position (seeds).
   * Note that this method does not connect the pipeline. The algorithm will
   * work on the input data as it is without updating the producer of the data.
   * See SetSourceConnection for connecting the pipeline.
   */
  void SetSourceData(vtkDataObject* source);
  vtkDataObject* GetSource();
  //@}

  /**
   * Specify the source object used to generate particle initial position (seeds).
   */
  void SetSourceConnection(vtkAlgorithmOutput* algOutput);

  //@{
  /**
   * Specify the source object used to compute surface interaction with
   * Note that this method does not connect the pipeline. The algorithm will
   * work on the input data as it is without updating the producer of the data.
   * See SetSurfaceConnection for connecting the pipeline.
   */
  void SetSurfaceData(vtkDataObject *source);
  vtkDataObject *GetSurface();
  //@}

  /**
   * Specify the object used to compute surface interaction with.
   */
  void SetSurfaceConnection(vtkAlgorithmOutput* algOutput);

  /**
   * Declare input port type
   */
  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  /**
   * Declare output port type
   */
  int FillOutputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  /**
   * Create outputs objects.
   */
  int RequestDataObject(vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*) VTK_OVERRIDE;

  /**
   * Process inputs to integrate particle and generate output.
   */
  int RequestData(vtkInformation *request,
    vtkInformationVector **inputVector,
    vtkInformationVector *outputVector) VTK_OVERRIDE;

  /**
   * Get the tracker modified time taking into account the integration model
   * and the integrator.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

  /**
   * Get an unique id for a particle
   */
  virtual vtkIdType GetNewParticleId();

protected:
  vtkLagrangianParticleTracker();
  ~vtkLagrangianParticleTracker() VTK_OVERRIDE;

  virtual bool InitializeInputs(vtkInformationVector **inputVector,
    vtkDataObject*& flow, vtkDataObject*& seeds, vtkDataObject*& surfaces,
    std::queue<vtkLagrangianParticle*>& particleQueue, vtkPointData* seedData);
  virtual bool InitializeFlow(vtkDataObject* flow, vtkBoundingBox* bounds);
  virtual bool InitializeParticles(const vtkBoundingBox* bounds, vtkDataObject* seeds,
    std::queue<vtkLagrangianParticle*>& particles, vtkPointData* seedData);
  virtual void GenerateParticles(const vtkBoundingBox* bounds, vtkDataSet* seeds,
    vtkDataArray* initialVelocities, vtkDataArray* initialIntegrationTimes,
    vtkPointData* seedData, int nVar, std::queue<vtkLagrangianParticle*>& particles);
  virtual void InitializeSurface(vtkDataObject*& surfaces);
  virtual bool InitializeOutputs(vtkInformationVector *outputVector, vtkPointData* seedData,
    vtkIdType numberOfSeeds, vtkDataObject* surfaces,
    vtkPolyData*& particlePathsOutput, vtkDataObject*& interactionOutput);

  virtual bool InitializePathsOutput(vtkInformationVector *outputVector,
    vtkPointData* seedData, vtkIdType numberOfSeeds,
    vtkPolyData*& particlePathsOutput);

  virtual bool InitializeInteractionOutput(vtkInformationVector *outputVector,
    vtkPointData* seedData, vtkDataObject* surfaces, vtkDataObject*& interractionOutput);

  virtual void InitializeParticleData(vtkFieldData* particleData, int maxTuples = 0);
  virtual void InitializePathData(vtkFieldData* data);
  virtual void InitializeInteractionData(vtkFieldData* data);

  virtual bool FinalizeOutputs(vtkPolyData* particlePathsOutput,
    vtkDataObject* interractionOutput);

  static void InsertPolyVertexCell(vtkPolyData* polydata);

  virtual void GetParticleFeed(std::queue<vtkLagrangianParticle*>& particleQueue);

  virtual int Integrate(vtkLagrangianParticle*, std::queue<vtkLagrangianParticle*>&,
    vtkPolyData* particlePathsOutput, vtkIdList* particlePathPointId,
    vtkDataObject* interactionOutput);

  void InsertPathOutputPoint(vtkLagrangianParticle* particle,
    vtkPolyData* particlePathsOutput, vtkIdList* particlePathPointId,
    bool prev = false);

  void InsertInteractionOutputPoint(vtkLagrangianParticle* particle,
    unsigned int interactedSurfaceFlatIndex, vtkDataObject* interactionOutput);

  void InsertSeedData(vtkLagrangianParticle* particle, vtkFieldData* data);
  void InsertPathData(vtkLagrangianParticle* particle, vtkFieldData* data);
  void InsertInteractionData(vtkLagrangianParticle* particle, vtkFieldData* data);
  void InsertParticleData(vtkLagrangianParticle* particle, vtkFieldData* data, int stepEnum);

  double ComputeCellLength(vtkLagrangianParticle* particle);

  bool ComputeNextStep(
    double* xprev, double* xnext,
    double t, double& delT, double& delTActual,
    double minStep, double maxStep,
    int& integrationRes);

  virtual bool CheckParticlePathsRenderingThreshold(vtkPolyData* particlePathsOutput);

  vtkLagrangianBasicIntegrationModel* IntegrationModel;
  vtkInitialValueProblemSolver* Integrator;

  int CellLengthComputationMode;
  double StepFactor;
  double StepFactorMin;
  double StepFactorMax;
  int MaximumNumberOfSteps;
  bool AdaptiveStepReintegration;
  bool UseParticlePathsRenderingThreshold;
  int ParticlePathsRenderingPointsThreshold;
  bool CreateOutOfDomainParticle;
  vtkIdType ParticleCounter;

  // internal parameters use for step computation
  double MinimumVelocityMagnitude;
  double MinimumReductionFactor;
private:
  vtkLagrangianParticleTracker(const vtkLagrangianParticleTracker&) VTK_DELETE_FUNCTION;
  void operator=(const vtkLagrangianParticleTracker&) VTK_DELETE_FUNCTION;
};

#endif
