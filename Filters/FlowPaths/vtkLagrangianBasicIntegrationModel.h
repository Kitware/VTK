/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangianBasicIntegrationModel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLagrangianBasicIntegrationModel
 * @brief   vtkFunctionSet abstract implementation to be used
 * in the vtkLagrangianParticleTracker integrator.
 *
 * This vtkFunctionSet abstract implementation
 * is meant to be used as a parameter of vtkLagrangianParticleTracker.
 * It manages multiples datasets locator in order to evaluate the
 * vtkFunctionSet::FunctionValues method.
 * The actual FunctionValues implementation should be found in class inheriting
 * this class
 * Input Array to process are expected as follows :
 * Index 0 : "SurfaceType" array of surface input of the particle tracker
 *
 * Inherited classes MUST implement
 * int FunctionValues(vtkDataSet* detaSet, vtkIdType cellId, double* weights,
 *    double * x, double * f);
 * to define how the integration works.
 *
 * Inherited classes could reimplement SetCurrentParticle, InitializeVariablesParticleData, and
 * InsertVariablesParticleData to add new UserVariables to integrate with
 *
 * Inherited classes could reimplement InteractWithSurface or other surface interaction method
 * to changes the way particle interact with surface
 *
 * Inherited classes could reimplement IntersectWithLine to use a specific algorithm
 * to intersect particles and surface cells.
 *
 * Inherited class could reimplement CheckFreeFlightTermination to set
 * the way particle terminate in free flight
 *
 * @sa
 * vtkLagrangianParticleTracker vtkLagrangianParticle
 * vtkLagrangianMatidaIntegrationModel
*/

#ifndef vtkLagrangianBasicIntegrationModel_h
#define vtkLagrangianBasicIntegrationModel_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkNew.h" // For arrays
#include "vtkWeakPointer.h" // For weak pointer
#include "vtkFunctionSet.h"

#include <queue> // for new particles
#include <map> // for array indexes

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
class vtkIntArray;
class vtkLagrangianParticle;
class vtkLagrangianParticleTracker;
class vtkLocatorsType;
class vtkPointData;
class vtkPolyData;
class vtkStringArray;
class vtkSurfaceType;

class VTKFILTERSFLOWPATHS_EXPORT vtkLagrangianBasicIntegrationModel :
  public vtkFunctionSet
{
public:
  vtkTypeMacro(vtkLagrangianBasicIntegrationModel, vtkFunctionSet);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

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

  typedef std::pair< unsigned int, vtkLagrangianParticle*> PassThroughParticlesItem;
  typedef std::queue< PassThroughParticlesItem > PassThroughParticlesType;

  /**
   * Evaluate integration model velocity f at position x.
   * Look for the cell containing the position x in all it's added dataset
   * in found this will call
   * FunctionValues(vtkDataSet* detaSet, vtkIdType cellId, double* x, double* f)
   */
  int FunctionValues(double* x, double* f) VTK_OVERRIDE;

  //@{
  /**
   * Set/Get the locator used to locate cells in the datasets.
   * Only the locator class matter here, as it is used only to
   * create NewInstance of it.
   * Default is a vtkCellLocator.
   */
  virtual void SetLocator(vtkAbstractCellLocator* locator);
  vtkGetObjectMacro(Locator, vtkAbstractCellLocator);
  //@}

  /**
   * Set the parent tracker.
   */
  virtual void SetTracker(vtkLagrangianParticleTracker* Tracker);

  //@{
  /**
   * Add a dataset to locate cells in
   * This create a specific locator for the provided dataset
   * using the Locator member of this class
   * The surface flag allow to manage surfaces datasets for surface interaction
   * instead of flow datasets
   * surfaceFlatIndex, used only with composite surface, in order to identify the
   * flatIndex of the surface for particle interaction
   */
  virtual void AddDataSet(vtkDataSet* dataset, bool surface = false,
    unsigned int surfaceFlatIndex = 0);
  virtual void ClearDataSets(bool surface = false);
  //@}

  //@{
  /**
   * Set/Get the Use of initial integration input array to process
   */
  vtkSetMacro(UseInitialIntegrationTime, bool);
  vtkGetMacro(UseInitialIntegrationTime, bool);
  vtkBooleanMacro(UseInitialIntegrationTime, bool);
  //@}

  //@{
  /**
   * Set the current particle
   */
  vtkSetMacro(CurrentParticle, vtkLagrangianParticle*);
  //@}

  //@{
  /**
   * Get the tolerance to use with this model
   */
  vtkGetMacro(Tolerance, double);
  //@}

  /**
   * Interact the current particle with a surfaces
   * Return a particle to record as interaction point if not NULL
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
    std::queue<vtkLagrangianParticle*>& particles,
    unsigned int& interactedSurfaceFlatIndex, PassThroughParticlesType& passThroughParticles);

  /**
   * Set a input array to process at a specific index, indentified by a port,
   * connection, fieldAssociation and a name.
   * Each inherited class can specify their own input array to process
   */
  virtual void SetInputArrayToProcess(int idx, int port, int connection,
    int fieldAssociation, const char* name);

  //@{
  /**
   * Look for a dataset in this integration model
   * containing the point x. return false if out of domain,
   * return true and data to recover the cell if in domain.
   * does not filter out ghost cells.
   * convienence method with less outputs.
   */
  virtual bool FindInLocators(double* x, vtkDataSet*& dataset, vtkIdType& cellId,
    vtkAbstractCellLocator*& loc, double*& weights);
  virtual bool FindInLocators(double* x, vtkDataSet*& dataset, vtkIdType& cellId);
  virtual bool FindInLocators(double* x);
  //@}

  /**
   * Empty method to be reimplemented if necessary
   * in inherited classes.
   * Allows a intehrited class to create
   * Specific array in the output point data
   * for storing variables.
   */
  virtual void InitializeVariablesParticleData(
    vtkPointData* vtkNotUsed(particleData), int vtkNotUsed(maxTuples) = 0) {}

  /**
   * Empty method to be reimplemented if necessary in inherited classes.
   * Allows a inherited class to create
   * Specific array in the output point data
   * for filling point data with variables.
   */
  virtual void InsertVariablesParticleData(vtkLagrangianParticle* vtkNotUsed(particle),
    vtkPointData* vtkNotUsed(particleData), int vtkNotUsed(stepEnum)) {}

  /**
   * Initialize a particle by setting user variables
   * and perform any user model specific operation
   * empty in basic implementation
   */
  virtual void InitializeParticle(vtkLagrangianParticle* vtkNotUsed(particle)){}

  /**
   * Empty method to be reimplemented if necessary in inherited classes.
   * Allows a inherited class to check if a particle
   * should be terminated only based on particle parameters.
   * This method return true if the particle must be terminated, false otherwise.
   */
  virtual bool CheckFreeFlightTermination(
    vtkLagrangianParticle* vtkNotUsed(particle)){return false;}

  //@{
  /**
   * Set/Get Non Planar Quad Support
   */
  vtkSetMacro(NonPlanarQuadSupport, bool);
  vtkGetMacro(NonPlanarQuadSupport, bool);
  vtkBooleanMacro(NonPlanarQuadSupport, bool);
  //@}

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

  //@{
  /**
   * Get the maximum weights size necessary for calling
   * FindInLocators with weights
   */
  vtkGetMacro(WeightsSize, int);
  //@}

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
   * implementation
   */
  virtual bool ManualIntegration(double* xcur, double* xnext,
    double t, double& delT, double& delTActual,
    double minStep, double maxStep,
    double maxError, double& error, int& integrationResult);

  /**
   * Method called by parallel algorithm
   * after receiving a particle from stream if PManualShift flag has been set to true
   * on the particle. Does nothing in base implementation
   */
  virtual void ParallelManualShift(vtkLagrangianParticle* vtkNotUsed(particle)){}

  /**
   * Enable model post process on output
   * Return true if sucessful, false otherwise
   * Empty and Always return true with basic model
   */
  virtual bool FinalizeOutputs(vtkPolyData* vtkNotUsed(particlePathsOutput),
    vtkDataObject* vtkNotUsed(interractionOutput)){return true;}

  /**
   * Enable model to modify particle before integration
   */
  virtual void PreIntegrate(std::queue<vtkLagrangianParticle*>& vtkNotUsed(particles)){}

  /**
   * Get a seed array, as set in setInputArrayToProcess
   * from the provided seed point data
   */
  virtual vtkAbstractArray* GetSeedArray(int idx, vtkPointData* pointData);

protected:
  vtkLagrangianBasicIntegrationModel();
  ~vtkLagrangianBasicIntegrationModel() VTK_OVERRIDE;

  /**
   * Actually compute the integration model velocity field
   * pure abstract, to be implemented in inherited class
   */
  virtual int FunctionValues(vtkDataSet* detaSet, vtkIdType cellId, double* weights,
    double * x, double * f) = 0;

  /**
   * Look in the given dataset and associated locator to see if it contains
   * the point x, if so return the cellId and output the cell containing the point
   * and the weights of the point in the cell
   */
  virtual vtkIdType FindInLocator(vtkDataSet* dataSet, vtkAbstractCellLocator* locator,
    double* x, vtkGenericCell* cell, double* weights);

  /**
   * Terminate a particle, by positioning flags.
   * Return true to record the interaction, false otherwise
   */
  virtual bool TerminateParticle(vtkLagrangianParticle* particle);

  /**
   * Bounce a particle, using the normal of the cell it bounces on.
   * Return true to record the interaction, false otherwise
   */
  virtual bool BounceParticle(vtkLagrangianParticle* particle,
    vtkDataSet*  surface, vtkIdType cellId);

  /**
   * Breakup a particle at intersection point, by terminating it and creating two
   * new particle using the intersected cells normals
   * Return true to record the interaction, false otherwise
   */
  virtual bool BreakParticle(vtkLagrangianParticle* particle,
    vtkDataSet*  surface, vtkIdType cellId, std::queue<vtkLagrangianParticle*>& particles);

  /**
   * Call vtkLagrangianBasicIntegrationModel::Terminate
   * This method is to be reimplemented in inherited classes willing
   * to implement specific particle surface interactions
   * Return true to record the interaction, false otherwise
   */
  virtual bool InteractWithSurface(int surfaceType, vtkLagrangianParticle* particle,
    vtkDataSet* surface, vtkIdType cellId, std::queue<vtkLagrangianParticle*>& particles);

  /**
   * Call vtkCell::IntersectWithLine
   * This method is to be reimplemented in inherited classes willing
   * to implement specific line/surface intersection
   */
  virtual bool IntersectWithLine(vtkCell* cell, double p1[3], double p2[3],
    double tol, double& t, double x[3]);

  /**
   * compute all particle variables using interpolation factor
   */
  virtual void InterpolateNextParticleVariables(vtkLagrangianParticle* particle,
    double interpolationFactor, bool forceInside = false);

  /**
   * Given a particle, check if it perforate a surface cell
   * ie : interact with next step after interracting with it
   */
  virtual bool CheckSurfacePerforation(vtkLagrangianParticle* particle,
    vtkDataSet*  surface, vtkIdType cellId);

  /**
   * Get a seed array, as set in setInputArrayToProcess
   * from the provided particle seed data
   * Access then the correct tuple using
   * vtkLagrangianParticle::GetSeedArrayTupleIndex()
   */
  virtual vtkAbstractArray* GetSeedArray(int idx, vtkLagrangianParticle* particle);

  /**
   * Directly get a double value from flow or surface data
   * as defined in SetInputArrayToProcess
   * data pointer is valid until next call to this method.
   */
  virtual bool GetFlowOrSurfaceData(int idx, vtkDataSet* flowDataSet,
    vtkIdType tupleId, double* weights, double*& data, int& nComponents);

  /**
   * Recover a field association for a specified array index
   * if it has been set using SetInputArrayToProcess
   */
  virtual int GetFlowOrSurfaceDataFieldAssociation(int idx);

  /**
   * Methods used by ParaView surface helper to get default
   * values for each leaf of each dataset of surface
   * nComponents could be retrived with arrayName but is
   * given for simplication purposes.
   * it is your responsibility to initialize all components of
   * defaultValues[nComponent]
   */
  virtual void ComputeSurfaceDefaultValues(const char* arrayName, vtkDataSet* dataset,
                                             int nComponent, double* defaultValues);

  vtkAbstractCellLocator* Locator;
  vtkAbstractCellLocator* LastLocator;
  vtkLocatorsType* Locators;

  vtkDataSet* LastDataSet;
  vtkDataSetsType* DataSets;
  vtkGenericCell* Cell;
  double* LastWeights;
  int WeightsSize;

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
    std::vector< std::pair< int, std::string > > enumValues;
  } SurfaceArrayDescription;
  std::map<std::string, SurfaceArrayDescription> SurfaceArrayDescriptions;

  vtkLagrangianParticle* CurrentParticle;
  vtkLagrangianParticle* TmpParticle;

  vtkSurfaceType* Surfaces;
  vtkLocatorsType* SurfaceLocators;

  vtkDataArray* TmpArray;

  double Tolerance;
  bool NonPlanarQuadSupport;
  bool UseInitialIntegrationTime;

  vtkNew<vtkStringArray> SeedArrayNames;
  vtkNew<vtkIntArray> SeedArrayComps;
  vtkNew<vtkIntArray> SeedArrayTypes;
  vtkNew<vtkStringArray> SurfaceArrayNames;
  vtkNew<vtkIntArray> SurfaceArrayComps;
  vtkNew<vtkIntArray> SurfaceArrayTypes;
  vtkNew<vtkStringArray> SurfaceArrayEnumValues;
  vtkNew<vtkDoubleArray> SurfaceArrayDefaultValues;

  vtkWeakPointer<vtkLagrangianParticleTracker> Tracker;

private:
  vtkLagrangianBasicIntegrationModel(const vtkLagrangianBasicIntegrationModel&) VTK_DELETE_FUNCTION;
  void operator=(const vtkLagrangianBasicIntegrationModel&) VTK_DELETE_FUNCTION;
};

#endif
