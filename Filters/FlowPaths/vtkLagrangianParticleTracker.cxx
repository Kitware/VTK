/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangianParticleTracker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLagrangianParticleTracker.h"

#include "vtkBoundingBox.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSet.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLagrangianMatidaIntegrationModel.h"
#include "vtkLagrangianParticle.h"
#include "vtkLongLongArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataNormals.h"
#include "vtkPolyLine.h"
#include "vtkPolygon.h"
#include "vtkRungeKutta2.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkVoxel.h"

#include <algorithm>
#include <limits>
#include <sstream>

vtkObjectFactoryNewMacro(vtkLagrangianParticleTracker);
vtkCxxSetObjectMacro(vtkLagrangianParticleTracker, IntegrationModel, vtkLagrangianBasicIntegrationModel);
vtkCxxSetObjectMacro(vtkLagrangianParticleTracker, Integrator, vtkInitialValueProblemSolver);

//---------------------------------------------------------------------------
vtkLagrangianParticleTracker::vtkLagrangianParticleTracker()
{
  this->IntegrationModel = vtkLagrangianMatidaIntegrationModel::New();
  this->Integrator = vtkRungeKutta2::New();

  this->SetNumberOfInputPorts(3);
  this->SetNumberOfOutputPorts(2);

  this->CellLengthComputationMode = STEP_LAST_CELL_LENGTH;
  this->AdaptiveStepReintegration = false;
  this->StepFactor = 1.0;
  this->StepFactorMin = 0.5;
  this->StepFactorMax = 1.5;
  this->MaximumNumberOfSteps = 100;

  this->MinimumVelocityMagnitude = 0.001;
  this->MinimumReductionFactor = 1.1;

  this->UseParticlePathsRenderingThreshold = false;
  this->ParticlePathsRenderingPointsThreshold = 100;

  this->CreateOutOfDomainParticle = false;
  this->ParticleCounter = 0;
}

//---------------------------------------------------------------------------
vtkLagrangianParticleTracker::~vtkLagrangianParticleTracker()
{
  this->SetIntegrator(NULL);
  this->SetIntegrationModel(NULL);
}

//---------------------------------------------------------------------------
void vtkLagrangianParticleTracker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->IntegrationModel)
  {
    os << indent << "IntegrationModel: " << endl;
    this->IntegrationModel->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "IntegrationModel: " << this->IntegrationModel << endl;
  }
  if (this->Integrator)
  {
    os << indent << "Integrator: " << endl;
    this->Integrator->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Integrator: " << this->Integrator << endl;
  }
  os << indent << "CellLengthComputationMode: "
    << this->CellLengthComputationMode << endl;
  os << indent << "StepFactor: " << this->StepFactor << endl;
  os << indent << "StepFactorMin: " << this->StepFactorMin << endl;
  os << indent << "StepFactorMax: " << this->StepFactorMax << endl;
  os << indent << "MaximumNumberOfSteps: " << this->MaximumNumberOfSteps << endl;
  os << indent << "AdaptiveStepReintegration: " << this->AdaptiveStepReintegration << endl;
  os << indent << "UseParticlePathsRenderingThreshold: "
    << this->UseParticlePathsRenderingThreshold << endl;
  os << indent << "ParticlePathsRenderingPointsThreshold: "
    << this->ParticlePathsRenderingPointsThreshold << endl;
  os << indent << "MinimumVelocityMagnitude: " << this->MinimumVelocityMagnitude << endl;
  os << indent << "MinimumReductionFactor: " << this->MinimumReductionFactor << endl;
  os << indent << "ParticleCounter: " << this->ParticleCounter << endl;
}

//---------------------------------------------------------------------------
void vtkLagrangianParticleTracker::SetSourceConnection(vtkAlgorithmOutput* algInput)
{
  this->SetInputConnection(1, algInput);
}

//---------------------------------------------------------------------------
void vtkLagrangianParticleTracker::SetSourceData(vtkDataObject *source)
{
  this->SetInputData(1, source);
}

//---------------------------------------------------------------------------
vtkDataObject* vtkLagrangianParticleTracker::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return 0;
  }
  return vtkDataObject::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//---------------------------------------------------------------------------
void vtkLagrangianParticleTracker::SetSurfaceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(2, algOutput);
}

//---------------------------------------------------------------------------
void vtkLagrangianParticleTracker::SetSurfaceData(vtkDataObject *surface)
{
  this->SetInputData(2, surface);
}

//---------------------------------------------------------------------------
vtkDataObject* vtkLagrangianParticleTracker::GetSurface()
{
  if (this->GetNumberOfInputConnections(2) < 1)
  {
    return 0;
  }
  return this->GetExecutive()->GetInputData(2, 0);
}

//---------------------------------------------------------------------------
int vtkLagrangianParticleTracker::FillInputPortInformation(int port,
  vtkInformation *info)
{
  if (port == 2)
  {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return this->Superclass::FillInputPortInformation(port, info);
}

//----------------------------------------------------------------------------
int vtkLagrangianParticleTracker::FillOutputPortInformation(int port,
  vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  }
  return this->Superclass::FillOutputPortInformation(port, info);
}

//----------------------------------------------------------------------------
int vtkLagrangianParticleTracker::RequestDataObject(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Create a polydata output
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkNew<vtkPolyData> particlePathsOutput;
  info->Set(vtkDataObject::DATA_OBJECT(), particlePathsOutput.Get());

  // Create a surface interaction output
  // First check for composite
  vtkInformation* inInfo = inputVector[2]->GetInformationObject(0);
  info = outputVector->GetInformationObject(1);
  if (inInfo)
  {
    vtkDataObject *input = vtkDataObject::SafeDownCast(
      inInfo->Get(vtkDataObject::DATA_OBJECT()));
    if (input)
    {
      vtkCompositeDataSet *hdInput = vtkCompositeDataSet::SafeDownCast(input);
      if (hdInput)
      {
        vtkDataObject* interactionOutput = input->NewInstance();
        info->Set(vtkDataObject::DATA_OBJECT(), interactionOutput);
        interactionOutput->Delete();
        return 1;
      }
    }
  }
  // In any other case, create a polydata
  vtkNew<vtkPolyData> interactionOutput;
  info->Set(vtkDataObject::DATA_OBJECT(), interactionOutput.Get());
  return 1;
}

//---------------------------------------------------------------------------
int vtkLagrangianParticleTracker::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // Initialize inputs
  vtkDataObject* flow = 0;
  vtkDataObject* seeds = 0;
  vtkDataObject* surfaces = 0;
  std::queue<vtkLagrangianParticle*> particlesQueue;

  if (!this->IntegrationModel)
  {
    vtkErrorMacro(<< "Integration Model is NULL, cannot integrate");
    return 0;
  }
  this->IntegrationModel->SetTracker(this);

  vtkNew<vtkPointData> seedData;
  if (!this->InitializeInputs(inputVector, flow, seeds, surfaces,
    particlesQueue, seedData.Get()))
  {
    vtkErrorMacro(<< "Cannot initialize inputs");
    return 0;
  }

  // Initialize outputs
  vtkPolyData* particlePathsOutput;
  vtkDataObject* interactionOutput;
  if (!this->InitializeOutputs(outputVector, seedData.Get(),
    static_cast<vtkIdType>(particlesQueue.size()), surfaces,
    particlePathsOutput, interactionOutput))
  {
    vtkErrorMacro(<< "Cannot initialize outputs");
    return 0;
  }

  // Let model a chance to change the particles or compute things
  // before integration.
  this->IntegrationModel->PreIntegrate(particlesQueue);

  // Integrate each particle
  while (!this->GetAbortExecute())
  {
    // Check for particle feed
    this->GetParticleFeed(particlesQueue);
    if (particlesQueue.empty())
    {
      break;
    }

    // Recover particle
    vtkLagrangianParticle* particle = particlesQueue.front();
    particlesQueue.pop();

    // Create polyLine output cell
    vtkNew<vtkPolyLine> particlePath;

    // Integrate
    this->Integrate(particle, particlesQueue, particlePathsOutput,
      particlePath->GetPointIds(), interactionOutput);

    if (particlePath->GetPointIds()->GetNumberOfIds() == 1)
    {
      particlePath->GetPointIds()->InsertNextId(particlePath->GetPointId(0));
    }

    // Duplicate single point particle paths, to avoid degenerated lines.
    if (particlePath->GetPointIds()->GetNumberOfIds() > 0)
    {
      // Add particle path or vertex to cell array
      particlePathsOutput->GetLines()->InsertNextCell(particlePath.Get());
      this->InsertPathData(particle, particlePathsOutput->GetCellData());

      // Insert data from seed data only on not yet written arrays
      this->InsertSeedData(particle, particlePathsOutput->GetCellData());
    }

    // Delete integrated particle
    delete particle;
  }

  // Abort if necessary
  if (this->GetAbortExecute())
  {
    // delete all remaining particle
    while (!particlesQueue.empty())
    {
      vtkLagrangianParticle* particle = particlesQueue.front();
      particlesQueue.pop();
      delete particle;
    }
  }
  // Finalize outputs
  else if (!this->FinalizeOutputs(particlePathsOutput, interactionOutput))
  {
    vtkErrorMacro(<< "Cannot Finalize outputs");
    return 0;
  }
  return 1;
}

//---------------------------------------------------------------------------
bool vtkLagrangianParticleTracker::CheckParticlePathsRenderingThreshold(vtkPolyData* particlePathsOutput)
{
  return this->UseParticlePathsRenderingThreshold &&
    particlePathsOutput->GetNumberOfPoints() > this->ParticlePathsRenderingPointsThreshold;
}

//---------------------------------------------------------------------------
vtkMTimeType vtkLagrangianParticleTracker::GetMTime()
{
  // Take integrator and integration model MTime into account
  return std::max(this->Superclass::GetMTime(),
    std::max(this->IntegrationModel ? this->IntegrationModel->GetMTime() : 0,
      this->Integrator ? this->Integrator->GetMTime() : 0));
}

//---------------------------------------------------------------------------
vtkIdType vtkLagrangianParticleTracker::GetNewParticleId()
{
  vtkIdType id = this->ParticleCounter;
  this->ParticleCounter++;
  return id;
}

//---------------------------------------------------------------------------
bool vtkLagrangianParticleTracker::InitializeInputs(vtkInformationVector **inputVector,
  vtkDataObject*& flow, vtkDataObject*& seeds, vtkDataObject*& surfaces,
  std::queue<vtkLagrangianParticle*>& particlesQueue, vtkPointData* seedData)
{
  // Initialize flow
  vtkInformation* flowInInfo = inputVector[0]->GetInformationObject(0);
  flow = flowInInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkBoundingBox bounds;
  if (!this->InitializeFlow(flow, &bounds))
  {
    vtkErrorMacro(<< "Could not initialize flow, aborting.");
    return false;
  }

  // Recover seeds
  vtkInformation *seedsInInfo = inputVector[1]->GetInformationObject(0);
  seeds = vtkDataObject::SafeDownCast(seedsInInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!seeds)
  {
    vtkErrorMacro(<< "Cannot recover seeds, aborting.");
    return false;
  }

  // Configure integrator (required before particle initialization)
  this->Integrator->SetFunctionSet(this->IntegrationModel);

  // Initialize Particles
  if (!this->InitializeParticles(&bounds, seeds, particlesQueue, seedData))
  {
    vtkErrorMacro(<< "Could not initialize particles, aborting.");
    return false;
  }

  // Recover surfaces
  vtkInformation* surfacesInInfo = inputVector[2]->GetInformationObject(0);
  if (surfacesInInfo != NULL)
  {
    surfaces = surfacesInInfo->Get(vtkDataObject::DATA_OBJECT());
    this->InitializeSurface(surfaces);
  }
  return true;
}

//---------------------------------------------------------------------------
bool vtkLagrangianParticleTracker::InitializeOutputs(
  vtkInformationVector *outputVector, vtkPointData* seedData,
  vtkIdType numberOfSeeds, vtkDataObject* surfaces,
  vtkPolyData*& particlePathsOutput, vtkDataObject*& interactionOutput)
{
  if (!this->InitializePathsOutput(outputVector, seedData, numberOfSeeds, particlePathsOutput))
  {
    return false;
  }
  if (!this->InitializeInteractionOutput(outputVector, seedData, surfaces, interactionOutput))
  {
    return false;
  }
  return true;
}

//---------------------------------------------------------------------------
bool vtkLagrangianParticleTracker::InitializePathsOutput(vtkInformationVector *outputVector,
  vtkPointData* seedData, vtkIdType numberOfSeeds, vtkPolyData*& particlePathsOutput)
{
  // Prepare path output
  vtkInformation* particleOutInfo = outputVector->GetInformationObject(0);
  particlePathsOutput = vtkPolyData::SafeDownCast(particleOutInfo->Get(
    vtkPolyData::DATA_OBJECT()));
  if (!particlePathsOutput)
  {
    vtkErrorMacro(<< "Cannot find a vtkPolyData particle paths output. aborting");
    return false;
  }

  // Set information keys
  particlePathsOutput->GetInformation()->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    particleOutInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  particlePathsOutput->GetInformation()->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    particleOutInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  particlePathsOutput->GetInformation()->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    particleOutInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));

  vtkNew<vtkPoints> particlePathsPoints;
  vtkNew<vtkCellArray> particlePaths;
  vtkNew<vtkCellArray> particleVerts;
  particlePathsOutput->SetPoints(particlePathsPoints.Get());
  particlePathsOutput->SetLines(particlePaths.Get());
  particlePathsOutput->SetVerts(particleVerts.Get());

  // Prepare particle paths output point data
  vtkCellData* particlePathsCellData = particlePathsOutput->GetCellData();
  particlePathsCellData->CopyStructure(seedData);
  this->InitializePathData(particlePathsCellData);

  // Initialize Particle Paths Point Data
  vtkPointData* particlePathsPointData = particlePathsOutput->GetPointData();
  this->InitializeParticleData(particlePathsPointData, numberOfSeeds);

  // Initialize particle data from integration model, if any
  this->IntegrationModel->
    InitializeVariablesParticleData(particlePathsPointData, numberOfSeeds);
  return true;
}

//---------------------------------------------------------------------------
bool vtkLagrangianParticleTracker::InitializeInteractionOutput(
  vtkInformationVector *outputVector, vtkPointData* seedData,
  vtkDataObject* surfaces, vtkDataObject*& interactionOutput)
{
  // Prepare interaction output
  vtkInformation* particleOutInfo = outputVector->GetInformationObject(1);
  interactionOutput = particleOutInfo->Get(vtkDataObject::DATA_OBJECT());
  if (!interactionOutput)
  {
    vtkErrorMacro(<< "Cannot find a vtkDataObject particle interaction output. aborting");
    return false;
  }

  // Set information keys
  interactionOutput->GetInformation()->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    particleOutInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  interactionOutput->GetInformation()->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    particleOutInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  interactionOutput->GetInformation()->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    particleOutInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));

  // Check surfaces dataset type
  vtkCompositeDataSet *hdInput = vtkCompositeDataSet::SafeDownCast(surfaces);
  vtkDataSet* dsInput = vtkDataSet::SafeDownCast(surfaces);
  if (hdInput)
  {
    // Composite data
    vtkCompositeDataSet* hdOutput = vtkCompositeDataSet::SafeDownCast(interactionOutput);
    if (!hdOutput)
    {
      vtkErrorMacro(<< "Cannot find composite interaction output, aborting");
      return false;
    }

    hdOutput->CopyStructure(hdInput);
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(hdInput->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkNew<vtkPolyData> pd;
      vtkNew<vtkCellArray> cells;
      vtkNew<vtkPoints> points;
      pd->SetPoints(points.Get());
      pd->GetPointData()->CopyStructure(seedData);
      this->InitializePathData(pd->GetPointData());
      this->InitializeInteractionData(pd->GetPointData());
      this->InitializeParticleData(pd->GetPointData());
      this->IntegrationModel->
        InitializeVariablesParticleData(pd->GetPointData());
      hdOutput->SetDataSet(iter, pd.Get());
    }
  }
  else if (dsInput)
  {
    vtkPolyData* pd = vtkPolyData::SafeDownCast(interactionOutput);
    if (!pd)
    {
      vtkErrorMacro(<< "Cannot find polydata interaction output, aborting");
      return false;
    }

    vtkNew<vtkPoints> points;
    vtkNew<vtkCellArray> cells;
    pd->SetPoints(points.Get());
    pd->GetPointData()->CopyStructure(seedData);
    this->InitializePathData(pd->GetPointData());
    this->InitializeInteractionData(pd->GetPointData());
    this->InitializeParticleData(pd->GetPointData());
    this->IntegrationModel->
      InitializeVariablesParticleData(pd->GetPointData());
  }
  return true;
}

//---------------------------------------------------------------------------
void vtkLagrangianParticleTracker::InitializeParticleData(vtkFieldData* particleData, int maxTuple)
{
  vtkNew<vtkIntArray> particleStepNumArray;
  particleStepNumArray->SetName("StepNumber");
  particleStepNumArray->SetNumberOfComponents(1);
  particleStepNumArray->Allocate(maxTuple);
  particleData->AddArray(particleStepNumArray.Get());

  vtkNew<vtkDoubleArray> particleVelArray;
  particleVelArray->SetName("ParticleVelocity");
  particleVelArray->SetNumberOfComponents(3);
  particleVelArray->Allocate(maxTuple*3);
  particleData->AddArray(particleVelArray.Get());

  vtkNew<vtkDoubleArray> particleIntegrationTimeArray;
  particleIntegrationTimeArray->SetName("IntegrationTime");
  particleIntegrationTimeArray->SetNumberOfComponents(1);
  particleIntegrationTimeArray->Allocate(maxTuple);
  particleData->AddArray(particleIntegrationTimeArray.Get());
}

//---------------------------------------------------------------------------
void vtkLagrangianParticleTracker::InitializePathData(vtkFieldData* data)
{
  vtkNew<vtkLongLongArray> particleIdArray;
  particleIdArray->SetName("Id");
  particleIdArray->SetNumberOfComponents(1);
  data->AddArray(particleIdArray.Get());

  vtkNew<vtkLongLongArray> particleParentIdArray;
  particleParentIdArray->SetName("ParentId");
  particleParentIdArray->SetNumberOfComponents(1);
  data->AddArray(particleParentIdArray.Get());

  vtkNew<vtkLongLongArray> particleSeedIdArray;
  particleSeedIdArray->SetName("SeedId");
  particleSeedIdArray->SetNumberOfComponents(1);
  data->AddArray(particleSeedIdArray.Get());

  vtkNew<vtkIntArray> particleTerminationArray;
  particleTerminationArray->SetName("Termination");
  particleTerminationArray->SetNumberOfComponents(1);
  data->AddArray(particleTerminationArray.Get());
}

//---------------------------------------------------------------------------
void vtkLagrangianParticleTracker::InitializeInteractionData(vtkFieldData* data)
{
  vtkNew<vtkIntArray> interactionArray;
  interactionArray->SetName("Interaction");
  interactionArray->SetNumberOfComponents(1);
  data->AddArray(interactionArray.Get());
}

//---------------------------------------------------------------------------
bool vtkLagrangianParticleTracker::FinalizeOutputs(
  vtkPolyData* particlePathsOutput,
  vtkDataObject* interactionOutput)
{
  // Recover structures
  vtkPointData* particlePathsPointData = particlePathsOutput->GetPointData();
  vtkPoints* particlePathsPoints = particlePathsOutput->GetPoints();

  // Squeeze and resize point data
  for (int i = 0; i < particlePathsPointData->GetNumberOfArrays(); i++)
  {
    vtkDataArray* array = particlePathsPointData->GetArray(i);
    array->Resize(particlePathsPoints->GetNumberOfPoints());
    array->Squeeze();
  }

  // Insert interaction poly-vertex cell
  vtkCompositeDataSet *hd = vtkCompositeDataSet::SafeDownCast(interactionOutput);
  vtkPolyData* pd = vtkPolyData::SafeDownCast(interactionOutput);
  if (hd)
  {
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(hd->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkPolyData* pdBlock = vtkPolyData::SafeDownCast(hd->GetDataSet(iter));
      if (!pdBlock)
      {
        vtkErrorMacro(<< "Cannot recover interaction output, something went wrong");
        return false;
      }
      this->InsertPolyVertexCell(pdBlock);
    }
  }
  else if (pd)
  {
    this->InsertPolyVertexCell(pd);
  }

  // Enable model post processing
  this->IntegrationModel->FinalizeOutputs(particlePathsOutput, interactionOutput);

  // Optional paths rendering
  if (this->CheckParticlePathsRenderingThreshold(particlePathsOutput))
  {
    particlePathsOutput->Initialize();
  }
  return true;
}

//---------------------------------------------------------------------------
void vtkLagrangianParticleTracker::InsertPolyVertexCell(vtkPolyData* polydata)
{
  // Insert a vertex cell for each point
  int nPoint = polydata->GetNumberOfPoints();
  if (nPoint > 0)
  {
    vtkNew<vtkCellArray> polyVertex;
    polyVertex->Allocate(polyVertex->EstimateSize(1, nPoint));
    polyVertex->InsertNextCell(nPoint);
    for (int i = 0; i<nPoint; i++)
    {
      polyVertex->InsertCellPoint(i);
    }
    polydata->SetVerts(polyVertex.Get());
  }
}

//---------------------------------------------------------------------------
bool vtkLagrangianParticleTracker::InitializeFlow(vtkDataObject* input, vtkBoundingBox* bounds)
{
  // Clear previously setup flow
  this->IntegrationModel->ClearDataSets();

  // Check flow dataset type
  vtkCompositeDataSet *hdInput = vtkCompositeDataSet::SafeDownCast(input);
  vtkDataSet* dsInput = vtkDataSet::SafeDownCast(input);
  if (hdInput)
  {
    // Composite data
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(hdInput->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkDataSet *ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (ds)
      {
        // Add each leaf to the integration model
        this->IntegrationModel->AddDataSet(ds);
        ds->ComputeBounds();
        bounds->AddBounds(ds->GetBounds());
      }
    }
    return true;
  }
  else if (dsInput)
  {
    // Add dataset to integration model
    this->IntegrationModel->AddDataSet(dsInput);
    dsInput->ComputeBounds();
    bounds->AddBounds(dsInput->GetBounds());
    return true;
  }
  else
  {
    vtkErrorMacro(<< "This filter cannot handle input of type: " <<
      (input ? input->GetClassName() : "(none)"));
    return false;
  }
}

//---------------------------------------------------------------------------
void vtkLagrangianParticleTracker::InitializeSurface(vtkDataObject*& surfaces)
{
  // Clear previously setup surfaces
  this->IntegrationModel->ClearDataSets(true);

  // Check surfaces dataset type
  vtkCompositeDataSet *hdInput = vtkCompositeDataSet::SafeDownCast(surfaces);
  vtkDataSet* dsInput = vtkDataSet::SafeDownCast(surfaces);

  if (hdInput)
  {
    // Composite data
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(hdInput->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (ds)
      {
        vtkPolyData* pd = vtkPolyData::SafeDownCast(iter->GetCurrentDataObject());
        vtkNew<vtkDataSetSurfaceFilter> surfaceFilter;
        if (pd == NULL)
        {
          surfaceFilter->SetInputData(ds);
          surfaceFilter->Update();
          pd = surfaceFilter->GetOutput();
        }

        // Add each leaf to the integration model surfaces
        // Compute normals if non-present
        vtkNew<vtkPolyDataNormals> normals;
        if (!pd->GetCellData()->GetNormals())
        {
          normals->ComputePointNormalsOff();
          normals->ComputeCellNormalsOn();
          normals->SetInputData(pd);
          normals->Update();
          pd = normals->GetOutput();
        }
        if (pd->GetNumberOfCells() > 0)
        {
          this->IntegrationModel->AddDataSet(pd, true, iter->GetCurrentFlatIndex());
        }
      }
    }
  }
  else if (dsInput)
  {
    vtkPolyData* pd = vtkPolyData::SafeDownCast(dsInput);
    vtkNew<vtkDataSetSurfaceFilter> surfaceFilter;
    if (pd == NULL)
    {
      surfaceFilter->SetInputData(dsInput);
      surfaceFilter->Update();
      pd = surfaceFilter->GetOutput();
    }

    // Add surface to integration model
    // Compute normals if non-present
    vtkNew<vtkPolyDataNormals> normals;
    if (!pd->GetCellData()->GetNormals())
    {
      normals->ComputePointNormalsOff();
      normals->ComputeCellNormalsOn();
      normals->SetInputData(pd);
      normals->Update();
      pd = normals->GetOutput();
    }
    if (pd->GetNumberOfCells() > 0)
    {
      this->IntegrationModel->AddDataSet(pd, true);
    }
  }
}

//---------------------------------------------------------------------------
bool vtkLagrangianParticleTracker::InitializeParticles(
  const vtkBoundingBox* bounds, vtkDataObject* seeds,
  std::queue<vtkLagrangianParticle*>& particles, vtkPointData* seedData)
{
  // Sanity check
  if (seeds == NULL)
  {
    vtkErrorMacro(<< "Cannot generate Particles without seeds");
    return false;
  }

  // Check seed dataset type
  vtkCompositeDataSet *hdInput = vtkCompositeDataSet::SafeDownCast(seeds);
  vtkDataSet* actualSeeds = vtkDataSet::SafeDownCast(seeds);
  if (hdInput)
  {
    // Composite data
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(hdInput->NewIterator());
    bool leafFound = false;
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkDataSet *ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (ds)
      {
        // We show the warning only when the input contains more than one leaf
        if (leafFound)
        {
          vtkWarningMacro("Only the first block of seeds have been used to "
            "generate seeds, other blocks are ignored");
          break;
        }
        actualSeeds = ds;
        leafFound = true;
      }
    }
  }

  if (!actualSeeds)
  {
    vtkErrorMacro(<< "This filter cannot handle input of type: " <<
      (seeds ? seeds->GetClassName() : "(none)"));
    return false;
  }

  // Recover data
  int nVar = this->IntegrationModel->GetNumberOfIndependentVariables();
  seedData->DeepCopy(actualSeeds->GetPointData());

  vtkDataArray* initialVelocities = NULL;
  vtkDataArray* initialIntegrationTimes = NULL;

  if (actualSeeds->GetNumberOfPoints() > 0)
  {
    // Recover initial velocities, index 0
    initialVelocities = vtkDataArray::SafeDownCast(
      this->IntegrationModel->GetSeedArray(0, actualSeeds->GetPointData()));
    if (initialVelocities == NULL)
    {
      vtkErrorMacro(<< "initialVelocity is not set in particle data, "
        "unable to initialize particles!");
      return false;
    }

    // Recover initial integration time if any, index 1
    if (this->IntegrationModel->GetUseInitialIntegrationTime())
    {
      initialIntegrationTimes = vtkDataArray::SafeDownCast(
        this->IntegrationModel->GetSeedArray(1, actualSeeds->GetPointData()));
      if (initialVelocities == NULL)
      {
        vtkWarningMacro("initialIntegrationTimes is not set in particle data, "
          "initial integration time set to zero!");
      }
    }
  }

  // Create one particle for each point
  this->GenerateParticles(bounds, actualSeeds, initialVelocities,
    initialIntegrationTimes, seedData, nVar, particles);
  return true;
}

//---------------------------------------------------------------------------
void vtkLagrangianParticleTracker::GenerateParticles(
  const vtkBoundingBox* vtkNotUsed(bounds), vtkDataSet* seeds,
  vtkDataArray* initialVelocities, vtkDataArray* initialIntegrationTimes,
  vtkPointData* seedData, int nVar, std::queue<vtkLagrangianParticle*>& particles)
{
  this->ParticleCounter = 0;
  for (vtkIdType i = 0; i < seeds->GetNumberOfPoints(); i++)
  {
    double position[3];
    seeds->GetPoint(i, position);
    double initialIntegrationTime = initialIntegrationTimes ?
      initialIntegrationTimes->GetTuple1(i) : 0;
    vtkIdType particleId = this->GetNewParticleId();
    vtkLagrangianParticle* particle = new vtkLagrangianParticle(nVar, particleId,
      particleId, i, initialIntegrationTime, seedData);
    memcpy(particle->GetPosition(), position, 3 * sizeof(double));
    initialVelocities->GetTuple(i, particle->GetVelocity());
    this->IntegrationModel->InitializeParticle(particle);
    if (this->IntegrationModel->FindInLocators(particle->GetPosition()))
    {
      particles.push(particle);
    }
    else
    {
      delete particle;
    }
  }
}

//---------------------------------------------------------------------------
void vtkLagrangianParticleTracker::GetParticleFeed(
  std::queue<vtkLagrangianParticle*>& vtkNotUsed(particleQueue))
{
}

//---------------------------------------------------------------------------
int vtkLagrangianParticleTracker::Integrate(vtkLagrangianParticle* particle,
  std::queue<vtkLagrangianParticle*>& particlesQueue,
  vtkPolyData* particlePathsOutput, vtkIdList* particlePathPointId,
  vtkDataObject* interactionOutput)
{
  // Sanity check
  if (particle == NULL)
  {
    vtkErrorMacro(<< "Cannot integrate NULL particle");
    return -1;
  }

  // Set the current particle
  this->IntegrationModel->SetCurrentParticle(particle);

  // Integrate until MaximumNumberOfSteps is reached or special case stops integration
  int integrationRes = 0;
  double stepFactor = this->StepFactor;
  double reintegrationFactor = 1;
  double& stepTimeActual = particle->GetStepTimeRef();
  while (particle->GetNumberOfSteps() < this->MaximumNumberOfSteps)
  {
    // Update progress
    if (particle->GetNumberOfSteps() % 100 == 0 && this->ParticleCounter > 0)
    {
      double progress = static_cast<double>(particle->GetId() +
        static_cast<double>(particle->GetNumberOfSteps()) / this->MaximumNumberOfSteps) /
        this->ParticleCounter;
      this->UpdateProgress(progress);
      if (this->GetAbortExecute())
      {
        break;
      }
    }

    // Compute step
    double velocityMagnitude = reintegrationFactor * std::max(
      this->MinimumVelocityMagnitude,
      vtkMath::Norm(particle->GetVelocity()));
    double cellLength = this->ComputeCellLength(particle);

    double stepLength    = stepFactor          * cellLength;
    double stepLengthMin = this->StepFactorMin * cellLength;
    double stepLengthMax = this->StepFactorMax * cellLength;
    double stepTime    = stepLength    / velocityMagnitude;
    double stepTimeMin = stepLengthMin / velocityMagnitude;
    double stepTimeMax = stepLengthMax / velocityMagnitude;

    // Integrate one step
    if (!this->ComputeNextStep(particle->GetEquationVariables(),
      particle->GetNextEquationVariables(), particle->GetIntegrationTime(),
      stepTime, stepTimeActual, stepTimeMin, stepTimeMax, integrationRes))
    {
      vtkErrorMacro(<< "Integration Error");
      break;
    }

    bool stagnating =
      std::abs(particle->GetPosition()[0] - particle->GetNextPosition()[0]) <
      std::numeric_limits<double>::epsilon() &&
      std::abs(particle->GetPosition()[1] - particle->GetNextPosition()[1]) <
      std::numeric_limits<double>::epsilon() &&
      std::abs(particle->GetPosition()[2] - particle->GetNextPosition()[2]) <
      std::numeric_limits<double>::epsilon();

    // Only stagnating OUT_OF_DOMAIN are actually out of domain
    bool outOfDomain = integrationRes ==
      vtkInitialValueProblemSolver::OUT_OF_DOMAIN && stagnating;

    // Simpler Adaptive Step Reintegration code
    if (this->AdaptiveStepReintegration)
    {
      double stepLengthCurr2 = vtkMath::Distance2BetweenPoints(
        particle->GetPosition(), particle->GetNextPosition());
      double stepLengthMax2 = stepLengthMax * stepLengthMax;
      if (stepLengthCurr2 > stepLengthMax2)
      {
        reintegrationFactor *= 2;
        continue;
      }
      reintegrationFactor = 1;
    }

    if (outOfDomain)
    {
      // Stop integration
      particle->SetTermination(vtkLagrangianParticle::PARTICLE_TERMINATION_OUT_OF_DOMAIN);
      break;
    }

    // We care only about non-stagnating particle
    if (!stagnating)
    {
      // Surface interaction
      vtkLagrangianBasicIntegrationModel::PassThroughParticlesType passThroughParticles;
      unsigned int interactedSurfaceFlaxIndex;
      vtkLagrangianParticle* interactionParticle =
        this->IntegrationModel->ComputeSurfaceInteraction(
        particle, particlesQueue, interactedSurfaceFlaxIndex, passThroughParticles);
      if (interactionParticle != NULL)
      {
        this->InsertInteractionOutputPoint(interactionParticle,
          interactedSurfaceFlaxIndex, interactionOutput);
        delete interactionParticle;
        interactionParticle = NULL;
      }

      // Insert pass through interaction points
      // Note: when going out of domain right after going some pass through
      // surfaces, the pass through interaction point will not be
      // on a particle track, since we do not want to show out of domain particle
      // track. The pass through interaction still has occurred and it is not a bug.
      while (!passThroughParticles.empty())
      {
        vtkLagrangianBasicIntegrationModel::PassThroughParticlesItem item =
          passThroughParticles.front();
        passThroughParticles.pop();
        this->InsertInteractionOutputPoint(item.second, item.first, interactionOutput);

        // the pass through particles needs to be deleted
        delete item.second;
      }

      // Particle has been correctly integrated and interacted, record it
      // Insert Current particle as an output point
      this->InsertPathOutputPoint(particle, particlePathsOutput, particlePathPointId);

      // Particle has been terminated by surface
      if (particle->GetTermination() !=
        vtkLagrangianParticle::PARTICLE_TERMINATION_NOT_TERMINATED)
      {
        // Insert last particle path point on surface
        particle->MoveToNextPosition();
        this->InsertPathOutputPoint(particle, particlePathsOutput, particlePathPointId);

        // stop integration
        break;
      }
    }

    if (this->IntegrationModel->CheckFreeFlightTermination(particle))
    {
      particle->SetTermination(
        vtkLagrangianParticle::PARTICLE_TERMINATION_FLIGHT_TERMINATED);
      break;
    }

    // Keep integrating
    particle->MoveToNextPosition();

    // Compute now adaptive step
    if (this->Integrator->IsAdaptive() || this->AdaptiveStepReintegration)
    {
      stepFactor = stepTime * velocityMagnitude / cellLength;
    }
  }
  if (particle->GetNumberOfSteps() == this->MaximumNumberOfSteps &&
    particle->GetTermination() ==
    vtkLagrangianParticle::PARTICLE_TERMINATION_NOT_TERMINATED)
  {
    particle->SetTermination(
      vtkLagrangianParticle::PARTICLE_TERMINATION_OUT_OF_STEPS);
  }
  this->IntegrationModel->SetCurrentParticle(NULL);
  return integrationRes;
}

//---------------------------------------------------------------------------
void vtkLagrangianParticleTracker::InsertPathOutputPoint(
  vtkLagrangianParticle* particle, vtkPolyData* particlePathsOutput,
  vtkIdList* particlePathPointId, bool prev)
{
  // Recover structures
  vtkPoints* particlePathsPoints = particlePathsOutput->GetPoints();
  vtkPointData* particlePathsPointData = particlePathsOutput->GetPointData();

  // Store previous position
  vtkIdType pointId = particlePathsPoints->InsertNextPoint(
    prev ? particle->GetPrevPosition() : particle->GetPosition());

  particlePathPointId->InsertNextId(pointId);

  // Insert particle data
  this->InsertParticleData(particle, particlePathsPointData,
    prev ? vtkLagrangianBasicIntegrationModel::VARIABLE_STEP_PREV :
    vtkLagrangianBasicIntegrationModel::VARIABLE_STEP_CURRENT);

  // Add Variables data
  this->IntegrationModel->InsertVariablesParticleData(particle,
    particlePathsPointData, prev ?
    vtkLagrangianBasicIntegrationModel::VARIABLE_STEP_PREV :
    vtkLagrangianBasicIntegrationModel::VARIABLE_STEP_CURRENT);
}

//---------------------------------------------------------------------------
void vtkLagrangianParticleTracker::InsertInteractionOutputPoint(
  vtkLagrangianParticle* particle, unsigned int interactedSurfaceFlatIndex,
  vtkDataObject* interactionOutput)
{
  // Find the correct output
  vtkCompositeDataSet *hdOutput = vtkCompositeDataSet::SafeDownCast(interactionOutput);
  vtkPolyData* pdOutput = vtkPolyData::SafeDownCast(interactionOutput);
  vtkPolyData* interactionPd = NULL;
  if (hdOutput)
  {
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(hdOutput->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      if (interactedSurfaceFlatIndex == iter->GetCurrentFlatIndex())
      {
        interactionPd = vtkPolyData::SafeDownCast(hdOutput->GetDataSet(iter));
        break;
      }
    }
  }
  else if (pdOutput)
  {
    interactionPd = pdOutput;
  }

  if (!interactionPd)
  {
    vtkErrorMacro(<< "Something went wrong with interaction output, "
      "cannot find correct interaction output polydata");
    return;
  }

  // "Next" Point
  vtkPoints* points = interactionPd->GetPoints();
  points->InsertNextPoint(particle->GetNextPosition());

  // Fill up interaction point data
  vtkPointData* pointData = interactionPd->GetPointData();
  this->InsertPathData(particle, pointData);
  this->InsertInteractionData(particle, pointData);
  this->InsertParticleData(particle, pointData,
    vtkLagrangianBasicIntegrationModel::VARIABLE_STEP_NEXT);

  // Add Variables data
  this->IntegrationModel->InsertVariablesParticleData(particle, pointData,
    vtkLagrangianBasicIntegrationModel::VARIABLE_STEP_NEXT);

  // Finally, Insert data from seed data only on not yet written arrays
  this->InsertSeedData(particle, pointData);
}

//---------------------------------------------------------------------------
void vtkLagrangianParticleTracker::InsertSeedData(vtkLagrangianParticle* particle,
  vtkFieldData* data)
{
  // Check for max number of tuples in arrays
  vtkIdType maxTuples = 0;
  for (int i = 0; i < data->GetNumberOfArrays(); i++)
  {
    maxTuples = std::max(data->GetArray(i)->GetNumberOfTuples(), maxTuples);
  }

  // Copy seed data in not yet written array only
  // ie not yet at maxTuple
  vtkPointData* seedData = particle->GetSeedData();
  for (int i = 0; i < seedData->GetNumberOfArrays(); i++)
  {
    const char* name = seedData->GetArrayName(i);
    vtkDataArray* arr = data->GetArray(name);
    if (arr->GetNumberOfTuples() < maxTuples)
    {
      arr->InsertNextTuple(
        seedData->GetArray(i)->GetTuple(particle->GetSeedArrayTupleIndex()));
    }
  }
  // here all arrays from data should have the exact same size
}

//---------------------------------------------------------------------------
void vtkLagrangianParticleTracker::InsertPathData(vtkLagrangianParticle* particle,
  vtkFieldData* data)
{
  vtkLongLongArray::SafeDownCast(
    data->GetArray("Id"))->InsertNextValue(particle->GetId());
  vtkLongLongArray::SafeDownCast(
    data->GetArray("ParentId"))
    ->InsertNextValue(particle->GetParentId());
  vtkLongLongArray::SafeDownCast(
    data->GetArray("SeedId"))->InsertNextValue(particle->GetSeedId());
  vtkIntArray::SafeDownCast(
    data->GetArray("Termination"))->InsertNextValue(particle->GetTermination());
}

//---------------------------------------------------------------------------
void vtkLagrangianParticleTracker::InsertInteractionData(
  vtkLagrangianParticle* particle, vtkFieldData* data)
{
  vtkIntArray::SafeDownCast(
    data->GetArray("Interaction"))->InsertNextValue(particle->GetInteraction());
}

//---------------------------------------------------------------------------
void vtkLagrangianParticleTracker::InsertParticleData(vtkLagrangianParticle* particle,
  vtkFieldData* data, int stepEnum)
{
  switch (stepEnum)
  {
    case vtkLagrangianBasicIntegrationModel::VARIABLE_STEP_PREV:
      vtkIntArray::SafeDownCast(
        data->GetArray("StepNumber"))->InsertNextValue(particle->GetNumberOfSteps() - 1);
      data->GetArray("ParticleVelocity")->InsertNextTuple(particle->GetPrevVelocity());
      data->GetArray("IntegrationTime")->InsertNextTuple1(particle->GetPrevIntegrationTime());
      break;
    case vtkLagrangianBasicIntegrationModel::VARIABLE_STEP_CURRENT:
      vtkIntArray::SafeDownCast(
        data->GetArray("StepNumber"))->InsertNextValue(particle->GetNumberOfSteps());
      data->GetArray("ParticleVelocity")->InsertNextTuple(particle->GetVelocity());
      data->GetArray("IntegrationTime")->InsertNextTuple1(particle->GetIntegrationTime());
      break;
    case vtkLagrangianBasicIntegrationModel::VARIABLE_STEP_NEXT:
      vtkIntArray::SafeDownCast(
        data->GetArray("StepNumber"))->InsertNextValue(particle->GetNumberOfSteps() + 1);
      data->GetArray("ParticleVelocity")->InsertNextTuple(particle->GetNextVelocity());
      data->GetArray("IntegrationTime")->InsertNextTuple1(particle->GetIntegrationTime() +
        particle->GetStepTimeRef());
      break;
    default:
      break;
  }
}

//---------------------------------------------------------------------------
double vtkLagrangianParticleTracker::ComputeCellLength(
  vtkLagrangianParticle* particle)
{
  double cellLength = 1.0;
  vtkDataSet* dataset = NULL;
  vtkCell* cell = NULL;
  bool forceLastCell = false;
  if (this->CellLengthComputationMode == STEP_CUR_CELL_LENGTH ||
    this->CellLengthComputationMode == STEP_CUR_CELL_VEL_DIR ||
    this->CellLengthComputationMode == STEP_CUR_CELL_DIV_THEO)
  {
    vtkIdType cellId;
    if (this->IntegrationModel->FindInLocators(particle->GetPosition(), dataset, cellId))
    {
      cell = dataset->GetCell(cellId);
    }
    else
    {
      forceLastCell = true;
    }
  }
  if (this->CellLengthComputationMode == STEP_LAST_CELL_LENGTH ||
    this->CellLengthComputationMode == STEP_LAST_CELL_VEL_DIR ||
    this->CellLengthComputationMode == STEP_LAST_CELL_DIV_THEO ||
    forceLastCell)
  {
    dataset = particle->GetLastDataSet();
    if (!dataset)
    {
      return cellLength;
    }
    cell = dataset->GetCell(particle->GetLastCellId());
    if (!cell)
    {
      return cellLength;
    }
  }
  if (cell == NULL)
  {
    vtkWarningMacro("Unsupported Cell Length Computation Mode"
      " or could not find a cell to compute cell length with");
    return 1.0;
  }

  double* vel = particle->GetVelocity();
  if ((this->CellLengthComputationMode == STEP_CUR_CELL_VEL_DIR ||
    this->CellLengthComputationMode == STEP_LAST_CELL_VEL_DIR) &&
      vtkMath::Norm(vel) > 0.0)
  {
    double velHat[3] = { vel[0], vel[1], vel[2] };
    vtkMath::Normalize(velHat);
    double tmpCellLength = 0.0;
    for (int ne = 0; ne < cell->GetNumberOfEdges(); ++ne)
    {
      double evect[3], x0[3], x1[3];
      vtkCell* edge = cell->GetEdge(ne);
      vtkIdType e0 = edge->GetPointId(0);
      vtkIdType e1 = edge->GetPointId(1);
      dataset->GetPoint(e0, x0);
      dataset->GetPoint(e1, x1);
      vtkMath::Subtract(x0, x1, evect);
      double elength = std::fabs(vtkMath::Dot(evect, velHat));
      tmpCellLength = std::max(tmpCellLength, elength);
    }
    cellLength = tmpCellLength;
  }
  else if ((this->CellLengthComputationMode == STEP_CUR_CELL_DIV_THEO ||
    this->CellLengthComputationMode == STEP_LAST_CELL_DIV_THEO) &&
      vtkMath::Norm(vel) > 0.0 && !vtkVoxel::SafeDownCast(cell))
  {
    double velHat[3] = {vel[0], vel[1], vel[2]};
    vtkMath::Normalize(velHat);
    double xa = 0.0;  // cell cross-sectional area in velHat direction
    double vol = 0.0; // cell volume
    for (int nf = 0; nf < cell->GetNumberOfFaces(); ++nf)
    {
      double norm[3];  // cell face normal
      double centroid[3] = {0.0, 0.0, 0.0}; // cell face centroid
      vtkCell* face = cell->GetFace(nf);
      vtkPoints* pts = face->GetPoints();
      vtkIdType nPoints = pts->GetNumberOfPoints();
      const double area = vtkPolygon::ComputeArea(pts, nPoints, NULL, norm);
      const double fact = 1.0 / static_cast<double>(nPoints);
      for (int np = 0; np < nPoints; ++np)
      {
        double* x = pts->GetPoint(np);
        for (int nc = 0; nc < 3; ++nc)
        {
          centroid[nc] += x[nc] * fact;
        }
      }
      xa += std::fabs(vtkMath::Dot(norm, velHat) * area) / 2.0;   // sum unsigned areas
      vol += vtkMath::Dot(norm, centroid) * area / 3.0;           // using divergence theorem
    }
    // characteristic length is cell volume / cell cross-sectional area in velocity direction
    // Absolute value of volume because of some Fluent cases where all the volumes seem negative
    cellLength = std::fabs(vol) / xa;
  }
  else
  {
    cellLength = std::sqrt(cell->GetLength2());
  }
  return cellLength;
}

//---------------------------------------------------------------------------
bool vtkLagrangianParticleTracker::ComputeNextStep(
  double* xprev, double* xnext,
  double t, double& delT, double& delTActual,
  double minStep, double maxStep,
  int& integrationRes)
{
  // Check for potential manual integration
  double error;
  if (!this->IntegrationModel->ManualIntegration(xprev, xnext, t, delT, delTActual,
    minStep, maxStep, this->IntegrationModel->GetTolerance(), error, integrationRes))
  {
    // integrate one step
    integrationRes =
      this->Integrator->ComputeNextStep(xprev, xnext, t, delT, delTActual,
        minStep, maxStep, this->IntegrationModel->GetTolerance(), error);
  }

  // Check failure cases
  if (integrationRes == vtkInitialValueProblemSolver::NOT_INITIALIZED)
  {
    vtkErrorMacro(<< "Integrator is not initialized. Aborting.");
    return false;
  }
  if (integrationRes == vtkInitialValueProblemSolver::UNEXPECTED_VALUE)
  {
    vtkErrorMacro(<< "Integrator encountered an unexpected value. Dropping particle.");
    return false;
  }
  return true;
}
