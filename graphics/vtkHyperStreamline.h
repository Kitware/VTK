/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperStreamline.h
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

class vtkHyperArray;

class VTK_EXPORT vtkHyperStreamline : public vtkDataSetToPolyDataFilter
{
public:
  vtkTypeMacro(vtkHyperStreamline,vtkDataSetToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with initial starting position (0,0,0); integration
  // step length 0.2; step length 0.01; forward integration; terminal
  // eigenvalue 0.0; number of sides 6; radius 0.5; and logarithmic scaling
  // off.
  static vtkHyperStreamline *New();

  // Description:
  // Specify the start of the hyperstreamline in the cell coordinate system. 
  // That is, cellId and subId (if composite cell), and parametric coordinates.
  void SetStartLocation(vtkIdType cellId, int subId, float pcoords[3]);

  // Description:
  // Specify the start of the hyperstreamline in the cell coordinate system. 
  // That is, cellId and subId (if composite cell), and parametric coordinates.
  void SetStartLocation(vtkIdType cellId, int subId, float r, float s,
                        float t);

  // Description:
  // Get the starting location of the hyperstreamline in the cell coordinate
  // system. Returns the cell that the starting point is in.
  vtkIdType GetStartLocation(int& subId, float pcoords[3]);

  // Description:
  // Specify the start of the hyperstreamline in the global coordinate system. 
  // Starting from position implies that a search must be performed to find 
  // initial cell to start integration from.
  void SetStartPosition(float x[3]);

  // Description:
  // Specify the start of the hyperstreamline in the global coordinate system. 
  // Starting from position implies that a search must be performed to find 
  // initial cell to start integration from.
  void SetStartPosition(float x, float y, float z);

  // Description:
  // Get the start position of the hyperstreamline in global x-y-z coordinates.
  float *GetStartPosition();

  // Description:
  // Set / get the maximum length of the hyperstreamline expressed as absolute
  // distance (i.e., arc length) value.
  vtkSetClampMacro(MaximumPropagationDistance,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(MaximumPropagationDistance,float);

  // Description:
  // Use the major eigenvector field as the vector field through which to 
  // integrate. The major eigenvector is the eigenvector whose corresponding
  // eigenvalue is closest to positive infinity.
  void IntegrateMajorEigenvector();

  // Description:
  // Use the major eigenvector field as the vector field through which to 
  // integrate. The major eigenvector is the eigenvector whose corresponding
  // eigenvalue is between the major and minor eigenvalues.
  void IntegrateMediumEigenvector();

  // Description:
  // Use the major eigenvector field as the vector field through which to 
  // integrate. The major eigenvector is the eigenvector whose corresponding
  // eigenvalue is closest to negative infinity.
  void IntegrateMinorEigenvector();

  // Description:
  // Set / get a nominal integration step size (expressed as a fraction of
  // the size of each cell).
  vtkSetClampMacro(IntegrationStepLength,float,0.001,0.5);
  vtkGetMacro(IntegrationStepLength,float);

  // Description:
  // Set / get the length of a tube segment composing the hyperstreamline. The
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
  // Set / get the number of sides for the hyperstreamlines. At a minimum,
  // number of sides is 3.
  vtkSetClampMacro(NumberOfSides,int,3,VTK_LARGE_INTEGER);
  vtkGetMacro(NumberOfSides,int);

  // Description:
  // Set / get the initial tube radius. This is the maximum "elliptical"
  // radius at the beginning of the tube. Radius varies based on ratio of
  // eigenvalues.  Note that tube section is actually elliptical and may
  // become a point or line in cross section in some cases.
  vtkSetClampMacro(Radius,float,0.0001,VTK_LARGE_FLOAT);
  vtkGetMacro(Radius,float);

  // Description:
  // Turn on/off logarithmic scaling. If scaling is on, the log base 10
  // of the computed eigenvalues are used to scale the cross section radii.
  vtkSetMacro(LogScaling,int);
  vtkGetMacro(LogScaling,int);
  vtkBooleanMacro(LogScaling,int);

protected:
  vtkHyperStreamline();
  ~vtkHyperStreamline();
  vtkHyperStreamline(const vtkHyperStreamline&) {};
  void operator=(const vtkHyperStreamline&) {};

  // Integrate data
  void Execute();
  void BuildTube();

  // Flag indicates where streamlines start from (either position or location)
  int StartFrom;

  // Starting from cell location
  vtkIdType StartCell;
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


