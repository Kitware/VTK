/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkStreamer
 * @brief   abstract object implements integration of massless particle through vector field
 *
 * vtkStreamer is a filter that integrates a massless particle through a vector
 * field. The integration is performed using second order Runge-Kutta method.
 * vtkStreamer often serves as a base class for other classes that perform
 * numerical integration through a vector field (e.g., vtkStreamLine).
 *
 * Note that vtkStreamer can integrate both forward and backward in time,
 * or in both directions. The length of the streamer is controlled by
 * specifying an elapsed time. (The elapsed time is the time each particle
 * travels.) Otherwise, the integration terminates after exiting the dataset or
 * if the particle speed is reduced to a value less than the terminal speed.
 *
 * vtkStreamer integrates through any type of dataset. As a result, if the
 * dataset contains 2D cells such as polygons or triangles, the integration is
 * constrained to lie on the surface defined by the 2D cells.
 *
 * The starting point of streamers may be defined in three different ways.
 * Starting from global x-y-z "position" allows you to start a single streamer
 * at a specified x-y-z coordinate. Starting from "location" allows you to
 * start at a specified cell, subId, and parametric coordinate. Finally, you
 * may specify a source object to start multiple streamers. If you start
 * streamers using a source object, for each point in the source that is
 * inside the dataset a streamer is created.
 *
 * vtkStreamer implements the integration process in the Integrate() method.
 * Because vtkStreamer does not implement the Execute() method that its
 * superclass (i.e., Filter) requires, it is an abstract class. Its subclasses
 * implement the execute method and use the Integrate() method, and then build
 * their own representation of the integration path (i.e., lines, dashed
 * lines, points, etc.).
 *
 * @sa
 * vtkStreamLine vtkDashedStreamLine vtkStreamPoints
*/

#ifndef vtkStreamer_h
#define vtkStreamer_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkInitialValueProblemSolver;
class vtkMultiThreader;

#ifndef VTK_LEGACY_REMOVE

#define VTK_INTEGRATE_FORWARD 0
#define VTK_INTEGRATE_BACKWARD 1
#define VTK_INTEGRATE_BOTH_DIRECTIONS 2

class VTKFILTERSFLOWPATHS_EXPORT vtkStreamer : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkStreamer,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Specify the start of the streamline in the cell coordinate system. That
   * is, cellId and subId (if composite cell), and parametric coordinates.
   */
  void SetStartLocation(vtkIdType cellId, int subId, double pcoords[3]);

  /**
   * Specify the start of the streamline in the cell coordinate system. That
   * is, cellId and subId (if composite cell), and parametric coordinates.
   */
  void SetStartLocation(vtkIdType cellId, int subId, double r, double s,
                        double t);

  /**
   * Get the starting location of the streamline in the cell coordinate system.
   */
  vtkIdType GetStartLocation(int& subId, double pcoords[3]);

  /**
   * Specify the start of the streamline in the global coordinate
   * system. Search must be performed to find initial cell to start
   * integration from.
   */
  void SetStartPosition(double x[3]);

  /**
   * Specify the start of the streamline in the global coordinate
   * system. Search must be performed to find initial cell to start
   * integration from.
   */
  void SetStartPosition(double x, double y, double z);

  /**
   * Get the start position in global x-y-z coordinates.
   */
  double *GetStartPosition();

  //@{
  /**
   * Specify the source object used to generate starting points.
   */
  void SetSourceData(vtkDataSet *source);
  vtkDataSet *GetSource();
  //@}

  /**
   * Specify the source object used to generate starting points
   * by making a pipeline connection
   */
  void SetSourceConnection(vtkAlgorithmOutput* algOutput);

  //@{
  /**
   * Specify the maximum length of the Streamer expressed in elapsed time.
   */
  vtkSetClampMacro(MaximumPropagationTime,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(MaximumPropagationTime,double);
  //@}

  //@{
  /**
   * Specify the direction in which to integrate the Streamer.
   */
  vtkSetClampMacro(IntegrationDirection,int,
                   VTK_INTEGRATE_FORWARD,VTK_INTEGRATE_BOTH_DIRECTIONS);
  vtkGetMacro(IntegrationDirection,int);
  void SetIntegrationDirectionToForward()
    {this->SetIntegrationDirection(VTK_INTEGRATE_FORWARD);};
  void SetIntegrationDirectionToBackward()
    {this->SetIntegrationDirection(VTK_INTEGRATE_BACKWARD);};
  void SetIntegrationDirectionToIntegrateBothDirections()
    {this->SetIntegrationDirection(VTK_INTEGRATE_BOTH_DIRECTIONS);};
  const char *GetIntegrationDirectionAsString();
  //@}

  //@{
  /**
   * Specify a nominal integration step size (expressed as a fraction of
   * the size of each cell). This value can be larger than 1.
   */
  vtkSetClampMacro(IntegrationStepLength,double,0.0000001,VTK_DOUBLE_MAX);
  vtkGetMacro(IntegrationStepLength,double);
  //@}

  //@{
  /**
   * Turn on/off the creation of scalar data from velocity magnitude. If off,
   * and input dataset has scalars, input dataset scalars are used.
   */
  vtkSetMacro(SpeedScalars,int);
  vtkGetMacro(SpeedScalars,int);
  vtkBooleanMacro(SpeedScalars,int);
  //@}

  //@{
  /**
   * Turn on/off the creation of scalar data from vorticity information.
   * The scalar information is currently the orientation value "theta"
   * used in rotating stream tubes. If off, and input dataset has scalars,
   * then input dataset scalars are used, unless SpeedScalars is also on.
   * SpeedScalars takes precedence over OrientationScalars.
   */
  vtkSetMacro(OrientationScalars, int);
  vtkGetMacro(OrientationScalars, int);
  vtkBooleanMacro(OrientationScalars, int);
  //@}

  //@{
  /**
   * Set/get terminal speed (i.e., speed is velocity magnitude).  Terminal
   * speed is speed at which streamer will terminate propagation.
   */
  vtkSetClampMacro(TerminalSpeed,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(TerminalSpeed,double);
  //@}

  //@{
  /**
   * Turn on/off the computation of vorticity. Vorticity is an indication of
   * the rotation of the flow. In combination with vtkStreamLine and
   * vtkTubeFilter can be used to create rotated tubes.
   * If vorticity is turned on, in the output, the velocity vectors
   * are replaced by vorticity vectors.
   */
  vtkSetMacro(Vorticity,int);
  vtkGetMacro(Vorticity,int);
  vtkBooleanMacro(Vorticity,int);
  //@}

  vtkSetMacro( NumberOfThreads, int );
  vtkGetMacro( NumberOfThreads, int );

  vtkSetMacro( SavePointInterval, double );
  vtkGetMacro( SavePointInterval, double );

  //@{
  /**
   * Set/get the integrator type to be used in the stream line
   * calculation. The object passed is not actually used but
   * is cloned with NewInstance by each thread/process in the
   * process of integration (prototype pattern). The default is
   * 2nd order Runge Kutta.
   */
  void SetIntegrator(vtkInitialValueProblemSolver *);
  vtkGetObjectMacro ( Integrator, vtkInitialValueProblemSolver );
  //@}

  //@{
  /**
   * A positive value, as small as possible for numerical comparison.
   * The initial value is 1E-12.
   */
  vtkSetMacro(Epsilon,double);
  vtkGetMacro(Epsilon,double);
  //@}

protected:
  //@{
  /**
   * Construct object to start from position (0,0,0); integrate forward;
   * terminal speed 0.0; vorticity computation off; integrations step length
   * 0.2; and maximum propagation time 100.0.
   */
  vtkStreamer();
  ~vtkStreamer();
  //@}

  // Integrate data
  void Integrate(vtkDataSet *input, vtkDataSet *source);

  // Controls where streamlines start from (either position or location).
  int StartFrom;

  // Starting from cell location
  vtkIdType StartCell;
  int StartSubId;
  double StartPCoords[3];

  // starting from global x-y-z position
  double StartPosition[3];

  //
  // Special classes for manipulating data
  //
  class StreamPoint {
  public:
    double    x[3];    // position
    vtkIdType cellId;  // cell
    int       subId;   // cell sub id
    double    p[3];    // parametric coords in cell
    double    v[3];    // velocity
    double    speed;   // velocity norm
    double    s;       // scalar value
    double    t;       // time travelled so far
    double    d;       // distance travelled so far
    double    omega;   // stream vorticity, if computed
    double    theta;   // rotation angle, if vorticity is computed
  };

  class StreamArray;
  friend class StreamArray;
  class StreamArray { //;prevent man page generation
  public:
    StreamArray();
    ~StreamArray()
    {
        delete [] this->Array;
    };
    vtkIdType GetNumberOfPoints() {return this->MaxId + 1;};
    StreamPoint *GetStreamPoint(vtkIdType i) {return this->Array + i;};
    vtkIdType InsertNextStreamPoint()
    {
        if ( ++this->MaxId >= this->Size )
        {
          this->Resize(this->MaxId);
        }
        return this->MaxId; //return offset from array
    }
    StreamPoint *Resize(vtkIdType sz); //reallocates data
    void Reset() {this->MaxId = -1;};

    StreamPoint *Array;     // pointer to data
    vtkIdType MaxId;        // maximum index inserted thus far
    vtkIdType Size;         // allocated size of data
    vtkIdType Extend;       // grow array by this amount
    double Direction;       // integration direction
  };

  //

  //array of streamers
  StreamArray *Streamers;
  vtkIdType NumberOfStreamers;

  // length of Streamer is generated by time, or by MaximumSteps
  double MaximumPropagationTime;

  // integration direction
  int IntegrationDirection;

  // the length (fraction of cell size) of integration steps
  double IntegrationStepLength;

  // boolean controls whether vorticity is computed
  int Vorticity;

  // terminal propagation speed
  double TerminalSpeed;

  // boolean controls whether data scalars or velocity magnitude are used
  int SpeedScalars;

  // boolean controls whether data scalars or vorticity orientation are used
  int OrientationScalars;

  // Prototype showing the integrator type to be set by the user.
  vtkInitialValueProblemSolver* Integrator;

  // A positive value, as small as possible for numerical comparison.
  // The initial value is 1E-12.
  double Epsilon;

  // Interval with which the stream points will be stored.
  // Useful in reducing the memory footprint. Since the initial
  // value is small, by default, it will store all/most points.
  double SavePointInterval;

  static VTK_THREAD_RETURN_TYPE ThreadedIntegrate( void *arg );

  //@{
  /**
   * These methods were added to allow access to these variables from the
   * threads.
   */
  vtkGetMacro( NumberOfStreamers, vtkIdType );
  StreamArray *GetStreamers() { return this->Streamers; };
  //@}

  void InitializeThreadedIntegrate();
  vtkMultiThreader           *Threader;
  int                        NumberOfThreads;

  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkStreamer(const vtkStreamer&) VTK_DELETE_FUNCTION;
  void operator=(const vtkStreamer&) VTK_DELETE_FUNCTION;
};

//@{
/**
 * Return the integration direction as a character string.
 */
inline const char *vtkStreamer::GetIntegrationDirectionAsString()
{
  if ( this->IntegrationDirection == VTK_INTEGRATE_FORWARD )
  {
    return "IntegrateForward";
  }
  else if ( this->IntegrationDirection == VTK_INTEGRATE_BACKWARD )
  {
    return "IntegrateBackward";
  }
  else
  {
    return "IntegrateBothDirections";
  }
}
//@}

#endif // VTK_LEGACY_REMOVE
#endif
