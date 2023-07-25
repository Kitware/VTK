// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkBoundingBox.h"
#include "vtkMath.h"
#include "vtkSmartPointer.h"

#include <limits>

#define TestBoundingBoxFailMacro(b, msg)                                                           \
  do                                                                                               \
  {                                                                                                \
    if (!(b))                                                                                      \
    {                                                                                              \
      std::cerr << msg << std::endl;                                                               \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (false)

int TestBoundingBox(int, char*[])
{
  {
    double n[3] = { -1, 0.5, 0 };
    double p[3] = { -1, -1, -1 };
    double bb[6] = { -1, 1, -1, 1, -1, 1 };
    vtkBoundingBox bbox(bb);
    bool res = bbox.IntersectPlane(p, n);
    bbox.GetBounds(bb);
    TestBoundingBoxFailMacro(res && bb[0] == -1 && bb[1] == 0, "Intersect Plane Failed!");
  }
  {
    double n[3] = { 0, 0, 1 };
    double p[3] = { 0, 0, 0 };
    double bb[6] = { -1, 1, -1, 1, -1, 1 };
    vtkBoundingBox bbox(bb);
    bool res = bbox.IntersectPlane(p, n);
    bbox.GetBounds(bb);
    TestBoundingBoxFailMacro(res && bb[4] == 0 && bb[5] == 1, "Intersect Plane Failed!");
  }
  {
    double n[3] = { 0, 0, -1 };
    double p[3] = { 0, 0, 0 };
    double bb[6] = { -1, 1, -1, 1, -1, 1 };
    vtkBoundingBox bbox(bb);
    bool res = bbox.IntersectPlane(p, n);
    bbox.GetBounds(bb);
    TestBoundingBoxFailMacro(res && bb[4] == -1 && bb[5] == 0, "Intersect Plane Failed!");
  }
  {
    double n[3] = { 0, -1, 0 };
    double p[3] = { 0, 0, 0 };
    double bb[6] = { -1, 1, -1, 1, -1, 1 };
    vtkBoundingBox bbox(bb);
    bool res = bbox.IntersectPlane(p, n);
    bbox.GetBounds(bb);
    TestBoundingBoxFailMacro(res && bb[2] == -1 && bb[3] == 0, "Intersect Plane Failed!");
  }

  {
    double n[3] = { 1, 1, 1 };
    double p[3] = { 0, 0, 0 };
    double bb[6] = { -1, 1, -1, 1, -1, 1 };
    vtkBoundingBox bbox(bb);
    bool res = bbox.IntersectPlane(p, n);
    bbox.GetBounds(bb);
    TestBoundingBoxFailMacro(
      !res && bb[0] == -1 && bb[1] == 1 && bb[2] == -1 && bb[3] == 1 && bb[4] == -1 && bb[5] == 1,
      "Intersect Plane Failed!");
  }
  {
    double bb[6];
    vtkBoundingBox invalidBBox;
    invalidBBox.GetBounds(bb);
    vtkBoundingBox bbox(bb);
    TestBoundingBoxFailMacro(!bbox.IsValid(), "Bounding box from invalid bounds Failed!");
  }
  return EXIT_SUCCESS;
}
