/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataNormals.h
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
// .NAME vtkPolyDataNormals - compute normals for polygonal mesh
// .SECTION Description
// vtkPolyDataNormals is a filter that computes point normals for a polygonal 
// mesh. The filter can reorder polygons to insure consistent orientation
// across polygon neighbors. Sharp edges can be split and points duplicated
// with separate normals to give crisp (rendered) surface definition. It is
// also possible to globally flip the normal orientation.
//
// The algorithm works by determining normals for each polygon and then
// averaging them at shared points. When sharp edges are present, the edges
// are split and new points generated to prevent blurry edges (due to 
// Gouraud shading).

// .SECTION Caveats
// Normals are computed only for polygons and triangle strips. Normals are
// not computed for lines or vertices.
//
// Triangle strips are broken up into triangle polygons. You may want to 
// restrip the triangles.

#ifndef __vtkPolyDataNormals_h
#define __vtkPolyDataNormals_h

#include "vtkPolyDataToPolyDataFilter.h"
class vtkFloatArray;

class VTK_EXPORT vtkPolyDataNormals : public vtkPolyDataToPolyDataFilter
{
public:
  vtkTypeMacro(vtkPolyDataNormals,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with feature angle=30, splitting and consistency turned on, 
  // flipNormals turned off, and non-manifold traversal turned on.
  // ComputePointNormals is on and ComputeCellNormals is off.
  static vtkPolyDataNormals *New();

  // Description:
  // Specify the angle that defines a sharp edge. If the difference in
  // angle across neighboring polygons is greater than this value, the
  // shared edge is considered "sharp".
  vtkSetClampMacro(FeatureAngle,float,0.0,180.0);
  vtkGetMacro(FeatureAngle,float);

  // Description:
  // Turn on/off the splitting of sharp edges.
  vtkSetMacro(Splitting,int);
  vtkGetMacro(Splitting,int);
  vtkBooleanMacro(Splitting,int);

  // Description:
  // Turn on/off the enforcement of consistent polygon ordering.
  vtkSetMacro(Consistency,int);
  vtkGetMacro(Consistency,int);
  vtkBooleanMacro(Consistency,int);

  // Description:
  // Turn on/off the computation of point normals.
  vtkSetMacro(ComputePointNormals,int);
  vtkGetMacro(ComputePointNormals,int);
  vtkBooleanMacro(ComputePointNormals,int);

  // Description:
  // Turn on/off the computation of cell normals.
  vtkSetMacro(ComputeCellNormals,int);
  vtkGetMacro(ComputeCellNormals,int);
  vtkBooleanMacro(ComputeCellNormals,int);

  // Description:
  // Turn on/off the global flipping of normal orientation. Flipping
  // reverves the meaning of front and back for Frontface and Backface
  // culling in vtkProperty.  Flipping modifies both the normal
  // direction and the order of a cell's points.
  vtkSetMacro(FlipNormals,int);
  vtkGetMacro(FlipNormals,int);
  vtkBooleanMacro(FlipNormals,int);

  // Description:
  // Turn on/off traversal across non-manifold edges. This will prevent
  // problems where the consistency of polygonal ordering is corrupted due
  // to topological loops.
  vtkSetMacro(NonManifoldTraversal,int);
  vtkGetMacro(NonManifoldTraversal,int);
  vtkBooleanMacro(NonManifoldTraversal,int);

#ifndef VTK_REMOVE_LEGACY_CODE
  // Description:
  // FOR LEGACY COMPATIBILITY ONLY, DO NOT USE.
  // The connectivity extraction algorithm works recursively. In some systems 
  // the stack depth is limited. This methods specifies the maximum recursion 
  // depth.
  void SetMaxRecursionDepth(int) 
    {VTK_LEGACY_METHOD(SetMaxRecursionDepth,"4.0");}
  int GetMaxRecursionDepth()
    {VTK_LEGACY_METHOD(GetMaxRecursionDepth,"4.0"); return 0;}
#endif
  
protected:
  vtkPolyDataNormals();
  ~vtkPolyDataNormals() {};
  vtkPolyDataNormals(const vtkPolyDataNormals&) {};
  void operator=(const vtkPolyDataNormals&) {};

  // Usual data generation method
  void Execute();
  
  void ComputeInputUpdateExtents(vtkDataObject *output);

  float FeatureAngle;
  int Splitting;
  int Consistency;
  int FlipNormals;
  int NonManifoldTraversal;
  int ComputePointNormals;
  int ComputeCellNormals;
  int NumFlips;

private:
  vtkIdList *Wave;
  vtkIdList *Wave2;
  vtkIdList *CellIds;
  vtkIdList *Map;
  vtkPolyData *OldMesh;
  vtkPolyData *NewMesh;
  int *Visited;
  vtkFloatArray *PolyNormals;
  float CosAngle;

  // Uses the list of cell ids (this->Wave) to propagate a wave of
  // checked and properly ordered polygons.
  void TraverseAndOrder(void);

  // Check the point id give to see whether it lies on a feature
  // edge. If so, split the point (i.e., duplicate it) to topologically
  // separate the mesh.
  void MarkAndSplit(vtkIdType ptId);

};

#endif
