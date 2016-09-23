/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBridgeCell.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBridgeCell - Implementation of vtkGenericAdaptorCell
// .SECTION Description
// It is just an example that show how to implement the Generic. It is also
// used for testing and evaluating the Generic.
// .SECTION See Also
// vtkGenericAdaptorCell, vtkBridgeDataSet


#include "vtkBridgeCell.h"

#include <cassert>

#include "vtkBridgeCellIterator.h"
#include "vtkObjectFactory.h"
#include "vtkBridgeDataSet.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkGenericAttributeCollection.h"
#include "vtkBridgeAttribute.h"
#include "vtkContourValues.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkBridgePointIterator.h"

// All that stuff is for InterpolationFunction()

#include "vtkEmptyCell.h"
#include "vtkVertex.h"
#include "vtkPolyVertex.h"
#include "vtkLine.h"
#include "vtkPolyLine.h"
#include "vtkTriangle.h"
#include "vtkTriangleStrip.h"
#include "vtkQuad.h"
#include "vtkPixel.h"
#include "vtkPolygon.h"
#include "vtkTetra.h"
#include "vtkHexahedron.h"
#include "vtkVoxel.h"
#include "vtkWedge.h"
#include "vtkPyramid.h"

#include "vtkQuadraticEdge.h"
#include "vtkQuadraticTriangle.h"
#include "vtkQuadraticQuad.h"
#include "vtkQuadraticTetra.h"
#include "vtkQuadraticHexahedron.h"
#include "vtkBiQuadraticTriangle.h"
#include "vtkConvexPointSet.h"
# include "vtkPentagonalPrism.h"
# include "vtkHexagonalPrism.h"
# include "vtkQuadraticWedge.h"
# include "vtkQuadraticPyramid.h"



vtkStandardNewMacro(vtkBridgeCell);

//-----------------------------------------------------------------------------
void vtkBridgeCell::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//-----------------------------------------------------------------------------
// Description:
// Unique identification number of the cell over the whole
// data set. This unique key may not be contiguous.
vtkIdType vtkBridgeCell::GetId()
{
  return this->Id;
}

//-----------------------------------------------------------------------------
// Description:
// Does `this' a cell of a dataset? (otherwise, it is a boundary cell)
int vtkBridgeCell::IsInDataSet()
{
  return this->BoolIsInDataSet;
}

//-----------------------------------------------------------------------------
// Description:
// Type of the current cell.
// \post (result==VTK_HIGHER_ORDER_EDGE)||
//       (result==VTK_HIGHER_ORDER_TRIANGLE)||
//       (result==VTK_HIGHER_ORDER_TETRAHEDRON)
int vtkBridgeCell::GetType()
{
  int result=0;
  switch(this->Cell->GetCellType())
  {
    case VTK_TRIANGLE:
    case VTK_QUADRATIC_TRIANGLE:
    case VTK_BIQUADRATIC_TRIANGLE:
      result=VTK_HIGHER_ORDER_TRIANGLE;
      break;
    case VTK_QUAD:
    case VTK_QUADRATIC_QUAD:
      result=VTK_HIGHER_ORDER_QUAD;
      break;
    case VTK_TETRA:
    case VTK_QUADRATIC_TETRA:
      result=VTK_HIGHER_ORDER_TETRAHEDRON;
      break;
    case VTK_VOXEL:
    case VTK_HEXAHEDRON:
    case VTK_QUADRATIC_HEXAHEDRON:
      result=VTK_HIGHER_ORDER_HEXAHEDRON;
      break;
    case VTK_WEDGE:
    case VTK_QUADRATIC_WEDGE:
      result=VTK_HIGHER_ORDER_WEDGE;
      break;
    case VTK_PYRAMID:
    case VTK_QUADRATIC_PYRAMID:
      result=VTK_HIGHER_ORDER_PYRAMID;
      break;
    case VTK_PENTAGONAL_PRISM:
      assert("check: TODO" && 0);
      break;
    case VTK_HEXAGONAL_PRISM:
      assert("check: TODO" && 0);
      break;
    default:
      assert("check: impossible case" && 0);
      break;
  }
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Topological dimension of the current cell.
// \post valid_result: result>=0 && result<=3
int vtkBridgeCell::GetDimension()
{
  int result=this->Cell->GetCellDimension();
  assert("post: valid_result" && (result>=0)&&(result<=3));
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Interpolation order of the geometry.
// \post positive_result: result>=0
int vtkBridgeCell::GetGeometryOrder()
{
  int result;
  if(this->Cell->IsLinear())
  {
    result=1;
  }
  else
  {
    result=2; // GetOrder() is missing in vtkCell...
  }
  assert("post: positive_result" && result>=0);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Does the cell have no higher-order interpolation for geometry?
// \post definition: result==(GetGeometryOrder()==1)
int vtkBridgeCell::IsGeometryLinear()
{
  int result=this->Cell->IsLinear();
  assert("post: definition" && result==(GetGeometryOrder()==1));
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Interpolation order of attribute `a' on the cell (may differ by cell).
// \pre a_exists: a!=0
// \post positive_result: result>=0
int vtkBridgeCell::GetAttributeOrder(vtkGenericAttribute *vtkNotUsed(a))
{
  int result=this->GetGeometryOrder();
  assert("post: positive_result" && result>=0);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Does the attribute `a' have no higher-order interpolation for the cell?
// \pre a_exists: a!=0
// \post definition: result==(GetAttributeOrder()==1)
int vtkBridgeCell::IsAttributeLinear(vtkGenericAttribute *a)
{
  (void)a; // The attribute order is the order of the geometry.
  int result=this->IsGeometryLinear();
  assert("post: definition" && result==(GetAttributeOrder(a)==1));
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Is the cell primary (i.e. not composite) ?
int vtkBridgeCell::IsPrimary()
{
  return this->Cell->IsPrimaryCell();
}

//-----------------------------------------------------------------------------
// Description:
// Number of points that compose the cell.
// \post positive_result: result>=0
int vtkBridgeCell::GetNumberOfPoints()
{
  int result=this->Cell->GetNumberOfPoints();
  assert("post: positive_result" && result>=0);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Number of boundaries of dimension `dim' (or all dimensions less than
// GetDimension() if -1) of the cell.
// \pre valid_dim_range: (dim==-1) || ((dim>=0)&&(dim<GetDimension()))
// \post positive_result: result>=0
int vtkBridgeCell::GetNumberOfBoundaries(int dim)
{
  assert("pre: valid_dim_range" && ((dim==-1) ||((dim>=0)&&(dim<GetDimension()))));

  int result=0;
  if( (dim==0) && (this->GetDimension()>1) )
  {
    result += this->Cell->GetNumberOfPoints();
    if(!this->Cell->IsLinear())
    { // Old cell API treats mid-edge nodes as vertices; subtract those out:
      result -= this->Cell->GetNumberOfEdges();
    }
  }
  if( ((dim==-1) && (this->GetDimension()>1)) || (dim==1) )
  {
    result=result+this->Cell->GetNumberOfEdges();
  }
  if( ((dim==-1) && (this->GetDimension()>2)) || (dim==2) )
  {
    result=result+this->Cell->GetNumberOfFaces();
  }

  assert("post: positive_result" && result>=0);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Accumulated number of DOF nodes of the current cell. A DOF node is
// a component of cell with a given topological dimension. e.g.: a triangle
// has 7 DOF: 1 face, 3 edges, 3 vertices. An hexahedron has 27 DOF:
// 1 region, 6 faces, 12 edges, 8 vertices.
// \post valid_result: result==GetNumberOfBoundaries(-1)+1
int vtkBridgeCell::GetNumberOfDOFNodes()
{
  return this->GetNumberOfBoundaries(-1)+1;
}

//-----------------------------------------------------------------------------
// Description:
// Return the points of cell into `it'.
// \pre it_exists: it!=0
void vtkBridgeCell::GetPointIterator(vtkGenericPointIterator *it)
{
  assert("pre: it_exists" && it!=0);
  static_cast<vtkBridgePointIterator *>(it)->InitWithCell(this);
}

//-----------------------------------------------------------------------------
// Description:
// Create an empty cell iterator.
// \post result_exists: result!=0
vtkGenericCellIterator *vtkBridgeCell::NewCellIterator()
{
  vtkGenericCellIterator *result=vtkBridgeCellIterator::New();
  assert("post: result_exists" && result!=0);
  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Return in `boundaries' the cells of dimension `dim' (or all dimensions
// less than GetDimension() if -1) that are part of the boundary of the cell.
// \pre valid_dim_range: (dim==-1) || ((dim>=0)&&(dim<GetDimension()))
// \pre boundaries_exist: boundaries!=0
void vtkBridgeCell::GetBoundaryIterator(vtkGenericCellIterator *boundaries,
                                        int dim)
{
  assert("pre: valid_dim_range" && ((dim==-1) ||((dim>=0)&&(dim<GetDimension()))));
  assert("pre: boundaries_exist" && boundaries!=0);
  static_cast<vtkBridgeCellIterator *>(boundaries)->InitWithCellBoundaries(this,dim);
}

//-----------------------------------------------------------------------------
// Description:
// Number of cells (dimension>boundary->GetDimension()) of the dataset
// that share the boundary `boundary' of `this'.
// `this' IS NOT INCLUDED.
// \pre boundary_exists: boundary!=0
// \pre real_boundary: !boundary->IsInDataSet()
// \pre cell_of_the_dataset: IsInDataSet()
// \pre boundary: HasBoundary(boundary)
// \post positive_result: result>=0
int vtkBridgeCell::CountNeighbors(vtkGenericAdaptorCell *boundary)
{
  assert("pre: boundary_exists" && boundary!=0);
  assert("pre: real_boundary" && !boundary->IsInDataSet());
  assert("pre: cell_of_the_dataset" && IsInDataSet());

  vtkIdList *cells=vtkIdList::New();
  vtkBridgeCell *b=static_cast<vtkBridgeCell *>(boundary);
  vtkIdList *pts=b->Cell->GetPointIds();
  this->DataSet->Implementation->GetCellNeighbors(this->Id,pts,cells);
  int result=cells->GetNumberOfIds();
  cells->Delete();

  assert("post: positive_result" && result>=0);

  return result;
}

//-----------------------------------------------------------------------------
// \pre large_enough: GetDimension()>=2
// \pre right_size: sizeof(sharing)==GetNumberOfBoundaries(1);
void vtkBridgeCell::CountEdgeNeighbors(int *sharing)
{
  assert("pre: large_enough" && this->GetDimension()>=2);

  vtkIdType c=this->Cell->GetNumberOfEdges();
  vtkIdList *cells=vtkIdList::New();
  vtkIdType i=0;
  vtkCell *edge;
  vtkIdList *pts;

  while(i<c)
  {
    edge=this->Cell->GetEdge(i); // edge is deleted automatically by this->Cell
    pts=edge->GetPointIds();
    this->DataSet->Implementation->GetCellNeighbors(this->Id,pts,cells);
    sharing[i]=cells->GetNumberOfIds();
    ++i;
  }
  cells->Delete();
}

//-----------------------------------------------------------------------------
// Description:
// Put into `neighbors' the cells (dimension>boundary->GetDimension())
// of the dataset that share the boundary `boundary' of `this'.
// `this' IS NOT INCLUDED.
// \pre boundary_exists: boundary!=0
// \pre real_boundary: !boundary->IsInDataSet()
// \pre cell_of_the_dataset: IsInDataSet()
// \pre boundary: HasBoundary(boundary)
// \pre neighbors_exist: neighbors!=0
void vtkBridgeCell::GetNeighbors(vtkGenericAdaptorCell *boundary,
                                 vtkGenericCellIterator *neighbors)
{
  assert("pre: boundary_exists" && boundary!=0);
  assert("pre: real_boundary" && !boundary->IsInDataSet());
  assert("pre: cell_of_the_dataset" && IsInDataSet());
  assert("pre: neighbors_exist" && neighbors!=0);

  vtkIdList *cells=vtkIdList::New();
  vtkIdList *pts=static_cast<vtkBridgeCell *>(boundary)->Cell->GetPointIds();
  this->DataSet->Implementation->GetCellNeighbors(this->Id,pts,cells);

  static_cast<vtkBridgeCellIterator *>(neighbors)->InitWithCells(cells,this->DataSet);

  cells->Delete();
}

//-----------------------------------------------------------------------------
// Description:
// Compute the closest boundary of the current sub-cell `subId' for point
// `pcoord' (in parametric coordinates) in `boundary', and return whether
// the point is inside the cell or not. `boundary' is of dimension
// GetDimension()-1.
// \pre positive_subId: subId>=0
int vtkBridgeCell::FindClosestBoundary(int subId,
                                       double pcoords[3],
                                       vtkGenericCellIterator* &boundary)
{
  assert("pre: positive_subId" && subId>=0);

  vtkIdList *pts=vtkIdList::New();
  int result=this->Cell->CellBoundary(subId,pcoords,pts);
  static_cast<vtkBridgeCellIterator *>(boundary)->InitWithPoints(this->Cell->Points,pts,this->GetDimension()-1,0); // id of the boundary always 0?
  pts->Delete();

  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Is `x' inside the current cell? It also evaluate parametric coordinates
// `pcoords', sub-cell id `subId' (0 means primary cell), distance squared
// to the sub-cell in `dist2' and closest corner point `closestPoint'.
// `dist2' and `closestPoint' are not evaluated if `closestPoint'==0.
// If a numerical error occurred, -1 is returned and all other results
// should be ignored.
// \post valid_result: result==-1 || result==0 || result==1
// \post positive_distance: result!=-1 implies (closestPoint!=0 implies
//                                               dist2>=0)
int vtkBridgeCell::EvaluatePosition(double x[3],
                                    double *closestPoint,
                                    int &subId,
                                    double pcoords[3],
                                    double &dist2)
{
  this->AllocateWeights();
  int result=this->Cell->EvaluatePosition(x,closestPoint,subId,pcoords,dist2,
                                          this->Weights);

  if(result)
  {
    // clamp pcoords
    int i=0;
    while(i<3)
    {
      if(pcoords[i]<0)
      {
        pcoords[i]=0;
      }
      else if(pcoords[i]>1)
      {
        pcoords[i]=1;
      }
      ++i;
    }
  }

  assert("post: valid_result" && (result==-1 || result==0 || result==1));
  assert("post: positive_distance" && (!(result!=-1) || (!(closestPoint!=0)||dist2>=0))); // A=>B: !A || B
  return result;
}

//----------------------------------------------------------------------------
// Description:
// Determine global coordinates `x' from sub-cell `subId' and parametric
// coordinates `pcoords' in the cell.
// \pre positive_subId: subId>=0
// \pre clamped_pcoords: (0<=pcoords[0])&&(pcoords[0]<=1)&&(0<=pcoords[1])
// &&(pcoords[1]<=1)&&(0<=pcoords[2])&&(pcoords[2]<=1)
void vtkBridgeCell::EvaluateLocation(int subId,
                                     double pcoords[3],
                                     double x[3])
{
  assert("pre: positive_subId" && subId>=0);
  assert("pre: clamped_pcoords" && (0<=pcoords[0])&&(pcoords[0]<=1)
             &&(0<=pcoords[1])&&(pcoords[1]<=1)&&(0<=pcoords[2])
             &&(pcoords[2]<=1));

  this->AllocateWeights();
  this->Cell->EvaluateLocation(subId,pcoords,x,this->Weights);
}

//----------------------------------------------------------------------------
// Description:
// Interpolate the attribute `a' at local position `pcoords' of the cell into
// `val'.
// \pre a_exists: a!=0
// \pre a_is_point_centered: a->GetCentering()==vtkPointCentered
// \pre clamped_point: pcoords[0]>=0 && pcoords[0]<=1 && pcoords[1]>=0 &&
//                     pcoords[1]<=1 && pcoords[2]>=0 && pcoords[2]<=1
// \pre val_exists: val!=0
// \pre valid_size: sizeof(val)==a->GetNumberOfComponents()
void vtkBridgeCell::InterpolateTuple(vtkGenericAttribute *a, double pcoords[3],
                                     double *val)
{
  assert("pre: a_exists" && a!=0);
  assert("pre: a_is_point_centered" && a->GetCentering()==vtkPointCentered);
  assert("pre: clamped_point" && (pcoords[0]>=0 && pcoords[0]<=1
             && pcoords[1]>=0 && pcoords[1]<=1 && pcoords[2]>=0
                                  && pcoords[2]<=1));
  assert("pre: val_exists" && val!=0);

  vtkBridgeAttribute *ba = static_cast<vtkBridgeAttribute *>(a);

  int componentCount = a->GetNumberOfComponents();
  int ptCount = this->GetNumberOfPoints();

  if(a->GetCentering() == vtkPointCentered)
  {
    this->AllocateWeights();
    this->InterpolationFunctions(pcoords, this->Weights);

    memset(val,0, sizeof(double)*componentCount);
    for(int pt = 0; pt<ptCount; ++pt)
    {
      ba->Data->GetArray(ba->AttributeNumber)->
        GetTuple(this->Cell->GetPointId(pt),ba->InternalTuple);
      for(int component = 0; component<componentCount; ++component)
      {
        val[component] += ba->InternalTuple[component]*this->Weights[pt];
      }
    }
  }
  else // cell centered
  {
    // not need to interpolate
    ba->Data->GetArray(ba->AttributeNumber)->GetTuple(this->GetId(),val);
  }
}

//----------------------------------------------------------------------------
// Description:
// Interpolate the whole collection of attributes `c' at local position
// `pcoords' of the cell into `val'. Only point centered attributes are
// taken into account.
// \pre c_exists: c!=0
// \pre clamped_point: pcoords[0]>=0 && pcoords[0]<=1 && pcoords[1]>=0 &&
//                     pcoords[1]<=1 && pcoords[2]>=0 && pcoords[2]<=1
// \pre val_exists: val!=0
// \pre valid_size: sizeof(val)==c->GetNumberOfPointCenteredComponents()
void vtkBridgeCell::InterpolateTuple(vtkGenericAttributeCollection *c,
                                     double pcoords[3],
                                     double *val)
{
  assert("pre: c_exists" && c!=0);
  assert("pre: clamped_point" && (pcoords[0]>=0 && pcoords[0]<=1
             && pcoords[1]>=0 && pcoords[1]<=1 && pcoords[2]>=0
                                  && pcoords[2]<=1));
  assert("pre: val_exists" && val!=0);

///  assert("check: used!" && 0);

  double *p=val;
  int i=0;
  int count=c->GetNumberOfAttributes();
  while(i<count)
  {
    if(c->GetAttribute(i)->GetCentering()==vtkPointCentered)
    {
      this->InterpolateTuple(c->GetAttribute(i),pcoords,p);
      p=p+c->GetAttribute(i)->GetNumberOfComponents();
    }
    ++i;
  }
}

//-----------------------------------------------------------------------------
// Description:
// Is there an intersection between the current cell and the ray (`p1',`p2')
// according to a tolerance `tol'? If true, `x' is the global intersection,
// `t' is the parametric coordinate for the line, `pcoords' are the
// parametric coordinates for cell. `subId' is the sub-cell where
// the intersection occurs.
// \pre positive_tolerance: tol>0
int vtkBridgeCell::IntersectWithLine(double p1[3],
                                     double p2[3],
                                     double tol,
                                     double &t,
                                     double x[3],
                                     double pcoords[3],
                                     int &subId)
{
  return this->Cell->IntersectWithLine(p1,p2,tol,t,x,pcoords,subId);
}

//-----------------------------------------------------------------------------
// Description:
// Compute derivatives `derivs' of the attribute `attribute' (from its
// values at the corner points of the cell) given sub-cell `subId' (0 means
// primary cell) and parametric coordinates `pcoords'.
// Derivatives are in the x-y-z coordinate directions for each data value.
// \pre positive_subId: subId>=0
// \pre clamped_pcoords: (0<=pcoords[0])&&(pcoords[0]<=1)&&(0<=pcoords[1])
// &&(pcoords[1]<=1)&&(0<=pcoords[2])%%(pcoords[2]<=1)
// \pre attribute_exists: attribute!=0
// \pre derivs_exists: derivs!=0
// \pre valid_size: sizeof(derivs)>=attribute->GetNumberOfComponents()*3
void vtkBridgeCell::Derivatives(int subId,
                                double pcoords[3],
                                vtkGenericAttribute *attribute,
                                double *derivs)
{
  double *tuples =
    new double[attribute->GetNumberOfComponents()*this->GetNumberOfPoints()];
  attribute->GetTuple(this->InternalIterator,tuples);
  this->Cell->Derivatives(subId,pcoords,tuples,
                          attribute->GetNumberOfComponents(),derivs);
  delete [] tuples;
}

//----------------------------------------------------------------------------
// Description:
// Compute the bounding box of the current cell in `bounds' in global
// coordinates.
// THREAD SAFE
void vtkBridgeCell::GetBounds(double bounds[6])
{
  this->Cell->GetBounds(bounds);
}

//----------------------------------------------------------------------------
// Description:
// Return the bounding box of the current cell in global coordinates.
// NOT THREAD SAFE
// \post result_exists: result!=0
// \post valid_size: sizeof(result)>=6
double *vtkBridgeCell::GetBounds()
{
  return this->Cell->GetBounds();
}

//----------------------------------------------------------------------------
// Description:
// Bounding box diagonal squared of the current cell.
// \post positive_result: result>=0
double vtkBridgeCell::GetLength2()
{
  return this->Cell->GetLength2();
}

//----------------------------------------------------------------------------
// Description:
// Center of the current cell in parametric coordinates `pcoords'.
// If the current cell is a composite, the return value is the sub-cell id
// that the center is in.
// \post valid_result: (result>=0) && (IsPrimary() implies result==0)
int vtkBridgeCell::GetParametricCenter(double pcoords[3])
{
  return this->Cell->GetParametricCenter(pcoords);
}

//----------------------------------------------------------------------------
// Description:
// Distance of the parametric coordinate `pcoords' to the current cell.
// If inside the cell, a distance of zero is returned. This is used during
// picking to get the correct cell picked. (The tolerance will occasionally
// allow cells to be picked who are not really intersected "inside" the
// cell.)
// \post positive_result: result>=0
double vtkBridgeCell::GetParametricDistance(double pcoords[3])
{
  return this->Cell->GetParametricDistance(pcoords);
}

//----------------------------------------------------------------------------
// Description:
// Return a contiguous array of parametric coordinates of the points defining
// the current cell. In other words, (px,py,pz, px,py,pz, etc..) The
// coordinates are ordered consistent with the definition of the point
// ordering for the cell. Note that 3D parametric coordinates are returned
// no matter what the topological dimension of the cell. It includes the DOF
// nodes.
// \post valid_result_exists: ((IsPrimary()) && (result!=0)) ||
//                             ((!IsPrimary()) && (result==0))
//                     result!=0 implies sizeof(result)==GetNumberOfPoints()
double *vtkBridgeCell::GetParametricCoords()
{
  return this->Cell->GetParametricCoords();
}

// For the internals of the tesselation algorithm (the hash table in particular)
// Is the face `faceId' of the current cell on a exterior boundary of the
// dataset or not?
// \pre 3d: GetDimension()==3
//----------------------------------------------------------------------------
int vtkBridgeCell::IsFaceOnBoundary(vtkIdType faceId)
{
  assert("pre: 3d" && this->GetDimension()==3);

  // result=CountNeighbors(boundary(faceId))==0;

  vtkCell *face = this->Cell->GetFace(faceId);
  vtkIdList *cells = vtkIdList::New(); // expensive
  this->DataSet->Implementation->
    GetCellNeighbors(this->Id,face->GetPointIds(),cells);

  int result=cells->GetNumberOfIds()==0;
  cells->Delete(); // expensive
#if 0
  if(this->GetType()==VTK_QUADRATIC_TETRA)
  {
    if(result)
    {
      cout<<"************************************************ boundary"<<endl;
    }
    else
    {
      cout<<"************************************************ NOT boundary"<<endl;
    }
//    assert(result);
  }
#endif
  return result;
}

// Is the cell on the exterior boundary of the dataset?
// \pre 2d: GetDimension()==2
//----------------------------------------------------------------------------
int vtkBridgeCell::IsOnBoundary()
{
  assert("pre: 2d" && this->GetDimension()==2);
//  assert("check: TODO" && 0);
  return 1;
}

//----------------------------------------------------------------------------
// Description:
// Put into `id' the list of ids the point of the cell.
// \pre id_exists: id!=0
// \pre valid_size: sizeof(id)==GetNumberOfPoints();
void vtkBridgeCell::GetPointIds(vtkIdType *id)
{
  vtkIdType i=0;
  vtkIdList *l=this->Cell->GetPointIds();
  vtkIdType c=this->GetNumberOfBoundaries(0);
  while(i<c)
  {
     id[i]=l->GetId(i);
    ++i;
  }
}
//----------------------------------------------------------------------------
// Description:
// Return the ids of the vertices defining face `faceId'.
// \pre is_3d: this->GetDimension()==3
// \pre valid_faceId_range: faceId>=0 && faceId<this->GetNumberOfBoundaries(2)
// \post result_exists: result!=0
// \post valid_size: sizeof(result)>=GetNumberOfVerticesOnFace(faceId)
int *vtkBridgeCell::GetFaceArray(int faceId)
{
  assert("pre: is_3d" && this->GetDimension()==3);
  assert("pre: valid_faceId_range" && faceId>=0
    && faceId<this->GetNumberOfBoundaries(2));

  int *result = 0;

  switch(this->GetType())
  {
    case VTK_HIGHER_ORDER_TETRAHEDRON:
      result = vtkTetra::GetFaceArray(faceId);
      break;
    case VTK_HIGHER_ORDER_HEXAHEDRON:
      if(this->Cell->GetCellType()==VTK_VOXEL)
      {
        result = vtkVoxel::GetFaceArray(faceId);
      }
      else
      {
        result = vtkHexahedron::GetFaceArray(faceId);
      }
      break;
    case VTK_HIGHER_ORDER_WEDGE:
      result = vtkWedge::GetFaceArray(faceId);
      break;
    case VTK_HIGHER_ORDER_PYRAMID:
      result = vtkPyramid::GetFaceArray(faceId);
      break;
    case VTK_PENTAGONAL_PRISM:
      assert("check: TODO" && 0);
      break;
    case VTK_HEXAGONAL_PRISM:
      assert("check: TODO" && 0);
      break;
    default:
      assert("check: impossible case" && 0);
      break;
  }
  return result;
}

//----------------------------------------------------------------------------
// Description:
// Return the number of vertices defining face `faceId'
// \pre is_3d: this->GetDimension()==3
// \pre valid_faceId_range: faceId>=0 && faceId<this->GetNumberOfBoundaries(2)
// \post positive_result: && result>0
int vtkBridgeCell::GetNumberOfVerticesOnFace(int faceId)
{
  assert("pre: is_3d" && this->GetDimension()==3);
  assert("pre: valid_faceId_range" && faceId>=0
    && faceId<this->GetNumberOfBoundaries(2));

  int result = 0;

  switch(this->GetType())
  {
    case VTK_HIGHER_ORDER_TETRAHEDRON:
      result = 3;
      break;
    case VTK_HIGHER_ORDER_HEXAHEDRON:
      result = 4;
      break;
    case  VTK_HIGHER_ORDER_WEDGE:
      if(faceId <= 1) // triangle face
      {
        result = 3;
      }
      else // quad face
      {
        result = 4;
      }
      break;
    case VTK_HIGHER_ORDER_PYRAMID:
      if( faceId == 0)  // base
      {
        result = 4;
      }
      else // side
      {
        result = 3;
      }
      break;
#if 0 // TODO
    case VTK_PENTAGONAL_PRISM:
      if(faceId<=1)
      {
        result=4;
      }
      else
      {
        result=3;
      }
      break;
    case VTK_HEXAGONAL_PRISM:
       if(faceId<=1)
       {
        result=6;
       }
      else
      {
        result=4;
      }
      break;
#endif
    default:
      assert("check: impossible case" && 0);
      break;
  }

  assert("post: positive_result" && result>0);
  return result;
}

// copy/paste of vtkTriangle.cxx
static int triangleEdges[3][2] = { {0,1}, {1,2}, {2,0} };
static int quadEdges[4][2] = { {0,1}, {1,2}, {3,2}, {0,3} };

//----------------------------------------------------------------------------
// Description:
// Return the ids of the vertices defining edge `edgeId'.
// \pre valid_dimension: this->GetDimension()>=2
// \pre valid_edgeId_range: edgeId>=0 && edgeId<this->GetNumberOfBoundaries(1)
// \post result_exists: result!=0
// \post valid_size: sizeof(result)==2
int *vtkBridgeCell::GetEdgeArray(int edgeId)
{
  assert("pre: valid_dimension" && this->GetDimension()>=2);
  assert("pre: valid_faceId_range" && edgeId>=0
    && edgeId<this->GetNumberOfBoundaries(1));

  int *result = 0;

  switch(this->GetType())
  {
    case VTK_HIGHER_ORDER_TRIANGLE:
      result = triangleEdges[edgeId];
      break;
    case VTK_HIGHER_ORDER_QUAD:
      result = quadEdges[edgeId];
      break;
    case VTK_HIGHER_ORDER_TETRAHEDRON:
      result = vtkTetra::GetEdgeArray(edgeId);
      break;
    case VTK_HIGHER_ORDER_HEXAHEDRON:
      if(this->Cell->GetCellType()==VTK_VOXEL)
      {
        result = vtkVoxel::GetEdgeArray(edgeId);
      }
      else
      {
        result = vtkHexahedron::GetEdgeArray(edgeId);
      }
      break;
    case VTK_HIGHER_ORDER_WEDGE:
      result = vtkWedge::GetEdgeArray(edgeId);
      break;
    case VTK_HIGHER_ORDER_PYRAMID:
      result = vtkPyramid::GetEdgeArray(edgeId);
      break;
    case VTK_PENTAGONAL_PRISM:
      assert("check: TODO" && 0);
      break;
    case VTK_HEXAGONAL_PRISM:
      assert("check: TODO" && 0);
      break;
    default:
      assert("check: impossible case" && 0);
      break;
  }

  return result;
}

//----------------------------------------------------------------------------
// Description:
// Used internally for the Bridge.
// Initialize the cell from a dataset `ds' and `cellid'.
// \pre ds_exists: ds!=0
// \pre valid_cellid: (cellid>=0) && (cellid<ds->GetNumberOfCells())
void vtkBridgeCell::Init(vtkBridgeDataSet *ds,
                         vtkIdType cellid)
{
  assert("pre: ds_exists" && ds!=0);
  assert("pre: valid_cellid" && (cellid>=0)
         && (cellid<ds->GetNumberOfCells()));

  vtkSetObjectBodyMacro(DataSet,vtkBridgeDataSet,ds);
  vtkCell *tmp = ds->Implementation->GetCell(cellid);
  vtkSetObjectBodyMacro(Cell,vtkCell,tmp);
  this->Id = cellid;
  this->BoolIsInDataSet = 1;
  if(this->InternalIterator == 0)
  {
    this->InternalIterator = vtkBridgeCellIterator::New();
  }
  this->InternalIterator->InitWithOneCell(this);

  this->InternalIterator->Begin();
}

//----------------------------------------------------------------------------
// Description:
// Used internally for the Bridge.
// Initialize the cell from a cell `c' and an `id'.
// \pre c_exists: c!=0
void vtkBridgeCell::InitWithCell(vtkCell *c, vtkIdType id)
{
  assert("pre: c_exists" && c!=0);

  vtkSetObjectBodyMacro(DataSet,vtkBridgeDataSet,0);
  this->Id = id;

  // warning: do directly vtkSetObjectBodyMacro(Cell,vtkCell,c->NewInstance())
  // add memory leak because the text "c->NewInstance()" is copied several
  // time in the macro...

  if(this->Cell)
  {
    this->Cell->Delete();
  }
  this->Cell = c->NewInstance();

  this->Cell->DeepCopy(c);
  this->BoolIsInDataSet=0;

  if(this->InternalIterator==0)
  {
    this->InternalIterator=vtkBridgeCellIterator::New();
  }
  this->InternalIterator->InitWithOneCell(this);
  this->InternalIterator->Begin();
}

//----------------------------------------------------------------------------
// Description:
// Recursive copy of `other' into `this'.
// \pre other_exists: other!=0
// \pre other_differ: this!=other
void vtkBridgeCell::DeepCopy(vtkBridgeCell *other)
{
  assert("pre: other_exists" && other!=0);
  assert("pre: other_differ" && this!=other);

  vtkCell *tmp;

  if(this->InternalIterator==0)
  {
    this->InternalIterator=vtkBridgeCellIterator::New();
  }
  this->Id = other->Id;
  this->BoolIsInDataSet = other->BoolIsInDataSet;
  if(other->BoolIsInDataSet)
  {
    vtkSetObjectBodyMacro(DataSet,vtkBridgeDataSet,other->DataSet);
    tmp = this->DataSet->Implementation->GetCell(this->Id);
    vtkSetObjectBodyMacro(Cell,vtkCell,tmp);
    this->InternalIterator->InitWithOneCell(this);
    this->InternalIterator->Begin();
  }
  else
  {
    vtkSetObjectBodyMacro(DataSet,vtkBridgeDataSet,0);
    tmp = other->Cell->NewInstance();
    vtkSetObjectBodyMacro(Cell,vtkCell,tmp);
    this->Cell->Delete(); // because newinstance+macro=2 ref
    this->Cell->DeepCopy(other->Cell);
    this->InternalIterator->InitWithOneCell(this);
    this->InternalIterator->Begin();
  }
  this->Modified();
}

//----------------------------------------------------------------------------
vtkBridgeCell::vtkBridgeCell()
{
  this->DataSet = 0;
  this->InternalIterator = 0; // we cannot create the cell iterator here
  // because we will have an infinite recursion: a cell creates a
  // celliterator which creates a cell, which creates a celliterator ...
  this->Cell = 0;
  this->BoolIsInDataSet = 0;
  this->Id = -1000; // magic ?

  this->Weights = 0;
  this->WeightsCapacity = 0;
}

//----------------------------------------------------------------------------
vtkBridgeCell::~vtkBridgeCell()
{
  vtkSetObjectBodyMacro(DataSet,vtkBridgeDataSet,0);
  vtkSetObjectBodyMacro(InternalIterator,vtkBridgeCellIterator,0);
  vtkSetObjectBodyMacro(Cell,vtkCell,0);

  delete[] this->Weights;
}

//----------------------------------------------------------------------------
// Description:
// Allocate an array for the weights, only if it does not exist yet or if
// the capacity is too small.
void vtkBridgeCell::AllocateWeights()
{
  if( this->Weights != 0
    && this->WeightsCapacity < this->GetNumberOfPoints() )
  {
    delete[] this->Weights;
    this->Weights = 0;
  }
  if(this->Weights == 0)
  {
    this->Weights = new double[this->GetNumberOfPoints()];
    this->WeightsCapacity = this->GetNumberOfPoints();
  }
}

//----------------------------------------------------------------------------
// Description:
// Compute the weights for parametric coordinates `pcoords'.
void vtkBridgeCell::InterpolationFunctions(double pcoords[3], double *weights)
{
  this->Cell->InterpolateFunctions(pcoords, weights);
}
