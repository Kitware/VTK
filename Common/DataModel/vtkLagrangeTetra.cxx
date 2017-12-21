/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangeTetra.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLagrangeTetra.h"

#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkLine.h"
#include "vtkLagrangeCurve.h"
#include "vtkLagrangeTriangle.h"
#include "vtkTetra.h"
#include "vtkDoubleArray.h"
#include "vtkPoints.h"

#define ENABLE_CACHING
#define FIFTEEN_POINT_TETRA

namespace
{
  // The linearized tetra is comprised of four linearized faces. Each face is
  // comprised of three vertices. These must be consistent with vtkTetra.
/*
  static const vtkIdType FaceVertices[4][3] = {{0,1,3}, {1,2,3},
                                               {2,0,3}, {0,2,1}};
*/

  // The linearized tetra is comprised of six linearized edges. Each edge is
  // comprised of two vertices. These must be consistent with vtkTetra.
  static const vtkIdType EdgeVertices[6][2] = {{0,1},{1,2},{2,0},
                                               {0,3},{1,3},{2,3}};

  // The barycentric coordinates of the four vertices of the linear tetra.
  static const vtkIdType LinearVertices[4][4] = {{0,0,0,1}, {1,0,0,0},
                                                 {0,1,0,0}, {0,0,1,0}};

  // When describing a linearized tetra face, there is a mapping between the
  // four-component barycentric tetra system and the three-component barycentric
  // triangle system. These are the relevant indices within the four-component
  // system for each face (e.g. face 0 varies across the barycentric tetra
  // coordinates 0, 2 and 3).
  static const vtkIdType FaceBCoords[4][3] = {{0,2,3}, {2,0,1},
                                              {2,1,3}, {1,0,3}};

  // When describing a linearized tetra face, there is a mapping between the
  // four-component barycentric tetra system and the three-component barycentric
  // triangle system. These are the constant indices within the four-component
  // system for each face (e.g. face 0 holds barycentric tetra coordinate 1
  // constant).
  static const vtkIdType FaceMinCoord[4] = {1,3,0,2};

  // Each linearized tetra edge holds two barycentric tetra coordinates constant
  // and varies the other two. These are the coordinates that are held constant
  // for each edge.
  static const vtkIdType EdgeMinCoords[6][2] = {{1,2},{2,3},{0,2},
                                                {0,1},{1,3},{0,3}};

  // The coordinate that increments when traversing an edge (i.e. the coordinate
  // of the nonzero component of the second vertex of the edge).
  static const vtkIdType EdgeCountingCoord[6] = {0,1,3,2,2,2};

  // When a linearized tetra vertex is cast into barycentric coordinates, one of
  // its coordinates is maximal and the other three are minimal. These are the
  // indices of the maximal barycentric coordinate for each vertex.
  static const vtkIdType VertexMaxCoords[4] = {3,0,1,2};

  // There are three different layouts for breaking an octahedron into four
  // tetras. given the six vertices of the octahedron, these are the layouts for
  // each of the three four-tetra configurations.
  static const int LinearTetras[3][4][4] =
    { { {2,0,1,4}, {2,1,5,4}, {2,5,3,4}, {2,3,0,4} },
      { {0,4,1,5}, {0,1,2,5}, {0,2,3,5}, {0,3,4,5} },
      { {1,5,2,3}, {1,2,0,3}, {1,0,4,3}, {1,4,5,3} } };

#ifdef FIFTEEN_POINT_TETRA
  double FifteenPointTetraCoords[15*3] = {0.,0.,0.,
                                          1.,0.,0.,
                                          0.,1.,0.,
                                          0.,0.,1.,
                                          .5,0.,0.,
                                          .5,.5,0.,
                                          0.,.5,0.,
                                          0.,0.,.5,
                                          .5,0.,.5,
                                          0.,.5,.5,
                                          1./3.,1./3.,0.,
                                          1./3.,0.,1./3.,
                                          1./3.,1./3,1./3.,
                                          0.,1./3.,1./3.,
                                          .25,.25,.25};

  static const vtkIdType FifteenPointTetraSubtetras[28][4] =
    {{0,4,10,14},{1,4,10,14},{1,5,10,14},{2,5,10,14},{2,6,10,14},{0,6,10,14},
     {0,7,11,14},{3,7,11,14},{3,8,11,14},{1,8,11,14},{1,4,11,14},{0,4,11,14},
     {1,5,12,14},{2,5,12,14},{2,9,12,14},{3,9,12,14},{3,8,12,14},{1,8,12,14},
     {0,7,13,14},{3,7,13,14},{3,9,13,14},{2,9,13,14},{2,6,13,14},{0,6,13,14}};
#endif
}

vtkStandardNewMacro(vtkLagrangeTetra);
//----------------------------------------------------------------------------
vtkLagrangeTetra::vtkLagrangeTetra() : ParametricCoordinates(nullptr)
{
  this->Order = 0;

  this->Edge = vtkLagrangeCurve::New();
  this->Face = vtkLagrangeTriangle::New();
  this->Tetra = vtkTetra::New();
  this->Scalars = vtkDoubleArray::New();
  this->Scalars->SetNumberOfTuples(4);

  this->Points->SetNumberOfPoints(4);
  this->PointIds->SetNumberOfIds(4);
  for (int i = 0; i < 4; i++)
    {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i,0);
    }
}

//----------------------------------------------------------------------------
vtkLagrangeTetra::~vtkLagrangeTetra()
{
  delete [] this->ParametricCoordinates;
  this->Edge->Delete();
  this->Face->Delete();
  this->Tetra->Delete();
  this->Scalars->Delete();
}

//----------------------------------------------------------------------------
vtkCell *vtkLagrangeTetra::GetEdge(int edgeId)
{
  vtkIdType order = this->GetOrder();

  vtkIdType bindex[4] = {0,0,0,0};
  bindex[EdgeVertices[edgeId][0]] = order;
  for (vtkIdType i=0;i<=order;i++)
    {
    this->EdgeIds[i] = this->PointIds->GetId(this->ToIndex(bindex));
    bindex[EdgeVertices[edgeId][0]]--;
    bindex[EdgeVertices[edgeId][1]]++;
    }
  this->Edge->vtkCell::Initialize(order + 1, this->EdgeIds, this->Points);
  return this->Edge;
}

//----------------------------------------------------------------------------
vtkCell *vtkLagrangeTetra::GetFace(int faceId)
{
  assert(faceId >= 0 && faceId < 4);

  vtkIdType order = this->GetOrder();

  vtkIdType nPoints = (order + 1)*(order + 2)/2;

#ifdef FIFTEEN_POINT_TETRA
  if (this->Points->GetNumberOfPoints() == 15)
    {
    nPoints = 7;
    }
#endif

  this->Face->GetPointIds()->SetNumberOfIds(nPoints);
  this->Face->GetPoints()->SetNumberOfPoints(nPoints);

  vtkIdType tetBCoords[4], triBCoords[3];
  for (vtkIdType p = 0; p < nPoints; p++)
    {
    vtkLagrangeTriangle::BarycentricIndex(p,triBCoords,order);

    for (vtkIdType coord = 0; coord < 3; coord++)
      {
      tetBCoords[FaceBCoords[faceId][coord]] = triBCoords[coord];
      }
    tetBCoords[FaceMinCoord[faceId]] = 0;

    vtkIdType pointIndex = vtkLagrangeTetra::Index(tetBCoords, order);
    this->Face->GetPointIds()->SetId(p,this->PointIds->GetId(pointIndex));
    this->Face->GetPoints()->SetPoint(p,this->Points->GetPoint(pointIndex));
    }

#ifdef FIFTEEN_POINT_TETRA
  if (this->Points->GetNumberOfPoints() == 15)
    {
    vtkIdType pointIndex = 10 + ((faceId + 1)%4);
    this->Face->GetPointIds()->SetId(6,this->PointIds->GetId(pointIndex));
    this->Face->GetPoints()->SetPoint(6,this->Points->GetPoint(pointIndex));
    }
#endif

  this->Face->Initialize();

  return this->Face;
}

//----------------------------------------------------------------------------
void vtkLagrangeTetra::Initialize()
{
  vtkIdType order = this->ComputeOrder();

  if (this->Order != order)
    {
    // Reset our caches
    this->Order = order;

    this->NumberOfSubtetras = this->ComputeNumberOfSubtetras();

#ifdef ENABLE_CACHING
    for (vtkIdType i = 0; i < this->GetPointIds()->GetNumberOfIds(); i++)
      {
      this->BarycentricIndexMap[4*i] = -1;
      }

    // we sacrifice memory for efficiency here
    vtkIdType nIndexMap = (this->Order+1)*(this->Order+1)*(this->Order+1);
    for (vtkIdType i = 0; i < nIndexMap; i++)
      {
      this->IndexMap[i] = -1;
      }

    vtkIdType nSubtetras = this->GetNumberOfSubtetras();
    for (vtkIdType i = 0; i < nSubtetras; i++)
      {
      this->SubtetraIndexMap[16*i] = -1;
      }
#endif
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkLagrangeTetra::ComputeNumberOfSubtetras()
{
#ifdef FIFTEEN_POINT_TETRA
  if (this->Points->GetNumberOfPoints() == 15)
    {
    return 28;
    }
#endif
  vtkIdType order = this->GetOrder();

  // # of rightside-up tetras: order*(order+1)*(order+2)/6
  // # of octahedra: (order-1)*order*(order+1)/6
  // # of upside-down tetras: (order-2)*(order-1)*order/6

  vtkIdType nRightSideUp = order*(order+1)*(order+2)/6;
  vtkIdType nOctahedra = (order-1)*order*(order+1)/6;
  vtkIdType nUpsideDown = (order > 2 ? (order-2)*(order-1)*order/6 : 0);

  return nRightSideUp + 4*nOctahedra + nUpsideDown;
}

//----------------------------------------------------------------------------
void vtkLagrangeTetra::SubtetraBarycentricPointIndices(
  vtkIdType cellIndex, vtkIdType (&pointBIndices)[4][4])
{
  // We tesselllate a tetrahedron into a tetrahedral-octahedral honeycomb, and
  // then discretize each octahedron into 4 tetrahedra. The pattern is as
  // follows: for each additional level in our tetrahedron (propagating
  // downwards in parametric z), a pattern of upside-down and rightside-up
  // triangles are formed. The rightside-up triangles form tetrahedra with the
  // single point above them, and the upside-down triangles form octahedra with
  // the righteside-up triangles above them.

  assert(cellIndex < this->GetNumberOfSubtetras());

#ifdef FIFTEEN_POINT_TETRA
  if (this->Points->GetNumberOfPoints() == 15)
    {
    pointBIndices[0][0] = FifteenPointTetraSubtetras[cellIndex][0];
    pointBIndices[1][0] = FifteenPointTetraSubtetras[cellIndex][1];
    pointBIndices[2][0] = FifteenPointTetraSubtetras[cellIndex][2];
    pointBIndices[3][0] = FifteenPointTetraSubtetras[cellIndex][3];
    return;
    }
#endif

#ifdef ENABLE_CACHING
    vtkIdType cellIndexStart = cellIndex * 16;
  if (this->SubtetraIndexMap[cellIndexStart] == -1)
#endif
    {
    vtkIdType order = this->GetOrder();

    if (order == 1)
      {
      for (vtkIdType i=0;i<4;i++)
        {
        for (vtkIdType j=0;j<4;j++)
          {
          pointBIndices[i][j] = LinearVertices[i][j];
          }
        }
      }
    else
      {
      vtkIdType nRightSideUp = order*(order+1)*(order+2)/6;
      vtkIdType nOctahedra = (order-1)*order*(order+1)/6;

      if (cellIndex < nRightSideUp)
        {
        // there are nRightSideUp subtetras whose orientation is the same as the
        // projected tetra. We traverse them here.
        vtkLagrangeTetra::BarycentricIndex(cellIndex,pointBIndices[0],order-1);

        pointBIndices[0][3] += 1;

        pointBIndices[1][0] = pointBIndices[0][0];
        pointBIndices[1][1] = pointBIndices[0][1] + 1;
        pointBIndices[1][2] = pointBIndices[0][2];
        pointBIndices[1][3] = pointBIndices[0][3] - 1;

        pointBIndices[2][0] = pointBIndices[0][0] + 1;
        pointBIndices[2][1] = pointBIndices[0][1];
        pointBIndices[2][2] = pointBIndices[0][2];
        pointBIndices[2][3] = pointBIndices[0][3] - 1;

        pointBIndices[3][0] = pointBIndices[0][0];
        pointBIndices[3][1] = pointBIndices[0][1];
        pointBIndices[3][2] = pointBIndices[0][2] + 1;
        pointBIndices[3][3] = pointBIndices[0][3] - 1;
        }
      else if (cellIndex < nRightSideUp + 4*nOctahedra)
       {
        // the next set of subtetras are embedded in octahedra, so we need to
        // identify and subdivide the octahedra. We traverse them here.
        cellIndex -= nRightSideUp;

        vtkIdType octIndex = cellIndex/4;
        vtkIdType tetIndex = cellIndex%4;

        vtkIdType octBIndices[6][4];

        if (order == 2)
          {
          octBIndices[2][0] = octBIndices[2][1] = octBIndices[2][2] =
            octBIndices[2][3] = 0;
          }
        else
          {
            vtkLagrangeTetra::BarycentricIndex(octIndex,octBIndices[2],order-2);
          }
        octBIndices[2][1] += 1;
        octBIndices[2][3] += 1;

        octBIndices[1][0] = octBIndices[2][0] + 1;
        octBIndices[1][1] = octBIndices[2][1];
        octBIndices[1][2] = octBIndices[2][2];
        octBIndices[1][3] = octBIndices[2][3] - 1;

        octBIndices[0][0] = octBIndices[2][0] + 1;
        octBIndices[0][1] = octBIndices[2][1] - 1;
        octBIndices[0][2] = octBIndices[2][2];
        octBIndices[0][3] = octBIndices[2][3];

        octBIndices[3][0] = octBIndices[0][0] - 1;
        octBIndices[3][1] = octBIndices[0][1];
        octBIndices[3][2] = octBIndices[0][2] + 1;
        octBIndices[3][3] = octBIndices[0][3];

        octBIndices[4][0] = octBIndices[3][0] + 1;
        octBIndices[4][1] = octBIndices[3][1];
        octBIndices[4][2] = octBIndices[3][2];
        octBIndices[4][3] = octBIndices[3][3] - 1;

        octBIndices[5][0] = octBIndices[3][0];
        octBIndices[5][1] = octBIndices[3][1] + 1;
        octBIndices[5][2] = octBIndices[3][2];
        octBIndices[5][3] = octBIndices[3][3] - 1;

        this->TetraFromOctahedron(tetIndex,octBIndices,pointBIndices);
        }
      else
        {
        // there are nUpsideDown subtetras whose orientation is inverted w.r.t.
        // the projected tetra. We traverse them here.
        cellIndex -= (nRightSideUp + 4*nOctahedra);

        if (order == 3)
          {
          pointBIndices[2][0] = pointBIndices[2][1] = pointBIndices[2][2] =
            pointBIndices[2][3] = 0;
          }
        else
          {
            vtkLagrangeTetra::BarycentricIndex(cellIndex, pointBIndices[2],
                                               order-3);
          }
        pointBIndices[2][0] += 1;
        pointBIndices[2][1] += 1;
        pointBIndices[2][3] += 1;

        pointBIndices[1][0] = pointBIndices[2][0] - 1;
        pointBIndices[1][1] = pointBIndices[2][1];
        pointBIndices[1][2] = pointBIndices[2][2] + 1;
        pointBIndices[1][3] = pointBIndices[2][3];

        pointBIndices[0][0] = pointBIndices[2][0];
        pointBIndices[0][1] = pointBIndices[2][1] - 1;
        pointBIndices[0][2] = pointBIndices[2][2] + 1;
        pointBIndices[0][3] = pointBIndices[2][3];

        pointBIndices[3][0] = pointBIndices[2][0];
        pointBIndices[3][1] = pointBIndices[2][1];
        pointBIndices[3][2] = pointBIndices[2][2] + 1;
        pointBIndices[3][3] = pointBIndices[2][3] - 1;
        }
      }

#ifdef ENABLE_CACHING
    for (vtkIdType i=0;i<4;i++)
      {
      for (vtkIdType j=0;j<4;j++)
        {
        this->SubtetraIndexMap[cellIndexStart + 4*i + j] = pointBIndices[i][j];
        }
      }
#endif
    }
#ifdef ENABLE_CACHING
  else
    {
    for (vtkIdType i=0;i<4;i++)
      {
      for (vtkIdType j=0;j<4;j++)
        {
        pointBIndices[i][j] = this->SubtetraIndexMap[cellIndexStart + 4*i + j];
        }
      }
    }
#endif
}

//----------------------------------------------------------------------------
void vtkLagrangeTetra::TetraFromOctahedron(
  vtkIdType cellIndex, const vtkIdType (&octBIndices)[6][4],
  vtkIdType (&tetraBIndices)[4][4])
{
  // TODO: intelligently select which of the three linearizations reduce
  // artifacts. For now, we always choose the first linearization.
  static vtkIdType linearization = 0;

  for (vtkIdType i = 0; i < 4; i++)
    {
    for (vtkIdType j = 0; j < 4; j++)
      {
      tetraBIndices[i][j] =
        octBIndices[LinearTetras[linearization][cellIndex][i]][j];
      }
    }
}

//----------------------------------------------------------------------------
int vtkLagrangeTetra::CellBoundary(int vtkNotUsed(subId), double pcoords[3],
                                   vtkIdList *pts)
{
  const double ijk = 1.0 - pcoords[0] - pcoords[1] - pcoords[2];
  int axis = 3;
  double dmin = ijk;
  for (int ii = 0; ii < 3; ++ii)
  {
    if (dmin > pcoords[ii])
    {
      axis = ii;
      dmin = pcoords[ii];
    }
  }

  const int closestFaceByAxis[4][3] = {
    { 0, 3, 2 },
    { 0, 1, 3 },
    { 0, 2, 1 },
    { 1, 2, 3 }
  };

  pts->SetNumberOfIds(3);
  for (int ii = 0; ii < 3; ++ii)
  {
    pts->SetId(ii, this->PointIds->GetId(closestFaceByAxis[axis][ii]));
  }

  return
    pcoords[0] < 0 || pcoords[0] > 1.0 ||
    pcoords[1] < 0 || pcoords[1] > 1.0 ||
    pcoords[2] < 0 || pcoords[2] > 1.0 ||
    ijk < 0 || ijk > 1.0 ?
    0 : 1;
}

//----------------------------------------------------------------------------
int vtkLagrangeTetra::EvaluatePosition(double* x, double* closestPoint,
                                       int& subId, double pcoords[3],
                                       double& minDist2, double *weights)
{
  double pc[3], dist2, tempWeights[3], closest[3];
  double pcoordsMin[3] = {0., 0., 0.};
  int returnStatus=0, status, ignoreId;
  vtkIdType minBIndices[4][4], bindices[4][4], pointIndices[4];

  vtkIdType order = this->GetOrder();
  vtkIdType numberOfSubtetras = this->GetNumberOfSubtetras();

  minDist2 = VTK_DOUBLE_MAX;
  for (vtkIdType subCellId = 0; subCellId < numberOfSubtetras; subCellId++)
    {
    this->SubtetraBarycentricPointIndices(subCellId, bindices);

    for (vtkIdType i=0;i<4;i++)
      {
      pointIndices[i] = this->ToIndex(bindices[i]);
      this->Tetra->Points->SetPoint(i, this->Points->GetPoint(pointIndices[i]));
      }

    status = this->Tetra->EvaluatePosition(x, closest, ignoreId, pc, dist2,
                                           tempWeights);

    if ( status != -1 && dist2 < minDist2 )
      {
      returnStatus = status;
      minDist2 = dist2;
      subId = subCellId;
      pcoordsMin[0] = pc[0];
      pcoordsMin[1] = pc[1];
      pcoordsMin[2] = pc[2];
      for (vtkIdType i=0;i<4;i++)
        {
        for (vtkIdType j=0;j<4;j++)
          {
          minBIndices[i][j] = bindices[i][j];
          }
        }
      }
    }

  // adjust parametric coordinates
  if ( returnStatus != -1 )
    {
    for (vtkIdType i=0;i<3;i++)
      {
      pcoords[i]= (minBIndices[0][i] +
                   pcoordsMin[0]*(minBIndices[1][i] - minBIndices[0][i]) +
                   pcoordsMin[1]*(minBIndices[2][i] - minBIndices[0][i]) +
                   pcoordsMin[2]*(minBIndices[3][i] - minBIndices[0][i]))/order;
      }

    if(closestPoint!=nullptr)
      {
      // Compute both closestPoint and weights
      this->EvaluateLocation(subId,pcoords,closestPoint,weights);
      }
    else
      {
      // Compute weights only
      this->InterpolateFunctions(pcoords,weights);
      }
    }

  return returnStatus;
}

//----------------------------------------------------------------------------
void vtkLagrangeTetra::EvaluateLocation(int& vtkNotUsed(subId),
                                        double pcoords[3], double x[3],
                                        double *weights)
{
  x[0] = x[1] = x[2] = 0.;

  this->InterpolateFunctions(pcoords,weights);

  double p[3];
  vtkIdType nPoints = this->GetPoints()->GetNumberOfPoints();
  for (vtkIdType idx = 0; idx < nPoints; idx++)
    {
    this->Points->GetPoint(idx,p);
    for (vtkIdType jdx = 0; jdx < 3; jdx++)
      {
      x[jdx] += p[jdx]*weights[idx];
      }
    }
}

//----------------------------------------------------------------------------
void vtkLagrangeTetra::Contour(double value,
                               vtkDataArray* cellScalars,
                               vtkIncrementalPointLocator* locator,
                               vtkCellArray *verts,
                               vtkCellArray* lines,
                               vtkCellArray* polys,
                               vtkPointData* inPd,
                               vtkPointData* outPd,
                               vtkCellData* inCd,
                               vtkIdType cellId,
                               vtkCellData* outCd)
{
  vtkIdType bindices[4][4];
  vtkIdType numberOfSubtetras = this->GetNumberOfSubtetras();

  for (vtkIdType subCellId = 0; subCellId < numberOfSubtetras; subCellId++)
    {
    this->SubtetraBarycentricPointIndices(subCellId, bindices);

    for (vtkIdType i=0;i<4;i++)
      {
      vtkIdType pointIndex = this->ToIndex(bindices[i]);
      this->Tetra->Points->SetPoint(i, this->Points->GetPoint(pointIndex));
      if ( outPd )
        {
        this->Tetra->PointIds->SetId(i, this->PointIds->GetId(pointIndex));
        }
      this->Scalars->SetTuple(i, cellScalars->GetTuple(pointIndex));
      }

    this->Tetra->Contour(value, this->Scalars, locator, verts,
                         lines, polys, inPd, outPd, inCd, cellId, outCd);

    }
}

//----------------------------------------------------------------------------
void vtkLagrangeTetra::Clip(double value,
                            vtkDataArray* cellScalars,
                            vtkIncrementalPointLocator* locator,
                            vtkCellArray* polys,
                            vtkPointData* inPd,
                            vtkPointData* outPd,
                            vtkCellData* inCd,
                            vtkIdType cellId,
                            vtkCellData* outCd,
                            int insideOut)
{
  vtkIdType bindices[4][4];
  vtkIdType numberOfSubtetras = this->GetNumberOfSubtetras();

  for (vtkIdType subCellId = 0; subCellId < numberOfSubtetras; subCellId++)
    {
    this->SubtetraBarycentricPointIndices(subCellId, bindices);

    for (vtkIdType i=0;i<4;i++)
      {
      vtkIdType pointIndex = this->ToIndex(bindices[i]);
      this->Tetra->Points->SetPoint(i, this->Points->GetPoint(pointIndex));
      if ( outPd )
        {
        this->Tetra->PointIds->SetId(i, this->PointIds->GetId(pointIndex));
        }
      this->Scalars->SetTuple(i, cellScalars->GetTuple(pointIndex));
      }

    this->Tetra->Clip(value, this->Scalars, locator, polys, inPd, outPd,
                      inCd, cellId, outCd, insideOut);
    }
}

//----------------------------------------------------------------------------
int vtkLagrangeTetra::IntersectWithLine(double* p1,
                                        double* p2,
                                        double tol,
                                        double& t,
                                        double* x,
                                        double* pcoords,
                                        int& subId)
{
  int subTest;

  t = VTK_DOUBLE_MAX;
  double tTmp;
  double xMin[3], pcoordsMin[3];

  for (int i=0;i<this->GetNumberOfFaces();i++)
    {
    if (this->GetFace(i)->IntersectWithLine(p1, p2, tol, tTmp, xMin, pcoordsMin,
                                            subTest) && tTmp < t)
      {
      for (vtkIdType j=0;j<3;j++)
        {
        x[j] = xMin[j];
        if (FaceBCoords[i][j] != 3)
          {
          pcoords[FaceBCoords[i][j]] = pcoordsMin[j];
          }
        }
      if (FaceMinCoord[i] != 3)
        {
        pcoords[FaceMinCoord[i]] = 0.;
        }
      t = tTmp;
      }
    }
  subId = 0;
  return (t == VTK_DOUBLE_MAX ? 0 : 1);
}

//----------------------------------------------------------------------------
int vtkLagrangeTetra::Triangulate(int vtkNotUsed(index), vtkIdList *ptIds,
                                  vtkPoints *pts)
{
  pts->Reset();
  ptIds->Reset();

  vtkIdType bindices[4][4];
  vtkIdType numberOfSubtetras = this->GetNumberOfSubtetras();

  pts->SetNumberOfPoints(4*numberOfSubtetras);
  ptIds->SetNumberOfIds(4*numberOfSubtetras);
  for (vtkIdType subCellId = 0; subCellId < numberOfSubtetras; subCellId++)
    {
    this->SubtetraBarycentricPointIndices(subCellId, bindices);

    for (vtkIdType i=0;i<4;i++)
      {
      vtkIdType pointIndex = this->ToIndex(bindices[i]);
      ptIds->SetId(4*subCellId+i,this->PointIds->GetId(pointIndex));
      pts->SetPoint(4*subCellId+i,this->Points->GetPoint(pointIndex));
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkLagrangeTetra::JacobianInverse(double pcoords[3], double**inverse,
                                       double* derivs)
{
  // Given parametric coordinates compute inverse Jacobian transformation
  // matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
  // function derivatives.

  int i, j, k;
  double *m[3], m0[3], m1[3], m2[3];
  double x[3];

  vtkIdType numberOfPoints = this->Points->GetNumberOfPoints();

  // compute interpolation function derivatives
  this->InterpolateDerivs(pcoords, derivs);

  // create Jacobian matrix
  m[0] = m0; m[1] = m1; m[2] = m2;
  for (i=0; i < 3; i++) //initialize matrix
    {
    m0[i] = m1[i] = m2[i] = 0.0;
    }

  for (j=0; j < numberOfPoints; j++)
    {
    this->Points->GetPoint(j, x);
    for (i=0; i < 3; i++)
      {
      for (k=0; k < this->GetCellDimension(); k++)
        {
        m[k][i] += x[i] * derivs[numberOfPoints*k + j];
        }
      }
    }

  // Compute third row vector in transposed Jacobian and normalize it, so that
  // Jacobian determinant stays the same.
  if (this->GetCellDimension() == 2)
    {
    vtkMath::Cross(m0,m1,m2);
    }

  if ( vtkMath::Normalize(m2) == 0.0 || !vtkMath::InvertMatrix(m,inverse,3))
    {
    vtkErrorMacro(<<"Jacobian inverse not found");
    return;
    }
}

//----------------------------------------------------------------------------
void vtkLagrangeTetra::Derivatives(int vtkNotUsed(subId),
                                   double pcoords[3],
                                   double* values,
                                   int dim,
                                   double *derivs)
{
  double *jI[3], j0[3], j1[3], j2[3];
  double fDs[(VTK_LAGRANGE_TETRAHEDRON_MAX_ORDER + 1) *
             (VTK_LAGRANGE_TETRAHEDRON_MAX_ORDER + 2) *
             (VTK_LAGRANGE_TETRAHEDRON_MAX_ORDER + 3)/2];
  double sum[3];
  int i, j, k;
  vtkIdType numberOfPoints = this->Points->GetNumberOfPoints();

  // compute inverse Jacobian and interpolation function derivatives
  jI[0] = j0; jI[1] = j1; jI[2] = j2;
  this->JacobianInverse(pcoords, jI, fDs);

  // now compute derivates of values provided
  for (k=0; k < dim; k++) //loop over values per vertex
    {
    sum[0] = sum[1] = sum[2] = 0.0;
    for ( i=0; i < numberOfPoints; i++) //loop over interp. function derivatives
      {
      sum[0] += fDs[i] * values[dim*i + k];
      sum[1] += fDs[numberOfPoints + i] * values[dim*i + k];
      }
    for (j=0; j < 3; j++) //loop over derivative directions
      {
      derivs[3*k + j] = 0.;
      for (i=0; i < this->GetCellDimension(); i++)
        {
        derivs[3*k + j] += sum[i]*jI[j][i];
        }
      }
    }
}

//----------------------------------------------------------------------------

double* vtkLagrangeTetra::GetParametricCoords()
{
#ifdef FIFTEEN_POINT_TETRA
  if (this->Points->GetNumberOfPoints() == 15)
    {
    return FifteenPointTetraCoords;
    }
#endif

  if (!this->ParametricCoordinates)
    {
    vtkIdType order = this->GetOrder();
    double order_d = static_cast<vtkIdType>(order);

    vtkIdType nPoints = (order + 1)*(order + 2)*(order + 3)/6;
    this->ParametricCoordinates = new double[3*nPoints];

    vtkIdType pIdx = 0;
    vtkIdType bindex[4];
    for (vtkIdType p = 0; p < nPoints; p++)
      {
      this->ToBarycentricIndex(p,bindex);
      this->ParametricCoordinates[pIdx] = bindex[0]/order_d;
      this->ParametricCoordinates[pIdx + 1] = bindex[1]/order_d;
      this->ParametricCoordinates[pIdx + 2] = bindex[2]/order_d;
      pIdx += 3;
      }
    }

  return this->ParametricCoordinates;
}

//----------------------------------------------------------------------------
int vtkLagrangeTetra::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = pcoords[2] = 0.25;
  return 0;
}

//----------------------------------------------------------------------------
double vtkLagrangeTetra::GetParametricDistance(double pcoords[3])
{
  int i;
  double pDist, pDistMax=0.0;
  double pc[4];

  pc[0] = pcoords[0];
  pc[1] = pcoords[1];
  pc[2] = pcoords[2];
  pc[3] = 1.0 - pcoords[0] - pcoords[1] - pcoords[2];

  for (i=0; i<4; i++)
    {
    if ( pc[i] < 0.0 )
      {
      pDist = -pc[i];
      }
    else if ( pc[i] > 1.0 )
      {
      pDist = pc[i] - 1.0;
      }
    else //inside the cell in the parametric direction
      {
      pDist = 0.0;
      }
    if ( pDist > pDistMax )
      {
      pDistMax = pDist;
      }
    }

  return pDistMax;
}

//----------------------------------------------------------------------------
void vtkLagrangeTetra::InterpolateFunctions(double pcoords[3], double* weights)
{
  // Adapted from P. Silvester, "High-Order Polynomial Triangular Finite
  // Elements for Potential Problems". Int. J. Engng Sci. Vol. 7, pp. 849-861.
  // Pergamon Press, 1969. The generic method is valid for all orders, but we
  // unroll the first two orders to reduce computational cost.

  double tau[4] = {pcoords[0], pcoords[1], pcoords[2],
                   1. - pcoords[0] - pcoords[1] - pcoords[2]};

  vtkIdType n = this->GetOrder();

  if (n == 1)
    {
    // for the linear case, we simply return the parametric coordinates, rotated
    // into the parametric frame (e.g. barycentric tau_3 = parametric x).
    weights[0] = tau[3];
    weights[1] = tau[0];
    weights[2] = tau[1];
    weights[3] = tau[2];
    }
  else if (n == 2)
    {
    if (this->GetPoints()->GetNumberOfPoints() == 15)
      {
      double u = tau[3], r = tau[0], s = tau[1], t = tau[2];
      double ur = u*r, us = u*s, ut = u*t, rs = r*s, rt = r*t, st = s*t;
      double urs = ur*s, urt = ur*t, ust = us*t, rst = rs*t;
      double urst = urs*t;

      weights[0] = u - 2.0*(ur + us + ut) + 3.0*(urs + urt + ust) - 4.0*urst;
      weights[1] = r - 2.0*(ur + rs + rt) + 3.0*(urs + urt + rst) - 4.0*urst;
      weights[2] = s - 2.0*(rs + us + st) + 3.0*(urs + rst + ust) - 4.0*urst;
      weights[3] = t - 2.0*(ut + rt + st) + 3.0*(urt + ust + rst) - 4.0*urst;
      weights[4] = 4.0*ur - 12.0*(urs + urt) + 32.0*urst;
      weights[5] = 4.0*rs - 12.0*(urs + rst) + 32.0*urst;
      weights[6] = 4.0*us - 12.0*(urs + ust) + 32.0*urst;
      weights[7] = 4.0*ut - 12.0*(urt + ust) + 32.0*urst;
      weights[8] = 4.0*rt - 12.0*(urt + rst) + 32.0*urst;
      weights[9] = 4.0*st - 12.0*(rst + ust) + 32.0*urst;
      weights[10] = 27.0*urs - 108.0*urst;
      weights[11] = 27.0*urt - 108.0*urst;
      weights[12] = 27.0*rst - 108.0*urst;
      weights[13] = 27.0*ust - 108.0*urst;
      weights[14] = 256.0*urst;
      return;
      }

    weights[0] = tau[3]*(2.0*tau[3] - 1.0);
    weights[1] = tau[0]*(2.0*tau[0] - 1.0);
    weights[2] = tau[1]*(2.0*tau[1] - 1.0);
    weights[3] = tau[2]*(2.0*tau[2] - 1.0);
    weights[4] = 4.0 * tau[3] * tau[0];
    weights[5] = 4.0 * tau[0] * tau[1];
    weights[6] = 4.0 * tau[1] * tau[3];
    weights[7] = 4.0 * tau[2] * tau[3];
    weights[8] = 4.0 * tau[0] * tau[2];
    weights[9] = 4.0 * tau[1] * tau[2];
    }
  else
    {
    vtkIdType nPoints = this->GetPoints()->GetNumberOfPoints();

    for (vtkIdType idx=0; idx < nPoints; idx++)
      {
      vtkIdType lambda[4];
      this->ToBarycentricIndex(idx,lambda);

      weights[idx] = (vtkLagrangeTriangle::eta(n, lambda[0], tau[0]) *
                      vtkLagrangeTriangle::eta(n, lambda[1], tau[1]) *
                      vtkLagrangeTriangle::eta(n, lambda[2], tau[2]) *
                      vtkLagrangeTriangle::eta(n, lambda[3], tau[3]));
      }
    }
}

//----------------------------------------------------------------------------
void vtkLagrangeTetra::InterpolateDerivs(double pcoords[3], double* derivs)
{
  // Analytic differentiation of the tetra shape functions, as adapted from
  // P. Silvester, "High-Order Polynomial Triangular Finite Elements for
  // Potential Problems". Int. J. Engng Sci. Vol. 7, pp. 849-861. Pergamon
  // Press, 1969. The generic method is valid for all orders, but we unroll the
  // first two orders to reduce computational cost.

  double tau[4] = {pcoords[0], pcoords[1], pcoords[2],
                   1. - pcoords[0] - pcoords[1] - pcoords[2]};

  vtkIdType n = this->GetOrder();

  if (n == 1)
    {
    derivs[0] = -1.0;
    derivs[1] = 1.0;
    derivs[2] = 0.0;
    derivs[3] = 0.0;
    derivs[4] = -1.0;
    derivs[5] = 0.0;
    derivs[6] = 1.0;
    derivs[7] = 0.0;
    derivs[8] = -1.0;
    derivs[9] = 0.0;
    derivs[10] = 0.0;
    derivs[11] = 1.0;
    }
  else if (n == 2)
    {
    if (this->GetPoints()->GetNumberOfPoints() == 15)
      {
      double r = tau[0], s = tau[1], t = tau[2], u = tau[3];
      double rs = r*s, rt = r*t, st = s*t;
      double umr = u-r, ums = u-s, umt = u-t;
      double *dWdr = &derivs[0], *dWds = &derivs[15], *dWdt = &derivs[30];

      dWdr[0] = 1.0 - 4.0*u + 3.0*((s + t)*umr - st) - 4.0*st*umr;
      dWdr[1] = 1.0 - 2.0*(umr + s + t) + 3.0*((s + t)*umr + st) - 4.0*st*umr;
      dWdr[2] = 3.0*s*umr - 4.0*st*umr;
      dWdr[3] = 3.0*t*umr - 4.0*st*umr;
      dWdr[4] = 4.0*umr - 12.0*umr*(s + t) + 32.0*st*umr;
      dWdr[5] = 4.0*s - 12.0*s*(umr + t) + 32.0*st*umr;
      dWdr[6] = -4.0*s - 12.0*s*(umr -t) + 32.0*st*umr;
      dWdr[7] = -4.0*t - 12.0*t*(umr - s) + 32.0*st*umr;
      dWdr[8] = 4.0*t - 12.0*t*(umr + s) + 32.0*st*umr;
      dWdr[9] = 32.0*st*umr;
      dWdr[10] = 27.0*s*umr - 108.0*st*umr;
      dWdr[11] = 27.0*t*umr - 108.0*st*umr;
      dWdr[12] = 27.0*st - 108.0*st*umr;
      dWdr[13] = -27.0*st - 108.0*st*umr;
      dWdr[14] = 256.0*st*umr;

      dWds[0] = 1.0 - 4.0*u + 3.0*((r + t)*ums -rt) - 4.0*rt*ums;
      dWds[1] = 3.0*r*ums - 4.0*rt*ums;
      dWds[2] = 1 - 2.0*(r + ums + t) + 3.0*((r + t)*ums + rt) - 4.0*rt*ums;
      dWds[3] = 3.0*t*ums - 4.0*rt*ums;
      dWds[4] = -4.0*r - 12.0*r*(ums - t) + 32.0*rt*ums;
      dWds[5] = 4.0*r - 12.0*r*(ums + t) + 32.0*rt*ums;
      dWds[6] = 4.0*ums - 12.0*ums*(r + t) + 32.0*rt*ums;
      dWds[7] = -4.0*t - 12.0*t*(ums - r) + 32.0*rt*ums;
      dWds[8] = 32.0*rt*ums;
      dWds[9] = 4.0*t - 12.0*t*(r + ums) + 32.0*rt*ums;
      dWds[10] = 27.0*r*ums - 108.0*rt*ums;
      dWds[11] = -27.0*rt - 108.0*rt*ums;
      dWds[12] = 27.0*rt - 108.0*rt*ums;
      dWds[13] = 27.0*t*ums - 108.0*rt*ums;
      dWds[14] = 256.0*rt*ums;

      dWdt[0] = 1.0 - 4.0*u + 3.0*((r + s)*umt - rs) - 4.0*rs*umt;
      dWdt[1] = 3.0*r*umt - 4.0*rs*umt;
      dWdt[2] = 3.0*s*umt - 4.0*rs*umt;
      dWdt[3] = 1 - 2.0*(umt + r + s) + 3.0*((r + s)*umt + rs) - 4.0*rs*umt;
      dWdt[4] = -4.0*r - 12.0*r*(umt - s) + 32.0*rs*umt;
      dWdt[5] = 32.0*rs*umt;
      dWdt[6] = -4.0*s - 12.0*s*(umt - r) + 32.0*rs*umt;
      dWdt[7] = 4.0*umt - 12.0*umt*(r + s) + 32.0*rs*umt;
      dWdt[8] = 4.0*r - 12.0*r*(umt + s) + 32.0*rs*umt;
      dWdt[9] = 4.0*s - 12.0*s*(r + umt) + 32.0*rs*umt;
      dWdt[10] = -27.0*rs - 108.0*rs*umt;
      dWdt[11] = 27.0*r*umt - 108.0*rs*umt;
      dWdt[12] = 27.0*rs - 108.0*rs*umt;
      dWdt[13] = 27.0*s*umt - 108.0*rs*umt;
      dWdt[14] = 256.0*rs*umt;

      return;
      }
    derivs[0] = 1.0 - 4.0*tau[3];
    derivs[1] = 4.0*tau[0] - 1.0;
    derivs[2] = 0.0;
    derivs[3] = 0.0;
    derivs[4] = 4.0*(tau[3] - tau[0]);
    derivs[5] = 4.0*tau[1];
    derivs[6] = -4.0*tau[1];
    derivs[7] = -4.0*tau[2];
    derivs[8] = 4.0*tau[2];
    derivs[9] = 0.0;
    derivs[10] = 1.0 - 4.0*tau[3];
    derivs[11] = 0.0;
    derivs[12] = 4.0*tau[1] - 1.0;
    derivs[13] = 0.0;
    derivs[14] = -4.0*tau[0];
    derivs[15] =  4.0*tau[0];
    derivs[16] = 4.0*(tau[3] - tau[1]);
    derivs[17] = -4.0*tau[2];
    derivs[18] = 0.0;
    derivs[19] =  4.0*tau[2];
    derivs[20] = 1.0 - 4.0*tau[3];
    derivs[21] = 0.0;
    derivs[22] = 0.0;
    derivs[23] = 4.0*tau[2] - 1.0;
    derivs[24] = -4.0*tau[0];
    derivs[25] = 0.0;
    derivs[26] = -4.0*tau[1];
    derivs[27] = 4.0*(tau[3] - tau[2]);
    derivs[28] = 4.0*tau[0];
    derivs[29] = 4.0*tau[1];
    }
  else
    {
    vtkIdType nPoints = this->GetPoints()->GetNumberOfPoints();

    for (vtkIdType idx = 0; idx < nPoints; idx++)
      {
      vtkIdType lambda[4];
      this->ToBarycentricIndex(idx,lambda);

      double eta_alpha = vtkLagrangeTriangle::eta(n,lambda[0],tau[0]);
      double eta_beta  = vtkLagrangeTriangle::eta(n,lambda[1],tau[1]);
      double eta_gamma = vtkLagrangeTriangle::eta(n,lambda[2],tau[2]);
      double eta_delta = vtkLagrangeTriangle::eta(n,lambda[3],tau[3]);

      double d_eta_alpha = vtkLagrangeTriangle::d_eta(n,lambda[0],tau[0]);
      double d_eta_beta  = vtkLagrangeTriangle::d_eta(n,lambda[1],tau[1]);
      double d_eta_gamma = vtkLagrangeTriangle::d_eta(n,lambda[2],tau[2]);
      double d_eta_delta = vtkLagrangeTriangle::d_eta(n,lambda[3],tau[3]);

      double d_f_d_tau1 = (d_eta_alpha*eta_beta*eta_gamma*eta_delta -
                           eta_alpha*eta_beta*eta_gamma*d_eta_delta);
      double d_f_d_tau2 = (eta_alpha*d_eta_beta*eta_gamma*eta_delta -
                           eta_alpha*eta_beta*eta_gamma*d_eta_delta);
      double d_f_d_tau3 = (eta_alpha*eta_beta*d_eta_gamma*eta_delta -
                           eta_alpha*eta_beta*eta_gamma*d_eta_delta);
      // double d_f_d_tau4 = (eta_alpha*eta_beta*eta_gamma*d_eta_delta -
      //                      d_eta_alpha*eta_beta*eta_gamma*eta_delta -
      //                      eta_alpha*d_eta_beta*eta_gamma*eta_delta -
      //                      eta_alpha*eta_beta*d_eta_gamma*eta_delta);

      derivs[idx] = d_f_d_tau1;
      derivs[nPoints + idx] = d_f_d_tau2;
      derivs[2*nPoints + idx] = d_f_d_tau3;
      }
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkLagrangeTetra::ComputeOrder()
{
  // when order = n, # points = (n+1)*(n+2)*(n+3)/6

#ifdef FIFTEEN_POINT_TETRA
  if (this->Points->GetNumberOfPoints() == 15)
    {
    return 2;
    }
#endif

  vtkIdType order = 1;
  vtkIdType nPoints = this->Points->GetNumberOfPoints();
  vtkIdType nPointsForOrder = 4;

  while (nPointsForOrder < nPoints)
  {
    order++;
    nPointsForOrder = (order+1)*(order+2)*(order+3)/6;
  }

  assert (nPoints == nPointsForOrder);

  return order;
}

//----------------------------------------------------------------------------
void vtkLagrangeTetra::ToBarycentricIndex(vtkIdType index,
                                          vtkIdType* bindex)
{
#ifdef ENABLE_CACHING
  if (this->BarycentricIndexMap[4*index] == -1)
    {
    vtkLagrangeTetra::BarycentricIndex(index,
                                       &this->BarycentricIndexMap[4*index],
                                       this->GetOrder());
    }
  for (vtkIdType i=0;i<4;i++)
    {
    bindex[i] = this->BarycentricIndexMap[4*index + i];
    }
#else
  return vtkLagrangeTetra::BarycentricIndex(index, bindex, this->GetOrder());
#endif
}

//----------------------------------------------------------------------------
vtkIdType vtkLagrangeTetra::ToIndex(const vtkIdType* bindex)
{
#ifdef FIFTEEN_POINT_TETRA
  if (this->Points->GetNumberOfPoints() == 15)
    {
    return bindex[0];
    }
#endif

#ifdef ENABLE_CACHING
  vtkIdType cacheIdx = ((this->Order+1)*(this->Order+1)*bindex[0] +
                        (this->Order+1)*bindex[1] + bindex[2]);
  if (this->IndexMap[cacheIdx] == -1)
    {
    this->IndexMap[cacheIdx] =
      vtkLagrangeTetra::Index(bindex, this->GetOrder());
    }
  return this->IndexMap[cacheIdx];
#else
  return vtkLagrangeTetra::Index(bindex, this->GetOrder());
#endif
}

//----------------------------------------------------------------------------
void vtkLagrangeTetra::BarycentricIndex(vtkIdType index, vtkIdType* bindex,
                                        vtkIdType order)
{
  // "Barycentric index" is a set of 4 integers, each running from 0 to
  // <Order>. It is the index of a point in the tetrahedron in barycentric
  // coordinates.

  assert(order >= 1);

  vtkIdType max = order;
  vtkIdType min = 0;

  // scope into the correct tetra
  while (index >= 2*(order*order + 1) && index != 0 && order > 3)
    {
    index -= 2*(order*order + 1);
    max-=3;
    min++;
    order -= 4;
    }

  if (index < 4)
    {
    // we are on a vertex
    for (vtkIdType coord = 0; coord < 4; coord++)
      {
      bindex[coord] = (coord == VertexMaxCoords[index] ? max : min);
      }
    return;
    }
  else if (index - 4 < 6*(order - 1))
    {
    // we are on an edge
    vtkIdType edgeId = (index - 4) / (order - 1);
    vtkIdType vertexId = (index - 4) % (order - 1);
    for (vtkIdType coord = 0; coord < 4; coord++)
      {
      bindex[coord] =
        min + (LinearVertices[EdgeVertices[edgeId][0]][coord] *
               (max - min - 1 - vertexId) +
               LinearVertices[EdgeVertices[edgeId][1]][coord] *
               (1 + vertexId));
      }
    return;
    }
  else
    {
    // we are on a face
    vtkIdType faceId = (index - 4 - 6*(order - 1))/((order - 2)*(order - 1)/2);
    vtkIdType vertexId = (index -4- 6*(order - 1))%((order - 2)*(order - 1)/2);

    vtkIdType projectedBIndex[3];
    if (order == 3)
      {
      projectedBIndex[0] = projectedBIndex[1] = projectedBIndex[2] = 0;
      }
    else
      {
        vtkLagrangeTriangle::BarycentricIndex(vertexId, projectedBIndex,
                                                order-3);
      }

      for (vtkIdType i = 0; i < 3; i++)
        {
        bindex[FaceBCoords[faceId][i]] = (min + 1 + projectedBIndex[i]);
        }
      bindex[FaceMinCoord[faceId]] = min;
      return;
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkLagrangeTetra::Index(const vtkIdType* bindex, vtkIdType order)
{
  vtkIdType index = 0;

  assert(order >= 1);
  assert(bindex[0] + bindex[1] + bindex[2] + bindex[3] == order);

  vtkIdType max = order;
  vtkIdType min = 0;

  vtkIdType bmin =
    std::min(std::min(std::min(bindex[0], bindex[1]), bindex[2]), bindex[3]);

  // scope into the correct tetra
  while (bmin > min)
    {
    index += 2*(order*order + 1);
    max-=3;
    min++;
    order -= 4;
    }

  for (vtkIdType vertex = 0; vertex < 4; vertex++)
    {
    if (bindex[VertexMaxCoords[vertex]] == max)
      {
      // we are on a vertex
      return index;
      }
    index++;
    }

  for (vtkIdType edge = 0; edge < 6; edge++)
    {
    if (bindex[EdgeMinCoords[edge][0]] == min &&
        bindex[EdgeMinCoords[edge][1]] == min)
      {
      // we are on an edge
      return index + bindex[EdgeCountingCoord[edge]] - (min + 1);
      }
    index += max - (min + 1);
    }

  for (vtkIdType face = 0; face < 4; face++)
    {
    if (bindex[FaceMinCoord[face]] == min)
      {
      // we are on a face
      vtkIdType projectedBIndex[3];
      for (vtkIdType i = 0; i < 3; i++)
        {
        projectedBIndex[i] = bindex[FaceBCoords[face][i]] - min;
        }
      // we must subtract the indices of the face's vertices and edges, which
      // total to 3*order
      return (index + vtkLagrangeTriangle::Index(projectedBIndex,order)
              - 3*order);
      }
    index += (order+1)*(order+2)/2 - 3*order;
    }
  return index;
}

//----------------------------------------------------------------------------
void vtkLagrangeTetra::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
