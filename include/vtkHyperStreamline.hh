/*=========================================================================

  Program:   Visualization Library
  Module:    vtkHyperStreamline.hh
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
// .NAME vtkHyperStreamLine - generate streamline in arbitrary dataset
// .SECTION Description
// vtkHyperStreamline is a filter that integrates 

#ifndef __vtkHyperStreamline_h
#define __vtkHyperStreamline_h

#include "vtkDataSetToPolyFilter.hh"

#define INTEGRATE_FORWARD 0
#define INTEGRATE_BACKWARD 1
#define INTEGRATE_BOTH_DIRECTIONS 2

#define START_FROM_POSITION 0
#define START_FROM_LOCATION 1

typedef struct _vtkHyperPoint {
    float   x[3];    // position 
    int     cellId;  // cell
    int     subId;   // cell sub id
    float   p[3];    // parametric coords in cell 
    float   v[3];    // velocity 
    float   speed;   // velocity norm 
    float   s;       // scalar value 
    float   t;       // time travelled so far 
    float   d;       // distance travelled so far 
} vtkHyperPoint;

//
// Special classes for manipulating data
//
//BTX - begin tcl exclude
//
class vtkHyperArray { //;prevent man page generation
public:
  vtkHyperArray();
  ~vtkHyperArray() {if (this->Array) delete [] this->Array;};
  int GetNumberOfPoints() {return this->MaxId + 1;};
  vtkHyperPoint *GetHyperPoint(int i) {return this->Array + i;};
  vtkHyperPoint *InsertNextHyperPoint() 
    {
    if ( ++this->MaxId >= this->Size ) this->Resize(this->MaxId);
    return this->Array + this->MaxId;
    }
  vtkHyperPoint *Resize(int sz); //reallocates data
  void Reset() {this->MaxId = -1;};

  vtkHyperPoint *Array;  // pointer to data
  int MaxId;             // maximum index inserted thus far
  int Size;       // allocated size of data
  int Extend;     // grow array by this amount
  float Direction;  // integration direction
};
//ETX - end tcl exclude
//

class vtkHyperStreamline : public vtkDataSetToPolyFilter
{
public:
  vtkHyperStreamline();
  char *GetClassName() {return "vtkHyperStreamline";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetStartLocation(int cellId, int subId, float pcoords[3]);
  void SetStartLocation(int cellId, int subId, float r, float s, float t);
  int GetStartLocation(int& subId, float pcoords[3]);

  void SetStartPosition(float x[3]);
  void SetStartPosition(float x, float y, float z);
  float *GetStartPosition();

  // Description:
  // Specify the maximum length of the Streamer expressed in elapsed time.
  vtkSetClampMacro(MaximumPropagationTime,float,0.0,LARGE_FLOAT);
  vtkGetMacro(MaximumPropagationTime,float);

  // Description:
  // Specify the direction in which to integrate the Streamer.
  vtkSetClampMacro(IntegrationDirection,int,
                  INTEGRATE_FORWARD,INTEGRATE_BOTH_DIRECTIONS);
  vtkGetMacro(IntegrationDirection,int);

  // Description:
  // Specify a nominal integration step size (expressed as a fraction of
  // the size of each cell).
  vtkSetClampMacro(IntegrationStepLength,float,0.001,0.5);
  vtkGetMacro(IntegrationStepLength,float);

  // Description:
  // Set/get terminal speed (i.e., speed is maximum eigenvalue).  Terminal 
  // speed is speed at which streamer will terminate propagation.
  vtkSetClampMacro(TerminalSpeed,float,0.0,LARGE_FLOAT);
  vtkGetMacro(TerminalSpeed,float);

protected:
  // Integrate data
  void Execute();
  void BuildTube();

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
  vtkHyperArray *Streamers;
  int NumberOfStreamers;

  // length of Streamer is generated by time, or by MaximumSteps
  float MaximumPropagationTime;

  // integration direction
  int IntegrationDirection;

  // the length (fraction of cell size) of integration steps
  float IntegrationStepLength;

  // terminal propagation speed
  float TerminalSpeed;

};

#endif


