/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSmoothPolyFilter.h
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
// .NAME vtkSmoothPolyFilter - adjust point positions using Laplacian smoothing
// .SECTION Description
// vtkSmoothPolyFilter is a filter that adjusts point coordinates using 
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
// are modified according to an average of the connected vertices.  (An
// expansion factor is available to control the amount of displacement of
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
// NumberOfIterations is a cap on the maximum numper of smoothing passes.
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
// An excellent reference for this technique is from Gabriel Taubin. "A
// Signal Processing Approach To Fair Surface Design." Proceedings of
// SIGGRAPH '95. 

// .SECTION Caveats
// The Laplacian operation reduces high frequency information in the
// geometry of the mesh. With excessive smoothing important details may be
// lost. Enabling FeatureEdgeSmoothing helps reduce this effect, but cannot
// entirely eliminate it.
//
// The contraction factor is by default a positive value, while the expansion
// factor is a negative value. This is counter-intuitive for many people.

// .SECTION See Also
// vtkDecimate

#ifndef __vtkSmoothPolyFilter_h
#define __vtkSmoothPolyFilter_h

#include "vtkPolyToPolyFilter.h"

class VTK_EXPORT vtkSmoothPolyFilter : public vtkPolyToPolyFilter
{
public:
  vtkSmoothPolyFilter();
  vtkSmoothPolyFilter *New() {return new vtkSmoothPolyFilter;};
  char *GetClassName() {return "vtkSmoothPolyFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify a convergence criterion for the iteration process. Smaller numbers result
  // in more smoothing iterations.
  vtkSetClampMacro(Convergence,float,0.0,1.0);
  vtkGetMacro(Convergence,float);

  // Description:
  // Specify the number of iterations for Laplacian smoothing,
  vtkSetClampMacro(NumberOfIterations,int,0,VTK_LARGE_INTEGER);
  vtkGetMacro(NumberOfIterations,int);

  // Description:
  // Specify the contraction factor for Laplacian smoothing, Contraction
  // pass will only occur is this is a non-zero value.
  vtkSetMacro(ContractionFactor,float);
  vtkGetMacro(ContractionFactor,float);

  // Description:
  // Specify the expansion factor for Laplacian smoothing, If this term is
  // non-zero, then the contraction pass is followed by a expansion pass. 
  // Using contraction in combination with expansion tends to preserve the 
  // original "volume" of the mesh.
  vtkSetMacro(ExpansionFactor,float);
  vtkGetMacro(ExpansionFactor,float);

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

protected:
  void Execute();

  float Convergence;
  int NumberOfIterations;
  float ExpansionFactor;
  float ContractionFactor;
  int FeatureEdgeSmoothing;
  float FeatureAngle;
  float EdgeAngle;
  int BoundarySmoothing;
  int GenerateErrorScalars;
  int GenerateErrorVectors;

};


#endif


