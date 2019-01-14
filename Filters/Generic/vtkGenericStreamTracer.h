/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericStreamTracer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkGenericStreamTracer
 * @brief   Streamline generator
 *
 * vtkGenericStreamTracer is a filter that integrates a vector field to
 * generate streamlines. The integration is performed using the provided
 * integrator. The default is second order Runge-Kutta.
 *
 * vtkGenericStreamTracer generate polylines as output. Each cell (polyline)
 * corresponds to one streamline. The values associated with each streamline
 * are stored in the cell data whereas the values associated with points
 * are stored in point data.
 *
 * Note that vtkGenericStreamTracer can integrate both forward and backward.
 * The length of the streamline is controlled by specifying either
 * a maximum value in the units of length, cell length or elapsed time
 * (the elapsed time is the time each particle would have traveled if
 * flow were steady). Otherwise, the integration terminates after exiting
 * the dataset or if the particle speed is reduced to a value less than
 * the terminal speed or when a maximum number of steps is reached.
 * The reason for the termination is stored in a cell array named
 * ReasonForTermination.
 *
 * The quality of integration can be controlled by setting integration
 * step (InitialIntegrationStep) and in the case of adaptive solvers
 * the maximum error, the minimum integration step and the maximum
 * integration step. All of these can have units of length, cell length
 * or elapsed time.
 *
 * The integration time, vorticity, rotation and angular velocity
 * are stored in point arrays named "IntegrationTime", "Vorticity",
 * "Rotation" and "AngularVelocity" respectively (vorticity, rotation
 * and angular velocity are computed only when ComputeVorticity is on).
 * All point attributes in the source data set are interpolated on the
 * new streamline points.
 *
 * vtkGenericStreamTracer integrates through any type of dataset. As a result,
 * if the dataset contains 2D cells such as polygons or triangles, the
 * integration is constrained to lie on the surface defined by the 2D cells.
 *
 * The starting point of traces may be defined in two different ways.
 * Starting from global x-y-z "position" allows you to start a single trace
 * at a specified x-y-z coordinate. If you specify a source object,
 * a trace will be generated for each point in the source that is
 * inside the dataset.
 *
 * @sa
 * vtkRibbonFilter vtkRuledSurfaceFilter vtkInitialValueProblemSolver
 * vtkRungeKutta2 vtkRungeKutta4 vtkRungeKutta45
*/

#ifndef vtkGenericStreamTracer_h
#define vtkGenericStreamTracer_h

#include "vtkFiltersGenericModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#include "vtkInitialValueProblemSolver.h" // Needed for constants

class vtkDataArray;
class vtkGenericAdaptorCell;
class vtkIdList;
class vtkIntArray;
class vtkGenericInterpolatedVelocityField;
class vtkDataSet;
class vtkGenericAttribute;
class vtkGenericDataSet;

class VTKFILTERSGENERIC_EXPORT vtkGenericStreamTracer : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkGenericStreamTracer,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object to start from position (0,0,0), integrate forward,
   * terminal speed 1.0E-12, vorticity computation on, integration
   * step length 0.5 (unit cell length), maximum number of steps 2000,
   * using 2nd order Runge Kutta and maximum propagation 1.0 (unit length).
   */
  static vtkGenericStreamTracer *New();

  //@{
  /**
   * Specify the start of the streamline in the global coordinate
   * system. Search must be performed to find initial cell to start
   * integration from.
   */
  vtkSetVector3Macro(StartPosition, double);
  vtkGetVector3Macro(StartPosition, double);
  //@}

  //@{
  /**
   * Specify the source object used to generate starting points.
   */
  void SetSourceData(vtkDataSet *source);
  vtkDataSet *GetSource();
  //@}

  /**
   * Specify the source object used to generate starting points (seeds).
   * New style.
   */
  void SetSourceConnection(vtkAlgorithmOutput* algOutput);

  int FillInputPortInformation(int port, vtkInformation* info) override;

  enum Units
  {
    TIME_UNIT,
    LENGTH_UNIT,
    CELL_LENGTH_UNIT
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

  //@{
  /**
   * Set/get the integrator type to be used in the stream line
   * calculation. The object passed is not actually used but
   * is cloned with NewInstance in the process of integration
   * (prototype pattern). The default is 2nd order Runge Kutta.
   * The integrator can also be changed using SetIntegratorType.
   * The recognized solvers are:
   * RUNGE_KUTTA2  = 0
   * RUNGE_KUTTA4  = 1
   * RUNGE_KUTTA45 = 2
   */
  void SetIntegrator(vtkInitialValueProblemSolver *);
  vtkGetObjectMacro ( Integrator, vtkInitialValueProblemSolver );
  void SetIntegratorType(int type);
  int GetIntegratorType();
  void SetIntegratorTypeToRungeKutta2()
    {this->SetIntegratorType(RUNGE_KUTTA2);};
  void SetIntegratorTypeToRungeKutta4()
    {this->SetIntegratorType(RUNGE_KUTTA4);};
  void SetIntegratorTypeToRungeKutta45()
    {this->SetIntegratorType(RUNGE_KUTTA45);};
  //@}

  //@{
  /**
   * Specify the maximum length of the streamlines expressed in
   * one of the:
   * TIME_UNIT        = 0
   * LENGTH_UNIT      = 1
   * CELL_LENGTH_UNIT = 2
   */
  void SetMaximumPropagation(int unit, double max);
  void SetMaximumPropagation(double max);
  void SetMaximumPropagationUnit(int unit);
  int GetMaximumPropagationUnit();
  double GetMaximumPropagation();
  void SetMaximumPropagationUnitToTimeUnit()
    {this->SetMaximumPropagationUnit(TIME_UNIT);};
  void SetMaximumPropagationUnitToLengthUnit()
    {this->SetMaximumPropagationUnit(LENGTH_UNIT);};
  void SetMaximumPropagationUnitToCellLengthUnit()
    {this->SetMaximumPropagationUnit(CELL_LENGTH_UNIT);};
  //@}

  //@{
  /**
   * Specify the minimum step used in the integration expressed in
   * one of the:
   * TIME_UNIT        = 0
   * LENGTH_UNIT      = 1
   * CELL_LENGTH_UNIT = 2
   * Only valid when using adaptive integrators.
   */
  void SetMinimumIntegrationStep(int unit, double step);
  void SetMinimumIntegrationStepUnit(int unit);
  void SetMinimumIntegrationStep(double step);
  int GetMinimumIntegrationStepUnit();
  double GetMinimumIntegrationStep();
  void SetMinimumIntegrationStepUnitToTimeUnit()
    {this->SetMinimumIntegrationStepUnit(TIME_UNIT);};
  void SetMinimumIntegrationStepUnitToLengthUnit()
    {this->SetMinimumIntegrationStepUnit(LENGTH_UNIT);};
  void SetMinimumIntegrationStepUnitToCellLengthUnit()
    {this->SetMinimumIntegrationStepUnit(CELL_LENGTH_UNIT);};
  //@}

  //@{
  /**
   * Specify the maximum step used in the integration expressed in
   * one of the:
   * TIME_UNIT        = 0
   * LENGTH_UNIT      = 1
   * CELL_LENGTH_UNIT = 2
   * Only valid when using adaptive integrators.
   */
  void SetMaximumIntegrationStep(int unit, double step);
  void SetMaximumIntegrationStepUnit(int unit);
  void SetMaximumIntegrationStep(double step);
  int GetMaximumIntegrationStepUnit();
  double GetMaximumIntegrationStep();
  void SetMaximumIntegrationStepUnitToTimeUnit()
    {this->SetMaximumIntegrationStepUnit(TIME_UNIT);};
  void SetMaximumIntegrationStepUnitToLengthUnit()
    {this->SetMaximumIntegrationStepUnit(LENGTH_UNIT);};
  void SetMaximumIntegrationStepUnitToCellLengthUnit()
    {this->SetMaximumIntegrationStepUnit(CELL_LENGTH_UNIT);};
  //@}

  //@{
  /**
   * Specify the initial step used in the integration expressed in
   * one of the:
   * TIME_UNIT        = 0
   * LENGTH_UNIT      = 1
   * CELL_LENGTH_UNIT = 2
   * If the integrator is not adaptive, this is the actual
   * step used.
   */
  void SetInitialIntegrationStep(int unit, double step);
  void SetInitialIntegrationStepUnit(int unit);
  void SetInitialIntegrationStep(double step);
  int GetInitialIntegrationStepUnit();
  double GetInitialIntegrationStep();
  void SetInitialIntegrationStepUnitToTimeUnit()
    {this->SetInitialIntegrationStepUnit(TIME_UNIT);};
  void SetInitialIntegrationStepUnitToLengthUnit()
    {this->SetInitialIntegrationStepUnit(LENGTH_UNIT);};
  void SetInitialIntegrationStepUnitToCellLengthUnit()
    {this->SetInitialIntegrationStepUnit(CELL_LENGTH_UNIT);};
  //@}

  //@{
  /**
   * Specify the maximum error in the integration. This value
   * is passed to the integrator. Therefore, it's meaning depends
   * on the integrator used.
   */
  vtkSetMacro(MaximumError, double);
  vtkGetMacro(MaximumError, double);
  //@}

  //@{
  /**
   * Specify the maximum number of steps used in the integration.
   */
  vtkSetMacro(MaximumNumberOfSteps, vtkIdType);
  vtkGetMacro(MaximumNumberOfSteps, vtkIdType);
  //@}

  //@{
  /**
   * If at any point, the speed is below this value, the integration
   * is terminated.
   */
  vtkSetMacro(TerminalSpeed, double);
  vtkGetMacro(TerminalSpeed, double);
  //@}

  //@{
  /**
   * Simplified API to set an homogeneous unit across Min/Max/Init IntegrationStepUnit
   */
  void SetIntegrationStepUnit(int unit)
  {
    this->SetInitialIntegrationStepUnit(unit);
    this->SetMinimumIntegrationStepUnit(unit);
    this->SetMaximumIntegrationStepUnit(unit);
  }
  //@}

  enum
  {
    FORWARD,
    BACKWARD,
    BOTH
  };

  //@{
  /**
   * Specify whether the streamtrace will be generated in the
   * upstream or downstream direction.
   */
  vtkSetClampMacro(IntegrationDirection, int, FORWARD, BOTH);
  vtkGetMacro(IntegrationDirection, int);
  void SetIntegrationDirectionToForward()
    {this->SetIntegrationDirection(FORWARD);};
  void SetIntegrationDirectionToBackward()
    {this->SetIntegrationDirection(BACKWARD);};
  void SetIntegrationDirectionToBoth()
    {this->SetIntegrationDirection(BOTH);};
  //@}

  //@{
  /**
   * Turn on/off calculation of vorticity at streamline points
   * (necessary for generating proper streamribbons using the
   * vtkRibbonFilter.
   */
  vtkSetMacro(ComputeVorticity, vtkTypeBool);
  vtkGetMacro(ComputeVorticity, vtkTypeBool);
  vtkBooleanMacro(ComputeVorticity, vtkTypeBool);
  //@}

  //@{
  /**
   * This can be used to scale the rate with which the streamribbons
   * twist. The default is 1.
   */
  vtkSetMacro(RotationScale, double);
  vtkGetMacro(RotationScale, double);
  //@}

  //@{
  /**
   * If you want to generate traces using an arbitrary vector array,
   * then set its name here. By default this in nullptr and the filter will
   * use the active vector array.
   */
  vtkGetStringMacro(InputVectorsSelection);
  void SelectInputVectors(const char *fieldName)
    {this->SetInputVectorsSelection(fieldName);}
  //@}

  /**
   * Add a dataset to the list inputs
   */
  void AddInputData(vtkGenericDataSet *in);

  /**
   * The object used to interpolate the velocity field during
   * integration is of the same class as this prototype.
   */
  void SetInterpolatorPrototype(vtkGenericInterpolatedVelocityField* ivf);

protected:
  vtkGenericStreamTracer();
  ~vtkGenericStreamTracer() override;

  // hide the superclass' AddInput() from the user and the compiler
  void AddInput(vtkDataObject *)
    { vtkErrorMacro( << "AddInput() must be called with a vtkGenericDataSet not a vtkDataObject."); };

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  /**
   * Compute the vorticity at point `pcoords' in cell `cell' for the
   * vector attribute `attribute'.
   * \pre attribute_exists: attribute!=0
   * \pre  point_centered_attribute: attribute->GetCentering()==vtkPointCentered
   * \pre vector_attribute: attribute->GetType()==vtkDataSetAttributes::VECTORS);
   */
  void CalculateVorticity(vtkGenericAdaptorCell* cell,
                          double pcoords[3],
                          vtkGenericAttribute *attribute,
                          double vorticity[3]);

  void Integrate(vtkGenericDataSet *input0,
                 vtkPolyData* output,
                 vtkDataArray* seedSource,
                 vtkIdList* seedIds,
                 vtkIntArray* integrationDirections,
                 double lastPoint[3],
                 vtkGenericInterpolatedVelocityField* func);
  void SimpleIntegrate(double seed[3],
                       double lastPoint[3],
                       double delt,
                       vtkGenericInterpolatedVelocityField* func);
  int CheckInputs(vtkGenericInterpolatedVelocityField*& func,
    vtkInformationVector **inputVector);
  void GenerateNormals(vtkPolyData* output, double* firstNormal);

  int GenerateNormalsInIntegrate;

  vtkSetStringMacro(InputVectorsSelection);
  char *InputVectorsSelection;


  // starting from global x-y-z position
  double StartPosition[3];

  static const double EPSILON;
  double TerminalSpeed;

  double LastUsedTimeStep;

  struct IntervalInformation
  {
    double Interval;
    int Unit;
  };

  IntervalInformation MaximumPropagation;
  IntervalInformation MinimumIntegrationStep;
  IntervalInformation MaximumIntegrationStep;
  IntervalInformation InitialIntegrationStep;

  void SetIntervalInformation(int unit, double interval,
                              IntervalInformation& currentValues);
  void SetIntervalInformation(int unit,IntervalInformation& currentValues);
  static double ConvertToTime(IntervalInformation& interval,
                             double cellLength, double speed);
  static double ConvertToLength(IntervalInformation& interval,
                               double cellLength, double speed);
  static double ConvertToCellLength(IntervalInformation& interval,
                                   double cellLength, double speed);
  static double ConvertToUnit(IntervalInformation& interval, int unit,
                             double cellLength, double speed);
  void ConvertIntervals(double& step, double& minStep, double& maxStep,
                        int direction, double cellLength, double speed);

  void InitializeSeeds(vtkDataArray*& seeds,
                       vtkIdList*& seedIds,
                       vtkIntArray*& integrationDirections);

  int IntegrationDirection;

  // Prototype showing the integrator type to be set by the user.
  vtkInitialValueProblemSolver* Integrator;

  double MaximumError;
  vtkIdType MaximumNumberOfSteps;

  vtkTypeBool ComputeVorticity;
  double RotationScale;

  vtkGenericInterpolatedVelocityField* InterpolatorPrototype;

private:
  vtkGenericStreamTracer(const vtkGenericStreamTracer&) = delete;
  void operator=(const vtkGenericStreamTracer&) = delete;
};

#endif
