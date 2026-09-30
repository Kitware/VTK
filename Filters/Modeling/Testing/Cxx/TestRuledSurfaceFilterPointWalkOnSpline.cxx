// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAppendPolyData.h"
#include "vtkFeatureEdges.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkNew.h"
#include "vtkParametricFunctionSource.h"
#include "vtkParametricSpline.h"
#include "vtkPointSource.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkRegularPolygonSource.h"
#include "vtkRuledSurfaceFilter.h"
#include "vtkSmartPointer.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>

namespace
{
void ComputeTangents(vtkPoints* points, vtkPoints* tangents)
{
  const vtkIdType numberOfPoints = points->GetNumberOfPoints();
  tangents->SetNumberOfPoints(numberOfPoints);
  for (vtkIdType pointId = 0; pointId < numberOfPoints; ++pointId)
  {
    double previousPoint[3];
    double nextPoint[3];
    points->GetPoint(pointId > 0 ? pointId - 1 : pointId, previousPoint);
    points->GetPoint(pointId + 1 < numberOfPoints ? pointId + 1 : pointId, nextPoint);

    double tangent[3];
    vtkMath::Subtract(nextPoint, previousPoint, tangent);
    vtkMath::Normalize(tangent);
    tangents->SetPoint(pointId, tangent);
  }
}

void TransportFrame(const double previousTangent[3], const double tangent[3],
  const double previousNormal[3], double normal[3], double binormal[3])
{
  double rotationAxis[3];
  vtkMath::Cross(previousTangent, tangent, rotationAxis);
  const double sine = vtkMath::Normalize(rotationAxis);
  const double cosine = std::clamp(vtkMath::Dot(previousTangent, tangent), -1.0, 1.0);

  if (sine > 1e-12)
  {
    double axisCrossNormal[3];
    vtkMath::Cross(rotationAxis, previousNormal, axisCrossNormal);
    const double axisDotNormal = vtkMath::Dot(rotationAxis, previousNormal);
    for (int component = 0; component < 3; ++component)
    {
      normal[component] = previousNormal[component] * cosine + axisCrossNormal[component] * sine +
        rotationAxis[component] * axisDotNormal * (1.0 - cosine);
    }
  }
  else
  {
    std::copy(previousNormal, previousNormal + 3, normal);
  }

  const double normalProjection = vtkMath::Dot(normal, tangent);
  for (int component = 0; component < 3; ++component)
  {
    normal[component] -= normalProjection * tangent[component];
  }
  vtkMath::Normalize(normal);
  vtkMath::Cross(tangent, normal, binormal);
  vtkMath::Normalize(binormal);
}

vtkSmartPointer<vtkPoints> CreateSplinePoints()
{
  vtkNew<vtkMinimalStandardRandomSequence> randomSequence;
  randomSequence->SetSeed(7859821);

  vtkNew<vtkPointSource> pointSource;
  constexpr vtkIdType numberOfControlPoints = 5;
  pointSource->SetNumberOfPoints(numberOfControlPoints);
  pointSource->SetRandomSequence(randomSequence);
  pointSource->Update();

  vtkNew<vtkParametricSpline> spline;
  spline->SetPoints(pointSource->GetOutput()->GetPoints());

  vtkNew<vtkParametricFunctionSource> functionSource;
  functionSource->SetParametricFunction(spline);
  functionSource->SetUResolution(50 * numberOfControlPoints);
  functionSource->Update();

  return functionSource->GetOutput()->GetPoints();
}

// Sweep a regular polygon along the line, placing a profile with
// getNumberOfSides(pointId) sides at each line point, and generate the ruled
// surface between consecutive profiles in PointWalk mode.
vtkSmartPointer<vtkPolyData> SweepProfilesAlongLine(
  vtkPoints* linePoints, const std::function<int(vtkIdType)>& getNumberOfSides)
{
  vtkNew<vtkRegularPolygonSource> profile;
  profile->GeneratePolygonOff();
  profile->GeneratePolylineOn();

  vtkNew<vtkPoints> tangents;
  ComputeTangents(linePoints, tangents);

  vtkNew<vtkAppendPolyData> skeleton;
  double tangent[3];
  tangents->GetPoint(0, tangent);
  double normal[3];
  double binormal[3];
  vtkMath::Perpendiculars(tangent, normal, binormal, 0.0);

  for (vtkIdType pointId = 0; pointId < linePoints->GetNumberOfPoints(); ++pointId)
  {
    if (pointId > 0)
    {
      double previousTangent[3];
      double previousNormal[3] = { normal[0], normal[1], normal[2] };
      tangents->GetPoint(pointId - 1, previousTangent);
      tangents->GetPoint(pointId, tangent);
      TransportFrame(previousTangent, tangent, previousNormal, normal, binormal);
    }

    profile->SetNumberOfSides(getNumberOfSides(pointId));

    double position[3];
    linePoints->GetPoint(pointId, position);
    vtkNew<vtkMatrix4x4> matrix;
    matrix->Identity();
    for (int component = 0; component < 3; ++component)
    {
      matrix->SetElement(component, 0, 0.02 * normal[component]);
      matrix->SetElement(component, 1, 0.02 * binormal[component]);
      matrix->SetElement(component, 2, 0.02 * tangent[component]);
      matrix->SetElement(component, 3, position[component]);
    }

    vtkNew<vtkTransform> transform;
    transform->SetMatrix(matrix);
    vtkNew<vtkTransformFilter> transformProfile;
    transformProfile->SetTransform(transform);
    transformProfile->SetInputConnection(profile->GetOutputPort());
    transformProfile->Update();

    vtkNew<vtkPolyData> profileCopy;
    profileCopy->DeepCopy(transformProfile->GetOutput());
    skeleton->AddInputData(profileCopy);
  }

  vtkNew<vtkRuledSurfaceFilter> ruledSurface;
  ruledSurface->SetRuledModeToPointWalk();
  ruledSurface->CloseSurfaceOff();
  ruledSurface->SetOnRatio(1);
  ruledSurface->SetDistanceFactor(10000000);
  ruledSurface->SetOrientLoops(true);
  ruledSurface->SetInputConnection(skeleton->GetOutputPort());
  ruledSurface->Update();

  return ruledSurface->GetOutput();
}

bool CheckSurfaceEdges(vtkPolyData* surface, vtkIdType expectedNumberOfBoundaryEdges)
{
  vtkNew<vtkFeatureEdges> boundaryEdges;
  boundaryEdges->SetInputData(surface);
  boundaryEdges->BoundaryEdgesOn();
  boundaryEdges->FeatureEdgesOff();
  boundaryEdges->ManifoldEdgesOff();
  boundaryEdges->NonManifoldEdgesOff();
  boundaryEdges->ColoringOff();
  boundaryEdges->Update();

  vtkNew<vtkFeatureEdges> nonManifoldEdges;
  nonManifoldEdges->SetInputData(surface);
  nonManifoldEdges->BoundaryEdgesOff();
  nonManifoldEdges->FeatureEdgesOff();
  nonManifoldEdges->ManifoldEdgesOff();
  nonManifoldEdges->NonManifoldEdgesOn();
  nonManifoldEdges->ColoringOff();
  nonManifoldEdges->Update();

  const vtkIdType numberOfBoundaryEdges = boundaryEdges->GetOutput()->GetNumberOfCells();
  const vtkIdType numberOfNonManifoldEdges = nonManifoldEdges->GetOutput()->GetNumberOfCells();
  if (numberOfBoundaryEdges != expectedNumberOfBoundaryEdges || numberOfNonManifoldEdges != 0)
  {
    std::cerr << "Expected " << expectedNumberOfBoundaryEdges
              << " boundary edges and no non-manifold edges, but found boundary="
              << numberOfBoundaryEdges << ", nonManifold=" << numberOfNonManifoldEdges << "."
              << std::endl;
    return false;
  }

  return true;
}

bool TestConstantNumberOfSides()
{
  static constexpr int numberOfSides = 5;
  vtkSmartPointer<vtkPoints> linePoints = CreateSplinePoints();
  vtkSmartPointer<vtkPolyData> surface =
    SweepProfilesAlongLine(linePoints, [](vtkIdType) { return numberOfSides; });

  if (surface->GetNumberOfPolys() == 0)
  {
    std::cerr << "Expected PointWalk mode to generate polygons." << std::endl;
    return false;
  }

  // An uncapped extrusion has one boundary loop at each end.
  return CheckSurfaceEdges(surface, 2 * numberOfSides);
}

bool TestVaryingNumberOfSides()
{
  const auto getNumberOfSides = [](vtkIdType pointId)
  {
    constexpr int minimumNumberOfSides = 3;
    constexpr int numberOfSideCounts = 6;
    return minimumNumberOfSides + static_cast<int>(pointId % numberOfSideCounts);
  };

  vtkSmartPointer<vtkPoints> linePoints = CreateSplinePoints();
  vtkSmartPointer<vtkPolyData> surface = SweepProfilesAlongLine(linePoints, getNumberOfSides);

  const vtkIdType numberOfProfiles = linePoints->GetNumberOfPoints();
  vtkIdType expectedNumberOfPolys = 0;
  for (vtkIdType pointId = 1; pointId < numberOfProfiles; ++pointId)
  {
    expectedNumberOfPolys += getNumberOfSides(pointId - 1) + getNumberOfSides(pointId);
  }

  const vtkIdType numberOfPolys = surface->GetNumberOfPolys();
  if (numberOfPolys != expectedNumberOfPolys)
  {
    std::cerr << "Expected " << expectedNumberOfPolys << " polygons, but found " << numberOfPolys
              << "." << std::endl;
    return false;
  }

  return CheckSurfaceEdges(surface, getNumberOfSides(0) + getNumberOfSides(numberOfProfiles - 1));
}
} // namespace

int TestRuledSurfaceFilterPointWalkOnSpline(int, char*[])
{
  bool success = TestConstantNumberOfSides();
  success = TestVaryingNumberOfSides() && success;
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
