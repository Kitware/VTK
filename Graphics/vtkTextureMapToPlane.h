/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextureMapToPlane.h
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
// .NAME vtkTextureMapToPlane - generate texture coordinates by mapping points to plane
// .SECTION Description
// vtkTextureMapToPlane is a filter that generates 2D texture coordinates
// by mapping input dataset points onto a plane. The plane can either be
// user specified or generated automatically. (A least squares method is
// used to generate the plane automatically.)
//
// There are two ways you can specify the plane. The first is to provide a
// plane normal. In this case the points are projected to a plane, and the
// points are then mapped into the user specified s-t coordinate range. For
// more control, you can specify a plane with three points: an origin and two
// points defining the two axes of the plane. (This is compatible with the
// vtkPlaneSource.) Using the second method, the SRange and TRange vectors
// are ignored, since the presumption is that the user does not want to scale
// the texture coordinates; and you can adjust the origin and axes points to
// achieve the texture coordinate scaling you need. Note also that using the
// three point method the axes do not have to be orthogonal.

// .SECTION See Also
// vtkTextureMapToBox vtkPlaneSource vtkTextureMapToCylinder
// vtkTextureMapToSphere vtkThresholdTextureCoords

#ifndef __vtkTextureMapToPlane_h
#define __vtkTextureMapToPlane_h

#include "vtkDataSetToDataSetFilter.h"

class VTK_EXPORT vtkTextureMapToPlane : public vtkDataSetToDataSetFilter 
{
public:
  vtkTypeMacro(vtkTextureMapToPlane,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with s,t range=(0,1) and automatic plane generation turned on.
  static vtkTextureMapToPlane *New();

  // Description:
  // Specify a point defining the origin of the plane. Used in conjunction with
  // the Point1 and Point2 ivars to specify a map plane.
  vtkSetVector3Macro(Origin,float);
  vtkGetVectorMacro(Origin,float,3);

  // Description:
  // Specify a point defining the first axis of the plane.
  vtkSetVector3Macro(Point1,float);
  vtkGetVectorMacro(Point1,float,3);

  // Description:
  // Specify a point defining the second axis of the plane.
  vtkSetVector3Macro(Point2,float);
  vtkGetVectorMacro(Point2,float,3);

  // Description:
  // Specify plane normal. An alternative way to specify a map plane. Using
  // this method, the object will scale the resulting texture coordinate
  // between the SRange and TRange specified.
  vtkSetVector3Macro(Normal,float);
  vtkGetVectorMacro(Normal,float,3);

  // Description:
  // Specify s-coordinate range for texture s-t coordinate pair.
  vtkSetVector2Macro(SRange,float);
  vtkGetVectorMacro(SRange,float,2);

  // Description:
  // Specify t-coordinate range for texture s-t coordinate pair.
  vtkSetVector2Macro(TRange,float);
  vtkGetVectorMacro(TRange,float,2);

  // Description:
  // Turn on/off automatic plane generation.
  vtkSetMacro(AutomaticPlaneGeneration,int);
  vtkGetMacro(AutomaticPlaneGeneration,int);
  vtkBooleanMacro(AutomaticPlaneGeneration,int);

protected:
  vtkTextureMapToPlane();
  ~vtkTextureMapToPlane() {};
  vtkTextureMapToPlane(const vtkTextureMapToPlane&) {};
  void operator=(const vtkTextureMapToPlane&) {};

  void Execute();
  void ComputeNormal();

  float Origin[3];
  float Point1[3];
  float Point2[3];
  float Normal[3];
  float SRange[2];
  float TRange[2];
  int AutomaticPlaneGeneration;

};

#endif
