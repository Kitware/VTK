/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDecimatePro.h
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
// .NAME vtkDecimatePro - reduce the number of triangles in a mesh
// .SECTION Description
// vtkDecimatePro is a filter to reduce the number of triangles in a triangle 
// mesh, forming a good approximation to the original geometry. The input to 
// vtkDecimatePro is a vtkPolyData object, and only triangles are treated. If 
// you desire to decimate polygonal meshes, first triangulate the polygons
// with vtkTriangleFilter object.
// 
// The implementation of vtkDecimatePro is similar to the algorithm originally
// described in "Decimation of Triangle Meshes", Proc Siggraph `92, with
// three major differences. First, this algorithm does not necessarily
// preserve the topology of the mesh. Second, it is guaranteed to give the a
// mesh reduction factor specified by the user (as long as certain
// contraints are not set - see Caveats). Third, it is set up generate progressive
// meshes, that is a stream of operations that can be easily transmitted and
// incrementally updated (see Hugues Hoppe's Siggraph '96 paper on
// progressive meshes).
// 
// The algorithm proceeds as follows. Each vertex in the mesh is classified
// and inserted into a priority queue. The priority is based on the error to
// delete the vertex and retriangulate the hole. Vertices that cannot be
// deleted or triangulated (at this point in the algorithm) are
// skipped. Then, each vertex in the priority queue is processed (i.e.,
// deleted followed by hole triangulation using edge collapse). This
// continues until the priority queue is empty. Next, all remaining vertices
// are processed, and the mesh is split into separate pieces along sharp
// edges or at non-manifold attachment points and reinserted into the
// priority queue. Again, the priority queue is processed until empty. If
// the desired reduction is still not achieved, the remaining vertices are
// split as necessary (in a recursive fashion) so that it is possible to
// eliminate every triangle as necessary.
// 
// To use this object, at a minimum you need to specify the ivar
// TargetReduction. The algorithm is guaranteed to generate a reduced mesh
// at this level as long as the following four conditions are met: 1)
// topology modification is allowed (i.e., the ivar PreserveTopology is on);
// 2) mesh splitting is enabled (i.e., the ivar Splitting is on); 3) the
// algorithm is allowed to modify the boundary of the mesh (i.e., the ivar
// BoundaryVertexDeletion is on); and 4) the maximum allowable error (i.e.,
// the ivar MaximumError) is set to VTK_LARGE_FLOAT.  Other important
// parameters to adjust include the FeatureAngle and SplitAngle ivars, since
// these can impact the quality of the final mesh. Also, you can set the
// ivar AccumulateError to force incremental error update and distribution
// to surrounding vertices as each vertex is deleted. The accumulated error
// is a conservative global error bounds and decimation error, but requires
// additional memory and time to compute.

// .SECTION Caveats
// To guarantee a given level of reduction, the ivar PreserveTopology must
// be off; the ivar Splitting is on; the ivar BoundaryVertexDeletion is on;
// and the ivar MaximumError is set to VTK_LARGE_FLOAT.
//
// If PreserveTopology is off, and SplitEdges is off; the mesh topology may
// be modified by closing holes.
//
// Once mesh splitting begins, the feature angle is set to the split angle.

// .SECTION See Also
// vtkDecimate


#ifndef __vtkDecimatePro_h
#define __vtkDecimatePro_h

#include "vtkPolyDataToPolyDataFilter.h"
#include "vtkPriorityQueue.h"

// Special structures for building loops
typedef struct _vtkProLocalVertex 
  {
  int     id;
  float   x[3];
  float   FAngle;
  } vtkProLocalVertex, *vtkProLocalVertexPtr;
    
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
    {this->MaxId = -1; this->Array = new vtkProLocalVertex[sz];};
  ~vtkVertexArray() {if (this->Array) delete [] this->Array;};
  int GetNumberOfVertices() {return this->MaxId + 1;};
  void InsertNextVertex(vtkProLocalVertex& v) 
    {this->MaxId++; this->Array[this->MaxId] = v;};
  vtkProLocalVertex& GetVertex(int i) {return this->Array[i];};
  void Reset() {this->MaxId = -1;};

  vtkProLocalVertex *Array; // pointer to data
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
  int MaxId;           // maximum index inserted thus far
};
//ETX - end tcl exclude
//


class VTK_EXPORT vtkDecimatePro : public vtkPolyDataToPolyDataFilter
{
public:
  vtkDecimatePro();
  ~vtkDecimatePro();
  static vtkDecimatePro *New() {return new vtkDecimatePro;};
  const char *GetClassName() {return "vtkDecimatePro";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the desired reduction in the total number of polygons. Because
  // of various constraints, this level of reduction may not be realized. If
  // you want to guarantee a particular reduction, you must turn off 
  // PreserveTopology and BoundaryVertexDeletion, turn on SplitEdges,
  // and set the MaximumError to VTK_LARGE_FLOAT (these ivars are initialized 
  // this way when the object is instantiated).
  vtkSetClampMacro(TargetReduction,float,0.0,1.0);
  vtkGetMacro(TargetReduction,float);

  // Description:
  // Turn on/off whether to preserve the topology of the original mesh. If
  // on, mesh splitting and hole elimination will not occur. This may limit
  // the maximumm reduction that may be achieved.
  vtkSetMacro(PreserveTopology,int);
  vtkGetMacro(PreserveTopology,int);
  vtkBooleanMacro(PreserveTopology,int);

  // Description:
  // Specify the mesh feature angle. This angle is used to define what
  // an edge is (i.e., if the surface normal between two adjacent triangles
  // is >= FeatureAngle, an edge exists).
  vtkSetClampMacro(FeatureAngle,float,0.0,180.0);
  vtkGetMacro(FeatureAngle,float);

  // Description:
  // Turn on/off the splitting of the mesh at corners, along edges, at
  // non-manifold points, or anywhere else a split is required. Turning 
  // splitting off will better preserve the original topology of the
  // mesh, but you may not obtain the requested reduction.
  vtkSetMacro(Splitting,int);
  vtkGetMacro(Splitting,int);
  vtkBooleanMacro(Splitting,int);

  // Description:
  // Specify the mesh split angle. This angle is used to control the splitting
  // of the mesh. A split line exists when the surface normals between
  // two edge connected triangles are >= SplitAngle.
  vtkSetClampMacro(SplitAngle,float,0.0,180.0);
  vtkGetMacro(SplitAngle,float);

  // Description:
  // In some cases you may wish to split the mesh prior to algorithm
  // execution. This separates the mesh into semi-planar patches, which are
  // disconnected from each other. This can give superior results in some
  // cases. If the ivar PreSplitMesh ivar is enabled, the mesh is split with
  // the specified SplitAngle. Otherwise mesh splitting is deferred as long
  // as possible.
  vtkSetMacro(PreSplitMesh,int);
  vtkGetMacro(PreSplitMesh,int);
  vtkBooleanMacro(PreSplitMesh,int);

  // Description:
  // Set the largest decimation error that is allowed during the decimation
  // process. This may limit the maximum reduction that may be achieved. The
  // maximum error is specified as a fraction of the maximum length of
  // the input data bounding box.
  vtkSetClampMacro(MaximumError,float,0.0,10.0);
  vtkGetMacro(MaximumError,float);

  // Description:
  // The computed error can either be computed directly from the mesh
  // or the error may be accumulated as the mesh is modified. If the error
  // is accumulated, then it represents a global error bounds, and the ivar
  // MaximumError becomes a global bounds on mesh error. Accumulating the
  // error requires extra memory proportional to the number of vertices in
  // the mesh. If AccumulateError is off, then the error is not accumulated.
  vtkSetMacro(AccumulateError,int);
  vtkGetMacro(AccumulateError,int);
  vtkBooleanMacro(AccumulateError,int);

  // Description:
  // Turn on/off the deletion of vertices on the boundary of a mesh. This
  // may limit the maximum reduction that may be achieved.
  vtkSetMacro(BoundaryVertexDeletion,int);
  vtkGetMacro(BoundaryVertexDeletion,int);
  vtkBooleanMacro(BoundaryVertexDeletion,int);

  // Description:
  // If the number of triangles connected to a vertex exceeds "Degree", then
  // the vertex will be split. (NOTE: the complexity of the triangulation
  // algorithm is proportional to Degree^2. Setting degree small can improve
  // the performance of the algorithm.)
  vtkSetClampMacro(Degree,int,25,VTK_CELL_SIZE);
  vtkGetMacro(Degree,int);
  
  // Description:
  // Specify the inflection point ratio. An inflection point occurs
  // when the ratio of reduction error between two iterations is greaten
  // than or equal to the InflectionPointRatio.
  vtkSetClampMacro(InflectionPointRatio,float,1.001,VTK_LARGE_FLOAT);
  vtkGetMacro(InflectionPointRatio,float);

  // Get a list of inflection points. The values in the list are mesh
  // reduction values at each inflection point. Note: the first inflection
  // point always occurs right before non-planar triangles are decimated
  // (i.e., as the error becomes non-zero).
  int GetNumberOfInflectionPoints();
  void GetInflectionPoints(float *inflectionPoints);
  float *GetInflectionPoints();

protected:
  void Execute();

  float TargetReduction;
  float FeatureAngle;
  float MaximumError;
  int AccumulateError;
  float SplitAngle;
  int Splitting;
  int PreSplitMesh;
  int BoundaryVertexDeletion;  
  int PreserveTopology;
  int Degree;
  float InflectionPointRatio;
  vtkFloatArray *InflectionPoints;


  void SplitMesh();
  int EvaluateVertex(int ptId, unsigned short int numTris, int *tris,
                     int fedges[2]);
  int FindSplit(int type, int fedges[2], int& pt1, int& pt2, 
                vtkIdList& CollapseTris);
  int IsValidSplit(int index);
  void SplitLoop(int fedges[2], int& n1, int *l1, int& n2, int *l2);
  void SplitVertex(int ptId,int type, unsigned short int numTris, int *tris,
                   int insert);
  int CollapseEdge(int type, int ptId, int collapseId, int pt1, int pt2,
                   vtkIdList& CollapseTris);
  void DistributeError(float error);

private:
  void InitializeQueue(int numPts);
  void DeleteQueue() {if (this->Queue) delete this->Queue; this->Queue=NULL;};
  void Insert(int id, float error= -1.0);
  int Pop(float &error);
  float Delete(int id) {return this->Queue->Delete(id);};
  void Reset() {this->Queue->Reset();};

  vtkPriorityQueue *Queue;
  vtkFloatArray *VertexError;

};

#endif


