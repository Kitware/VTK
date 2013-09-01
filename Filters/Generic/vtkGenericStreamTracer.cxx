/*=========================================================================

  Program:   Visualization Toolkits
  Module:    vtkGenericStreamTracer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericStreamTracer.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkGenericInterpolatedVelocityField.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyLine.h"
#include "vtkRungeKutta2.h"
#include "vtkRungeKutta4.h"
#include "vtkRungeKutta45.h"
#include "vtkGenericDataSet.h"
#include "vtkGenericAttributeCollection.h"
#include "vtkGenericAttribute.h"
#include "vtkGenericAdaptorCell.h"
#include <cassert>

#include "vtkInformation.h"
#include "vtkExecutive.h" // for GetExecutive()
#include "vtkInformationVector.h"

vtkStandardNewMacro(vtkGenericStreamTracer);
vtkCxxSetObjectMacro(vtkGenericStreamTracer,Integrator,vtkInitialValueProblemSolver);
vtkCxxSetObjectMacro(vtkGenericStreamTracer,InterpolatorPrototype,vtkGenericInterpolatedVelocityField);

const double vtkGenericStreamTracer::EPSILON = 1.0E-12;

//-----------------------------------------------------------------------------
vtkGenericStreamTracer::vtkGenericStreamTracer()
{
  this->SetNumberOfInputPorts(2);

  this->Integrator = vtkRungeKutta2::New();
  this->IntegrationDirection = FORWARD;
  for(int i=0; i<3; i++)
    {
    this->StartPosition[i] = 0.0;
    }
  this->MaximumPropagation.Unit = LENGTH_UNIT;
  this->MaximumPropagation.Interval = 1.0;

  this->MinimumIntegrationStep.Unit = CELL_LENGTH_UNIT;
  this->MinimumIntegrationStep.Interval = 1.0E-2;

  this->MaximumIntegrationStep.Unit = CELL_LENGTH_UNIT;
  this->MaximumIntegrationStep.Interval = 1.0;

  this->InitialIntegrationStep.Unit = CELL_LENGTH_UNIT;
  this->InitialIntegrationStep.Interval = 0.5;

  this->MaximumError = 1.0e-6;

  this->MaximumNumberOfSteps = 2000;

  this->TerminalSpeed = EPSILON;

  this->ComputeVorticity = 1;
  this->RotationScale = 1.0;

  this->InputVectorsSelection = 0;

  this->LastUsedTimeStep = 0.0;

  this->GenerateNormalsInIntegrate = 1;

  this->InterpolatorPrototype = 0;
}

//-----------------------------------------------------------------------------
vtkGenericStreamTracer::~vtkGenericStreamTracer()
{
  this->SetIntegrator(0);
  this->SetInputVectorsSelection(0);
  this->SetInterpolatorPrototype(0);
}

//-----------------------------------------------------------------------------
void vtkGenericStreamTracer::SetSourceData(vtkDataSet *source)
{
  this->SetInputDataInternal(1, source);
}

//-----------------------------------------------------------------------------
vtkDataSet *vtkGenericStreamTracer::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1) // because the port is optional
    {
    return 0;
    }
  return static_cast<vtkDataSet *>(this->GetExecutive()->GetInputData(1, 0));
}

//-----------------------------------------------------------------------------
void vtkGenericStreamTracer::AddInputData(vtkGenericDataSet* input)
{
  this->Superclass::AddInputData(input);
}


//----------------------------------------------------------------------------
int vtkGenericStreamTracer
::FillInputPortInformation(int port, vtkInformation* info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  if(port==1)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(),1);
    }
  else
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGenericDataSet");
    }
  return 1;
}


//-----------------------------------------------------------------------------
int vtkGenericStreamTracer::GetIntegratorType()
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

//-----------------------------------------------------------------------------
void vtkGenericStreamTracer::SetIntegratorType(int type)
{
  vtkInitialValueProblemSolver* ivp = 0;
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

//-----------------------------------------------------------------------------
void vtkGenericStreamTracer::SetIntervalInformation(
  int unit,
  vtkGenericStreamTracer::IntervalInformation& currentValues)
{
  if ( unit == currentValues.Unit )
    {
    return;
    }

  if ( (unit < TIME_UNIT) || (unit > CELL_LENGTH_UNIT) )
    {
    vtkWarningMacro("Unrecognized unit. Using TIME_UNIT instead.");
    currentValues.Unit = TIME_UNIT;
    }
  else
    {
    currentValues.Unit = unit;
    }

  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkGenericStreamTracer::SetIntervalInformation(
  int unit,
  double interval, vtkGenericStreamTracer::IntervalInformation& currentValues)
{
  if ( (unit == currentValues.Unit) && (interval == currentValues.Interval) )
    {
    return;
    }

  this->SetIntervalInformation(unit, currentValues);

  currentValues.Interval = interval;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkGenericStreamTracer::SetMaximumPropagation(int unit, double max)
{
  this->SetIntervalInformation(unit, max, this->MaximumPropagation);
}
//-----------------------------------------------------------------------------
void vtkGenericStreamTracer::SetMaximumPropagation( double max)
{
  if ( max == this->MaximumPropagation.Interval )
    {
    return;
    }
  this->MaximumPropagation.Interval = max;
  this->Modified();
}
//-----------------------------------------------------------------------------
void vtkGenericStreamTracer::SetMaximumPropagationUnit(int unit)
{
  this->SetIntervalInformation(unit, this->MaximumPropagation);
}
//-----------------------------------------------------------------------------
int vtkGenericStreamTracer::GetMaximumPropagationUnit()
{
  return this->MaximumPropagation.Unit;
}
//-----------------------------------------------------------------------------
double vtkGenericStreamTracer::GetMaximumPropagation()
{
  return this->MaximumPropagation.Interval;
}

//-----------------------------------------------------------------------------
void vtkGenericStreamTracer::SetMinimumIntegrationStep(int unit, double step)
{
  this->SetIntervalInformation(unit, step, this->MinimumIntegrationStep);
}
//-----------------------------------------------------------------------------
void vtkGenericStreamTracer::SetMinimumIntegrationStepUnit(int unit)
{
  this->SetIntervalInformation(unit, this->MinimumIntegrationStep);
}
//-----------------------------------------------------------------------------
void vtkGenericStreamTracer::SetMinimumIntegrationStep(double step)
{
  if ( step == this->MinimumIntegrationStep.Interval )
    {
    return;
    }
  this->MinimumIntegrationStep.Interval = step;
  this->Modified();
}
//-----------------------------------------------------------------------------
int vtkGenericStreamTracer::GetMinimumIntegrationStepUnit()
{
  return this->MinimumIntegrationStep.Unit;
}
//-----------------------------------------------------------------------------
double vtkGenericStreamTracer::GetMinimumIntegrationStep()
{
  return this->MinimumIntegrationStep.Interval;
}

//-----------------------------------------------------------------------------
void vtkGenericStreamTracer::SetMaximumIntegrationStep(int unit, double step)
{
  this->SetIntervalInformation(unit, step, this->MaximumIntegrationStep);
}
//-----------------------------------------------------------------------------
void vtkGenericStreamTracer::SetMaximumIntegrationStepUnit(int unit)
{
  this->SetIntervalInformation(unit, this->MaximumIntegrationStep);
}
//-----------------------------------------------------------------------------
void vtkGenericStreamTracer::SetMaximumIntegrationStep(double step)
{
  if ( step == this->MaximumIntegrationStep.Interval )
    {
    return;
    }
  this->MaximumIntegrationStep.Interval = step;
  this->Modified();
}
//-----------------------------------------------------------------------------
int vtkGenericStreamTracer::GetMaximumIntegrationStepUnit()
{
  return this->MaximumIntegrationStep.Unit;
}
//-----------------------------------------------------------------------------
double vtkGenericStreamTracer::GetMaximumIntegrationStep()
{
  return this->MaximumIntegrationStep.Interval;
}

//-----------------------------------------------------------------------------
void vtkGenericStreamTracer::SetInitialIntegrationStep(int unit, double step)
{
  this->SetIntervalInformation(unit, step, this->InitialIntegrationStep);
}
//-----------------------------------------------------------------------------
void vtkGenericStreamTracer::SetInitialIntegrationStepUnit(int unit)
{
  this->SetIntervalInformation(unit, this->InitialIntegrationStep);
}
//-----------------------------------------------------------------------------
void vtkGenericStreamTracer::SetInitialIntegrationStep(double step)
{
  if ( step == this->InitialIntegrationStep.Interval )
    {
    return;
    }
  this->InitialIntegrationStep.Interval = step;
  this->Modified();
}
//-----------------------------------------------------------------------------
int vtkGenericStreamTracer::GetInitialIntegrationStepUnit()
{
  return this->InitialIntegrationStep.Unit;
}
//-----------------------------------------------------------------------------
double vtkGenericStreamTracer::GetInitialIntegrationStep()
{
  return this->InitialIntegrationStep.Interval;
}

//-----------------------------------------------------------------------------
double vtkGenericStreamTracer::ConvertToTime(
  vtkGenericStreamTracer::IntervalInformation& interval,
  double cellLength,
  double speed)
{
  double retVal = 0.0;
  switch (interval.Unit)
    {
    case TIME_UNIT:
      retVal = interval.Interval;
      break;
    case LENGTH_UNIT:
      retVal = interval.Interval/speed;
      break;
    case CELL_LENGTH_UNIT:
      retVal = interval.Interval*cellLength/speed;
      break;
    }
  return retVal;
}

//-----------------------------------------------------------------------------
double vtkGenericStreamTracer::ConvertToLength(
  vtkGenericStreamTracer::IntervalInformation& interval,
  double cellLength,
  double speed)
{
  double retVal = 0.0;
  switch (interval.Unit)
    {
    case TIME_UNIT:
      retVal = interval.Interval * speed;
      break;
    case LENGTH_UNIT:
      retVal = interval.Interval;
      break;
    case CELL_LENGTH_UNIT:
      retVal = interval.Interval*cellLength;
      break;
    }
  return retVal;
}

//-----------------------------------------------------------------------------
double vtkGenericStreamTracer::ConvertToCellLength(
  vtkGenericStreamTracer::IntervalInformation& interval,
  double cellLength,
  double speed)
{
  double retVal = 0.0;
  switch (interval.Unit)
    {
    case TIME_UNIT:
      retVal = (interval.Interval * speed)/cellLength;
      break;
    case LENGTH_UNIT:
      retVal = interval.Interval/cellLength;
      break;
    case CELL_LENGTH_UNIT:
      retVal = interval.Interval;
      break;
    }
  return retVal;
}

//-----------------------------------------------------------------------------
double vtkGenericStreamTracer::ConvertToUnit(
  vtkGenericStreamTracer::IntervalInformation& interval,
  int unit,
  double cellLength,
  double speed)
{
  double retVal = 0.0;
  switch (unit)
    {
    case TIME_UNIT:
      retVal = ConvertToTime(interval, cellLength, speed);
      break;
    case LENGTH_UNIT:
      retVal = ConvertToLength(interval, cellLength, speed);
      break;
    case CELL_LENGTH_UNIT:
      retVal = ConvertToCellLength(interval, cellLength, speed);
      break;
    }
  return retVal;
}

//-----------------------------------------------------------------------------
void vtkGenericStreamTracer::ConvertIntervals(double& step,
                                              double& minStep,
                                              double& maxStep,
                                              int direction,
                                              double cellLength,
                                              double speed)
{
  step = direction * this->ConvertToTime(
    this->InitialIntegrationStep, cellLength, speed);
  if ( this->MinimumIntegrationStep.Interval <= 0.0 )
    {
    minStep = step;
    }
  else
    {
    minStep = this->ConvertToTime(this->MinimumIntegrationStep, cellLength,
                                  speed);
    }
  if ( this->MaximumIntegrationStep.Interval <= 0.0 )
    {
    maxStep = step;
    }
  else
    {
    maxStep = this->ConvertToTime(this->MaximumIntegrationStep,cellLength,
                                  speed);
    }
}

//-----------------------------------------------------------------------------
void vtkGenericStreamTracer::CalculateVorticity(vtkGenericAdaptorCell* cell,
                                                double pcoords[3],
                                                vtkGenericAttribute *attribute,
                                                double vorticity[3])
{
  assert("pre: attribute_exists" && attribute!=0);
  assert("pre: point_centered_attribute" && attribute->GetCentering()==vtkPointCentered);
  assert("pre: vector_attribute" && attribute->GetType()==vtkDataSetAttributes::VECTORS);

  double derivs[9];
  cell->Derivatives(0,pcoords,attribute,derivs);

  vorticity[0] = derivs[7] - derivs[5];
  vorticity[1] = derivs[2] - derivs[6];
  vorticity[2] = derivs[3] - derivs[1];
}

//-----------------------------------------------------------------------------
void vtkGenericStreamTracer::InitializeSeeds(
  vtkDataArray*& seeds,
  vtkIdList*& seedIds,
  vtkIntArray*& integrationDirections)
{
  vtkDataSet* source = this->GetSource();
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

//-----------------------------------------------------------------------------
int vtkGenericStreamTracer::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkGenericDataSet *input = vtkGenericDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataArray* seeds = 0;
  vtkIdList* seedIds = 0;
  vtkIntArray* integrationDirections = 0;
  this->InitializeSeeds(seeds, seedIds, integrationDirections);

  if (seeds)
    {
    double lastPoint[3];
    vtkGenericInterpolatedVelocityField* func;
    if (this->CheckInputs(func, inputVector) != VTK_OK)
      {
      vtkDebugMacro("No appropriate inputs have been found. Can not execute.");
      func->Delete();
      seeds->Delete();
      integrationDirections->Delete();
      seedIds->Delete();
      return 1;
      }
    this->Integrate(input,
                    output,
                    seeds,
                    seedIds,
                    integrationDirections,
                    lastPoint,
                    func);
    func->Delete();
    seeds->Delete();
    }

  integrationDirections->Delete();
  seedIds->Delete();
  return 1;
}

//-----------------------------------------------------------------------------
int vtkGenericStreamTracer::CheckInputs(
  vtkGenericInterpolatedVelocityField*& func,
  vtkInformationVector **inputVector
  )
{
  // Set the function set to be integrated
  if (!this->InterpolatorPrototype)
    {
    func = vtkGenericInterpolatedVelocityField::New();
    }
  else
    {
    func = this->InterpolatorPrototype->NewInstance();
    func->CopyParameters(this->InterpolatorPrototype);
    }
  func->SelectVectors(this->InputVectorsSelection);

  // Add all the inputs ( except source, of course ) which
  // have the appropriate vectors and compute the maximum
  // cell size.
  int numInputs = 0;
  int numInputConnections = this->GetNumberOfInputConnections(0);
  for (int i = 0; i < numInputConnections; i++)
    {
    vtkInformation *info = inputVector[0]->GetInformationObject(i);
    vtkGenericDataSet* inp=0;

    if(info!=0)
      {
      inp = vtkGenericDataSet::SafeDownCast(
        info->Get(vtkDataObject::DATA_OBJECT()));
      }
    if(inp!=0)
      {
    int attrib;
      int attributeFound;
      if(this->InputVectorsSelection!=0)
        {
        attrib=inp->GetAttributes()->FindAttribute(this->InputVectorsSelection);

        attributeFound=attrib>=0;
        if(attributeFound)
          {
          attributeFound=(inp->GetAttributes()->GetAttribute(attrib)->GetType()==vtkDataSetAttributes::VECTORS)&&(inp->GetAttributes()->GetAttribute(attrib)->GetCentering()==vtkPointCentered);
          }
        }
      else
        {
        // Find the first attribute, point centered and with vector type.
        attrib=0;
        attributeFound=0;
        int c=inp->GetAttributes()->GetNumberOfAttributes();
        while(attrib<c&&!attributeFound)
          {
          attributeFound =
            (inp->GetAttributes()->GetAttribute(attrib)->GetType() ==
             vtkDataSetAttributes::VECTORS)
            &&
            (inp->GetAttributes()->GetAttribute(attrib)->GetCentering() ==
             vtkPointCentered);
          ++attrib;
          }
        if(attributeFound)
          {
          --attrib;
          this->SetInputVectorsSelection(inp->GetAttributes()->GetAttribute(attrib)->GetName());
          }
        }
      if (!attributeFound)
        {
        vtkDebugMacro("Input " << i << "does not contain a velocity vector.");
        continue;
        }
      func->AddDataSet(inp);
      numInputs++;
      }
    }
  if ( numInputs == 0 )
    {
    vtkDebugMacro("No appropriate inputs have been found. Can not execute.");
    return VTK_ERROR;
    }
  return VTK_OK;
}

//-----------------------------------------------------------------------------
void vtkGenericStreamTracer::Integrate(
  vtkGenericDataSet *input0,
  vtkPolyData* output,
  vtkDataArray* seedSource,
  vtkIdList* seedIds,
  vtkIntArray* integrationDirections,
  double lastPoint[3],
  vtkGenericInterpolatedVelocityField* func)
{
  int i;
  vtkIdType numLines = seedIds->GetNumberOfIds();

  // Useful pointers
  vtkDataSetAttributes* outputPD = output->GetPointData();
  vtkDataSetAttributes* outputCD = output->GetCellData();
  vtkGenericDataSet* input;
  vtkGenericAttribute* inVectors;

  int direction=1;

  if (this->GetIntegrator() == 0)
    {
    vtkErrorMacro("No integrator is specified.");
    return;
    }

  // Used in GetCell()
//  vtkGenericCell* cell = vtkGenericCell::New();
  vtkGenericAdaptorCell *cell=0;

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

  // We will keep track of time in this array
  vtkDoubleArray* time = vtkDoubleArray::New();
  time->SetName("IntegrationTime");

  // This array explains why the integration stopped
  vtkIntArray* retVals = vtkIntArray::New();
  retVals->SetName("ReasonForTermination");

  vtkDoubleArray* vorticity = 0;
  vtkDoubleArray* rotation = 0;
  vtkDoubleArray* angularVel = 0;
  if (this->ComputeVorticity)
    {
    vorticity = vtkDoubleArray::New();
    vorticity->SetName("Vorticity");
    vorticity->SetNumberOfComponents(3);

    rotation = vtkDoubleArray::New();
    rotation->SetName("Rotation");

    angularVel = vtkDoubleArray::New();
    angularVel->SetName("AngularVelocity");
    }

  // We will interpolate all point attributes of the input on
  // each point of the output (unless they are turned off)
  // Note that we are using only the first input, if there are more
  // than one, the attributes have to match.

  // prepare the output attributes
  vtkGenericAttributeCollection *attributes=input0->GetAttributes();
  vtkGenericAttribute *attribute;
  vtkDataArray *attributeArray;

  int c = attributes->GetNumberOfAttributes();
  int attributeType;

  // Only point centered attributes will be interpolated.
  // Cell centered attributes are not ignored and not copied in output:
  // is a missing part in vtkStreamTracer? Need to ask to the Berk.
  for(i=0; i<c; ++i)
    {
    attribute=attributes->GetAttribute(i);
    attributeType=attribute->GetType();
    if(attribute->GetCentering()==vtkPointCentered)
      {
      attributeArray=vtkDataArray::CreateDataArray(attribute->GetComponentType());
      attributeArray->SetNumberOfComponents(attribute->GetNumberOfComponents());
      attributeArray->SetName(attribute->GetName());
      outputPD->AddArray(attributeArray);
      attributeArray->Delete();

      if(outputPD->GetAttribute(attributeType)==0)
        {
        outputPD->SetActiveAttribute(outputPD->GetNumberOfArrays()-1,
                                     attributeType);
        }
      }
    }
  double *values = new double[outputPD->GetNumberOfComponents()]; // point centered attributes at some point.


  // Note:  It is an overestimation to have the estimate the same number of
  // output points and input points.  We sill have to squeeze at end.

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
    func->ClearLastCell();


    // Initial point
    seedSource->GetTuple(seedIds->GetId(currentLine), point1);
    memcpy(point2, point1, 3*sizeof(double));
    if (!func->FunctionValues(point1, velocity))
      {
      continue;
      }

    numPts++;
    numPtsTotal++;
    vtkIdType nextPoint = outputPoints->InsertNextPoint(point1);
    time->InsertNextValue(0.0);

    // We will always pass a time step to the integrator.
    // If the user specifies a step size with another unit, we will
    // have to convert it to time.
    IntervalInformation delT;
    delT.Unit = TIME_UNIT;
    delT.Interval = 0;
    IntervalInformation aStep;
    aStep.Unit = this->MaximumPropagation.Unit;
    double propagation = 0.0, step, minStep=0, maxStep=0;
    double stepTaken, accumTime=0;
    double speed;
    double cellLength;
    int retVal = OUT_OF_TIME, tmp;

    // Make sure we use the dataset found
    // by the vtkGenericInterpolatedVelocityField
    input = func->GetLastDataSet();

    inVectors=input->GetAttributes()->GetAttribute(input->GetAttributes()->FindAttribute(this->InputVectorsSelection));

    // Convert intervals to time unit
    cell=func->GetLastCell();
    cellLength = sqrt(static_cast<double>(cell->GetLength2()));
    speed = vtkMath::Norm(velocity);

    // Never call conversion methods if speed == 0
    if (speed != 0.0)
      {
      this->ConvertIntervals(delT.Interval, minStep, maxStep, direction,
                             cellLength, speed);
      }

    // Interpolate all point attributes on first point
    func->GetLastLocalCoordinates(pcoords);
    cell->InterpolateTuple(input->GetAttributes(),pcoords,
                           values);

    double *p = values;
    vtkDataArray *dataArray;
    c = outputPD->GetNumberOfArrays();
    int j;
    for(j = 0; j<c; ++j)
      {
      dataArray=outputPD->GetArray(j);
      dataArray->InsertTuple(nextPoint,p);
      p += dataArray->GetNumberOfComponents();
      }

    // Compute vorticity if required
    // This can be used later for streamribbon generation.
    if (this->ComputeVorticity)
      {
      // Here, we're assuming a linear cell by only taken values at
      // corner points. there should be a subdivision step here instead.
      // What is the criterium to stop the subdivision?
      // Note: the original vtkStreamTracer is taking cell points, it means
      // that for the quadratic cell, the standard stream tracer is more
      // accurate than this one!
      vtkGenericStreamTracer::CalculateVorticity(cell, pcoords, inVectors,
                                                 vort);

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

    vtkIdType numSteps = 0;
    double error = 0;
    // Integrate until the maximum propagation length is reached,
    // maximum number of steps is reached or until a boundary is encountered.
    // Begin Integration
    while ( propagation < this->MaximumPropagation.Interval )
      {

      if (numSteps > this->MaximumNumberOfSteps)
        {
        retVal = OUT_OF_STEPS;
        break;
        }

      if ( numSteps++ % 1000 == 1 )
        {
        progress =
          (currentLine + propagation / this->MaximumPropagation.Interval) /
          numLines ;
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
      aStep.Interval = fabs(this->ConvertToUnit(delT,
                                                this->MaximumPropagation.Unit,
                                                cellLength, speed));
      if ( (propagation + aStep.Interval) >
           this->MaximumPropagation.Interval )
        {
        aStep.Interval = this->MaximumPropagation.Interval - propagation;
        if (delT.Interval >= 0)
          {
          delT.Interval = this->ConvertToTime(aStep, cellLength, speed);
          }
        else
          {
          delT.Interval = -1.0 * this->ConvertToTime(aStep, cellLength, speed);
          }
        maxStep = delT.Interval;
        }
      this->LastUsedTimeStep = delT.Interval;

      // Calculate the next step using the integrator provided
      // Break if the next point is out of bounds.
      if ((tmp=
           integrator->ComputeNextStep(point1, point2, 0, delT.Interval,
                                       stepTaken, minStep, maxStep,
                                       this->MaximumError, error)) != 0)
        {
        retVal = tmp;
        memcpy(lastPoint, point2, 3*sizeof(double));
        break;
        }

      accumTime += stepTaken;
      // Calculate propagation (using the same units as MaximumPropagation
      propagation += fabs(this->ConvertToUnit(delT,
                                              this->MaximumPropagation.Unit,
                                              cellLength, speed));


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

      // Make sure we use the dataset found by the vtkInterpolatedVelocityField
      input = func->GetLastDataSet();

      inVectors=input->GetAttributes()->GetAttribute(input->GetAttributes()->FindAttribute(this->InputVectorsSelection));


      // Point is valid. Insert it.
      numPts++;
      numPtsTotal++;
      nextPoint = outputPoints->InsertNextPoint(point1);
      time->InsertNextValue(accumTime);

      // Calculate cell length and speed to be used in unit conversions
      cell=func->GetLastCell();
      cellLength = sqrt(static_cast<double>(cell->GetLength2()));

      speed = vtkMath::Norm(velocity);

      // Interpolate all point attributes on current point
      func->GetLastLocalCoordinates(pcoords);
      cell->InterpolateTuple(input->GetAttributes(),pcoords,
                             values);

      p = values;
      c = outputPD->GetNumberOfArrays();
      for(j = 0; j<c; ++j)
        {
        dataArray = outputPD->GetArray(j);
        dataArray->InsertTuple(nextPoint,p);
        p += dataArray->GetNumberOfComponents();
        }

      // Compute vorticity if required
      // This can be used later for streamribbon generation.
      if (this->ComputeVorticity)
        {
        vtkGenericStreamTracer::CalculateVorticity(cell, pcoords, inVectors,
                                                 vort);

        vorticity->InsertNextTuple(vort);
        // rotation
        // angular velocity = vorticity . unit tangent ( i.e. velocity/speed )
        // rotation = sum ( angular velocity * delT )
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

      // Convert all intervals to time
      this->ConvertIntervals(step, minStep, maxStep, direction,
                             cellLength, speed);


      // If the solver is adaptive and the next time step (delT.Interval)
      // that the solver wants to use is smaller than minStep or larger
      // than maxStep, re-adjust it. This has to be done every step
      // because minStep and maxStep can change depending on the cell
      // size (unless it is specified in time units)
      if (integrator->IsAdaptive())
        {
        if (fabs(delT.Interval) < fabs(minStep))
          {
          delT.Interval = fabs(minStep) * delT.Interval/fabs(delT.Interval);
          }
        else if (fabs(delT.Interval) > fabs(maxStep))
          {
          delT.Interval = fabs(maxStep) * delT.Interval/fabs(delT.Interval);
          }
        }
      else
        {
        delT.Interval = step;
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
    }

  if (!shouldAbort)
    {
    // Create the output polyline
    output->SetPoints(outputPoints);
    outputPD->AddArray(time);
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
        this->GenerateNormals(output, 0);
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


  retVals->Delete();

  outputPoints->Delete();
  outputLines->Delete();

  time->Delete();


  integrator->Delete();

  delete[] values;

  output->Squeeze();
  return;
}

//-----------------------------------------------------------------------------
void vtkGenericStreamTracer::GenerateNormals(vtkPolyData* output,
                                             double* firstNormal)
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

      lineNormalGenerator->GenerateSlidingNormals(outputPoints,
                                                  outputLines,
                                                  normals,
                                                  firstNormal);
      lineNormalGenerator->Delete();

      int i, j;
      double normal[3], local1[3], local2[3], theta, costheta, sintheta, length;
      double velocity[3];
      normals->SetName("Normals");
      vtkDataArray* newVectors =
        outputPD->GetVectors(this->InputVectorsSelection);
      for(i=0; i<numPts; i++)
        {
        normals->GetTuple(i, normal);
        if (newVectors == NULL)
          { // This should never happen.
          vtkErrorMacro("Could not find output array.");
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


//-----------------------------------------------------------------------------
// This is used by sub-classes in certain situations. It
// does a lot less (for example, does not compute attributes)
// than Integrate.
void vtkGenericStreamTracer::SimpleIntegrate(
  double seed[3],
  double lastPoint[3],
  double delt,
  vtkGenericInterpolatedVelocityField* func)
{
  vtkIdType numSteps = 0;
  vtkIdType maxSteps = 20;
  double error = 0;
  double stepTaken;
  double point1[3], point2[3];
  double velocity[3];
  double speed;

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
    if (integrator->ComputeNextStep(point1, point2, 0, delt,
                                     stepTaken, 0, 0, 0, error) != 0)
      {
      memcpy(lastPoint, point2, 3*sizeof(double));
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

//-----------------------------------------------------------------------------
void vtkGenericStreamTracer::PrintSelf(ostream& os,
                                       vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Start position: "
     << this->StartPosition[0] << " "
     << this->StartPosition[1] << " "
     << this->StartPosition[2] << endl;
  os << indent << "Terminal speed: " << this->TerminalSpeed << endl;
  os << indent << "Maximum propagation: " << this->MaximumPropagation.Interval
     << " unit: ";
  switch (this->MaximumPropagation.Unit)
    {
    case TIME_UNIT:
      os << "time.";
      break;
    case LENGTH_UNIT:
      os << "length.";
      break;
    case CELL_LENGTH_UNIT:
      os << "cell length.";
      break;
    }
  os << endl;

  os << indent << "Min. integration step: "
     << this->MinimumIntegrationStep.Interval
     << " unit: ";
  switch (this->MinimumIntegrationStep.Unit)
    {
    case TIME_UNIT:
      os << "time.";
      break;
    case LENGTH_UNIT:
      os << "length.";
      break;
    case CELL_LENGTH_UNIT:
      os << "cell length.";
      break;
    }
  os << endl;

  os << indent << "Max. integration step: "
     << this->MaximumIntegrationStep.Interval
     << " unit: ";
  switch (this->MaximumIntegrationStep.Unit)
    {
    case TIME_UNIT:
      os << "time.";
      break;
    case LENGTH_UNIT:
      os << "length.";
      break;
    case CELL_LENGTH_UNIT:
      os << "cell length.";
      break;
    }
  os << endl;

  os << indent << "Initial integration step: "
     << this->InitialIntegrationStep.Interval
     << " unit: ";
  switch (this->InitialIntegrationStep.Unit)
    {
    case TIME_UNIT:
      os << "time.";
      break;
    case LENGTH_UNIT:
      os << "length.";
      break;
    case CELL_LENGTH_UNIT:
      os << "cell length.";
      break;
    }
  os << endl;

  os << indent << "Integration direction: ";
  switch (this->IntegrationDirection)
    {
    case FORWARD:
      os << "forward.";
      break;
    case BACKWARD:
      os << "backward.";
      break;
    }
  os << endl;

  os << indent << "Integrator: " << this->Integrator << endl;
  os << indent << "Maximum error: " << this->MaximumError << endl;
  os << indent << "Max. number of steps: " << this->MaximumNumberOfSteps
     << endl;
  os << indent << "Vorticity computation: "
     << (this->ComputeVorticity ? " On" : " Off") << endl;
  os << indent << "Rotation scale: " << this->RotationScale << endl;

  if (this->InputVectorsSelection)
    {
    os << indent << "InputVectorsSelection: " << this->InputVectorsSelection;
    }
}
//-----------------------------------------------------------------------------
void vtkGenericStreamTracer::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}
