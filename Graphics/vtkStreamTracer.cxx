/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamTracer.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStreamTracer.h"
#include "vtkInterpolatedVelocityField.h"
#include "vtkRungeKutta2.h"
#include "vtkRungeKutta4.h"
#include "vtkRungeKutta45.h"
#include "vtkMath.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"
#include "vtkPolyLine.h"

vtkCxxRevisionMacro(vtkStreamTracer, "1.4");
vtkStandardNewMacro(vtkStreamTracer);

const float vtkStreamTracer::EPSILON = 1.0E-12;

vtkStreamTracer::vtkStreamTracer()
{
  this->Integrator = vtkRungeKutta2::New();
  this->IntegrationDirection = FORWARD;
  for(int i=0; i<3; i++)
    {
    this->StartPosition[i] = 0.0;
    }
  this->MaximumPropagation.Unit = LENGTH_UNIT;
  this->MaximumPropagation.Interval = 1.0;

  this->MinimumIntegrationStep.Unit = CELL_LENGTH_UNIT;
  this->MinimumIntegrationStep.Interval = -1.0;

  this->MaximumIntegrationStep.Unit = CELL_LENGTH_UNIT;
  this->MaximumIntegrationStep.Interval = -1.0;

  this->InitialIntegrationStep.Unit = CELL_LENGTH_UNIT;
  this->InitialIntegrationStep.Interval = 0.5;

  this->MaximumError = 1.0e-5;

  this->MaximumNumberOfSteps = 2000;

  this->TerminalSpeed = EPSILON;

  this->ComputeVorticity = 1;
  this->RotationScale = 1.0;
}

vtkStreamTracer::~vtkStreamTracer()
{
  this->SetIntegrator(0);
}

void vtkStreamTracer::SetSource(vtkDataSet *source)
{
  this->vtkProcessObject::SetNthInput(1, source);
}

vtkDataSet *vtkStreamTracer::GetSource()
{
  if (this->NumberOfInputs < 2)
    {
    return NULL;
    }
  return (vtkDataSet *)(this->Inputs[1]);
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

void vtkStreamTracer::SetIntervalInformation(int unit, 
                                           vtkStreamTracer::IntervalInformation& currentValues)
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

void vtkStreamTracer::SetIntervalInformation(int unit, float interval,
                                           vtkStreamTracer::IntervalInformation& currentValues)
{
  if ( (unit == currentValues.Unit) && (interval == currentValues.Interval) )
    {
    return;
    }

  this->SetIntervalInformation(unit, currentValues);

  currentValues.Interval = interval;
  this->Modified();
}

void vtkStreamTracer::SetMaximumPropagation(int unit, float max)
{
  this->SetIntervalInformation(unit, max, this->MaximumPropagation);
}
void vtkStreamTracer::SetMaximumPropagation( float max)
{
  if ( max == this->MaximumPropagation.Interval )
    {
    return;
    }
  this->MaximumPropagation.Interval = max;
  this->Modified();
}
void vtkStreamTracer::SetMaximumPropagationUnit(int unit)
{
  this->SetIntervalInformation(unit, this->MaximumPropagation);
}
int vtkStreamTracer::GetMaximumPropagationUnit()
{
  return this->MaximumPropagation.Unit;
}
float vtkStreamTracer::GetMaximumPropagation()
{
  return this->MaximumPropagation.Interval;
}

void vtkStreamTracer::SetMinimumIntegrationStep(int unit, float step)
{
  this->SetIntervalInformation(unit, step, this->MinimumIntegrationStep);
}
void vtkStreamTracer::SetMinimumIntegrationStepUnit(int unit)
{
  this->SetIntervalInformation(unit, this->MinimumIntegrationStep);
}
void vtkStreamTracer::SetMinimumIntegrationStep(float step)
{
  if ( step == this->MinimumIntegrationStep.Interval )
    {
    return;
    }
  this->MinimumIntegrationStep.Interval = step;
  this->Modified();
}
int vtkStreamTracer::GetMinimumIntegrationStepUnit()
{
  return this->MinimumIntegrationStep.Unit;
}
float vtkStreamTracer::GetMinimumIntegrationStep()
{
  return this->MinimumIntegrationStep.Interval;
}

void vtkStreamTracer::SetMaximumIntegrationStep(int unit, float step)
{
  this->SetIntervalInformation(unit, step, this->MaximumIntegrationStep);
}
void vtkStreamTracer::SetMaximumIntegrationStepUnit(int unit)
{
  this->SetIntervalInformation(unit, this->MaximumIntegrationStep);
}
void vtkStreamTracer::SetMaximumIntegrationStep(float step)
{
  if ( step == this->MaximumIntegrationStep.Interval )
    {
    return;
    }
  this->MaximumIntegrationStep.Interval = step;
  this->Modified();
}
int vtkStreamTracer::GetMaximumIntegrationStepUnit()
{
  return this->MaximumIntegrationStep.Unit;
}
float vtkStreamTracer::GetMaximumIntegrationStep()
{
  return this->MaximumIntegrationStep.Interval;
}

void vtkStreamTracer::SetInitialIntegrationStep(int unit, float step)
{
  this->SetIntervalInformation(unit, step, this->InitialIntegrationStep);
}
void vtkStreamTracer::SetInitialIntegrationStepUnit(int unit)
{
  this->SetIntervalInformation(unit, this->InitialIntegrationStep);
}
void vtkStreamTracer::SetInitialIntegrationStep(float step)
{
  if ( step == this->InitialIntegrationStep.Interval )
    {
    return;
    }
  this->InitialIntegrationStep.Interval = step;
  this->Modified();
}
int vtkStreamTracer::GetInitialIntegrationStepUnit()
{
  return this->InitialIntegrationStep.Unit;
}
float vtkStreamTracer::GetInitialIntegrationStep()
{
  return this->InitialIntegrationStep.Interval;
}

float vtkStreamTracer::ConvertToTime(vtkStreamTracer::IntervalInformation& interval, float cellLength, float speed)
{
  float retVal = 0.0;
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

float vtkStreamTracer::ConvertToLength(vtkStreamTracer::IntervalInformation& interval, float cellLength, float speed)
{
  float retVal = 0.0;
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

float vtkStreamTracer::ConvertToCellLength(vtkStreamTracer::IntervalInformation& interval, float cellLength, float speed)
{
  float retVal = 0.0;
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

float vtkStreamTracer::ConvertToUnit(vtkStreamTracer::IntervalInformation& interval, int unit, float cellLength, float speed)
{
  float retVal = 0.0;
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

void vtkStreamTracer::ConvertIntervals(float& step, float& minStep, 
				       float& maxStep, int direction,
				       float cellLength, float speed)
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

void vtkStreamTracer::CalculateVorticity(vtkGenericCell* cell, float pcoords[3],
                                       vtkFloatArray* cellVectors, 
                                       float vorticity[3])
{
  float* cellVel;
  float derivs[9];

  cellVel = cellVectors->GetPointer(0);
  cell->Derivatives(0, pcoords, cellVel, 3, derivs);
  vorticity[0] = derivs[7] - derivs[5];
  vorticity[1] = derivs[2] - derivs[6];
  vorticity[2] = derivs[3] - derivs[1];
  
}

void vtkStreamTracer::Execute()
{
  vtkDataSet* source = this->GetSource();
  vtkIdList* seedIds = vtkIdList::New();
  if (source)
    {
    int i;
    int numSeeds = source->GetNumberOfPoints();
    if (numSeeds > 0)
      {
      // For now, one thread will do all
      seedIds->SetNumberOfIds(numSeeds);
      for (i=0; i<numSeeds; i++)
        {
        seedIds->SetId(i, i);
        }
      // Check if the source is a PointSet
      vtkPointSet* seedPts = vtkPointSet::SafeDownCast(source);
      if (seedPts)
        {
        // If it is, use it's points as source
        this->Integrate(seedPts->GetPoints()->GetData(), seedIds);
        }
      else
        {
        // Else, create a seed source
        vtkFloatArray* seeds = vtkFloatArray::New();
        seeds->SetNumberOfComponents(3);
        seeds->SetNumberOfTuples(numSeeds);
        for (i=0; i<numSeeds; i++)
          {
          seeds->SetTuple(i, source->GetPoint(i));
          }
        this->Integrate(seeds, seedIds);
        seeds->Delete();
        }
      }
    }
  else
    {
    vtkFloatArray* seed = vtkFloatArray::New();
    seed->SetNumberOfComponents(3);
    seed->InsertNextTuple(this->StartPosition);
    seedIds->InsertNextId(0);
    this->Integrate(seed, seedIds);
    seed->Delete();
    }
  seedIds->Delete();
}

void vtkStreamTracer::Integrate(vtkDataArray* seedSource, vtkIdList* seedIds)
{
  int i;
  int numLines = seedIds->GetNumberOfIds();

  // Useful pointers
  vtkPolyData* output = this->GetOutput();
  vtkDataSetAttributes* outputPD = this->GetOutput()->GetPointData();
  vtkDataSetAttributes* outputCD = this->GetOutput()->GetCellData();
  vtkPointData* inputPD  = this->GetInput()->GetPointData();
  vtkDataSet* input = this->GetInput();
  vtkDataArray* inVectors = input->GetPointData()->GetVectors();
  if (!inVectors)
    {
    vtkErrorMacro("The input does not contain a velocity vector.");
    return;
    }

  int direction=1;
  // Used in GetCell() 
  vtkGenericCell* cell = vtkGenericCell::New();
  float* weights = new float[input->GetMaxCellSize()];

  // Set the function set to be integrated
  vtkInterpolatedVelocityField* func = vtkInterpolatedVelocityField::New();
  func->SetDataSet(input);

  if (this->GetIntegrator() == 0)
    {
    vtkErrorMacro("No integrator is specified.");
    func->Delete();
    cell->Delete();
    return;
    }

  // Create a new integrator, the type is the same as Integrator
  vtkInitialValueProblemSolver* integrator = 
    this->GetIntegrator()->MakeObject();
  integrator->SetFunctionSet(func);


  // Calculate the initial velocity (to check if the first point 
  // is in bounds)
  float velocity[3], position[3];
  for (i=0; i<numLines; i++)
    {
    seedSource->GetTuple(seedIds->GetId(i), position);
    if ( !func->FunctionValues(position, velocity) )
      {
      func->Delete();
      integrator->Delete();
      cell->Delete();
      vtkWarningMacro("The initial position is not in the input data set.");
      return;
      }
    }

  // Since we do not know what the total number of points
  // will be, we do not allocate any. This is important for
  // cases where a lot of streamers are used at once. If we
  // were to allocate any points here, potentially, we can
  // waste a lot of memory if a lot of streamers are used.
  // Always insert the first point
  vtkPoints* outputPoints = vtkPoints::New();
  vtkCellArray* outputLines = vtkCellArray::New();

  // We will keep track of time in this array
  vtkFloatArray* time = vtkFloatArray::New();
  time->SetName("IntegrationTime");

  // This array explains why the integration stopped
  vtkIntArray* retVals = vtkIntArray::New();
  retVals->SetName("ReasonForTermination");

  vtkFloatArray* cellVectors = 0;
  vtkFloatArray* vorticity = 0;
  vtkFloatArray* rotation = 0;
  vtkFloatArray* angularVel = 0;
  if (this->ComputeVorticity)
    {
    cellVectors = vtkFloatArray::New();
    cellVectors->SetNumberOfComponents(3);
    cellVectors->Allocate(3*VTK_CELL_SIZE);
    
    vorticity = vtkFloatArray::New();
    vorticity->SetName("Vorticity");
    vorticity->SetNumberOfComponents(3);

    rotation = vtkFloatArray::New();
    rotation->SetName("Rotation");

    angularVel = vtkFloatArray::New();
    angularVel->SetName("AngularVelocity");
    }

  switch (this->IntegrationDirection)
    {
    case FORWARD:
      direction = 1;
      break;
    case BACKWARD:
      direction = -1;
      break;
    }

  // We will interpolate all point attributes of the input on
  // each point of the output (unless they are turned off)
  outputPD->InterpolateAllocate(inputPD);

  vtkIdType numPtsTotal=0;

  for(int currentLine = 0; currentLine < numLines; currentLine++)
    {
    // temporary variables used in the integration
    float point1[3], point2[3], pcoords[3], vort[3], omega;
    vtkIdType index, numPts=0;
    
    // Clear the last cell to avoid starting a search from
    // the last point in the streamline
    func->ClearLastCellId();

    // Initial point
    seedSource->GetTuple(seedIds->GetId(currentLine), point1);
    func->FunctionValues(point1, velocity);

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
    float propagation = 0.0, step, minStep=0, maxStep=0;
    float stepTaken, accumTime=0;
    float speed;
    double cellLength;
    int retVal=OUT_OF_TIME, tmp;

    // Convert intervals to time unit
    input->GetCell(func->GetLastCellId(), cell);
    cellLength = sqrt(static_cast<double>(cell->GetLength2()));
    speed = vtkMath::Norm(velocity);

    // Never call conversion methods if speed == 0
    if (speed != 0.0)
      {
      this->ConvertIntervals(delT.Interval, minStep, maxStep, direction, 
                             cellLength, speed);
      }

    // Interpolate all point attributes on first point
    func->GetLastWeights(weights);
    outputPD->InterpolatePoint(inputPD, nextPoint, cell->PointIds, weights);
    
    // Compute vorticity if required
    // This can be used later for streamribbon generation.
    if (this->ComputeVorticity)
      {
      inVectors->GetTuples(cell->PointIds, cellVectors);
      func->GetLastLocalCoordinates(pcoords);
      vtkStreamTracer::CalculateVorticity(cell, pcoords, cellVectors, vort);
      vorticity->InsertNextTuple(vort);
      // rotation
      // local rotation = vorticity . unit tangent ( i.e. velocity/speed )
      omega = vtkMath::Dot(vort, velocity);
      omega /= speed;
      omega *= this->RotationScale;
      angularVel->InsertNextValue(omega);
      rotation->InsertNextValue(0.0);
      }

    vtkIdType numSteps = 0;
    float error = 0, maxPropTime;
    // Integrate until the maximum propagation length is reached, 
    // maximum number of steps is reached or until a boundary is encountered.
    while ( propagation < this->MaximumPropagation.Interval )
      {
      if (numSteps > this->MaximumNumberOfSteps)
        {
        retVal = OUT_OF_STEPS;
        break;
        }
      numSteps++;

      // Never call conversion methods if speed == 0
      if ( (speed == 0) || (speed <= this->TerminalSpeed) )
        {
        retVal = STAGNATION;
        break;
        }

      // If, with the next step, propagation will be larger than
      // max, reduce it so that it is (approximately) equal to max.
      maxPropTime = this->ConvertToTime(this->MaximumPropagation, cellLength, 
                                        speed);
      if ( (accumTime + delT.Interval) > maxPropTime )
        {
        delT.Interval = maxPropTime - accumTime;
        maxStep = delT.Interval;
        }
          
      // Calculate the next step using the integrator provided
      // Break if the next point is out of bounds.
      if ((tmp=
           integrator->ComputeNextStep(point1, point2, 0, delT.Interval, 
                                       stepTaken, minStep, maxStep, 
                                       this->MaximumError, error)) != 0)
        {
        retVal = tmp;
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
        break;
        }

      // Point is valid. Insert it.
      numPts++;
      numPtsTotal++;
      nextPoint = outputPoints->InsertNextPoint(point1);
      time->InsertNextValue(accumTime);

      // Calculate cell length and speed to be used in unit conversions
      input->GetCell(func->GetLastCellId(), cell);
      cellLength = sqrt(static_cast<double>(cell->GetLength2()));
      speed = vtkMath::Norm(velocity);

      // Interpolate all point attributes on current point
      func->GetLastWeights(weights);
      outputPD->InterpolatePoint(inputPD, nextPoint, cell->PointIds, weights);

      // Compute vorticity if required
      // This can be used later for streamribbon generation.
      if (this->ComputeVorticity)
        {
        inVectors->GetTuples(cell->PointIds, cellVectors);
        func->GetLastLocalCoordinates(pcoords);
        vtkStreamTracer::CalculateVorticity(cell, pcoords, cellVectors, vort);
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
        if (delT.Interval < minStep)
          {
          delT.Interval = minStep * delT.Interval/fabs(delT.Interval);
          }
        else if (delT.Interval > maxStep)
          {
          delT.Interval = maxStep * delT.Interval/fabs(delT.Interval);
          }
        }
      else
        {
        delT.Interval = step;
        }

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

  // Create the output polyline
  output->SetPoints(outputPoints);
  outputPD->AddArray(time);
  if (vorticity)
    {
    outputPD->AddArray(vorticity);
    vorticity->Delete();
    outputPD->AddArray(rotation);
    rotation->Delete();
    outputPD->AddArray(angularVel);
    angularVel->Delete();

    }
  if (cellVectors)
    {
    cellVectors->Delete();
    }
  
  vtkIdType numPts = outputPoints->GetNumberOfPoints();
  if ( numPts > 1 )
    {

    // Assign geometry and attributes
    output->SetLines(outputLines);

    if (vorticity)
      {
      vtkPolyLine* lineNormalGenerator = vtkPolyLine::New();
      vtkFloatArray* normals = vtkFloatArray::New();
      normals->SetNumberOfComponents(3);
      normals->SetNumberOfTuples(numPts);

      lineNormalGenerator->GenerateSlidingNormals(outputPoints,
                                                  outputLines,
                                                  normals);
      lineNormalGenerator->Delete();
      
      int j;
      float normal[3], local1[3], local2[3], theta, costheta, sintheta, length;
      for(i=0; i<numPts; i++)
        {
        normals->GetTuple(i, normal);
        vtkDataArray* newVectors = outputPD->GetVectors();
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
      outputPD->SetNormals(normals);
      normals->Delete();
      }
  
    outputCD->AddArray(retVals);
    }
  retVals->Delete();

  outputPoints->Delete();
  outputLines->Delete();

  time->Delete();


  func->Delete();
  integrator->Delete();
  cell->Delete();

  delete[] weights;
  return;
}

void vtkStreamTracer::PrintSelf(ostream& os, vtkIndent indent)
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
  os << indent << "Integrator type: " << this->IntegratorType << endl;
  os << indent << "Maximum error: " << this->MaximumError << endl;
  os << indent << "Max. number of steps: " << this->MaximumNumberOfSteps 
     << endl;
  os << indent << "Vorticity computation: " 
     << (this->ComputeVorticity ? " On" : " Off") << endl;
  os << indent << "Rotation scale: " << this->RotationScale << endl;
}
