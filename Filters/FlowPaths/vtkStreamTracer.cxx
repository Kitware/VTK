/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkStreamTracer.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStreamTracer.h"

#include "vtkAMRInterpolatedVelocityField.h"
#include "vtkAbstractInterpolatedVelocityField.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellLocatorStrategy.h"
#include "vtkClosestPointStrategy.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeInterpolatedVelocityField.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPolyData.h"
#include "vtkPolyLine.h"
#include "vtkRungeKutta2.h"
#include "vtkRungeKutta4.h"
#include "vtkRungeKutta45.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"

#include <vector>

vtkObjectFactoryNewMacro(vtkStreamTracer);
vtkCxxSetObjectMacro(vtkStreamTracer, Integrator, vtkInitialValueProblemSolver);
vtkCxxSetObjectMacro(vtkStreamTracer, InterpolatorPrototype, vtkAbstractInterpolatedVelocityField);

// Initial value for streamline terminal speed
const double vtkStreamTracer::EPSILON = 1.0E-12;

//------------------------------------------------------------------------------
vtkStreamTracer::vtkStreamTracer()
{
  this->Integrator = vtkRungeKutta2::New();
  this->IntegrationDirection = FORWARD;
  for (int i = 0; i < 3; i++)
  {
    this->StartPosition[i] = 0.0;
  }

  this->MaximumPropagation = 1.0;
  this->IntegrationStepUnit = CELL_LENGTH_UNIT;
  this->InitialIntegrationStep = 0.5;
  this->MinimumIntegrationStep = 1.0E-2;
  this->MaximumIntegrationStep = 1.0;

  this->MaximumError = 1.0e-6;
  this->MaximumNumberOfSteps = 2000;
  this->TerminalSpeed = EPSILON;

  this->ComputeVorticity = true;
  this->RotationScale = 1.0;

  this->LastUsedStepSize = 0.0;

  this->GenerateNormalsInIntegrate = true;

  this->InterpolatorPrototype = nullptr;

  this->SetNumberOfInputPorts(2);

  // by default process active point vectors
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::VECTORS);

  this->HasMatchingPointAttributes = true;

  this->SurfaceStreamlines = false;

  this->ForceSerialExecution = false;
  this->SerialExecution = false;

  this->UseLocalSeedSource = true;
}

//------------------------------------------------------------------------------
vtkStreamTracer::~vtkStreamTracer()
{
  this->SetIntegrator(nullptr);
  this->SetInterpolatorPrototype(nullptr);
}

//------------------------------------------------------------------------------
void vtkStreamTracer::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//------------------------------------------------------------------------------
void vtkStreamTracer::SetSourceData(vtkDataSet* source)
{
  this->SetInputData(1, source);
}

//------------------------------------------------------------------------------
vtkDataSet* vtkStreamTracer::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }
  return vtkDataSet::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//------------------------------------------------------------------------------
int vtkStreamTracer::GetIntegratorType()
{
  if (!this->Integrator)
  {
    return NONE;
  }
  if (!strcmp(this->Integrator->GetClassName(), "vtkRungeKutta2"))
  {
    return RUNGE_KUTTA2;
  }
  if (!strcmp(this->Integrator->GetClassName(), "vtkRungeKutta4"))
  {
    return RUNGE_KUTTA4;
  }
  if (!strcmp(this->Integrator->GetClassName(), "vtkRungeKutta45"))
  {
    return RUNGE_KUTTA45;
  }
  return UNKNOWN;
}

//------------------------------------------------------------------------------
void vtkStreamTracer::SetInterpolatorTypeToDataSetPointLocator()
{
  this->SetInterpolatorType(static_cast<int>(INTERPOLATOR_WITH_DATASET_POINT_LOCATOR));
}

//------------------------------------------------------------------------------
void vtkStreamTracer::SetInterpolatorTypeToCellLocator()
{
  this->SetInterpolatorType(static_cast<int>(INTERPOLATOR_WITH_CELL_LOCATOR));
}

//------------------------------------------------------------------------------
void vtkStreamTracer::SetInterpolatorType(int interpType)
{
  vtkNew<vtkCompositeInterpolatedVelocityField> cIVF;
  if (interpType == INTERPOLATOR_WITH_CELL_LOCATOR)
  {
    // create an interpolator equipped with a cell locator
    vtkNew<vtkCellLocatorStrategy> strategy;
    cIVF->SetFindCellStrategy(strategy);
  }
  else
  {
    // create an interpolator equipped with a point locator (by default)
    vtkNew<vtkClosestPointStrategy> strategy;
    cIVF->SetFindCellStrategy(strategy);
  }
  this->SetInterpolatorPrototype(cIVF);
}

//------------------------------------------------------------------------------
void vtkStreamTracer::SetIntegratorType(int type)
{
  vtkInitialValueProblemSolver* ivp = nullptr;
  switch (type)
  {
    case RUNGE_KUTTA2:
      ivp = vtkRungeKutta2::New();
      break;
    case RUNGE_KUTTA4:
      ivp = vtkRungeKutta4::New();
      break;
    case RUNGE_KUTTA45:
      ivp = vtkRungeKutta45::New();
      break;
    default:
      vtkWarningMacro("Unrecognized integrator type. Keeping old one.");
      break;
  }
  if (ivp)
  {
    this->SetIntegrator(ivp);
    ivp->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkStreamTracer::SetIntegrationStepUnit(int unit)
{
  if (unit != LENGTH_UNIT && unit != CELL_LENGTH_UNIT)
  {
    unit = CELL_LENGTH_UNIT;
  }

  if (unit == this->IntegrationStepUnit)
  {
    return;
  }

  this->IntegrationStepUnit = unit;
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkIntervalInformation::ConvertToLength(double interval, int unit, double cellLength)
{
  double retVal = 0.0;
  if (unit == vtkStreamTracer::LENGTH_UNIT)
  {
    retVal = interval;
  }
  else if (unit == vtkStreamTracer::CELL_LENGTH_UNIT)
  {
    retVal = interval * cellLength;
  }
  return retVal;
}

//------------------------------------------------------------------------------
double vtkIntervalInformation::ConvertToLength(vtkIntervalInformation& interval, double cellLength)
{
  return ConvertToLength(interval.Interval, interval.Unit, cellLength);
}

//------------------------------------------------------------------------------
void vtkStreamTracer::ConvertIntervals(
  double& step, double& minStep, double& maxStep, int direction, double cellLength)
{
  minStep = maxStep = step = direction *
    vtkIntervalInformation::ConvertToLength(
      this->InitialIntegrationStep, this->IntegrationStepUnit, cellLength);

  if (this->MinimumIntegrationStep > 0.0)
  {
    minStep = vtkIntervalInformation::ConvertToLength(
      this->MinimumIntegrationStep, this->IntegrationStepUnit, cellLength);
  }

  if (this->MaximumIntegrationStep > 0.0)
  {
    maxStep = vtkIntervalInformation::ConvertToLength(
      this->MaximumIntegrationStep, this->IntegrationStepUnit, cellLength);
  }
}

//------------------------------------------------------------------------------
void vtkStreamTracer::CalculateVorticity(
  vtkGenericCell* cell, double pcoords[3], vtkDoubleArray* cellVectors, double vorticity[3])
{
  double* cellVel;
  double derivs[9];

  cellVel = cellVectors->GetPointer(0);
  cell->Derivatives(0, pcoords, cellVel, 3, derivs);
  vorticity[0] = derivs[7] - derivs[5];
  vorticity[1] = derivs[2] - derivs[6];
  vorticity[2] = derivs[3] - derivs[1];
}

//------------------------------------------------------------------------------
void vtkStreamTracer::InitializeSeeds(vtkDataArray*& seeds, vtkIdList*& seedIds,
  vtkIntArray*& integrationDirections, vtkDataSet* source)
{
  seedIds = vtkIdList::New();
  integrationDirections = vtkIntArray::New();
  seeds = nullptr;

  if (source)
  {
    vtkIdType numSeeds = source->GetNumberOfPoints();
    if (numSeeds > 0)
    {
      // For now, one thread will do all

      if (this->IntegrationDirection == BOTH)
      {
        seedIds->SetNumberOfIds(2 * numSeeds);
        for (vtkIdType i = 0; i < numSeeds; ++i)
        {
          seedIds->SetId(i, i);
          seedIds->SetId(numSeeds + i, i);
        }
      }
      else
      {
        seedIds->SetNumberOfIds(numSeeds);
        for (vtkIdType i = 0; i < numSeeds; ++i)
        {
          seedIds->SetId(i, i);
        }
      }
      // Check if the source is a PointSet
      vtkPointSet* seedPts = vtkPointSet::SafeDownCast(source);
      if (seedPts)
      {
        // If it is, use it's points as source
        vtkDataArray* orgSeeds = seedPts->GetPoints()->GetData();
        seeds = orgSeeds->NewInstance();
        seeds->DeepCopy(orgSeeds);
      }
      else
      {
        // Else, create a seed source
        seeds = vtkDoubleArray::New();
        seeds->SetNumberOfComponents(3);
        seeds->SetNumberOfTuples(numSeeds);
        for (vtkIdType i = 0; i < numSeeds; ++i)
        {
          seeds->SetTuple(i, source->GetPoint(i));
        }
      }
    }
  } // if a source is available

  else // source not defined, use the start position
  {
    seeds = vtkDoubleArray::New();
    seeds->SetNumberOfComponents(3);
    seeds->InsertNextTuple(this->StartPosition);
    seedIds->InsertNextId(0);
    if (this->IntegrationDirection == BOTH)
    {
      seedIds->InsertNextId(0);
    }
  }

  if (seeds)
  {
    vtkIdType i;
    vtkIdType numSeeds = seeds->GetNumberOfTuples();
    if (this->IntegrationDirection == BOTH)
    {
      for (i = 0; i < numSeeds; i++)
      {
        integrationDirections->InsertNextValue(FORWARD);
      }
      for (i = 0; i < numSeeds; i++)
      {
        integrationDirections->InsertNextValue(BACKWARD);
      }
    }
    else
    {
      for (i = 0; i < numSeeds; i++)
      {
        integrationDirections->InsertNextValue(this->IntegrationDirection);
      }
    }
  } // if seeds are available
}

//------------------------------------------------------------------------------
int vtkStreamTracer::SetupOutput(vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!inInfo || !outInfo)
  {
    vtkErrorMacro("Input/Output information is not set, aborting.");
    return 0;
  }

  int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());

  // Pass through field data
  output->GetFieldData()->PassData(input->GetFieldData());

  vtkCompositeDataSet* hdInput = vtkCompositeDataSet::SafeDownCast(input);
  vtkDataSet* dsInput = vtkDataSet::SafeDownCast(input);
  if (hdInput)
  {
    this->InputData = hdInput;
    hdInput->Register(this);
  }
  else if (dsInput)
  {
    vtkNew<vtkMultiBlockDataSet> mb;
    mb->SetNumberOfBlocks(numPieces);
    mb->SetBlock(piece, dsInput);
    this->InputData = mb;
    mb->Register(this);
  }
  else
  {
    vtkErrorMacro(
      "This filter cannot handle input of type: " << (input ? input->GetClassName() : "(none)"));
    return 0;
  }

  // Okay, now we need to create representative output dataset attributes. This
  // will be used for interpolating point data across all datasets contained
  // in the composite dataset. Also determine if the dataset point attributes
  // match across all leaf datasets.
  this->InputPD.Reset();
  auto datasets = vtkCompositeDataSet::GetDataSets(this->InputData);
  for (size_t cc = 0; cc < datasets.size(); ++cc)
  {
    auto inDSA = datasets[cc]->GetPointData();
    this->InputPD.IntersectFieldList(inDSA);
  }

  // Configure the point attributes. This is nasty stuff because we may be
  // processing composite datasets with datasets with point attributes that don't
  // match one another. If the attributes don't match, then point interpolation
  // needs to be treated specially (in InterpolatePoint()), which is much slower.
  int numIntersectedArrays = this->InputPD.GetNumberOfArrays();
  this->HasMatchingPointAttributes = true;
  for (size_t cc = 0; cc < datasets.size(); ++cc)
  {
    auto inDSA = datasets[cc]->GetPointData();
    if (inDSA->GetNumberOfArrays() != numIntersectedArrays)
    {
      this->HasMatchingPointAttributes = false;
      break;
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkStreamTracer::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Configure the output for the filter. This means creating a composite
  // dataset (for simplicity) and configure the point data.
  if (!this->SetupOutput(inInfo, outInfo))
  {
    return 0;
  }

  // Configure the interpolated velocity field and begin integrating.
  vtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkDataSet* source = nullptr;
  if (sourceInfo)
  {
    source = vtkDataSet::SafeDownCast(sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
  }
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataArray* seeds = nullptr;
  vtkIdList* seedIds = nullptr;
  vtkIntArray* integrationDirections = nullptr;
  this->InitializeSeeds(seeds, seedIds, integrationDirections, source);

  if (seeds)
  {
    vtkAbstractInterpolatedVelocityField* func = nullptr;
    int maxCellSize = 0;
    this->SerialExecution = this->ForceSerialExecution;
    if (this->CheckInputs(func, &maxCellSize) != VTK_OK)
    {
      vtkDebugMacro("No appropriate inputs have been found. Can not execute.");
      if (func)
      {
        func->Delete();
      }
      seeds->Delete();
      integrationDirections->Delete();
      seedIds->Delete();
      this->InputData->UnRegister(this);
      return 1;
    }

    if (vtkOverlappingAMR::SafeDownCast(this->InputData))
    {
      vtkOverlappingAMR* amr = vtkOverlappingAMR::SafeDownCast(this->InputData);
      amr->GenerateParentChildInformation();
    }

    // The data that is interpolated comes from the "shape" of the input
    // point data.  This gets tricky when the data is composite, we need to
    // find a leaf dataset which defines the shape.
    vtkDataSet* input0 = nullptr;
    vtkCompositeDataIterator* iter = this->InputData->NewIterator();
    vtkSmartPointer<vtkCompositeDataIterator> iterP(iter);
    iter->Delete();

    iterP->GoToFirstItem();
    if (!iterP->IsDoneWithTraversal() && !input0)
    {
      input0 = vtkDataSet::SafeDownCast(iterP->GetCurrentDataObject());
      iterP->GoToNextItem();
    }

    int vecType(0);
    vtkDataArray* vectors = this->GetInputArrayToProcess(0, input0, vecType);

    if (vectors)
    {
      const char* vecName = vectors->GetName();
      double propagation = 0;
      vtkIdType numSteps = 0;
      double integrationTime = 0;
      this->Integrate(input0->GetPointData(), output, seeds, seedIds, integrationDirections, func,
        maxCellSize, vecType, vecName, propagation, numSteps, integrationTime,
        this->CustomTerminationCallback, this->CustomTerminationClientData,
        this->CustomReasonForTermination);
    } // if vectors are available
    func->Delete();
    seeds->Delete();
  } // if seeds are provided

  integrationDirections->Delete();
  seedIds->Delete();

  this->InputData->UnRegister(this);
  return 1;
}

//------------------------------------------------------------------------------
// The primary task of this function is to define the appropriate
// vtkAbstractInterpolatedVelocityField to use, and configure it.  This
// method clones the filter's InterpolatorPrototype (specified by the user),
// then initializes it (meaning building a cache for threaded computing), and
// returns a new vtkAbstractInterpolatedVelocityField. Later, during
// threading, this newly created vtkAbstractInterpolatedVelocityField is
// cloned to produce local instances in each thread. This is necessary
// because we don't want to modify the filters's user specified function
// prototype, so we have to make copies.
int vtkStreamTracer::CheckInputs(vtkAbstractInterpolatedVelocityField*& func, int* maxCellSize)
{
  if (!this->InputData)
  {
    return VTK_ERROR;
  }

  vtkOverlappingAMR* amrData = vtkOverlappingAMR::SafeDownCast(this->InputData);

  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(this->InputData->NewIterator());

  vtkDataSet* input0 = nullptr;
  iter->GoToFirstItem();
  while (!iter->IsDoneWithTraversal() && input0 == nullptr)
  {
    input0 = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    iter->GoToNextItem();
  }
  if (!input0)
  {
    return VTK_ERROR;
  }

  int vecType(0);
  vtkDataArray* vectors = this->GetInputArrayToProcess(0, input0, vecType);
  if (!vectors)
  {
    return VTK_ERROR;
  }

  // Set the function set to be integrated
  if (!this->InterpolatorPrototype)
  {
    if (amrData)
    {
      func = vtkAMRInterpolatedVelocityField::New();
    }
    else
    {
      func = vtkCompositeInterpolatedVelocityField::New();
    }
  }
  else
  {
    if (amrData &&
      vtkAMRInterpolatedVelocityField::SafeDownCast(this->InterpolatorPrototype) == nullptr)
    {
      this->InterpolatorPrototype = vtkAMRInterpolatedVelocityField::New();
    }
    func = this->InterpolatorPrototype->NewInstance();
  }

  // Copy information from interpolator.
  if (this->InterpolatorPrototype)
  {
    func->CopyParameters(this->InterpolatorPrototype);
  }

  // Tweak special cases.
  if (auto amrVelocityField = vtkAMRInterpolatedVelocityField::SafeDownCast(func))
  {
    assert(amrData);
    amrVelocityField->SetAMRData(amrData);
    if (maxCellSize)
    {
      *maxCellSize = 8;
    }
  }
  else if (auto compVelocityField = vtkCompositeInterpolatedVelocityField::SafeDownCast(func))
  {
    iter->GoToFirstItem();
    while (!iter->IsDoneWithTraversal())
    {
      vtkDataSet* inp = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (inp)
      {
        int cellSize = inp->GetMaxCellSize();
        if (cellSize > *maxCellSize)
        {
          *maxCellSize = cellSize;
        }
        compVelocityField->AddDataSet(inp);
      }
      iter->GoToNextItem();
    }
  }
  else
  {
    assert(false);
  }

  // Retrieve the vector name and type
  const char* vecName = vectors->GetName();
  func->SelectVectors(vecType, vecName);

  // This initializes / builds the data processing cache in support of threading etc.
  // It takes into account the input to the filter (which may be a composite dataset)
  // as well as any additional added datasets via AddDataSet().
  func->Initialize(this->InputData);
  if (func->GetInitializationState() == vtkAbstractInterpolatedVelocityField::SELF_INITIALIZE)
  {
    this->SerialExecution = true; // this is done for thread safety reasons
  }

  return VTK_OK;
}

// Support threaded integration of streamlines. Each streamline integration
// executes in a different thread (i.e., there is no benefit to threading
// if only a single streamline is integrated). Basically the way the threading
// works is that each thread processes a portion of the streamline seeds, each
// accumulating their own "output" via thread local storage. Then these thread
// outputs are combined to produce the final output.
namespace
{ // anonymous

// Special function to interpolate the point data from the input to the
// output if fast == true, then it just calls the usual InterpolatePoint
// function; otherwise, it makes sure the array exists in the input before
// trying to copy it to the output. This is meant for multiblock data sets
// where the grids may not have the same point data arrays or have them in
// different orders.
void InterpolatePoint(vtkDataSetAttributes* outPointData, vtkDataSetAttributes* inPointData,
  vtkIdType toId, vtkIdList* ids, double* weights, bool fast)
{
  if (fast)
  {
    outPointData->InterpolatePoint(inPointData, toId, ids, weights);
  }
  else
  {
    for (int i = outPointData->GetNumberOfArrays() - 1; i >= 0; i--)
    {
      vtkAbstractArray* toArray = outPointData->GetAbstractArray(i);
      if (vtkAbstractArray* fromArray = inPointData->GetAbstractArray(toArray->GetName()))
      {
        toArray->InterpolateTuple(toId, ids, fromArray, weights);
      }
    }
  }
}

// Each threaded tracer maintains its own output. To simplify things,
// the thread local output is contained in one struct.
struct vtkLocalThreadOutput
{
  // These are initialized in the Initialize() method.
  vtkSmartPointer<vtkInitialValueProblemSolver> LocalIntegrator;
  vtkSmartPointer<vtkAbstractInterpolatedVelocityField> Func;

  // These helper objects can be (mostly) initialized in this
  // struct's default constructor.
  std::vector<double> Weights;
  vtkSmartPointer<vtkGenericCell> Cell;
  vtkSmartPointer<vtkPoints> OutputPoints;
  vtkSmartPointer<vtkDoubleArray> Time;
  vtkSmartPointer<vtkDoubleArray> VelocityVectors;
  vtkSmartPointer<vtkDoubleArray> CellVectors;
  vtkSmartPointer<vtkDoubleArray> Vorticity;
  vtkSmartPointer<vtkDoubleArray> Rotation;
  vtkSmartPointer<vtkDoubleArray> AngularVelocity;
  vtkSmartPointer<vtkPolyData> Output;
  vtkPointData* OutputPD;  // convenience to get at Output's point data
  double LastUsedStepSize; // Used by streamline to convey step size

  // Construct the data local to each thread. This constructor
  // handles hard-wired initialization. In the thread Initialize()
  // method, additional initialization is performed which depends
  // on user-specified parameters.
  vtkLocalThreadOutput()
  {
    this->Cell.TakeReference(vtkGenericCell::New());
    this->OutputPoints.TakeReference(vtkPoints::New());

    this->Time.TakeReference(vtkDoubleArray::New());
    this->Time->SetName("IntegrationTime");

    this->VelocityVectors.TakeReference(vtkDoubleArray::New());

    this->CellVectors.TakeReference(vtkDoubleArray::New());
    this->CellVectors->SetNumberOfComponents(3);
    this->CellVectors->Allocate(3 * VTK_CELL_SIZE);

    this->Vorticity.TakeReference(vtkDoubleArray::New());
    this->Vorticity->SetNumberOfComponents(3);
    this->Vorticity->SetName("Vorticity");

    this->Rotation.TakeReference(vtkDoubleArray::New());
    this->Rotation->SetName("Rotation");

    this->AngularVelocity.TakeReference(vtkDoubleArray::New());
    this->AngularVelocity->SetName("AngularVelocity");

    this->Output.TakeReference(vtkPolyData::New());
    this->OutputPD = this->Output->GetPointData();

    this->LastUsedStepSize = 0.0;
  }

  // The copy constructor is necessary because the default copy constructor
  // doesn't work with smart pointers. The copy constructor is used to create
  // instances of vtkLocalThreadOutput for each thread.
  vtkLocalThreadOutput(const vtkLocalThreadOutput& other)
  {
    this->Cell.TakeReference(vtkGenericCell::New());
    this->OutputPoints.TakeReference(vtkPoints::New());

    this->Time.TakeReference(vtkDoubleArray::New());
    this->Time->SetName("IntegrationTime");

    this->VelocityVectors.TakeReference(vtkDoubleArray::New());

    this->CellVectors.TakeReference(vtkDoubleArray::New());
    this->CellVectors->SetNumberOfComponents(3);
    this->CellVectors->Allocate(3 * VTK_CELL_SIZE);

    this->Vorticity.TakeReference(vtkDoubleArray::New());
    this->Vorticity->SetNumberOfComponents(3);
    this->Vorticity->SetName("Vorticity");

    this->Rotation.TakeReference(vtkDoubleArray::New());
    this->Rotation->SetName("Rotation");

    this->AngularVelocity.TakeReference(vtkDoubleArray::New());
    this->AngularVelocity->SetName("AngularVelocity");

    this->Output.TakeReference(vtkPolyData::New());
    this->OutputPD = this->Output->GetPointData();

    this->LastUsedStepSize = other.LastUsedStepSize;
  }
};

// In order to ensure that the threaded output is the same as serial output,
// we organize output based on the seed number. Each seed will (likely)
// produce a stream tracer - these stream tracers are eventually composited
// (i.e., Reduced()) by seed number. That way, no matter what order the seeds
// are processed the output will be the same (i.e., results invariant).
//
// Also, this struct contains a more compact representation of streamlines
// (and associated cell data) which is expanded (in Reduce()) to produce the
// final filter output. A prefix sum is performed after all of the
// streamlines are generated, this generates offsets and such which are used
// to control where the output points and lines are written to the filter
// output.
struct TracerOffset
{
  // For each seed (and hence streamline), these are where the data
  // originated from (i.e., which thread generated it), a pointer to the
  // local thread data, the consecutive sequence of points that compose the
  // streamline, and the reason for termination. Note that in some
  // situations, a streamline will not be generated when just single points
  // are added (which may be outside of the domain so do not form a
  // streamline).
  vtkLocalThreadOutput* ThreadOutput;
  vtkIdType ThreadPtId; // the first point id defining the polyline
  vtkIdType NumPts;     // number of points defining polyline
  int RetVal;           // the return value / exit condition of the streamline

  // These keep track of where the output is written to (in the global
  // filter output).  Generated via a prefix sum/scan in Reduce().
  vtkIdType StartingPtId;   // the first point id in the polyline
  vtkIdType CellId;         // the cell id of the polyline
  vtkIdType CellConnOffset; // the offset into the connectivity array

  TracerOffset()
    : ThreadOutput(nullptr)
    , ThreadPtId(-1)
    , NumPts(0)
    , RetVal(vtkStreamTracer::NOT_INITIALIZED)
    , StartingPtId(0)
    , CellId(0)
    , CellConnOffset(0)
  {
  }
};

using TracerOffsets = std::vector<TracerOffset>;

// The following class performs the threaded streamline integration.  The
// data members below control the propagation of streamlines based on the
// state of the vtkStreamTracer. Because threads may execute in a different
// order between runs, and we'd like the output to stay the same across runs,
// we order the output based on seed number (in Offsets).
struct TracerIntegrator
{
  // Integrator data members
  vtkStreamTracer* StreamTracer;
  vtkCompositeDataSet* InputData;
  double MaximumError;
  vtkIdType MaximumNumberOfSteps;
  double MaximumPropagation;
  double RotationScale;
  double TerminalSpeed;
  double LastUsedStepSize;

  vtkDataSetAttributes* ProtoPD;
  vtkDataArray* SeedSource;
  vtkIdList* SeedIds;
  vtkIntArray* IntegrationDirections;
  TracerOffsets& Offsets;
  vtkAbstractInterpolatedVelocityField* FuncPrototype;
  vtkSmartPointer<vtkInitialValueProblemSolver> Integrator;
  double InPropagation;     // only applicable to streamline 0
  vtkIdType InNumSteps;     // only applicable to streamline 0
  double InIntegrationTime; // only applicable to streamline 0
  const char* VecName;
  vtkPolyData* Output;
  std::vector<CustomTerminationCallbackType> CustomTerminationCallback;
  std::vector<void*> CustomTerminationClientData;
  std::vector<int> CustomReasonForTermination;

  // The `LocalThreadOutput` data is collected on a per-thread basis. Each
  // thread generates one or more streamlines.
  vtkSMPThreadLocal<vtkLocalThreadOutput> LocalThreadOutput;

  int MaxCellSize;
  int VecType;
  bool ComputeVorticity;
  bool SurfaceStreamlines;
  bool HasMatchingPointAttributes;
  bool GenerateNormalsInIntegrate;

  TracerIntegrator(vtkStreamTracer* streamTracer, vtkCompositeDataSet* inputData, bool matchingAttr,
    vtkDataSetAttributes* protoPD, vtkDataArray* seedSource, vtkIdList* seedIds,
    vtkIntArray* intDirs, TracerOffsets& offsets, vtkAbstractInterpolatedVelocityField* func,
    vtkInitialValueProblemSolver* integrator, int maxCellSize, double inPropagation,
    vtkIdType inNumSteps, double inIntegrationTime, int vecType, const char* vecName,
    bool genNormals, vtkPolyData* output,
    std::vector<CustomTerminationCallbackType>& customTerminationCallback,
    std::vector<void*>& customTerminationClientData, std::vector<int>& customReasonForTermination)
    : StreamTracer(streamTracer)
    , InputData(inputData)
    , ProtoPD(protoPD)
    , SeedSource(seedSource)
    , SeedIds(seedIds)
    , IntegrationDirections(intDirs)
    , Offsets(offsets)
    , FuncPrototype(func)
    , Integrator(integrator)
    , InPropagation(inPropagation)
    , InNumSteps(inNumSteps)
    , InIntegrationTime(inIntegrationTime)
    , VecName(vecName)
    , Output(output)
    , CustomTerminationCallback(customTerminationCallback)
    , CustomTerminationClientData(customTerminationClientData)
    , CustomReasonForTermination(customReasonForTermination)
    , MaxCellSize(maxCellSize)
    , VecType(vecType)
    , HasMatchingPointAttributes(matchingAttr)
    , GenerateNormalsInIntegrate(genNormals)
  {
    this->MaximumError = this->StreamTracer->GetMaximumError();
    this->MaximumNumberOfSteps = this->StreamTracer->GetMaximumNumberOfSteps();
    this->MaximumPropagation = this->StreamTracer->GetMaximumPropagation();
    this->ComputeVorticity = this->StreamTracer->GetComputeVorticity();
    this->RotationScale = this->StreamTracer->GetRotationScale();
    this->TerminalSpeed = this->StreamTracer->GetTerminalSpeed();
    this->SurfaceStreamlines = this->StreamTracer->GetSurfaceStreamlines();
    this->LastUsedStepSize = 0.0;
  }

  void Initialize()
  {
    // Some data members of the local output require per-thread initialization.
    vtkLocalThreadOutput& localOutput = this->LocalThreadOutput.Local();

    localOutput.LocalIntegrator.TakeReference(this->Integrator->NewInstance());

    localOutput.Func.TakeReference(this->FuncPrototype->NewInstance());
    localOutput.Func->CopyParameters(this->FuncPrototype);

    if (this->VecType != vtkDataObject::POINT)
    {
      localOutput.VelocityVectors = vtkSmartPointer<vtkDoubleArray>::New();
      localOutput.VelocityVectors->SetName(this->VecName);
      localOutput.VelocityVectors->SetNumberOfComponents(3);
    }
    this->LocalThreadOutput.Local().Weights.resize(this->MaxCellSize);

    // Note: We have to use a specific value (safe to employ the maximum number
    //       of steps) as the size of the initial memory allocation here. The
    //       use of the default argument might incur a crash problem (due to
    //       "insufficient memory") in the parallel mode. This is the case when
    //       a streamline intensely shuttles between two processes in an exactly
    //       interleaving fashion --- only one point is produced on each process
    //       (and actually two points, after point duplication, are saved to a
    //       vtkPolyData in vtkDistributedStreamTracer::NoBlockProcessTask) and
    //       as a consequence a large number of such small vtkPolyData objects
    localOutput.Output->GetPointData()->InterpolateAllocate(
      this->ProtoPD, this->MaximumNumberOfSteps);
  }

  void operator()(vtkIdType seedNum, vtkIdType endSeedNum)
  {
    // Symbolic shortcuts to thread local data
    vtkLocalThreadOutput& localOutput = this->LocalThreadOutput.Local();
    std::vector<double>& weights = localOutput.Weights;
    vtkGenericCell* cell = localOutput.Cell;
    vtkInitialValueProblemSolver* integrator = localOutput.LocalIntegrator;
    vtkAbstractInterpolatedVelocityField* func = localOutput.Func;
    vtkPoints* outputPoints = localOutput.OutputPoints;
    vtkDoubleArray* time = localOutput.Time;
    vtkDoubleArray* velocityVectors = localOutput.VelocityVectors;
    vtkDoubleArray* cellVectors = localOutput.CellVectors;
    vtkDoubleArray* vorticity = localOutput.Vorticity;
    vtkDoubleArray* rotation = localOutput.Rotation;
    vtkDoubleArray* angularVel = localOutput.AngularVelocity;
    vtkPolyData* output = localOutput.Output;
    double& lastUsedStepSize = localOutput.LastUsedStepSize;

    // Initialize in preparation for stream tracer production
    vtkDataArray* seedSource = this->SeedSource;
    vtkIdList* seedIds = this->SeedIds;
    vtkIntArray* integrationDirections = this->IntegrationDirections;
    int vecType = this->VecType;
    const char* vecName = this->VecName;
    double lastPoint[3];
    double propagation;
    vtkIdType numSteps;
    double integrationTime;

    // Useful pointers
    vtkDataSetAttributes* outputPD = output->GetPointData();
    vtkPointData* inputPD;
    vtkDataSet* input;
    vtkDataArray* inVectors;

    int direction = 1;
    // Associate the interpolation function with the integrator
    integrator->SetFunctionSet(func);

    // Check Surface option
    vtkCompositeInterpolatedVelocityField* surfaceFunc = nullptr;
    if (this->SurfaceStreamlines)
    {
      surfaceFunc = vtkCompositeInterpolatedVelocityField::SafeDownCast(func);
      if (surfaceFunc)
      {
        surfaceFunc->SetForceSurfaceTangentVector(true);
        surfaceFunc->SetSurfaceDataset(true);
      }
    }

    // We will interpolate all point attributes of the input on each point of
    // the output (unless they are turned off). Note that we are using only
    // the first input, if there are more than one, the attributes have to match.
    double velocity[3];
    for (; seedNum < endSeedNum; ++seedNum)
    {
      if (seedNum == 0) // only update the first streamline, otherwise zero
      {
        propagation = this->InPropagation;
        numSteps = this->InNumSteps;
        integrationTime = this->InIntegrationTime;
      }
      else
      {
        propagation = 0;
        numSteps = 0;
        integrationTime = 0;
      }

      switch (integrationDirections->GetValue(seedNum))
      {
        case vtkStreamTracer::FORWARD:
          direction = 1;
          break;
        case vtkStreamTracer::BACKWARD:
          direction = -1;
          break;
      }

      // temporary variables used in the integration
      double point1[3], point2[3], pcoords[3], vort[3], omega;
      vtkIdType index, numPts = 0;

      // Clear the last cell to avoid starting a search from
      // the last point in the streamline
      func->ClearLastCellId();

      // Initial point
      seedSource->GetTuple(seedIds->GetId(seedNum), point1);
      memcpy(point2, point1, 3 * sizeof(double));
      if (!func->FunctionValues(point1, velocity))
      {
        continue;
      }

      if (propagation >= this->MaximumPropagation || numSteps > this->MaximumNumberOfSteps)
      {
        continue;
      }

      numPts++;
      vtkIdType nextPoint = outputPoints->InsertNextPoint(point1);
      double lastInsertedPoint[3];
      outputPoints->GetPoint(nextPoint, lastInsertedPoint);
      time->InsertNextValue(integrationTime);

      // We will always pass an arc-length step size to the integrator.
      // If the user specifies a step size in cell length unit, we will
      // have to convert it to arc length.
      vtkIntervalInformation stepSize; // either positive or negative
      stepSize.Unit = vtkStreamTracer::LENGTH_UNIT;
      stepSize.Interval = 0;
      vtkIntervalInformation aStep; // always positive
      aStep.Unit = vtkStreamTracer::LENGTH_UNIT;
      double step, minStep = 0, maxStep = 0;
      double stepTaken;
      double speed;
      double cellLength;
      int retVal = vtkStreamTracer::OUT_OF_LENGTH, tmp;

      // Make sure we use the dataset found by the vtkAbstractInterpolatedVelocityField
      input = func->GetLastDataSet();
      inputPD = input->GetPointData();
      inVectors = input->GetAttributesAsFieldData(vecType)->GetArray(vecName);
      // Convert intervals to arc-length unit
      input->GetCell(func->GetLastCellId(), cell);
      cellLength = std::sqrt(static_cast<double>(cell->GetLength2()));
      speed = vtkMath::Norm(velocity);
      // Never call conversion methods if speed == 0
      if (speed != 0.0)
      {
        this->StreamTracer->ConvertIntervals(
          stepSize.Interval, minStep, maxStep, direction, cellLength);
      }

      // Interpolate all point attributes on first point
      func->GetLastWeights(weights.data());
      InterpolatePoint(outputPD, inputPD, nextPoint, cell->PointIds, weights.data(),
        this->HasMatchingPointAttributes);
      // handle both point and cell velocity attributes.
      vtkDataArray* outputVelocityVectors = outputPD->GetArray(vecName);
      if (vecType != vtkDataObject::POINT)
      {
        velocityVectors->InsertNextTuple(velocity);
        outputVelocityVectors = velocityVectors;
      }

      // Compute vorticity if required.
      // This can be used later for streamribbon generation.
      if (this->ComputeVorticity)
      {
        if (vecType == vtkDataObject::POINT)
        {
          inVectors->GetTuples(cell->PointIds, cellVectors);
          func->GetLastLocalCoordinates(pcoords);
          this->StreamTracer->CalculateVorticity(cell, pcoords, cellVectors, vort);
        }
        else
        {
          vort[0] = 0;
          vort[1] = 0;
          vort[2] = 0;
        }
        vorticity->InsertNextTuple(vort);
        // rotation
        // local rotation = vorticity . unit tangent ( i.e. velocity/speed )
        if (speed != 0.0)
        {
          omega = vtkMath::Dot(vort, velocity);
          omega /= speed;
          omega *= this->RotationScale;
        }
        else
        {
          omega = 0.0;
        }
        angularVel->InsertNextValue(omega);
        rotation->InsertNextValue(0.0);
      }

      double error = 0;

      // Integrate until the maximum propagation length is reached,
      // maximum number of steps is reached or until a boundary is encountered.
      // Begin Integration
      while (propagation < this->MaximumPropagation)
      {

        if (numSteps++ > this->MaximumNumberOfSteps)
        {
          retVal = vtkStreamTracer::OUT_OF_STEPS;
          break;
        }

        bool endIntegration = false;
        for (std::size_t i = 0; i < this->CustomTerminationCallback.size(); ++i)
        {
          if (this->CustomTerminationCallback[i](this->CustomTerminationClientData[i], outputPoints,
                outputVelocityVectors, direction))
          {
            retVal = this->CustomReasonForTermination[i];
            endIntegration = true;
            break;
          }
        }
        if (endIntegration)
        {
          break;
        }

        // Never call conversion methods if speed == 0
        if ((speed == 0) || (speed <= this->TerminalSpeed))
        {
          retVal = vtkStreamTracer::STAGNATION;
          break;
        }

        // If, with the next step, propagation will be larger than
        // max, reduce it so that it is (approximately) equal to max.
        aStep.Interval = std::abs(stepSize.Interval);

        if ((propagation + aStep.Interval) > this->MaximumPropagation)
        {
          aStep.Interval = this->MaximumPropagation - propagation;
          if (stepSize.Interval >= 0)
          {
            stepSize.Interval = vtkIntervalInformation::ConvertToLength(aStep, cellLength);
          }
          else
          {
            stepSize.Interval = vtkIntervalInformation::ConvertToLength(aStep, cellLength) * (-1.0);
          }
          maxStep = stepSize.Interval;
        }
        lastUsedStepSize = stepSize.Interval;

        // Calculate the next step using the integrator provided
        // Break if the next point is out of bounds.
        func->SetNormalizeVector(true);
        tmp = integrator->ComputeNextStep(point1, point2, 0, stepSize.Interval, stepTaken, minStep,
          maxStep, this->MaximumError, error);
        func->SetNormalizeVector(false);
        if (tmp != 0)
        {
          retVal = tmp;
          memcpy(lastPoint, point2, 3 * sizeof(double));
          break;
        }

        // This is the next starting point
        if (this->SurfaceStreamlines && surfaceFunc != nullptr)
        {
          if (surfaceFunc->SnapPointOnCell(point2, point1) != 1)
          {
            retVal = vtkStreamTracer::OUT_OF_DOMAIN;
            memcpy(lastPoint, point2, 3 * sizeof(double));
            break;
          }
        }
        else
        {
          for (int i = 0; i < 3; i++)
          {
            point1[i] = point2[i];
          }
        }

        // Interpolate the velocity at the next point
        if (!func->FunctionValues(point2, velocity))
        {
          retVal = vtkStreamTracer::OUT_OF_DOMAIN;
          memcpy(lastPoint, point2, 3 * sizeof(double));
          break;
        }

        // It is not enough to use the starting point for stagnation calculation
        // Use average speed to check if it is below stagnation threshold
        double speed2 = vtkMath::Norm(velocity);
        if ((speed + speed2) / 2 <= this->TerminalSpeed)
        {
          retVal = vtkStreamTracer::STAGNATION;
          break;
        }

        integrationTime += stepTaken / speed;
        // Calculate propagation (using the same units as MaximumPropagation
        propagation += std::abs(stepSize.Interval);

        // Make sure we use the dataset found by the vtkAbstractInterpolatedVelocityField
        input = func->GetLastDataSet();
        inputPD = input->GetPointData();
        inVectors = input->GetAttributesAsFieldData(vecType)->GetArray(vecName);

        // Calculate cell length and speed to be used in unit conversions
        input->GetCell(func->GetLastCellId(), cell);
        cellLength = std::sqrt(static_cast<double>(cell->GetLength2()));
        speed = speed2;

        // Check if conversion to float will produce a point in same place
        float convertedPoint[3];
        for (int i = 0; i < 3; i++)
        {
          convertedPoint[i] = point1[i];
        }
        if (lastInsertedPoint[0] != convertedPoint[0] ||
          lastInsertedPoint[1] != convertedPoint[1] || lastInsertedPoint[2] != convertedPoint[2])
        {
          // Point is valid. Insert it.
          numPts++;
          nextPoint = outputPoints->InsertNextPoint(point1);
          outputPoints->GetPoint(nextPoint, lastInsertedPoint);
          time->InsertNextValue(integrationTime);

          // Interpolate all point attributes on current point
          func->GetLastWeights(weights.data());
          InterpolatePoint(outputPD, inputPD, nextPoint, cell->PointIds, weights.data(),
            this->HasMatchingPointAttributes);

          if (vecType != vtkDataObject::POINT)
          {
            velocityVectors->InsertNextTuple(velocity);
          }
          // Compute vorticity if required
          // This can be used later for streamribbon generation.
          if (this->ComputeVorticity)
          {
            if (vecType == vtkDataObject::POINT)
            {
              inVectors->GetTuples(cell->PointIds, cellVectors);
              func->GetLastLocalCoordinates(pcoords);
              this->StreamTracer->CalculateVorticity(cell, pcoords, cellVectors, vort);
            }
            else
            {
              vort[0] = 0;
              vort[1] = 0;
              vort[2] = 0;
            }
            vorticity->InsertNextTuple(vort);
            // rotation
            // angular velocity = vorticity . unit tangent ( i.e. velocity/speed )
            // rotation = sum ( angular velocity * stepSize )
            omega = vtkMath::Dot(vort, velocity);
            omega /= speed;
            omega *= this->RotationScale;
            index = angularVel->InsertNextValue(omega);
            rotation->InsertNextValue(rotation->GetValue(index - 1) +
              (angularVel->GetValue(index - 1) + omega) / 2 *
                (integrationTime - time->GetValue(index - 1)));
          }
        }

        // Never call conversion methods if speed == 0
        if ((speed == 0) || (speed <= this->TerminalSpeed))
        {
          retVal = vtkStreamTracer::STAGNATION;
          break;
        }

        // Convert all intervals to arc length
        this->StreamTracer->ConvertIntervals(step, minStep, maxStep, direction, cellLength);

        // If the solver is adaptive and the next step size (stepSize.Interval)
        // that the solver wants to use is smaller than minStep or larger
        // than maxStep, re-adjust it. This has to be done every step
        // because minStep and maxStep can change depending on the cell
        // size (unless it is specified in arc-length unit)
        if (integrator->IsAdaptive())
        {
          if (std::abs(stepSize.Interval) < std::abs(minStep))
          {
            stepSize.Interval = std::abs(minStep) * stepSize.Interval / std::abs(stepSize.Interval);
          }
          else if (std::abs(stepSize.Interval) > std::abs(maxStep))
          {
            stepSize.Interval = std::abs(maxStep) * stepSize.Interval / std::abs(stepSize.Interval);
          }
        }
        else
        {
          stepSize.Interval = step;
        }
      }

      // If points have been inserted, keep track of information related to
      // this seed. A special case exists when numPts==1 since a valid
      // polyline has not been defined. However, the point is inserted and
      // for historical reasons this needs to be sent to the output. We also
      // keep track of other related information for the purposes of
      // generating offsets and in general managing the threading output.
      if (numPts > 0)
      {
        TracerOffset& offset = this->Offsets[seedNum];
        offset.ThreadOutput = &localOutput;
        offset.ThreadPtId = outputPoints->GetNumberOfPoints() - numPts;
        offset.NumPts = numPts;
        offset.RetVal = retVal;
      }

      // Update values of inPropagation, inNumSteps, and inIntegrationTime
      // which are passed out of the execution process. It is expected that
      // These values passed in the function call are only used for the first
      // line. What this means is that non-zero inPropagation, inNumSteps,
      // and inIntegrationTime only affect one (the very first)
      // streamline. This is an artifact of bad design since some of the API
      // presumes a single streamline (this also includes
      // LastUsedStepSize). This single streamline assumption is most
      // commonly used in MPI applications (e.g., see vtkPStreamTracer) where
      // single processes are processed in a distributed parallel manner.
      if (seedNum == 0) // if first seed
      {
        this->InPropagation = propagation;
        this->InNumSteps = numSteps;
        this->InIntegrationTime = integrationTime;
      }
    } // for all seeds in this batch
  }

  // Perform the final compositing operation to assemble the
  // filter output. Each seed is processed (which typically produces
  // one streamline) and copied to the filter output.
  struct CompositeOverSeeds
  {
    TracerOffsets& Offsets;
    vtkPoints* OutPoints;
    vtkIdType* CAOffsets;
    vtkIdType* CAConn;
    vtkPointData* OutPD;
    vtkIdList* SeedIds;
    int* OutSeedIds;
    int* OutRetVals;

    CompositeOverSeeds(TracerOffsets& offsets, vtkPoints* outPoints, vtkIdType* caOffsetsPtr,
      vtkIdType* caConnPtr, vtkPointData* outPD, vtkIdList* seedIds, int* seedIdsPtr,
      int* retValsPtr)
      : Offsets(offsets)
      , OutPoints(outPoints)
      , CAOffsets(caOffsetsPtr)
      , CAConn(caConnPtr)
      , OutPD(outPD)
      , SeedIds(seedIds)
      , OutSeedIds(seedIdsPtr)
      , OutRetVals(retValsPtr)
    {
    }

    void operator()(vtkIdType seedId, vtkIdType endSeedId)
    {
      double x[3];

      for (; seedId < endSeedId; ++seedId)
      {
        TracerOffset& offset = this->Offsets[seedId];
        if (offset.NumPts > 0) // If a point or polyline created
        {
          // Copy the thread points to the filter output. Also copy the point
          // data.
          vtkPoints* threadPts = offset.ThreadOutput->OutputPoints;
          vtkIdType outPtId = offset.StartingPtId;
          vtkPointData* threadPD = offset.ThreadOutput->OutputPD;
          for (auto i = 0; i < offset.NumPts; ++i)
          {
            vtkIdType threadId = offset.ThreadPtId + i;
            vtkIdType outId = outPtId + i;
            threadPts->GetPoint(threadId, x);
            this->OutPoints->SetPoint(outId, x);
            this->OutPD->CopyData(threadPD, threadId, outId);
          }

          // Now if this is a valid polyine (i.e., more than
          // one point) create the cell related information.
          if (offset.NumPts > 1)
          {
            vtkIdType cellId = offset.CellId;
            this->CAOffsets[cellId] = offset.CellConnOffset;
            vtkIdType connLoc = offset.CellConnOffset;
            outPtId = offset.StartingPtId;
            for (auto i = 0; i < offset.NumPts; ++i)
            {
              vtkIdType outId = outPtId + i;
              this->CAConn[connLoc++] = outId;
            }

            // Copy the cell data
            this->OutSeedIds[cellId] = this->SeedIds->GetId(seedId);
            this->OutRetVals[cellId] = offset.RetVal;
          } // if a valid polyline is created
        }   // if a streamline generated for this seed
      }     // for all seeds
    }       // operator()
  };

  // Assemble the thread output. This means adding the
  // appropriate output data arrays expected by the user.
  void AssembleOutput(vtkLocalThreadOutput& threadOutput)
  {
    vtkPointData* outputPD = threadOutput.OutputPD;
    outputPD->AddArray(threadOutput.Time);

    if (this->VecType != vtkDataObject::POINT)
    {
      outputPD->AddArray(threadOutput.VelocityVectors);
    }

    if (this->ComputeVorticity)
    {
      outputPD->AddArray(threadOutput.Vorticity);
      outputPD->AddArray(threadOutput.Rotation);
      outputPD->AddArray(threadOutput.AngularVelocity);
    }
  }

  // Combine the outputs of the threads into the filter output. This is
  // effectively a parallel append operation.
  void Reduce()
  {
    // Perform a prefix sum to generate offsets (i.e., point ids and cell
    // ids) and to determine the size of the containers that hold
    // them. These will be used to allocate the global filter output, and
    // copy thread data to the filter output.
    vtkIdType ptId = 0, cellId = 0, npts, cellConnOffset = 0;
    for (auto& offIter : this->Offsets)
    {
      // If points were created from this seed
      if ((npts = offIter.NumPts) > 0)
      {
        offIter.StartingPtId = ptId;
        ptId += npts;
        // If a polyline was created from this seed
        if (npts > 1)
        {
          offIter.CellId = cellId++;
          offIter.CellConnOffset = cellConnOffset;
          cellConnOffset += npts;
        }
      }
    }
    // The number of filter output points and cells.
    vtkIdType numPts = ptId;
    vtkIdType numCells = cellId;

    // Now finalize the output in each thread. Meaning assigning point data
    // to the thread output. (We deferred doing this previously so as not to
    // interfere with the point data interpolation of filter input
    // attributes.)  In the magical process of copying data from the threads
    // to the final filter output, all point data must be properly set up in
    // the thread output so that vtkPointData::CopyData() works properly.
    auto ldEnd = this->LocalThreadOutput.end();
    for (auto ldItr = this->LocalThreadOutput.begin(); ldItr != ldEnd; ++ldItr)
    {
      this->AssembleOutput(*ldItr);
      this->LastUsedStepSize = ldItr->LastUsedStepSize;
    }

    // In the following, allocate the output points, cell array, and the
    // point and cell attribute data.

    // Geometry: points
    vtkNew<vtkPoints> outPoints;
    outPoints->SetNumberOfPoints(numPts);

    // Topology: allocate objects that are assembled into the polyline
    // cell array.
    vtkNew<vtkIdTypeArray> caOffsets;
    vtkIdType* caOffsetsPtr = caOffsets->WritePointer(0, numCells + 1);
    caOffsetsPtr[numCells] = cellConnOffset;
    vtkNew<vtkIdTypeArray> caConn;
    vtkIdType* caConnPtr = caConn->WritePointer(0, cellConnOffset);

    // Interpolated point data: need to copy from thread local to the filter
    // output. Streamer point data: use the first thread local data to
    // configure the arrays (i.e., CopyAllocate()) because all threads have
    // been configured to have the same data attributes.
    vtkPointData* threadPD = this->LocalThreadOutput.begin()->OutputPD;
    vtkPointData* outputPD = this->Output->GetPointData();
    outputPD->CopyAllocate(threadPD, numPts);

    // Allocate streamer cell data: seed ids and streamer termination return
    // values. Only add this information if the number of output cells
    // is >0.
    int* seedIdsPtr = nullptr;
    int* retValsPtr = nullptr;
    if (numCells > 0)
    {
      vtkNew<vtkIntArray> seedIds;
      seedIdsPtr = seedIds->WritePointer(0, numCells);
      seedIds->SetName("SeedIds");

      vtkNew<vtkIntArray> retVals;
      retVals->SetName("ReasonForTermination");
      retValsPtr = retVals->WritePointer(0, numCells);

      this->Output->GetCellData()->AddArray(retVals);
      this->Output->GetCellData()->AddArray(seedIds);
    }

    // Now thread over the seeds, producing the final points, polylines,
    // and attribute data, as well as copying over interpolated point data.
    CompositeOverSeeds comp(this->Offsets, outPoints, caOffsetsPtr, caConnPtr, outputPD,
      this->SeedIds, seedIdsPtr, retValsPtr);
    vtkSMPTools::For(0, this->Offsets.size(), comp);

    // Finally, assemble the objects to create the filter output. It's possible
    // no streamlines were generated.
    this->Output->SetPoints(outPoints);
    if (numCells > 0)
    {
      vtkNew<vtkCellArray> streamers;
      streamers->SetData(caOffsets, caConn);
      this->Output->SetLines(streamers);
    }

    // If requested, generate normals
    if (this->GenerateNormalsInIntegrate)
    {
      this->StreamTracer->GenerateNormals(this->Output, nullptr, this->VecName);
    }

  } // Reduce()

}; // TracerIntegrator

} // anonymous

//------------------------------------------------------------------------------
// This method sets up the integration for one or more threads. Care has to
// be taken to ensure that duplicate locators are not created (memory
// issues), and that thread-safe operations are used. Note that the
// inPropagation, inNumSteps, and inIntegrationTime *affect only the first
// streamline* generated, and are typically used to continue a streamline
// across multiple executions.
void vtkStreamTracer::Integrate(vtkPointData* input0Data, vtkPolyData* output,
  vtkDataArray* seedSource, vtkIdList* seedIds, vtkIntArray* intDirs,
  vtkAbstractInterpolatedVelocityField* func, int maxCellSize, int vecType, const char* vecName,
  double& inPropagation, vtkIdType& inNumSteps, double& inIntegrationTime,
  std::vector<CustomTerminationCallbackType>& customTerminationCallback,
  std::vector<void*>& customTerminationClientData, std::vector<int>& customReasonForTermination)
{
  vtkInitialValueProblemSolver* integrator = this->GetIntegrator();
  if (integrator == nullptr)
  {
    vtkErrorMacro("No integrator is specified.");
    return;
  }

  // Setup the offsets for compositing data.
  vtkIdType numSeeds = seedIds->GetNumberOfIds();
  TracerOffsets offsets(numSeeds);

  // We will interpolate all point attributes of the input on each point of
  // the output (unless they are turned off). Note that we are using a prototype
  // of the input point data, which is the intersection of all possible input
  // attributes (if using composite/multiblock). The prototype is used to
  // establish which data arrays to interpolate from.
  vtkNew<vtkPointData> protoPD;
  this->InputPD.BuildPrototype(protoPD, input0Data);

  // Generate streamlines.
  TracerIntegrator ti(this, this->InputData, this->HasMatchingPointAttributes, protoPD, seedSource,
    seedIds, intDirs, offsets, func, integrator, maxCellSize, inPropagation, inNumSteps,
    inIntegrationTime, vecType, vecName, this->GenerateNormalsInIntegrate, output,
    customTerminationCallback, customTerminationClientData, customReasonForTermination);

  // Streamline threading only kicks in when the number of seeds exceeds a
  // threshold value.  This is because there is a cost to spinning up
  // threads, and then compositing the results. So for small numbers of
  // seeds, just use a serial approach. Otherwise thread the streamlines.
  const int VTK_ST_THREADING_THRESHOLD = 8;
  if (numSeeds < VTK_ST_THREADING_THRESHOLD || this->SerialExecution)
  { // Serial
    ti.Initialize();
    ti(0, numSeeds);
    ti.Reduce();
  }
  else
  {
    vtkSMPTools::For(0, numSeeds, ti);
  }

  // Update information from streamer execution
  this->LastUsedStepSize = ti.LastUsedStepSize;
  inPropagation = ti.InPropagation;
  inNumSteps = ti.InNumSteps;
  inIntegrationTime = ti.InIntegrationTime;
}

//------------------------------------------------------------------------------
void vtkStreamTracer::GenerateNormals(vtkPolyData* output, double* firstNormal, const char* vecName)
{
  vtkDataSetAttributes* outputPD = output->GetPointData();
  vtkPoints* outputPoints = output->GetPoints();
  vtkIdType numPts = outputPoints->GetNumberOfPoints();
  if (numPts <= 1 || !this->ComputeVorticity)
  {
    return;
  }

  // Setup the computation
  vtkCellArray* outputLines = output->GetLines();
  vtkDataArray* rotation = outputPD->GetArray("Rotation");

  vtkNew<vtkDoubleArray> normals;
  normals->SetNumberOfComponents(3);
  normals->SetNumberOfTuples(numPts);
  normals->SetName("Normals");

  // Make sure the normals are initialized in case
  // GenerateSlidingNormals() fails and returns before
  // creating all normals
  vtkSMPTools::For(0, numPts, [&](vtkIdType ptId, vtkIdType endPtId) {
    for (; ptId < endPtId; ++ptId)
    {
      normals->SetTuple3(ptId, 1, 0, 0);
    }
  });

  // Generate the orientation normals. This will be threaded since none of the
  // lines "reuse" points from another line.
  vtkNew<vtkPolyLine> lineNormalGenerator;
  lineNormalGenerator->GenerateSlidingNormals(
    outputPoints, outputLines, normals, firstNormal, true);

  // Now generate the final streamer normals
  vtkDataArray* newVectors = outputPD->GetVectors(vecName);
  if (newVectors == nullptr || newVectors->GetNumberOfTuples() != numPts)
  { // This should never happen.
    vtkErrorMacro("Bad velocity array.");
    return;
  }

  // Thread the final normal generation
  vtkSMPTools::For(0, numPts, [&](vtkIdType ptId, vtkIdType endPtId) {
    double normal[3], local1[3], local2[3], theta, costheta, sintheta, length;
    double velocity[3];
    for (; ptId < endPtId; ++ptId)
    {
      normals->GetTuple(ptId, normal);
      newVectors->GetTuple(ptId, velocity);
      // obtain two unit orthogonal vectors on the plane perpendicular to
      // the streamline
      for (auto j = 0; j < 3; j++)
      {
        local1[j] = normal[j];
      }
      length = vtkMath::Normalize(local1);
      vtkMath::Cross(local1, velocity, local2);
      vtkMath::Normalize(local2);
      // Rotate the normal with theta
      rotation->GetTuple(ptId, &theta);
      costheta = std::cos(theta);
      sintheta = std::sin(theta);
      for (auto j = 0; j < 3; j++)
      {
        normal[j] = length * (costheta * local1[j] + sintheta * local2[j]);
      }
      normals->SetTuple(ptId, normal);
    }
  }); // lambda

  // Associate normals with the output
  outputPD->AddArray(normals);
  outputPD->SetActiveAttribute("Normals", vtkDataSetAttributes::VECTORS);
}

//------------------------------------------------------------------------------
// This is used by sub-classes in certain situations. It
// does a lot less (for example, does not compute attributes)
// than Integrate.
double vtkStreamTracer::SimpleIntegrate(
  double seed[3], double lastPoint[3], double stepSize, vtkAbstractInterpolatedVelocityField* func)
{
  vtkIdType numSteps = 0;
  vtkIdType maxSteps = 20;
  double error = 0;
  double stepTaken = 0;
  double point1[3], point2[3];
  double velocity[3];
  double speed;
  int stepResult;

  (void)seed; // Seed is not used

  memcpy(point1, lastPoint, 3 * sizeof(double));

  // Create a new integrator, the type is the same as Integrator
  auto integrator =
    vtkSmartPointer<vtkInitialValueProblemSolver>::Take(this->GetIntegrator()->NewInstance());
  integrator->SetFunctionSet(func);

  while (true)
  {
    if (numSteps++ > maxSteps)
    {
      break;
    }

    // Calculate the next step using the integrator provided
    // Break if the next point is out of bounds.
    func->SetNormalizeVector(true);
    double tmpStepTaken = 0;
    stepResult =
      integrator->ComputeNextStep(point1, point2, 0, stepSize, tmpStepTaken, 0, 0, 0, error);
    stepTaken += tmpStepTaken;
    func->SetNormalizeVector(false);
    if (stepResult != 0)
    {
      memcpy(lastPoint, point2, 3 * sizeof(double));
      break;
    }

    // This is the next starting point
    for (int i = 0; i < 3; i++)
    {
      point1[i] = point2[i];
    }

    // Interpolate the velocity at the next point
    if (!func->FunctionValues(point2, velocity))
    {
      memcpy(lastPoint, point2, 3 * sizeof(double));
      break;
    }

    speed = vtkMath::Norm(velocity);

    // Never call conversion methods if speed == 0
    if ((speed == 0) || (speed <= this->TerminalSpeed))
    {
      break;
    }

    memcpy(point1, point2, 3 * sizeof(double));
    // End Integration
  }

  return stepTaken;
}

//------------------------------------------------------------------------------
int vtkStreamTracer::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  }
  else if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkStreamTracer::AddCustomTerminationCallback(
  CustomTerminationCallbackType callback, void* clientdata, int reasonForTermination)
{
  this->CustomTerminationCallback.push_back(callback);
  this->CustomTerminationClientData.push_back(clientdata);
  this->CustomReasonForTermination.push_back(reasonForTermination);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkStreamTracer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Start position: " << this->StartPosition[0] << " " << this->StartPosition[1]
     << " " << this->StartPosition[2] << endl;
  os << indent << "Terminal speed: " << this->TerminalSpeed << endl;

  os << indent << "Maximum propagation: " << this->MaximumPropagation << " unit: length." << endl;

  os << indent << "Integration step unit: "
     << ((this->IntegrationStepUnit == LENGTH_UNIT) ? "length." : "cell length.") << endl;

  os << indent << "Initial integration step: " << this->InitialIntegrationStep << endl;

  os << indent << "Minimum integration step: " << this->MinimumIntegrationStep << endl;

  os << indent << "Maximum integration step: " << this->MaximumIntegrationStep << endl;

  os << indent << "Integration direction: ";
  switch (this->IntegrationDirection)
  {
    case FORWARD:
      os << "forward.";
      break;
    case BACKWARD:
      os << "backward.";
      break;
    case BOTH:
      os << "both directions.";
      break;
  }
  os << endl;

  os << indent << "Integrator: " << this->Integrator << endl;
  os << indent << "Maximum error: " << this->MaximumError << endl;
  os << indent << "Maximum number of steps: " << this->MaximumNumberOfSteps << endl;
  os << indent << "Vorticity computation: " << (this->ComputeVorticity ? " On" : " Off") << endl;
  os << indent << "Rotation scale: " << this->RotationScale << endl;

  os << indent << "Force Serial Execution: " << (this->ForceSerialExecution ? " On" : " Off")
     << endl;
  os << indent << "UseLocalSeedSource: " << (this->UseLocalSeedSource ? "On" : "Off") << endl;
}

//------------------------------------------------------------------------------
vtkExecutive* vtkStreamTracer::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}
