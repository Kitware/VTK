/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStreamer.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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

#include "vtkDataSetToPolyFilter.hh"

#define VTK_INTEGRATE_FORWARD 0
#define VTK_INTEGRATE_BACKWARD 1
#define VTK_INTEGRATE_BOTH_DIRECTIONS 2

#define VTK_START_FROM_POSITION 0
#define VTK_START_FROM_LOCATION 1

typedef struct _vtkStreamPoint {
    float   x[3];    // position 
    int     cellId;  // cell
    int     subId;   // cell sub id
    float   p[3];    // parametric coords in cell 
    float   v[3];    // velocity 
    float   speed;   // velocity norm 
    float   s;       // scalar value 
    float   t;       // time travelled so far 
    float   d;       // distance travelled so far 
    float   w[3];    // vorticity (if vorticity is computed)
    float   n[3];    // normal (if vorticity is computed)
} vtkStreamPoint;

//
// Special classes for manipulating data
//
//BTX - begin tcl exclude
//
class vtkStreamArray { //;prevent man page generation
public:
  vtkStreamArray();
  ~vtkStreamArray() {if (this->Array) delete [] this->Array;};
  int GetNumberOfPoints() {return this->MaxId + 1;};
  vtkStreamPoint *GetStreamPoint(int i) {return this->Array + i;};
  vtkStreamPoint *InsertNextStreamPoint() 
    {
    if ( ++this->MaxId >= this->Size ) this->Resize(this->MaxId);
    return this->Array + this->MaxId;
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

class vtkStreamer : public vtkDataSetToPolyFilter
{
public:
  vtkStreamer();
  ~vtkStreamer();
  char *GetClassName() {return "vtkStreamer";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetStartLocation(int cellId, int subId, float pcoords[3]);
  void SetStartLocation(int cellId, int subId, float r, float s, float t);
  int GetStartLocation(int& subId, float pcoords[3]);

  void SetStartPosition(float x[3]);
  void SetStartPosition(float x, float y, float z);
  float *GetStartPosition();

  void Update();

  // Description:
  // Specify the source object used to generate starting points.
  vtkSetObjectMacro(Source,vtkDataSet);
  vtkGetObjectMacro(Source,vtkDataSet);

  // Description:
  // Specify the maximum length of the Streamer expressed in elapsed time.
  vtkSetClampMacro(MaximumPropagationTime,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(MaximumPropagationTime,float);

  // Description:
  // Specify the direction in which to integrate the Streamer.
  vtkSetClampMacro(IntegrationDirection,int,
                  VTK_INTEGRATE_FORWARD,VTK_INTEGRATE_BOTH_DIRECTIONS);
  vtkGetMacro(IntegrationDirection,int);

  // Description:
  // Specify a nominal integration step size (expressed as a fraction of
  // the size of each cell).
  vtkSetClampMacro(IntegrationStepLength,float,0.001,0.5);
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
  vtkSetMacro(Vorticity,int);
  vtkGetMacro(Vorticity,int);
  vtkBooleanMacro(Vorticity,int);

protected:
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

  //points used to seed streamlines  
  vtkDataSet *Source; 

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
};

#endif


