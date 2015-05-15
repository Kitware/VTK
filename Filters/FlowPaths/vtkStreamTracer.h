/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamTracer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkStreamTracer - Streamline generator
// .SECTION Description
// vtkStreamTracer is a filter that integrates a vector field to generate
// streamlines. The integration is performed using a specified integrator,
// by default Runge-Kutta2.
//
// vtkStreamTracer produces polylines as the output, with each cell (i.e.,
// polyline) representing a streamline. The attribute values associated
// with each streamline are stored in the cell data, whereas those
// associated with streamline-points are stored in the point data.
//
// vtkStreamTracer supports forward (the default), backward, and combined
// (i.e., BOTH) integration. The length of a streamline is governed by
// specifying a maximum value either in physical arc length or in (local)
// cell length. Otherwise, the integration terminates upon exiting the
// flow field domain, or if the particle speed is reduced to a value less
// than a specified terminal speed, or when a maximum number of steps is
// completed. The specific reason for the termination is stored in a cell
// array named ReasonForTermination.
//
// Note that normalized vectors are adopted in streamline integration,
// which achieves high numerical accuracy/smoothness of flow lines that is
// particularly guaranteed for Runge-Kutta45 with adaptive step size and
// error control). In support of this feature, the underlying step size is
// ALWAYS in arc length unit (LENGTH_UNIT) while the 'real' time interval
// (virtual for steady flows) that a particle actually takes to trave in a
// single step is obtained by dividing the arc length by the LOCAL speed.
// The overall elapsed time (i.e., the life span) of the particle is the
// sum of those individual step-wise time intervals.
//
// The quality of streamline integration can be controlled by setting the
// initial integration step (InitialIntegrationStep), particularly for
// Runge-Kutta2 and Runge-Kutta4 (with a fixed step size), and in the case
// of Runge-Kutta45 (with an adaptive step size and error control) the
// minimum integration step, the maximum integration step, and the maximum
// error. These steps are in either LENGTH_UNIT or CELL_LENGTH_UNIT while
// the error is in physical arc length. For the former two integrators,
// there is a trade-off between integration speed and streamline quality.
//
// The integration time, vorticity, rotation and angular velocity are stored
// in point data arrays named "IntegrationTime", "Vorticity", "Rotation" and
// "AngularVelocity", respectively (vorticity, rotation and angular velocity
// are computed only when ComputeVorticity is on). All point data attributes
// in the source dataset are interpolated on the new streamline points.
//
// vtkStreamTracer supports integration through any type of dataset. Thus if
// the dataset contains 2D cells like polygons or triangles, the integration
// is constrained to lie on the surface defined by 2D cells.
//
// The starting point, or the so-called 'seed', of a streamline may be set
// in two different ways. Starting from global x-y-z "position" allows you
// to start a single trace at a specified x-y-z coordinate. If you specify
// a source object, traces will be generated from each point in the source
// that is inside the dataset.
//
// .SECTION See Also
// vtkRibbonFilter vtkRuledSurfaceFilter vtkInitialValueProblemSolver
// vtkRungeKutta2 vtkRungeKutta4 vtkRungeKutta45 vtkTemporalStreamTracer
// vtkAbstractInterpolatedVelocityField vtkInterpolatedVelocityField
// vtkCellLocatorInterpolatedVelocityField
//

#ifndef vtkStreamTracer_h
#define vtkStreamTracer_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#include "vtkInitialValueProblemSolver.h" // Needed for constants

class vtkCompositeDataSet;
class vtkDataArray;
class vtkDoubleArray;
class vtkExecutive;
class vtkGenericCell;
class vtkIdList;
class vtkIntArray;
class vtkAbstractInterpolatedVelocityField;

class VTKFILTERSFLOWPATHS_EXPORT vtkStreamTracer : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkStreamTracer,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object to start from position (0,0,0), with forward
  // integration, terminal speed 1.0E-12, vorticity computation on,
  // integration step size 0.5 (in cell length unit), maximum number
  // of steps 2000, using Runge-Kutta2, and maximum propagation 1.0
  // (in arc length unit).
  static vtkStreamTracer *New();

  // Description:
  // Specify the starting point (seed) of a streamline in the global
  // coordinate system. Search must be performed to find the initial cell
  // from which to start integration.
  vtkSetVector3Macro(StartPosition, double);
  vtkGetVector3Macro(StartPosition, double);

  // Description:
  // Specify the source object used to generate starting points (seeds).
  // Note that this method does not connect the pipeline. The algorithm will
  // work on the input data as it is without updating the producer of the data.
  // See SetSourceConnection for connecting the pipeline.
  void SetSourceData(vtkDataSet *source);
  vtkDataSet *GetSource();

  // Description:
  // Specify the source object used to generate starting points (seeds).
  // New style.
  void SetSourceConnection(vtkAlgorithmOutput* algOutput);

//BTX
  // The previously-supported TIME_UNIT is excluded in this current
  // enumeration definition because the underlying step size is ALWAYS in
  // arc length unit (LENGTH_UNIT) while the 'real' time interval (virtual
  // for steady flows) that a particle actually takes to trave in a single
  // step is obtained by dividing the arc length by the LOCAL speed. The
  // overall elapsed time (i.e., the life span) of the particle is the sum
  // of those individual step-wise time intervals. The arc-length-to-time
  // conversion only occurs for vorticity computation and for generating a
  // point data array named 'IntegrationTime'.
  enum Units
  {
    LENGTH_UNIT = 1,
    CELL_LENGTH_UNIT = 2
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
    OUT_OF_LENGTH = 4,
    OUT_OF_STEPS = 5,
    STAGNATION = 6
  };
//ETX

  // Description:
  // Set/get the integrator type to be used for streamline generation.
  // The object passed is not actually used but is cloned with
  // NewInstance in the process of integration  (prototype pattern).
  // The default is Runge-Kutta2. The integrator can also be changed
  // using SetIntegratorType. The recognized solvers are:
  // RUNGE_KUTTA2  = 0
  // RUNGE_KUTTA4  = 1
  // RUNGE_KUTTA45 = 2
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

  // Description:
  // Set the velocity field interpolator type to the one involving
  // a dataset point locator.
  void SetInterpolatorTypeToDataSetPointLocator();

  // Description:
  // Set the velocity field interpolator type to the one involving
  // a cell locator.
  void SetInterpolatorTypeToCellLocator();

  // Description:
  // Specify the maximum length of a streamline expressed in LENGTH_UNIT.
  vtkSetMacro(MaximumPropagation, double);
  vtkGetMacro(MaximumPropagation, double);

  // Description:
  // Specify a uniform integration step unit for MinimumIntegrationStep,
  // InitialIntegrationStep, and MaximumIntegrationStep. NOTE: The valid
  // unit is now limited to only LENGTH_UNIT (1) and CELL_LENGTH_UNIT (2),
  // EXCLUDING the previously-supported TIME_UNIT.
  void SetIntegrationStepUnit( int unit );
  int  GetIntegrationStepUnit() { return this->IntegrationStepUnit; }

  // Description:
  // Specify the Initial step size used for line integration, expressed in:
  // LENGTH_UNIT      = 1
  // CELL_LENGTH_UNIT = 2
  // (either the starting size for an adaptive integrator, e.g., RK45,
  // or the constant / fixed size for non-adaptive ones, i.e., RK2 and RK4)
  vtkSetMacro(InitialIntegrationStep, double);
  vtkGetMacro(InitialIntegrationStep, double);

  // Description:
  // Specify the Minimum step size used for line integration, expressed in:
  // LENGTH_UNIT      = 1
  // CELL_LENGTH_UNIT = 2
  // (Only valid for an adaptive integrator, e.g., RK45)
  vtkSetMacro(MinimumIntegrationStep, double);
  vtkGetMacro(MinimumIntegrationStep, double);

  // Description:
  // Specify the Maximum step size used for line integration, expressed in:
  // LENGTH_UNIT      = 1
  // CELL_LENGTH_UNIT = 2
  // (Only valid for an adaptive integrator, e.g., RK45)
  vtkSetMacro(MaximumIntegrationStep, double);
  vtkGetMacro(MaximumIntegrationStep, double);

  // Description
  // Specify the maximum error tolerated throughout streamline integration.
  vtkSetMacro(MaximumError, double);
  vtkGetMacro(MaximumError, double);

  // Description
  // Specify the maximum number of steps for integrating a streamline.
  vtkSetMacro(MaximumNumberOfSteps, vtkIdType);
  vtkGetMacro(MaximumNumberOfSteps, vtkIdType);

  // Description
  // Specify the terminal speed value, below which integration is terminated.
  vtkSetMacro(TerminalSpeed, double);
  vtkGetMacro(TerminalSpeed, double);

  // Description:
  // Set/Unset the streamlines to be computed on a surface
  vtkGetMacro(SurfaceStreamlines, bool);
  vtkSetMacro(SurfaceStreamlines, bool);
  vtkBooleanMacro(SurfaceStreamlines, bool);

//BTX
  enum
  {
    FORWARD,
    BACKWARD,
    BOTH
  };

  enum
  {
    INTERPOLATOR_WITH_DATASET_POINT_LOCATOR,
    INTERPOLATOR_WITH_CELL_LOCATOR
  };
//ETX

  // Description:
  // Specify whether the streamline is integrated in the upstream or
  // downstream direction.
  vtkSetClampMacro(IntegrationDirection, int, FORWARD, BOTH);
  vtkGetMacro(IntegrationDirection, int);
  void SetIntegrationDirectionToForward()
    {this->SetIntegrationDirection(FORWARD);};
  void SetIntegrationDirectionToBackward()
    {this->SetIntegrationDirection(BACKWARD);};
  void SetIntegrationDirectionToBoth()
    {this->SetIntegrationDirection(BOTH);};

  // Description
  // Turn on/off vorticity computation at streamline points
  // (necessary for generating proper stream-ribbons using the
  // vtkRibbonFilter.
  vtkSetMacro(ComputeVorticity, bool);
  vtkGetMacro(ComputeVorticity, bool);

  // Description
  // This can be used to scale the rate with which the streamribbons
  // twist. The default is 1.
  vtkSetMacro(RotationScale, double);
  vtkGetMacro(RotationScale, double);

  // Description:
  // The object used to interpolate the velocity field during
  // integration is of the same class as this prototype.
  void SetInterpolatorPrototype( vtkAbstractInterpolatedVelocityField * ivf );

  // Description:
  // Set the type of the velocity field interpolator to determine whether
  // vtkInterpolatedVelocityField (INTERPOLATOR_WITH_DATASET_POINT_LOCATOR) or
  // vtkCellLocatorInterpolatedVelocityField (INTERPOLATOR_WITH_CELL_LOCATOR)
  // is employed for locating cells during streamline integration. The latter
  // (adopting vtkAbstractCellLocator sub-classes such as vtkCellLocator and
  // vtkModifiedBSPTree) is more robust then the former (through vtkDataSet /
  // vtkPointSet::FindCell() coupled with vtkPointLocator).
  void SetInterpolatorType( int interpType );

protected:

  vtkStreamTracer();
  ~vtkStreamTracer();

  // Create a default executive.
  virtual vtkExecutive* CreateDefaultExecutive();

  // hide the superclass' AddInput() from the user and the compiler
  void AddInput(vtkDataObject *)
    { vtkErrorMacro( << "AddInput() must be called with a vtkDataSet not a vtkDataObject."); };

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int, vtkInformation *);

  void CalculateVorticity( vtkGenericCell* cell, double pcoords[3],
                           vtkDoubleArray* cellVectors, double vorticity[3] );
  void Integrate(vtkPointData *inputData,
                 vtkPolyData* output,
                 vtkDataArray* seedSource,
                 vtkIdList* seedIds,
                 vtkIntArray* integrationDirections,
                 double lastPoint[3],
                 vtkAbstractInterpolatedVelocityField* func,
                 int maxCellSize,
                 int vecType,
                 const char *vecFieldName,
                 double& propagation,
                 vtkIdType& numSteps);
  void SimpleIntegrate(double seed[3],
                       double lastPoint[3],
                       double stepSize,
                       vtkAbstractInterpolatedVelocityField* func);
  int CheckInputs(vtkAbstractInterpolatedVelocityField*& func,
                  int* maxCellSize);
  void GenerateNormals(vtkPolyData* output, double* firstNormal, const char *vecName);

  bool GenerateNormalsInIntegrate;

  // starting from global x-y-z position
  double StartPosition[3];

  static const double EPSILON;
  double TerminalSpeed;

  double LastUsedStepSize;

//BTX
  struct IntervalInformation
  {
    double Interval;
    int Unit;
  };

  double MaximumPropagation;
  double MinimumIntegrationStep;
  double MaximumIntegrationStep;
  double InitialIntegrationStep;

  void ConvertIntervals( double& step, double& minStep, double& maxStep,
                        int direction, double cellLength );
  static double ConvertToLength( double interval, int unit, double cellLength );
  static double ConvertToLength( IntervalInformation& interval, double cellLength );

//ETX

  int SetupOutput(vtkInformation* inInfo,
                  vtkInformation* outInfo);
  void InitializeSeeds(vtkDataArray*& seeds,
                       vtkIdList*& seedIds,
                       vtkIntArray*& integrationDirections,
                       vtkDataSet *source);

  int IntegrationStepUnit;
  int IntegrationDirection;

  // Prototype showing the integrator type to be set by the user.
  vtkInitialValueProblemSolver* Integrator;

  double MaximumError;
  vtkIdType MaximumNumberOfSteps;

  bool ComputeVorticity;
  double RotationScale;

  // Compute streamlines only on surface.
  bool SurfaceStreamlines;

  vtkAbstractInterpolatedVelocityField * InterpolatorPrototype;

  vtkCompositeDataSet* InputData;
  bool HasMatchingPointAttributes; //does the point data in the multiblocks have the same attributes?

  friend class PStreamTracerUtils;

private:
  vtkStreamTracer(const vtkStreamTracer&);  // Not implemented.
  void operator=(const vtkStreamTracer&);  // Not implemented.
};


#endif
