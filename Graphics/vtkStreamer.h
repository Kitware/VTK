/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamer.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkStreamer - abstract object implements integration of massless particle through vector field
// .SECTION Description
// vtkStreamer is a filter that integrates a massless particle through a vector
// field. The integration is performed using second order Runge-Kutta method. 
// vtkStreamer often serves as a base class for other classes that perform 
// numerical integration through a vector field (e.g., vtkStreamLine).
//
// Note that vtkStreamer can integrate both forward and backward in time,
// or in both directions. The length of the streamer is controlled by 
// specifying an elapsed time. (The elapsed time is the time each particle 
// travels.) Otherwise, the integration terminates after exiting the dataset or
// if the particle speed is reduced to a value less than the terminal speed.
//
// vtkStreamer integrates through any type of dataset. As a result, if the 
// dataset contains 2D cells such as polygons or triangles, the integration is
// constrained to lie on the surface defined by the 2D cells.
//
// The starting point of streamers may be defined in three different ways.
// Starting from global x-y-z "position" allows you to start a single streamer
// at a specified x-y-z coordinate. Starting from "location" allows you to 
// start at a specified cell, subId, and parametric coordinate. Finally, you 
// may specify a source object to start multiple streamers. If you start 
// streamers using a source object, for each point in the source that is 
// inside the dataset a streamer is created.
//
// vtkStreamer implements the integration process in the Integrate() method.
// Because vtkStreamer does not implement the Execute() method that its 
// superclass (i.e., Filter) requires, it is an abstract class. Its subclasses
// implement the execute method and use the Integrate() method, and then build
// their own representation of the integration path (i.e., lines, dashed 
// lines, points, etc.).

// .SECTION See Also
// vtkStreamLine vtkDashedStreamLine vtkStreamPoints

#ifndef __vtkStreamer_h
#define __vtkStreamer_h

#include "vtkDataSetToPolyDataFilter.h"
#include "vtkInitialValueProblemSolver.h"
#include "vtkMultiThreader.h"

#define VTK_INTEGRATE_FORWARD 0
#define VTK_INTEGRATE_BACKWARD 1
#define VTK_INTEGRATE_BOTH_DIRECTIONS 2

typedef struct _vtkStreamPoint {
  float   x[3];    // position 
  vtkIdType     cellId;  // cell
  int     subId;   // cell sub id
  float   p[3];    // parametric coords in cell 
  float   v[3];    // velocity 
  float   speed;   // velocity norm 
  float   s;       // scalar value 
  float   t;       // time travelled so far 
  float   d;       // distance travelled so far 
  float   omega;   // stream vorticity, if computed
  float   theta;    // rotation angle, if vorticity is computed
} vtkStreamPoint;

//
// Special classes for manipulating data
//
//BTX - begin tcl exclude
//
class vtkStreamArray { //;prevent man page generation
public:
  vtkStreamArray();
  ~vtkStreamArray()
    {
    if (this->Array)
      {
      delete [] this->Array;
      }
    };
  int GetNumberOfPoints() {return this->MaxId + 1;};
  vtkStreamPoint *GetStreamPoint(int i) {return this->Array + i;};
  int InsertNextStreamPoint() 
    {
    if ( ++this->MaxId >= this->Size )
      {
      this->Resize(this->MaxId);
      }
    return this->MaxId; //return offset from array
    }
  vtkStreamPoint *Resize(int sz); //reallocates data
  void Reset() {this->MaxId = -1;};

  vtkStreamPoint *Array;  // pointer to data
  int MaxId;              // maximum index inserted thus far
  int Size;               // allocated size of data
  int Extend;             // grow array by this amount
  float Direction;        // integration direction
};
//ETX
//

class VTK_EXPORT vtkStreamer : public vtkDataSetToPolyDataFilter
{
public:
  vtkTypeMacro(vtkStreamer,vtkDataSetToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object to start from position (0,0,0); integrate forward;
  // terminal speed 0.0; vorticity computation off; integrations step length
  // 0.2; and maximum propagation time 100.0.
  static vtkStreamer *New();
  
  // Description:
  // Specify the start of the streamline in the cell coordinate system. That
  // is, cellId and subId (if composite cell), and parametric coordinates.
  void SetStartLocation(int cellId, int subId, float pcoords[3]);

  // Description:
  // Specify the start of the streamline in the cell coordinate system. That
  // is, cellId and subId (if composite cell), and parametric coordinates.
  void SetStartLocation(int cellId, int subId, float r, float s, float t);

  // Description:
  // Get the starting location of the streamline in the cell coordinate system.
  int GetStartLocation(int& subId, float pcoords[3]);

  // Description:
  // Specify the start of the streamline in the global coordinate
  // system. Search must be performed to find initial cell to start
  // integration from.
  void SetStartPosition(float x[3]);

  // Description:
  // Specify the start of the streamline in the global coordinate
  // system. Search must be performed to find initial cell to start
  // integration from.
  void SetStartPosition(float x, float y, float z);

  // Description:
  // Get the start position in global x-y-z coordinates.
  float *GetStartPosition();

  // Description:
  // Specify the source object used to generate starting points.
  void SetSource(vtkDataSet *source);
  vtkDataSet *GetSource();
  
  // Description:
  // Specify the maximum length of the Streamer expressed in elapsed time.
  vtkSetClampMacro(MaximumPropagationTime,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(MaximumPropagationTime,float);

  // Description:
  // Specify the direction in which to integrate the Streamer.
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

  // Description:
  // Specify a nominal integration step size (expressed as a fraction of
  // the size of each cell). This value can be larger than 1.
  vtkSetClampMacro(IntegrationStepLength,float,0.0000001,VTK_LARGE_FLOAT);
  vtkGetMacro(IntegrationStepLength,float);

  // Description:
  // Turn on/off the creation of scalar data from velocity magnitude. If off,
  // and input dataset has scalars, input dataset scalars are used.
  vtkSetMacro(SpeedScalars,int);
  vtkGetMacro(SpeedScalars,int);
  vtkBooleanMacro(SpeedScalars,int);

  // Description:
  // Set/get terminal speed (i.e., speed is velocity magnitude).  Terminal 
  // speed is speed at which streamer will terminate propagation.
  vtkSetClampMacro(TerminalSpeed,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(TerminalSpeed,float);

  // Description:
  // Turn on/off the computation of vorticity. Vorticity is an indication of
  // the rotation of the flow. In combination with vtkStreamLine and 
  // vtkTubeFilter can be used to create rotated tubes. 
  // If vorticity is turned on, in the output, the velocity vectors 
  // are replaced by vorticity vectors.
  vtkSetMacro(Vorticity,int);
  vtkGetMacro(Vorticity,int);
  vtkBooleanMacro(Vorticity,int);

  vtkSetMacro( NumberOfThreads, int );
  vtkGetMacro( NumberOfThreads, int );

  vtkSetMacro( SavePointInterval, float );
  vtkGetMacro( SavePointInterval, float );

  // Description:
  // Set/get the integrator type to be used in the stream line
  // calculation. The object passed is not actually used but
  // is cloned with MakeObject by each thread/process in the
  // process of integration (prototype pattern). The default is 
  // 2nd order Runge Kutta.
  vtkSetObjectMacro ( Integrator, vtkInitialValueProblemSolver );
  vtkGetObjectMacro ( Integrator, vtkInitialValueProblemSolver );

protected:

  vtkStreamer();
  ~vtkStreamer();
  vtkStreamer(const vtkStreamer&) {};
  void operator=(const vtkStreamer&) {};

  // Integrate data
  void Integrate();

  // Special method for computing streamer vorticity
  void ComputeVorticity();

  // Controls where streamlines start from (either position or location).
  int StartFrom;

  // Starting from cell location
  int StartCell;
  int StartSubId;
  float StartPCoords[3];

  // starting from global x-y-z position
  float StartPosition[3];

  //array of streamers
  vtkStreamArray *Streamers;
  int NumberOfStreamers;

  // length of Streamer is generated by time, or by MaximumSteps
  float MaximumPropagationTime;

  // integration direction
  int IntegrationDirection;

  // the length (fraction of cell size) of integration steps
  float IntegrationStepLength;

  // boolean controls whether vorticity is computed
  int Vorticity;

  // terminal propagation speed
  float TerminalSpeed;

  // boolean controls whether data scalars or velocity magnitude are used
  int SpeedScalars;

  // Prototype showing the integrator type to be set by the user.
  vtkInitialValueProblemSolver* Integrator;

  // Interval with which the stream points will be stored.
  // Useful in reducing the memory footprint. Since the initial
  // value is small, by default, it will store all/most points.
  float SavePointInterval;

  static  VTK_THREAD_RETURN_TYPE ThreadedIntegrate( void *arg );

  // Description:
  // These methods were added to allow access to these variables from the
  // threads. 
  vtkGetMacro( NumberOfStreamers, int );
  vtkStreamArray *GetStreamers() { return this->Streamers; };

  void InitializeThreadedIntegrate();
  vtkMultiThreader           *Threader;
  int                        NumberOfThreads;

};

// Description:
// Return the integration direction as a character string.
inline const char *vtkStreamer::GetIntegrationDirectionAsString(void)
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

#endif


