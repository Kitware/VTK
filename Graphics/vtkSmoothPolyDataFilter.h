/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSmoothPolyDataFilter.h
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
// .NAME vtkSmoothPolyDataFilter.h - adjust point positions using Laplacian smoothing
// .SECTION Description
// vtkSmoothPolyDataFilter.h is a filter that adjusts point coordinates using 
// Laplacian smoothing. The effect is to "relax" the mesh, making the cells 
// better shaped and the vertices more evenly distributed. Note that this
// filter operates on the lines, polygons, and triangle strips composing an
// instance of vtkPolyData. Vertex or poly-vertex cells are never modified.
// 
// The algorithm proceeds as follows. For each vertex v, a topological and
// geometric analysis is performed to determine which vertices are connected
// to v, and which cells are connected to v. Then, a connectivity array is
// constructed for each vertex. (The connectivity array is a list of lists
// of vertices that directly attach to each vertex.) Next, an iteration
// phase begins over all vertices. For each vertex v, the coordinates of v
// are modified according to an average of the connected vertices.  (A
// relaxation factor is available to control the amount of displacement of
// v).  The process repeats for each vertex. This pass over the list of
// vertices is a single iteration. Many iterations (generally around 20 or
// so) are repeated until the desired result is obtained.
// 
// There are some special instance variables used to control the execution
// of this filter. (These ivars basically control what vertices can be
// smoothed, and the creation of the connectivity array.) The
// BoundarySmoothing ivar enables/disables the smoothing operation on
// vertices that are on the "boundary" of the mesh. A boundary vertex is one
// that is surrounded by a semi-cycle of polygons (or used by a single
// line).
// 
// Another important ivar is FeatureEdgeSmoothing. If this ivar is
// enabled, then interior vertices are classified as either "simple",
// "interior edge", or "fixed", and smoothed differently. (Interior
// vertices are manifold vertices surrounded by a cycle of polygons; or used
// by two line cells.) The classification is based on the number of feature 
// edges attached to v. A feature edge occurs when the angle between the two
// surface normals of a polygon sharing an edge is greater than the
// FeatureAngle ivar. Then, vertices used by no feature edges are classified
// "simple", vertices used by exactly two feature edges are classified
// "interior edge", and all others are "fixed" vertices.
//
// Once the classification is known, the vertices are smoothed
// differently. Corner (i.e., fixed) vertices are not smoothed at all. 
// Simple vertices are smoothed as before (i.e., average of connected 
// vertex coordinates). Interior edge vertices are smoothed only along 
// their two connected edges, and only if the angle between the edges 
// is less than the EdgeAngle ivar.
//
// The total smoothing can be controlled by using two ivars. The 
// NumberOfIterations is a cap on the maximum number of smoothing passes.
// The Convergence ivar is a limit on the maximum point motion. If the 
// maximum motion during an iteration is less than Convergence, then the 
// smoothing process terminates. (Convergence is expressed as a fraction of 
// the diagonal of the bounding box.)
//
// There are two instance variables that control the generation of error
// data. If the ivar GenerateErrorScalars is on, then a scalar value indicating
// the distance of each vertex from its original position is computed. If the
// ivar GenerateErrorVectors is on, then a vector representing change in 
// position is computed.
//
// Optionally you can further control the smoothing process by defining a
// second input: the Source. If defined, the input mesh is constrained to
// lie on the surface defined by the Source ivar.
//
// .SECTION Caveats
// 
// The Laplacian operation reduces high frequency information in the geometry
// of the mesh. With excessive smoothing important details may be lost, and
// the surface may shrink towards the centroid. Enabling FeatureEdgeSmoothing
// helps reduce this effect, but cannot entirely eliminate it. You may also
// wish to try vtkWindowedSincPolyDataFilter. It does a better job of 
// minimizing shrinkage.
//
// .SECTION See Also
// vtkWindowedSincPolyDataFilter vtkDecimate vtkDecimatePro

#ifndef __vtkSmoothPolyDataFilter_h
#define __vtkSmoothPolyDataFilter_h

#include "vtkPolyDataToPolyDataFilter.h"

class vtkSmoothPoints;

class VTK_GRAPHICS_EXPORT vtkSmoothPolyDataFilter : public vtkPolyDataToPolyDataFilter
{
public:
  vtkTypeMacro(vtkSmoothPolyDataFilter,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with number of iterations 20; relaxation factor .01;
  // feature edge smoothing turned off; feature 
  // angle 45 degrees; edge angle 15 degrees; and boundary smoothing turned 
  // on. Error scalars and vectors are not generated (by default). The 
  // convergence criterion is 0.0 of the bounding box diagonal.
  static vtkSmoothPolyDataFilter *New();

  // Description:
  // Specify a convergence criterion for the iteration
  // process. Smaller numbers result in more smoothing iterations.
  vtkSetClampMacro(Convergence,float,0.0,1.0);
  vtkGetMacro(Convergence,float);

  // Description:
  // Specify the number of iterations for Laplacian smoothing,
  vtkSetClampMacro(NumberOfIterations,int,0,VTK_LARGE_INTEGER);
  vtkGetMacro(NumberOfIterations,int);

  // Description:
  // Specify the relaxation factor for Laplacian smoothing. As in all
  // iterative methods, the stability of the process is sensitive to
  // this parameter. In general, small relaxation factors and large
  // numbers of iterations are more stable than larger relaxation
  // factors and smaller numbers of iterations.
  vtkSetMacro(RelaxationFactor,float);
  vtkGetMacro(RelaxationFactor,float);

  // Description:
  // Turn on/off smoothing along sharp interior edges.
  vtkSetMacro(FeatureEdgeSmoothing,int);
  vtkGetMacro(FeatureEdgeSmoothing,int);
  vtkBooleanMacro(FeatureEdgeSmoothing,int);

  // Description:
  // Specify the feature angle for sharp edge identification.
  vtkSetClampMacro(FeatureAngle,float,0.0,180.0);
  vtkGetMacro(FeatureAngle,float);

  // Description:
  // Specify the edge angle to control smoothing along edges (either interior
  // or boundary).
  vtkSetClampMacro(EdgeAngle,float,0.0,180.0);
  vtkGetMacro(EdgeAngle,float);

  // Description:
  // Turn on/off the smoothing of vertices on the boundary of the mesh.
  vtkSetMacro(BoundarySmoothing,int);
  vtkGetMacro(BoundarySmoothing,int);
  vtkBooleanMacro(BoundarySmoothing,int);

  // Description:
  // Turn on/off the generation of scalar distance values.
  vtkSetMacro(GenerateErrorScalars,int);
  vtkGetMacro(GenerateErrorScalars,int);
  vtkBooleanMacro(GenerateErrorScalars,int);

  // Description:
  // Turn on/off the generation of error vectors.
  vtkSetMacro(GenerateErrorVectors,int);
  vtkGetMacro(GenerateErrorVectors,int);
  vtkBooleanMacro(GenerateErrorVectors,int);

  // Description:
  // Specify the source object which is used to constrain smoothing. The 
  // source defines a surface that the input (as it is smoothed) is 
  // constrained to lie upon.
  void SetSource(vtkPolyData *source);
  vtkPolyData *GetSource();
  
protected:
  vtkSmoothPolyDataFilter();
  ~vtkSmoothPolyDataFilter() {};

  void Execute();

  float Convergence;
  int NumberOfIterations;
  float RelaxationFactor;
  int FeatureEdgeSmoothing;
  float FeatureAngle;
  float EdgeAngle;
  int BoundarySmoothing;
  int GenerateErrorScalars;
  int GenerateErrorVectors;

  vtkSmoothPoints *SmoothPoints;
private:
  vtkSmoothPolyDataFilter(const vtkSmoothPolyDataFilter&);  // Not implemented.
  void operator=(const vtkSmoothPolyDataFilter&);  // Not implemented.
};

#endif
