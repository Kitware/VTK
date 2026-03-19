// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkQuadraticHexahedron.h"

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkHexahedron.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkQuadraticEdge.h"
#include "vtkQuadraticQuad.h"

#include <algorithm> //std::copy

namespace
{
//------------------------------------------------------------------------------
[[maybe_unused]] constexpr const char* Topology = R"(
   QuadraticHexahedron topology:

             7------14------6
            /|             /|
          15 |           13 |
         /   19         /   18
        4------12------5    |
        |    |         |    |
        |    3-----10--|----2
       16   /          17  /
        |  11          |  9
        | /            | /
        0------8-------1
)";

//------------------------------------------------------------------------------
constexpr vtkNonLinearCell3D::PointType PointTypes[20] = {
  vtkNonLinearCell3D::PointType::CornerPoint,  // point 0
  vtkNonLinearCell3D::PointType::CornerPoint,  // point 1
  vtkNonLinearCell3D::PointType::CornerPoint,  // point 2
  vtkNonLinearCell3D::PointType::CornerPoint,  // point 3
  vtkNonLinearCell3D::PointType::CornerPoint,  // point 4
  vtkNonLinearCell3D::PointType::CornerPoint,  // point 5
  vtkNonLinearCell3D::PointType::CornerPoint,  // point 6
  vtkNonLinearCell3D::PointType::CornerPoint,  // point 7
  vtkNonLinearCell3D::PointType::EdgeMidPoint, // point 8
  vtkNonLinearCell3D::PointType::EdgeMidPoint, // point 9
  vtkNonLinearCell3D::PointType::EdgeMidPoint, // point 10
  vtkNonLinearCell3D::PointType::EdgeMidPoint, // point 11
  vtkNonLinearCell3D::PointType::EdgeMidPoint, // point 12
  vtkNonLinearCell3D::PointType::EdgeMidPoint, // point 13
  vtkNonLinearCell3D::PointType::EdgeMidPoint, // point 14
  vtkNonLinearCell3D::PointType::EdgeMidPoint, // point 15
  vtkNonLinearCell3D::PointType::EdgeMidPoint, // point 16
  vtkNonLinearCell3D::PointType::EdgeMidPoint, // point 17
  vtkNonLinearCell3D::PointType::EdgeMidPoint, // point 18
  vtkNonLinearCell3D::PointType::EdgeMidPoint, // point 19
};

//------------------------------------------------------------------------------
double ParametricCoords[60] = {
  0.0, 0.0, 0.0, //
  1.0, 0.0, 0.0, //
  1.0, 1.0, 0.0, //
  0.0, 1.0, 0.0, //
  0.0, 0.0, 1.0, //
  1.0, 0.0, 1.0, //
  1.0, 1.0, 1.0, //
  0.0, 1.0, 1.0, //
  0.5, 0.0, 0.0, //
  1.0, 0.5, 0.0, //
  0.5, 1.0, 0.0, //
  0.0, 0.5, 0.0, //
  0.5, 0.0, 1.0, //
  1.0, 0.5, 1.0, //
  0.5, 1.0, 1.0, //
  0.0, 0.5, 1.0, //
  0.0, 0.0, 0.5, //
  1.0, 0.0, 0.5, //
  1.0, 1.0, 0.5, //
  0.0, 1.0, 0.5  //
};

//------------------------------------------------------------------------------
constexpr double MidPoints[7][3] = {
  { 0.0, 0.5, 0.5 }, // 20
  { 1.0, 0.5, 0.5 }, // 21
  { 0.5, 0.0, 0.5 }, // 22
  { 0.5, 1.0, 0.5 }, // 23
  { 0.5, 0.5, 0.0 }, // 24
  { 0.5, 0.5, 1.0 }, // 25
  { 0.5, 0.5, 0.5 }, // 26
};

//------------------------------------------------------------------------------
constexpr vtkIdType Edges[12][3] = {
  { 0, 1, 8 },
  { 1, 2, 9 },
  { 3, 2, 10 },
  { 0, 3, 11 },
  { 4, 5, 12 },
  { 5, 6, 13 },
  { 7, 6, 14 },
  { 4, 7, 15 },
  { 0, 4, 16 },
  { 1, 5, 17 },
  { 3, 7, 19 },
  { 2, 6, 18 },
};

//------------------------------------------------------------------------------
constexpr vtkIdType Faces[6][8] = {
  { 0, 4, 7, 3, 16, 15, 19, 11 },
  { 1, 2, 6, 5, 9, 18, 13, 17 },
  { 0, 1, 5, 4, 8, 17, 12, 16 },
  { 3, 7, 6, 2, 19, 14, 18, 10 },
  { 0, 3, 2, 1, 11, 10, 9, 8 },
  { 4, 5, 6, 7, 12, 13, 14, 15 },
};

//------------------------------------------------------------------------------
constexpr vtkIdType EdgeToAdjacentFaces[12][2] = {
  { 2, 4 }, // edge 0:  corners 0,1 — faces 2 and 4
  { 1, 4 }, // edge 1:  corners 1,2 — faces 1 and 4
  { 3, 4 }, // edge 2:  corners 3,2 — faces 3 and 4
  { 0, 4 }, // edge 3:  corners 0,3 — faces 0 and 4
  { 2, 5 }, // edge 4:  corners 4,5 — faces 2 and 5
  { 1, 5 }, // edge 5:  corners 5,6 — faces 1 and 5
  { 3, 5 }, // edge 6:  corners 7,6 — faces 3 and 5
  { 0, 5 }, // edge 7:  corners 4,7 — faces 0 and 5
  { 0, 2 }, // edge 8:  corners 0,4 — faces 0 and 2
  { 1, 2 }, // edge 9:  corners 1,5 — faces 1 and 2
  { 0, 3 }, // edge 10: corners 3,7 — faces 0 and 3
  { 1, 3 }, // edge 11: corners 2,6 — faces 1 and 3
};

//------------------------------------------------------------------------------
constexpr vtkIdType FaceToAdjacentFaces[6][4] = {
  { 2, 3, 4, 5 }, // face 0: shares edge with faces 2(0,4), 3(3,7), 4(0,3), 5(4,7)
  { 2, 3, 4, 5 }, // face 1: shares edge with faces 2(1,5), 3(2,6), 4(1,2), 5(5,6)
  { 0, 1, 4, 5 }, // face 2: shares edge with faces 0(0,4), 1(1,5), 4(0,1), 5(4,5)
  { 0, 1, 4, 5 }, // face 3: shares edge with faces 0(3,7), 1(2,6), 4(3,2), 5(7,6)
  { 0, 1, 2, 3 }, // face 4: shares edge with faces 0(0,3), 1(1,2), 2(0,1), 3(3,2)
  { 0, 1, 2, 3 }, // face 5: shares edge with faces 0(4,7), 1(5,6), 2(4,5), 3(7,6)
};

//------------------------------------------------------------------------------
constexpr vtkIdType PointToIncidentEdges[20][3] = {
  { 0, 3, 8 },    // point 0:  corner
  { 0, 1, 9 },    // point 1:  corner
  { 1, 2, 11 },   // point 2:  corner
  { 2, 3, 10 },   // point 3:  corner
  { 4, 7, 8 },    // point 4:  corner
  { 4, 5, 9 },    // point 5:  corner
  { 5, 6, 11 },   // point 6:  corner
  { 6, 7, 10 },   // point 7:  corner
  { 0, -1, -1 },  // point 8:  mid-edge
  { 1, -1, -1 },  // point 9:  mid-edge
  { 2, -1, -1 },  // point 10: mid-edge
  { 3, -1, -1 },  // point 11: mid-edge
  { 4, -1, -1 },  // point 12: mid-edge
  { 5, -1, -1 },  // point 13: mid-edge
  { 6, -1, -1 },  // point 14: mid-edge
  { 7, -1, -1 },  // point 15: mid-edge
  { 8, -1, -1 },  // point 16: mid-edge
  { 9, -1, -1 },  // point 17: mid-edge
  { 11, -1, -1 }, // point 18: mid-edge
  { 10, -1, -1 }, // point 19: mid-edge
};

//------------------------------------------------------------------------------
constexpr vtkIdType PointToIncidentFaces[20][3] = {
  { 0, 2, 4 },  // point 0:  corner,   3 faces
  { 1, 2, 4 },  // point 1:  corner,   3 faces
  { 1, 3, 4 },  // point 2:  corner,   3 faces
  { 0, 3, 4 },  // point 3:  corner,   3 faces
  { 0, 2, 5 },  // point 4:  corner,   3 faces
  { 1, 2, 5 },  // point 5:  corner,   3 faces
  { 1, 3, 5 },  // point 6:  corner,   3 faces
  { 0, 3, 5 },  // point 7:  corner,   3 faces
  { 2, 4, -1 }, // point 8:  mid-edge, 2 faces
  { 1, 4, -1 }, // point 9:  mid-edge, 2 faces
  { 3, 4, -1 }, // point 10: mid-edge, 2 faces
  { 0, 4, -1 }, // point 11: mid-edge, 2 faces
  { 2, 5, -1 }, // point 12: mid-edge, 2 faces
  { 1, 5, -1 }, // point 13: mid-edge, 2 faces
  { 3, 5, -1 }, // point 14: mid-edge, 2 faces
  { 0, 5, -1 }, // point 15: mid-edge, 2 faces
  { 0, 2, -1 }, // point 16: mid-edge, 2 faces
  { 1, 2, -1 }, // point 17: mid-edge, 2 faces
  { 1, 3, -1 }, // point 18: mid-edge, 2 faces
  { 0, 3, -1 }, // point 19: mid-edge, 2 faces
};

//------------------------------------------------------------------------------
constexpr vtkIdType PointToOneRingPoints[20][6] = {
  { 4, 16, 3, 11, 1, 8 },   // point 0:  corner
  { 0, 8, 2, 9, 5, 17 },    // point 1:  corner
  { 1, 9, 3, 10, 6, 18 },   // point 2:  corner
  { 2, 10, 0, 11, 7, 19 },  // point 3:  corner
  { 5, 12, 7, 15, 0, 16 },  // point 4:  corner
  { 1, 17, 6, 13, 4, 12 },  // point 5:  corner
  { 2, 18, 7, 14, 5, 13 },  // point 6:  corner
  { 3, 19, 4, 15, 6, 14 },  // point 7:  corner
  { 0, 1, -1, -1, -1, -1 }, // point 8:  mid-edge
  { 1, 2, -1, -1, -1, -1 }, // point 9:  mid-edge
  { 3, 2, -1, -1, -1, -1 }, // point 10: mid-edge
  { 0, 3, -1, -1, -1, -1 }, // point 11: mid-edge
  { 4, 5, -1, -1, -1, -1 }, // point 12: mid-edge
  { 5, 6, -1, -1, -1, -1 }, // point 13: mid-edge
  { 7, 6, -1, -1, -1, -1 }, // point 14: mid-edge
  { 4, 7, -1, -1, -1, -1 }, // point 15: mid-edge
  { 0, 4, -1, -1, -1, -1 }, // point 16: mid-edge
  { 1, 5, -1, -1, -1, -1 }, // point 17: mid-edge
  { 2, 6, -1, -1, -1, -1 }, // point 18: mid-edge
  { 3, 7, -1, -1, -1, -1 }, // point 19: mid-edge
};

//------------------------------------------------------------------------------
constexpr vtkIdType LinearCells[8][8] = {
  { 0, 8, 24, 11, 16, 22, 26, 20 },
  { 8, 1, 9, 24, 22, 17, 21, 26 },
  { 11, 24, 10, 3, 20, 26, 23, 19 },
  { 24, 9, 2, 10, 26, 21, 18, 23 },
  { 16, 22, 26, 20, 4, 12, 25, 15 },
  { 22, 17, 21, 26, 12, 5, 13, 25 },
  { 20, 26, 23, 19, 15, 25, 14, 7 },
  { 26, 21, 18, 23, 25, 13, 6, 14 },
};

constexpr double VTK_DIVERGED = 1.e6;
constexpr int VTK_MAX_ITERATIONS = 20;
constexpr double VTK_CONVERGED = 1.e-04;
}

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkQuadraticHexahedron);

//------------------------------------------------------------------------------
// Construct the hex with 20 points + 7 extra points for internal
// computation.
vtkQuadraticHexahedron::vtkQuadraticHexahedron()
{
  // At times the cell looks like it has 27 points (during interpolation)
  // We initially allocate for 27.
  this->Points->SetNumberOfPoints(27);
  this->PointIds->SetNumberOfIds(27);
  for (int i = 0; i < 27; i++)
  {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i, 0);
  }
  this->Points->SetNumberOfPoints(20);
  this->PointIds->SetNumberOfIds(20);

  this->Edge = vtkSmartPointer<vtkQuadraticEdge>::New();
  this->Face = vtkSmartPointer<vtkQuadraticQuad>::New();
  this->Hex = vtkSmartPointer<vtkHexahedron>::New();

  this->PointData = vtkSmartPointer<vtkPointData>::New();
  this->CellData = vtkSmartPointer<vtkCellData>::New();
  this->CellScalars = vtkSmartPointer<vtkDoubleArray>::New();
  this->CellScalars->SetNumberOfTuples(27);
  this->Scalars = vtkSmartPointer<vtkDoubleArray>::New();
  this->Scalars->SetNumberOfTuples(8);
}

//------------------------------------------------------------------------------
vtkCell* vtkQuadraticHexahedron::GetEdge(int edgeId)
{
  edgeId = std::clamp(edgeId, 0, 11);

  for (int i = 0; i < 3; i++)
  {
    this->Edge->PointIds->SetId(i, this->PointIds->GetId(Edges[edgeId][i]));
    this->Edge->Points->SetPoint(i, this->Points->GetPoint(Edges[edgeId][i]));
  }

  return this->Edge;
}

//------------------------------------------------------------------------------
vtkCell* vtkQuadraticHexahedron::GetFace(int faceId)
{
  faceId = std::clamp(faceId, 0, 5);

  for (int i = 0; i < 8; i++)
  {
    this->Face->PointIds->SetId(i, this->PointIds->GetId(Faces[faceId][i]));
    this->Face->Points->SetPoint(i, this->Points->GetPoint(Faces[faceId][i]));
  }

  return this->Face;
}

//------------------------------------------------------------------------------
const vtkIdType* vtkQuadraticHexahedron::GetEdgeArray(vtkIdType edgeId)
{
  return Edges[edgeId];
}

//------------------------------------------------------------------------------
const vtkIdType* vtkQuadraticHexahedron::GetFaceArray(vtkIdType faceId)
{
  return Faces[faceId];
}

//------------------------------------------------------------------------------
vtkNonLinearCell3D::PointType vtkQuadraticHexahedron::GetPointType(vtkIdType pointId)
{
  assert(pointId < GetNumberOfPoints() && "pointId too large");
  return PointTypes[pointId];
}

//------------------------------------------------------------------------------
vtkIdType vtkQuadraticHexahedron::GetEdgePoints(vtkIdType edgeId, const vtkIdType*& pts)
{
  pts = vtkQuadraticHexahedron::GetEdgeArray(edgeId);
  return 3;
}

//------------------------------------------------------------------------------
vtkIdType vtkQuadraticHexahedron::GetFacePoints(vtkIdType faceId, const vtkIdType*& pts)
{
  pts = vtkQuadraticHexahedron::GetFaceArray(faceId);
  return 8;
}

//------------------------------------------------------------------------------
void vtkQuadraticHexahedron::GetEdgeToAdjacentFaces(vtkIdType edgeId, const vtkIdType*& faceIds)
{
  assert(edgeId < GetNumberOfEdges() && "edgeId too large");
  faceIds = EdgeToAdjacentFaces[edgeId];
}

//------------------------------------------------------------------------------
vtkIdType vtkQuadraticHexahedron::GetFaceToAdjacentFaces(
  vtkIdType faceId, const vtkIdType*& faceIds)
{
  assert(faceId < GetNumberOfFaces() && "faceId too large");
  faceIds = FaceToAdjacentFaces[faceId];
  return 4;
}

//------------------------------------------------------------------------------
vtkIdType vtkQuadraticHexahedron::GetPointToIncidentEdges(
  vtkIdType pointId, const vtkIdType*& edgeIds)
{
  assert(pointId < GetNumberOfPoints() && "pointId too large");
  edgeIds = PointToIncidentEdges[pointId];
  return pointId < /*corner points*/ 8 ? 3 : 1;
}

//------------------------------------------------------------------------------
vtkIdType vtkQuadraticHexahedron::GetPointToIncidentFaces(
  vtkIdType pointId, const vtkIdType*& faceIds)
{
  assert(pointId < GetNumberOfPoints() && "pointId too large");
  faceIds = PointToIncidentFaces[pointId];
  return pointId < /*corner points*/ 8 ? 3 : 2;
}

//------------------------------------------------------------------------------
vtkIdType vtkQuadraticHexahedron::GetPointToOneRingPoints(vtkIdType pointId, const vtkIdType*& pts)
{
  assert(pointId < GetNumberOfPoints() && "pointId too large");
  pts = PointToOneRingPoints[pointId];
  return pointId < /*corner points*/ 8 ? 6 : 2;
}

//------------------------------------------------------------------------------
void vtkQuadraticHexahedron::Subdivide(
  vtkPointData* inPd, vtkCellData* inCd, vtkIdType cellId, vtkDataArray* cellScalars)
{
  double weights[20], x[3];

  // Copy point and cell attribute data, first make sure it's empty:
  this->PointData->Initialize();
  this->CellData->Initialize();
  // Make sure to copy ALL arrays. These field data have to be
  // identical to the input field data. Otherwise, CopyData
  // that occurs later may not work because the output field
  // data was initialized (CopyAllocate) with the input field
  // data.
  this->PointData->CopyAllOn();
  this->CellData->CopyAllOn();
  this->PointData->CopyAllocate(inPd, 27);
  this->CellData->CopyAllocate(inCd, 8);
  for (int i = 0; i < 20; i++)
  {
    this->PointData->CopyData(inPd, this->PointIds->GetId(i), i);
    this->CellScalars->SetValue(i, cellScalars->GetTuple1(i));
  }
  for (int i = 0; i < 8; i++)
  {
    this->CellData->CopyData(inCd, cellId, i);
  }

  // Interpolate new values
  double p[3];
  this->Points->Reserve(27);
  this->CellScalars->ReserveTuples(27);
  for (int numMidPts = 0; numMidPts < 7; numMidPts++)
  {
    vtkQuadraticHexahedron::InterpolationFunctions(MidPoints[numMidPts], weights);

    x[0] = x[1] = x[2] = 0.0;
    double s = 0.0;
    for (int i = 0; i < 20; i++)
    {
      this->Points->GetPoint(i, p);
      for (int j = 0; j < 3; j++)
      {
        x[j] += p[j] * weights[i];
      }
      s += cellScalars->GetTuple1(i) * weights[i];
    }
    this->Points->SetPoint(20 + numMidPts, x);
    this->CellScalars->SetValue(20 + numMidPts, s);
    this->PointData->InterpolatePoint(inPd, 20 + numMidPts, this->PointIds, weights);
  }
}

//------------------------------------------------------------------------------
int vtkQuadraticHexahedron::EvaluatePosition(const double x[3], double closestPoint[3], int& subId,
  double pcoords[3], double& dist2, double weights[])
{
  double params[3] = { 0.5, 0.5, 0.5 };
  double derivs[60];

  // Efficient point access
  const auto pointsArray = vtkDoubleArray::FastDownCast(this->Points->GetData());
  if (!pointsArray)
  {
    vtkErrorMacro(<< "Points should be double type");
    return 0;
  }
  const double* pts = pointsArray->GetPointer(0);

  // compute a bound on the volume to get a scale for an acceptable determinant
  static constexpr vtkIdType diagonals[4][2] = { { 0, 6 }, { 1, 7 }, { 2, 4 }, { 3, 5 } };
  double longestDiagonal = 0;
  for (int i = 0; i < 4; i++)
  {
    const double* pt0 = pts + 3 * diagonals[i][0];
    const double* pt1 = pts + 3 * diagonals[i][1];
    double d2 = vtkMath::Distance2BetweenPoints(pt0, pt1);
    longestDiagonal = std::max(longestDiagonal, d2);
  }
  // longestDiagonal value is already squared
  double volumeBound = longestDiagonal * std::sqrt(longestDiagonal);
  double determinantTolerance = 1e-20 < .00001 * volumeBound ? 1e-20 : .00001 * volumeBound;

  //  set initial position for Newton's method
  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = 0.5;

  //  enter iteration loop
  int converged = 0;
  for (int iteration = 0; !converged && iteration < VTK_MAX_ITERATIONS; iteration++)
  {
    //  calculate element interpolation functions and derivatives
    vtkQuadraticHexahedron::InterpolationFunctions(pcoords, weights);
    vtkQuadraticHexahedron::InterpolationDerivs(pcoords, derivs);

    //  calculate newton functions
    double fcol[3] = { 0, 0, 0 }, rcol[3] = { 0, 0, 0 }, scol[3] = { 0, 0, 0 },
           tcol[3] = { 0, 0, 0 };
    for (int i = 0; i < 20; i++)
    {
      const double* pt = pts + 3 * i;
      for (int j = 0; j < 3; j++)
      {
        fcol[j] += pt[j] * weights[i];
        rcol[j] += pt[j] * derivs[i];
        scol[j] += pt[j] * derivs[i + 20];
        tcol[j] += pt[j] * derivs[i + 40];
      }
    }

    for (int i = 0; i < 3; i++)
    {
      fcol[i] -= x[i];
    }

    //  compute determinants and generate improvements
    double d = vtkMath::Determinant3x3(rcol, scol, tcol);
    if (std::abs(d) < determinantTolerance)
    {
      vtkDebugMacro(<< "Determinant incorrect, iteration " << iteration);
      return -1;
    }

    pcoords[0] = params[0] - 0.5 * vtkMath::Determinant3x3(fcol, scol, tcol) / d;
    pcoords[1] = params[1] - 0.5 * vtkMath::Determinant3x3(rcol, fcol, tcol) / d;
    pcoords[2] = params[2] - 0.5 * vtkMath::Determinant3x3(rcol, scol, fcol) / d;

    //  check for convergence
    if (std::abs(pcoords[0] - params[0]) < VTK_CONVERGED &&
      std::abs(pcoords[1] - params[1]) < VTK_CONVERGED &&
      std::abs(pcoords[2] - params[2]) < VTK_CONVERGED)
    {
      converged = 1;
    }
    // Test for bad divergence (S.Hirschberg 11.12.2001)
    else if (std::abs(pcoords[0]) > VTK_DIVERGED || std::abs(pcoords[1]) > VTK_DIVERGED ||
      std::abs(pcoords[2]) > VTK_DIVERGED)
    {
      return -1;
    }
    //  if not converged, repeat
    else
    {
      params[0] = pcoords[0];
      params[1] = pcoords[1];
      params[2] = pcoords[2];
    }
  }

  //  if not converged, set the parametric coordinates to arbitrary values
  //  outside of element
  if (!converged)
  {
    return -1;
  }

  vtkQuadraticHexahedron::InterpolationFunctions(pcoords, weights);

  if (pcoords[0] >= -0.001 && pcoords[0] <= 1.001 && pcoords[1] >= -0.001 && pcoords[1] <= 1.001 &&
    pcoords[2] >= -0.001 && pcoords[2] <= 1.001)
  {
    if (closestPoint)
    {
      closestPoint[0] = x[0];
      closestPoint[1] = x[1];
      closestPoint[2] = x[2];
      dist2 = 0.0; // inside hexahedron
    }
    return 1;
  }
  else
  {
    double pc[3], w[20];
    if (closestPoint)
    {
      for (int i = 0; i < 3; i++) // only approximate, not really true for warped hexa
      {
        if (pcoords[i] < 0.0)
        {
          pc[i] = 0.0;
        }
        else if (pcoords[i] > 1.0)
        {
          pc[i] = 1.0;
        }
        else
        {
          pc[i] = pcoords[i];
        }
      }
      this->EvaluateLocation(subId, pc, closestPoint, static_cast<double*>(w));
      dist2 = vtkMath::Distance2BetweenPoints(closestPoint, x);
    }
    return 0;
  }
}

//------------------------------------------------------------------------------
void vtkQuadraticHexahedron::EvaluateLocation(
  int& vtkNotUsed(subId), const double pcoords[3], double x[3], double* weights)
{
  vtkQuadraticHexahedron::InterpolationFunctions(pcoords, weights);

  // Efficient point access
  const auto pointsArray = vtkDoubleArray::FastDownCast(this->Points->GetData());
  if (!pointsArray)
  {
    vtkErrorMacro(<< "Points should be double type");
    return;
  }
  const double* pts = pointsArray->GetPointer(0);
  x[0] = x[1] = x[2] = 0.0;
  for (int i = 0; i < 20; i++)
  {
    const double* pt = pts + 3 * i;
    for (int j = 0; j < 3; j++)
    {
      x[j] += pt[j] * weights[i];
    }
  }
}

//------------------------------------------------------------------------------
int vtkQuadraticHexahedron::CellBoundary(int subId, const double pcoords[3], vtkIdList* pts)
{
  for (int i = 0; i < 8; ++i) // For each of the eight vertices of the hex
  {
    this->Hex->PointIds->SetId(i, this->PointIds->GetId(i));
  }

  return this->Hex->CellBoundary(subId, pcoords, pts);
}

//------------------------------------------------------------------------------
void vtkQuadraticHexahedron::Contour(double value, vtkDataArray* cellScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* verts, vtkCellArray* lines,
  vtkCellArray* polys, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId,
  vtkCellData* outCd)
{
  // subdivide into 8 linear hexs
  this->Subdivide(inPd, inCd, cellId, cellScalars);

  // contour each linear quad separately
  for (int i = 0; i < 8; i++) // For each subdivided hexahedron
  {
    for (int j = 0; j < 8; j++) // For each of the eight vertices of the hexhedron
    {
      this->Hex->Points->SetPoint(j, this->Points->GetPoint(LinearCells[i][j]));
      this->Hex->PointIds->SetId(j, LinearCells[i][j]);
      this->Scalars->SetValue(j, this->CellScalars->GetValue(LinearCells[i][j]));
    }
    this->Hex->Contour(value, this->Scalars, locator, verts, lines, polys, this->PointData, outPd,
      this->CellData, i, outCd);
  }
}

//------------------------------------------------------------------------------
// Line-hex intersection. Intersection has to occur within [0,1] parametric
// coordinates and with specified tolerance.
int vtkQuadraticHexahedron::IntersectWithLine(
  const double* p1, const double* p2, double tol, double& t, double* x, double* pcoords, int& subId)
{
  int intersection = 0;
  double tTemp;
  double pc[3], xTemp[3];

  t = VTK_DOUBLE_MAX;
  for (int faceNum = 0; faceNum < 6; faceNum++)
  {
    for (int i = 0; i < 8; i++)
    {
      this->Face->Points->SetPoint(i, this->Points->GetPoint(Faces[faceNum][i]));
    }

    if (this->Face->IntersectWithLine(p1, p2, tol, tTemp, xTemp, pc, subId))
    {
      intersection = 1;
      if (tTemp < t)
      {
        t = tTemp;
        x[0] = xTemp[0];
        x[1] = xTemp[1];
        x[2] = xTemp[2];
        switch (faceNum)
        {
          case 0:
            pcoords[0] = 0.0;
            pcoords[1] = pc[1];
            pcoords[2] = pc[0];
            break;

          case 1:
            pcoords[0] = 1.0;
            pcoords[1] = pc[0];
            pcoords[2] = pc[1];
            break;

          case 2:
            pcoords[0] = pc[0];
            pcoords[1] = 0.0;
            pcoords[2] = pc[1];
            break;

          case 3:
            pcoords[0] = pc[1];
            pcoords[1] = 1.0;
            pcoords[2] = pc[0];
            break;

          case 4:
            pcoords[0] = pc[1];
            pcoords[1] = pc[0];
            pcoords[2] = 0.0;
            break;

          case 5:
          default:
            pcoords[0] = pc[0];
            pcoords[1] = pc[1];
            pcoords[2] = 1.0;
            break;
        }
      }
    }
  }
  return intersection;
}

//------------------------------------------------------------------------------
int vtkQuadraticHexahedron::TriangulateLocalIds(int vtkNotUsed(index), vtkIdList* ptIds)
{
  ptIds->SetNumberOfIds(88);
  constexpr vtkIdType localPtIds[22][4] = { { 8, 11, 0, 16 }, { 1, 9, 8, 17 }, { 2, 10, 9, 18 },
    { 11, 8, 10, 12 }, { 10, 8, 9, 12 }, { 11, 10, 3, 19 }, { 12, 9, 10, 13 }, { 13, 10, 12, 14 },
    { 11, 12, 10, 14 }, { 14, 11, 12, 15 }, { 12, 11, 8, 16 }, { 4, 15, 12, 16 },
    { 15, 11, 12, 16 }, { 9, 12, 8, 17 }, { 12, 13, 5, 17 }, { 13, 12, 9, 17 }, { 13, 9, 10, 18 },
    { 13, 14, 6, 18 }, { 14, 13, 10, 18 }, { 11, 14, 10, 19 }, { 14, 15, 7, 19 },
    { 15, 14, 11, 19 } };
  std::copy_n(&localPtIds[0][0], 88, ptIds->begin());
  return 1;
}

//------------------------------------------------------------------------------
// Given parametric coordinates compute inverse Jacobian transformation
// matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
// function derivatives.
void vtkQuadraticHexahedron::JacobianInverse(
  const double pcoords[3], double** inverse, double derivs[60])
{
  double *m[3], m0[3], m1[3], m2[3];
  double x[3];

  // compute interpolation function derivatives
  vtkQuadraticHexahedron::InterpolationDerivs(pcoords, derivs);

  // create Jacobian matrix
  m[0] = m0;
  m[1] = m1;
  m[2] = m2;
  for (int i = 0; i < 3; i++) // initialize matrix
  {
    m0[i] = m1[i] = m2[i] = 0.0;
  }

  for (int j = 0; j < 20; j++)
  {
    this->Points->GetPoint(j, x);
    for (int i = 0; i < 3; i++)
    {
      m0[i] += x[i] * derivs[j];
      m1[i] += x[i] * derivs[20 + j];
      m2[i] += x[i] * derivs[40 + j];
    }
  }

  // now find the inverse
  if (vtkMath::InvertMatrix(m, inverse, 3) == 0)
  {
    vtkErrorMacro(<< "Jacobian inverse not found");
    return;
  }
}

//------------------------------------------------------------------------------
void vtkQuadraticHexahedron::Derivatives(
  int vtkNotUsed(subId), const double pcoords[3], const double* values, int dim, double* derivs)
{
  double *jI[3], j0[3], j1[3], j2[3];
  double functionDerivs[60], sum[3];

  // compute inverse Jacobian and interpolation function derivatives
  jI[0] = j0;
  jI[1] = j1;
  jI[2] = j2;
  this->JacobianInverse(pcoords, jI, functionDerivs);

  // now compute derivates of values provided
  for (int k = 0; k < dim; k++) // loop over values per vertex
  {
    sum[0] = sum[1] = sum[2] = 0.0;
    for (int i = 0; i < 20; i++) // loop over interp. function derivatives
    {
      sum[0] += functionDerivs[i] * values[dim * i + k];
      sum[1] += functionDerivs[20 + i] * values[dim * i + k];
      sum[2] += functionDerivs[40 + i] * values[dim * i + k];
    }
    for (int j = 0; j < 3; j++) // loop over derivative directions
    {
      derivs[3 * k + j] = sum[0] * jI[j][0] + sum[1] * jI[j][1] + sum[2] * jI[j][2];
    }
  }
}

//------------------------------------------------------------------------------
// Clip this quadratic hex using scalar value provided. Like contouring,
// except that it cuts the hex to produce tetrahedra.
void vtkQuadraticHexahedron::Clip(double value, vtkDataArray* cellScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* tets, vtkPointData* inPd, vtkPointData* outPd,
  vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd, int insideOut)
{
  // create eight linear hexes
  this->Subdivide(inPd, inCd, cellId, cellScalars);

  // contour each linear hex separately
  for (int i = 0; i < 8; i++) // For each subdivided hexahedron
  {
    for (int j = 0; j < 8; j++) // For each of the eight vertices of the hexhedron
    {
      this->Hex->Points->SetPoint(j, this->Points->GetPoint(LinearCells[i][j]));
      this->Hex->PointIds->SetId(j, LinearCells[i][j]);
      this->Scalars->SetValue(j, this->CellScalars->GetValue(LinearCells[i][j]));
    }
    this->Hex->Clip(value, this->Scalars, locator, tets, this->PointData, outPd, this->CellData, i,
      outCd, insideOut);
  }
}

//------------------------------------------------------------------------------
// Compute interpolation functions for the twenty nodes.
void vtkQuadraticHexahedron::InterpolationFunctions(const double pcoords[3], double weights[20])
{
  // VTK needs parametric coordinates to be between (0,1). Isoparametric
  // shape functions are formulated between (-1,1). Here we do a
  // coordinate system conversion from (0,1) to (-1,1).
  double r = 2.0 * (pcoords[0] - 0.5);
  double s = 2.0 * (pcoords[1] - 0.5);
  double t = 2.0 * (pcoords[2] - 0.5);

  double rm = 1.0 - r;
  double rp = 1.0 + r;
  double sm = 1.0 - s;
  double sp = 1.0 + s;
  double tm = 1.0 - t;
  double tp = 1.0 + t;
  double r2 = 1.0 - r * r;
  double s2 = 1.0 - s * s;
  double t2 = 1.0 - t * t;

  // The eight corner points
  weights[0] = 0.125 * rm * sm * tm * (-r - s - t - 2.0);
  weights[1] = 0.125 * rp * sm * tm * (r - s - t - 2.0);
  weights[2] = 0.125 * rp * sp * tm * (r + s - t - 2.0);
  weights[3] = 0.125 * rm * sp * tm * (-r + s - t - 2.0);
  weights[4] = 0.125 * rm * sm * tp * (-r - s + t - 2.0);
  weights[5] = 0.125 * rp * sm * tp * (r - s + t - 2.0);
  weights[6] = 0.125 * rp * sp * tp * (r + s + t - 2.0);
  weights[7] = 0.125 * rm * sp * tp * (-r + s + t - 2.0);

  // The mid-edge nodes
  weights[8] = 0.25 * r2 * sm * tm;
  weights[9] = 0.25 * s2 * rp * tm;
  weights[10] = 0.25 * r2 * sp * tm;
  weights[11] = 0.25 * s2 * rm * tm;
  weights[12] = 0.25 * r2 * sm * tp;
  weights[13] = 0.25 * s2 * rp * tp;
  weights[14] = 0.25 * r2 * sp * tp;
  weights[15] = 0.25 * s2 * rm * tp;
  weights[16] = 0.25 * t2 * rm * sm;
  weights[17] = 0.25 * t2 * rp * sm;
  weights[18] = 0.25 * t2 * rp * sp;
  weights[19] = 0.25 * t2 * rm * sp;
}

//------------------------------------------------------------------------------
// Derivatives in parametric space.
void vtkQuadraticHexahedron::InterpolationDerivs(const double pcoords[3], double derivs[60])
{
  // VTK needs parametric coordinates to be between (0,1). Isoparametric
  // shape functions are formulated between (-1,1). Here we do a
  // coordinate system conversion from (0,1) to (-1,1).
  double r = 2.0 * (pcoords[0] - 0.5);
  double s = 2.0 * (pcoords[1] - 0.5);
  double t = 2.0 * (pcoords[2] - 0.5);

  double rm = 1.0 - r;
  double rp = 1.0 + r;
  double sm = 1.0 - s;
  double sp = 1.0 + s;
  double tm = 1.0 - t;
  double tp = 1.0 + t;

  // r-derivatives
  derivs[0] = -0.125 * (sm * tm - 2.0 * r * sm * tm - s * sm * tm - t * sm * tm - 2.0 * sm * tm);
  derivs[1] = 0.125 * (sm * tm + 2.0 * r * sm * tm - s * sm * tm - t * sm * tm - 2.0 * sm * tm);
  derivs[2] = 0.125 * (sp * tm + 2.0 * r * sp * tm + s * sp * tm - t * sp * tm - 2.0 * sp * tm);
  derivs[3] = -0.125 * (sp * tm - 2.0 * r * sp * tm + s * sp * tm - t * sp * tm - 2.0 * sp * tm);
  derivs[4] = -0.125 * (sm * tp - 2.0 * r * sm * tp - s * sm * tp + t * sm * tp - 2.0 * sm * tp);
  derivs[5] = 0.125 * (sm * tp + 2.0 * r * sm * tp - s * sm * tp + t * sm * tp - 2.0 * sm * tp);
  derivs[6] = 0.125 * (sp * tp + 2.0 * r * sp * tp + s * sp * tp + t * sp * tp - 2.0 * sp * tp);
  derivs[7] = -0.125 * (sp * tp - 2.0 * r * sp * tp + s * sp * tp + t * sp * tp - 2.0 * sp * tp);
  derivs[8] = -0.5 * r * sm * tm;
  derivs[9] = 0.25 * (tm - s * s * tm);
  derivs[10] = -0.5 * r * sp * tm;
  derivs[11] = -0.25 * (tm - s * s * tm);
  derivs[12] = -0.5 * r * sm * tp;
  derivs[13] = 0.25 * (tp - s * s * tp);
  derivs[14] = -0.5 * r * sp * tp;
  derivs[15] = -0.25 * (tp - s * s * tp);
  derivs[16] = -0.25 * (sm - t * t * sm);
  derivs[17] = 0.25 * (sm - t * t * sm);
  derivs[18] = 0.25 * (sp - t * t * sp);
  derivs[19] = -0.25 * (sp - t * t * sp);

  // s-derivatives
  derivs[20] = -0.125 * (rm * tm - 2.0 * s * rm * tm - r * rm * tm - t * rm * tm - 2.0 * rm * tm);
  derivs[21] = -0.125 * (rp * tm - 2.0 * s * rp * tm + r * rp * tm - t * rp * tm - 2.0 * rp * tm);
  derivs[22] = 0.125 * (rp * tm + 2.0 * s * rp * tm + r * rp * tm - t * rp * tm - 2.0 * rp * tm);
  derivs[23] = 0.125 * (rm * tm + 2.0 * s * rm * tm - r * rm * tm - t * rm * tm - 2.0 * rm * tm);
  derivs[24] = -0.125 * (rm * tp - 2.0 * s * rm * tp - r * rm * tp + t * rm * tp - 2.0 * rm * tp);
  derivs[25] = -0.125 * (rp * tp - 2.0 * s * rp * tp + r * rp * tp + t * rp * tp - 2.0 * rp * tp);
  derivs[26] = 0.125 * (rp * tp + 2.0 * s * rp * tp + r * rp * tp + t * rp * tp - 2.0 * rp * tp);
  derivs[27] = 0.125 * (rm * tp + 2.0 * s * rm * tp - r * rm * tp + t * rm * tp - 2.0 * rm * tp);
  derivs[28] = -0.25 * (tm - r * r * tm);
  derivs[29] = -0.5 * s * rp * tm;
  derivs[30] = 0.25 * (tm - r * r * tm);
  derivs[31] = -0.5 * s * rm * tm;
  derivs[32] = -0.25 * (tp - r * r * tp);
  derivs[33] = -0.5 * s * rp * tp;
  derivs[34] = 0.25 * (tp - r * r * tp);
  derivs[35] = -0.5 * s * rm * tp;
  derivs[36] = -0.25 * (rm - t * t * rm);
  derivs[37] = -0.25 * (rp - t * t * rp);
  derivs[38] = 0.25 * (rp - t * t * rp);
  derivs[39] = 0.25 * (rm - t * t * rm);

  // t-derivatives
  derivs[40] = -0.125 * (rm * sm - 2.0 * t * rm * sm - r * rm * sm - s * rm * sm - 2.0 * rm * sm);
  derivs[41] = -0.125 * (rp * sm - 2.0 * t * rp * sm + r * rp * sm - s * rp * sm - 2.0 * rp * sm);
  derivs[42] = -0.125 * (rp * sp - 2.0 * t * rp * sp + r * rp * sp + s * rp * sp - 2.0 * rp * sp);
  derivs[43] = -0.125 * (rm * sp - 2.0 * t * rm * sp - r * rm * sp + s * rm * sp - 2.0 * rm * sp);
  derivs[44] = 0.125 * (rm * sm + 2.0 * t * rm * sm - r * rm * sm - s * rm * sm - 2.0 * rm * sm);
  derivs[45] = 0.125 * (rp * sm + 2.0 * t * rp * sm + r * rp * sm - s * rp * sm - 2.0 * rp * sm);
  derivs[46] = 0.125 * (rp * sp + 2.0 * t * rp * sp + r * rp * sp + s * rp * sp - 2.0 * rp * sp);
  derivs[47] = 0.125 * (rm * sp + 2.0 * t * rm * sp - r * rm * sp + s * rm * sp - 2.0 * rm * sp);
  derivs[48] = -0.25 * (sm - r * r * sm);
  derivs[49] = -0.25 * (rp - s * s * rp);
  derivs[50] = -0.25 * (sp - r * r * sp);
  derivs[51] = -0.25 * (rm - s * s * rm);
  derivs[52] = 0.25 * (sm - r * r * sm);
  derivs[53] = 0.25 * (rp - s * s * rp);
  derivs[54] = 0.25 * (sp - r * r * sp);
  derivs[55] = 0.25 * (rm - s * s * rm);
  derivs[56] = -0.5 * t * rm * sm;
  derivs[57] = -0.5 * t * rp * sm;
  derivs[58] = -0.5 * t * rp * sp;
  derivs[59] = -0.5 * t * rm * sp;

  // we compute derivatives in [-1; 1] but we need them in [ 0; 1]
  for (int i = 0; i < 60; i++)
  {
    derivs[i] *= 2;
  }
}

//------------------------------------------------------------------------------
double* vtkQuadraticHexahedron::GetParametricCoords()
{
  return ParametricCoords;
}

//------------------------------------------------------------------------------
void vtkQuadraticHexahedron::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Edge:\n";
  this->Edge->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Face:\n";
  this->Face->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Hex:\n";
  this->Hex->PrintSelf(os, indent.GetNextIndent());
  os << indent << "PointData:\n";
  this->PointData->PrintSelf(os, indent.GetNextIndent());
  os << indent << "CellData:\n";
  this->CellData->PrintSelf(os, indent.GetNextIndent());
  os << indent << "CellScalars:\n";
  this->CellScalars->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Scalars:\n";
  this->Scalars->PrintSelf(os, indent.GetNextIndent());
}
VTK_ABI_NAMESPACE_END
