/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamTracer.h
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
// .NAME vtkStreamTracer - Streamline generator
// .SECTION Description
// vtkStreamTracer is a filter that integrates a vector field to generate
// streamlines. The integration is performed using the provided integrator.
// The default is second order Runge-Kutta. 
// 
// vtkStreamTracer generate polylines as output. Each cell (polyline) 
// corresponds to one streamline. The values associated with each streamline
// are stored in the cell data whereas the values associated with points
// are stored in point data.
//
// Note that vtkStreamTracer can integrate both forward and backward. 
// The length of the streamline is controlled by specifying either
// a maximum value in the units of length, cell length or elapsed time 
// (the elapsed time is the time each particle would have traveled if
// flow were steady). Otherwise, the integration terminates after exiting 
// the dataset or if the particle speed is reduced to a value less than 
// the terminal speed or when a maximum number of steps is reached. 
// The reason for the termination is stored in a cell array named 
// ReasonForTermination.
//
// The quality of integration can be controlled by setting integration
// step (InitialIntegrationStep) and in the case of adaptive solvers
// the maximum error, the minimum integration step and the maximum 
// integration step. All of these can have units of length, cell length 
// or elapsed time.
//
// The integration time, vorticity, rotation and angular velocity
// are stored in point arrays named "IntegrationTime", "Vorticity",
// "Rotation" and "AngularVelocity" respectively (vorticity, rotation
// and angular velocity are computed only when ComputeVorticity is on).
// All point attributes in the source data set are interpolated on the 
// new streamline points.
//
// vtkStreamTracer integrates through any type of dataset. As a result, if the 
// dataset contains 2D cells such as polygons or triangles, the integration is
// constrained to lie on the surface defined by the 2D cells.
//
// The starting point of traces may be defined in two different ways.
// Starting from global x-y-z "position" allows you to start a single trace
// at a specified x-y-z coordinate. If you specify a source object, 
// a trace will be generated for each point in the source that is 
// inside the dataset.
//
// .SECTION See Also
// vtkRibbonFilter vtkRuledSurfaceFilter vtkInitialValueProblemSolver 
// vtkRungeKutta2 vtkRungeKutta4 vtkRungeKutta45 

#ifndef __vtkStreamTracer_h
#define __vtkStreamTracer_h

#include "vtkDataSetToPolyDataFilter.h"
#include "vtkInitialValueProblemSolver.h"

class vtkInterpolatedVelocityField;
class vtkIdList;

class VTK_GRAPHICS_EXPORT vtkStreamTracer : public vtkDataSetToPolyDataFilter
{
public:
  vtkTypeRevisionMacro(vtkStreamTracer,vtkDataSetToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object to start from position (0,0,0), integrate forward,
  // terminal speed 1.0E-12, vorticity computation on, integration
  // step length 0.5 (unit cell length), maximum number of steps 2000,
  // using 2nd order Runge Kutta and maximum propagation 1.0 (unit length).
  static vtkStreamTracer *New();
  
  // Description:
  // Specify the start of the streamline in the global coordinate
  // system. Search must be performed to find initial cell to start
  // integration from.
  vtkSetVector3Macro(StartPosition, float);
  vtkGetVector3Macro(StartPosition, float);

  // Description:
  // Specify the source object used to generate starting points.
  void SetSource(vtkDataSet *source);
  vtkDataSet *GetSource();

//BTX
  enum Units
  {
    TIME_UNIT,
    LENGTH_UNIT,
    CELL_LENGTH_UNIT,
  };

  enum Solvers
  {
    RUNGE_KUTTA2,
    RUNGE_KUTTA4,
    RUNGE_KUTTA45,
    NONE,
    UNKNOWN
  };

  enum ReasonForTermination
  {
    OUT_OF_DOMAIN = vtkInitialValueProblemSolver::OUT_OF_DOMAIN,
    NOT_INITIALIZED = vtkInitialValueProblemSolver::NOT_INITIALIZED ,
    UNEXPECTED_VALUE = vtkInitialValueProblemSolver::UNEXPECTED_VALUE,
    OUT_OF_TIME = 4,
    OUT_OF_STEPS = 5,
    STAGNATION = 6
  };
//ETX

  // Description:
  // Set/get the integrator type to be used in the stream line
  // calculation. The object passed is not actually used but
  // is cloned with MakeObject in the process of integration 
  // (prototype pattern). The default is 2nd order Runge Kutta.
  // The integrator can also be changed using SetIntegratorType.
  // The recognized solvers are:
  // RUNGE_KUTTA2  = 0
  // RUNGE_KUTTA4  = 1
  // RUNGE_KUTTA45 = 2
  vtkSetObjectMacro ( Integrator, vtkInitialValueProblemSolver );
  vtkGetObjectMacro ( Integrator, vtkInitialValueProblemSolver );
  void SetIntegratorType(int type);
  int GetIntegratorType();

  // Description:
  // Specify the maximum length of the streamlines expressed in 
  // one of the: 
  // TIME_UNIT        = 0
  // LENGTH_UNIT      = 1
  // CELL_LENGTH_UNIT = 2
  void SetMaximumPropagation(int unit, float max);
  void SetMaximumPropagation(float max);
  void SetMaximumPropagationUnit(int unit);
  int GetMaximumPropagationUnit();
  float GetMaximumPropagation();

  // Description:
  // Specify the minimum step used in the integration expressed in 
  // one of the: 
  // TIME_UNIT        = 0
  // LENGTH_UNIT      = 1
  // CELL_LENGTH_UNIT = 2
  // Only valid when using adaptive integrators.
  void SetMinimumIntegrationStep(int unit, float step);
  void SetMinimumIntegrationStepUnit(int unit);
  void SetMinimumIntegrationStep(float step);
  int GetMinimumIntegrationStepUnit();
  float GetMinimumIntegrationStep();

  // Description:
  // Specify the maximum step used in the integration expressed in 
  // one of the: 
  // TIME_UNIT        = 0
  // LENGTH_UNIT      = 1
  // CELL_LENGTH_UNIT = 2
  // Only valid when using adaptive integrators.
  void SetMaximumIntegrationStep(int unit, float step);
  void SetMaximumIntegrationStepUnit(int unit);
  void SetMaximumIntegrationStep(float step);
  int GetMaximumIntegrationStepUnit();
  float GetMaximumIntegrationStep();

  // Description:
  // Specify the initial step used in the integration expressed in 
  // one of the: 
  // TIME_UNIT        = 0
  // LENGTH_UNIT      = 1
  // CELL_LENGTH_UNIT = 2
  // If the integrator is not adaptive, this is the actual
  // step used.
  void SetInitialIntegrationStep(int unit, float step);
  void SetInitialIntegrationStepUnit(int unit);
  void SetInitialIntegrationStep(float step);
  int GetInitialIntegrationStepUnit();
  float GetInitialIntegrationStep();

  // Description
  // Specify the maximum error in the integration. This value
  // is passed to the integrator. Therefore, it's meaning depends
  // on the integrator used. 
  vtkSetMacro(MaximumError, float);
  vtkGetMacro(MaximumError, float);

  // Description
  // Specify the maximum number of steps used in the integration.
  vtkSetMacro(MaximumNumberOfSteps, vtkIdType);
  vtkGetMacro(MaximumNumberOfSteps, vtkIdType);

  // Description
  // If at any point, the speed is below this value, the integration
  // is terminated.
  vtkSetMacro(TerminalSpeed, float);
  vtkGetMacro(TerminalSpeed, float);

//BTX
  enum
  {
    FORWARD,
    BACKWARD
  };
//ETX

  // Description:
  // Specify whether the streamtrace will be generated in the
  // upstream or downstream direction.
  vtkSetClampMacro(IntegrationDirection, int,
                   FORWARD, BACKWARD);
  vtkGetMacro(IntegrationDirection, int);


  // Description
  // Turn on/off calculation of vorticity at streamline points
  // (necessary for generating proper streamribbons using the
  // vtkRibbonFilter.
  vtkSetMacro(ComputeVorticity, int);
  vtkGetMacro(ComputeVorticity, int);
  vtkBooleanMacro(ComputeVorticity, int);

  // Description
  // This can be used to scale the rate with which the streamribbons
  // twist. The default is 1.
  vtkSetMacro(RotationScale, float);
  vtkGetMacro(RotationScale, float);

protected:

  vtkStreamTracer();
  ~vtkStreamTracer();

  void Execute();
  void CalculateVorticity( vtkGenericCell* cell, float pcoords[3],
			   vtkFloatArray* cellVectors, float vorticity[3] );
  void Integrate(vtkDataArray* seedSource, vtkIdList* seedIds);


  // starting from global x-y-z position
  float StartPosition[3];

  static const float EPSILON;
  float TerminalSpeed;

//BTX
  struct IntervalInformation
  {
    float Interval;
    int Unit;
  };

  IntervalInformation MaximumPropagation;
  IntervalInformation MinimumIntegrationStep;
  IntervalInformation MaximumIntegrationStep;
  IntervalInformation InitialIntegrationStep;

  void SetIntervalInformation(int unit, float interval,
			      IntervalInformation& currentValues);
  void SetIntervalInformation(int unit,IntervalInformation& currentValues);
  static float ConvertToTime(IntervalInformation& interval, 
			     float cellLength, float speed);
  static float ConvertToLength(IntervalInformation& interval, 
			       float cellLength, float speed);
  static float ConvertToCellLength(IntervalInformation& interval, 
				   float cellLength, float speed);
  static float ConvertToUnit(IntervalInformation& interval, int unit,
			     float cellLength, float speed);
  void ConvertIntervals(float& step, float& minStep, float& maxStep, 
			int direction, float cellLength, float speed);
//ETX


  int IntegrationDirection;

  // Prototype showing the integrator type to be set by the user.
  vtkInitialValueProblemSolver* Integrator;

  int IntegratorType;
  float MaximumError;
  vtkIdType MaximumNumberOfSteps;

  int ComputeVorticity;
  float RotationScale;
private:
  vtkStreamTracer(const vtkStreamTracer&);  // Not implemented.
  void operator=(const vtkStreamTracer&);  // Not implemented.
};


#endif


