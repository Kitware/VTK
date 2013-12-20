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
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkCellLocator.h"
#include "vtkModifiedBSPTree.h"
#include "vtkInterpolatedVelocityField.h"
#include "vtkAbstractInterpolatedVelocityField.h"
#include "vtkCellLocatorInterpolatedVelocityField.h"
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
#include "vtkSmartPointer.h"

#include <vector>

vtkObjectFactoryNewMacro(vtkStreamTracer)
vtkCxxSetObjectMacro(vtkStreamTracer,Integrator,vtkInitialValueProblemSolver);
vtkCxxSetObjectMacro(vtkStreamTracer,InterpolatorPrototype,vtkAbstractInterpolatedVelocityField);

const double vtkStreamTracer::EPSILON = 1.0E-12;

namespace
{
  // special function to interpolate the point data from the input to the output
  // if fast == true, then it just calls the usual InterpolatePoint function,
  // otherwise,
  // it makes sure the array exists in the input before trying to copy it to the
  // output. if it doesn't exist in the input but is in the output then we
  // remove it from the output instead of having bad values there.
  // this is meant for multiblock data sets where the grids may not have the
  // same point data arrays or have them in different orders.
  void InterpolatePoint(vtkDataSetAttributes* outPointData, vtkDataSetAttributes* inPointData,
                        vtkIdType toId, vtkIdList *ids, double *weights, bool fast)
  {
    if(fast)
      {
      outPointData->InterpolatePoint(inPointData, toId, ids, weights);
      }
    else
      {
      for(int i=outPointData->GetNumberOfArrays()-1;i>=0;i--)
        {
        vtkAbstractArray* toArray = outPointData->GetAbstractArray(i);
        if(vtkAbstractArray* fromArray = inPointData->GetAbstractArray(toArray->GetName()))
          {
          toArray->InterpolateTuple(toId, ids, fromArray, weights);
          }
        else
          {
          outPointData->RemoveArray(toArray->GetName());
          }
        }
      }
  }

}

vtkStreamTracer::vtkStreamTracer()
{
  this->Integrator = vtkRungeKutta2::New();
  this->IntegrationDirection = FORWARD;
  for(int i=0; i<3; i++)
    {
    this->StartPosition[i] = 0.0;
    }

  this->MaximumPropagation     = 1.0;
  this->IntegrationStepUnit    = CELL_LENGTH_UNIT;
  this->InitialIntegrationStep = 0.5;
  this->MinimumIntegrationStep = 1.0E-2;
  this->MaximumIntegrationStep = 1.0;

  this->MaximumError         = 1.0e-6;
  this->MaximumNumberOfSteps = 2000;
  this->TerminalSpeed        = EPSILON;

  this->ComputeVorticity = true;
  this->RotationScale    = 1.0;

  this->LastUsedStepSize = 0.0;

  this->GenerateNormalsInIntegrate = true;

  this->InterpolatorPrototype = 0;

  this->SetNumberOfInputPorts(2);

  // by default process active point vectors
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::VECTORS);

  this->HasMatchingPointAttributes = true;
}

vtkStreamTracer::~vtkStreamTracer()
{
  this->SetIntegrator(0);
  this->SetInterpolatorPrototype(0);
}

void vtkStreamTracer::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

void vtkStreamTracer::SetSourceData(vtkDataSet *source)
{
  this->SetInputData(1, source);
}

vtkDataSet *vtkStreamTracer::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
    {
    return 0;
    }
  return vtkDataSet::SafeDownCast(
    this->GetExecutive()->GetInputData(1, 0));
}

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

void vtkStreamTracer::SetInterpolatorTypeToDataSetPointLocator()
{
  this->SetInterpolatorType
    (  static_cast<int> ( INTERPOLATOR_WITH_DATASET_POINT_LOCATOR )  );
}

void vtkStreamTracer::SetInterpolatorTypeToCellLocator()
{
  this->SetInterpolatorType
    (  static_cast<int> ( INTERPOLATOR_WITH_CELL_LOCATOR )  );
}

void vtkStreamTracer::SetInterpolatorType( int interpType )
{
  if ( interpType == INTERPOLATOR_WITH_CELL_LOCATOR )
    {
    // create an interpolator equipped with a cell locator
    vtkSmartPointer< vtkCellLocatorInterpolatedVelocityField > cellLoc =
    vtkSmartPointer< vtkCellLocatorInterpolatedVelocityField >::New();

    // specify the type of the cell locator attached to the interpolator
    vtkSmartPointer< vtkModifiedBSPTree > cellLocType =
    vtkSmartPointer< vtkModifiedBSPTree >::New();
    cellLoc->SetCellLocatorPrototype( cellLocType.GetPointer() );

    this->SetInterpolatorPrototype( cellLoc.GetPointer() );
    }
  else
    {
    // create an interpolator equipped with a point locator (by default)
    vtkSmartPointer< vtkInterpolatedVelocityField > pntLoc =
    vtkSmartPointer< vtkInterpolatedVelocityField >::New();
    this->SetInterpolatorPrototype( pntLoc.GetPointer() );
    }
}

void vtkStreamTracer::SetIntegratorType(int type)
{
  vtkInitialValueProblemSolver* ivp=0;
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

void vtkStreamTracer::SetIntegrationStepUnit( int unit )
{
  if ( unit != LENGTH_UNIT && unit != CELL_LENGTH_UNIT )
    {
    unit = CELL_LENGTH_UNIT;
    }

  if ( unit == this->IntegrationStepUnit )
    {
    return;
    }

  this->IntegrationStepUnit = unit;
  this->Modified();
}

double vtkStreamTracer::ConvertToLength(
  double interval, int unit, double cellLength )
{
  double retVal = 0.0;
  if ( unit == LENGTH_UNIT )
    {
    retVal = interval;
    }
  else
  if ( unit == CELL_LENGTH_UNIT )
    {
    retVal = interval * cellLength;
    }
  return retVal;
}

double vtkStreamTracer::ConvertToLength(
  vtkStreamTracer::IntervalInformation& interval, double cellLength )
{
  return ConvertToLength( interval.Interval, interval.Unit, cellLength );
}

void vtkStreamTracer::ConvertIntervals( double& step, double& minStep,
  double& maxStep, int direction, double cellLength )
{
  minStep = maxStep = step =
    direction * this->ConvertToLength( this->InitialIntegrationStep,
                                       this->IntegrationStepUnit, cellLength );

  if ( this->MinimumIntegrationStep > 0.0 )
    {
    minStep = this->ConvertToLength( this->MinimumIntegrationStep,
                                     this->IntegrationStepUnit, cellLength );
    }

  if ( this->MaximumIntegrationStep > 0.0 )
    {
    maxStep = this->ConvertToLength( this->MaximumIntegrationStep,
                                     this->IntegrationStepUnit, cellLength );
    }
}

void vtkStreamTracer::CalculateVorticity(vtkGenericCell* cell,
                                         double pcoords[3],
                                         vtkDoubleArray* cellVectors,
                                         double vorticity[3])
{
  double* cellVel;
  double derivs[9];

  cellVel = cellVectors->GetPointer(0);
  cell->Derivatives(0, pcoords, cellVel, 3, derivs);
  vorticity[0] = derivs[7] - derivs[5];
  vorticity[1] = derivs[2] - derivs[6];
  vorticity[2] = derivs[3] - derivs[1];

}

void vtkStreamTracer::InitializeSeeds(vtkDataArray*& seeds,
                                      vtkIdList*& seedIds,
                                      vtkIntArray*& integrationDirections,
                                      vtkDataSet *source)
{
  seedIds = vtkIdList::New();
  integrationDirections = vtkIntArray::New();
  seeds=0;

  if (source)
    {
    int i;
    vtkIdType numSeeds = source->GetNumberOfPoints();
    if (numSeeds > 0)
      {
      // For now, one thread will do all

      if (this->IntegrationDirection == BOTH)
        {
        seedIds->SetNumberOfIds(2*numSeeds);
        for (i=0; i<numSeeds; i++)
          {
          seedIds->SetId(i, i);
          seedIds->SetId(numSeeds + i, i);
          }
        }
      else
        {
        seedIds->SetNumberOfIds(numSeeds);
        for (i=0; i<numSeeds; i++)
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
        for (i=0; i<numSeeds; i++)
          {
          seeds->SetTuple(i, source->GetPoint(i));
          }
        }
      }
    }
  else
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
      for(i=0; i<numSeeds; i++)
        {
        integrationDirections->InsertNextValue(FORWARD);
        }
      for(i=0; i<numSeeds; i++)
        {
        integrationDirections->InsertNextValue(BACKWARD);
        }
      }
    else
      {
      for(i=0; i<numSeeds; i++)
        {
        integrationDirections->InsertNextValue(this->IntegrationDirection);
        }
      }
    }
}

int vtkStreamTracer::SetupOutput(vtkInformation* inInfo,
                                 vtkInformation* outInfo)
{
  int piece=outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numPieces =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());

  vtkCompositeDataSet *hdInput = vtkCompositeDataSet::SafeDownCast(input);
  vtkDataSet* dsInput = vtkDataSet::SafeDownCast(input);
  if (hdInput)
    {
    this->InputData = hdInput;
    hdInput->Register(this);
    return 1;
    }
  else if (dsInput)
    {
    vtkDataSet* copy = dsInput->NewInstance();
    copy->ShallowCopy(dsInput);
    vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::New();
    mb->SetNumberOfBlocks(numPieces);
    mb->SetBlock(piece, copy);
    copy->Delete();
    this->InputData = mb;
    mb->Register(this);
    mb->Delete();
    return 1;
    }
  else
    {
    vtkErrorMacro("This filter cannot handle input of type: "
                  << (input?input->GetClassName():"(none)"));
    return 0;
    }

}

int vtkStreamTracer::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  if (!this->SetupOutput(inInfo, outInfo))
    {
    return 0;
    }

  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkDataSet *source = 0;
  if (sourceInfo)
    {
    source = vtkDataSet::SafeDownCast(
      sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
    }
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataArray* seeds = 0;
  vtkIdList* seedIds = 0;
  vtkIntArray* integrationDirections = 0;
  this->InitializeSeeds(seeds, seedIds, integrationDirections, source);

  if (seeds)
    {
    double lastPoint[3];
    vtkAbstractInterpolatedVelocityField* func = 0;
    int maxCellSize = 0;
    if (this->CheckInputs(func, &maxCellSize) != VTK_OK)
      {
      vtkDebugMacro("No appropriate inputs have been found. Can not execute.");
      if(func)
        {
        func->Delete();
        }
      seeds->Delete();
      integrationDirections->Delete();
      seedIds->Delete();
      this->InputData->UnRegister(this);
      return 1;
      }

    if(vtkOverlappingAMR::SafeDownCast(this->InputData))
      {
      vtkOverlappingAMR* amr =vtkOverlappingAMR::SafeDownCast(this->InputData);
      amr->GenerateParentChildInformation();
      }

    vtkCompositeDataIterator* iter = this->InputData->NewIterator();
    vtkSmartPointer<vtkCompositeDataIterator> iterP(iter);
    iter->Delete();

    iterP->GoToFirstItem();
    vtkDataSet* input0 = 0;
    if (!iterP->IsDoneWithTraversal() && !input0)
      {
      input0 = vtkDataSet::SafeDownCast(iterP->GetCurrentDataObject());
      iterP->GoToNextItem();
      }
    int vecType(0);
    vtkDataArray *vectors = this->GetInputArrayToProcess(0,input0,vecType);
    if (vectors)
      {
      const char *vecName = vectors->GetName();
      double propagation = 0;
      vtkIdType numSteps = 0;
      this->Integrate(input0->GetPointData(), output,
                      seeds, seedIds,
                      integrationDirections,
                      lastPoint, func,
                      maxCellSize, vecType,vecName,
                      propagation, numSteps);
      }
    func->Delete();
    seeds->Delete();
    }

  integrationDirections->Delete();
  seedIds->Delete();

  this->InputData->UnRegister(this);
  return 1;
}

int vtkStreamTracer::CheckInputs(vtkAbstractInterpolatedVelocityField*& func,
                                   int* maxCellSize)
{
  if (!this->InputData)
    {
    return VTK_ERROR;
    }

  vtkOverlappingAMR* amrData = vtkOverlappingAMR::SafeDownCast(this->InputData);

  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(this->InputData->NewIterator());

  vtkDataSet* input0 =NULL;
  iter->GoToFirstItem();
  while (!iter->IsDoneWithTraversal() && input0==NULL)
    {
    input0 = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    iter->GoToNextItem();
    }
  if(!input0)
    {
    return VTK_ERROR;
    }

  int vecType(0);
  vtkDataArray *vectors = this->GetInputArrayToProcess(0,input0,vecType);
  if (!vectors)
    {
    return VTK_ERROR;
    }

  // Set the function set to be integrated
  if ( !this->InterpolatorPrototype )
    {
    if(amrData)
      {
      func = vtkAMRInterpolatedVelocityField::New();
      }
    else
      {
      func = vtkInterpolatedVelocityField::New();
      }
    // turn on the following segment, in place of the above line, if an
    // interpolator equipped with a cell locator is dedired as the default
    //
    // func = vtkCellLocatorInterpolatedVelocityField::New();
    // vtkSmartPointer< vtkModifiedBSPTree > locator =
    // vtkSmartPointer< vtkModifiedBSPTree >::New();
    // vtkCellLocatorInterpolatedVelocityField::SafeDownCast( func )
    //   ->SetCellLocatorPrototype( locator.GetPointer() );
    }
  else
    {
    if(amrData && vtkAMRInterpolatedVelocityField::SafeDownCast(this->InterpolatorPrototype)==NULL)
      {
      this->InterpolatorPrototype = vtkAMRInterpolatedVelocityField::New();
      }
    func = this->InterpolatorPrototype->NewInstance();
    func->CopyParameters(this->InterpolatorPrototype);
    }

  if(vtkAMRInterpolatedVelocityField::SafeDownCast(func))
    {
    assert(amrData);
    vtkAMRInterpolatedVelocityField::SafeDownCast(func)->SetAMRData(amrData);
    if(maxCellSize)
      {
      *maxCellSize = 8;
      }
    }
  else if(vtkCompositeInterpolatedVelocityField::SafeDownCast(func))
    {
    iter->GoToFirstItem();
    while (!iter->IsDoneWithTraversal())
      {
      vtkDataSet* inp = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (inp)
        {
        int cellSize = inp->GetMaxCellSize();
        if ( cellSize > *maxCellSize )
          {
          *maxCellSize = cellSize;
          }
        vtkCompositeInterpolatedVelocityField::SafeDownCast(func)->AddDataSet(inp);
        }
      iter->GoToNextItem();
      }
    }
  else
    {
    assert(false);
    }

  const char *vecName = vectors->GetName();
  func->SelectVectors(vecType,vecName);

  //Check if the data attributes match, warn if not
  vtkPointData* pd0 = input0->GetPointData();
  int numPdArrays = pd0->GetNumberOfArrays();
  this->HasMatchingPointAttributes = true;
  for(iter->GoToFirstItem();!iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkDataSet* data = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    vtkPointData* pd = data->GetPointData();
    if(pd->GetNumberOfArrays()!=numPdArrays)
      {
      this->HasMatchingPointAttributes = false;
      }
    for(int i=0; i<numPdArrays; i++)
      {
      if( !pd->GetArray(pd0->GetArrayName(i))
        ||!pd0->GetArray(pd->GetArrayName(i)))
        {
        this->HasMatchingPointAttributes = false;
        }
      }
    }
  return VTK_OK;
}

void vtkStreamTracer::Integrate(vtkPointData *input0Data,
                                vtkPolyData* output,
                                vtkDataArray* seedSource,
                                vtkIdList* seedIds,
                                vtkIntArray* integrationDirections,
                                double lastPoint[3],
                                vtkAbstractInterpolatedVelocityField* func,
                                int maxCellSize,
                                int vecType,
                                const char *vecName,
                                double& inPropagation,
                                vtkIdType& inNumSteps)
{
  int i;
  vtkIdType numLines = seedIds->GetNumberOfIds();
  double propagation = inPropagation;
  vtkIdType numSteps = inNumSteps;

  // Useful pointers
  vtkDataSetAttributes* outputPD = output->GetPointData();
  vtkDataSetAttributes* outputCD = output->GetCellData();
  vtkPointData* inputPD;
  vtkDataSet* input;
  vtkDataArray* inVectors;

  int direction=1;

  if (this->GetIntegrator() == 0)
    {
    vtkErrorMacro("No integrator is specified.");
    return;
    }

  double* weights = 0;
  if ( maxCellSize > 0 )
    {
    weights = new double[maxCellSize];
    }

  // Used in GetCell()
  vtkGenericCell* cell = vtkGenericCell::New();

  // Create a new integrator, the type is the same as Integrator
  vtkInitialValueProblemSolver* integrator =
    this->GetIntegrator()->NewInstance();
  integrator->SetFunctionSet(func);

  // Since we do not know what the total number of points
  // will be, we do not allocate any. This is important for
  // cases where a lot of streamers are used at once. If we
  // were to allocate any points here, potentially, we can
  // waste a lot of memory if a lot of streamers are used.
  // Always insert the first point
  vtkPoints* outputPoints = vtkPoints::New();
  vtkCellArray* outputLines = vtkCellArray::New();

  // We will keep track of integration time in this array
  vtkDoubleArray* time = vtkDoubleArray::New();
  time->SetName("IntegrationTime");

  // This array explains why the integration stopped
  vtkIntArray* retVals = vtkIntArray::New();
  retVals->SetName("ReasonForTermination");

  vtkSmartPointer<vtkDoubleArray> velocityVectors;
  if(vecType != vtkDataObject::POINT)
    {
    velocityVectors = vtkSmartPointer<vtkDoubleArray>::New();
    velocityVectors->SetName(vecName);
    velocityVectors->SetNumberOfComponents(3);
    }
  vtkDoubleArray* cellVectors = 0;
  vtkDoubleArray* vorticity = 0;
  vtkDoubleArray* rotation = 0;
  vtkDoubleArray* angularVel = 0;
  if (this->ComputeVorticity)
    {
    cellVectors = vtkDoubleArray::New();
    cellVectors->SetNumberOfComponents(3);
    cellVectors->Allocate(3*VTK_CELL_SIZE);

    vorticity = vtkDoubleArray::New();
    vorticity->SetName("Vorticity");
    vorticity->SetNumberOfComponents(3);

    rotation = vtkDoubleArray::New();
    rotation->SetName("Rotation");

    angularVel = vtkDoubleArray::New();
    angularVel->SetName("AngularVelocity");
    }

  // We will interpolate all point attributes of the input on each point of
  // the output (unless they are turned off). Note that we are using only
  // the first input, if there are more than one, the attributes have to match.
  //
  // Note: We have to use a specific value (safe to employ the maximum number
  //       of steps) as the size of the initial memory allocation here. The
  //       use of the default argument might incur a crash problem (due to
  //       "insufficient memory") in the parallel mode. This is the case when
  //       a streamline intensely shuttles between two processes in an exactly
  //       interleaving fashion --- only one point is produced on each process
  //       (and actually two points, after point duplication, are saved to a
  //       vtkPolyData in vtkDistributedStreamTracer::NoBlockProcessTask) and
  //       as a consequence a large number of such small vtkPolyData objects
  //       are needed to represent a streamline, consuming up the memory before
  //       the intermediate memory is timely released.
  outputPD->InterpolateAllocate( input0Data,
                                 this->MaximumNumberOfSteps );

  vtkIdType numPtsTotal=0;
  double velocity[3];

  int shouldAbort = 0;

  for(int currentLine = 0; currentLine < numLines; currentLine++)
    {

    double progress = static_cast<double>(currentLine)/numLines;
    this->UpdateProgress(progress);

    switch (integrationDirections->GetValue(currentLine))
      {
      case FORWARD:
        direction = 1;
        break;
      case BACKWARD:
        direction = -1;
        break;
      }

    // temporary variables used in the integration
    double point1[3], point2[3], pcoords[3], vort[3], omega;
    vtkIdType index, numPts=0;

    // Clear the last cell to avoid starting a search from
    // the last point in the streamline
    func->ClearLastCellId();

    // Initial point
    seedSource->GetTuple(seedIds->GetId(currentLine), point1);
    memcpy(point2, point1, 3*sizeof(double));
    if (!func->FunctionValues(point1, velocity))
      {
      continue;
      }

    if ( propagation >= this->MaximumPropagation ||
         numSteps    >  this->MaximumNumberOfSteps)
      {
      continue;
      }

    numPts++;
    numPtsTotal++;
    vtkIdType nextPoint = outputPoints->InsertNextPoint(point1);
    time->InsertNextValue(0.0);

    // We will always pass an arc-length step size to the integrator.
    // If the user specifies a step size in cell length unit, we will
    // have to convert it to arc length.
    IntervalInformation stepSize;  // either positive or negative
    stepSize.Unit  = LENGTH_UNIT;
    stepSize.Interval = 0;
    IntervalInformation aStep; // always positive
    aStep.Unit = LENGTH_UNIT;
    double step, minStep=0, maxStep=0;
    double stepTaken, accumTime=0;
    double speed;
    double cellLength;
    int retVal=OUT_OF_LENGTH, tmp;

    // Make sure we use the dataset found by the vtkAbstractInterpolatedVelocityField
    input = func->GetLastDataSet();
    inputPD = input->GetPointData();
    inVectors = input->GetAttributesAsFieldData(vecType)->GetArray(vecName);
    // Convert intervals to arc-length unit
    input->GetCell(func->GetLastCellId(), cell);
    cellLength = sqrt(static_cast<double>(cell->GetLength2()));
    speed = vtkMath::Norm(velocity);
    // Never call conversion methods if speed == 0
    if ( speed != 0.0 )
      {
      this->ConvertIntervals( stepSize.Interval, minStep, maxStep,
                              direction, cellLength );
      }

    // Interpolate all point attributes on first point
    func->GetLastWeights(weights);
    InterpolatePoint(outputPD, inputPD, nextPoint, cell->PointIds, weights, this->HasMatchingPointAttributes);
    if(vecType != vtkDataObject::POINT)
      {
      velocityVectors->InsertNextTuple(velocity);
      }

    // Compute vorticity if required
    // This can be used later for streamribbon generation.
    if (this->ComputeVorticity)
      {
      if(vecType == vtkDataObject::POINT)
        {
        inVectors->GetTuples(cell->PointIds, cellVectors);
        func->GetLastLocalCoordinates(pcoords);
        vtkStreamTracer::CalculateVorticity(cell, pcoords, cellVectors, vort);
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
    while ( propagation < this->MaximumPropagation )
      {

      if (numSteps > this->MaximumNumberOfSteps)
        {
        retVal = OUT_OF_STEPS;
        break;
        }

      if ( numSteps++ % 1000 == 1 )
        {
        progress =
          ( currentLine + propagation / this->MaximumPropagation ) / numLines;
        this->UpdateProgress(progress);

        if (this->GetAbortExecute())
          {
          shouldAbort = 1;
          break;
          }
        }

      // Never call conversion methods if speed == 0
      if ( (speed == 0) || (speed <= this->TerminalSpeed) )
        {
        retVal = STAGNATION;
        break;
        }

      // If, with the next step, propagation will be larger than
      // max, reduce it so that it is (approximately) equal to max.
      aStep.Interval = fabs( stepSize.Interval );

      if ( ( propagation + aStep.Interval ) > this->MaximumPropagation )
        {
        aStep.Interval = this->MaximumPropagation - propagation;
        if ( stepSize.Interval >= 0 )
          {
          stepSize.Interval = this->ConvertToLength( aStep, cellLength );
          }
        else
          {
          stepSize.Interval = this->ConvertToLength( aStep, cellLength ) * ( -1.0 );
          }
        maxStep = stepSize.Interval;
        }
      this->LastUsedStepSize = stepSize.Interval;

      // Calculate the next step using the integrator provided
      // Break if the next point is out of bounds.
      func->SetNormalizeVector( true );
      tmp = integrator->ComputeNextStep( point1, point2, 0, stepSize.Interval,
                                         stepTaken, minStep, maxStep,
                                         this->MaximumError, error );
      func->SetNormalizeVector( false );
      if ( tmp != 0 )
        {
        retVal = tmp;
        memcpy(lastPoint, point2, 3*sizeof(double));
        break;
        }

      // This is the next starting point
      for(i=0; i<3; i++)
        {
        point1[i] = point2[i];
        }

      // Interpolate the velocity at the next point
      if ( !func->FunctionValues(point2, velocity) )
        {
        retVal = OUT_OF_DOMAIN;
        memcpy(lastPoint, point2, 3*sizeof(double));
        break;
        }

      // It is not enough to use the starting point for stagnation calculation
      // Use average speed to check if it is below stagnation threshold
      double speed2 = vtkMath::Norm(velocity);
      if ( (speed+speed2)/2 <= this->TerminalSpeed )
        {
        retVal = STAGNATION;
        break;
        }

      accumTime += stepTaken / speed;
      // Calculate propagation (using the same units as MaximumPropagation
      propagation += fabs( stepSize.Interval );

      // Make sure we use the dataset found by the vtkAbstractInterpolatedVelocityField
      input = func->GetLastDataSet();
      inputPD = input->GetPointData();
      inVectors = input->GetAttributesAsFieldData(vecType)->GetArray(vecName);


      // Point is valid. Insert it.
      numPts++;
      numPtsTotal++;
      nextPoint = outputPoints->InsertNextPoint(point1);
      time->InsertNextValue(accumTime);

      // Calculate cell length and speed to be used in unit conversions
      input->GetCell(func->GetLastCellId(), cell);
      cellLength = sqrt(static_cast<double>(cell->GetLength2()));
      speed = speed2;
      // Interpolate all point attributes on current point
      func->GetLastWeights(weights);
      InterpolatePoint(outputPD, inputPD, nextPoint, cell->PointIds, weights, this->HasMatchingPointAttributes);
      if(vecType != vtkDataObject::POINT)
        {
        velocityVectors->InsertNextTuple(velocity);
        }
      // Compute vorticity if required
      // This can be used later for streamribbon generation.
      if (this->ComputeVorticity)
        {
        if(vecType == vtkDataObject::POINT)
          {
          inVectors->GetTuples(cell->PointIds, cellVectors);
          func->GetLastLocalCoordinates(pcoords);
          vtkStreamTracer::CalculateVorticity(cell, pcoords, cellVectors, vort);
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
        rotation->InsertNextValue(rotation->GetValue(index-1) +
                                  (angularVel->GetValue(index-1) + omega)/2 *
                                  (accumTime - time->GetValue(index-1)));
        }

      // Never call conversion methods if speed == 0
      if ( (speed == 0) || (speed <= this->TerminalSpeed) )
        {
        retVal = STAGNATION;
        break;
        }

      // Convert all intervals to arc length
      this->ConvertIntervals( step, minStep, maxStep, direction, cellLength );


      // If the solver is adaptive and the next step size (stepSize.Interval)
      // that the solver wants to use is smaller than minStep or larger
      // than maxStep, re-adjust it. This has to be done every step
      // because minStep and maxStep can change depending on the cell
      // size (unless it is specified in arc-length unit)
      if (integrator->IsAdaptive())
        {
        if (fabs(stepSize.Interval) < fabs(minStep))
          {
          stepSize.Interval = fabs( minStep ) *
                                stepSize.Interval / fabs( stepSize.Interval );
          }
        else if (fabs(stepSize.Interval) > fabs(maxStep))
          {
          stepSize.Interval = fabs( maxStep ) *
                                stepSize.Interval / fabs( stepSize.Interval );
          }
        }
      else
        {
        stepSize.Interval = step;
        }

      // End Integration
      }

    if (shouldAbort)
      {
      break;
      }

    if (numPts > 1)
      {
      outputLines->InsertNextCell(numPts);
      for (i=numPtsTotal-numPts; i<numPtsTotal; i++)
        {
        outputLines->InsertCellPoint(i);
        }
      retVals->InsertNextValue(retVal);
      }

    // Initialize these to 0 before starting the next line.
    // The values passed in the function call are only used
    // for the first line.
    inPropagation = propagation;
    inNumSteps = numSteps;

    propagation = 0;
    numSteps = 0;
    }

  if (!shouldAbort)
    {
    // Create the output polyline
    output->SetPoints(outputPoints);
    outputPD->AddArray(time);
    if(vecType != vtkDataObject::POINT)
      {
      outputPD->AddArray(velocityVectors);
      }
    if (vorticity)
      {
      outputPD->AddArray(vorticity);
      outputPD->AddArray(rotation);
      outputPD->AddArray(angularVel);
      }

    vtkIdType numPts = outputPoints->GetNumberOfPoints();
    if ( numPts > 1 )
      {
      // Assign geometry and attributes
      output->SetLines(outputLines);
      if (this->GenerateNormalsInIntegrate)
        {
        this->GenerateNormals(output, 0, vecName);
        }

      outputCD->AddArray(retVals);
      }
    }

  if (vorticity)
    {
    vorticity->Delete();
    rotation->Delete();
    angularVel->Delete();
    }

  if (cellVectors)
    {
    cellVectors->Delete();
    }
  retVals->Delete();

  outputPoints->Delete();
  outputLines->Delete();

  time->Delete();


  integrator->Delete();
  cell->Delete();

  delete[] weights;

  output->Squeeze();
  return;
}

void vtkStreamTracer::GenerateNormals(vtkPolyData* output, double* firstNormal,
                                      const char *vecName)
{
  // Useful pointers
  vtkDataSetAttributes* outputPD = output->GetPointData();

  vtkPoints* outputPoints = output->GetPoints();
  vtkCellArray* outputLines = output->GetLines();

  vtkDataArray* rotation = outputPD->GetArray("Rotation");

  vtkIdType numPts = outputPoints->GetNumberOfPoints();
  if ( numPts > 1 )
    {
    if (this->ComputeVorticity)
      {
      vtkPolyLine* lineNormalGenerator = vtkPolyLine::New();
      vtkDoubleArray* normals = vtkDoubleArray::New();
      normals->SetNumberOfComponents(3);
      normals->SetNumberOfTuples(numPts);
      // Make sure the normals are initialized in case
      // GenerateSlidingNormals() fails and returns before
      // creating all normals
      for(vtkIdType idx=0; idx<numPts; idx++)
        {
        normals->SetTuple3(idx, 1, 0, 0);
        }

      lineNormalGenerator->GenerateSlidingNormals(outputPoints,
                                                  outputLines,
                                                  normals,
                                                  firstNormal);
      lineNormalGenerator->Delete();

      vtkIdType i;
      int j;
      double normal[3], local1[3], local2[3], theta, costheta, sintheta, length;
      double velocity[3];
      normals->SetName("Normals");
      vtkDataArray* newVectors =
        outputPD->GetVectors(vecName);
      for(i=0; i<numPts; i++)
        {
        normals->GetTuple(i, normal);
        if (newVectors == NULL || newVectors->GetNumberOfTuples()!=numPts)
          { // This should never happen.
          vtkErrorMacro("Bad velocity array.");
          return;
          }
        newVectors->GetTuple(i, velocity);
        // obtain two unit orthogonal vectors on the plane perpendicular to
        // the streamline
        for(j=0; j<3; j++) { local1[j] = normal[j]; }
        length = vtkMath::Normalize(local1);
        vtkMath::Cross(local1, velocity, local2);
        vtkMath::Normalize(local2);
        // Rotate the normal with theta
        rotation->GetTuple(i, &theta);
        costheta = cos(theta);
        sintheta = sin(theta);
        for(j=0; j<3; j++)
          {
          normal[j] = length* (costheta*local1[j] + sintheta*local2[j]);
          }
        normals->SetTuple(i, normal);
        }
      outputPD->AddArray(normals);
      outputPD->SetActiveAttribute("Normals", vtkDataSetAttributes::VECTORS);
      normals->Delete();
      }
    }
}


// This is used by sub-classes in certain situations. It
// does a lot less (for example, does not compute attributes)
// than Integrate.
void vtkStreamTracer::SimpleIntegrate(double seed[3],
                                      double lastPoint[3],
                                      double stepSize,
                                      vtkAbstractInterpolatedVelocityField* func)
{
  vtkIdType numSteps = 0;
  vtkIdType maxSteps = 20;
  double error = 0;
  double stepTaken;
  double point1[3], point2[3];
  double velocity[3];
  double speed;
  int    stepResult;

  (void)seed; // Seed is not used

  memcpy(point1, lastPoint, 3*sizeof(double));

  // Create a new integrator, the type is the same as Integrator
  vtkInitialValueProblemSolver* integrator =
    this->GetIntegrator()->NewInstance();
  integrator->SetFunctionSet(func);

  while ( 1 )
    {

    if (numSteps++ > maxSteps)
      {
      break;
      }

    // Calculate the next step using the integrator provided
    // Break if the next point is out of bounds.
    func->SetNormalizeVector( true );
    stepResult = integrator->ComputeNextStep( point1, point2, 0, stepSize,
                                              stepTaken, 0, 0, 0, error );
    func->SetNormalizeVector( false );
    if ( stepResult != 0 )
      {
      memcpy( lastPoint, point2, 3 * sizeof(double) );
      break;
      }


    // This is the next starting point
    for(int i=0; i<3; i++)
      {
      point1[i] = point2[i];
      }

    // Interpolate the velocity at the next point
    if ( !func->FunctionValues(point2, velocity) )
      {
      memcpy(lastPoint, point2, 3*sizeof(double));
      break;
      }

    speed = vtkMath::Norm(velocity);

    // Never call conversion methods if speed == 0
    if ( (speed == 0) || (speed <= this->TerminalSpeed) )
      {
      break;
      }

    memcpy(point1, point2, 3*sizeof(double));
    // End Integration
    }

  integrator->Delete();
}

int vtkStreamTracer::FillInputPortInformation(int port, vtkInformation *info)
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

void vtkStreamTracer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Start position: "
     << this->StartPosition[0] << " "
     << this->StartPosition[1] << " "
     << this->StartPosition[2] << endl;
  os << indent << "Terminal speed: " << this->TerminalSpeed << endl;

  os << indent << "Maximum propagation: " << this->MaximumPropagation
     << " unit: length." << endl;

  os << indent << "Integration step unit: "
     << ( ( this->IntegrationStepUnit == LENGTH_UNIT )
          ? "length." : "cell length." ) << endl;

  os << indent << "Initial integration step: "
     << this->InitialIntegrationStep << endl;

  os << indent << "Minimum integration step: "
     << this->MinimumIntegrationStep << endl;

  os << indent << "Maximum integration step: "
     << this->MaximumIntegrationStep << endl;

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
  os << indent << "Maximum number of steps: " << this->MaximumNumberOfSteps
     << endl;
  os << indent << "Vorticity computation: "
     << (this->ComputeVorticity ? " On" : " Off") << endl;
  os << indent << "Rotation scale: " << this->RotationScale << endl;
}

vtkExecutive* vtkStreamTracer::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}
