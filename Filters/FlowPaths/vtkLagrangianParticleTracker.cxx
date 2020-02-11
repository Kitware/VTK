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

#include "vtkAppendPolyData.h"
#include "vtkBilinearQuadIntersection.h"
#include "vtkBoundingBox.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataSet.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLagrangianMatidaIntegrationModel.h"
#include "vtkLagrangianParticle.h"
#include "vtkLagrangianThreadedData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataNormals.h"
#include "vtkPolyLine.h"
#include "vtkPolygon.h"
#include "vtkRungeKutta2.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <algorithm>
#include <limits>
#include <sstream>

vtkObjectFactoryNewMacro(vtkLagrangianParticleTracker);
vtkCxxSetObjectMacro(
  vtkLagrangianParticleTracker, IntegrationModel, vtkLagrangianBasicIntegrationModel);
vtkCxxSetObjectMacro(vtkLagrangianParticleTracker, Integrator, vtkInitialValueProblemSolver);

struct IntegratingFunctor
{
  vtkLagrangianParticleTracker* Tracker;
  std::vector<vtkLagrangianParticle*>& ParticlesVec;
  std::queue<vtkLagrangianParticle*>& ParticlesQueue;
  vtkPolyData* ParticlePathsOutput;
  vtkDataObject* Surfaces;
  vtkDataObject* InteractionOutput;
  vtkSMPThreadLocal<vtkLagrangianThreadedData*> LocalData;
  bool Serial = false;

  IntegratingFunctor(vtkLagrangianParticleTracker* tracker,
    std::vector<vtkLagrangianParticle*>& particlesVec,
    std::queue<vtkLagrangianParticle*>& particlesQueue, vtkPolyData* particlePathsOutput,
    vtkDataObject* surfaces, vtkDataObject* interactionOutput, bool serial)
    : Tracker(tracker)
    , ParticlesVec(particlesVec)
    , ParticlesQueue(particlesQueue)
    , ParticlePathsOutput(particlePathsOutput)
    , Surfaces(surfaces)
    , InteractionOutput(interactionOutput)
    , Serial(serial)
  {
  }

  void Initialize()
  {
    // Create a local threaded data
    vtkLagrangianThreadedData* localData = new vtkLagrangianThreadedData;
    this->LocalData.Local() = localData;

    // Create a local non-threadsafe integrator with a threadsafe integration model
    localData->Integrator = this->Tracker->Integrator->NewInstance();
    localData->Integrator->SetFunctionSet(this->Tracker->IntegrationModel);

    // Initialize a local idList
    localData->IdList->Allocate(10);

    // Create a local bilinear quad intersection
    localData->BilinearQuadIntersection = new vtkBilinearQuadIntersection;

    if (this->Tracker->GenerateParticlePathsOutput)
    {
      vtkPolyData* localParticlePathsOutput = localData->ParticlePathsOutput;
      // Initialize a local particle path output
      this->Tracker->InitializePathsOutput(this->Tracker->SeedData,
        static_cast<vtkIdType>(this->LocalData.size()), localParticlePathsOutput);
    }

    if (this->Surfaces)
    {
      // Create and initialize a local interaction output
      localData->InteractionOutput = this->InteractionOutput->NewInstance();
      this->Tracker->InitializeInteractionOutput(
        this->Tracker->SeedData, this->Surfaces, localData->InteractionOutput);
    }

    // Let the model initialize the user data if needed
    this->Tracker->IntegrationModel->InitializeThreadedData(localData);
  }

  void operator()(vtkIdType partId, vtkIdType endPartId)
  {
    for (vtkIdType id = partId; id < endPartId; id++)
    {
      vtkLagrangianParticle* particle = this->ParticlesVec[id];
      vtkLagrangianThreadedData* localData = this->LocalData.Local();

      // Set threaded data on the particle
      particle->SetThreadedData(localData);

      // Create polyLine output cell
      vtkNew<vtkPolyLine> particlePath;

      // Integrate
      this->Tracker->Integrate(localData->Integrator, particle, this->ParticlesQueue,
        localData->ParticlePathsOutput, particlePath, localData->InteractionOutput);

      this->Tracker->IntegratedParticleCounter += this->Tracker->IntegratedParticleCounterIncrement;

      this->Tracker->IntegrationModel->ParticleAboutToBeDeleted(particle);
      delete particle;

      // Special case to show progress in serial
      if (this->Serial)
      {
        double progress = static_cast<double>(this->Tracker->IntegratedParticleCounter) /
          this->Tracker->ParticleCounter;
        this->Tracker->UpdateProgress(progress);
      }
    }
    if (!this->Serial)
    {
      // In multithread, protect the progress event with a mutex
      std::lock_guard<std::mutex> guard(this->Tracker->ProgressMutex);
      double progress = static_cast<double>(this->Tracker->IntegratedParticleCounter) /
        this->Tracker->ParticleCounter;
      this->Tracker->UpdateProgress(progress);
    }
  }

  void Reduce()
  {
    // Particle Path reduction
    if (this->Tracker->GenerateParticlePathsOutput)
    {
      vtkNew<vtkAppendPolyData> append;
      append->AddInputData(this->ParticlePathsOutput);
      for (auto data : this->LocalData)
      {
        append->AddInputData(data->ParticlePathsOutput);
      }
      append->Update();
      this->ParticlePathsOutput->ShallowCopy(append->GetOutput());
    }

    if (this->Surfaces)
    {
      // Interaction Reduction
      vtkCompositeDataSet* hdInteractionOutput =
        vtkCompositeDataSet::SafeDownCast(this->InteractionOutput);
      vtkPolyData* pdInteractionOutput = vtkPolyData::SafeDownCast(this->InteractionOutput);
      if (hdInteractionOutput)
      {
        vtkCompositeDataSet* hdSurfaces = vtkCompositeDataSet::SafeDownCast(this->Surfaces);
        vtkSmartPointer<vtkCompositeDataIterator> iter;
        iter.TakeReference(hdSurfaces->NewIterator());
        for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
        {
          vtkNew<vtkAppendPolyData> append;
          vtkPolyData* initialPD = vtkPolyData::SafeDownCast(hdInteractionOutput->GetDataSet(iter));
          if (initialPD)
          {
            append->AddInputData(initialPD);
          }
          for (auto data : this->LocalData)
          {
            append->AddInputData(vtkPolyData::SafeDownCast(
              vtkCompositeDataSet::SafeDownCast(data->InteractionOutput)->GetDataSet(iter)));
          }
          append->Update();
          hdInteractionOutput->SetDataSet(iter, append->GetOutput());
        }
        for (auto data : this->LocalData)
        {
          data->InteractionOutput->Delete();
        }
      }
      else
      {
        vtkNew<vtkAppendPolyData> append;
        append->AddInputData(pdInteractionOutput);
        for (auto data : this->LocalData)
        {
          auto interOut = data->InteractionOutput;
          vtkPolyData* pd = vtkPolyData::SafeDownCast(interOut);
          append->AddInputData(pd);
          interOut->Delete();
        }
        append->Update();
        pdInteractionOutput->ShallowCopy(append->GetOutput());
      }
    }

    // Other threaded Data Reduction
    for (auto data : this->LocalData)
    {
      data->Integrator->Delete();
      delete data->BilinearQuadIntersection;
      this->Tracker->IntegrationModel->FinalizeThreadedData(data);
      delete data;
    }
  }
};

//---------------------------------------------------------------------------
vtkLagrangianParticleTracker::vtkLagrangianParticleTracker()
  : IntegrationModel(vtkLagrangianMatidaIntegrationModel::New())
  , Integrator(vtkRungeKutta2::New())
  , CellLengthComputationMode(STEP_LAST_CELL_LENGTH)
  , StepFactor(1.0)
  , StepFactorMin(0.5)
  , StepFactorMax(1.5)
  , MaximumNumberOfSteps(100)
  , MaximumIntegrationTime(-1.0)
  , AdaptiveStepReintegration(false)
  , GeneratePolyVertexInteractionOutput(false)
  , ParticleCounter(0)
  , IntegratedParticleCounter(0)
  , IntegratedParticleCounterIncrement(1)
  , MinimumVelocityMagnitude(0.001)
  , MinimumReductionFactor(1.1)
  , FlowCache(nullptr)
  , FlowTime(0)
  , SurfacesCache(nullptr)
  , SurfacesTime(0)
{
  this->SetNumberOfInputPorts(3);
  this->SetNumberOfOutputPorts(2);
}

//---------------------------------------------------------------------------
vtkLagrangianParticleTracker::~vtkLagrangianParticleTracker()
{
  this->SetIntegrator(nullptr);
  this->SetIntegrationModel(nullptr);
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
  os << indent << "CellLengthComputationMode: " << this->CellLengthComputationMode << endl;
  os << indent << "StepFactor: " << this->StepFactor << endl;
  os << indent << "StepFactorMin: " << this->StepFactorMin << endl;
  os << indent << "StepFactorMax: " << this->StepFactorMax << endl;
  os << indent << "MaximumNumberOfSteps: " << this->MaximumNumberOfSteps << endl;
  os << indent << "MaximumIntegrationTime: " << this->MaximumIntegrationTime << endl;
  os << indent << "AdaptiveStepReintegration: " << this->AdaptiveStepReintegration << endl;
  os << indent << "GenerateParticlePathsOutput: " << this->GenerateParticlePathsOutput << endl;
  os << indent << "MinimumVelocityMagnitude: " << this->MinimumVelocityMagnitude << endl;
  os << indent << "MinimumReductionFactor: " << this->MinimumReductionFactor << endl;
  os << indent << "ParticleCounter: " << this->ParticleCounter << endl;
  os << indent << "IntegratedParticleCounter: " << this->IntegratedParticleCounter << endl;
}

//---------------------------------------------------------------------------
void vtkLagrangianParticleTracker::SetSourceConnection(vtkAlgorithmOutput* algInput)
{
  this->SetInputConnection(1, algInput);
}

//---------------------------------------------------------------------------
void vtkLagrangianParticleTracker::SetSourceData(vtkDataObject* source)
{
  this->SetInputData(1, source);
}

//---------------------------------------------------------------------------
vtkDataObject* vtkLagrangianParticleTracker::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }
  return vtkDataObject::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//---------------------------------------------------------------------------
void vtkLagrangianParticleTracker::SetSurfaceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(2, algOutput);
}

//---------------------------------------------------------------------------
void vtkLagrangianParticleTracker::SetSurfaceData(vtkDataObject* surface)
{
  this->SetInputData(2, surface);
}

//---------------------------------------------------------------------------
vtkDataObject* vtkLagrangianParticleTracker::GetSurface()
{
  if (this->GetNumberOfInputConnections(2) < 1)
  {
    return nullptr;
  }
  return this->GetExecutive()->GetInputData(2, 0);
}

//---------------------------------------------------------------------------
int vtkLagrangianParticleTracker::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 2)
  {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return this->Superclass::FillInputPortInformation(port, info);
}

//----------------------------------------------------------------------------
int vtkLagrangianParticleTracker::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  }
  return this->Superclass::FillOutputPortInformation(port, info);
}

//----------------------------------------------------------------------------
int vtkLagrangianParticleTracker::RequestDataObject(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Create particle path output
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkNew<vtkPolyData> particlePathsOutput;
  info->Set(vtkDataObject::DATA_OBJECT(), particlePathsOutput);

  // Create a surface interaction output
  // First check for composite
  vtkInformation* inInfo = inputVector[2]->GetInformationObject(0);
  info = outputVector->GetInformationObject(1);
  if (inInfo)
  {
    vtkDataObject* input = vtkDataObject::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    if (input)
    {
      vtkCompositeDataSet* hdInput = vtkCompositeDataSet::SafeDownCast(input);
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
  info->Set(vtkDataObject::DATA_OBJECT(), interactionOutput);
  return 1;
}

//---------------------------------------------------------------------------
int vtkLagrangianParticleTracker::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  std::queue<vtkLagrangianParticle*> particlesQueue;

  if (!this->IntegrationModel)
  {
    vtkErrorMacro(<< "Integration Model is nullptr, cannot integrate");
    return 0;
  }
  this->IntegrationModel->SetTracker(this);

  // Initialize flow
  vtkDataObject* flow = vtkDataObject::GetData(inputVector[0]);
  vtkBoundingBox bounds;
  if (!this->InitializeFlow(flow, &bounds))
  {
    vtkErrorMacro(<< "Could not initialize flow, aborting.");
    return false;
  }

  // Initialize surfaces
  vtkInformation* surfacesInInfo = inputVector[2]->GetInformationObject(0);
  vtkDataObject* surfaces = nullptr;
  if (surfacesInInfo)
  {
    surfaces = surfacesInInfo->Get(vtkDataObject::DATA_OBJECT());
    if (this->UpdateSurfaceCacheIfNeeded(surfaces))
    {
      this->InitializeSurface(surfaces);
    }
  }

  // Recover seeds
  vtkDataObject* seeds = vtkDataObject::GetData(inputVector[1]);
  if (!seeds)
  {
    vtkErrorMacro(<< "Cannot recover seeds, aborting.");
    return 0;
  }

  // Check seed dataset type
  vtkCompositeDataSet* hdInput = vtkCompositeDataSet::SafeDownCast(seeds);
  vtkDataSet* actualSeeds = vtkDataSet::SafeDownCast(seeds);
  if (hdInput)
  {
    // Composite data
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(hdInput->NewIterator());
    bool leafFound = false;
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
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
    vtkErrorMacro(<< "This filter cannot handle input of type: "
                  << (seeds ? seeds->GetClassName() : "(none)"));
    return 0;
  }
  this->SeedData = actualSeeds->GetPointData();

  // Initialize Particles from the seeds
  if (!this->InitializeParticles(&bounds, actualSeeds, particlesQueue, this->SeedData))
  {
    vtkErrorMacro(<< "Could not initialize particles, aborting.");
    return false;
  }

  // Initialize outputs
  vtkPolyData* particlePathsOutput = nullptr;
  if (this->GenerateParticlePathsOutput)
  {
    particlePathsOutput = vtkPolyData::GetData(outputVector);
    if (!particlePathsOutput)
    {
      vtkErrorMacro(<< "Cannot find a vtkMultiPiece particle paths output. aborting");
      return 0;
    }
    this->InitializePathsOutput(this->SeedData, 0, particlePathsOutput);
  }

  vtkDataObject* interactionOutput = nullptr;
  if (surfaces)
  {
    vtkInformation* interactionOutInfo = outputVector->GetInformationObject(1);
    interactionOutput = interactionOutInfo->Get(vtkPolyData::DATA_OBJECT());
    if (!interactionOutput)
    {
      vtkErrorMacro(<< "Cannot find a vtkMultiBlock interaction output. aborting");
      return 0;
    }
    vtkCompositeDataSet* hdInteractionOutput = vtkCompositeDataSet::SafeDownCast(interactionOutput);
    if (hdInteractionOutput)
    {
      hdInteractionOutput->CopyStructure(vtkCompositeDataSet::SafeDownCast(surfaces));
    }
    this->InitializeInteractionOutput(this->SeedData, surfaces, interactionOutput);
  }

  // Let model a chance to change the particles or compute things
  // before integration.
  this->IntegrationModel->PreIntegrate(particlesQueue);

  std::vector<vtkLagrangianParticle*> particlesVec;
  while (!this->GetAbortExecute())
  {
    // Check for particle feed
    this->GetParticleFeed(particlesQueue);
    if (particlesQueue.empty())
    {
      break;
    }

    // Move the current particle queue into a SMP usable vector
    particlesVec.clear();
    particlesVec.resize(particlesQueue.size());
    for (auto& particlePtr : particlesVec)
    {
      // Recover particle
      vtkLagrangianParticle* particleTmp = particlesQueue.front();
      particlesQueue.pop();
      particlePtr = particleTmp;
    }

    // Integrate all available particles
    IntegratingFunctor functor(this, particlesVec, particlesQueue, particlePathsOutput, surfaces,
      interactionOutput, vtkSMPTools::GetEstimatedNumberOfThreads() == 1);
    vtkSMPTools::For(0, static_cast<vtkIdType>(particlesVec.size()), functor);
  }

  // Abort if necessary
  if (this->GetAbortExecute())
  {
    // delete all remaining particle
    while (!particlesQueue.empty())
    {
      vtkLagrangianParticle* particle = particlesQueue.front();
      particlesQueue.pop();
      this->IntegrationModel->ParticleAboutToBeDeleted(particle);
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
  return this->ParticleCounter++;
}

//---------------------------------------------------------------------------
bool vtkLagrangianParticleTracker::InitializePathsOutput(
  vtkPointData* seedData, vtkIdType numberOfSeeds, vtkPolyData*& particlePathsOutput)
{

  vtkNew<vtkPoints> particlePathsPoints;
  vtkNew<vtkCellArray> particlePaths;
  vtkNew<vtkCellArray> particleVerts;
  particlePathsOutput->SetPoints(particlePathsPoints);
  particlePathsOutput->SetLines(particlePaths);
  particlePathsOutput->SetVerts(particleVerts);

  // Prepare particle paths output point data
  vtkCellData* particlePathsCellData = particlePathsOutput->GetCellData();
  particlePathsCellData->CopyStructure(seedData);
  this->IntegrationModel->InitializePathData(particlePathsCellData);

  // Initialize Particle Paths Point Data
  vtkPointData* particlePathsPointData = particlePathsOutput->GetPointData();
  this->IntegrationModel->InitializeParticleData(particlePathsPointData, numberOfSeeds);

  return true;
}

//---------------------------------------------------------------------------
bool vtkLagrangianParticleTracker::InitializeInteractionOutput(
  vtkPointData* seedData, vtkDataObject* surfaces, vtkDataObject*& interactionOutput)
{
  // Check surfaces dataset type
  vtkCompositeDataSet* hdInput = vtkCompositeDataSet::SafeDownCast(surfaces);
  vtkDataSet* dsInput = vtkDataSet::SafeDownCast(surfaces);
  if (hdInput)
  {
    vtkCompositeDataSet* hdInteractionOutput = vtkCompositeDataSet::SafeDownCast(interactionOutput);
    hdInteractionOutput->CopyStructure(hdInput);
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(hdInput->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkNew<vtkPolyData> pd;
      vtkNew<vtkCellArray> cells;
      vtkNew<vtkPoints> points;
      pd->SetPoints(points);
      pd->GetPointData()->CopyStructure(seedData);
      this->IntegrationModel->InitializePathData(pd->GetPointData());
      this->IntegrationModel->InitializeInteractionData(pd->GetPointData());
      this->IntegrationModel->InitializeParticleData(pd->GetPointData());
      hdInteractionOutput->SetDataSet(iter, pd);
    }
  }
  else if (dsInput)
  {
    vtkPolyData* pdInteractionOutput = vtkPolyData::SafeDownCast(interactionOutput);
    vtkNew<vtkPoints> points;
    vtkNew<vtkCellArray> cells;
    pdInteractionOutput->SetPoints(points);
    pdInteractionOutput->GetPointData()->CopyStructure(seedData);
    this->IntegrationModel->InitializePathData(pdInteractionOutput->GetPointData());
    this->IntegrationModel->InitializeInteractionData(pdInteractionOutput->GetPointData());
    this->IntegrationModel->InitializeParticleData(pdInteractionOutput->GetPointData());
  }
  return true;
}

//---------------------------------------------------------------------------
bool vtkLagrangianParticleTracker::FinalizeOutputs(
  vtkPolyData* particlePathsOutput, vtkDataObject* interactionOutput)
{
  if (this->GenerateParticlePathsOutput)
  {
    if (!particlePathsOutput)
    {
      vtkErrorMacro("Could not recover a output path polydata, something went wrong");
      return false;
    }

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
  }

  // Insert interaction poly-vertex cell
  if (interactionOutput)
  {
    vtkCompositeDataSet* hdInteractionOutput = vtkCompositeDataSet::SafeDownCast(interactionOutput);
    vtkPolyData* pdInteractionOutput = vtkPolyData::SafeDownCast(interactionOutput);
    if (hdInteractionOutput)
    {
      vtkNew<vtkDataObjectTreeIterator> iter;
      iter->SetDataSet(hdInteractionOutput);
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        vtkPolyData* pdBlock = vtkPolyData::SafeDownCast(hdInteractionOutput->GetDataSet(iter));
        if (!pdBlock)
        {
          vtkErrorMacro(<< "Cannot recover interaction output, something went wrong");
          return false;
        }
        if (this->GeneratePolyVertexInteractionOutput)
        {
          this->InsertPolyVertexCell(pdBlock);
        }
        else
        {
          this->InsertVertexCells(pdBlock);
        }
      }
    }
    else
    {
      if (this->GeneratePolyVertexInteractionOutput)
      {
        this->InsertPolyVertexCell(pdInteractionOutput);
      }
      else
      {
        this->InsertVertexCells(pdInteractionOutput);
      }
    }
  }

  // Enable model post processing
  this->IntegrationModel->FinalizeOutputs(particlePathsOutput, interactionOutput);
  return true;
}

//---------------------------------------------------------------------------
void vtkLagrangianParticleTracker::InsertPolyVertexCell(vtkPolyData* polydata)
{
  // Insert a vertex cell for each point
  vtkIdType nPoint = polydata->GetNumberOfPoints();
  if (nPoint > 0)
  {
    vtkNew<vtkCellArray> polyVertex;
    polyVertex->AllocateEstimate(1, nPoint);
    polyVertex->InsertNextCell(nPoint);
    for (vtkIdType i = 0; i < nPoint; i++)
    {
      polyVertex->InsertCellPoint(i);
    }
    polydata->SetVerts(polyVertex);
  }
}

//---------------------------------------------------------------------------
void vtkLagrangianParticleTracker::InsertVertexCells(vtkPolyData* polydata)
{
  // Insert a vertex cell for each point
  vtkIdType nPoint = polydata->GetNumberOfPoints();
  if (nPoint > 0)
  {
    vtkNew<vtkCellArray> polyVertex;
    polyVertex->AllocateEstimate(1, nPoint);
    for (vtkIdType i = 0; i < nPoint; i++)
    {
      polyVertex->InsertNextCell(1);
      polyVertex->InsertCellPoint(i);
    }
    polydata->SetVerts(polyVertex);
  }
}

//---------------------------------------------------------------------------
bool vtkLagrangianParticleTracker::InitializeFlow(vtkDataObject* input, vtkBoundingBox* bounds)
{
  // Check for updated cache
  if (input == this->FlowCache && input->GetMTime() <= this->FlowTime &&
    this->IntegrationModel->GetLocatorsBuilt())
  {
    bounds->Reset();
    bounds->AddBox(this->FlowBoundsCache);
    return true;
  }

  // No Cache, do the initialization
  // Clear previously setup flow
  this->IntegrationModel->ClearDataSets();

  // Check flow dataset type
  vtkCompositeDataSet* hdInput = vtkCompositeDataSet::SafeDownCast(input);
  vtkDataSet* dsInput = vtkDataSet::SafeDownCast(input);
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
        // Add each leaf to the integration model
        this->IntegrationModel->AddDataSet(ds);
        ds->ComputeBounds();
        bounds->AddBounds(ds->GetBounds());
      }
    }
  }
  else if (dsInput)
  {
    // Add dataset to integration model
    this->IntegrationModel->AddDataSet(dsInput);
    dsInput->ComputeBounds();
    bounds->AddBounds(dsInput->GetBounds());
  }
  else
  {
    vtkErrorMacro(<< "This filter cannot handle input of type: "
                  << (input ? input->GetClassName() : "(none)"));
    return false;
  }
  this->IntegrationModel->SetLocatorsBuilt(true);
  this->FlowCache = input;
  this->FlowTime = input->GetMTime();
  this->FlowBoundsCache.Reset();
  this->FlowBoundsCache.AddBox(*bounds);
  return true;
}

//---------------------------------------------------------------------------
bool vtkLagrangianParticleTracker::UpdateSurfaceCacheIfNeeded(vtkDataObject*& surfaces)
{
  if (surfaces != this->SurfacesCache || surfaces->GetMTime() > this->SurfacesTime)
  {
    this->SurfacesCache = surfaces;
    this->SurfacesTime = surfaces->GetMTime();
    return true;
  }
  return false;
}

//---------------------------------------------------------------------------
void vtkLagrangianParticleTracker::InitializeSurface(vtkDataObject*& surfaces)
{
  // Clear previously setup surfaces
  this->IntegrationModel->ClearDataSets(/*surface*/ true);

  // Check surfaces dataset type
  vtkCompositeDataSet* hdInput = vtkCompositeDataSet::SafeDownCast(surfaces);
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
        if (!pd)
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
          this->IntegrationModel->AddDataSet(pd, /*surface*/ true, iter->GetCurrentFlatIndex());
        }
      }
    }
  }
  else if (dsInput)
  {
    vtkPolyData* pd = vtkPolyData::SafeDownCast(dsInput);
    vtkNew<vtkDataSetSurfaceFilter> surfaceFilter;
    if (!pd)
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
      this->IntegrationModel->AddDataSet(pd, /*surface*/ true);
    }
  }
}

//---------------------------------------------------------------------------
bool vtkLagrangianParticleTracker::InitializeParticles(const vtkBoundingBox* bounds,
  vtkDataSet* seeds, std::queue<vtkLagrangianParticle*>& particles, vtkPointData* seedData)
{
  // Sanity check
  if (!seeds)
  {
    vtkErrorMacro(<< "Cannot generate Particles without seeds");
    return false;
  }

  // Recover data
  vtkDataArray* initialVelocities = nullptr;
  vtkDataArray* initialIntegrationTimes = nullptr;
  if (seeds->GetNumberOfPoints() > 0)
  {
    // Recover initial velocities, index 0
    initialVelocities =
      vtkDataArray::SafeDownCast(this->IntegrationModel->GetSeedArray(0, seedData));
    if (!initialVelocities)
    {
      vtkErrorMacro(<< "initialVelocity is not set in particle data, "
                       "unable to initialize particles!");
      return false;
    }

    // Recover initial integration time if any, index 1
    if (this->IntegrationModel->GetUseInitialIntegrationTime())
    {
      initialIntegrationTimes =
        vtkDataArray::SafeDownCast(this->IntegrationModel->GetSeedArray(1, seedData));
      if (!initialVelocities)
      {
        vtkWarningMacro("initialIntegrationTimes is not set in particle data, "
                        "initial integration time set to zero!");
      }
    }
  }

  // Create one particle for each point
  int nVar = this->IntegrationModel->GetNumberOfIndependentVariables();
  this->GenerateParticles(
    bounds, seeds, initialVelocities, initialIntegrationTimes, seedData, nVar, particles);
  return true;
}

//---------------------------------------------------------------------------
void vtkLagrangianParticleTracker::GenerateParticles(const vtkBoundingBox* vtkNotUsed(bounds),
  vtkDataSet* seeds, vtkDataArray* initialVelocities, vtkDataArray* initialIntegrationTimes,
  vtkPointData* seedData, int nVar, std::queue<vtkLagrangianParticle*>& particles)
{
  // Create and set a dummy particle so FindInLocators can use caching.
  std::unique_ptr<vtkLagrangianThreadedData> dummyData(new vtkLagrangianThreadedData);
  vtkLagrangianParticle dummyParticle(
    0, 0, 0, 0, 0, nullptr, this->IntegrationModel->GetWeightsSize(), 0);
  dummyParticle.SetThreadedData(dummyData.get());

  this->ParticleCounter = 0;
  this->IntegratedParticleCounter = 0;

  for (vtkIdType i = 0; i < seeds->GetNumberOfPoints(); i++)
  {
    double position[3];
    seeds->GetPoint(i, position);
    double initialIntegrationTime =
      initialIntegrationTimes ? initialIntegrationTimes->GetTuple1(i) : 0;
    vtkIdType particleId = this->GetNewParticleId();
    vtkLagrangianParticle* particle = new vtkLagrangianParticle(nVar, particleId, particleId, i,
      initialIntegrationTime, seedData, this->IntegrationModel->GetWeightsSize(),
      this->IntegrationModel->GetNumberOfTrackedUserData());
    memcpy(particle->GetPosition(), position, 3 * sizeof(double));
    initialVelocities->GetTuple(i, particle->GetVelocity());
    this->IntegrationModel->InitializeParticle(particle);
    if (this->IntegrationModel->FindInLocators(particle->GetPosition(), &dummyParticle))
    {
      particles.push(particle);
    }
    else
    {
      this->IntegrationModel->ParticleAboutToBeDeleted(particle);
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
int vtkLagrangianParticleTracker::Integrate(vtkInitialValueProblemSolver* integrator,
  vtkLagrangianParticle* particle, std::queue<vtkLagrangianParticle*>& particlesQueue,
  vtkPolyData* particlePathsOutput, vtkPolyLine* particlePath, vtkDataObject* interactionOutput)
{
  // Sanity check
  if (!particle)
  {
    vtkErrorMacro(<< "Cannot integrate nullptr particle");
    return -1;
  }

  // Integrate until MaximumNumberOfSteps or MaximumIntegrationTime is reached or special case stops
  int integrationRes = 0;
  double stepFactor = this->StepFactor;
  double reintegrationFactor = 1;
  double& stepTimeActual = particle->GetStepTimeRef();
  while (particle->GetTermination() == vtkLagrangianParticle::PARTICLE_TERMINATION_NOT_TERMINATED)
  {
    // Compute step
    double velocityMagnitude = reintegrationFactor *
      std::max(this->MinimumVelocityMagnitude, vtkMath::Norm(particle->GetVelocity()));
    double cellLength = this->ComputeCellLength(particle);

    double stepLength = stepFactor * cellLength;
    double stepLengthMin = this->StepFactorMin * cellLength;
    double stepLengthMax = this->StepFactorMax * cellLength;
    double stepTime = stepLength / (reintegrationFactor * velocityMagnitude);
    double stepTimeMin = stepLengthMin / (reintegrationFactor * velocityMagnitude);
    double stepTimeMax = stepLengthMax / (reintegrationFactor * velocityMagnitude);

    // Integrate one step
    if (!this->ComputeNextStep(integrator, particle->GetEquationVariables(),
          particle->GetNextEquationVariables(), particle->GetIntegrationTime(), stepTime,
          stepTimeActual, stepTimeMin, stepTimeMax, cellLength, integrationRes, particle))
    {
      vtkErrorMacro(<< "Integration Error");
      break;
    }

    bool stagnating = std::abs(particle->GetPosition()[0] - particle->GetNextPosition()[0]) <
        std::numeric_limits<double>::epsilon() &&
      std::abs(particle->GetPosition()[1] - particle->GetNextPosition()[1]) <
        std::numeric_limits<double>::epsilon() &&
      std::abs(particle->GetPosition()[2] - particle->GetNextPosition()[2]) <
        std::numeric_limits<double>::epsilon();

    // Only stagnating OUT_OF_DOMAIN are actually out of domain
    bool outOfDomain = integrationRes == vtkInitialValueProblemSolver::OUT_OF_DOMAIN && stagnating;

    // Simpler Adaptive Step Reintegration code
    if (this->AdaptiveStepReintegration &&
      this->IntegrationModel->CheckAdaptiveStepReintegration(particle))
    {
      double stepLengthCurr2 =
        vtkMath::Distance2BetweenPoints(particle->GetPosition(), particle->GetNextPosition());
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
      if (interactionParticle)
      {
        this->InsertInteractionOutputPoint(
          interactionParticle, interactedSurfaceFlaxIndex, interactionOutput);
        this->IntegrationModel->ParticleAboutToBeDeleted(interactionParticle);
        delete interactionParticle;
        interactionParticle = nullptr;
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
        this->IntegrationModel->ParticleAboutToBeDeleted(item.second);
        delete item.second;
      }

      // Particle has been correctly integrated and interacted, record it
      // Insert Current particle as an output point

      if (this->GenerateParticlePathsOutput)
      {
        this->InsertPathOutputPoint(particle, particlePathsOutput, particlePath->GetPointIds());
      }

      // Particle has been terminated by surface
      if (particle->GetTermination() != vtkLagrangianParticle::PARTICLE_TERMINATION_NOT_TERMINATED)
      {
        // Insert last particle path point on surface
        particle->MoveToNextPosition();

        if (this->GenerateParticlePathsOutput)
        {
          this->InsertPathOutputPoint(particle, particlePathsOutput, particlePath->GetPointIds());
        }

        // stop integration
        break;
      }
    }

    if (this->IntegrationModel->CheckFreeFlightTermination(particle))
    {
      particle->SetTermination(vtkLagrangianParticle::PARTICLE_TERMINATION_FLIGHT_TERMINATED);
      break;
    }

    // Keep integrating
    particle->MoveToNextPosition();

    // Compute now adaptive step
    if (integrator->IsAdaptive() || this->AdaptiveStepReintegration)
    {
      stepFactor = stepTime * reintegrationFactor * velocityMagnitude / cellLength;
    }
    if (this->MaximumNumberOfSteps > -1 &&
      particle->GetNumberOfSteps() == this->MaximumNumberOfSteps &&
      particle->GetTermination() == vtkLagrangianParticle::PARTICLE_TERMINATION_NOT_TERMINATED)
    {
      particle->SetTermination(vtkLagrangianParticle::PARTICLE_TERMINATION_OUT_OF_STEPS);
    }
    if (this->MaximumIntegrationTime >= 0.0 &&
      particle->GetIntegrationTime() >= this->MaximumIntegrationTime &&
      particle->GetTermination() == vtkLagrangianParticle::PARTICLE_TERMINATION_NOT_TERMINATED)
    {
      particle->SetTermination(vtkLagrangianParticle::PARTICLE_TERMINATION_OUT_OF_TIME);
    }
  }

  if (this->GenerateParticlePathsOutput)
  {
    if (particlePath->GetPointIds()->GetNumberOfIds() == 1)
    {
      particlePath->GetPointIds()->InsertNextId(particlePath->GetPointId(0));
    }

    // Duplicate single point particle paths, to avoid degenerated lines.
    if (particlePath->GetPointIds()->GetNumberOfIds() > 0)
    {

      // Add particle path or vertex to cell array
      particlePathsOutput->GetLines()->InsertNextCell(particlePath);
      this->IntegrationModel->InsertPathData(particle, particlePathsOutput->GetCellData());

      // Insert data from seed data only in not yet written arrays
      this->IntegrationModel->InsertParticleSeedData(particle, particlePathsOutput->GetCellData());
    }
  }

  return integrationRes;
}

//---------------------------------------------------------------------------
void vtkLagrangianParticleTracker::InsertPathOutputPoint(vtkLagrangianParticle* particle,
  vtkPolyData* particlePathsOutput, vtkIdList* particlePathPointId, bool prev)
{
  // Recover structures
  vtkPoints* particlePathsPoints = particlePathsOutput->GetPoints();
  vtkPointData* particlePathsPointData = particlePathsOutput->GetPointData();

  // Store previous position
  vtkIdType pointId = particlePathsPoints->InsertNextPoint(
    prev ? particle->GetPrevPosition() : particle->GetPosition());

  particlePathPointId->InsertNextId(pointId);

  // Insert particle data
  this->IntegrationModel->InsertParticleData(particle, particlePathsPointData,
    prev ? vtkLagrangianBasicIntegrationModel::VARIABLE_STEP_PREV
         : vtkLagrangianBasicIntegrationModel::VARIABLE_STEP_CURRENT);
}

//---------------------------------------------------------------------------
void vtkLagrangianParticleTracker::InsertInteractionOutputPoint(vtkLagrangianParticle* particle,
  unsigned int interactedSurfaceFlatIndex, vtkDataObject* interactionOutput)
{
  // Find the correct output
  vtkCompositeDataSet* hdOutput = vtkCompositeDataSet::SafeDownCast(interactionOutput);
  vtkPolyData* pdOutput = vtkPolyData::SafeDownCast(interactionOutput);
  vtkPolyData* interactionPd = nullptr;
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
  this->IntegrationModel->InsertPathData(particle, pointData);
  this->IntegrationModel->InsertInteractionData(particle, pointData);
  this->IntegrationModel->InsertParticleData(
    particle, pointData, vtkLagrangianBasicIntegrationModel::VARIABLE_STEP_NEXT);

  // Finally, Insert data from seed data only on not yet written arrays
  this->IntegrationModel->InsertParticleSeedData(particle, pointData);
}

//---------------------------------------------------------------------------
double vtkLagrangianParticleTracker::ComputeCellLength(vtkLagrangianParticle* particle)
{
  double cellLength = 1.0;
  vtkDataSet* dataset = nullptr;
  vtkGenericCell* cell = particle->GetThreadedData()->GenericCell;
  if (!cell)
  {
    vtkErrorMacro("Could not recover a generic cell for cell length computation");
    return 1.0;
  }
  bool forceLastCell = false;
  if (this->CellLengthComputationMode == STEP_CUR_CELL_LENGTH ||
    this->CellLengthComputationMode == STEP_CUR_CELL_VEL_DIR ||
    this->CellLengthComputationMode == STEP_CUR_CELL_DIV_THEO)
  {
    vtkIdType cellId;
    vtkAbstractCellLocator* loc;
    double* weights = particle->GetLastWeights();
    if (this->IntegrationModel->FindInLocators(
          particle->GetPosition(), particle, dataset, cellId, loc, weights))
    {
      dataset->GetCell(cellId, cell);
    }
    else
    {
      forceLastCell = true;
    }
  }
  if (this->CellLengthComputationMode == STEP_LAST_CELL_LENGTH ||
    this->CellLengthComputationMode == STEP_LAST_CELL_VEL_DIR ||
    this->CellLengthComputationMode == STEP_LAST_CELL_DIV_THEO || forceLastCell)
  {
    dataset = particle->GetLastDataSet();
    if (!dataset)
    {
      return cellLength;
    }
    dataset->GetCell(particle->GetLastCellId(), cell);
    if (!cell)
    {
      return cellLength;
    }
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
    vtkMath::Norm(vel) > 0.0)
  {
    double velHat[3] = { vel[0], vel[1], vel[2] };
    vtkMath::Normalize(velHat);
    double xa = 0.0;  // cell cross-sectional area in velHat direction
    double vol = 0.0; // cell volume
    for (int nf = 0; nf < cell->GetNumberOfFaces(); ++nf)
    {
      double norm[3];                         // cell face normal
      double centroid[3] = { 0.0, 0.0, 0.0 }; // cell face centroid
      vtkCell* face = cell->GetFace(nf);
      vtkPoints* pts = face->GetPoints();
      vtkIdType nPoints = pts->GetNumberOfPoints();
      const double area = vtkPolygon::ComputeArea(pts, nPoints, nullptr, norm);
      const double fact = 1.0 / static_cast<double>(nPoints);
      for (int np = 0; np < nPoints; ++np)
      {
        double* x = pts->GetPoint(np);
        for (int nc = 0; nc < 3; ++nc)
        {
          centroid[nc] += x[nc] * fact;
        }
      }
      xa += std::fabs(vtkMath::Dot(norm, velHat) * area) / 2.0; // sum unsigned areas
      vol += vtkMath::Dot(norm, centroid) * area / 3.0;         // using divergence theorem
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
bool vtkLagrangianParticleTracker::ComputeNextStep(vtkInitialValueProblemSolver* integrator,
  double* xprev, double* xnext, double t, double& delT, double& delTActual, double minStep,
  double maxStep, double cellLength, int& integrationRes, vtkLagrangianParticle* particle)
{
  // Check for potential manual integration
  double error;
  if (!this->IntegrationModel->ManualIntegration(integrator, xprev, xnext, t, delT, delTActual,
        minStep, maxStep, this->IntegrationModel->GetTolerance(), cellLength, error, integrationRes,
        particle))
  {
    // integrate one step
    integrationRes = integrator->ComputeNextStep(xprev, xnext, t, delT, delTActual, minStep,
      maxStep, this->IntegrationModel->GetTolerance(), error, particle);
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
