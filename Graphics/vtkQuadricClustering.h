/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadricClustering.h
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
// .NAME vtkQuadricClustering - reduce the number of triangles in a mesh
// .SECTION Description
// vtkQuadricClustering is a filter to reduce the number of triangles in a
// triangle mesh, forming a good approximation to the original geometry.  The
// input to vtkQuadricClustering is a vtkPolyData object, and all types of
// polygonal data are handled.
//
// The algorithm used is the one described by Peter Lindstrom in his Siggraph
// 2000 paper, "Out-of-Core Simplification of Large Polygonal Models."  The
// general approach of the algorithm is to cluster vertices in a uniform
// binning of space, accumulating the quadric of each triangle (pushed out to
// the triangles vertices) within each bin, and then determining an optimal
// position for a single vertex in a bin by using the accumulated quadric. In
// more detail, the algorithm first gets the bounds of the input poly data.
// It then breaks this bounding volume into a user-specified number of
// spatial bins.  It then reads each triangle from the input and hashes its
// vertices into these bins.  (If this is the first time a bin has been
// visited, initialize its quadric to the 0 matrix.) The algorithm computes
// the error quadric for this triangle and adds it to the existing quadric of
// the bin in which each vertex is contained. Then, if 2 or more vertices of
// the triangle fall in the same bin, the triangle is dicarded.  If the
// triangle is not discarded, it adds the triangle to the list of output
// triangles as a list of vertex identifiers.  (There is one vertex id per
// bin.)  After all the triangles have been read, the representative vertex
// for each bin is computed (an optimal location is found) using the quadric
// for that bin.  This determines the spatial location of the vertices of
// each of the triangles in the output.
//
// To use this filter, specify the divisions defining the spatial subdivision
// in the x, y, and z directions. You must also specify an input vtkPolyData.
//
// This filter can take multiple inputs.  To do this, the user must explicity
// call StartAppend, Append (once for each input), and EndAppend.  StartAppend
// sets up the data structure to hold the quadric matrices.  Append processes
// each triangle in the input poly data it was called on, hashes its vertices
// to the appropriate bins, determines whether to keep this triangle, and
// updates the appropriate quadric matrices.  EndAppend determines the spatial
// location of each of the representative vertices for the visited bins.

// .SECTION Caveats
// This filter can drastically affect topology, i.e., topology is not 
// preserved.

// .SECTION See Also
// vtkDecimatePro vtkDecimate

#ifndef __vtkQuadricClustering_h
#define __vtkQuadricClustering_h

#include "vtkPolyDataToPolyDataFilter.h"
#include "vtkDataSetCollection.h"

class vtkFeatureEdges;

typedef struct {
  vtkIdType VertexId;
  // Dimension is supposed to be a flag representing the dimension of the cells
  // contributing to the quadric. Lines: 1, Triangles: 2 (and points 0 in the future?)
  unsigned char Dimension;
  float Quadric[9];
} VTK_POINT_QUADRIC;

class VTK_EXPORT vtkQuadricClustering : public vtkPolyDataToPolyDataFilter
{
public:
  vtkTypeMacro(vtkQuadricClustering, vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkQuadricClustering *New();

  // Description:
  // By default, this flag is off.  When "UseFeatureEdges" is on, then quadrics
  // are computed for boundary edges/feature edges.  They influence the quadrics
  // (position of points), but not the mesh.  Which features to use can be controlled 
  // by the filter "FeatureEdges".  
  vtkSetMacro(UseFeatureEdges, int);
  vtkGetMacro(UseFeatureEdges, int);
  vtkBooleanMacro(UseFeatureEdges, int);
  vtkFeatureEdges *GetFeatureEdges() {return this->FeatureEdges;}

  // Description:
  // By default, this flag is off.  It only has an effect when
  // "UseFeatureEdges" is also on.  When "UseFeaturePoints" is on, then
  // quadrics are computed for boundary / feature points used in the boundary /
  // feature edges.  They influence the quadrics (position of points), but not
  // the mesh.
  vtkSetMacro(UseFeaturePoints, int);
  vtkGetMacro(UseFeaturePoints, int);
  vtkBooleanMacro(UseFeaturePoints, int);

  // Description:
  // Set/Get the angle to use in determining whether a point on a boundary /
  // feature edge is a feature point.
  vtkSetClampMacro(FeaturePointsAngle, float, 0.0, 180.0);
  vtkGetMacro(FeaturePointsAngle, float);
  
  // Description:
  // Set/Get the number of divisions along each axis for the spatial bins.
  // The number of spatial bins is NumberOfXDivisions*NumberOfYDivisions*
  // NumberOfZDivisions.
  void SetNumberOfXDivisions(int num);
  void SetNumberOfYDivisions(int num);
  void SetNumberOfZDivisions(int num);
  vtkGetMacro(NumberOfXDivisions, int);
  vtkGetMacro(NumberOfYDivisions, int);
  vtkGetMacro(NumberOfZDivisions, int);
  void SetNumberOfDivisions(int div[3]);
  int *GetNumberOfDivisions();
  void GetNumberOfDivisions(int div[3]);

  // Description:
  // This is an alternative way to set up the bins.  If you are trying to match
  // boundaries between pieces, then you should use these methods rather than
  // SetNumberOfDivisions.
  void SetDivisionOrigin(float x, float y, float z);
  void SetDivisionOrigin(float o[3]) {this->SetDivisionOrigin(o[0],o[1],o[2]);}
  vtkGetVector3Macro(DivisionOrigin, float);
  void SetDivisionSpacing(float x, float y, float z);
  void SetDivisionSpacing(float s[3]) {this->SetDivisionSpacing(s[0],s[1],s[2]);}
  vtkGetVector3Macro(DivisionSpacing, float);

  // Description:
  // When this flag is on (and it is on by default), then triangles that are 
  // completely contained in a bin are added to the bin quadrics.  When the
  // the flag is off the filter operates faster, but the surface may not be
  // as well behaved.
  vtkSetMacro(UseInternalTriangles, int);
  vtkGetMacro(UseInternalTriangles, int);
  vtkBooleanMacro(UseInternalTriangles, int);

  // Description:
  // Normally the point that minimizes the quadric error function 
  // is used as the output of the bin.  When this flag is on,
  // the bin point is forced to be one of the points from the input
  // (the one with the smallest error). This option does not work when
  // the append methods are being called directly.
  vtkSetMacro(UseInputPoints, int);
  vtkGetMacro(UseInputPoints, int);
  vtkBooleanMacro(UseInputPoints, int);

  // Description:
  // These methods provide an alternative way of executing the filter.
  // PolyData can be added to the result in pieces (append).
  // In this mode, the user must specify the bounds of the entire model
  // as an argument to the "StartAppend" method.
  void StartAppend(float *bounds);
  void StartAppend(float x0,float x1,float y0,float y1,float z0,float z1)
    {float b[6]; b[0]=x0; b[1]=x1; b[2]=y0; b[3]=y1; b[4]=z0; b[5]=z1; 
    this->StartAppend(b);}  
  void Append(vtkPolyData *piece);
  void EndAppend();

  // Description:
  // This flag makes the filter copy cell data from input to output 
  // (the best it can).  It uses input cells that trigger the addition
  // of output cells (no averaging).  This is off by default, and does
  // not work when append is being called explicitely (non pipeline usage).
  vtkSetMacro(CopyCellData, int); 
  vtkGetMacro(CopyCellData, int); 
  vtkBooleanMacro(CopyCellData, int); 

protected:
  vtkQuadricClustering();
  ~vtkQuadricClustering();
  vtkQuadricClustering(const vtkQuadricClustering&);
  void operator=(const vtkQuadricClustering&);

  void Execute();
    
  // Description:
  // Given a point, determine what bin it falls into.
  vtkIdType HashPoint(float point[3]);
  
  // Description:
  // Determine the representative point for this bin.
  void ComputeRepresentativePoint(float quadric[9], vtkIdType binId,
				  float point[3]);

  // Description:
  // Add triangles to the quadric array.  If geometry flag is on then
  // triangles are added to the output.
  void AddTriangles(vtkCellArray *tris, vtkPoints *points,
                    int geometryFlag);
  void AddPolygons(vtkCellArray *polys, vtkPoints *points,
                   int geometryFlag);
  void AddTriangle(vtkIdType *binIds, float *pt0, float *pt1, float *pt2,
                   int geometeryFlag);

  // Description:
  // Add edges to the quadric array.  If geometry flag is on then
  // edges are added to the output.
  void AddEdges(vtkCellArray *edges, vtkPoints *points,
                int geometryFlag);
  void AddEdge(vtkIdType *binIds, float *pt0, float *pt1, int geometeryFlag);

  // Description:
  // Add vertices to the quadric array.  If geometry flag is on then
  // vertices are added to the output.
  void AddVertices(vtkCellArray *verts, vtkPoints *points,
                   int geometryFlag);
  void AddVertex(vtkIdType binId, float *pt, int geometeryFlag);

  // Description:
  // Initialize the quadric matrix to 0's.
  void InitializeQuadric(float quadric[9]);
  
  // Description:
  // Add this quadric to the quadric already associated with this bin.
  void AddQuadric(vtkIdType binId, float quadric[9]);

  // Description:
  // Find the feature points of a given set of edges.
  // The points returned are (1) those used by only one edge, (2) those
  // used by > 2 edges, and (3) those where the angle between 2 edges
  // using this point is < angle.
  void FindFeaturePoints(vtkCellArray *edges, vtkPoints *edgePts, float angle);
  
  // Description:
  // This method will rep[lace the quadric  generated points with the
  // input points with the lowest error.
  void EndAppendUsingPoints(vtkPolyData *input);
  int UseInputPoints;

  // Unfinished option to handle boundary edges differently.
  void AppendFeatureQuadrics(vtkPolyData *pd);
  int UseFeatureEdges;
  int UseFeaturePoints;
  int UseInternalTriangles;

  int NumberOfXDivisions;
  int NumberOfYDivisions;
  int NumberOfZDivisions;

  // Since there are two was of specifing the grid, we have this flag
  // to indicate which the user has set.  When this flag is on, 
  // the bin sizes are computed from the DivisionOrigin and DivisionSpacing. 
  int ComputeNumberOfDivisions;

  float DivisionOrigin[3];
  float DivisionSpacing[3];

  float Bounds[6];
  float XBinSize;
  float YBinSize;
  float ZBinSize;
  VTK_POINT_QUADRIC* QuadricArray;
  vtkIdType NumberOfBinsUsed;

  // Have to make these instance variables if we are going to allow
  // the algorithm to be driven by the Append methods.
  vtkCellArray *OutputTriangleArray;
  vtkCellArray *OutputLines;
  vtkCellArray *OutputVerts;

  vtkFeatureEdges *FeatureEdges;
  vtkPoints *FeaturePoints;
  float FeaturePointsAngle;

  int CopyCellData;
  int InCellCount;
  int OutCellCount;
};

#endif
