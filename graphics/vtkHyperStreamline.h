/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperStreamline.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkHyperStreamline - generate hyperstreamline in arbitrary dataset
// .SECTION Description
// vtkHyperStreamline is a filter that integrates through a tensor field to 
// generate a hyperstreamline. The integration is along the maximum eigenvector
// and the cross section of the hyperstreamline is defined by the two other
// eigenvectors. Thus the shape of the hyperstreamline is "tube-like", with 
// the cross section being elliptical. Hyperstreamlines are used to visualize
// tensor fields.
//
// The starting point of a hyperstreamline can be defined in one of two ways. 
// First, you may specify an initial position. This is a x-y-z global 
// coordinate. The second option is to specify a starting location. This is 
// cellId, subId, and  cell parametric coordinates.
//
// The integration of the hyperstreamline occurs through the major eigenvector 
// field. IntegrationStepLength controls the step length within each cell 
// (i.e., this is the fraction of the cell length). The length of the 
// hyperstreamline is controlled by MaximumPropagationDistance. This parameter
// is the length of the hyperstreamline in units of distance. The tube itself 
// is composed of many small sub-tubes - NumberOfSides controls the number of 
// sides in the tube, and StepLength controls the length of the sub-tubes.
//
// Because hyperstreamlines are often created near regions of singularities, it
// is possible to control the scaling of the tube cross section by using a 
// logarithmic scale. Use LogScalingOn to turn this capability on. The Radius 
// value controls the initial radius of the tube.
// .SECTION See Also
// vtkTensorGlyph vtkStreamer

#ifndef __vtkHyperStreamline_h
#define __vtkHyperStreamline_h

#include "vtkDataSetToPolyDataFilter.h"

#define VTK_INTEGRATE_FORWARD 0
#define VTK_INTEGRATE_BACKWARD 1
#define VTK_INTEGRATE_BOTH_DIRECTIONS 2

//
// Special classes for manipulating data
//
//BTX - begin tcl exclude
//
class vtkHyperPoint { //;prevent man page generation
public:
    vtkHyperPoint(); // method sets up storage
    vtkHyperPoint &operator=(const vtkHyperPoint& hp); //for resizing
    
    float   x[3];    // position 
    int     cellId;  // cell
    int     subId;   // cell sub id
    float   p[3];    // parametric coords in cell 
    float   w[3];    // eigenvalues (sorted in decreasing value)
    float   *v[3];   // pointers to eigenvectors (also sorted)
    float   v0[3];   // storage for eigenvectors
    float   v1[3];
    float   v2[3];
    float   s;       // scalar value 
    float   d;       // distance travelled so far 
};

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
  int Size;              // allocated size of data
  int Extend;            // grow array by this amount
  float Direction;       // integration direction
};
//ETX - end tcl exclude
//

class VTK_EXPORT vtkHyperStreamline : public vtkDataSetToPolyDataFilter
{
public:
  vtkHyperStreamline();
  ~vtkHyperStreamline();
  static vtkHyperStreamline *New() {return new vtkHyperStreamline;};
  char *GetClassName() {return "vtkHyperStreamline";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetStartLocation(int cellId, int subId, float pcoords[3]);
  void SetStartLocation(int cellId, int subId, float r, float s, float t);
  int GetStartLocation(int& subId, float pcoords[3]);

  void SetStartPosition(float x[3]);
  void SetStartPosition(float x, float y, float z);
  float *GetStartPosition();

  // Description:
  // Specify the maximum length of the hyperstreamline expressed as absolute
  // distance (i.e., arc length) value.
  vtkSetClampMacro(MaximumPropagationDistance,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(MaximumPropagationDistance,float);

  void IntegrateMajorEigenvector();
  void IntegrateMediumEigenvector();
  void IntegrateMinorEigenvector();

  // Description:
  // Specify a nominal integration step size (expressed as a fraction of
  // the size of each cell).
  vtkSetClampMacro(IntegrationStepLength,float,0.001,0.5);
  vtkGetMacro(IntegrationStepLength,float);

  // Description:
  // Specify the length of a tube segment composing the hyperstreamline. The
  // length is specified as a fraction of the diagonal length of the input
  // bounding box.
  vtkSetClampMacro(StepLength,float,0.000001,1.0);
  vtkGetMacro(StepLength,float);

  // Description:
  // Specify the direction in which to integrate the hyperstreamline.
  vtkSetClampMacro(IntegrationDirection,int,
                  VTK_INTEGRATE_FORWARD,VTK_INTEGRATE_BOTH_DIRECTIONS);
  vtkGetMacro(IntegrationDirection,int);
  void SetIntegrationDirectionToForward()
    {this->SetIntegrationDirection(VTK_INTEGRATE_FORWARD);};
  void SetIntegrationDirectionToBackward()
    {this->SetIntegrationDirection(VTK_INTEGRATE_BACKWARD);};
  void SetIntegrationDirectionToIntegrateBothDirections()
    {this->SetIntegrationDirection(VTK_INTEGRATE_BOTH_DIRECTIONS);};

  // Description:
  // Set/get terminal eigenvalue.  If major eigenvalue falls below this
  // value, hyperstreamline terminates propagation.
  vtkSetClampMacro(TerminalEigenvalue,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(TerminalEigenvalue,float);

  // Description:
  // Set the number of sides for the hyperstreamlines. At a minimum, number 
  // of sides is 3.
  vtkSetClampMacro(NumberOfSides,int,3,VTK_LARGE_INTEGER);
  vtkGetMacro(NumberOfSides,int);

  // Description:
  // Set the initial tube radius. This is the maximum "elliptical" radius
  // at the beginning of the tube. Radius varies based on ratio of eigenvalues.
  // Note that tube section is actually elliptical and may become a point or
  // line in cross section in some cases.
  vtkSetClampMacro(Radius,float,0.0001,VTK_LARGE_FLOAT);
  vtkGetMacro(Radius,float);

  // Description:
  // Turn on/off logarithmic scaling. If scaling is on, the log base 10
  // of the computed eigenvalues are used to scale the cross section radii.
  vtkSetMacro(LogScaling,int);
  vtkGetMacro(LogScaling,int);
  vtkBooleanMacro(LogScaling,int);

protected:
  // Integrate data
  void Execute();
  void BuildTube();

  // Flag indicates where streamlines start from (either position or location)
  int StartFrom;

  // Starting from cell location
  int StartCell;
  int StartSubId;
  float StartPCoords[3];

  // starting from global x-y-z position
  float StartPosition[3];

  //array of hyperstreamlines
  vtkHyperArray *Streamers;
  int NumberOfStreamers;

  // length of hyperstreamline in absolute distance
  float MaximumPropagationDistance;

  // integration direction
  int IntegrationDirection;

  // the length (fraction of cell size) of integration steps
  float IntegrationStepLength;

  // the length of the tube segments composing the hyperstreamline
  float StepLength;

  // terminal propagation speed
  float TerminalEigenvalue;

  // number of sides of tube
  int NumberOfSides;

  // maximum radius of tube
  float Radius;

  // boolean controls whether scaling is clamped
  int LogScaling;

  // which eigenvector to use as integration vector field
  int IntegrationEigenvector;
};

#endif


