/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDecimate.h
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
// .NAME vtkDecimate - reduce the number of triangles in a mesh
// .SECTION Description
// vtkDecimate is a filter to reduce the number of triangles in a triangle 
// mesh, while preserving the original topology and a forming good 
// approximation to the original geometry. The input to vtkDecimate is a 
// vtkPolyData object, and only triangles are treated. If you desire to 
// decimate polygonal meshes, first triangulate the polygons with the 
// vtkTriangleFilter object.
//
// The algorithm proceeds as follows. Each vertex in the triangle
// list is evaluated for local planarity (i.e., the triangles using
// the vertex are gathered and compared to an "average" plane). If the
// region is locally planar, that is if the target vertex is within a
// certain distance of the average plane (i.e., the error), and
// there are no edges radiating from the vertex that have a dihedral angle
// greater than a user-specified edge angle (i.e., feature angle), and
// topology is not altered, then that vertex is deleted. The resulting
// hole is then patched by re-triangulation. The process continues over
// the entire vertex list (this constitutes an iteration). Iterations
// proceed until a target reduction is reached or a maximum iteration
// count is exceeded.
// 
// There are a number of additional parameters you can set to control
// the decimation algorithm. The Error ivar may be increased over each
// iteration with the ErrorIncrement. (These two variables have the
// largest effect.) Edge preservation (i.e., PreserveEdges ivar) may
// be disabled or enabled. You can turn on/off edge vertex deletion
// (i.e., BoundaryVertexDeletion ivar). (Edge vertices are vertices
// that lie along boundaries of meshes.) Sub iterations are iterations
// that are performed without changing the decimation criterion. The
// AspectRatio ivar controls the shape of the triangles that are
// created, and is the ratio of maximum edge length to minimum edge
// length. The Degree is the number of triangles using a single
// vertex. Vertices of high degree are considered "complex" and are
// never deleted.
//
// .SECTION Caveats
// This implementation has been adapted for a global error bound decimation
// criterion. That is, the error is a global bound on distance to original
// surface. This is an improvement over the original Siggraph paper ("Decimation
// of Triangle Meshes", Proc Siggraph `92.)
//
// The algorithm has been extended with a special flag to allow
// topology modification. When the PreserveTopology flag is on, then
// the algorithm will preserve the topology of the original mesh. If
// off, the algorithm may close holes and/or collapse tunnels (i.e.,
// form non-manifold attachments).

// .SECTION See Also
// vtkDecimatePro

#ifndef __vtkDecimate_h
#define __vtkDecimate_h

#include "vtkPolyToPolyFilter.h"
#include "vtkMath.h"
#include "vtkTriangle.h"
#include "vtkPlane.h"
#include "vtkPolygon.h"
#include "vtkLine.h"

#define VTK_NUMBER_STATISTICS 12

// Special structures for building loops
typedef struct _vtkLocalVertex 
  {
  int     id;
  float   x[3];
  float   FAngle;
  int     deRefs; //monitor memory requirements; new only when necessary
  int     newRefs;
  } vtkLocalVertex, *vtkLocalVertexPtr;
    
typedef struct _vtkLocalTri
  {
  int     id;
  float   area;
  float   n[3];
  int     verts[3];
  } vtkLocalTri, *vtkLocalTriPtr;

//
// Special classes for manipulating data
//
//BTX - begin tcl exclude
//
class vtkVertexArray { //;prevent man page generation
public:
  vtkVertexArray(const int sz) 
    {this->MaxId = -1; this->Array = new vtkLocalVertex[sz];};
  ~vtkVertexArray() {if (this->Array) delete [] this->Array;};
  int GetNumberOfVertices() {return this->MaxId + 1;};
  void InsertNextVertex(vtkLocalVertex& v) 
    {this->MaxId++; this->Array[this->MaxId] = v;};
  vtkLocalVertex& GetVertex(int i) {return this->Array[i];};
  void Reset() {this->MaxId = -1;};

  vtkLocalVertex *Array;  // pointer to data
  int MaxId;             // maximum index inserted thus far
};

class vtkTriArray { //;prevent man page generation
public:
  vtkTriArray(const int sz) 
    {this->MaxId = -1; this->Array = new vtkLocalTri[sz];};
  ~vtkTriArray() {if (this->Array) delete [] this->Array;};
  int GetNumberOfTriangles() {return this->MaxId + 1;};
  void InsertNextTriangle(vtkLocalTri& t) 
    {this->MaxId++; this->Array[this->MaxId] = t;};
  vtkLocalTri& GetTriangle(int i) {return this->Array[i];};
  void Reset() {this->MaxId = -1;};

  vtkLocalTri *Array;  // pointer to data
  int MaxId;            // maximum index inserted thus far
};
//ETX - end tcl exclude
//


class VTK_EXPORT vtkDecimate : public vtkPolyToPolyFilter
{
public:
  vtkDecimate();
  char *GetClassName() {return "vtkDecimate";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the decimation error bounds. Expressed as a fraction of the longest
  // side of the input data's bounding box.
  vtkSetClampMacro(InitialError,float,0.0,1.0);
  vtkGetMacro(InitialError,float);

  // Description:
  // Set the value of the increment by which to increase the decimation
  // error after each iteration.
  vtkSetClampMacro(ErrorIncrement,float,0.0,1.0);
  vtkGetMacro(ErrorIncrement,float);

  // Description:
  // Set the largest decimation error that can be achieved 
  // by incrementing the error.
  vtkSetClampMacro(MaximumError,float,0.0,1.0);
  vtkGetMacro(MaximumError,float);

  // Description:
  // Specify the desired reduction in the total number of polygons. Because
  // of various constraints, this level of reduction may not be realizable.
  vtkSetClampMacro(TargetReduction,float,0.0,1.0);
  vtkGetMacro(TargetReduction,float);

  // Description:
  // Specify the maximum number of iterations to attempt. If decimation target
  // is reached first, this value will not be reached.
  vtkSetClampMacro(MaximumIterations,int,0,VTK_LARGE_INTEGER);
  vtkGetMacro(MaximumIterations,int);

  // Description:
  // Specify the maximum sub-iterations to perform. If no triangles are deleted
  // in a sub-iteration, the sub-iteration process is stopped.
  vtkSetClampMacro(MaximumSubIterations,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(MaximumSubIterations,int);
  
  // Description:
  // Specify the mesh feature angles.
  vtkSetClampMacro(InitialFeatureAngle,float,0.0,180.0);
  vtkGetMacro(InitialFeatureAngle,float);

  // Description:
  // Set/Get the angle by which to increase feature angle over each iteration.
  vtkSetClampMacro(FeatureAngleIncrement,float,0.0,180.0);
  vtkGetMacro(FeatureAngleIncrement,float);

  // Description:
  // Set the largest permissible feature angle.
  vtkSetClampMacro(MaximumFeatureAngle,float,0.0,180.0);
  vtkGetMacro(MaximumFeatureAngle,float);

  // Description:
  // Turn on/off the preservation of feature edges.
  vtkSetMacro(PreserveEdges,int);
  vtkGetMacro(PreserveEdges,int);
  vtkBooleanMacro(PreserveEdges,int);

  // Description:
  // Turn on/off the deletion of vertices on the boundary of a mesh.
  vtkSetMacro(BoundaryVertexDeletion,int);
  vtkGetMacro(BoundaryVertexDeletion,int);
  vtkBooleanMacro(BoundaryVertexDeletion,int);

  // Description:
  // Specify the maximum allowable aspect ratio during triangulation.
  vtkSetClampMacro(AspectRatio,float,1.0,1000.0);
  vtkGetMacro(AspectRatio,float);

  // Decsription:
  // Turn on/off whether to preserve the topology of the original mesh. If
  // off, hole elimination and non-manifold attachment can occur.
  vtkSetMacro(PreserveTopology,int);
  vtkGetMacro(PreserveTopology,int);
  vtkBooleanMacro(PreserveTopology,int);

  // Description:
  // If the number of triangles connected to a vertex exceeds "Degree", then 
  // the vertex is considered complex and is never deleted. (NOTE: the
  // complexity of the triangulation algorithm is proportional to Degree^2.)
  vtkSetClampMacro(Degree,int,25,VTK_CELL_SIZE);
  vtkGetMacro(Degree,int);
  
  // Description:
  // Control the printout of warnings. This flag limits the number of warnings
  // regarding non-manifold geometry and complex vertices. If set to zero, no
  // warnings will appear.
  vtkSetClampMacro(MaximumNumberOfSquawks,int,0,VTK_LARGE_INTEGER);
  vtkGetMacro(MaximumNumberOfSquawks,int);
  
protected:
  void Execute();

  float InitialFeatureAngle; // dihedral angle constraint
  float FeatureAngleIncrement;
  float MaximumFeatureAngle;
  int PreserveEdges; // do/don't worry about feature edges
  int BoundaryVertexDeletion;  
  float InitialError; // decimation error in fraction of bounding box
  float ErrorIncrement; // each iteration will bump error this amount
  float MaximumError; // maximum error
  float TargetReduction; //target reduction of mesh (fraction)
  int MaximumIterations; // maximum number of passes over data
  int MaximumSubIterations; // maximum non-incrementing passes
  float AspectRatio; // control triangle shape during triangulation
  int Degree; // maximum number of triangles incident on vertex
  int Stats[VTK_NUMBER_STATISTICS]; // keep track of interesting statistics
  int GenerateErrorScalars; // turn on/off vertex error scalar generation
  int MaximumNumberOfSquawks; //control number of error messages
  int PreserveTopology; //control whether mesh topology is preserved

  void CreateOutput(int numPts, int numTris, int numEliminated, 
                    vtkPointData *pd, vtkPoints *inPts);
  int BuildLoop(int ptId, unsigned short int nTris, int* tris);
  void EvaluateLoop(int& vtype, int& numFEdges, vtkLocalVertexPtr fedges[]);
  int CanSplitLoop(vtkLocalVertexPtr fedges[2], int numVerts, 
                   vtkLocalVertexPtr verts[], int& n1, vtkLocalVertexPtr l1[], 
                   int& n2, vtkLocalVertexPtr l2[], float& ar);
  void SplitLoop(vtkLocalVertexPtr fedges[2], int numVerts, 
                 vtkLocalVertexPtr *verts, int& n1, vtkLocalVertexPtr *l1, 
                 int& n2, vtkLocalVertexPtr *l2);
  void Triangulate(int numVerts, vtkLocalVertexPtr verts[]);
  int CheckError();
};

#endif


