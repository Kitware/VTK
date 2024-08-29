// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLagrangianBasicIntegrationModel
 * @brief   vtkFunctionSet abstract implementation to be used
 * in the vtkLagrangianParticleTracker integrator.
 *
 * This vtkFunctionSet abstract implementation
 * is meant to be used as a parameter of vtkLagrangianParticleTracker.
 * It manages multiple dataset locators in order to evaluate the
 * vtkFunctionSet::FunctionValues method.
 * The actual FunctionValues implementation should be found in the class inheriting
 * this class.
 * Input Arrays to process are expected as follows:
 * Index 0 : "SurfaceType" array of surface input of the particle tracker
 *
 * Inherited classes MUST implement
 * int FunctionValues(vtkDataSet* dataSet, vtkIdType cellId, double* weights,
 *    double * x, double * f);
 * to define how the integration works.
 *
 * Inherited classes could reimplement InteractWithSurface or other surface interaction methods
 * to change the way particles interact with surfaces.
 *
 * Inherited classes could reimplement IntersectWithLine to use a specific algorithm
 * to intersect particles and surface cells.
 *
 * Inherited classes could reimplement CheckFreeFlightTermination to set
 * the way particles terminate in free flight.
 *
 * Inherited classes could reimplement Initialize*Data and Insert*Data in order
 * to customize the output of the tracker
 *
 * @sa
 * vtkLagrangianParticleTracker vtkLagrangianParticle
 * vtkLagrangianMatidaIntegrationModel
 */

#ifndef vtkLagrangianBasicIntegrationModel_h
#define vtkLagrangianBasicIntegrationModel_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkFunctionSet.h"
#include "vtkNew.h"         // For arrays
#include "vtkWeakPointer.h" // For weak pointer

#include <map>   // for array indexes
#include <mutex> // for mutexes
#include <queue> // for new particles

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractArray;
class vtkAbstractCellLocator;
class vtkCell;
class vtkCellData;
class vtkDataArray;
class vtkDataObject;
class vtkDataSet;
class vtkDataSetsType;
class vtkDoubleArray;
class vtkFieldData;
class vtkGenericCell;
class vtkInitialValueProblemSolver;
class vtkIntArray;
class vtkLagrangianParticle;
class vtkLagrangianParticleTracker;
class vtkLocatorsType;
class vtkMultiBlockDataSet;
class vtkMultiPieceDataSet;
class vtkPointData;
class vtkPolyData;
class vtkStringArray;
class vtkSurfaceType;
struct vtkLagrangianThreadedData;

class VTKFILTERSFLOWPATHS_EXPORT vtkLagrangianBasicIntegrationModel : public vtkFunctionSet
{
public:
  vtkTypeMacro(vtkLagrangianBasicIntegrationModel, vtkFunctionSet);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  typedef enum SurfaceType
  {
    SURFACE_TYPE_MODEL = 0,
    SURFACE_TYPE_TERM = 1,
    SURFACE_TYPE_BOUNCE = 2,
    SURFACE_TYPE_BREAK = 3,
    SURFACE_TYPE_PASS = 4
  } SurfaceType;

  typedef enum VariableStep
  {
    VARIABLE_STEP_PREV = -1,
    VARIABLE_STEP_CURRENT = 0,
    VARIABLE_STEP_NEXT = 1,
  } VariableStep;

  typedef std::pair<unsigned int, vtkLagrangianParticle*> PassThroughParticlesItem;
  typedef std::queue<PassThroughParticlesItem> PassThroughParticlesType;

  using Superclass::FunctionValues;
  /**
   * Evaluate integration model velocity f at position x.
   * Look for the cell containing the position x in all its added datasets
   * if found this will call
   * FunctionValues(vtkDataSet* dataSet, vtkIdType cellId, double* x, double* f)
   * This method is thread safe.
   */
  int FunctionValues(double* x, double* f, void* userData) override;

  ///@{
  /**
   * Set/Get the locator used to locate cells in the datasets.
   * Only the locator class matter here, as it is used only to
   * create NewInstance of it.
   * Default is a vtkCellLocator.
   */
  virtual void SetLocator(vtkAbstractCellLocator* locator);
  vtkGetObjectMacro(Locator, vtkAbstractCellLocator);
  ///@}

  ///@{
  /**
   * Get the state of the current locators
   */
  vtkGetMacro(LocatorsBuilt, bool);
  vtkSetMacro(LocatorsBuilt, bool);
  ///@}

  /**
   * Set the parent tracker.
   */
  virtual void SetTracker(vtkLagrangianParticleTracker* Tracker);

  ///@{
  /**
   * Add a dataset to locate cells in
   * This create a specific locator for the provided dataset
   * using the Locator member of this class
   * The surface flag allow to manage surfaces datasets for surface interaction
   * instead of flow datasets
   * surfaceFlatIndex, used only with composite surface, in order to identify the
   * flatIndex of the surface for particle interaction
   */
  virtual void AddDataSet(
    vtkDataSet* dataset, bool surface = false, unsigned int surfaceFlatIndex = 0);
  virtual void ClearDataSets(bool surface = false);
  ///@}

  ///@{
  /**
   * Set/Get the Use of initial integration input array to process
   */
  vtkSetMacro(UseInitialIntegrationTime, bool);
  vtkGetMacro(UseInitialIntegrationTime, bool);
  vtkBooleanMacro(UseInitialIntegrationTime, bool);
  ///@}

  ///@{
  /**
   * Get the tolerance to use with this model.
   */
  vtkGetMacro(Tolerance, double);
  ///@}

  ///@{
  /**
   * Get the specific tolerance to use with the locators.
   */
  vtkGetMacro(LocatorTolerance, double);
  ///@}

  /**
   * Interact the current particle with a surfaces
   * Return a particle to record as interaction point if not nullptr
   * Uses SurfaceType array from the intersected surface cell
   * to compute the interaction.
   * MODEL :
   * vtkLagrangianBasicIntegrationModel::InteractWithSurface
   * method will be used, usually defined in inherited classes
   * TERM :
   * vtkLagrangianBasicIntegrationModel::Terminate method will be used
   * BOUNCE :
   * vtkLagrangianBasicIntegrationModel::Bounce method will be used
   * BREAK_UP :
   * vtkLagrangianBasicIntegrationModel::BreakUp method will be used
   * PASS : The interaction will be recorded
   * with no effect on the particle
   */
  virtual vtkLagrangianParticle* ComputeSurfaceInteraction(vtkLagrangianParticle* particle,
    std::queue<vtkLagrangianParticle*>& particles, unsigned int& interactedSurfaceFlatIndex,
    PassThroughParticlesType& passThroughParticles);

  /**
   * Set a input array to process at a specific index, identified by a port,
   * connection, fieldAssociation and a name.
   * Each inherited class can specify their own input array to process
   */
  virtual void SetInputArrayToProcess(
    int idx, int port, int connection, int fieldAssociation, const char* name);

  /**
   * Look for a dataset in this integration model
   * containing the point x. return false if out of domain,
   * return true and data to recover the cell if in domain.
   * does not filter out ghost cells.
   * convenience method with less outputs.
   * Provide a particle if a dataset/locator cache can be used.
   * This method is thread-safe.
   */
  virtual bool FindInLocators(double* x, vtkLagrangianParticle* particle, vtkDataSet*& dataset,
    vtkIdType& cellId, vtkAbstractCellLocator*& loc, double*& weights);

  ///@{
  /**
   * Convenience methods to call FindInLocators with less arguments
   * THESE METHODS ARE NOT THREAD-SAFE
   */
  virtual bool FindInLocators(
    double* x, vtkLagrangianParticle* particle, vtkDataSet*& dataset, vtkIdType& cellId);
  virtual bool FindInLocators(double* x, vtkLagrangianParticle* particle);
  ///@}

  /**
   * Initialize a particle by setting user variables and perform any user
   * model specific operation. empty in basic implementation.
   */
  virtual void InitializeParticle(vtkLagrangianParticle* vtkNotUsed(particle)) {}

  /**
   * Method to be reimplemented if needed in inherited classes.
   * Allows a inherited class to check if adaptive step reintegration
   * should be done or not, this method is called just before
   * potentially performing adaptative step reintegration,
   * the current particle is passed as an argument.
   * This method always returns true in this basis class.
   */
  virtual bool CheckAdaptiveStepReintegration(vtkLagrangianParticle* vtkNotUsed(particle))
  {
    return true;
  }

  /**
   * Method to be reimplemented if needed in inherited classes.
   * Allows a inherited class to check if a particle
   * should be terminated only based on particle parameters.
   * This method should return true if the particle must be terminated, false otherwise.
   * It always returns false in this basis class.
   * This method is thread-safe, its reimplementation should still be thread-safe.
   */
  virtual bool CheckFreeFlightTermination(vtkLagrangianParticle* vtkNotUsed(particle))
  {
    return false;
  }

  ///@{
  /**
   * Set/Get Non Planar Quad Support
   */
  vtkSetMacro(NonPlanarQuadSupport, bool);
  vtkGetMacro(NonPlanarQuadSupport, bool);
  vtkBooleanMacro(NonPlanarQuadSupport, bool);
  ///@}

  /**
   * Get the seed arrays expected name
   * Used Only be the vtkLagrangianSeedHelper in ParaView plugins
   */
  virtual vtkStringArray* GetSeedArrayNames();

  /**
   * Get the seed arrays expected number of components
   * Used Only be the vtkLagrangianSeedHelper in ParaView plugins
   */
  virtual vtkIntArray* GetSeedArrayComps();

  /**
   * Get the seed arrays expected type
   * Used Only be the vtkLagrangianSeedHelper in ParaView plugins
   */
  virtual vtkIntArray* GetSeedArrayTypes();

  /**
   * Get the surface arrays expected name
   * Used Only be the vtkLagrangianSurfaceHelper in ParaView plugins
   */
  virtual vtkStringArray* GetSurfaceArrayNames();

  /**
   * Get the surface arrays expected type
   * Used Only be the vtkLagrangianSurfaceHelper in ParaView plugins
   */
  virtual vtkIntArray* GetSurfaceArrayTypes();

  /**
   * Get the surface arrays expected values and associated enums
   * Used Only be the vtkLagrangianSurfaceHelper in ParaView plugins
   */
  virtual vtkStringArray* GetSurfaceArrayEnumValues();

  /**
   * Get the surface arrays default values for each leaf
   * Used Only be the vtkLagrangianSurfaceHelper in ParaView plugins
   */
  virtual vtkDoubleArray* GetSurfaceArrayDefaultValues();

  /**
   * Get the seed array expected number of components
   * Used Only be the vtkLagrangianSurfaceHelper in ParaView plugins
   */
  virtual vtkIntArray* GetSurfaceArrayComps();

  ///@{
  /**
   * Get the maximum weights size necessary for calling
   * FindInLocators with weights
   */
  virtual int GetWeightsSize();
  ///@}

  /**
   * Let the model define it's own way to integrate
   * Signature is very close to the integrator method signature
   * output is expected to be the same,
   * see vtkInitialValueProblemSolver::ComputeNextStep for reference
   * xcur is the current particle variables
   * xnext is the next particle variable
   * t is the current integration time
   * delT is the timeStep, which is also an output for adaptative algorithm
   * delTActual is the time step output corresponding to the actual movement of the particle
   * minStep is the minimum step time for adaptative algorithm
   * maxStep is the maximum step time for adaptative algorithm
   * maxError is the maximum acceptable error
   * error is the output of actual error
   * integrationResult is the result of the integration, see
   * vtkInitialValueProblemSolver::ErrorCodes for error report
   * otherwise it should be zero. be aware that only stagnating OUT_OF_DOMAIN
   * will be considered actual out of domain error.
   * Return true if manual integration was used, false otherwise
   * Simply return false in vtkLagrangianBasicIntegrationModel
   * implementation.
   * This method is thread-safe, its reimplementation should still be thread-safe.
   */
  virtual bool ManualIntegration(vtkInitialValueProblemSolver* integrator, double* xcur,
    double* xnext, double t, double& delT, double& delTActual, double minStep, double maxStep,
    double maxError, double cellLength, double& error, int& integrationResult,
    vtkLagrangianParticle* particle);

  /**
   * Method called by parallel algorithm
   * after receiving a particle from stream if PManualShift flag has been set to true
   * on the particle. Does nothing in base implementation
   */
  virtual void ParallelManualShift(vtkLagrangianParticle* vtkNotUsed(particle)) {}

  /**
   * Let the model allocate and initialize a threaded data.
   * This method is thread-safe, its reimplementation should still be thread-safe.
   */
  virtual vtkLagrangianThreadedData* InitializeThreadedData();

  /**
   * Let the model finalize and deallocate a user data at thread level
   * This method is called serially for each thread and does not require to be thread safe.
   */
  virtual void FinalizeThreadedData(vtkLagrangianThreadedData*& data);

  /**
   * Enable model post process on output
   * Return true if successful, false otherwise
   * Empty and Always return true with basic model
   */
  virtual bool FinalizeOutputs(
    vtkPolyData* vtkNotUsed(particlePathsOutput), vtkDataObject* vtkNotUsed(interractionOutput))
  {
    return true;
  }

  /**
   * Allow for model setup prior to Particle Initialization
   */
  virtual void PreParticleInitalization() {}

  /**
   * Enable model to modify particle before integration
   */
  virtual void PreIntegrate(std::queue<vtkLagrangianParticle*>& vtkNotUsed(particles)) {}

  /**
   * Get a seed array, as set in setInputArrayToProcess
   * from the provided seed point data
   */
  virtual vtkAbstractArray* GetSeedArray(int idx, vtkPointData* pointData);

  /**
   * Set/Get the number of tracked user data attached to the particles.
   * Tracked user data are data that are related to each particle position
   * but are not integrated like the user variables.
   * They are not saved in the particle path.
   * Default is 0.
   */
  vtkSetMacro(NumberOfTrackedUserData, int);
  vtkGetMacro(NumberOfTrackedUserData, int);

  /**
   * Method used by the LPT to initialize data insertion in the provided
   * vtkFieldData. It initializes Id, ParentID, SeedID and Termination.
   * Reimplement as needed in accordance with InsertPathData.
   */
  virtual void InitializePathData(vtkFieldData* data);

  /**
   * Method used by the LPT to initialize data insertion in the provided
   * vtkFieldData. It initializes Interaction.
   * Reimplement as needed in accordance with InsertInteractionData.
   */
  virtual void InitializeInteractionData(vtkFieldData* data);

  /**
   * Method used by the LPT to initialize data insertion in the provided
   * vtkFieldData. It initializes StepNumber, ParticleVelocity, IntegrationTime.
   * Reimplement as needed in accordance with InsertParticleData.
   */
  virtual void InitializeParticleData(vtkFieldData* particleData, int maxTuples = 0);

  /**
   * Method used by the LPT to insert data from the particle into
   * the provided vtkFieldData. It inserts Id, ParentID, SeedID and Termination.
   * Reimplement as needed in accordance with InitializePathData.
   */
  virtual void InsertPathData(vtkLagrangianParticle* particle, vtkFieldData* data);

  /**
   * Method used by the LPT to insert data from the particle into
   * the provided vtkFieldData. It inserts Interaction.
   * Reimplement as needed in accordance with InitializeInteractionData.
   */
  virtual void InsertInteractionData(vtkLagrangianParticle* particle, vtkFieldData* data);

  /**
   * Method used by the LPT to insert data from the particle into
   * the provided vtkFieldData. It inserts StepNumber, ParticleVelocity, IntegrationTime.
   * stepEnum enables to select which data to insert, Prev, Current or Next.
   * Reimplement as needed in accordance with InitializeParticleData.
   */
  virtual void InsertParticleData(
    vtkLagrangianParticle* particle, vtkFieldData* data, int stepEnum);

  /**
   * Method used by the LPT to insert data from the particle into
   * the provided vtkFieldData. It inserts all arrays from the original SeedData.
   * Reimplement as needed.
   */
  virtual void InsertParticleSeedData(vtkLagrangianParticle* particle, vtkFieldData* data);

  /**
   * Method to be reimplemented if needed in inherited classes.
   * Allows a inherited class to take action just before a particle is deleted
   * This can be practical when working with vtkLagrangianParticle::TemporaryUserData.
   * This can be called with not fully initialized particle.
   */
  virtual void ParticleAboutToBeDeleted(vtkLagrangianParticle* vtkNotUsed(particle)) {}

  /**
   * Method to be reimplemented if needed in inherited classes.
   * Allows a inherited class to add surface interaction data from the model
   */
  virtual void InsertSurfaceInteractionData(
    vtkLagrangianParticle* vtkNotUsed(particle), vtkPointData* vtkNotUsed(particleData))
  {
  }

protected:
  vtkLagrangianBasicIntegrationModel();
  ~vtkLagrangianBasicIntegrationModel() override;

  /**
   * Actually compute the integration model velocity field
   * pure abstract, to be implemented in inherited class
   * This method implementation should be thread-safe
   */
  virtual int FunctionValues(vtkLagrangianParticle* particle, vtkDataSet* dataSet, vtkIdType cellId,
    double* weights, double* x, double* f) = 0;

  /**
   * Look in the given dataset and associated locator to see if it contains
   * the point x, if so return the cellId and output the cell containing the point
   * and the weights of the point in the cell
   * This method is thread-safe, its reimplementation should also be.
   */
  virtual vtkIdType FindInLocator(vtkDataSet* dataSet, vtkAbstractCellLocator* locator, double* x,
    vtkGenericCell* cell, double* weights);

  /**
   * Terminate a particle, by positioning flags.
   * Return true to record the interaction, false otherwise
   * This method is thread-safe.
   */
  virtual bool TerminateParticle(vtkLagrangianParticle* particle);

  /**
   * Bounce a particle, using the normal of the cell it bounces on.
   * Return true to record the interaction, false otherwise
   * This method is thread-safe.
   */
  virtual bool BounceParticle(
    vtkLagrangianParticle* particle, vtkDataSet* surface, vtkIdType cellId);

  /**
   * Breakup a particle at intersection point, by terminating it and creating two
   * new particle using the intersected cells normals
   * Return true to record the interaction, false otherwise
   * This method is thread-safe and uses vtkLagrangianBasicIntegrationModel::ParticleQueueMutex
   * to access the particles queue, its reimplementation should also be.
   */
  virtual bool BreakParticle(vtkLagrangianParticle* particle, vtkDataSet* surface, vtkIdType cellId,
    std::queue<vtkLagrangianParticle*>& particles);

  /**
   * Call vtkLagrangianBasicIntegrationModel::Terminate
   * This method is to be reimplemented in inherited classes willing
   * to implement specific particle surface interactions
   * Return true to record the interaction, false otherwise
   * This method is thread-safe and should use
   * vtkLagrangianBasicIntegrationModel::ParticleQueueMutex
   *  to add particles to the particles queue, see BreakParticle for an example.
   */
  virtual bool InteractWithSurface(int surfaceType, vtkLagrangianParticle* particle,
    vtkDataSet* surface, vtkIdType cellId, std::queue<vtkLagrangianParticle*>& particles);

  /**
   * Call vtkCell::IntersectWithLine
   * This method is to be reimplemented in inherited classes willing
   * to implement specific line/surface intersection
   * This method is thread-safe.
   */
  virtual bool IntersectWithLine(vtkLagrangianParticle* particle, vtkCell* cell, double p1[3],
    double p2[3], double tol, double& t, double x[3]);

  /**
   * compute all particle variables using interpolation factor
   * This method is thread-safe.
   */
  virtual void InterpolateNextParticleVariables(
    vtkLagrangianParticle* particle, double interpolationFactor, bool forceInside = false);

  /**
   * Given a particle, check if it perforate a surface cell
   * ie : interact with next step after interacting with it
   * This method is thread-safe.
   */
  virtual bool CheckSurfacePerforation(
    vtkLagrangianParticle* particle, vtkDataSet* surface, vtkIdType cellId);

  /**
   * Get a seed array, as set in setInputArrayToProcess
   * from the provided particle seed data
   * Access then the first tuple to access the data
   * This method is thread-safe.
   */
  virtual vtkAbstractArray* GetSeedArray(int idx, vtkLagrangianParticle* particle);

  /**
   * Directly get a double value from flow or surface data
   * as defined in SetInputArrayToProcess.
   * Make sure that data pointer is large enough using
   * GetFlowOrSurfaceDataNumberOfComponents if needed.
   * This method is thread-safe.
   */
  virtual bool GetFlowOrSurfaceData(vtkLagrangianParticle* particle, int idx,
    vtkDataSet* flowDataSet, vtkIdType tupleId, double* weights, double* data);

  /**
   * Recover the number of components for a specified array index
   * if it has been set using SetInputArrayToProcess,
   * with provided dataset.
   * Returns -1 in case of error.
   * This method is thread-safe.
   */
  virtual int GetFlowOrSurfaceDataNumberOfComponents(int idx, vtkDataSet* dataSet);

  /**
   * Recover a field association for a specified array index
   * if it has been set using SetInputArrayToProcess
   * This method is thread-safe.
   */
  virtual int GetFlowOrSurfaceDataFieldAssociation(int idx);

  /**
   * Method used by ParaView surface helper to get default
   * values for each leaf of each dataset of surface
   * nComponents could be retrieved with arrayName but is
   * given for simplification purposes.
   * it is your responsibility to initialize all components of
   * defaultValues[nComponent]
   */
  virtual void ComputeSurfaceDefaultValues(
    const char* arrayName, vtkDataSet* dataset, int nComponent, double* defaultValues);

  vtkAbstractCellLocator* Locator;
  bool LocatorsBuilt;
  vtkLocatorsType* Locators;
  vtkDataSetsType* DataSets;
  int WeightsSize = 0;

  struct ArrayVal
  {
    int val[3];
  };
  typedef std::pair<ArrayVal, std::string> ArrayMapVal;
  std::map<int, ArrayMapVal> InputArrays;

  typedef struct SurfaceArrayDescription
  {
    int nComp;
    int type;
    std::vector<std::pair<int, std::string>> enumValues;
  } SurfaceArrayDescription;
  std::map<std::string, SurfaceArrayDescription> SurfaceArrayDescriptions;

  vtkSurfaceType* Surfaces;
  vtkLocatorsType* SurfaceLocators;

  double Tolerance;
  double LocatorTolerance = 0.001;
  bool NonPlanarQuadSupport;
  bool UseInitialIntegrationTime;
  int NumberOfTrackedUserData = 0;

  vtkNew<vtkStringArray> SeedArrayNames;
  vtkNew<vtkIntArray> SeedArrayComps;
  vtkNew<vtkIntArray> SeedArrayTypes;
  vtkNew<vtkStringArray> SurfaceArrayNames;
  vtkNew<vtkIntArray> SurfaceArrayComps;
  vtkNew<vtkIntArray> SurfaceArrayTypes;
  vtkNew<vtkStringArray> SurfaceArrayEnumValues;
  vtkNew<vtkDoubleArray> SurfaceArrayDefaultValues;

  vtkWeakPointer<vtkLagrangianParticleTracker> Tracker;
  std::mutex ParticleQueueMutex;

private:
  vtkLagrangianBasicIntegrationModel(const vtkLagrangianBasicIntegrationModel&) = delete;
  void operator=(const vtkLagrangianBasicIntegrationModel&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
