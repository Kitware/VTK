// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Thanks to Soeren Gebbert who developed this class and
// integrated it into VTK 5.0.

#include "vtkTriQuadraticHexahedron.h"

#include "vtkBiQuadraticQuad.h"
#include "vtkDoubleArray.h"
#include "vtkHexahedron.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkQuadraticEdge.h"
#include <algorithm>
#include <array>

namespace
{
//------------------------------------------------------------------------------
[[maybe_unused]] constexpr const char* QuadraticHexahedronTopology = R"(
   TriQuadraticHexahedron topology:

              7---------14--------6
             / |                / |
            /  |               /  |
           15  |     25       13  |
          /    19      23    /    18
         /     |            /     |
        4--------12--------5      |
        |  20  |     26    |  21  |
        |      3-------10--|------2
        |     /            |     /
       16    /   22        17   /
        |   11       24    |   9
        |  /               |  /
        | /                | /
        0---------8--------1
)";

//------------------------------------------------------------------------------
constexpr vtkNonLinearCell3D::PointType PointTypes[27] = {
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
  vtkNonLinearCell3D::PointType::FaceMidPoint, // point 20
  vtkNonLinearCell3D::PointType::FaceMidPoint, // point 21
  vtkNonLinearCell3D::PointType::FaceMidPoint, // point 22
  vtkNonLinearCell3D::PointType::FaceMidPoint, // point 23
  vtkNonLinearCell3D::PointType::FaceMidPoint, // point 24
  vtkNonLinearCell3D::PointType::FaceMidPoint, // point 25
  vtkNonLinearCell3D::PointType::CenterPoint,  // point 26
};

//------------------------------------------------------------------------------
double ParametricCoords[81] = {
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
  0.0, 1.0, 0.5, //
  0.0, 0.5, 0.5, // 20
  1.0, 0.5, 0.5, // 21
  0.5, 0.0, 0.5, // 22
  0.5, 1.0, 0.5, // 23
  0.5, 0.5, 0.0, // 24
  0.5, 0.5, 1.0, // 25
  0.5, 0.5, 0.5  // 26
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
constexpr vtkIdType Faces[6][9] = {
  { 0, 4, 7, 3, 16, 15, 19, 11, 20 },
  { 1, 2, 6, 5, 9, 18, 13, 17, 21 },
  { 0, 1, 5, 4, 8, 17, 12, 16, 22 },
  { 3, 7, 6, 2, 19, 14, 18, 10, 23 },
  { 0, 3, 2, 1, 11, 10, 9, 8, 24 },
  { 4, 5, 6, 7, 12, 13, 14, 15, 25 },
};

//------------------------------------------------------------------------------
constexpr vtkIdType EdgeToAdjacentFaces[12][2] = {
  { 2, 4 }, // edge 0:  corners 0,1
  { 1, 4 }, // edge 1:  corners 1,2
  { 3, 4 }, // edge 2:  corners 3,2
  { 0, 4 }, // edge 3:  corners 0,3
  { 2, 5 }, // edge 4:  corners 4,5
  { 1, 5 }, // edge 5:  corners 5,6
  { 3, 5 }, // edge 6:  corners 7,6
  { 0, 5 }, // edge 7:  corners 4,7
  { 0, 2 }, // edge 8:  corners 0,4
  { 1, 2 }, // edge 9:  corners 1,5
  { 0, 3 }, // edge 10: corners 3,7
  { 1, 3 }, // edge 11: corners 2,6
};

//------------------------------------------------------------------------------
constexpr vtkIdType FaceToAdjacentFaces[6][4] = {
  { 2, 3, 4, 5 }, // face 0
  { 2, 3, 4, 5 }, // face 1
  { 0, 1, 4, 5 }, // face 2
  { 0, 1, 4, 5 }, // face 3
  { 0, 1, 2, 3 }, // face 4
  { 0, 1, 2, 3 }, // face 5
};

//------------------------------------------------------------------------------
constexpr vtkIdType PointToIncidentEdges[27][3] = {
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
  { -1, -1, -1 }, // point 20: mid-face
  { -1, -1, -1 }, // point 21: mid-face
  { -1, -1, -1 }, // point 22: mid-face
  { -1, -1, -1 }, // point 23: mid-face
  { -1, -1, -1 }, // point 24: mid-face
  { -1, -1, -1 }, // point 25: mid-face
  { -1, -1, -1 }, // point 26: center
};

//------------------------------------------------------------------------------
constexpr vtkIdType PointToIncidentFaces[27][3] = {
  { 0, 2, 4 },    // point 0:  corner, 3 faces
  { 1, 2, 4 },    // point 1:  corner, 3 faces
  { 1, 3, 4 },    // point 2:  corner, 3 faces
  { 0, 3, 4 },    // point 3:  corner, 3 faces
  { 0, 2, 5 },    // point 4:  corner, 3 faces
  { 1, 2, 5 },    // point 5:  corner, 3 faces
  { 1, 3, 5 },    // point 6:  corner, 3 faces
  { 0, 3, 5 },    // point 7:  corner, 3 faces
  { 2, 4, -1 },   // point 8:  mid-edge, 2 faces
  { 1, 4, -1 },   // point 9:  mid-edge, 2 faces
  { 3, 4, -1 },   // point 10: mid-edge, 2 faces
  { 0, 4, -1 },   // point 11: mid-edge, 2 faces
  { 2, 5, -1 },   // point 12: mid-edge, 2 faces
  { 1, 5, -1 },   // point 13: mid-edge, 2 faces
  { 3, 5, -1 },   // point 14: mid-edge, 2 faces
  { 0, 5, -1 },   // point 15: mid-edge, 2 faces
  { 0, 2, -1 },   // point 16: mid-edge, 2 faces
  { 1, 2, -1 },   // point 17: mid-edge, 2 faces
  { 1, 3, -1 },   // point 18: mid-edge, 2 faces
  { 0, 3, -1 },   // point 19: mid-edge, 2 faces
  { 0, -1, -1 },  // point 20: mid-face, 1 face
  { 1, -1, -1 },  // point 21: mid-face, 1 face
  { 2, -1, -1 },  // point 22: mid-face, 1 face
  { 3, -1, -1 },  // point 23: mid-face, 1 face
  { 4, -1, -1 },  // point 24: mid-face, 1 face
  { 5, -1, -1 },  // point 25: mid-face, 1 face
  { -1, -1, -1 }, // point 26: center, 0 faces
};

//------------------------------------------------------------------------------
constexpr vtkIdType PointToOneRingPoints[27][6] = {
  { 1, 8, 3, 11, 4, 16 },     // point 0:  corner
  { 5, 17, 2, 9, 0, 8 },      // point 1:  corner
  { 6, 18, 3, 10, 1, 9 },     // point 2:  corner
  { 7, 19, 0, 11, 2, 10 },    // point 3:  corner
  { 0, 16, 7, 15, 5, 12 },    // point 4:  corner
  { 4, 12, 6, 13, 1, 17 },    // point 5:  corner
  { 5, 13, 7, 14, 2, 18 },    // point 6:  corner
  { 6, 14, 4, 15, 3, 19 },    // point 7:  corner
  { 0, 1, -1, -1, -1, -1 },   // point 8:  mid-edge
  { 1, 2, -1, -1, -1, -1 },   // point 9:  mid-edge
  { 3, 2, -1, -1, -1, -1 },   // point 10: mid-edge
  { 0, 3, -1, -1, -1, -1 },   // point 11: mid-edge
  { 4, 5, -1, -1, -1, -1 },   // point 12: mid-edge
  { 5, 6, -1, -1, -1, -1 },   // point 13: mid-edge
  { 7, 6, -1, -1, -1, -1 },   // point 14: mid-edge
  { 4, 7, -1, -1, -1, -1 },   // point 15: mid-edge
  { 0, 4, -1, -1, -1, -1 },   // point 16: mid-edge
  { 1, 5, -1, -1, -1, -1 },   // point 17: mid-edge
  { 2, 6, -1, -1, -1, -1 },   // point 18: mid-edge
  { 3, 7, -1, -1, -1, -1 },   // point 19: mid-edge
  { -1, -1, -1, -1, -1, -1 }, // point 20: mid-face
  { -1, -1, -1, -1, -1, -1 }, // point 21: mid-face
  { -1, -1, -1, -1, -1, -1 }, // point 22: mid-face
  { -1, -1, -1, -1, -1, -1 }, // point 23: mid-face
  { -1, -1, -1, -1, -1, -1 }, // point 24: mid-face
  { -1, -1, -1, -1, -1, -1 }, // point 25: mid-face
  { -1, -1, -1, -1, -1, -1 }, // point 26: center
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
vtkStandardNewMacro(vtkTriQuadraticHexahedron);

//------------------------------------------------------------------------------
// Construct the triquadhex with 27 nodes
vtkTriQuadraticHexahedron::vtkTriQuadraticHexahedron()
{
  this->Points->SetNumberOfPoints(27);
  this->PointIds->SetNumberOfIds(27);
  for (int i = 0; i < 27; i++)
  {
    this->Points->SetPoint(i, 0.0, 0.0, 0.0);
    this->PointIds->SetId(i, 0);
  }

  this->Edge = vtkSmartPointer<vtkQuadraticEdge>::New();
  this->Face = vtkSmartPointer<vtkBiQuadraticQuad>::New();
  this->Hex = vtkSmartPointer<vtkHexahedron>::New();

  this->Scalars = vtkSmartPointer<vtkDoubleArray>::New();
  this->Scalars->SetNumberOfTuples(8); // vertices of a linear hexahedron
}

//------------------------------------------------------------------------------
vtkCell* vtkTriQuadraticHexahedron::GetEdge(int edgeId)
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
vtkCell* vtkTriQuadraticHexahedron::GetFace(int faceId)
{
  faceId = std::clamp(faceId, 0, 5);

  for (int i = 0; i < 9; i++)
  {
    this->Face->PointIds->SetId(i, this->PointIds->GetId(Faces[faceId][i]));
    this->Face->Points->SetPoint(i, this->Points->GetPoint(Faces[faceId][i]));
  }

  return this->Face;
}

//------------------------------------------------------------------------------
const vtkIdType* vtkTriQuadraticHexahedron::GetEdgeArray(vtkIdType edgeId)
{
  return Edges[edgeId];
}
//------------------------------------------------------------------------------
const vtkIdType* vtkTriQuadraticHexahedron::GetFaceArray(vtkIdType faceId)
{
  return Faces[faceId];
}

//------------------------------------------------------------------------------
vtkNonLinearCell3D::PointType vtkTriQuadraticHexahedron::GetPointType(vtkIdType pointId)
{
  assert(pointId < GetNumberOfPoints() && "pointId too large");
  return PointTypes[pointId];
}

//------------------------------------------------------------------------------
vtkIdType vtkTriQuadraticHexahedron::GetEdgePoints(vtkIdType edgeId, const vtkIdType*& pts)
{
  pts = vtkTriQuadraticHexahedron::GetEdgeArray(edgeId);
  return 3;
}

//------------------------------------------------------------------------------
vtkIdType vtkTriQuadraticHexahedron::GetFacePoints(vtkIdType faceId, const vtkIdType*& pts)
{
  pts = vtkTriQuadraticHexahedron::GetFaceArray(faceId);
  return 9;
}

//------------------------------------------------------------------------------
void vtkTriQuadraticHexahedron::GetEdgeToAdjacentFaces(vtkIdType edgeId, const vtkIdType*& faceIds)
{
  assert(edgeId < GetNumberOfEdges() && "edgeId too large");
  faceIds = EdgeToAdjacentFaces[edgeId];
}

//------------------------------------------------------------------------------
vtkIdType vtkTriQuadraticHexahedron::GetFaceToAdjacentFaces(
  vtkIdType faceId, const vtkIdType*& faceIds)
{
  assert(faceId < GetNumberOfFaces() && "faceId too large");
  faceIds = FaceToAdjacentFaces[faceId];
  return 4;
}

//------------------------------------------------------------------------------
vtkIdType vtkTriQuadraticHexahedron::GetPointToIncidentEdges(
  vtkIdType pointId, const vtkIdType*& edgeIds)
{
  assert(pointId < GetNumberOfPoints() && "pointId too large");
  edgeIds = PointToIncidentEdges[pointId];
  if (pointId < /*corner points*/ 8)
  {
    return 3;
  }
  else if (pointId < /*mid edge points*/ 20)
  {
    return 1;
  }
  else /*mid face points/center*/
  {
    return 0;
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkTriQuadraticHexahedron::GetPointToIncidentFaces(
  vtkIdType pointId, const vtkIdType*& faceIds)
{
  assert(pointId < GetNumberOfPoints() && "pointId too large");
  faceIds = PointToIncidentFaces[pointId];
  if (pointId < /*corner points*/ 8)
  {
    return 3;
  }
  else if (pointId < /*mid edge points*/ 20)
  {
    return 2;
  }
  else if (pointId < /*mid edge points*/ 26)
  {
    return 1;
  }
  else /*corcer point*/
  {
    return 0;
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkTriQuadraticHexahedron::GetPointToOneRingPoints(
  vtkIdType pointId, const vtkIdType*& pts)
{
  assert(pointId < GetNumberOfPoints() && "pointId too large");
  pts = PointToOneRingPoints[pointId];
  if (pointId < /*corner points*/ 8)
  {
    return 6;
  }
  else if (pointId < /*mid edge points*/ 20)
  {
    return 2;
  }
  else /*mid face points/center*/
  {
    return 0;
  }
}

//------------------------------------------------------------------------------
int vtkTriQuadraticHexahedron::EvaluatePosition(const double* x, double* closestPoint, int& subId,
  double pcoords[3], double& dist2, double* weights)
{
  int converged;
  double params[3];
  double fcol[3], rcol[3], scol[3], tcol[3];
  const double* pt;
  double derivs[81];
  double hexweights[8];

  //  set initial position for Newton's method
  pcoords[0] = pcoords[1] = pcoords[2] = params[0] = params[1] = params[2] = 0.5;
  subId = 0;

  // Use a tri-linear hexahedron to get good starting values
  for (int i = 0; i < 8; i++)
  {
    this->Hex->GetPoints()->SetPoint(i, this->Points->GetPoint(i));
  }

  this->Hex->EvaluatePosition(x, closestPoint, subId, pcoords, dist2, hexweights);

  params[0] = pcoords[0];
  params[1] = pcoords[1];
  params[2] = pcoords[2];

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
    vtkTriQuadraticHexahedron::InterpolationFunctions(pcoords, weights);
    vtkTriQuadraticHexahedron::InterpolationDerivs(pcoords, derivs);

    //  calculate newton functions
    for (int i = 0; i < 3; i++)
    {
      fcol[i] = rcol[i] = scol[i] = tcol[i] = 0.0;
    }
    for (int i = 0; i < 27; i++)
    {
      pt = pts + 3 * i;
      for (int j = 0; j < 3; j++)
      {
        fcol[j] += pt[j] * weights[i];
        rcol[j] += pt[j] * derivs[i];
        scol[j] += pt[j] * derivs[i + 27];
        tcol[j] += pt[j] * derivs[i + 54];
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

  vtkTriQuadraticHexahedron::InterpolationFunctions(pcoords, weights);

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
    double pc[3], w[27];
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
void vtkTriQuadraticHexahedron::EvaluateLocation(
  int& vtkNotUsed(subId), const double pcoords[3], double x[3], double* weights)
{
  vtkTriQuadraticHexahedron::InterpolationFunctions(pcoords, weights);

  // Efficient point access
  const auto pointsArray = vtkDoubleArray::FastDownCast(this->Points->GetData());
  if (!pointsArray)
  {
    vtkErrorMacro(<< "Points should be double type");
    return;
  }
  const double* pts = pointsArray->GetPointer(0);

  x[0] = x[1] = x[2] = 0.0;
  for (int i = 0; i < 27; i++)
  {
    const double* pt = pts + 3 * i;
    for (int j = 0; j < 3; j++)
    {
      x[j] += pt[j] * weights[i];
    }
  }
}

//------------------------------------------------------------------------------
int vtkTriQuadraticHexahedron::CellBoundary(int subId, const double pcoords[3], vtkIdList* pts)
{
  return this->Hex->CellBoundary(subId, pcoords, pts);
}

//------------------------------------------------------------------------------
void vtkTriQuadraticHexahedron::Contour(double value, vtkDataArray* cellScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* verts, vtkCellArray* lines,
  vtkCellArray* polys, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId,
  vtkCellData* outCd)
{
  // contour each linear hex separately
  for (int i = 0; i < 8; i++)
  {
    for (int j = 0; j < 8; j++)
    {
      this->Hex->Points->SetPoint(j, this->Points->GetPoint(LinearCells[i][j]));
      this->Hex->PointIds->SetId(j, this->PointIds->GetId(LinearCells[i][j]));
      this->Scalars->SetValue(j, cellScalars->GetTuple1(LinearCells[i][j]));
    }
    this->Hex->Contour(
      value, this->Scalars, locator, verts, lines, polys, inPd, outPd, inCd, cellId, outCd);
  }
}

//------------------------------------------------------------------------------
// Clip this triquadratic hex using scalar value provided. Like contouring,
// except that it cuts the hex to produce tetrahedra.
void vtkTriQuadraticHexahedron::Clip(double value, vtkDataArray* cellScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* tets, vtkPointData* inPd, vtkPointData* outPd,
  vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd, int insideOut)
{
  // clip each linear hex separately
  for (int i = 0; i < 8; i++)
  {
    for (int j = 0; j < 8; j++)
    {
      this->Hex->Points->SetPoint(j, this->Points->GetPoint(LinearCells[i][j]));
      this->Hex->PointIds->SetId(j, this->PointIds->GetId(LinearCells[i][j]));
      this->Scalars->SetValue(j, cellScalars->GetTuple1(LinearCells[i][j]));
    }
    this->Hex->Clip(
      value, this->Scalars, locator, tets, inPd, outPd, inCd, cellId, outCd, insideOut);
  }
}

//------------------------------------------------------------------------------
// Line-hex intersection. Intersection has to occur within [0,1] parametric
// coordinates and with specified tolerance.
int vtkTriQuadraticHexahedron::IntersectWithLine(
  const double* p1, const double* p2, double tol, double& t, double* x, double* pcoords, int& subId)
{
  int intersection = 0;
  double tTemp;
  double pc[3], xTemp[3];

  t = VTK_DOUBLE_MAX;
  for (int faceNum = 0; faceNum < 6; faceNum++)
  {
    for (int i = 0; i < 9; i++)
    {
      this->Face->PointIds->SetId(i, this->PointIds->GetId(Faces[faceNum][i]));
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
int vtkTriQuadraticHexahedron::TriangulateLocalIds(int index, vtkIdList* ptIds)
{
  // Triangulation varies depending upon index See vtkHexahedron::TriangulateLocalIds
  ptIds->SetNumberOfIds(8 * 20);
  if ((index % 2))
  {
    constexpr std::array<vtkIdType, 20> linearHexPtIds{ 0, 1, 3, 4, 1, 4, 5, 6, 1, 4, 6, 3, 1, 3, 6,
      2, 3, 6, 7, 4 };
    for (int linear_hex_i = 0; linear_hex_i < 8; linear_hex_i++)
    {
      for (int node_i = 0; node_i < 20; node_i++)
      {
        ptIds->SetId(linear_hex_i * 20 + node_i, LinearCells[linear_hex_i][linearHexPtIds[node_i]]);
      }
    }
  }
  else
  {
    constexpr std::array<vtkIdType, 20> linearHexPtIds{ 2, 1, 5, 0, 0, 2, 3, 7, 2, 5, 6, 7, 0, 7, 4,
      5, 0, 2, 7, 5 };
    for (int linear_hex_i = 0; linear_hex_i < 8; linear_hex_i++)
    {
      for (int node_i = 0; node_i < 20; node_i++)
      {
        ptIds->SetId(linear_hex_i * 20 + node_i, LinearCells[linear_hex_i][linearHexPtIds[node_i]]);
      }
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
// Given parametric coordinates compute inverse Jacobian transformation
// matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
// function derivatives.
void vtkTriQuadraticHexahedron::JacobianInverse(
  const double pcoords[3], double** inverse, double derivs[81])
{
  double *m[3], m0[3], m1[3], m2[3];
  double x[3];

  // compute interpolation function derivatives
  vtkTriQuadraticHexahedron::InterpolationDerivs(pcoords, derivs);

  // create Jacobian matrix
  m[0] = m0;
  m[1] = m1;
  m[2] = m2;
  for (int i = 0; i < 3; i++) // initialize matrix
  {
    m0[i] = m1[i] = m2[i] = 0.0;
  }

  for (int j = 0; j < 27; j++)
  {
    this->Points->GetPoint(j, x);
    for (int i = 0; i < 3; i++)
    {
      m0[i] += x[i] * derivs[j];
      m1[i] += x[i] * derivs[27 + j];
      m2[i] += x[i] * derivs[54 + j];
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
void vtkTriQuadraticHexahedron::Derivatives(
  int vtkNotUsed(subId), const double pcoords[3], const double* values, int dim, double* derivs)
{
  double *jI[3], j0[3], j1[3], j2[3];
  double functionDerivs[81], sum[3];

  // compute inverse Jacobian and interpolation function derivatives
  jI[0] = j0;
  jI[1] = j1;
  jI[2] = j2;
  this->JacobianInverse(pcoords, jI, functionDerivs);

  // now compute derivates of values provided
  for (int k = 0; k < dim; k++) // loop over values per vertex
  {
    sum[0] = sum[1] = sum[2] = 0.0;
    for (int i = 0; i < 27; i++) // loop over interp. function derivatives
    {
      sum[0] += functionDerivs[i] * values[dim * i + k];
      sum[1] += functionDerivs[27 + i] * values[dim * i + k];
      sum[2] += functionDerivs[54 + i] * values[dim * i + k];
    }
    for (int j = 0; j < 3; j++) // loop over derivative directions
    {
      derivs[3 * k + j] = sum[0] * jI[j][0] + sum[1] * jI[j][1] + sum[2] * jI[j][2];
    }
  }
}

//------------------------------------------------------------------------------
// Compute interpolation functions for the 27 nodes.
void vtkTriQuadraticHexahedron::InterpolationFunctions(const double pcoords[3], double weights[27])
{
  // VTK needs parametric coordinates to be between (0,1). Isoparametric
  // shape functions are formulated between (-1,1). Here we do a
  // coordinate system conversion from (0,1) to (-1,1).
  double r = 2.0 * (pcoords[0] - 0.5);
  double s = 2.0 * (pcoords[1] - 0.5);
  double t = 2.0 * (pcoords[2] - 0.5);

  double g1r = -0.5 * r * (1 - r);
  double g1s = -0.5 * s * (1 - s);
  double g1t = -0.5 * t * (1 - t);

  double g2r = (1 + r) * (1 - r);
  double g2s = (1 + s) * (1 - s);
  double g2t = (1 + t) * (1 - t);

  double g3r = 0.5 * r * (1 + r);
  double g3s = 0.5 * s * (1 + s);
  double g3t = 0.5 * t * (1 + t);

  // The eight corner points
  weights[0] = g1r * g1s * g1t;
  weights[1] = g3r * g1s * g1t;
  weights[2] = g3r * g3s * g1t;
  weights[3] = g1r * g3s * g1t;
  weights[4] = g1r * g1s * g3t;
  weights[5] = g3r * g1s * g3t;
  weights[6] = g3r * g3s * g3t;
  weights[7] = g1r * g3s * g3t;

  // The mid-edge nodes
  weights[8] = g2r * g1s * g1t;
  weights[9] = g3r * g2s * g1t;
  weights[10] = g2r * g3s * g1t;
  weights[11] = g1r * g2s * g1t;
  weights[12] = g2r * g1s * g3t;
  weights[13] = g3r * g2s * g3t;
  weights[14] = g2r * g3s * g3t;
  weights[15] = g1r * g2s * g3t;
  weights[16] = g1r * g1s * g2t;
  weights[17] = g3r * g1s * g2t;
  weights[18] = g3r * g3s * g2t;
  weights[19] = g1r * g3s * g2t;

  // face center nodes
  weights[22] = g2r * g1s * g2t;
  weights[21] = g3r * g2s * g2t;
  weights[23] = g2r * g3s * g2t;
  weights[20] = g1r * g2s * g2t;
  weights[24] = g2r * g2s * g1t;
  weights[25] = g2r * g2s * g3t;

  // Cell center node
  weights[26] = g2r * g2s * g2t;
}

//------------------------------------------------------------------------------
// Derivatives in parametric space.
void vtkTriQuadraticHexahedron::InterpolationDerivs(const double pcoords[3], double derivs[81])
{
  // VTK needs parametric coordinates to be between (0,1). Isoparametric
  // shape functions are formulated between (-1,1). Here we do a
  // coordinate system conversion from (0,1) to (-1,1).
  double r = 2.0 * (pcoords[0] - 0.5);
  double s = 2.0 * (pcoords[1] - 0.5);
  double t = 2.0 * (pcoords[2] - 0.5);

  double g1r = -0.5 * r * (1 - r);
  double g1s = -0.5 * s * (1 - s);
  double g1t = -0.5 * t * (1 - t);

  double g2r = (1 + r) * (1 - r);
  double g2s = (1 + s) * (1 - s);
  double g2t = (1 + t) * (1 - t);

  double g3r = 0.5 * r * (1 + r);
  double g3s = 0.5 * s * (1 + s);
  double g3t = 0.5 * t * (1 + t);

  double g1r_r = r - 0.5;
  double g1s_s = s - 0.5;
  double g1t_t = t - 0.5;

  double g2r_r = -2 * r;
  double g2s_s = -2 * s;
  double g2t_t = -2 * t;

  double g3r_r = r + 0.5;
  double g3s_s = s + 0.5;
  double g3t_t = t + 0.5;

  // r-derivatives
  derivs[0] = g1r_r * g1s * g1t;
  derivs[1] = g3r_r * g1s * g1t;
  derivs[2] = g3r_r * g3s * g1t;
  derivs[3] = g1r_r * g3s * g1t;
  derivs[4] = g1r_r * g1s * g3t;
  derivs[5] = g3r_r * g1s * g3t;
  derivs[6] = g3r_r * g3s * g3t;
  derivs[7] = g1r_r * g3s * g3t;
  derivs[8] = g2r_r * g1s * g1t;
  derivs[9] = g3r_r * g2s * g1t;
  derivs[10] = g2r_r * g3s * g1t;
  derivs[11] = g1r_r * g2s * g1t;
  derivs[12] = g2r_r * g1s * g3t;
  derivs[13] = g3r_r * g2s * g3t;
  derivs[14] = g2r_r * g3s * g3t;
  derivs[15] = g1r_r * g2s * g3t;
  derivs[16] = g1r_r * g1s * g2t;
  derivs[17] = g3r_r * g1s * g2t;
  derivs[18] = g3r_r * g3s * g2t;
  derivs[19] = g1r_r * g3s * g2t;
  derivs[20] = g1r_r * g2s * g2t;
  derivs[21] = g3r_r * g2s * g2t;
  derivs[22] = g2r_r * g1s * g2t;
  derivs[23] = g2r_r * g3s * g2t;
  derivs[24] = g2r_r * g2s * g1t;
  derivs[25] = g2r_r * g2s * g3t;
  derivs[26] = g2r_r * g2s * g2t;

  // s-derivatives
  derivs[27] = g1r * g1s_s * g1t;
  derivs[28] = g3r * g1s_s * g1t;
  derivs[29] = g3r * g3s_s * g1t;
  derivs[30] = g1r * g3s_s * g1t;
  derivs[31] = g1r * g1s_s * g3t;
  derivs[32] = g3r * g1s_s * g3t;
  derivs[33] = g3r * g3s_s * g3t;
  derivs[34] = g1r * g3s_s * g3t;
  derivs[35] = g2r * g1s_s * g1t;
  derivs[36] = g3r * g2s_s * g1t;
  derivs[37] = g2r * g3s_s * g1t;
  derivs[38] = g1r * g2s_s * g1t;
  derivs[39] = g2r * g1s_s * g3t;
  derivs[40] = g3r * g2s_s * g3t;
  derivs[41] = g2r * g3s_s * g3t;
  derivs[42] = g1r * g2s_s * g3t;
  derivs[43] = g1r * g1s_s * g2t;
  derivs[44] = g3r * g1s_s * g2t;
  derivs[45] = g3r * g3s_s * g2t;
  derivs[46] = g1r * g3s_s * g2t;
  derivs[47] = g1r * g2s_s * g2t;
  derivs[48] = g3r * g2s_s * g2t;
  derivs[49] = g2r * g1s_s * g2t;
  derivs[50] = g2r * g3s_s * g2t;
  derivs[51] = g2r * g2s_s * g1t;
  derivs[52] = g2r * g2s_s * g3t;
  derivs[53] = g2r * g2s_s * g2t;

  // t-derivatives
  derivs[54] = g1r * g1s * g1t_t;
  derivs[55] = g3r * g1s * g1t_t;
  derivs[56] = g3r * g3s * g1t_t;
  derivs[57] = g1r * g3s * g1t_t;
  derivs[58] = g1r * g1s * g3t_t;
  derivs[59] = g3r * g1s * g3t_t;
  derivs[60] = g3r * g3s * g3t_t;
  derivs[61] = g1r * g3s * g3t_t;
  derivs[62] = g2r * g1s * g1t_t;
  derivs[63] = g3r * g2s * g1t_t;
  derivs[64] = g2r * g3s * g1t_t;
  derivs[65] = g1r * g2s * g1t_t;
  derivs[66] = g2r * g1s * g3t_t;
  derivs[67] = g3r * g2s * g3t_t;
  derivs[68] = g2r * g3s * g3t_t;
  derivs[69] = g1r * g2s * g3t_t;
  derivs[70] = g1r * g1s * g2t_t;
  derivs[71] = g3r * g1s * g2t_t;
  derivs[72] = g3r * g3s * g2t_t;
  derivs[73] = g1r * g3s * g2t_t;
  derivs[74] = g1r * g2s * g2t_t;
  derivs[75] = g3r * g2s * g2t_t;
  derivs[76] = g2r * g1s * g2t_t;
  derivs[77] = g2r * g3s * g2t_t;
  derivs[78] = g2r * g2s * g1t_t;
  derivs[79] = g2r * g2s * g3t_t;
  derivs[80] = g2r * g2s * g2t_t;

  // we compute derivatives in [-1; 1] but we need them in [0; 1]
  for (int i = 0; i < 81; i++)
  {
    derivs[i] *= 2;
  }
}

//------------------------------------------------------------------------------
double* vtkTriQuadraticHexahedron::GetParametricCoords()
{
  return ParametricCoords;
}

//------------------------------------------------------------------------------
void vtkTriQuadraticHexahedron::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Edge:\n";
  this->Edge->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Face:\n";
  this->Face->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Hex:\n";
  this->Hex->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Scalars:\n";
  this->Scalars->PrintSelf(os, indent.GetNextIndent());
}
VTK_ABI_NAMESPACE_END
