// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGridAxesHelper.h"

#include "vtkCamera.h"
#include "vtkCoordinate.h"
#include "vtkLogger.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkVectorOperators.h"

#include <algorithm>

namespace
{
inline vtkVector3d DoTransform(vtkMatrix4x4* matrix, const vtkVector3d& point)
{
  vtkTuple<double, 4> pointH;
  memcpy(pointH.GetData(), point.GetData(), sizeof(double) * 3);
  pointH[3] = 1.0;

  vtkTuple<double, 4> resultH;
  vtkMatrix4x4::MultiplyPoint(&matrix->Element[0][0], pointH.GetData(), resultH.GetData());

  vtkVector3d result(resultH.GetData());
  return result / vtkVector3d(resultH[3]);
}

const char* vtkFaceName(int face)
{
  switch (face)
  {
    case vtkGridAxesHelper::MIN_YZ:
      return "min-YZ";
    case vtkGridAxesHelper::MIN_ZX:
      return "min-ZX";
    case vtkGridAxesHelper::MIN_XY:
      return "min-XY";
    case vtkGridAxesHelper::MAX_YZ:
      return "max-YZ";
    case vtkGridAxesHelper::MAX_ZX:
      return "max-ZX";
    case vtkGridAxesHelper::MAX_XY:
      return "max-XY";
    default:
      return "invalid";
  }
}
}

vtkStandardNewMacro(vtkGridAxesHelper);
vtkCxxSetObjectMacro(vtkGridAxesHelper, Matrix, vtkMatrix4x4);
//----------------------------------------------------------------------------
vtkGridAxesHelper::vtkGridAxesHelper()
  : LabelVisibilityOverrides(true)
{
  this->GridBounds[0] = this->GridBounds[2] = this->GridBounds[4] = -1.0;
  this->GridBounds[1] = this->GridBounds[3] = this->GridBounds[5] = 1.0;
  this->Face = vtkGridAxesHelper::MIN_YZ;
  this->LabelMask = 0xff;
  this->Matrix = nullptr;

  this->GetPointsMTime = 0;
  this->GetTransformedPointsMTime = 0;
  this->Backface = false;
}

//----------------------------------------------------------------------------
vtkGridAxesHelper::~vtkGridAxesHelper()
{
  this->SetMatrix(nullptr);
}

//----------------------------------------------------------------------------
vtkTuple<vtkVector3d, 4> vtkGridAxesHelper::GetPoints()
{
  vtkMTimeType mtime = this->GetMTime();
  if (mtime == this->GetPointsMTime)
  {
    return this->Points;
  }
  this->GetPointsMTime = mtime;

  assert(vtkMath::AreBoundsInitialized(this->GridBounds));

  const double* bds = this->GridBounds;
  const unsigned int labelMask = this->LabelMask;

  // Setup the two axis that define the plane and the points that form the
  // plane. We set the planes so that the face normals are pointing outward from
  // the bounds.
  // The ActiveAxes help us determine which of the 2 3D-axes are being
  // represented by this 2D axis.
  const double minx = bds[0], miny = bds[2], minz = bds[4];
  const double maxx = bds[1], maxy = bds[3], maxz = bds[5];
  switch (this->Face)
  {
    case MIN_XY:
      this->Points[0] = vtkVector3d(minx, maxy, minz);
      this->Points[1] = vtkVector3d(maxx, maxy, minz);
      this->Points[2] = vtkVector3d(maxx, miny, minz);
      this->Points[3] = vtkVector3d(minx, miny, minz);
      this->ActiveAxes = vtkVector2i(0, 1);
      this->LabelVisibilities[0] = (labelMask & MAX_Y) != 0;
      this->LabelVisibilities[1] = (labelMask & MAX_X) != 0;
      this->LabelVisibilities[2] = (labelMask & MIN_Y) != 0;
      this->LabelVisibilities[3] = (labelMask & MIN_X) != 0;
      break;

    case MIN_YZ:
      this->Points[0] = vtkVector3d(minx, miny, maxz);
      this->Points[1] = vtkVector3d(minx, maxy, maxz);
      this->Points[2] = vtkVector3d(minx, maxy, minz);
      this->Points[3] = vtkVector3d(minx, miny, minz);
      this->ActiveAxes = vtkVector2i(1, 2);
      this->LabelVisibilities[0] = (labelMask & MAX_Z) != 0;
      this->LabelVisibilities[1] = (labelMask & MAX_Y) != 0;
      this->LabelVisibilities[2] = (labelMask & MIN_Z) != 0;
      this->LabelVisibilities[3] = (labelMask & MIN_Y) != 0;
      break;

    case MIN_ZX:
      this->Points[0] = vtkVector3d(maxx, miny, minz);
      this->Points[1] = vtkVector3d(maxx, miny, maxz);
      this->Points[2] = vtkVector3d(minx, miny, maxz);
      this->Points[3] = vtkVector3d(minx, miny, minz);
      this->ActiveAxes = vtkVector2i(2, 0);
      this->LabelVisibilities[0] = (labelMask & MAX_X) != 0;
      this->LabelVisibilities[1] = (labelMask & MAX_Z) != 0;
      this->LabelVisibilities[2] = (labelMask & MIN_X) != 0;
      this->LabelVisibilities[3] = (labelMask & MIN_Z) != 0;
      break;

    case MAX_XY:
      this->Points[0] = vtkVector3d(minx, miny, maxz);
      this->Points[1] = vtkVector3d(maxx, miny, maxz);
      this->Points[2] = vtkVector3d(maxx, maxy, maxz);
      this->Points[3] = vtkVector3d(minx, maxy, maxz);
      this->ActiveAxes = vtkVector2i(0, 1);
      this->LabelVisibilities[0] = (labelMask & MIN_Y) != 0;
      this->LabelVisibilities[1] = (labelMask & MAX_X) != 0;
      this->LabelVisibilities[2] = (labelMask & MAX_Y) != 0;
      this->LabelVisibilities[3] = (labelMask & MIN_X) != 0;
      break;

    case MAX_YZ:
      this->Points[0] = vtkVector3d(maxx, miny, minz);
      this->Points[1] = vtkVector3d(maxx, maxy, minz);
      this->Points[2] = vtkVector3d(maxx, maxy, maxz);
      this->Points[3] = vtkVector3d(maxx, miny, maxz);
      this->ActiveAxes = vtkVector2i(1, 2);
      this->LabelVisibilities[0] = (labelMask & MIN_Z) != 0;
      this->LabelVisibilities[1] = (labelMask & MAX_Y) != 0;
      this->LabelVisibilities[2] = (labelMask & MAX_Z) != 0;
      this->LabelVisibilities[3] = (labelMask & MIN_Y) != 0;
      break;

    case MAX_ZX:
      this->Points[0] = vtkVector3d(minx, maxy, minz);
      this->Points[1] = vtkVector3d(minx, maxy, maxz);
      this->Points[2] = vtkVector3d(maxx, maxy, maxz);
      this->Points[3] = vtkVector3d(maxx, maxy, minz);
      this->ActiveAxes = vtkVector2i(2, 0);
      this->LabelVisibilities[0] = (labelMask & MIN_X) != 0;
      this->LabelVisibilities[1] = (labelMask & MAX_Z) != 0;
      this->LabelVisibilities[2] = (labelMask & MAX_X) != 0;
      this->LabelVisibilities[3] = (labelMask & MIN_Z) != 0;
      break;
  }

  return this->Points;
}

//----------------------------------------------------------------------------
vtkVector2i vtkGridAxesHelper::GetActiveAxes()
{
  this->GetPoints();
  return this->ActiveAxes;
}
//----------------------------------------------------------------------------
vtkTuple<bool, 4> vtkGridAxesHelper::GetLabelVisibilities()
{
  this->GetPoints();

  for (int cc = 0; cc < 4; cc++)
  {
    this->ComputedLabelVisibilities[cc] =
      this->LabelVisibilities[cc] && this->LabelVisibilityOverrides[cc];
  }
  return this->ComputedLabelVisibilities;
}

//----------------------------------------------------------------------------
vtkTuple<vtkVector3d, 4> vtkGridAxesHelper::GetTransformedPoints()
{
  const vtkTuple<vtkVector3d, 4>& points = this->GetPoints();
  vtkMTimeType mtime =
    std::max(this->GetPointsMTime, (this->Matrix ? this->Matrix->GetMTime() : 0));

  if (mtime == this->GetTransformedPointsMTime)
  {
    return this->TransformedPoints;
  }
  this->GetTransformedPointsMTime = mtime;

  if (this->Matrix)
  {
    for (int cc = 0; cc < 4; cc++)
    {
      this->TransformedPoints[cc] = DoTransform(this->Matrix, points[cc]);
    }
  }
  else
  {
    this->TransformedPoints = points;
  }

  vtkVector3d v1 = this->TransformedPoints[1] - this->TransformedPoints[0];
  vtkVector3d v2 = this->TransformedPoints[2] - this->TransformedPoints[1];
  this->TransformedFaceNormal = v1.Cross(v2).Normalized();

  return this->TransformedPoints;
}

//----------------------------------------------------------------------------
vtkVector3d vtkGridAxesHelper::TransformPoint(const vtkVector3d& point)
{
  return this->Matrix ? DoTransform(this->Matrix, point) : point;
}

//----------------------------------------------------------------------------
vtkVector3d vtkGridAxesHelper::GetTransformedFaceNormal()
{
  // This ensures that the normal is recomputed, if needed.
  this->GetTransformedPoints();

  return this->TransformedFaceNormal;
}

//----------------------------------------------------------------------------
bool vtkGridAxesHelper::UpdateForViewport(vtkViewport* viewport)
{
  if (!viewport || !vtkMath::AreBoundsInitialized(this->GridBounds))
  {
    return false;
  }

  const vtkTuple<vtkVector3d, 4>& transformedPoints = this->GetTransformedPoints();

  // Compute the quadrilateral in viewport space that the transformed face
  // maps to.
  vtkNew<vtkCoordinate> coordinate;
  coordinate->SetCoordinateSystemToWorld();
  for (int cc = 0; cc < 4; cc++)
  {
    coordinate->SetValue(const_cast<double*>(transformedPoints[cc].GetData()));
    const int* viewportValue = coordinate->GetComputedViewportValue(viewport);
    this->ViewportPoints[cc] = vtkVector2i(viewportValue[0], viewportValue[1]);
    this->ViewportPointsAsDouble[cc] = vtkVector2d(viewportValue[0], viewportValue[1]);
  }

  // Compute axis vectors for each edge of the quadrilateral. Note that it's
  // not necessarily a parallelogram (due to perspective transformation).
  for (int cc = 0; cc < 4; cc++)
  {
    this->ViewportVectors[cc] =
      this->ViewportPointsAsDouble[(cc + 1) % 4] - this->ViewportPointsAsDouble[cc];
  }

  // Compute edge normals. In reality, we should compute a normal to the edge
  // in world coordinates and then project that to the viewport space. However,
  // due to perspective, the normal is doing to be different along the length
  // of the edge anyways. So we use this trick: use the normal using the prev
  // axis vector and the -'ve next axis vector and then use the average.
  for (int cc = 0; cc < 4; cc++)
  {
    int next = (cc + 1) % 4;
    int prev = (cc + 3) % 4;

    vtkVector2d normal =
      this->ViewportVectors[prev].Normalized() - this->ViewportVectors[next].Normalized();
    this->ViewportNormals[cc] = normal.Normalized();
  }

  // Determine if the face is facing backwards.
  // XXX: This is not robust. We should use vtkCoordinate instead.
  vtkCamera* camera = vtkRenderer::SafeDownCast(viewport)->GetActiveCamera();
  vtkVector3d viewdirection = camera->GetParallelProjection()
    ? vtkVector3d(camera->GetFocalPoint()) - vtkVector3d(camera->GetPosition())
    : this->TransformedPoints[0] - vtkVector3d(camera->GetPosition());
  viewdirection.Normalize();
  const auto dotproduct = (viewdirection.Dot(this->TransformedFaceNormal));
  // if face-normal is almost tangent to view direction, then the face is pretty much
  // not visible, let's not label it (see paraview/paraview#19182).
  const bool skip_face = (std::abs(dotproduct) <= 0.087); // i.e. within +/- 5 degrees of 90.
  vtkLogF(TRACE, "[%s] viewDir . faceNormal = %f, hidden=%d", vtkFaceName(this->Face), dotproduct,
    skip_face);
  this->Backface = (dotproduct >= 0);
  return !skip_face;
}

//----------------------------------------------------------------------------
void vtkGridAxesHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
