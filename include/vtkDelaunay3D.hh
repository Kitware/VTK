/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDelaunay3D.hh
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
// .NAME vtkDelaunay3D - create 3D Delaunay triangulation of input points
// .SECTION Description

// vtkDelaunay3D is a filter that constructs a 3D Delaunay
// triangulation from a list of input points. These points may be
// represented by any dataset of type vtkPointSet and subclasses. The
// output of the filter is an unstructured grid dataset. Usually the
// output is a tetrahedral mesh, but if a non-zero alpha distance
// value is specified (called the "alpha" value), then only tetrahedra,
// triangles, edges, and vertices lying within the alpha radius are 
// output. In other words, non-zero alpha values may result in arbitrary
// combinations of tetrahedra, triangles, lines, and vertices. (The notion 
// of alpha value is derived from Edelsbrunner's work on "alpha shapes".)
// 
// The 3D Delaunay triangulation is defined as the triangulation that
// satisfies the Delaunay criterion for n-dimensional simplexes (in
// this case n=3 and the simplexes are tetrahedra). This criterion
// states that a circumsphere of each simplex in a triangulation
// contains only the n+1 defining points of the simplex. (See text for
// more information.) While in two dimensions this translates into an
// "optimal" triangulation, this is not true in 3D, since a measurement 
// for optimality in 3D is not agreed on.
//
// Delaunay triangulations are used to build topological structures
// from unorganized (or unstructured) points. The input to this filter
// is a list of points specified in 3D. (If you wish to create 2D 
// triangulations see vtkDelaunay2D.) The output is an unstructured grid.

// .SECTION Caveats
// Points arranged on a regular lattice (termed degenerate cases) can be 
// triangulated in more than one way (at least according to the Delaunay 
// criterion). The choice of triangulation (as implemented by 
// this algorithm) depends on the order of the input points. The first four
// points will form a tetrahedron; other degenerate points (relative to this
// initial tetrahedron) will not break it.
//
// Points that are coincident (or nearly so) may be discarded by the
// algorithm.  This is because the Delaunay triangulation requires
// unique input points.  You can control the definition of coincidence
// with the "Tolerance" instance variable.
//
// The output of the Delaunay triangulation is supposedly a convex hull. In 
// certain cases this implementation may not generate the convex hull. This
// behavior can be controlled by the Offset instance variable. Offset is a
// multiplier used to control the size of the initial triangulation. The 
// larger the offset value, the more likely you will generate a convex hull;
// and the more likely you are to see numerical problems.
//
// The implementation of this algorithm varies from the 2D Delaunay
// algorithm (i.e., vtkDelaunay2D) in an important way. When points are
// injected into the triangulation, the search for the enclosing tetrahedron
// is quite different. In the 3D case, the closest previously inserted point
// point is found, and then the connected tetrahedra are searched to find
// the containing one. (In 2D, a "walk" towards the enclosing triangle is
// performed.) If the triangulation is Delaunay, then an 

// .SECTION Bugs
// Current implementation (vtk1.1) can break down due to numerical degeneracy. Also is 
// slower than it should be. 

// .SECTION See Also
// vtkDelaunay2D vtkGaussianSplatter vtkUnstructuredGrid

#ifndef __vtkDelaunay3D_h
#define __vtkDelaunay3D_h

#include "vtkPointSetFilter.hh"
#include "vtkUnstructuredGrid.hh"

class vtkDelaunay3D : public vtkPointSetFilter
{
public:
  vtkDelaunay3D();
  char *GetClassName() {return "vtkDelaunay3D";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify alpha (or distance) value to control output of this filter.
  // For a non-zero alpha value, only edges or triangles contained within
  // a sphere centered at mesh vertices will be output. Otherwise, only
  // triangles will be output.
  vtkSetClampMacro(Alpha,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(Alpha,float);

  // Description:
  // Specify a tolerance to control discarding of closely spaced points.
  // This tolerance is specified as a fraction of the diagonal length of
  // the bounding box of the points.
  vtkSetClampMacro(Tolerance,float,0.0,1.0);
  vtkGetMacro(Tolerance,float);

  // Description:
  // Specify a multiplier to control the size of the initial, bounding
  // Delaunay triangulation.
  vtkSetClampMacro(Offset,float,2.5,VTK_LARGE_FLOAT);
  vtkGetMacro(Offset,float);

  // Description:
  // Boolean controls whether bounding triangulation points (and associated
  // triangles) are included in the output. (These are introduced as an
  // initial triangulation to begin the triangulation process. This feature
  // is nice for debugging output.)
  vtkSetMacro(BoundingTriangulation,int);
  vtkGetMacro(BoundingTriangulation,int);
  vtkBooleanMacro(BoundingTriangulation,int);

  // Description:
  // Get the output of this filter.
  vtkUnstructuredGrid *GetOutput() {return (vtkUnstructuredGrid *)this->Output;};

protected:
  void Execute();

  float Alpha;
  float Tolerance;
  int BoundingTriangulation;
  float Offset;

  vtkPointLocator Locator; //help locate points faster

};

#endif


