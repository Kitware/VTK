/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGreedyTerrainDecimation.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGreedyTerrainDecimation.h"
#include "vtkObjectFactory.h"
#include "vtkPriorityQueue.h"
#include "vtkImageData.h"
#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkTriangle.h"
#include "vtkDoubleArray.h"

#ifdef _MSC_VER
#pragma warning (push, 3)
#endif

#include <vector>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

vtkCxxRevisionMacro(vtkGreedyTerrainDecimation, "1.2");
vtkStandardNewMacro(vtkGreedyTerrainDecimation);

// Define some constants describing vertices
//
#define VTK_VERTEX_NO_TRIANGLE -1
#define VTK_VERTEX_INSERTED    -2

//Supporting classes for points and triangles
class vtkTriangleInfo;
class vtkTerrainInfo
{
public:
  vtkTerrainInfo():TriangleId(VTK_VERTEX_NO_TRIANGLE) {}
  vtkIdType TriangleId;
};

class vtkTriangleInfo
{
public:
  double    Normal[3]; //plane equation info
  double    Constant;
};

//PIMPL STL encapsulation
//
//maps input point ids to owning mesh triangle
class vtkGreedyTerrainDecimationTerrainInfoType : public vtkstd::vector<vtkTerrainInfo> {};
//maps mesh point id to input point id
class vtkGreedyTerrainDecimationPointInfoType : public vtkstd::vector<vtkIdType> {};
//holds extra information about mesh triangles
class vtkGreedyTerrainDecimationTriangleInfoType : public vtkstd::vector<vtkTriangleInfo> {};

// Begin vtkGreedyTerrainDecimation class implementation
//
vtkGreedyTerrainDecimation::vtkGreedyTerrainDecimation()
{
  this->ErrorMeasure = VTK_ERROR_SPECIFIED_REDUCTION;
  this->NumberOfTriangles = 1000;
  this->Reduction = 0.90;
  this->AbsoluteError = 1;
  this->RelativeError = 0.01;
  this->BoundaryVertexDeletion = 1;
}

vtkGreedyTerrainDecimation::~vtkGreedyTerrainDecimation()
{
}

inline void vtkGreedyTerrainDecimation::GetTerrainPoint(int i, int j, double x[3])
{
  x[0] = this->Origin[0] + i*this->Spacing[0];
  x[1] = this->Origin[1] + j*this->Spacing[1];
}

inline void vtkGreedyTerrainDecimation::ComputeImageCoordinates(vtkIdType inputPtId, int ij[2])
{
  ij[0] = inputPtId % this->Dimensions[0];
  ij[1] = inputPtId / this->Dimensions[0];
}

inline vtkIdType vtkGreedyTerrainDecimation::InsertNextPoint(vtkIdType inputPtId, 
                                                             double x[3])
{
  if ( (this->CurrentPointId+1) >= (vtkIdType)this->PointInfo->capacity() )
    {
    this->PointInfo->reserve(2*this->PointInfo->capacity());
    }

  double *ptr = this->Points->WritePointer(3*this->CurrentPointId,3);
  *ptr++ = *x++;
  *ptr++ = *x++;
  *ptr   = *x;

  this->OutputPD->CopyData(this->InputPD,inputPtId,this->CurrentPointId);
  (*this->PointInfo)[this->CurrentPointId] = inputPtId;

  return this->CurrentPointId++;
}

inline double *vtkGreedyTerrainDecimation::GetPoint(vtkIdType id)
{
  return this->Points->GetPointer(3*id);
}

inline void vtkGreedyTerrainDecimation::GetPoint(vtkIdType id, double x[3])
{
  double *ptr = this->Points->GetPointer(3*id);
  x[0] = *ptr++;
  x[1] = *ptr++;
  x[2] = *ptr;
}

void vtkGreedyTerrainDecimation::EstimateOutputSize(const vtkIdType numInputPts,
                                                    vtkIdType &numPts, vtkIdType &numTris)
{
  switch (this->ErrorMeasure)
    {
    case VTK_ERROR_NUMBER_OF_TRIANGLES:
      numTris = this->NumberOfTriangles;
      break;
    case VTK_ERROR_SPECIFIED_REDUCTION:
      numTris = static_cast<int>(2*numInputPts*(1.0-this->Reduction));
      break;
    default:
      numTris = numInputPts;
    }

  numPts = numTris/2 + 1;
  return;
}

int vtkGreedyTerrainDecimation::SatisfiesErrorMeasure(double error)
{
  switch (this->ErrorMeasure)
    {
    case VTK_ERROR_NUMBER_OF_TRIANGLES:
      break;
    case VTK_ERROR_SPECIFIED_REDUCTION:
      break;
    case VTK_ERROR_ABSOLUTE:
      if ( error <= this->AbsoluteError ) return 1;
    case VTK_ERROR_RELATIVE:
      break;
    }

  return 0;
}

//Update all triangles connected to this mesh point
void vtkGreedyTerrainDecimation::UpdateTriangles(vtkIdType ptId)
{
  unsigned short ncells;
  vtkIdType *cells, npts, *pts;
  
  this->Mesh->GetPointCells(ptId,ncells,cells);
  for (unsigned short i=0; i<ncells; i++)
    {
    this->Mesh->GetCellPoints(cells[i], npts, pts);
    this->UpdateTriangle(cells[i], (*this->PointInfo)[pts[0]], 
                         (*this->PointInfo)[pts[1]], (*this->PointInfo)[pts[2]]);
    }
}

//Update all points as to which triangle they lie in. Basically a scanline algorithm.
void vtkGreedyTerrainDecimation::UpdateTriangle(vtkIdType triId, 
                                                vtkIdType p1, vtkIdType p2, vtkIdType p3)
{
  // Get the coordinates of the triangle
  double *x1 = this->GetPoint(p1);
  double *x2 = this->GetPoint(p2);
  double *x3 = this->GetPoint(p3);
  
  // Compute plane parameters
  vtkTriangle::ComputeNormal( x1, x2, x3, (*this->TriangleInfo)[triId].Normal );
  (*this->TriangleInfo)[triId].Constant = 
    -(vtkMath::Dot((*this->TriangleInfo)[triId].Normal,x1));

  // Scan convert triangle / update points as to which triangle contains each point
  int ij1[2], ij2[2], ij3[2];
  this->ComputeImageCoordinates(p1, ij1);
  this->ComputeImageCoordinates(p2, ij2);
  this->ComputeImageCoordinates(p3, ij3);
  
  float h[4]; //extra entry added for interpolated value
  h[0] = (float) this->Heights->GetTuple1(p1);
  h[1] = (float) this->Heights->GetTuple1(p2);
  h[2] = (float) this->Heights->GetTuple1(p3);

  this->UpdateTriangle(triId, ij1, ij2, ij3, h);
}

void vtkGreedyTerrainDecimation::InsertBoundaryVertices()
{
  int i, j;
  vtkIdType inputPtId, offset;

  // Insert vertices around boundary of image
  // Along x-axis at y=0.
  for (i=0; i<this->Dimensions[0]; i++)
    {
    inputPtId = i;
    this->AddPointToTriangulation(inputPtId);
    }

  // Along x-axis at y=dim[1].
  offset = this->Dimensions[0]*(this->Dimensions[1]-1);
  for (i=0; i<this->Dimensions[0]; i++)
    {
    inputPtId = offset + i;
    this->AddPointToTriangulation(inputPtId);
    }

  // Along y-axis at x=0. (the end points are already inserted)
  for (j=1; j<(this->Dimensions[1]-1); j++)
    {
    inputPtId = j*this->Dimensions[0];
    this->AddPointToTriangulation(inputPtId);
   }

  // Along y-axis at x=dims[0]. (the end points are already inserted)
  offset = this->Dimensions[0]-1;
  for (j=1; j<(this->Dimensions[1]-1); j++)
    {
    inputPtId = offset + j*this->Dimensions[0];
    this->AddPointToTriangulation(inputPtId);
    }
}

// Determine whether point x is inside of circumcircle of triangle
// defined by points (x1, x2, x3). Returns non-zero if inside circle.
// (Note that z-component is ignored.)
int vtkGreedyTerrainDecimation::InCircle (double x[3], double x1[3], double x2[3], 
                                          double x3[3])
{
  double radius2, center[2], dist2;

  radius2 = vtkTriangle::Circumcircle(x1,x2,x3,center);

  // check if inside/outside circumcircle
  dist2 = (x[0]-center[0]) * (x[0]-center[0]) + 
          (x[1]-center[1]) * (x[1]-center[1]);

  if ( dist2 < (0.999999999999*radius2) )
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

#define VTK_DEL2D_TOLERANCE 1.0e-014

// Recursive method to locate triangle containing point. Starts with arbitrary
// triangle (tri) and "walks" towards it. Influenced by some of Guibas and 
// Stolfi's work. Returns id of enclosing triangle, or -1 if no triangle
// found. Also, the array nei[3] is used to communicate info about points
// that lie on triangle edges: nei[0] is neighboring triangle id, and nei[1]
// and nei[2] are the vertices defining the edge.
vtkIdType vtkGreedyTerrainDecimation::FindTriangle(double x[3], vtkIdType ptIds[3],
                                                   vtkIdType tri, double tol,
                                                   vtkIdType nei[3], vtkIdList *neighbors)
{
  int i, j, ir, ic, inside, i2, i3;
  vtkIdType *pts, npts, newNei;
  double p[3][3], n[2], vp[2], vx[2], dp, minProj;

  // get local triangle info
  this->Mesh->GetCellPoints(tri,npts,pts);
  for (i=0; i<3; i++) 
    {
    ptIds[i] = pts[i];
    this->GetPoint(ptIds[i], p[i]);
    }

  // Randomization (of find edge neighbora) avoids walking in 
  // circles in certain weird cases
  srand(tri);
  ir = rand() % 3;
  // evaluate in/out of each edge
  for (inside=1, minProj=0.0, ic=0; ic<3; ic++)
    {
    i  = (ir+ic) % 3;
    i2 = (i+1) % 3;
    i3 = (i+2) % 3;

    // create a 2D edge normal to define a "half-space"; evaluate points (i.e.,
    // candiate point and other triangle vertex not on this edge).
    n[0] = -(p[i2][1] - p[i][1]);
    n[1] = p[i2][0] - p[i][0];
    vtkMath::Normalize2D(n);

    // compute local vectors
    for (j=0; j<2; j++)
      {
      vp[j] = p[i3][j] - p[i][j];
      vx[j] = x[j] - p[i][j];
      }

    //check for duplicate point
    vtkMath::Normalize2D(vp);
    if ( vtkMath::Normalize2D(vx) <= tol ) 
      {
      vtkErrorMacro("Duplicate point");
      return -1;
      }

    // see if two points are in opposite half spaces
    dp = vtkMath::Dot2D(n,vx) * (vtkMath::Dot2D(n,vp) < 0 ? -1.0 : 1.0);
    if ( dp < VTK_DEL2D_TOLERANCE )
      {
      if ( dp < minProj ) //track edge most orthogonal to point direction
        {
        inside = 0;
        nei[1] = ptIds[i];
        nei[2] = ptIds[i2];
        minProj = dp;
        }
      }//outside this edge
    }//for each edge

  if ( inside ) // all edges have tested positive
    {
    nei[0] = (-1);
    return tri;
    }

  else if ( !inside && (fabs(minProj) < VTK_DEL2D_TOLERANCE) ) // on edge
    {
    this->Mesh->GetCellEdgeNeighbors(tri,nei[1],nei[2],neighbors);
    nei[0] = neighbors->GetId(0);
    return tri;
    }

  else //walk towards point
    {
    this->Mesh->GetCellEdgeNeighbors(tri,nei[1],nei[2],neighbors);
    if ( (newNei=neighbors->GetId(0)) == nei[0] )
      {
      vtkErrorMacro("Degeneracy");
      return -1;
      }
    else
      {
      nei[0] = tri;
      return this->FindTriangle(x,ptIds,newNei,tol,nei,neighbors);
      }
    }
}

#undef VTK_DEL2D_TOLERANCE

// Recursive method checks whether edge is Delaunay, and if not, swaps edge.
// Continues until all edges are Delaunay. Points p1 and p2 form the edge in
// question; x is the coordinates of the inserted point; tri is the current
// triangle id.
void vtkGreedyTerrainDecimation::CheckEdge(vtkIdType ptId, double x[3], vtkIdType p1,
                                           vtkIdType p2, vtkIdType tri)
{
  int i;
  vtkIdType *pts, npts, numNei, nei, p3;
  double x1[3], x2[3], x3[3];
  vtkIdList *neighbors;
  vtkIdType swapTri[3];

  this->GetPoint(p1,x1);
  this->GetPoint(p2,x2);

  neighbors = vtkIdList::New();
  neighbors->Allocate(2);

  this->Mesh->GetCellEdgeNeighbors(tri,p1,p2,neighbors);
  numNei = neighbors->GetNumberOfIds();

  if ( numNei > 0 ) //i.e., not a boundary edge
    {
    // get neighbor info including opposite point
    nei = neighbors->GetId(0);
    this->Mesh->GetCellPoints(nei, npts, pts);
    for (i=0; i<2; i++)
      {
      if ( pts[i] != p1 && pts[i] != p2 )
        {
        break;
        }
      }
    p3 = pts[i];
    this->GetPoint(p3,x3);

    // see whether point is in circumcircle
    if ( this->InCircle (x3, x, x1, x2) )
      {// swap diagonal
      this->Mesh->RemoveReferenceToCell(p1,tri);
      this->Mesh->RemoveReferenceToCell(p2,nei);
      this->Mesh->ResizeCellList(ptId,1);
      this->Mesh->AddReferenceToCell(ptId,nei);
      this->Mesh->ResizeCellList(p3,1);
      this->Mesh->AddReferenceToCell(p3,tri);

      swapTri[0] = ptId; swapTri[1] = p3; swapTri[2] = p2;
      this->Mesh->ReplaceCell(tri,3,swapTri);

      swapTri[0] = ptId; swapTri[1] = p1; swapTri[2] = p3;
      this->Mesh->ReplaceCell(nei,3,swapTri);

      // two new edges become suspect
      this->CheckEdge(ptId, x, p3, p2, tri);
      this->CheckEdge(ptId, x, p1, p3, nei);

      }//in circle
    }//interior edge

  neighbors->Delete();
}

vtkIdType vtkGreedyTerrainDecimation::AddPointToTriangulation(vtkIdType inputPtId)
{
  vtkIdType ptId, nei[3], tri[4];
  vtkIdType nodes[4][3], pts[3], numNeiPts, *neiPts;
  vtkIdType i, p1=0, p2=0;
  int ij[2];
  double x[3];
  
  //Make sure the point has not been previously inserted
  if ( (*this->TerrainInfo)[inputPtId].TriangleId < 0 )
    {
    return -1;
    }

  //Indicate that it is now inserted
  (*this->TerrainInfo)[inputPtId].TriangleId = VTK_VERTEX_INSERTED;

  //Start off by determining the image coordinates and the position
  this->ComputeImageCoordinates(inputPtId, ij);
  this->GetTerrainPoint(ij[0], ij[1], x);
  x[2] = (double) this->Heights->GetTuple1(inputPtId);

  nei[0] = (-1); //where we are coming from...nowhere initially
  tri[0] = 0;
  if ( (tri[0] = this->FindTriangle(x,pts,tri[0],this->Tolerance,nei,this->Neighbors)) >= 0 )
    {
    // Insert the point into the output
    ptId = this->InsertNextPoint(inputPtId, x);

    if ( nei[0] < 0 ) //in triangle
      {
      //delete this triangle; create three new triangles
      //first triangle is replaced with one of the new ones
      nodes[0][0] = ptId; nodes[0][1] = pts[0]; nodes[0][2] = pts[1];
      this->Mesh->RemoveReferenceToCell(pts[2], tri[0]);
      this->Mesh->ReplaceCell(tri[0], 3, nodes[0]);
      this->Mesh->ResizeCellList(ptId,1);
      this->Mesh->AddReferenceToCell(ptId,tri[0]);

      //create two new triangles
      nodes[1][0] = ptId; nodes[1][1] = pts[1]; nodes[1][2] = pts[2];
      tri[1] = this->Mesh->InsertNextLinkedCell(VTK_TRIANGLE, 3, nodes[1]);

      nodes[2][0] = ptId; nodes[2][1] = pts[2]; nodes[2][2] = pts[0];
      tri[2] = this->Mesh->InsertNextLinkedCell(VTK_TRIANGLE, 3, nodes[2]);

      // Check edge neighbors for Delaunay criterion. If not satisfied, flip
      // edge diagonal. (This is done recursively.)
      this->CheckEdge(ptId, x, pts[0], pts[1], tri[0]);
      this->CheckEdge(ptId, x, pts[1], pts[2], tri[1]);
      this->CheckEdge(ptId, x, pts[2], pts[0], tri[2]);
      }

    else // on triangle edge
      {
      //update cell list
      this->Mesh->GetCellPoints(nei[0],numNeiPts,neiPts);
      for (i=0; i<3; i++)
        {
        if ( neiPts[i] != nei[1] && neiPts[i] != nei[2] ) 
          {
          p1 = neiPts[i];
          }
        if ( pts[i] != nei[1] && pts[i] != nei[2] ) 
          {
          p2 = pts[i];
          }
        }
      this->Mesh->ResizeCellList(p1,1);
      this->Mesh->ResizeCellList(p2,1);

      //replace two triangles
      this->Mesh->RemoveReferenceToCell(nei[2],tri[0]);
      this->Mesh->RemoveReferenceToCell(nei[2],nei[0]);

      nodes[0][0] = ptId; nodes[0][1] = p2; nodes[0][2] = nei[1];
      this->Mesh->ReplaceCell(tri[0], 3, nodes[0]);

      nodes[1][0] = ptId; nodes[1][1] = p1; nodes[1][2] = nei[1];
      this->Mesh->ReplaceCell(nei[0], 3, nodes[1]);

      this->Mesh->ResizeCellList(ptId, 2);
      this->Mesh->AddReferenceToCell(ptId,tri[0]);
      this->Mesh->AddReferenceToCell(ptId,nei[0]);

      tri[1] = nei[0];

      //create two new triangles
      nodes[2][0] = ptId; nodes[2][1] = p2; nodes[2][2] = nei[2];
      tri[2] = this->Mesh->InsertNextLinkedCell(VTK_TRIANGLE, 3, nodes[2]);

      nodes[3][0] = ptId; nodes[3][1] = p1; nodes[3][2] = nei[2];
      tri[3] = this->Mesh->InsertNextLinkedCell(VTK_TRIANGLE, 3, nodes[3]);

      // Check edge neighbors for Delaunay criterion.
      for ( i=0; i<4; i++ )
        {
        this->CheckEdge (ptId, x, nodes[i][1], nodes[i][2], tri[i]);
        }
      }
    }//if triangle found

  return 0;
}

void vtkGreedyTerrainDecimation::Execute()
{
  vtkImageData *input=this->GetInput();
  vtkIdType numInputPts=input->GetNumberOfPoints(), numPts, numTris;
  vtkIdType inputPtId;
  float error, bounds[6], center[3];
  vtkCellArray *triangles;
  this->Mesh = this->GetOutput();
  this->InputPD = input->GetPointData();
  this->OutputPD = this->Mesh->GetPointData();

  // Check input and initialize
  //
  vtkDebugMacro(<<"Decimating terrain...");

  if ( input->GetDataDimension() != 2 )
    {
    vtkWarningMacro(<<"This class treats 2D height fields only");
    return;
    }
  if ( (this->Heights = this->InputPD->GetScalars()) == NULL )
    {
    vtkWarningMacro(<<"This class requires height scalars");
    return;
    }

  input->GetBounds(bounds);
  input->GetCenter(center);
  input->GetDimensions(this->Dimensions);
  float *origin = input->GetOrigin();
  float *spacing = input->GetSpacing();
  for (int ii=0; ii<3; ii++)
    {
    this->Origin[ii] = (double)origin[ii];
    this->Spacing[ii] = (double)spacing[ii];
    }

  this->TerrainError = vtkPriorityQueue::New();
  this->TerrainError->Allocate(numInputPts, (vtkIdType)((float)0.25*numInputPts));
  
  // Create the initial Delaunay triangulation (two triangles 
  // connecting the four corners of the height image).
  //
  this->EstimateOutputSize(numInputPts, numPts, numTris);

  vtkPoints *newPts = vtkPoints::New();
  newPts->SetDataTypeToDouble();
  this->Points = static_cast<vtkDoubleArray *>(newPts->GetData());
  this->CurrentPointId = 0;
  
  // Supplemental arrays contain point and triangle information
  this->TerrainInfo = new vtkGreedyTerrainDecimationTerrainInfoType;
  this->TerrainInfo->reserve(numInputPts);

  this->PointInfo = new vtkGreedyTerrainDecimationPointInfoType;
  this->PointInfo->reserve(numPts);

  this->TriangleInfo = new vtkGreedyTerrainDecimationTriangleInfoType;
  this->TriangleInfo->reserve(numTris);

  // Insert initial points
  newPts->Allocate(numPts);

  inputPtId = 0;
  newPts->InsertPoint(0, bounds[0],bounds[2], (float)this->Heights->GetTuple1(inputPtId)); //ptId=0
  this->OutputPD->CopyData(this->InputPD,inputPtId,0);
  (*this->PointInfo)[0] = inputPtId;
  
  inputPtId = this->Dimensions[0] - 1;
  newPts->InsertPoint(1, bounds[1],bounds[2],(float)this->Heights->GetTuple1(inputPtId)); //ptId=1
  this->OutputPD->CopyData(this->InputPD,inputPtId,1);
  (*this->PointInfo)[1] = inputPtId;

  inputPtId = this->Dimensions[0]*(this->Dimensions[1]-1);
  newPts->InsertPoint(2, bounds[1],bounds[3], (float)this->Heights->GetTuple1(inputPtId)); //ptId=2
  this->OutputPD->CopyData(this->InputPD,inputPtId,2);
  (*this->PointInfo)[2] = inputPtId;

  inputPtId = this->Dimensions[0]*this->Dimensions[1] - 1;
  newPts->InsertPoint(3, bounds[0],bounds[3], (float)this->Heights->GetTuple1(inputPtId)); //ptId=3
  this->OutputPD->CopyData(this->InputPD,inputPtId,3);
  (*this->PointInfo)[3] = inputPtId;

  // Insert initial triangles into output mesh
  triangles = vtkCellArray::New();
  triangles->Allocate(numTris,3);

  triangles->InsertNextCell(3);
  triangles->InsertCellPoint(0); triangles->InsertCellPoint(1); triangles->InsertCellPoint(3);

  triangles->InsertNextCell(3);
  triangles->InsertCellPoint(1); triangles->InsertCellPoint(2); triangles->InsertCellPoint(3);

  // Construct the topological hierarchy for the output mesh
  this->Mesh->SetPoints(newPts);
  this->Mesh->SetPolys(triangles);
  this->Mesh->BuildLinks(); //build cell structure

  // Update all (two) triangles connected to this mesh point. All points contained
  // by these triangles are inserted into the error queue.
  this->UpdateTriangles(3); 

  // Scratch data structures
  this->Neighbors = vtkIdList::New(); this->Neighbors->Allocate(2);

  // If vertex deletion is not allowed, insert the boundary
  // points first.
  if ( ! this->BoundaryVertexDeletion )
    {
    this->InsertBoundaryVertices();
    }

  // Points within this tolerance are considered coincident
  //
  this->Tolerance = 0.01 * this->Spacing[0];
  
  // While error metric not satisfied, add point with greatest error
  //
  while ( (inputPtId = this->TerrainError->Pop(0, error)) >= 0 )
    {
    if ( this->SatisfiesErrorMeasure(error) )
      {
      break;
      }
    else
      {
      this->AddPointToTriangulation(inputPtId);
      }
    }

  // Create output poly data
  //
  this->TerrainError->Delete();
  delete this->TerrainInfo;
  delete this->PointInfo;
  delete this->TriangleInfo;

  newPts->Delete();
  triangles->Delete();
}


// "Scan conversion" routines to update all points lying in a triangle.
//
// Divide a triangle into two subtriangles as shown.
//     
//                     o  max
//                    / \
//                    |   \
//                   /      \
//                   |        \
//             midL o..........o  midR
//                  |        _/
//                  /      _/
//                 |     _/
//                 /   _/
//                |  _/
//                /_/
//               o    min
//
// This way we can scan the two subtriangles independently without worrying about
// the transistion in interpolation that occurs at the vertices.
//
// A triangle may be characterized in one of four ways:
//   VTK_TWO_TRIANGLES: We can create a two triangle representation
//   VTK_BOTTOM_TRIANGLE: We should only scan the lower triangle
//   VTK_TOP_TRIANGLE: We should only scan the upper triangle
//   VTK_DEGENERATE: The points are colinear (not scan converted)
//
// Configuration of the two triangles
#define VTK_TWO_TRIANGLES   0 //most often
#define VTK_BOTTOM_TRIANGLE 1
#define VTK_TOP_TRIANGLE    2
#define VTK_DEGENERATE      3 //should never happen in this application

//---------------------------------------------------------------------------
// Update all points lying in the given triangle. This means indicating the triangle
// that the point is in, plus computing the error in the height field.
//
void vtkGreedyTerrainDecimation::UpdateTriangle(vtkIdType tri, int ij1[2], int ij2[2], int ij3[2],
                                                float h[3])
{
  int *min, *max, *midL, *midR, *mid, mid2[2];
  double t, tt;
  int i, j, xL, xR;
  float hMin, hMax, hMidL, hMidR, hL, hR;
  vtkIdType idx, inputPtId;
  float error;

  int type = this->CharacterizeTriangle(ij1, ij2, ij3, min, max, midL, midR, mid, mid2, 
                                        h, hMin, hMax, hMidL, hMidR);
  
  switch(type)
    {
    case VTK_BOTTOM_TRIANGLE:
    case VTK_TWO_TRIANGLES:
      for (j=min[1]+1; j<midL[1]; j++) //for all scan lines; skip vertices
        {
        idx = j*this->Dimensions[0];
        t = (double)(j - min[1]) / (midL[1] - min[1]);
        xL = (int)((1.0-t)*min[0] + t*midL[0]);
        xR = (int)((1.0-t)*min[0] + t*midR[0]);
        hL = (1.0-t)*hMin + t*hMidL;
        hR = (1.0-t)*hMin + t*hMidR;
        for (i=xL; i<=xR; i++)
          {
          inputPtId = i + idx;
          if ( (*this->TerrainInfo)[inputPtId].TriangleId != VTK_VERTEX_INSERTED )
            {
            (*this->TerrainInfo)[inputPtId].TriangleId = tri;
            if ( (xR-xL) > 0 )
              {
              tt = (double)(i-xL) / (xR-xL);
              error = (1.0-tt)*hL + tt*hR;
              }
            else
              {
              error = hL;
              }
            error = fabs( (float)this->Heights->GetTuple1(inputPtId) - error );
            this->TerrainError->DeleteId(inputPtId);
            this->TerrainError->Insert(error,inputPtId);
            }
          }
        }
      if ( type == VTK_BOTTOM_TRIANGLE )
        {
        break;
        }

    case VTK_TOP_TRIANGLE:
      //Start scanning the upper triangle
      for (j=max[1]-1; j>midL[1]; j--) //for all scan lines; skip vertices
        {
        idx = j*this->Dimensions[0];
        t = (double)(j - midL[1]) / (max[1] - midL[1]);
        xL = t*max[0] + (1.0-t)*midL[0];
        xR = t*max[0] + (1.0-t)*midR[0];
        hL = t*hMax + (1.0-t)*hMidL;
        hR = t*hMax + (1.0-t)*hMidR;
        for (i=xL; i<=xR; i++)
          {
          inputPtId = i + idx;
          if ( (*this->TerrainInfo)[inputPtId].TriangleId != VTK_VERTEX_INSERTED )
            {
            (*this->TerrainInfo)[inputPtId].TriangleId = tri;
            if ( (xR-xL) > 0 )
              {
              tt = (double)(i-xL) / (xR-xL);
              error = (1.0-tt)*hL + tt*hR;
              }
            else
              {
              error = hL;
              }
            error = fabs( (float)this->Heights->GetTuple1(inputPtId) - error );
            this->TerrainError->DeleteId(inputPtId);
            this->TerrainError->Insert(error,inputPtId);
            }
          }
        }
      break;

    default:
      return;
    }
}


// Characterize the configuration of the triangle based on image coordinates
// (All points in triangulation are from an image).
//
int vtkGreedyTerrainDecimation::CharacterizeTriangle(int ij1[2], int ij2[2], int ij3[3],
                                                     int* &min, int* &max, int* &midL, int* &midR,
                                                     int* &mid, int mid2[2], float h[3], 
                                                     float &hMin, float &hMax, float &hL, 
                                                     float &hR)
{
  // Check for situations where one edge of triangle is horizontal
  //
  if ( ij1[1] == ij2[1] )
    {
    if ( ij1[0] < ij2[0] )
      {
      midL = ij1;
      midR = ij2;
      hL = h[0];
      hR = h[1];
      }
    else
      {
      midL = ij2;
      midR = ij1;
      hL = h[1];
      hR = h[0];
      }
    if( ij3[1] < ij1[1])
      {
      min = ij3;
      hMin = h[2];
      return VTK_BOTTOM_TRIANGLE;
      }
    else
      {
      max = ij3;
      hMax = h[2];
      return VTK_TOP_TRIANGLE;
      }
    }

  else if ( ij2[1] == ij3[1] )
    {
    if ( ij2[0] < ij3[0] )
      {
      midL = ij2;
      midR = ij3;
      hL = h[1];
      hR = h[2];
      }
    else
      {
      midL = ij3;
      midR = ij2;
      hL = h[2];
      hR = h[1];
      }
    if( ij1[1] < ij2[1])
      {
      min = ij1;
      hMin = h[0];
      return VTK_BOTTOM_TRIANGLE;
      }
    else
      {
      max = ij1;
      hMax = h[0];
      return VTK_TOP_TRIANGLE;
      }
    }

  else if ( ij3[1] == ij1[1] )
    {
    if ( ij3[0] < ij1[0] )
      {
      midL = ij3;
      midR = ij1;
      hL = h[2];
      hR = h[0];
      }
    else
      {
      midL = ij1;
      midR = ij3;
      hL = h[0];
      hR = h[2];
      }
    if( ij2[1] < ij3[1])
      {
      min = ij2;
      hMin = h[1];
      return VTK_BOTTOM_TRIANGLE;
      }
    else
      {
      max = ij2;
      hMax = h[1];
      return VTK_TOP_TRIANGLE;
      }
    }

  // Default situation (two triangles with no horizontal edges). 
  // Determine max, min and mid vertices.
  //
  // Find minimum
  if ( ij1[1] < ij2[1] )
    {
    if ( ij1[1] < ij3[1] )
      {
      min = ij1;
      hMin = h[0];
      }
    else
      {
      min = ij3;
      hMin = h[2];
      }
    }
  else
    {
    if ( ij2[1] < ij3[1] )
      {
      min = ij2;
      hMin = h[1];
      }
    else
      {
      min = ij3;
      hMin = h[2];
      }
    }

  // Find maximum
  if ( ij1[1] > ij2[1] )
    {
    if ( ij1[1] > ij3[1] )
      {
      max = ij1;
      hMax = h[0];
      }
    else
      {
      max = ij3;
      hMax = h[2];
      }
    }
  else
    {
    if ( ij2[1] > ij3[1] )
      {
      max = ij2;
      hMax = h[1];
      }
    else
      {
      max = ij3;
      hMax = h[2];
      }
    }

  // Find the midL and midR
  float hMid, hMid2;
  if ( ij1 != min && ij1 != max)
    {
    mid = ij1;
    hMid = h[0];
    }
  else if ( ij2 != min && ij2 != max)
    {
    mid = ij2;
    hMid = h[1];
    }
  else //if ( ij3 != min && ij2 != max)
    {
    mid = ij3;
    hMid = h[2];
    }

  // Computation of the intersection
  //
  mid2[1] = mid[1];
  double t = (double) (mid2[1] - min[1]) / (max[1] - min[1]);
  mid2[0] = (1.0-t)*min[0] + t*max[0] + 0.5; //rounding
  hMid2 = (1.0-t)*hMin + t*hMax;

  if ( mid[0] < mid2[0] )
    {
    midL = mid;
    midR = mid2;
    hL = hMid;
    hR = hMid2;
    }
  else
    {
    midL = mid2;
    midR = mid;
    hL = hMid2;
    hR = hMid;
    }

  return VTK_TWO_TRIANGLES;
}

void vtkGreedyTerrainDecimation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
 
  os << indent << "Error Measure: ";
  if ( this->ErrorMeasure == VTK_ERROR_NUMBER_OF_TRIANGLES )
    {
    os << "Number of triangles\n";
    os << indent << "Number of triangles: " << this->NumberOfTriangles << "\n";
    }
  else if ( this->ErrorMeasure == VTK_ERROR_SPECIFIED_REDUCTION )
    {
    os << "Specified reduction\n";
    os << indent << "Reduction: " << this->Reduction << "\n";
    }
  else if ( this->ErrorMeasure == VTK_ERROR_ABSOLUTE )
    {
    os << "Absolute\n";
    os << indent << "Absolute Error: " << this->AbsoluteError << "\n";
    }
  else // this->ErrorMeasure == VTK_ERROR_RELATIVE
    {
    os << "Relative\n";
    os << indent << "Relative Error: " << this->RelativeError << "\n";
    }
  
  os << indent << "BoundaryVertexDeletion: " 
     << (this->BoundaryVertexDeletion ? "On\n" : "Off\n");
}
