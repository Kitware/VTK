// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Thanks to Soeren Gebbert who developed this class and
// integrated it into VTK 5.0.

#include "vtkBiQuadraticQuadraticHexahedron.h"

#include "vtkBiQuadraticQuad.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkHexahedron.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkQuadraticEdge.h"
#include "vtkQuadraticQuad.h"

#include <algorithm>
#include <cassert>

namespace
{
//------------------------------------------------------------------------------
[[maybe_unused]] constexpr const char* QuadraticHexahedronTopology = R"(
   TriQuadraticHexahedron topology:

              7---------14--------6
             / |                / |
            /  |               /  |
           15  |              13  |
          /    19      23    /    18
         /     |            /     |
        4--------12--------5      |
        |  20  |           |  21  |
        |      3-------10--|------2
        |     /            |     /
       16    /   22        17   /
        |   11             |   9
        |  /               |  /
        | /                | /
        0---------8--------1
)";

//------------------------------------------------------------------------------
constexpr vtkNonLinearCell3D::PointType PointTypes[24] = {
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
};

//------------------------------------------------------------------------------
double ParametricCoords[72] = {
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
  0.5, 1.0, 0.5  // 23
};

//------------------------------------------------------------------------------
constexpr double MidPoints[3][3] = { { 0.5, 0.5, 0.0 }, { 0.5, 0.5, 1.0 }, { 0.5, 0.5, 0.5 } };

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
  { 0, 4, 7, 3, 16, 15, 19, 11, 20 }, // BiQuadQuad
  { 1, 2, 6, 5, 9, 18, 13, 17, 21 },  // BiQuadQuad
  { 0, 1, 5, 4, 8, 17, 12, 16, 22 },  // BiQuadQuad
  { 3, 7, 6, 2, 19, 14, 18, 10, 23 }, // BiQuadQuad
  { 0, 3, 2, 1, 11, 10, 9, 8, -1 },   // QuadQuad
  { 4, 5, 6, 7, 12, 13, 14, 15, -1 }, // QuadQuad
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
  { 2, 3, 4, 5 }, // face 0: BiQuadQuad
  { 2, 3, 4, 5 }, // face 1: BiQuadQuad
  { 0, 1, 4, 5 }, // face 2: BiQuadQuad
  { 0, 1, 4, 5 }, // face 3: BiQuadQuad
  { 0, 1, 2, 3 }, // face 4: QuadQuad
  { 0, 1, 2, 3 }, // face 5: QuadQuad
};

//------------------------------------------------------------------------------
constexpr vtkIdType PointToIncidentEdges[24][3] = {
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
};

//------------------------------------------------------------------------------
constexpr vtkIdType PointToIncidentFaces[24][3] = {
  { 0, 2, 4 },   // point 0:  corner, 3 faces
  { 1, 2, 4 },   // point 1:  corner, 3 faces
  { 1, 3, 4 },   // point 2:  corner, 3 faces
  { 0, 3, 4 },   // point 3:  corner, 3 faces
  { 0, 2, 5 },   // point 4:  corner, 3 faces
  { 1, 2, 5 },   // point 5:  corner, 3 faces
  { 1, 3, 5 },   // point 6:  corner, 3 faces
  { 0, 3, 5 },   // point 7:  corner, 3 faces
  { 2, 4, -1 },  // point 8:  mid-edge, 2 faces
  { 1, 4, -1 },  // point 9:  mid-edge, 2 faces
  { 3, 4, -1 },  // point 10: mid-edge, 2 faces
  { 0, 4, -1 },  // point 11: mid-edge, 2 faces
  { 2, 5, -1 },  // point 12: mid-edge, 2 faces
  { 1, 5, -1 },  // point 13: mid-edge, 2 faces
  { 3, 5, -1 },  // point 14: mid-edge, 2 faces
  { 0, 5, -1 },  // point 15: mid-edge, 2 faces
  { 0, 2, -1 },  // point 16: mid-edge, 2 faces
  { 1, 2, -1 },  // point 17: mid-edge, 2 faces
  { 1, 3, -1 },  // point 18: mid-edge, 2 faces
  { 0, 3, -1 },  // point 19: mid-edge, 2 faces
  { 0, -1, -1 }, // point 20: mid-face, 1 face
  { 1, -1, -1 }, // point 21: mid-face, 1 face
  { 2, -1, -1 }, // point 22: mid-face, 1 face
  { 3, -1, -1 }, // point 23: mid-face, 1 face
};

//------------------------------------------------------------------------------
constexpr vtkIdType PointToOneRingPoints[24][6] = {
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
vtkStandardNewMacro(vtkBiQuadraticQuadraticHexahedron);

//------------------------------------------------------------------------------
// Construct the hex with 24 points + 3 extra points for internal
// computation.
vtkBiQuadraticQuadraticHexahedron::vtkBiQuadraticQuadraticHexahedron()
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
  this->Points->SetNumberOfPoints(24);
  this->PointIds->SetNumberOfIds(24);

  this->Edge = vtkSmartPointer<vtkQuadraticEdge>::New();
  this->Face = vtkSmartPointer<vtkQuadraticQuad>::New();
  this->BiQuadFace = vtkSmartPointer<vtkBiQuadraticQuad>::New();
  this->Hex = vtkSmartPointer<vtkHexahedron>::New();

  this->PointData = vtkSmartPointer<vtkPointData>::New();
  this->CellData = vtkSmartPointer<vtkCellData>::New();
  this->CellScalars = vtkSmartPointer<vtkDoubleArray>::New();
  this->CellScalars->SetNumberOfTuples(27);
  this->Scalars = vtkSmartPointer<vtkDoubleArray>::New();
  this->Scalars->SetNumberOfTuples(8); // vertices of a linear hexahedron
}

//------------------------------------------------------------------------------
vtkCell* vtkBiQuadraticQuadraticHexahedron::GetEdge(int edgeId)
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
vtkCell* vtkBiQuadraticQuadraticHexahedron::GetFace(int faceId)
{
  faceId = std::clamp(faceId, 0, 5);

  // 4 BiQuaduadaticQuads
  if (faceId < 4)
  {
    for (int i = 0; i < 9; i++)
    {
      this->BiQuadFace->PointIds->SetId(i, this->PointIds->GetId(Faces[faceId][i]));
      this->BiQuadFace->Points->SetPoint(i, this->Points->GetPoint(Faces[faceId][i]));
    }
    return this->BiQuadFace;
  }
  else
  { // 2 QuadraticQuads
    for (int i = 0; i < 8; i++)
    {
      this->Face->PointIds->SetId(i, this->PointIds->GetId(Faces[faceId][i]));
      this->Face->Points->SetPoint(i, this->Points->GetPoint(Faces[faceId][i]));
    }
    return this->Face;
  }
}

//------------------------------------------------------------------------------
const vtkIdType* vtkBiQuadraticQuadraticHexahedron::GetEdgeArray(vtkIdType edgeId)
{
  return Edges[edgeId];
}
//------------------------------------------------------------------------------
const vtkIdType* vtkBiQuadraticQuadraticHexahedron::GetFaceArray(vtkIdType faceId)
{
  return Faces[faceId];
}

//------------------------------------------------------------------------------
vtkNonLinearCell3D::PointType vtkBiQuadraticQuadraticHexahedron::GetPointType(vtkIdType pointId)
{
  assert(pointId < GetNumberOfPoints() && "pointId too large");
  return PointTypes[pointId];
}

//------------------------------------------------------------------------------
vtkIdType vtkBiQuadraticQuadraticHexahedron::GetEdgePoints(vtkIdType edgeId, const vtkIdType*& pts)
{
  pts = vtkBiQuadraticQuadraticHexahedron::GetEdgeArray(edgeId);
  return 3;
}

//------------------------------------------------------------------------------
vtkIdType vtkBiQuadraticQuadraticHexahedron::GetFacePoints(vtkIdType faceId, const vtkIdType*& pts)
{
  pts = vtkBiQuadraticQuadraticHexahedron::GetFaceArray(faceId);
  return faceId < 4 ? 9 : 8; // BiQuadQuad faces have 9 points, QuadQuad faces have 8 points
}

//------------------------------------------------------------------------------
void vtkBiQuadraticQuadraticHexahedron::GetEdgeToAdjacentFaces(
  vtkIdType edgeId, const vtkIdType*& faceIds)
{
  assert(edgeId < GetNumberOfEdges() && "edgeId too large");
  faceIds = EdgeToAdjacentFaces[edgeId];
}

//------------------------------------------------------------------------------
vtkIdType vtkBiQuadraticQuadraticHexahedron::GetFaceToAdjacentFaces(
  vtkIdType faceId, const vtkIdType*& faceIds)
{
  assert(faceId < GetNumberOfFaces() && "faceId too large");
  faceIds = FaceToAdjacentFaces[faceId];
  return 4;
}

//------------------------------------------------------------------------------
vtkIdType vtkBiQuadraticQuadraticHexahedron::GetPointToIncidentEdges(
  vtkIdType pointId, const vtkIdType*& edgeIds)
{
  assert(pointId < GetNumberOfPoints() && "pointId too large");
  edgeIds = PointToIncidentEdges[pointId];
  if (pointId < /*corner points*/ 8)
  {
    return 3;
  }
  else if (pointId < 20 /*mid edge points*/)
  {
    return 1;
  }
  else /*mid face points*/
  {
    return 0;
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkBiQuadraticQuadraticHexahedron::GetPointToIncidentFaces(
  vtkIdType pointId, const vtkIdType*& faceIds)
{
  assert(pointId < GetNumberOfPoints() && "pointId too large");
  faceIds = PointToIncidentFaces[pointId];
  if (pointId < /*corner points*/ 8)
  {
    return 3;
  }
  else if (pointId < 20 /*mid edge points*/)
  {
    return 2;
  }
  else /*mid face points*/
  {
    return 1;
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkBiQuadraticQuadraticHexahedron::GetPointToOneRingPoints(
  vtkIdType pointId, const vtkIdType*& pts)
{
  assert(pointId < GetNumberOfPoints() && "pointId too large");
  pts = PointToOneRingPoints[pointId];
  if (pointId < /*corner points*/ 8)
  {
    return 6;
  }
  else if (pointId < 20 /*mid edge points*/)
  {
    return 2;
  }
  else /*mid face points*/
  {
    return 0;
  }
}

//------------------------------------------------------------------------------
void vtkBiQuadraticQuadraticHexahedron::Subdivide(
  vtkPointData* inPd, vtkCellData* inCd, vtkIdType cellId, vtkDataArray* cellScalars)
{
  double weights[24], x[3];

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
  for (int i = 0; i < 24; i++)
  {
    this->PointData->CopyData(inPd, this->PointIds->GetId(i), i);
    this->CellScalars->SetValue(i, cellScalars->GetTuple1(i));
  }
  this->CellData->CopyData(inCd, cellId, 0);

  // Interpolate new values
  double p[3];
  this->Points->Reserve(27);
  this->CellScalars->ReserveTuples(27);
  for (int numMidPts = 0; numMidPts < 3; numMidPts++)
  {
    vtkBiQuadraticQuadraticHexahedron::InterpolationFunctions(MidPoints[numMidPts], weights);

    x[0] = x[1] = x[2] = 0.0;
    double s = 0.0;
    for (int i = 0; i < 24; i++)
    {
      this->Points->GetPoint(i, p);
      for (int j = 0; j < 3; j++)
      {
        x[j] += p[j] * weights[i];
      }
      s += cellScalars->GetTuple1(i) * weights[i];
    }
    this->Points->SetPoint(24 + numMidPts, x);
    this->CellScalars->SetValue(24 + numMidPts, s);
    this->PointData->InterpolatePoint(inPd, 24 + numMidPts, this->PointIds, weights);
  }
}

//------------------------------------------------------------------------------
int vtkBiQuadraticQuadraticHexahedron::EvaluatePosition(const double x[3], double closestPoint[3],
  int& subId, double pcoords[3], double& dist2, double weights[])
{
  int converged;
  double params[3];
  double fcol[3], rcol[3], scol[3], tcol[3];
  double derivs[72];
  double hexweights[8];

  //  set initial position for Newton's method
  pcoords[0] = pcoords[1] = pcoords[2] = params[0] = params[1] = params[2] = 0.0;
  subId = 0;

  // Efficient point access
  const auto pointsArray = vtkDoubleArray::FastDownCast(this->Points->GetData());
  if (!pointsArray)
  {
    vtkErrorMacro(<< "Points should be double type");
    return 0;
  }
  const double* pts = pointsArray->GetPointer(0);

  // Use a tri-linear hexahederon to get good starting values
  vtkHexahedron* hex = vtkHexahedron::New();
  for (int i = 0; i < 8; i++)
  {
    hex->GetPoints()->SetPoint(i, pts + 3 * i);
  }

  hex->EvaluatePosition(x, closestPoint, subId, pcoords, dist2, hexweights);
  hex->Delete();

  params[0] = pcoords[0];
  params[1] = pcoords[1];
  params[2] = pcoords[2];

  //  enter iteration loop
  for (int iteration = converged = 0; !converged && (iteration < VTK_MAX_ITERATIONS); iteration++)
  {
    //  calculate element interpolation functions and derivatives
    vtkBiQuadraticQuadraticHexahedron::InterpolationFunctions(pcoords, weights);
    vtkBiQuadraticQuadraticHexahedron::InterpolationDerivs(pcoords, derivs);

    //  calculate newton functions
    for (int i = 0; i < 3; i++)
    {
      fcol[i] = rcol[i] = scol[i] = tcol[i] = 0.0;
    }
    for (int i = 0; i < 24; i++)
    {
      const double* pt = pts + 3 * i;
      for (int j = 0; j < 3; j++)
      {
        fcol[j] += pt[j] * weights[i];
        rcol[j] += pt[j] * derivs[i];
        scol[j] += pt[j] * derivs[i + 24];
        tcol[j] += pt[j] * derivs[i + 48];
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

  vtkBiQuadraticQuadraticHexahedron::InterpolationFunctions(pcoords, weights);

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
    double pc[3], w[24];
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
void vtkBiQuadraticQuadraticHexahedron::EvaluateLocation(
  int& vtkNotUsed(subId), const double pcoords[3], double x[3], double* weights)
{
  vtkBiQuadraticQuadraticHexahedron::InterpolationFunctions(pcoords, weights);

  // Efficient point access
  const auto pointsArray = vtkDoubleArray::FastDownCast(this->Points->GetData());
  if (!pointsArray)
  {
    vtkErrorMacro(<< "Points should be double type");
    return;
  }
  const double* pts = pointsArray->GetPointer(0);

  x[0] = x[1] = x[2] = 0.0;
  for (int i = 0; i < 24; i++)
  {
    const double* pt = pts + 3 * i;
    for (int j = 0; j < 3; j++)
    {
      x[j] += pt[j] * weights[i];
    }
  }
}

//------------------------------------------------------------------------------
int vtkBiQuadraticQuadraticHexahedron::CellBoundary(
  int subId, const double pcoords[3], vtkIdList* pts)
{
  return this->Hex->CellBoundary(subId, pcoords, pts);
}

//------------------------------------------------------------------------------
void vtkBiQuadraticQuadraticHexahedron::Contour(double value, vtkDataArray* cellScalars,
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
int vtkBiQuadraticQuadraticHexahedron::IntersectWithLine(
  const double* p1, const double* p2, double tol, double& t, double* x, double* pcoords, int& subId)
{
  int intersection = 0;
  double tTemp;
  double pc[3], xTemp[3];

  t = VTK_DOUBLE_MAX;
  for (int faceNum = 0; faceNum < 6; faceNum++)
  {
    int status = 0;
    // 4 BiQuaduadaticQuads
    if (faceNum < 4)
    {
      for (int i = 0; i < 9; i++)
      {
        this->BiQuadFace->PointIds->SetId(i, this->PointIds->GetId(Faces[faceNum][i]));
        this->BiQuadFace->Points->SetPoint(i, this->Points->GetPoint(Faces[faceNum][i]));
      }
      status = this->BiQuadFace->IntersectWithLine(p1, p2, tol, tTemp, xTemp, pc, subId);
    }
    else
    { // 2 QuadraticQuads
      for (int i = 0; i < 8; i++)
      {
        this->Face->PointIds->SetId(i, this->PointIds->GetId(Faces[faceNum][i]));
        this->Face->Points->SetPoint(i, this->Points->GetPoint(Faces[faceNum][i]));
      }
      status = this->Face->IntersectWithLine(p1, p2, tol, tTemp, xTemp, pc, subId);
    }

    if (status)
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
int vtkBiQuadraticQuadraticHexahedron::TriangulateLocalIds(int vtkNotUsed(index), vtkIdList* ptIds)
{
  // 12 wedges x 3 tets = 36 tets
  // Split into 2 layers (bottom z=0->0.5, top z=0.5->1),
  // each layer split into 6 wedges using face midpoints.
  constexpr vtkIdType wedges[12][6] = {
    // Bottom layer
    { 0, 8, 11, 16, 22, 20 },
    { 8, 1, 9, 22, 17, 21 },
    { 9, 2, 10, 21, 18, 23 },
    { 10, 3, 11, 23, 19, 20 },
    { 8, 9, 11, 22, 21, 20 },
    { 9, 10, 11, 21, 23, 20 },
    // Top layer
    { 16, 22, 20, 4, 12, 15 },
    { 22, 17, 21, 12, 5, 13 },
    { 21, 18, 23, 13, 6, 14 },
    { 23, 19, 20, 14, 7, 15 },
    { 22, 21, 20, 12, 13, 15 },
    { 21, 23, 20, 13, 14, 15 },
  };

  ptIds->SetNumberOfIds(12 * 3 * 4);
  int idx = 0;
  for (int wi = 0; wi < 12; wi++)
  {
    const vtkIdType* w = wedges[wi];
    // wedge(a,b,c/d,e,f) -> 3 tets: (a,b,c,d), (b,c,d,e), (c,d,e,f)
    ptIds->SetId(idx++, w[0]);
    ptIds->SetId(idx++, w[1]);
    ptIds->SetId(idx++, w[2]);
    ptIds->SetId(idx++, w[3]);

    ptIds->SetId(idx++, w[1]);
    ptIds->SetId(idx++, w[2]);
    ptIds->SetId(idx++, w[3]);
    ptIds->SetId(idx++, w[4]);

    ptIds->SetId(idx++, w[2]);
    ptIds->SetId(idx++, w[3]);
    ptIds->SetId(idx++, w[4]);
    ptIds->SetId(idx++, w[5]);
  }
  return 1;
}

//------------------------------------------------------------------------------
// Given parametric coordinates compute inverse Jacobian transformation
// matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
// function derivatives.
void vtkBiQuadraticQuadraticHexahedron::JacobianInverse(
  const double pcoords[3], double** inverse, double derivs[72])
{
  double *m[3], m0[3], m1[3], m2[3];
  double x[3];

  // compute interpolation function derivatives
  vtkBiQuadraticQuadraticHexahedron::InterpolationDerivs(pcoords, derivs);

  // create Jacobian matrix
  m[0] = m0;
  m[1] = m1;
  m[2] = m2;
  for (int i = 0; i < 3; i++) // initialize matrix
  {
    m0[i] = m1[i] = m2[i] = 0.0;
  }

  for (int j = 0; j < 24; j++)
  {
    this->Points->GetPoint(j, x);
    for (int i = 0; i < 3; i++)
    {
      m0[i] += x[i] * derivs[j];
      m1[i] += x[i] * derivs[24 + j];
      m2[i] += x[i] * derivs[48 + j];
    }
  }

  // now find the inverse
  if (vtkMath::InvertMatrix(m, inverse, 3) == 0)
  {
    return;
  }
}

//------------------------------------------------------------------------------
void vtkBiQuadraticQuadraticHexahedron::Derivatives(
  int vtkNotUsed(subId), const double pcoords[3], const double* values, int dim, double* derivs)
{
  double *jI[3], j0[3], j1[3], j2[3];
  double functionDerivs[72], sum[3];

  // compute inverse Jacobian and interpolation function derivatives
  jI[0] = j0;
  jI[1] = j1;
  jI[2] = j2;
  this->JacobianInverse(pcoords, jI, functionDerivs);

  // now compute derivates of values provided
  for (int k = 0; k < dim; k++) // loop over values per vertex
  {
    sum[0] = sum[1] = sum[2] = 0.0;
    for (int i = 0; i < 24; i++) // loop over interp. function derivatives
    {
      sum[0] += functionDerivs[i] * values[dim * i + k];
      sum[1] += functionDerivs[24 + i] * values[dim * i + k];
      sum[2] += functionDerivs[48 + i] * values[dim * i + k];
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
void vtkBiQuadraticQuadraticHexahedron::Clip(double value, vtkDataArray* cellScalars,
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
// Compute interpolation functions for the twenty four nodes.
void vtkBiQuadraticQuadraticHexahedron::InterpolationFunctions(
  const double pcoords[3], double weights[24])
{
  // VTK needs parametric coordinates to be between (0,1). Isoparametric
  // shape functions are formulated between (-1,1). Here we do a
  // coordinate system conversion from (0,1) to (-1,1).
  double x = 2.0 * (pcoords[0] - 0.5);
  double y = 2.0 * (pcoords[1] - 0.5);
  double z = 2.0 * (pcoords[2] - 0.5);

  // clang-format off
  //The eight corner points
  weights[0] = ( 0.25*(x*(1-x))*(y*(1-y)) - 0.25*(1+x)*(1-x)*(1+y)*(1-y))*(-0.5*z*(1-z));
  weights[1] = (-0.25*(x*(1+x))*(y*(1-y)) - 0.25*(1+x)*(1-x)*(1+y)*(1-y))*(-0.5*z*(1-z));
  weights[2] = ( 0.25*(x*(1+x))*(y*(1+y)) - 0.25*(1+x)*(1-x)*(1+y)*(1-y))*(-0.5*z*(1-z));
  weights[3] = (-0.25*(x*(1-x))*(y*(1+y)) - 0.25*(1+x)*(1-x)*(1+y)*(1-y))*(-0.5*z*(1-z));
  weights[4] = ( 0.25*(x*(1-x))*(y*(1-y)) - 0.25*(1+x)*(1-x)*(1+y)*(1-y))*( 0.5*z*(1+z));
  weights[5] = (-0.25*(x*(1+x))*(y*(1-y)) - 0.25*(1+x)*(1-x)*(1+y)*(1-y))*( 0.5*z*(1+z));
  weights[6] = ( 0.25*(x*(1+x))*(y*(1+y)) - 0.25*(1+x)*(1-x)*(1+y)*(1-y))*( 0.5*z*(1+z));
  weights[7] = (-0.25*(x*(1-x))*(y*(1+y)) - 0.25*(1+x)*(1-x)*(1+y)*(1-y))*( 0.5*z*(1+z));

  //The mid-edge nodes
  weights[8] =  0.5*((1+x)*(1-x))*(1-y) *(-0.5*z*(1-z));
  weights[9] =  0.5*((1+y)*(1-y))*(1+x) *(-0.5*z*(1-z));
  weights[10] = 0.5*((1+x)*(1-x))*(1+y) *(-0.5*z*(1-z));
  weights[11] = 0.5*((1+y)*(1-y))*(1-x) *(-0.5*z*(1-z));
  weights[12] = 0.5*((1+x)*(1-x))*(1-y) *( 0.5*z*(1+z));
  weights[13] = 0.5*((1+y)*(1-y))*(1+x) *( 0.5*z*(1+z));
  weights[14] = 0.5*((1+x)*(1-x))*(1+y) *( 0.5*z*(1+z));
  weights[15] = 0.5*((1+y)*(1-y))*(1-x) *( 0.5*z*(1+z));
  weights[16] =( 0.25*(x*(1-x))*(y*(1-y)) - 0.25*(1+x)*(1-x)*(1+y)*(1-y)) *((1+z)*(1-z));
  weights[17] =(-0.25*(x*(1+x))*(y*(1-y)) - 0.25*(1+x)*(1-x)*(1+y)*(1-y)) *((1+z)*(1-z));
  weights[18] =( 0.25*(x*(1+x))*(y*(1+y)) - 0.25*(1+x)*(1-x)*(1+y)*(1-y)) *((1+z)*(1-z));
  weights[19] =(-0.25*(x*(1-x))*(y*(1+y)) - 0.25*(1+x)*(1-x)*(1+y)*(1-y)) *((1+z)*(1-z));

  //Face center Nodes in xz and yz direction
  weights[20] = 0.5*((1+y)*(1-y))*(1-x)  *((1+z)*(1-z));
  weights[21] = 0.5*((1+y)*(1-y))*(1+x)  *((1+z)*(1-z));
  weights[22] = 0.5*((1+x)*(1-x))*(1-y)  *((1+z)*(1-z));
  weights[23] = 0.5*((1+x)*(1-x))*(1+y)  *((1+z)*(1-z));
  // clang-format on
}

//------------------------------------------------------------------------------
// Derivatives in parametric space.
void vtkBiQuadraticQuadraticHexahedron::InterpolationDerivs(
  const double pcoords[3], double derivs[72])
{
  // VTK needs parametric coordinates to be between (0,1). Isoparametric
  // shape functions are formulated between (-1,1). Here we do a
  // coordinate system conversion from (0,1) to (-1,1).
  double x = 2.0 * (pcoords[0] - 0.5);
  double y = 2.0 * (pcoords[1] - 0.5);
  double z = 2.0 * (pcoords[2] - 0.5);

  // x direction
  derivs[0] =
    -((y * y + (2 * x - 1) * y - 2 * x) * z * z + (-y * y + (1 - 2 * x) * y + 2 * x) * z) / 8;
  derivs[1] =
    ((y * y + (-2 * x - 1) * y + 2 * x) * z * z + (-y * y + (2 * x + 1) * y - 2 * x) * z) / 8;
  derivs[2] =
    ((y * y + (2 * x + 1) * y + 2 * x) * z * z + (-y * y + (-2 * x - 1) * y - 2 * x) * z) / 8;
  derivs[3] =
    -((y * y + (1 - 2 * x) * y - 2 * x) * z * z + (-y * y + (2 * x - 1) * y + 2 * x) * z) / 8;
  derivs[4] =
    -((y * y + (2 * x - 1) * y - 2 * x) * z * z + (y * y + (2 * x - 1) * y - 2 * x) * z) / 8;
  derivs[5] =
    ((y * y + (-2 * x - 1) * y + 2 * x) * z * z + (y * y + (-2 * x - 1) * y + 2 * x) * z) / 8;
  derivs[6] =
    ((y * y + (2 * x + 1) * y + 2 * x) * z * z + (y * y + (2 * x + 1) * y + 2 * x) * z) / 8;
  derivs[7] =
    -((y * y + (1 - 2 * x) * y - 2 * x) * z * z + (y * y + (1 - 2 * x) * y - 2 * x) * z) / 8;
  derivs[8] = ((x * y - x) * z * z + (x - x * y) * z) / 2;
  derivs[9] = -((y * y - 1) * z * z + (1 - y * y) * z) / 4;
  derivs[10] = -((x * y + x) * z * z + (-x * y - x) * z) / 2;
  derivs[11] = ((y * y - 1) * z * z + (1 - y * y) * z) / 4;
  derivs[12] = ((x * y - x) * z * z + (x * y - x) * z) / 2;
  derivs[13] = -((y * y - 1) * z * z + (y * y - 1) * z) / 4;
  derivs[14] = -((x * y + x) * z * z + (x * y + x) * z) / 2;
  derivs[15] = ((y * y - 1) * z * z + (y * y - 1) * z) / 4;
  derivs[16] = ((y * y + (2 * x - 1) * y - 2 * x) * z * z - y * y + (1 - 2 * x) * y + 2 * x) / 4;
  derivs[17] = -((y * y + (-2 * x - 1) * y + 2 * x) * z * z - y * y + (2 * x + 1) * y - 2 * x) / 4;
  derivs[18] = -((y * y + (2 * x + 1) * y + 2 * x) * z * z - y * y + (-2 * x - 1) * y - 2 * x) / 4;
  derivs[19] = ((y * y + (1 - 2 * x) * y - 2 * x) * z * z - y * y + (2 * x - 1) * y + 2 * x) / 4;
  derivs[20] = -((y * y - 1) * z * z - y * y + 1) / 2;
  derivs[21] = ((y * y - 1) * z * z - y * y + 1) / 2;
  derivs[22] = (x - x * y) * z * z + x * y - x;
  derivs[23] = (x * y + x) * z * z - x * y - x;
  // y direction
  derivs[24] = -(((2 * x - 2) * y + x * x - x) * z * z + ((2 - 2 * x) * y - x * x + x) * z) / 8;
  derivs[25] = (((2 * x + 2) * y - x * x - x) * z * z + ((-2 * x - 2) * y + x * x + x) * z) / 8;
  derivs[26] = (((2 * x + 2) * y + x * x + x) * z * z + ((-2 * x - 2) * y - x * x - x) * z) / 8;
  derivs[27] = -(((2 * x - 2) * y - x * x + x) * z * z + ((2 - 2 * x) * y + x * x - x) * z) / 8;
  derivs[28] = -(((2 * x - 2) * y + x * x - x) * z * z + ((2 * x - 2) * y + x * x - x) * z) / 8;
  derivs[29] = (((2 * x + 2) * y - x * x - x) * z * z + ((2 * x + 2) * y - x * x - x) * z) / 8;
  derivs[30] = (((2 * x + 2) * y + x * x + x) * z * z + ((2 * x + 2) * y + x * x + x) * z) / 8;
  derivs[31] = -(((2 * x - 2) * y - x * x + x) * z * z + ((2 * x - 2) * y - x * x + x) * z) / 8;
  derivs[32] = ((x * x - 1) * z * z + (1 - x * x) * z) / 4;
  derivs[33] = -((x + 1) * y * z * z + (-x - 1) * y * z) / 2;
  derivs[34] = -((x * x - 1) * z * z + (1 - x * x) * z) / 4;
  derivs[35] = ((x - 1) * y * z * z + (1 - x) * y * z) / 2;
  derivs[36] = ((x * x - 1) * z * z + (x * x - 1) * z) / 4;
  derivs[37] = -((x + 1) * y * z * z + (x + 1) * y * z) / 2;
  derivs[38] = -((x * x - 1) * z * z + (x * x - 1) * z) / 4;
  derivs[39] = ((x - 1) * y * z * z + (x - 1) * y * z) / 2;
  derivs[40] = (((2 * x - 2) * y + x * x - x) * z * z + (2 - 2 * x) * y - x * x + x) / 4;
  derivs[41] = -(((2 * x + 2) * y - x * x - x) * z * z + (-2 * x - 2) * y + x * x + x) / 4;
  derivs[42] = -(((2 * x + 2) * y + x * x + x) * z * z + (-2 * x - 2) * y - x * x - x) / 4;
  derivs[43] = (((2 * x - 2) * y - x * x + x) * z * z + (2 - 2 * x) * y + x * x - x) / 4;
  derivs[44] = (1 - x) * y * z * z + (x - 1) * y;
  derivs[45] = (x + 1) * y * z * z + (-x - 1) * y;
  derivs[46] = -((x * x - 1) * z * z - x * x + 1) / 2;
  derivs[47] = ((x * x - 1) * z * z - x * x + 1) / 2;
  // z direction
  derivs[48] = -(((2 * x - 2) * y * y + (2 * x * x - 2 * x) * y - 2 * x * x + 2) * z +
                 (1 - x) * y * y + (x - x * x) * y + x * x - 1) /
    8;
  derivs[49] = (((2 * x + 2) * y * y + (-2 * x * x - 2 * x) * y + 2 * x * x - 2) * z +
                 (-x - 1) * y * y + (x * x + x) * y - x * x + 1) /
    8;
  derivs[50] = (((2 * x + 2) * y * y + (2 * x * x + 2 * x) * y + 2 * x * x - 2) * z +
                 (-x - 1) * y * y + (-x * x - x) * y - x * x + 1) /
    8;
  derivs[51] = -(((2 * x - 2) * y * y + (2 * x - 2 * x * x) * y - 2 * x * x + 2) * z +
                 (1 - x) * y * y + (x * x - x) * y + x * x - 1) /
    8;
  derivs[52] = -(((2 * x - 2) * y * y + (2 * x * x - 2 * x) * y - 2 * x * x + 2) * z +
                 (x - 1) * y * y + (x * x - x) * y - x * x + 1) /
    8;
  derivs[53] = (((2 * x + 2) * y * y + (-2 * x * x - 2 * x) * y + 2 * x * x - 2) * z +
                 (x + 1) * y * y + (-x * x - x) * y + x * x - 1) /
    8;
  derivs[54] = (((2 * x + 2) * y * y + (2 * x * x + 2 * x) * y + 2 * x * x - 2) * z +
                 (x + 1) * y * y + (x * x + x) * y + x * x - 1) /
    8;
  derivs[55] = -(((2 * x - 2) * y * y + (2 * x - 2 * x * x) * y - 2 * x * x + 2) * z +
                 (x - 1) * y * y + (x - x * x) * y - x * x + 1) /
    8;
  derivs[56] = (((2 * x * x - 2) * y - 2 * x * x + 2) * z + (1 - x * x) * y + x * x - 1) / 4;
  derivs[57] = -(((2 * x + 2) * y * y - 2 * x - 2) * z + (-x - 1) * y * y + x + 1) / 4;
  derivs[58] = -(((2 * x * x - 2) * y + 2 * x * x - 2) * z + (1 - x * x) * y - x * x + 1) / 4;
  derivs[59] = (((2 * x - 2) * y * y - 2 * x + 2) * z + (1 - x) * y * y + x - 1) / 4;
  derivs[60] = (((2 * x * x - 2) * y - 2 * x * x + 2) * z + (x * x - 1) * y - x * x + 1) / 4;
  derivs[61] = -(((2 * x + 2) * y * y - 2 * x - 2) * z + (x + 1) * y * y - x - 1) / 4;
  derivs[62] = -(((2 * x * x - 2) * y + 2 * x * x - 2) * z + (x * x - 1) * y + x * x - 1) / 4;
  derivs[63] = (((2 * x - 2) * y * y - 2 * x + 2) * z + (x - 1) * y * y - x + 1) / 4;
  derivs[64] = ((x - 1) * y * y + (x * x - x) * y - x * x + 1) * z / 2;
  derivs[65] = -((x + 1) * y * y + (-x * x - x) * y + x * x - 1) * z / 2;
  derivs[66] = -((x + 1) * y * y + (x * x + x) * y + x * x - 1) * z / 2;
  derivs[67] = ((x - 1) * y * y + (x - x * x) * y - x * x + 1) * z / 2;
  derivs[68] = ((1 - x) * y * y + x - 1) * z;
  derivs[69] = ((x + 1) * y * y - x - 1) * z;
  derivs[70] = ((1 - x * x) * y + x * x - 1) * z;
  derivs[71] = ((x * x - 1) * y + x * x - 1) * z;

  // we compute derivatives in [-1; 1] but we need them in [ 0; 1]
  for (int i = 0; i < 72; i++)
  {
    derivs[i] *= 2;
  }
}

//------------------------------------------------------------------------------
double* vtkBiQuadraticQuadraticHexahedron::GetParametricCoords()
{
  return ParametricCoords;
}

//------------------------------------------------------------------------------
void vtkBiQuadraticQuadraticHexahedron::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Edge:\n";
  this->Edge->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Face:\n";
  this->Face->PrintSelf(os, indent.GetNextIndent());
  os << indent << "BiQuadFace:\n";
  this->BiQuadFace->PrintSelf(os, indent.GetNextIndent());
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
