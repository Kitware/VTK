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
// certain distance of the average plane (i.e., the criterion), and
// there are no edges radiating from the vertex that have a dihedral angle
// greater than a user-specified edge angle (i.e., feature angle), and
// topology is not altered, then that vertex is deleted. The resulting
// hole is then patched by retriangulation. The process creates over
// the entire vertex list (this constitutes an iteration). Iterations
// proceed until a target reduction is reached or a maximum iteration
// count is exceeded.
//    There are a number of additional parameters you can set to control the 
// decimation algorithm. The criterion may be increased over each iteration 
// with a criterion increment. Edge preservation may be disabled or enabled.
// You can turn on/off edge vertex deletion. (Edge vertices are vertices that
// lie along boundaries of meshes). Sub iterations are iterations that are 
// performed without changing the decimation criterion. The aspect ration 
// controls the shape of the triangles that are created, and is 
// the ratio of maximum edge length to minimum edge length. The degree 
// is the number of triangles using a single vertex. Vertices of high degree
// are considered "complex" and are never deleted.


#ifndef __vlDecimate_h
#define __vlDecimate_h

#include "P2PF.hh"

#define NUMBER_STATISTICS 12

typedef struct vlVertex    Vertex, *VertexPtr;
struct vlVertex 
  {
  int     id;
  float   FAngle;
  int     deRefs; //monitor memory requirements; new only when necessary
  int     newRefs;
  };
    
typedef struct vlTri    Tri, *TriPtr;
struct vlTri
  {
  int     id;
  float   area;
  float   n[3];
  int     verts[3];
  };

class vlDecimate : public vlPolyToPolyFilter
{
public:
  vlDecimate();
  ~vlDecimate() {};
  char *GetClassName() {return "vlDecimate";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Set the decimation criterion. Expressed as a fraction of the diagonal
  // of the input data's bounding box.
  vlSetClampMacro(Criterion,float,0.0,1.0);
  vlGetMacro(Criterion,float);

  // Description:
  // Set the value of the increment by which to increase the decimation
  // criterion after each iteration.
  vlSetClampMacro(CriterionIncrement,float,0.0,1.0);
  vlGetMacro(CriterionIncrement,float);

  // Description:
  // Set the largest decimation criterion that can be achieved during
  // by incrementing criterion.
  vlSetClampMacro(MaximumCriterion,float,0.0,1.0);
  vlGetMacro(MaximumCriterion,float);

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
  vlSetClampMacro(FeatureAngle,float,0.0,180.0);
  vlGetMacro(FeatureAngle,float);

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

  float FeatureAngle; // dihedral angle constraint
  float FeatureAngleIncrement;
  float MaximumFeatureAngle;
  int PreserveEdges; // do/don't worry about feature edges
  int BoundaryVertexDeletion;  
  float Criterion; // decimation criterion in fraction of bounding box
  float CriterionIncrement; // each iteration will bump criterion this amount
  float MaximumCriterion; // maximum criterion
  float TargetReduction; //target reduction of mesh (fraction)
  int MaximumIterations; // maximum number of passes over data
  int MaximumSubIterations; // maximum non-incrementing passes
  float AspectRatio; // control triangle shape during triangulation
  int Degree; // maximum number of triangles incident on vertex
  int Stats[NUMBER_STATISTICS]; // keep track of interesting statistics

  int BuildLoop(int ptId, unsigned short int nTris, int* tris);
  void EvaluateLoop(int ptId, int& vtype, int& numFEdges, VertexPtr fedges[]);
  int CanSplitLoop(VertexPtr fedges[2], int numVerts, VertexPtr verts[],
                  int& n1, VertexPtr l1[], int& n2, VertexPtr l2[], float& ar);
  void SplitLoop(VertexPtr fedges[2], int numVerts, VertexPtr *verts,
                   int& n1, VertexPtr *l1, int& n2, VertexPtr *l2);
  void Triangulate(int numVerts, VertexPtr verts[]);
};

#endif


