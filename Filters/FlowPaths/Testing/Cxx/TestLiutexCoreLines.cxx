// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// A Lamb-Oseen vortex column parallel to z whose circulation grows with z. The
// Liutex magnitude peaks on the axis, so the core line must follow it. The axis
// avoids grid lines and face diagonals: a solution on an edge is found by every
// face sharing it, which splits the line. A circulation that is constant along z
// would confine grad(R) to the xy plane and the Liutex vector to z, leaving no
// isolated solution on any triangle.

#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkDoubleArray.h"
#include "vtkGradientFilter.h"
#include "vtkImageData.h"
#include "vtkLiutex.h"
#include "vtkLiutexCoreLines.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkParallelVectors.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
constexpr double CenterX = 0.1337;
constexpr double CenterY = -0.0421;

vtkSmartPointer<vtkImageData> MakeVortex(
  int numberOfXYPoints, int numberOfZPoints, double spacing, double scale)
{
  constexpr double coreRadius = 0.5;
  constexpr double axialVelocity = 0.5;

  auto image = vtkSmartPointer<vtkImageData>::New();
  image->SetDimensions(numberOfXYPoints, numberOfXYPoints, numberOfZPoints);
  image->SetOrigin(-1.5, -1.5, 0.);
  image->SetSpacing(spacing, spacing, spacing);

  vtkNew<vtkDoubleArray> velocity;
  velocity->SetName("velocity");
  velocity->SetNumberOfComponents(3);
  velocity->SetNumberOfTuples(image->GetNumberOfPoints());
  for (vtkIdType id = 0; id < image->GetNumberOfPoints(); ++id)
  {
    double p[3];
    image->GetPoint(id, p);
    const double dx = p[0] - CenterX;
    const double dy = p[1] - CenterY;
    const double r2 = dx * dx + dy * dy;
    // Angular velocity u_theta / r of a Lamb-Oseen vortex with circulation
    // 2 pi (1 + 0.6 z).
    const double strength = 1. + 0.6 * p[2];
    const double angularVelocity = strength *
      (r2 > 1e-12 ? (1. - std::exp(-r2 / (coreRadius * coreRadius))) / r2
                  : 1. / (coreRadius * coreRadius));
    velocity->SetTuple3(
      id, -scale * angularVelocity * dy, scale * angularVelocity * dx, scale * axialVelocity);
  }
  image->GetPointData()->AddArray(velocity);
  return image;
}

bool CheckCoreLine(int numberOfXYPoints, int numberOfZPoints, double spacing, double scale)
{
  vtkNew<vtkLiutexCoreLines> coreLines;
  coreLines->SetInputData(MakeVortex(numberOfXYPoints, numberOfZPoints, spacing, scale));
  coreLines->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "velocity");
  coreLines->Update();
  vtkPolyData* output = coreLines->GetOutput();

  std::ostringstream labelStream;
  labelStream << "Spacing " << spacing << ", velocity scale " << scale << ": ";
  const std::string label = labelStream.str();
  if (output->GetNumberOfLines() != 1)
  {
    std::cerr << label << "expected one core line, got " << output->GetNumberOfLines() << " with "
              << output->GetNumberOfPoints() << " points." << std::endl;
    return false;
  }
  if (!output->GetPointData()->GetArray("LiutexMagnitude") ||
    !output->GetPointData()->GetArray("Liutex") || !output->GetPointData()->GetArray("velocity") ||
    output->GetPointData()->GetNumberOfArrays() != 3)
  {
    std::cerr << label << "expected velocity, Liutex and LiutexMagnitude on the output, got "
              << output->GetPointData()->GetNumberOfArrays() << " arrays." << std::endl;
    return false;
  }

  const double zLength = spacing * (numberOfZPoints - 1);
  double zMin = VTK_DOUBLE_MAX;
  double zMax = VTK_DOUBLE_MIN;
  for (vtkIdType id = 0; id < output->GetNumberOfPoints(); ++id)
  {
    double p[3];
    output->GetPoint(id, p);
    const double distance = std::hypot(p[0] - CenterX, p[1] - CenterY);
    if (distance > 1e-3)
    {
      std::cerr << label << "point (" << p[0] << ", " << p[1] << ", " << p[2] << ") is " << distance
                << " from the vortex axis." << std::endl;
      return false;
    }
    zMin = std::min(zMin, p[2]);
    zMax = std::max(zMax, p[2]);
  }
  if (zMin > 1e-6 || zMax < zLength - 1e-6)
  {
    std::cerr << label << "core line spans z in [" << zMin << ", " << zMax << "], expected [0, "
              << zLength << "]." << std::endl;
    return false;
  }
  return true;
}
// The parallel vectors operator used to accept nearly singular triangles,
// where an absolute determinant test let roundoff through, and emitted points
// with NaN coordinates. Grad(R) and Liutex without the magnitude threshold
// include such triangles where R vanishes.
bool CheckFinitePoints()
{
  vtkNew<vtkLiutex> liutex;
  liutex->SetInputData(MakeVortex(31, 6, 0.1, 1.));
  liutex->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "velocity");
  vtkNew<vtkGradientFilter> gradient;
  gradient->SetInputConnection(liutex->GetOutputPort());
  gradient->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "LiutexMagnitude");
  gradient->SetResultArrayName("LiutexMagnitudeGradient");
  vtkNew<vtkParallelVectors> parallelVectors;
  parallelVectors->SetInputConnection(gradient->GetOutputPort());
  parallelVectors->SetFirstVectorFieldName("LiutexMagnitudeGradient");
  parallelVectors->SetSecondVectorFieldName("Liutex");
  parallelVectors->Update();

  vtkPolyData* output = parallelVectors->GetOutput();
  for (vtkIdType id = 0; id < output->GetNumberOfPoints(); ++id)
  {
    double p[3];
    output->GetPoint(id, p);
    if (!std::isfinite(p[0]) || !std::isfinite(p[1]) || !std::isfinite(p[2]))
    {
      std::cerr << "vtkParallelVectors produced a point with non-finite coordinates." << std::endl;
      return false;
    }
  }
  return true;
}
// Each block of a composite input gets its own core lines, in an output with
// the same structure.
bool CheckComposite()
{
  vtkNew<vtkMultiBlockDataSet> multiBlock;
  multiBlock->SetNumberOfBlocks(3);
  multiBlock->SetBlock(0, MakeVortex(31, 6, 0.1, 1.));
  multiBlock->SetBlock(2, MakeVortex(31, 6, 0.1, 2.));

  vtkNew<vtkPartitionedDataSetCollection> collection;
  collection->SetNumberOfPartitionedDataSets(1);
  vtkNew<vtkPartitionedDataSet> partitions;
  partitions->SetNumberOfPartitions(2);
  partitions->SetPartition(0, MakeVortex(31, 6, 0.1, 1.));
  partitions->SetPartition(1, MakeVortex(31, 6, 0.1, 3.));
  collection->SetPartitionedDataSet(0, partitions);

  bool ok = true;
  for (vtkDataObject* input :
    { static_cast<vtkDataObject*>(multiBlock), static_cast<vtkDataObject*>(collection) })
  {
    vtkNew<vtkLiutexCoreLines> coreLines;
    coreLines->SetInputData(input);
    coreLines->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "velocity");
    coreLines->Update();

    auto* output = vtkDataObjectTree::SafeDownCast(coreLines->GetOutputDataObject(0));
    if (!output || std::string(output->GetClassName()) != input->GetClassName())
    {
      std::cerr << "Composite " << input->GetClassName()
                << ": expected an output of the same type, got "
                << (output ? output->GetClassName() : "none") << std::endl;
      ok = false;
      continue;
    }
    int leaves = 0;
    for (vtkDataObject* leaf :
      vtk::Range(output,
        vtk::DataObjectTreeOptions::TraverseSubTree | vtk::DataObjectTreeOptions::SkipEmptyNodes |
          vtk::DataObjectTreeOptions::VisitOnlyLeaves))
    {
      auto* lines = vtkPolyData::SafeDownCast(leaf);
      if (!lines || lines->GetNumberOfLines() != 1)
      {
        std::cerr << "Composite " << input->GetClassName() << ": expected one core line per block"
                  << std::endl;
        ok = false;
      }
      ++leaves;
    }
    if (leaves != 2)
    {
      std::cerr << "Composite " << input->GetClassName() << ": expected 2 non-empty blocks, got "
                << leaves << std::endl;
      ok = false;
    }
  }
  return ok;
}
}

int TestLiutexCoreLines(int, char*[])
{
  bool ok = CheckCoreLine(31, 6, 0.1, 1.);
  ok = CheckCoreLine(151, 26, 0.02, 1.) && ok;
  // The result must not depend on the units of the velocity.
  ok = CheckCoreLine(31, 6, 0.1, 1e-9) && ok;
  ok = CheckFinitePoints() && ok;
  ok = CheckComposite() && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
