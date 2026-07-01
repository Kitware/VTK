// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Thanks to Soeren Gebbert who developed this class and
// integrated it into VTK 5.0.

#include "vtkBiQuadraticQuadraticWedge.h"

#include "vtkBiQuadraticQuad.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkQuadraticEdge.h"
#include "vtkQuadraticTriangle.h"
#include "vtkWedge.h"

#include <algorithm> //std::copy
#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkBiQuadraticQuadraticWedge);

namespace
{
//------------------------------------------------------------------------------
[[maybe_unused]] constexpr const char* Topology = R"(
   Bi-Quadratic Quadratic Wedge topology:
              2
             /|\
            / | \
           /  |  \
          8   |   7
         /    14   \
        /     |     \
       /      |      \
      0-------6-------1    ← back triangle
      |   17  |   18  |
      |       5       |
      |      / \      |
      |     /   \     |
      12   /  15 \    13
      |   11      10  |
      |  /         \  |
      | /           \ |
      |/             \|
      3-------9-------4    ← front triangle
)";

//------------------------------------------------------------------------------
// vtkBiQuadraticQuadraticWedge
constexpr vtkNonLinearCell3D::PointType PointTypes[18] = {
  vtkNonLinearCell3D::PointType::CornerPoint,  // point 0
  vtkNonLinearCell3D::PointType::CornerPoint,  // point 1
  vtkNonLinearCell3D::PointType::CornerPoint,  // point 2
  vtkNonLinearCell3D::PointType::CornerPoint,  // point 3
  vtkNonLinearCell3D::PointType::CornerPoint,  // point 4
  vtkNonLinearCell3D::PointType::CornerPoint,  // point 5
  vtkNonLinearCell3D::PointType::EdgeMidPoint, // point 6
  vtkNonLinearCell3D::PointType::EdgeMidPoint, // point 7
  vtkNonLinearCell3D::PointType::EdgeMidPoint, // point 8
  vtkNonLinearCell3D::PointType::EdgeMidPoint, // point 9
  vtkNonLinearCell3D::PointType::EdgeMidPoint, // point 10
  vtkNonLinearCell3D::PointType::EdgeMidPoint, // point 11
  vtkNonLinearCell3D::PointType::EdgeMidPoint, // point 12
  vtkNonLinearCell3D::PointType::EdgeMidPoint, // point 13
  vtkNonLinearCell3D::PointType::EdgeMidPoint, // point 14
  vtkNonLinearCell3D::PointType::FaceMidPoint, // point 15
  vtkNonLinearCell3D::PointType::FaceMidPoint, // point 16
  vtkNonLinearCell3D::PointType::FaceMidPoint, // point 17
};

//------------------------------------------------------------------------------
double ParametricCoords[54] = {
  0.0, 0.0, 0.0, //
  1.0, 0.0, 0.0, //
  0.0, 1.0, 0.0, //
  0.0, 0.0, 1.0, //
  1.0, 0.0, 1.0, //
  0.0, 1.0, 1.0, //
  0.5, 0.0, 0.0, //
  0.5, 0.5, 0.0, //
  0.0, 0.5, 0.0, //
  0.5, 0.0, 1.0, //
  0.5, 0.5, 1.0, //
  0.0, 0.5, 1.0, //
  0.0, 0.0, 0.5, //
  1.0, 0.0, 0.5, //
  0.0, 1.0, 0.5, //
  0.5, 0.0, 0.5, //
  0.5, 0.5, 0.5, //
  0.0, 0.5, 0.5  //
};

//------------------------------------------------------------------------------
// We have 9 quadratic edges
constexpr vtkIdType Edges[9][3] = {
  { 0, 1, 6 },
  { 1, 2, 7 },
  { 2, 0, 8 },
  { 3, 4, 9 },
  { 4, 5, 10 },
  { 5, 3, 11 },
  { 0, 3, 12 },
  { 1, 4, 13 },
  { 2, 5, 14 },
};

//------------------------------------------------------------------------------
// We use 2 quadratic triangles and 3 quadratic-linear quads
constexpr vtkIdType Faces[5][9] = {
  { 0, 2, 1, 8, 7, 6, -1, -1, -1 },   // first quad triangle
  { 3, 4, 5, 9, 10, 11, -1, -1, -1 }, // second quad triangle
  { 0, 1, 4, 3, 6, 13, 9, 12, 15 },   // 1. biquad quad
  { 1, 2, 5, 4, 7, 14, 10, 13, 16 },  // 2. biquad quad
  { 2, 0, 3, 5, 8, 12, 11, 14, 17 },  // 3. biquad quad
};

//------------------------------------------------------------------------------
constexpr vtkIdType EdgeToAdjacentFaces[9][2] = {
  { 0, 2 }, // edge 0: corners 0,1
  { 0, 3 }, // edge 1: corners 1,2
  { 0, 4 }, // edge 2: corners 2,0
  { 1, 2 }, // edge 3: corners 3,4
  { 1, 3 }, // edge 4: corners 4,5
  { 1, 4 }, // edge 5: corners 5,3
  { 2, 4 }, // edge 6: corners 0,3
  { 2, 3 }, // edge 7: corners 1,4
  { 3, 4 }, // edge 8: corners 2,5
};

//------------------------------------------------------------------------------
constexpr vtkIdType FaceToAdjacentFaces[5][4] = {
  { 2, 3, 4, -1 }, // face 0: tri
  { 2, 3, 4, -1 }, // face 1: tri
  { 0, 1, 3, 4 },  // face 2: biquad quad
  { 0, 1, 2, 4 },  // face 3: biquad quad
  { 0, 1, 2, 3 },  // face 4: biquad quad
};

//------------------------------------------------------------------------------
constexpr vtkIdType PointToIncidentEdges[18][3] = {
  { 0, 2, 6 },    // point 0:  corner
  { 0, 1, 7 },    // point 1:  corner
  { 1, 2, 8 },    // point 2:  corner
  { 3, 5, 6 },    // point 3:  corner
  { 3, 4, 7 },    // point 4:  corner
  { 4, 5, 8 },    // point 5:  corner
  { 0, -1, -1 },  // point 6:  mid-edge
  { 1, -1, -1 },  // point 7:  mid-edge
  { 2, -1, -1 },  // point 8:  mid-edge
  { 3, -1, -1 },  // point 9:  mid-edge
  { 4, -1, -1 },  // point 10: mid-edge
  { 5, -1, -1 },  // point 11: mid-edge
  { 6, -1, -1 },  // point 12: mid-edge
  { 7, -1, -1 },  // point 13: mid-edge
  { 8, -1, -1 },  // point 14: mid-edge
  { -1, -1, -1 }, // point 15: mid-face
  { -1, -1, -1 }, // point 16: mid-face
  { -1, -1, -1 }, // point 17: mid-face
};

//------------------------------------------------------------------------------
constexpr vtkIdType PointToIncidentFaces[18][3] = {
  { 0, 2, 4 },   // point 0:  corner, 3 faces
  { 0, 2, 3 },   // point 1:  corner, 3 faces
  { 0, 3, 4 },   // point 2:  corner, 3 faces
  { 1, 2, 4 },   // point 3:  corner, 3 faces
  { 1, 2, 3 },   // point 4:  corner, 3 faces
  { 1, 3, 4 },   // point 5:  corner, 3 faces
  { 0, 2, -1 },  // point 6:  mid-edge, 2 faces
  { 0, 3, -1 },  // point 7:  mid-edge, 2 faces
  { 0, 4, -1 },  // point 8:  mid-edge, 2 faces
  { 1, 2, -1 },  // point 9:  mid-edge, 2 faces
  { 1, 3, -1 },  // point 10: mid-edge, 2 faces
  { 1, 4, -1 },  // point 11: mid-edge, 2 faces
  { 2, 4, -1 },  // point 12: mid-edge, 2 faces
  { 2, 3, -1 },  // point 13: mid-edge, 2 faces
  { 3, 4, -1 },  // point 14: mid-edge, 2 faces
  { 2, -1, -1 }, // point 15: mid-face, 1 face
  { 3, -1, -1 }, // point 16: mid-face, 1 face
  { 4, -1, -1 }, // point 17: mid-face, 1 face
};

//------------------------------------------------------------------------------
constexpr vtkIdType PointToOneRingPoints[18][6] = {
  { 1, 6, 3, 12, 2, 8 },      // point 0:  corner
  { 2, 7, 4, 13, 0, 6 },      // point 1:  corner
  { 0, 8, 5, 14, 1, 7 },      // point 2:  corner
  { 5, 11, 0, 12, 4, 9 },     // point 3:  corner
  { 3, 9, 1, 13, 5, 10 },     // point 4:  corner
  { 4, 10, 2, 14, 3, 11 },    // point 5:  corner
  { 0, 1, -1, -1, -1, -1 },   // point 6:  mid-edge
  { 1, 2, -1, -1, -1, -1 },   // point 7:  mid-edge
  { 2, 0, -1, -1, -1, -1 },   // point 8:  mid-edge
  { 3, 4, -1, -1, -1, -1 },   // point 9:  mid-edge
  { 4, 5, -1, -1, -1, -1 },   // point 10: mid-edge
  { 5, 3, -1, -1, -1, -1 },   // point 11: mid-edge
  { 0, 3, -1, -1, -1, -1 },   // point 12: mid-edge
  { 1, 4, -1, -1, -1, -1 },   // point 13: mid-edge
  { 2, 5, -1, -1, -1, -1 },   // point 14: mid-edge
  { -1, -1, -1, -1, -1, -1 }, // point 15: mid-face
  { -1, -1, -1, -1, -1, -1 }, // point 16: mid-face
  { -1, -1, -1, -1, -1, -1 }, // point 17: mid-face
};

//------------------------------------------------------------------------------
// We are using 8 linear wedge
constexpr vtkIdType LinearCells[8][6] = {
  { 8, 0, 6, 17, 12, 15 },
  { 8, 6, 7, 17, 15, 16 },
  { 7, 6, 1, 16, 15, 13 },
  { 2, 8, 7, 14, 17, 16 },
  { 17, 12, 15, 11, 3, 9 },
  { 17, 15, 16, 11, 9, 10 },
  { 16, 15, 13, 10, 9, 4 },
  { 14, 17, 16, 5, 11, 10 },
};

constexpr double VTK_DIVERGED = 1.e6;
constexpr int VTK_MAX_ITERATIONS = 20;
constexpr double VTK_CONVERGED = 1.e-04;
}

//------------------------------------------------------------------------------
// Construct the biquadratic quadratic wedge with 18 points
vtkBiQuadraticQuadraticWedge::vtkBiQuadraticQuadraticWedge()
{
  this->Points->SetNumberOfPoints(18);
  this->PointIds->SetNumberOfIds(18);
  for (int i = 0; i < 18; i++)
  {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i, 0);
  }

  this->Edge = vtkSmartPointer<vtkQuadraticEdge>::New();
  this->Face = vtkSmartPointer<vtkBiQuadraticQuad>::New();
  this->TriangleFace = vtkSmartPointer<vtkQuadraticTriangle>::New();
  this->Wedge = vtkSmartPointer<vtkWedge>::New();

  this->Scalars = vtkSmartPointer<vtkDoubleArray>::New();
  this->Scalars->SetNumberOfTuples(6); // Number of vertices from a linear wedge
}

//------------------------------------------------------------------------------
vtkCell* vtkBiQuadraticQuadraticWedge::GetEdge(int edgeId)
{
  edgeId = std::clamp(edgeId, 0, 8);

  // We have 9 quadratic edges
  for (int i = 0; i < 3; i++)
  {
    this->Edge->PointIds->SetId(i, this->PointIds->GetId(Edges[edgeId][i]));
    this->Edge->Points->SetPoint(i, this->Points->GetPoint(Edges[edgeId][i]));
  }

  return this->Edge;
}

//------------------------------------------------------------------------------
vtkCell* vtkBiQuadraticQuadraticWedge::GetFace(int faceId)
{
  faceId = std::clamp(faceId, 0, 4);

  // load point id's and coordinates
  // be careful with the last two:
  if (faceId < 2)
  {
    for (int i = 0; i < 6; i++)
    {
      this->TriangleFace->PointIds->SetId(i, this->PointIds->GetId(Faces[faceId][i]));
      this->TriangleFace->Points->SetPoint(i, this->Points->GetPoint(Faces[faceId][i]));
    }
    return this->TriangleFace;
  }
  else
  {
    for (int i = 0; i < 9; i++)
    {
      this->Face->PointIds->SetId(i, this->PointIds->GetId(Faces[faceId][i]));
      this->Face->Points->SetPoint(i, this->Points->GetPoint(Faces[faceId][i]));
    }
    return this->Face;
  }
}

//------------------------------------------------------------------------------
const vtkIdType* vtkBiQuadraticQuadraticWedge::GetEdgeArray(vtkIdType edgeId)
{
  return Edges[edgeId];
}
//------------------------------------------------------------------------------
const vtkIdType* vtkBiQuadraticQuadraticWedge::GetFaceArray(vtkIdType faceId)
{
  return Faces[faceId];
}

//------------------------------------------------------------------------------
vtkNonLinearCell3D::PointType vtkBiQuadraticQuadraticWedge::GetPointType(vtkIdType pointId)
{
  assert(pointId < GetNumberOfPoints() && "pointId too large");
  return PointTypes[pointId];
}

//------------------------------------------------------------------------------
vtkIdType vtkBiQuadraticQuadraticWedge::GetEdgePoints(vtkIdType edgeId, const vtkIdType*& pts)
{
  pts = vtkBiQuadraticQuadraticWedge::GetEdgeArray(edgeId);
  return 3;
}

//------------------------------------------------------------------------------
vtkIdType vtkBiQuadraticQuadraticWedge::GetFacePoints(vtkIdType faceId, const vtkIdType*& pts)
{
  pts = vtkBiQuadraticQuadraticWedge::GetFaceArray(faceId);
  return faceId < 2 ? 6 : 9; // tri faces have 6 points, quad faces have 9
}

//------------------------------------------------------------------------------
void vtkBiQuadraticQuadraticWedge::GetEdgeToAdjacentFaces(
  vtkIdType edgeId, const vtkIdType*& faceIds)
{
  assert(edgeId < GetNumberOfEdges() && "edgeId too large");
  faceIds = EdgeToAdjacentFaces[edgeId];
}

//------------------------------------------------------------------------------
vtkIdType vtkBiQuadraticQuadraticWedge::GetFaceToAdjacentFaces(
  vtkIdType faceId, const vtkIdType*& faceIds)
{
  assert(faceId < GetNumberOfFaces() && "faceId too large");
  faceIds = FaceToAdjacentFaces[faceId];
  return faceId < 2 ? 3 : 4; // tri faces have 3 adjacent faces, quad faces have 4
}

//------------------------------------------------------------------------------
vtkIdType vtkBiQuadraticQuadraticWedge::GetPointToIncidentEdges(
  vtkIdType pointId, const vtkIdType*& edgeIds)
{
  assert(pointId < GetNumberOfPoints() && "pointId too large");
  edgeIds = PointToIncidentEdges[pointId];
  if (pointId < /*corner points*/ 6)
  {
    return 3;
  }
  else if (pointId < /*mid-edge*/ 15)
  {
    return 1;
  }
  else /*mid face*/
  {
    return 0;
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkBiQuadraticQuadraticWedge::GetPointToIncidentFaces(
  vtkIdType pointId, const vtkIdType*& faceIds)
{
  assert(pointId < GetNumberOfPoints() && "pointId too large");
  faceIds = PointToIncidentFaces[pointId];
  if (pointId < /*corner points*/ 6)
  {
    return 3;
  }
  else if (pointId < /*mid-edge*/ 15)
  {
    return 2;
  }
  else /*mid face*/
  {
    return 1;
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkBiQuadraticQuadraticWedge::GetPointToOneRingPoints(
  vtkIdType pointId, const vtkIdType*& pts)
{
  assert(pointId < GetNumberOfPoints() && "pointId too large");
  pts = PointToOneRingPoints[pointId];
  if (pointId < /*corner points*/ 6)
  {
    return 6;
  }
  else if (pointId < /*mid-edge*/ 15)
  {
    return 2;
  }
  else /*mid face*/
  {
    return 0;
  }
}

//------------------------------------------------------------------------------
int vtkBiQuadraticQuadraticWedge::EvaluatePosition(const double x[3], double* closestPoint,
  int& subId, double pcoords[3], double& dist2, double* weights)
{
  int converged;
  double params[3];
  double fcol[3], rcol[3], scol[3], tcol[3];
  double derivs[3 * 18];

  //  set initial position for Newton's method
  subId = 0;
  pcoords[0] = pcoords[1] = pcoords[2] = params[0] = params[1] = params[2] = 0.5;

  // Efficient point access
  const auto pointsArray = vtkDoubleArray::FastDownCast(this->Points->GetData());
  if (!pointsArray)
  {
    vtkErrorMacro(<< "Points should be double type");
    return 0;
  }
  const double* pts = pointsArray->GetPointer(0);

  //  enter iteration loop
  for (int iteration = converged = 0; !converged && (iteration < VTK_MAX_ITERATIONS); iteration++)
  {
    //  calculate element interpolation functions and derivatives
    vtkBiQuadraticQuadraticWedge::InterpolationFunctions(pcoords, weights);
    vtkBiQuadraticQuadraticWedge::InterpolationDerivs(pcoords, derivs);

    //  calculate newton functions
    for (int i = 0; i < 3; i++)
    {
      fcol[i] = rcol[i] = scol[i] = tcol[i] = 0.0;
    }
    for (int i = 0; i < 18; i++)
    {
      const double* pt = pts + 3 * i;
      for (int j = 0; j < 3; j++)
      {
        fcol[j] += pt[j] * weights[i];
        rcol[j] += pt[j] * derivs[i];
        scol[j] += pt[j] * derivs[i + 18];
        tcol[j] += pt[j] * derivs[i + 36];
      }
    }

    for (int i = 0; i < 3; i++)
    {
      fcol[i] -= x[i];
    }

    //  compute determinants and generate improvements
    double d = vtkMath::Determinant3x3(rcol, scol, tcol);
    if (std::abs(d) < 1.e-20)
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

  vtkBiQuadraticQuadraticWedge::InterpolationFunctions(pcoords, weights);

  if (pcoords[0] >= -0.001 && pcoords[0] <= 1.001 && pcoords[1] >= -0.001 && pcoords[1] <= 1.001 &&
    pcoords[2] >= -0.001 && pcoords[2] <= 1.001)
  {
    if (closestPoint)
    {
      closestPoint[0] = x[0];
      closestPoint[1] = x[1];
      closestPoint[2] = x[2];
      dist2 = 0.0; // inside wedge
    }
    return 1;
  }
  else
  {
    double pc[3], w[18];
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
void vtkBiQuadraticQuadraticWedge::EvaluateLocation(
  int& vtkNotUsed(subId), const double pcoords[3], double x[3], double* weights)
{
  vtkBiQuadraticQuadraticWedge::InterpolationFunctions(pcoords, weights);

  // Efficient point access
  const auto pointsArray = vtkDoubleArray::FastDownCast(this->Points->GetData());
  if (!pointsArray)
  {
    vtkErrorMacro(<< "Points should be double type");
    return;
  }
  const double* pts = pointsArray->GetPointer(0);

  x[0] = x[1] = x[2] = 0.0;
  for (int i = 0; i < 18; i++)
  {
    const double* pt = pts + 3 * i;
    for (int j = 0; j < 3; j++)
    {
      x[j] += pt[j] * weights[i];
    }
  }
}

//------------------------------------------------------------------------------
int vtkBiQuadraticQuadraticWedge::CellBoundary(int subId, const double pcoords[3], vtkIdList* pts)
{
  return this->Wedge->CellBoundary(subId, pcoords, pts);
}

//------------------------------------------------------------------------------
void vtkBiQuadraticQuadraticWedge::Contour(double value, vtkDataArray* cellScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* verts, vtkCellArray* lines,
  vtkCellArray* polys, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId,
  vtkCellData* outCd)
{
  // contour each linear wedge separately
  for (int i = 0; i < 8; i++) // for each wedge
  {
    for (int j = 0; j < 6; j++) // for each point of wedge
    {
      this->Wedge->Points->SetPoint(j, this->Points->GetPoint(LinearCells[i][j]));
      this->Wedge->PointIds->SetId(j, this->PointIds->GetId(LinearCells[i][j]));
      this->Scalars->SetValue(j, cellScalars->GetTuple1(LinearCells[i][j]));
    }
    this->Wedge->Contour(
      value, this->Scalars, locator, verts, lines, polys, inPd, outPd, inCd, cellId, outCd);
  }
}

//------------------------------------------------------------------------------
// Clip this biquadratic wedge using scalar value provided. Like contouring,
// except that it cuts the wedge to produce tetrahedra.
void vtkBiQuadraticQuadraticWedge::Clip(double value, vtkDataArray* cellScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* tets, vtkPointData* inPd, vtkPointData* outPd,
  vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd, int insideOut)
{
  // contour each linear wedge separately
  for (int i = 0; i < 8; i++) // for each wedge
  {
    for (int j = 0; j < 6; j++) // for each of the six vertices of the wedge
    {
      this->Wedge->Points->SetPoint(j, this->Points->GetPoint(LinearCells[i][j]));
      this->Wedge->PointIds->SetId(j, this->PointIds->GetId(LinearCells[i][j]));
      this->Scalars->SetValue(j, cellScalars->GetTuple1(LinearCells[i][j]));
    }
    this->Wedge->Clip(
      value, this->Scalars, locator, tets, inPd, outPd, inCd, cellId, outCd, insideOut);
  }
}

//------------------------------------------------------------------------------
// Line-hex intersection. Intersection has to occur within [0,1] parametric
// coordinates and with specified tolerance.
int vtkBiQuadraticQuadraticWedge::IntersectWithLine(
  const double* p1, const double* p2, double tol, double& t, double* x, double* pcoords, int& subId)
{
  int intersection = 0;
  double tTemp;
  double pc[3], xTemp[3];
  int inter;

  t = VTK_DOUBLE_MAX;
  for (int faceNum = 0; faceNum < 5; faceNum++)
  {
    // We have 9 nodes on biquad face
    // and 6 on triangle faces
    if (faceNum < 2)
    {
      for (int i = 0; i < 6; i++)
      {
        this->TriangleFace->PointIds->SetId(i, this->PointIds->GetId(Faces[faceNum][i]));
        this->TriangleFace->Points->SetPoint(i, this->Points->GetPoint(Faces[faceNum][i]));
      }
      inter = this->TriangleFace->IntersectWithLine(p1, p2, tol, tTemp, xTemp, pc, subId);
    }
    else
    {
      for (int i = 0; i < 9; i++)
      {
        this->Face->Points->SetPoint(i, this->Points->GetPoint(Faces[faceNum][i]));
      }
      inter = this->Face->IntersectWithLine(p1, p2, tol, tTemp, xTemp, pc, subId);
    }
    if (inter)
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
          default:
            pcoords[0] = pc[1];
            pcoords[1] = pc[0];
            pcoords[2] = 0.0;
            break;
        }
      }
    }
  }
  return intersection;
}

//------------------------------------------------------------------------------
int vtkBiQuadraticQuadraticWedge::TriangulateLocalIds(int vtkNotUsed(index), vtkIdList* ptIds)
{
  // Split into 8 linear sub-wedges using the 3 face-center nodes (15,16,17)
  // and the 3 vertical mid-edge nodes (12,13,14).
  // The 8 sub-wedges are arranged in 2 layers of 4:
  //   Bottom layer (nodes 0-8, 12-17): 1 central + 3 corner wedges -> 4 * 3 = 12 tets
  //   Top layer    (nodes 3-5, 9-17):  1 central + 3 corner wedges -> 4 * 3 = 12 tets
  // Total: 24 tets, all 18 nodes used.
  constexpr vtkIdType ids[24][4] = {
    // W0: corner near 0 (bottom layer)
    { 8, 0, 6, 17 },
    { 0, 6, 17, 12 },
    { 6, 17, 12, 15 },
    // W1: central (bottom layer)
    { 8, 6, 7, 17 },
    { 6, 7, 17, 15 },
    { 7, 17, 15, 16 },
    // W2: corner near 1 (bottom layer)
    { 7, 6, 1, 16 },
    { 6, 1, 16, 15 },
    { 1, 16, 15, 13 },
    // W3: corner near 2 (bottom layer)
    { 2, 8, 7, 14 },
    { 8, 7, 14, 17 },
    { 7, 14, 17, 16 },
    // W4: corner near 3 (top layer)
    { 17, 12, 15, 11 },
    { 12, 15, 11, 3 },
    { 15, 11, 3, 9 },
    // W5: central (top layer)
    { 17, 15, 16, 11 },
    { 15, 16, 11, 9 },
    { 16, 11, 9, 10 },
    // W6: corner near 4 (top layer)
    { 16, 15, 13, 10 },
    { 15, 13, 10, 9 },
    { 13, 10, 9, 4 },
    // W7: corner near 5 (top layer)
    { 14, 17, 16, 5 },
    { 17, 16, 5, 11 },
    { 16, 5, 11, 10 },
  };
  ptIds->SetNumberOfIds(96);
  std::copy_n(&ids[0][0], 96, ptIds->begin());
  return 1;
}

//------------------------------------------------------------------------------
// Given parametric coordinates compute inverse Jacobian transformation
// matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
// function derivatives.
void vtkBiQuadraticQuadraticWedge::JacobianInverse(
  const double pcoords[3], double** inverse, double derivs[54])
{
  double *m[3], m0[3], m1[3], m2[3];
  double x[3];

  // compute interpolation function derivatives
  vtkBiQuadraticQuadraticWedge::InterpolationDerivs(pcoords, derivs);

  // create Jacobian matrix
  m[0] = m0;
  m[1] = m1;
  m[2] = m2;

  for (int i = 0; i < 3; i++) // initialize matrix
  {
    m0[i] = m1[i] = m2[i] = 0.0;
  }

  for (int j = 0; j < 18; j++)
  {
    this->Points->GetPoint(j, x);
    for (int i = 0; i < 3; i++)
    {
      m0[i] += x[i] * derivs[j];
      m1[i] += x[i] * derivs[18 + j];
      m2[i] += x[i] * derivs[36 + j];
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
void vtkBiQuadraticQuadraticWedge::Derivatives(
  int vtkNotUsed(subId), const double pcoords[3], const double* values, int dim, double* derivs)
{
  double *jI[3], j0[3], j1[3], j2[3];
  double functionDerivs[3 * 18], sum[3];

  // compute inverse Jacobian and interpolation function derivatives
  jI[0] = j0;
  jI[1] = j1;
  jI[2] = j2;

  this->JacobianInverse(pcoords, jI, functionDerivs);

  // now compute derivates of values provided
  for (int k = 0; k < dim; k++) // loop over values per vertex
  {
    sum[0] = sum[1] = sum[2] = 0.0;
    for (int i = 0; i < 18; i++) // loop over interp. function derivatives
    {
      sum[0] += functionDerivs[i] * values[dim * i + k];
      sum[1] += functionDerivs[18 + i] * values[dim * i + k];
      sum[2] += functionDerivs[36 + i] * values[dim * i + k];
    }
    for (int j = 0; j < 3; j++) // loop over derivative directions
    {
      derivs[3 * k + j] = sum[0] * jI[j][0] + sum[1] * jI[j][1] + sum[2] * jI[j][2];
    }
  }
}

//------------------------------------------------------------------------------
// Compute interpolation functions for the fifteen nodes.
void vtkBiQuadraticQuadraticWedge::InterpolationFunctions(
  const double pcoords[3], double weights[18])
{
  // VTK needs parametric coordinates to be between (0,1). Isoparametric
  // shape functions are formulated between (-1,1). Here we do a
  // coordinate system conversion from (0,1) to (-1,1).
  double x = 2 * (pcoords[0] - 0.5);
  double y = 2 * (pcoords[1] - 0.5);
  double z = 2 * (pcoords[2] - 0.5);

  // clang-format off
  // corners
  weights[0] =-0.25 * (x + y) * (x + y + 1) * z * (1 - z);
  weights[1] =-0.25 *  x      * (x + 1)     * z * (1 - z);
  weights[2] =-0.25 *      y  * (1 + y)     * z * (1 - z);
  weights[3] = 0.25 * (x + y) * (x + y + 1) * z * (1 + z);
  weights[4] = 0.25 *  x      * (x + 1)     * z * (1 + z);
  weights[5] = 0.25 *      y  * (1 + y)     * z * (1 + z);

  // midsides of quadratic triangles
  weights[6] =  (x + 1) * (x + y) *  0.5 * z * (1 - z);
  weights[7] = -(x + 1) * (y + 1) *  0.5 * z * (1 - z);
  weights[8] =  (y + 1) * (x + y) *  0.5 * z * (1 - z);
  weights[9] = -(x + 1) * (x + y) *  0.5 * z * (1 + z);
  weights[10]=  (x + 1) * (y + 1) *  0.5 * z * (1 + z);
  weights[11]= -(y + 1) * (x + y) *  0.5 * z * (1 + z);

  // midsides of edges between the two triangles
  weights[12] = 0.5 * (x + y) * (x + y + 1) * (1 + z) * (1 - z);
  weights[13] = 0.5 *  x      * (x + 1)     * (1 + z) * (1 - z);
  weights[14] = 0.5 *      y  * (1 + y)     * (1 + z) * (1 - z);

  //Centerpoints of the biquadratic quads
  weights[15] = -(x + 1)*(x + y) * (1 + z) * (1 - z);
  weights[16] =  (x + 1)*(y + 1) * (1 + z) * (1 - z);
  weights[17] = -(y + 1)*(x + y) * (1 + z) * (1 - z);
  // clang-format on
}

//------------------------------------------------------------------------------
// Derivatives in parametric space.
void vtkBiQuadraticQuadraticWedge::InterpolationDerivs(const double pcoords[3], double derivs[54])
{
  // VTK needs parametric coordinates to be between (0,1). Isoparametric
  // shape functions are formulated between (-1,1). Here we do a
  // coordinate system conversion from (0,1) to (-1,1).
  double x = 2 * (pcoords[0] - 0.5);
  double y = 2 * (pcoords[1] - 0.5);
  double z = 2 * (pcoords[2] - 0.5);

  // clang-format off
  // Derivatives in x-direction
  // corners
  derivs[0] = -0.25 * (2 * x + 2 * y + 1) * z * (1 - z);
  derivs[1] = -0.25 * (2 * x + 1)         * z * (1 - z);
  derivs[2] =  0;
  derivs[3] =  0.25 * (2 * x + 2 * y + 1) * z * (1 + z);
  derivs[4] =  0.25 * (2 * x + 1)         * z * (1 + z);
  derivs[5] =  0;
  // midsides of quadratic triangles
  derivs[6] =  (2 * x + y + 1) * 0.5 * z * (1 - z);
  derivs[7] = -(y + 1)         * 0.5 * z * (1 - z);
  derivs[8] =  (y + 1)         * 0.5 * z * (1 - z);
  derivs[9] = -(2 * x + y + 1) * 0.5 * z * (1 + z);
  derivs[10] = (y + 1)         * 0.5 * z * (1 + z) ;
  derivs[11] =-(y + 1)         * 0.5 * z * (1 + z) ;
  // midsides of edges between the two triangles
  derivs[12] = 0.5 * (2 * x + 2 * y + 1) * (1 + z)*(1 - z);
  derivs[13] = 0.5 * (2 * x + 1)         * (1 + z)*(1 - z);
  derivs[14] = 0;
  // Centerpoints of the biquadratic quads
  derivs[15] = -(2 * x + y + 1) * (1 + z) * (1 - z);
  derivs[16] =  (y + 1)         * (1 + z) * (1 - z);
  derivs[17] = -(y + 1)         * (1 + z) * (1 - z);

  // Derivatives in y-direction
  // corners
  derivs[18] = -0.25 * (2 * y + 2 * x + 1) * z * (1 - z);
  derivs[19] =  0;
  derivs[20] = -0.25 * (2 * y + 1)         * z * (1 - z);
  derivs[21] =  0.25 * (2 * y + 2 * x + 1) * z * (1 + z);
  derivs[22] =  0;
  derivs[23] =  0.25 * (2 * y + 1)         * z * (1 + z);
  // midsides of quadratic triangles
  derivs[24] =  (x + 1)         * 0.5 * z * (1 - z);
  derivs[25] = -(x + 1)         * 0.5 * z * (1 - z);
  derivs[26] =  (2 * y + x + 1) * 0.5 * z * (1 - z);
  derivs[27] = -(x + 1)         * 0.5 * z * (1 + z);
  derivs[28] =  (x + 1)         * 0.5 * z * (1 + z);
  derivs[29] = -(2 * y + x + 1) * 0.5 * z * (1 + z);
  // midsides of edges between the two triangles
  derivs[30] = 0.5 * (2 * y + 2 * x + 1) * (1 + z) * (1 - z);
  derivs[31] = 0;
  derivs[32] = 0.5 * (2 * y + 1)         * (1 + z) * (1 - z);
  // Centerpoints of the biquadratic quads
  derivs[33] = -(x + 1)         * (1 + z) * (1 - z);
  derivs[34] =  (x + 1)         * (1 + z) * (1 - z);
  derivs[35] = -(2 * y + x + 1) * (1 + z) * (1 - z);

  // Derivatives in z-direction
  // corners
  derivs[36] = -0.25 * (x + y) * (x + y + 1) * (1 - 2 * z);
  derivs[37] = -0.25 *  x      * (x + 1)     * (1 - 2 * z);
  derivs[38] = -0.25 *      y  * (1 + y)     * (1 - 2 * z);
  derivs[39] =  0.25 * (x + y) * (x + y + 1) * (1 + 2 * z);
  derivs[40] =  0.25 *  x      * (x + 1)     * (1 + 2 * z);
  derivs[41] =  0.25 *      y  * (1 + y)     * (1 + 2 * z);
  // midsides of quadratic triangles
  derivs[42] =  (x + 1) * (x + y) *  0.5 * (1 - 2 * z);
  derivs[43] = -(x + 1) * (y + 1) *  0.5 * (1 - 2 * z);
  derivs[44] =  (y + 1) * (x + y) *  0.5 * (1 - 2 * z);
  derivs[45] = -(x + 1) * (x + y) *  0.5 * (1 + 2 * z);
  derivs[46] =  (x + 1) * (y + 1) *  0.5 * (1 + 2 * z);
  derivs[47] = -(y + 1) * (x + y) *  0.5 * (1 + 2 * z);
  // midsides of edges between the two triangles
  derivs[48] = 0.5 * (x + y) * (x + y + 1) * (-2 * z);
  derivs[49] = 0.5 *  x      * (x + 1)     * (-2 * z);
  derivs[50] = 0.5 *      y  * (1 + y)     * (-2 * z);
  // Centerpoints of the biquadratic quads
  derivs[51] = -(x + 1) * (x + y) * (-2 * z);
  derivs[52] =  (x + 1) * (y + 1) * (-2 * z);
  derivs[53] = -(y + 1) * (x + y) * (-2 * z);
  // clang-format on

  // we compute derivatives in [-1; 1] but we need them in [ 0; 1]
  for (int i = 0; i < 54; i++)
  {
    derivs[i] *= 2;
  }
}

//------------------------------------------------------------------------------
double* vtkBiQuadraticQuadraticWedge::GetParametricCoords()
{
  return ParametricCoords;
}

//------------------------------------------------------------------------------
void vtkBiQuadraticQuadraticWedge::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Edge:\n";
  this->Edge->PrintSelf(os, indent.GetNextIndent());
  os << indent << "TriangleFace:\n";
  this->TriangleFace->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Face:\n";
  this->Face->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Wedge:\n";
  this->Wedge->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Scalars:\n";
  this->Scalars->PrintSelf(os, indent.GetNextIndent());
}
VTK_ABI_NAMESPACE_END
