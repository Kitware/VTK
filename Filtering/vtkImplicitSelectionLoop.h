/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitSelectionLoop.h
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
// .NAME vtkImplicitSelectionLoop - implicit function for a selection loop
// .SECTION Description
// vtkImplicitSelectionLoop computes the implicit function value and
// function gradient for a irregular, cylinder-like object whose cross
// section is defined by a set of points forming a loop. The loop need
// not be convex nor its points coplanar. However, the loop must be
// non-self-intersecting when projected onto the plane defined by the
// accumulated cross product around the loop (i.e., the axis of the
// loop). (Alternatively, you can specify the normal to use.)
//
// The following procedure is used to compute the implicit function
// value for a point x. Each point of the loop is first projected onto
// the plane defined by the loop normal. This forms a polygon. Then,
// to evaluate the implicit function value, inside/outside tests are
// used to determine if x is inside the polygon, and the distance to
// the loop boundary is computed (negative values are inside the
// loop).
//
// One example application of this implicit function class is to draw a 
// loop on the surface of a mesh, and use the loop to clip or extract 
// cells from within the loop. Remember, the selection loop is "infinite" 
// in length, you can use a plane (in boolean combination) to cap the extent
// of the selection loop. Another trick is to use a connectivity filter to
// extract the closest region to a given point (i.e., one of the points used
// to define the selection loop).

// .SECTION See Also
// vtkImplicitFunction vtkImplicitBoolean vtkExtractGeometry vtkClipPolyData
// vtkConnectivityFilter vtkPolyDataConnectivityFilter

#ifndef __vtkImplicitSelectionLoop_h
#define __vtkImplicitSelectionLoop_h

#include "vtkImplicitFunction.h"
#include "vtkPolygon.h"

class VTK_EXPORT vtkImplicitSelectionLoop : public vtkImplicitFunction
{
public:
  vtkTypeMacro(vtkImplicitSelectionLoop,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate object with no initial loop.
  static vtkImplicitSelectionLoop *New();

  // Description:
  // Evaluate selection loop returning a signed distance.
  float EvaluateFunction(float x[3]);
  float EvaluateFunction(float x, float y, float z)
    {return this->vtkImplicitFunction::EvaluateFunction(x, y, z); } ;

  // Description:
  // Evaluate selection loop returning the gradient.
  void EvaluateGradient(float x[3], float n[3]);

  // Description:
  // Set/Get the array of point coordinates defining the loop. There must
  // be at least three points used to define a loop.
  vtkSetObjectMacro(Loop,vtkPoints);
  vtkGetObjectMacro(Loop,vtkPoints);

  // Description:
  // Turn on/off automatic normal generation. By default, the normal is
  // computed from the accumulated cross product of the edges. You can also
  // specify the normal to use.
  vtkSetMacro(AutomaticNormalGeneration,int);
  vtkGetMacro(AutomaticNormalGeneration,int);
  vtkBooleanMacro(AutomaticNormalGeneration,int);

  // Description:
  // Set / get the normal used to determine what is inside and what is outside.
  vtkSetVector3Macro(Normal,float);
  vtkGetVectorMacro(Normal,float,3);

  // Description:
  // Overload GetMTime() because we depend on the Loop
  unsigned long GetMTime();

protected:
  vtkImplicitSelectionLoop();
  ~vtkImplicitSelectionLoop();
  vtkImplicitSelectionLoop(const vtkImplicitSelectionLoop&);
  void operator=(const vtkImplicitSelectionLoop&);

  vtkPoints *Loop;
  float Normal[3];
  int AutomaticNormalGeneration;

private:
  void Initialize();
  vtkPolygon *Polygon;

  float Origin[3];
  float Bounds[6];
  float DeltaX;
  float DeltaY;
  float DeltaZ;

  vtkTimeStamp InitializationTime;

};

#endif


