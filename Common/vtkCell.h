/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCell.h
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
// .NAME vtkCell - abstract class to specify cell behavior
// .SECTION Description
// vtkCell is an abstract class that specifies the interfaces for data cells.
// Data cells are simple topological elements like points, lines, polygons, 
// and tetrahedra of which visualization datasets are composed. In some 
// cases visualization datasets may explicitly represent cells (e.g., 
// vtkPolyData, vtkUnstructuredGrid), and in some cases, the datasets are 
// implicitly composed of cells (e.g., vtkStructuredPoints).
//
// .SECTION Caveats
// The #define VTK_CELL_SIZE is a parameter used to construct cells and provide
// a general guideline for controlling object execution. This parameter is 
// not a hard boundary: you can create cells with more points.

// .SECTION See Also
// vtkHexahedron vtkLine vtkPixel vtkPolyLine vtkPolyVertex
// vtkPolygon vtkQuad vtkTetra vtkTriangle 
// vtkTriangleStrip vtkVertex vtkVoxel vtkWedge vtkPyramid

#ifndef __vtkCell_h
#define __vtkCell_h

#define VTK_CELL_SIZE 512
#define VTK_TOL 1.e-05 // Tolerance for geometric calculation

#include "vtkObject.h"
#include "vtkPoints.h"
#include "vtkScalars.h"
#include "vtkIdList.h"

// Include vtkCellType to include the defined cell types
#include "vtkCellType.h"

class vtkCellArray;
class vtkPointLocator;
class vtkPointData;
class vtkCellData;

class VTK_EXPORT vtkCell : public vtkObject
{
public:
  vtkTypeMacro(vtkCell,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialize cell from outside with point ids and point
  // coordinates specified.
  void Initialize(int npts, int *pts, vtkPoints *p);

  // Description:
  // Create concrete copy of this cell. Initially, the copy is made by
  // performing a ShallowCopy() operation.
  virtual vtkCell *MakeObject() = 0;

  // Description:
  // Copy this cell by reference counting the internal data structures. 
  // This is safe if you want a "read-only" copy. If you modify the cell
  // you might wish to use DeepCopy().
  virtual void ShallowCopy(vtkCell *c);

  // Description:
  // Copy this cell by completely copying internal data structures. This is
  // slower but safer than ShallowCopy().
  virtual void DeepCopy(vtkCell *c);

  // Description:
  // Return the type of cell.
  virtual int GetCellType() = 0;

  // Description:
  // Return the topological dimensional of the cell (0,1,2, or 3).
  virtual int GetCellDimension() = 0;

  // Description:
  // Return the interpolation order of the cell. Usually linear.
  virtual int GetInterpolationOrder() {return 1;};

  // Description:
  // Get the point coordinates for the cell.
  vtkPoints *GetPoints() {return this->Points;};

  // Description:
  // Return the number of points in the cell.
  int GetNumberOfPoints() {return this->PointIds->GetNumberOfIds();};

  // Description:
  // Return the number of edges in the cell.
  virtual int GetNumberOfEdges() = 0;

  // Description:
  // Return the number of faces in the cell.
  virtual int GetNumberOfFaces() = 0;

  // Description:
  // Return the list of point ids defining the cell.
  vtkIdList *GetPointIds() {return this->PointIds;};

  // Description:
  // For cell point i, return the actual point id.
  int GetPointId(int ptId) {return this->PointIds->GetId(ptId);};

  // Description:
  // Return the edge cell from the edgeId of the cell.
  virtual vtkCell *GetEdge(int edgeId) = 0;

  // Description:
  // Return the face cell from the faceId of the cell.
  virtual vtkCell *GetFace(int faceId) = 0;

  // Description:
  // Given parametric coordinates of a point, return the closest cell boundary,
  // and whether the point is inside or outside of the cell. The cell boundary 
  // is defined by a list of points (pts) that specify a face (3D cell), edge 
  // (2D cell), or vertex (1D cell). If the return value of the method is != 0, 
  // then the point is inside the cell.
  virtual int CellBoundary(int subId, float pcoords[3], vtkIdList *pts) = 0;

  // Description:
  // Given a point x[3] return inside(=1) or outside(=0) cell; evaluate 
  // parametric coordinates, sub-cell id (!=0 only if cell is composite),
  // distance squared of point x[3] to cell (in particular, the sub-cell 
  // indicated), closest point on cell to x[3] (unless closestPoint is
  // null, in which case, the closest point and dist2 are not found), and 
  // interpolation weights 
  // in cell. (The number of weights is equal to the number of points
  // defining the cell). Note: on rare occasions a -1 is returned from the 
  // method. This means that numerical error has occurred and all data 
  // returned from this method should be ignored. Also, inside/outside 
  // is determine parametrically. That is, a point is inside if it 
  // satisfies parametric limits. This can cause problems for cells of 
  // topological dimension 2 or less, since a point in 3D can project 
  // onto the cell within parametric limits but be "far" from the cell. 
  // Thus the value dist2 may be checked to determine true in/out.
  virtual int EvaluatePosition(float x[3], float* closestPoint, 
                               int& subId, float pcoords[3], 
                               float& dist2, float *weights) = 0;

  // Description:
  // Determine global coordinate (x[3]) from subId and parametric coordinates.
  // Also returns interpolation weights. (The number of weights is equal to
  // the number of points in the cell.)
  virtual void EvaluateLocation(int& subId, float pcoords[3], 
                                float x[3], float *weights) = 0;

  // Description:
  // Generate contouring primitives. The scalar list cellScalars are
  // scalar values at each cell point. The point locator is essentially a 
  // points list that merges points as they are inserted (i.e., prevents 
  // duplicates). Contouring primitives can be vertices, lines, or
  // polygons. It is possible to interpolate point data along the edge
  // by providing input and output point data - if outPd is NULL, then
  // no interpolation is performed. Also, if the output cell data is
  // non-NULL, the cell data from the contoured cell is passed to the
  // generated contouring primitives. (Note: the CopyAllocate() method
  // must be invoked on both the output cell and point data. The 
  // cellId refers to the cell from which the cell data is copied.)
  virtual void Contour(float value, vtkScalars *cellScalars, 
                       vtkPointLocator *locator, vtkCellArray *verts, 
                       vtkCellArray *lines, vtkCellArray *polys, 
                       vtkPointData *inPd, vtkPointData *outPd,
                       vtkCellData *inCd, int cellId, vtkCellData *outCd) = 0;

  // Description:
  // Cut (or clip) the cell based on the input cellScalars and the
  // specified value. The output of the clip operation will be one or
  // more cells of the same topological dimension as the original cell. 
  // The flag insideOut controls what part of the cell is considered inside - 
  // normally cell points whose scalar value is greater than "value" are
  // considered inside. If insideOut is on, this is reversed. Also, if the 
  // output cell data is non-NULL, the cell data from the clipped cell is 
  // passed to the generated contouring primitives. (Note: the CopyAllocate() 
  // method must be invoked on both the output cell and point data. The
  // cellId refers to the cell from which the cell data is copied.)
  virtual void Clip(float value, vtkScalars *cellScalars, 
                    vtkPointLocator *locator, vtkCellArray *connectivity,
                    vtkPointData *inPd, vtkPointData *outPd,
                    vtkCellData *inCd, int cellId, vtkCellData *outCd, 
                    int insideOut) = 0;

  // Description:
  // Intersect with a ray. Return parametric coordinates (both line and cell)
  // and global intersection coordinates, given ray definition and tolerance. 
  // The method returns non-zero value if intersection occurs.
  virtual int IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                                float x[3], float pcoords[3], int& subId) = 0;

  // Description:
  // Generate simplices of proper dimension. If cell is 3D, tetrahedron are 
  // generated; if 2D triangles; if 1D lines; if 0D points. The form of the
  // output is a sequence of points, each n+1 points (where n is topological 
  // cell dimension) defining a simplex. The index is a parameter that controls
  // which triangulation to use (if more than one is possible). If numerical
  // degeneracy encountered, 0 is returned, otherwise 1 is returned.
  virtual int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts) = 0;

  // Description:
  // Compute derivatives given cell subId and parametric coordinates. The
  // values array is a series of data value(s) at the cell points. There is a
  // one-to-one correspondence between cell point and data value(s). Dim is
  // the number of data values per cell point. Derivs are derivatives in the
  // x-y-z coordinate directions for each data value. Thus, if computing
  // derivatives for a scalar function in a hexahedron, dim=1, 8 values are
  // supplied, and 3 deriv values are returned (i.e., derivatives in x-y-z
  // directions). On the other hand, if computing derivatives of velocity
  // (vx,vy,vz) dim=3, 24 values are supplied ((vx,vy,vz)1, (vx,vy,vz)2,
  // ....()8), and 9 deriv values are returned
  // ((d(vx)/dx),(d(vx)/dy),(d(vx)/dz), (d(vy)/dx),(d(vy)/dy), (d(vy)/dz),
  // (d(vz)/dx),(d(vz)/dy),(d(vz)/dz)).
  virtual void Derivatives(int subId, float pcoords[3], float *values, 
                           int dim, float *derivs) = 0;


  // Description:
  // Compute cell bounding box (xmin,xmax,ymin,ymax,zmin,zmax). Copy result
  // into user provided array.
  void GetBounds(float bounds[6]);


  // Description:
  // Compute cell bounding box (xmin,xmax,ymin,ymax,zmin,zmax). Return pointer
  // to array of six float values.
  float *GetBounds();


  // Description:
  // Compute Length squared of cell (i.e., bounding box diagonal squared).
  float GetLength2();


  // Description:
  // Return center of the cell in parametric coordinates.  Note that the
  // parametric center is not always located at (0.5,0.5,0.5). The return
  // value is the subId that the center is in (if a composite cell). If you
  // want the center in x-y-z space, invoke the EvaluateLocation() method.
  virtual int GetParametricCenter(float pcoords[3]);


  // Description:
  // Bounding box intersection modified from Graphics Gems Vol I. The method
  // returns a non-zero value if the bounding box is hit. Origin[3] starts
  // the ray, dir[3] is the vector components of the ray in the x-y-z
  // directions, coord[3] is the location of hit, and t is the parametric
  // coordinate along line. (Notes: the intersection ray dir[3] is NOT
  // normalized.  Valid intersections will only occur between 0<=t<=1.)
  static char HitBBox(float bounds[6], float origin[3], float dir[3], 
                      float coord[3], float& t);

  
  // left public for quick computational access
  vtkPoints *Points;
  vtkIdList *PointIds;

protected:
  vtkCell();
  ~vtkCell();
  vtkCell(const vtkCell&) {};
  void operator=(const vtkCell&) {};

  float Bounds[6];

};

#endif


