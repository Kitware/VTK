/*=========================================================================

  Program:   Visualization Library
  Module:    Decimate.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlDecimate - reduce the number of triangles in a mesh
// .SECTION Description
// vlDecimate is a filter to reduce the number of triangles in a triangle 
// mesh, while preserving the original topology and a good approximation
// to the original geometry. The input to vlDecimate is a vlPolyData object,
// and only triangles are treated. If you desire to decimate polygonal
// meshes, first triangulate the polygons with the vlTriangleFilter object.
//    The algorithm proceeds as follows. Each vertex in the triangle
// list is evaluated for local planarity (i.e., the triangles using
// the vertex are gathered and compared to an "average" plane). If the
// region is locally planar, that is if the target vertex is within a
// certain distance of the average plane (i.e., the error), and
// there are no edges radiating from the vertex that have a dihedral angle
// greater than a user-specified edge angle (i.e., feature angle), and
// topology is not altered, then that vertex is deleted. The resulting
// hole is then patched by re-triangulation. The process creates over
// the entire vertex list (this constitutes an iteration). Iterations
// proceed until a target reduction is reached or a maximum iteration
// count is exceeded.
//    There are a number of additional parameters you can set to control the 
// decimation algorithm. The error may be increased over each iteration 
// with the error increment. Edge preservation may be disabled or enabled.
// You can turn on/off edge vertex deletion. (Edge vertices are vertices that
// lie along boundaries of meshes). Sub iterations are iterations that are 
// performed without changing the decimation criterion. The aspect ratio
// controls the shape of the triangles that are created, and is the ratio 
// of maximum edge length to minimum edge length. The degree is the number 
// of triangles using a single vertex. Vertices of high degree are considered
// "complex" and are never deleted.
//    This implementation has been adapted for a global error bound decimation
// criterion. That is, the error is a global bounds on distance to original
// surface.

#ifndef __vlDecimate_h
#define __vlDecimate_h

#include "P2PF.hh"
#include "vlMath.hh"
#include "Triangle.hh"
#include "Plane.hh"
#include "Polygon.hh"
#include "Line.hh"

#define NUMBER_STATISTICS 12
#define TOLERANCE 1.0e-05

#define MAX_TRIS_PER_VERTEX MAX_CELL_SIZE
#define MAX_SQUAWKS 10

#define COMPLEX_VERTEX 0
#define SIMPLE_VERTEX 1
#define BOUNDARY_VERTEX 2
#define INTERIOR_EDGE_VERTEX 3
#define CORNER_VERTEX 4

#define ELIMINATED_DISTANCE_TO_PLANE 5
#define ELIMINATED_DISTANCE_TO_EDGE 6
#define FAILED_DEGREE_TEST 7
#define FAILED_NON_MANIFOLD 8
#define FAILED_ZERO_AREA_TEST 9
#define FAILED_ZERO_NORMAL_TEST 10
#define FAILED_TO_TRIANGULATE 11


// Special structures for building loops
typedef struct _vlLocalVertex 
  {
  int     id;
  float   x[3];
  float   FAngle;
  int     deRefs; //monitor memory requirements; new only when necessary
  int     newRefs;
  } vlLocalVertex, *vlLocalVertexPtr;
    
typedef struct _vlLocalTri
  {
  int     id;
  float   area;
  float   n[3];
  int     verts[3];
  } vlLocalTri, *vlLocalTriPtr;

//
// Special classes for manipulating data
//
//BTX - begin tcl exclude
//
class vlVertexArray { //;prevent man page generation
public:
  vlVertexArray(const int sz) 
    {this->MaxId = -1; this->Array = new vlLocalVertex[sz];};
  ~vlVertexArray() {if (this->Array) delete [] this->Array;};
  int GetNumberOfVertices() {return this->MaxId + 1;};
  void InsertNextVertex(vlLocalVertex& v) 
    {this->MaxId++; this->Array[this->MaxId] = v;};
  vlLocalVertex& GetVertex(int i) {return this->Array[i];};
  void Reset() {this->MaxId = -1;};

  vlLocalVertex *Array;  // pointer to data
  int MaxId;             // maximum index inserted thus far
};

class vlTriArray { //;prevent man page generation
public:
  vlTriArray(const int sz) 
    {this->MaxId = -1; this->Array = new vlLocalTri[sz];};
  ~vlTriArray() {if (this->Array) delete [] this->Array;};
  int GetNumberOfTriangles() {return this->MaxId + 1;};
  void InsertNextTriangle(vlLocalTri& t) 
    {this->MaxId++; this->Array[this->MaxId] = t;};
  vlLocalTri& GetTriangle(int i) {return this->Array[i];};
  void Reset() {this->MaxId = -1;};

  vlLocalTri *Array;  // pointer to data
  int MaxId;            // maximum index inserted thus far
};
//ETX - end tcl exclude
//

//
//  Static variables used by object
//
static vlPlane plane; // eliminate constructor overhead
static vlLine line;
static vlTriangle triangle;
static vlMath math;

static vlPolyData *Mesh; // operate on this data structure
static float Pt[3], Normal[3]; // least squares plane point & normal
static float Angle, Distance; // current feature angle and distance 
static float CosAngle; // Cosine of dihedral angle

static float Tolerance; // Intersection tolerance
static float AspectRatio2; // Allowable aspect ratio 
static int ContinueTriangulating; // Stops recursive tri. if necessary 
static int Squawks; // Control output 

static float X[3]; //coordinates of current point
static float *VertexError, Error, MinEdgeError; //support error omputation

// temporary working arrays
static vlVertexArray V(MAX_TRIS_PER_VERTEX+1);//cycle of vertices around point
static vlTriArray T(MAX_TRIS_PER_VERTEX+1); //cycle of triangles around point


class vlDecimate : public vlPolyToPolyFilter
{
public:
  vlDecimate();
  ~vlDecimate() {};
  char *GetClassName() {return "vlDecimate";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Set the decimation error bounds. Expressed as a fraction of the longest
  // side of the input data's bounding box.
  vlSetClampMacro(InitialError,float,0.0,1.0);
  vlGetMacro(InitialError,float);

  // Description:
  // Set the value of the increment by which to increase the decimation
  // error after each iteration.
  vlSetClampMacro(ErrorIncrement,float,0.0,1.0);
  vlGetMacro(ErrorIncrement,float);

  // Description:
  // Set the largest decimation error that can be achieved during
  // by incrementing error.
  vlSetClampMacro(MaximumError,float,0.0,1.0);
  vlGetMacro(MaximumError,float);

  // Description:
  // Specify the desired reduction in the total number of polygons. Because
  // of various constraints, this level of reduction may not be realizable.
  vlSetClampMacro(TargetReduction,float,0.0,1.0);
  vlGetMacro(TargetReduction,float);

  // Description:
  // Specify the maximum number of iterations to attempt. If decimation target
  // is reached first, this value will not be reached.
  vlSetClampMacro(MaximumIterations,int,1,LARGE_INTEGER);
  vlGetMacro(MaximumIterations,int);

  // Description:
  // Specify the maximum sub-iterations to perform. If no triangles are deleted
  // in a sub-iteration, the sub-iteration process is stopped.
  vlSetClampMacro(MaximumSubIterations,int,1,LARGE_INTEGER);
  vlGetMacro(MaximumSubIterations,int);
  
  // Description:
  // Specify the mesh feature angles.
  vlSetClampMacro(InitialFeatureAngle,float,0.0,180.0);
  vlGetMacro(InitialFeatureAngle,float);

  // Description:
  // Increment by which to increase feature angle over each iteration.
  vlSetClampMacro(FeatureAngleIncrement,float,0.0,180.0);
  vlGetMacro(FeatureAngleIncrement,float);

  // Description:
  // Set the largest permissable feature angle.
  vlSetClampMacro(MaximumFeatureAngle,float,0.0,180.0);
  vlGetMacro(MaximumFeatureAngle,float);

  // Description:
  // Turn on/off the preservation of feature edges.
  vlSetMacro(PreserveEdges,int);
  vlGetMacro(PreserveEdges,int);
  vlBooleanMacro(PreserveEdges,int);

  // Description:
  // Turn on/off the deletion of vertices on the boundary of a mesh.
  vlSetMacro(BoundaryVertexDeletion,int);
  vlGetMacro(BoundaryVertexDeletion,int);
  vlBooleanMacro(BoundaryVertexDeletion,int);

  // Description:
  // Specify the maximum allowable feature angle during triangulation.
  vlSetClampMacro(AspectRatio,float,1.0,1000.0);
  vlGetMacro(AspectRatio,float);

  // Description:
  // If the number of triangles connected to a vertex exceeds "Degree", then 
  // the vertex is considered complex and is never deleted. (NOTE: the
  // complexity of the triangulation algorithm is proportional to Degree^2.)
  vlSetClampMacro(Degree,int,25,MAX_CELL_SIZE);
  vlGetMacro(Degree,int);
  
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
  int Stats[NUMBER_STATISTICS]; // keep track of interesting statistics
  int GenerateErrorScalars; // turn on/off vertex error scalar generation

  void CreateOutput(int numPts, int numTris, int numEliminated, 
                    vlPointData *pd, vlPoints *inPts);
  int BuildLoop(int ptId, unsigned short int nTris, int* tris);
  void EvaluateLoop(int ptId, int& vtype, int& numFEdges, 
                    vlLocalVertexPtr fedges[]);
  int CanSplitLoop(vlLocalVertexPtr fedges[2], int numVerts, 
                   vlLocalVertexPtr verts[], int& n1, vlLocalVertexPtr l1[], 
                   int& n2, vlLocalVertexPtr l2[], float& ar);
  void SplitLoop(vlLocalVertexPtr fedges[2], int numVerts, 
                 vlLocalVertexPtr *verts, int& n1, vlLocalVertexPtr *l1, 
                 int& n2, vlLocalVertexPtr *l2);
  void Triangulate(int numVerts, vlLocalVertexPtr verts[]);
  int CheckError(int ptId);
};

#endif


