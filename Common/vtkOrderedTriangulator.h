/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOrderedTriangulator.h
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
// .NAME vtkOrderedTriangulator - helper class to generate triangulations
// .SECTION Description
// This class is used to generate unique triangulations of points. The
// uniqueness of the triangulation is controlled by the id of the inserted
// points in combination with a Delaunay criterion. The class is designed to
// be very fast and uses block memory allocations to support rapid
// triangulation generation. Also, the assumption behind the class is that a
// maximum of hundreds of points are to be triangulated. If you desire more
// robust triangulation methods use vtkPolygon::Triangulate(), vtkDelaunay2D,
// or vtkDelaunay3D.
//
// Background: Delaunay triangulations are unique assuming a random
// distribution of input points. The 3D Delaunay criterion is as follows:
// the circumsphere of each tetrahedron contains no other points of the
// triangulation except for the four points defining the tetrahedron.
// In application this property is hard to satisfy because objects like
// cubes are defined by eight points all sharing the same circumsphere
// (center and radius); hence the Delaunay triangulation is not unique.
// These so-called degenerate situations are typically resolved by
// arbitrary selecting a triangulation. This code does something different:
// it resolves degenerate triangulations by modifying the "InCircumsphere"
// method to use a slightly smaller radius. Hence, degenerate points are
// always considered "out" of the circumsphere. This, in combination with
// an ordering (based on id) of the input points, guarantees a unique
// triangulation.
//
// There is another related characteristic of Delaunay triangulations. Given
// a N-dimensional Delaunay triangulation, points lying on a (N-1) dimensional
// plane also form a (N-1) Delaunay triangulation. This means for example,
// that if a 3D cell is defined by a set of (2D) planar faces, then the
// face triangulations are Delaunay. Combinaing this with the method to
// generate unique triangulations described previously, the triangulations
// on the face are guaranteed unique. This fact can be used to triangulate
// 3D objects in such a way to guarantee compatible face triangulations.
// This is a very useful fact for parallel processing, or performing
// operations like clipping that require compatible triangulations across
// 3D cell faces.

// .SECTION Caveats
// Duplicate vertices will be ignored, i.e., if two points have the same
// coordinates the second one is discarded. The implications are that the
// user of this class most prevent duplicate points. Because the precision
// of this algorithm is double, it's also a good idea to merge points
// that are within some epsilon of one another.

// .SECTION See Also
// vtkDelaunay2D vtkDelaunay3D vtkPolygon

#ifndef __vtkOrderedTriangulator_h
#define __vtkOrderedTriangulator_h

#include "vtkObject.h"

class vtkUnstructuredGrid;
class vtkOTMesh;
class vtkCellArray;

class VTK_EXPORT vtkOrderedTriangulator : public vtkObject
{
public:
  vtkTypeMacro(vtkOrderedTriangulator,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object.
  static vtkOrderedTriangulator *New();

  // Description:
  // Initialize the triangulation process. Provide a bounding box and
  // the number of points to be inserted.
  void InitTriangulation(float bounds[6], int numPts);

  // Description:
  // For each point to be inserted, provide an id, a position x, and
  // whether the point is inside (type=0), outside (type=1), or on the
  // boundary (type=2). You must call InitTriangulation() prior to 
  // invoking this method.
  void InsertPoint(unsigned long id, float x[3], int type);

  // Description:
  // Perform the triangulation. (Complete all calls to InsertPoint() prior
  // to invoking this method.)
  void Triangulate();

  // Description:
  // Boolean indicates whether the points have been pre-sorted. If 
  // pre-sorted is enabled, the points are not sorted on point id.
  // By default, presorted is off.
  vtkSetMacro(PreSorted,int)
  vtkGetMacro(PreSorted,int)
  vtkBooleanMacro(PreSorted,int)

  // Description:
  // Add the tetrahedra classified (0=inside,1=outside) to the connectivity
  // list provided. Inside tetrahedron are those whose points are all
  // classified "inside." Outside tetrahedron have at least one point
  // classified "outside." 
  void GetTetras(int classification, vtkCellArray *connectivity);
  
  // Description:
  // Create a vtkUnstructuredGrid with the tetrahedra classified as
  // specified (0=inside,1=outside,2=all). Inside tetrahedron are 
  // those whose points are classified "inside" or on the "boundary." 
  // Outside tetrahedron have at least one point classified "outside." 
  void GetTetras(int classification, vtkUnstructuredGrid *ugrid);
  
protected:
  vtkOrderedTriangulator();
  ~vtkOrderedTriangulator();
  vtkOrderedTriangulator(const vtkOrderedTriangulator&) {}
  void operator=(const vtkOrderedTriangulator&) {};

private:
  vtkOTMesh  *Mesh;
  int NumberOfPoints;
  int PreSorted;
  
};

#endif


