/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextureMapToCylinder.h
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
// .NAME vtkTextureMapToCylinder - generate texture coordinates by mapping points to cylinder
// .SECTION Description
// vtkTextureMapToCylinder is a filter that generates 2D texture coordinates
// by mapping input dataset points onto a cylinder. The cylinder can either be
// user specified or generated automatically. (The cylinder is generated 
// automatically by computing the axis of the cylinder.)  Note that the
// generated texture coordinates for the s-coordinate ranges from (0-1) 
// (corresponding to angle of 0->360 around axis), while the mapping of 
// the t-coordinate is controlled by the projection of points along the axis.
//
// To specify a cylinder manually, you must provide two points that
// define the axis of the cylinder. The length of the axis will affect the
// t-coordinates.
// 
// A special ivar controls how the s-coordinate is generated. If PreventSeam
// is set to true, the s-texture varies from 0->1 and then 1->0 (corresponding
// to angles of 0->180 and 180->360).

// .SECTION Caveats
// Since the resulting texture s-coordinate will lie between (0,1), and the
// origin of the texture coordinates is not user-controllable, you may want
// to use the class vtkTransformTexture to linearly scale and shift the origin
// of the texture coordinates.

// .SECTION See Also
// vtkTextureMapToPlane vtkTextureMapToSphere vtkTextureMapToBox
// vtkTransformTexture vtkThresholdTextureCoords

#ifndef __vtkTextureMapToCylinder_h
#define __vtkTextureMapToCylinder_h

#include "vtkDataSetToDataSetFilter.h"

class VTK_GRAPHICS_EXPORT vtkTextureMapToCylinder : public vtkDataSetToDataSetFilter 
{
public:
  vtkTypeMacro(vtkTextureMapToCylinder,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create object with cylinder axis parallel to z-axis (points (0,0,-0.5) 
  // and (0,0,0.5)). The PreventSeam ivar is set to true. The cylinder is 
  // automatically generated.
  static vtkTextureMapToCylinder *New();

  // Description:
  // Specify the first point defining the cylinder axis,
  vtkSetVector3Macro(Point1,float);
  vtkGetVectorMacro(Point1,float,3);

  // Description:
  // Specify the second point defining the cylinder axis,
  vtkSetVector3Macro(Point2,float);
  vtkGetVectorMacro(Point2,float,3);

  // Description:
  // Turn on/off automatic cylinder generation. This means it automatically
  // finds the cylinder center and axis.
  vtkSetMacro(AutomaticCylinderGeneration,int);
  vtkGetMacro(AutomaticCylinderGeneration,int);
  vtkBooleanMacro(AutomaticCylinderGeneration,int);

  // Description:
  // Control how the texture coordinates are generated. If PreventSeam is
  // set, the s-coordinate ranges from 0->1 and 1->0 corresponding to the
  // angle variation from 0->180 and 180->0. Otherwise, the s-coordinate
  // ranges from 0->1 from 0->360 degrees.
  vtkSetMacro(PreventSeam,int);
  vtkGetMacro(PreventSeam,int);
  vtkBooleanMacro(PreventSeam,int);

protected:
  vtkTextureMapToCylinder();
  ~vtkTextureMapToCylinder() {};
  vtkTextureMapToCylinder(const vtkTextureMapToCylinder&);
  void operator=(const vtkTextureMapToCylinder&);

  void Execute();

  float Point1[3];
  float Point2[3];
  int AutomaticCylinderGeneration;
  int PreventSeam;

};

#endif


