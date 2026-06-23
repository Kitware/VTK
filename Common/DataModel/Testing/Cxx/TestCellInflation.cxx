// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkBiQuadraticQuadraticHexahedron.h"
#include "vtkBiQuadraticQuadraticWedge.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkPixel.h"
#include "vtkPoints.h"
#include "vtkQuadraticHexahedron.h"
#include "vtkQuadraticLinearWedge.h"
#include "vtkQuadraticPyramid.h"
#include "vtkQuadraticTetra.h"
#include "vtkQuadraticWedge.h"
#include "vtkTetra.h"
#include "vtkTriQuadraticHexahedron.h"
#include "vtkTriQuadraticPyramid.h"
#include "vtkTriangle.h"
#include "vtkVoxel.h"

#include <array>
#include <iostream>
#include <vector>

namespace
{

bool CheckInflation(vtkCell* cell, const std::vector<std::array<double, 3>>& expectedPts,
  double tol = VTK_DBL_EPSILON)
{
  // NearlyEqual is purely relative and fails when b==0 and a is a sub-epsilon
  // residual (e.g. 1/sqrt(3)*sqrt(3)/2 rounds to 0.5±1ulp on some platforms,
  // leaving a tiny non-zero contribution for nodes whose expected coordinate is 0).
  // FuzzyCompare adds the absolute fallback that handles that case.
  auto isClose = [](double a, double b, double tolerance) -> bool
  {
    return vtkMathUtilities::NearlyEqual<double>(a, b, tolerance) ||
      vtkMathUtilities::FuzzyCompare<double>(a, b, tolerance);
  };

  bool ok = true;
  double p[3];
  for (int i = 0; i < static_cast<int>(expectedPts.size()); ++i)
  {
    cell->Points->GetPoint(i, p);
    const auto& e = expectedPts[i];
    if (!isClose(p[0], e[0], tol) || !isClose(p[1], e[1], tol) || !isClose(p[2], e[2], tol))
    {
      std::cerr << "Inflating " << cell->GetClassName() << " failed at p" << i << "\n";
      ok = false;
    }
  }
  return ok;
}

} // namespace

int TestCellInflation(int, char*[])
{
  {
    const double s2 = std::sqrt(2.0);
    vtkNew<vtkTriangle> triangle;
    triangle->Points->SetPoint(0, 0.0, 0.0, 0.0);
    triangle->Points->SetPoint(1, 0.0, 1.0, 0.0);
    triangle->Points->SetPoint(2, 1.0, 0.0, 0.0);
    triangle->Inflate(0.5);
    // clang-format off
    if (!CheckInflation(triangle, {
      { -0.5,              -0.5,              0.0 },
      { -0.5,               1.5 + 1.0 / s2,  0.0 },
      {  1.5 + 1.0 / s2,  -0.5,              0.0 },
    }, 2.0 * VTK_DBL_EPSILON))
    // clang-format on
    {
      return EXIT_FAILURE;
    }
  }

  {
    const double s3 = std::sqrt(3.0);
    vtkNew<vtkTetra> tetra;
    tetra->Points->SetPoint(0, 0.0, 0.0, 0.0);
    tetra->Points->SetPoint(1, 0.0, 1.0, 0.0);
    tetra->Points->SetPoint(2, 1.0, 0.0, 0.0);
    tetra->Points->SetPoint(3, 0.0, 0.0, 1.0);
    tetra->Inflate(0.5);
    // clang-format off
    if (!CheckInflation(tetra, {
      { -0.5,               -0.5,               -0.5              },
      { -0.5,                2.0 + 0.5 * s3,    -0.5              },
      {  2.0 + 0.5 * s3,   -0.5,               -0.5              },
      { -0.5,               -0.5,                2.0 + 0.5 * s3  },
    }, 2.0 * VTK_DBL_EPSILON))
    // clang-format on
    {
      return EXIT_FAILURE;
    }
  }

  {
    vtkNew<vtkPixel> pixel;
    pixel->Points->SetPoint(0, 0.0, 0.0, 0.0);
    pixel->Points->SetPoint(1, 1.0, 0.0, 0.0);
    pixel->Points->SetPoint(2, 0.0, 1.0, 0.0);
    pixel->Points->SetPoint(3, 1.0, 1.0, 0.0);
    pixel->Inflate(0.5);
    // clang-format off
    if (!CheckInflation(pixel, {
      { -0.5,  -0.5,  0.0 },
      {  1.5,  -0.5,  0.0 },
      { -0.5,   1.5,  0.0 },
      {  1.5,   1.5,  0.0 },
    }))
    // clang-format on
    {
      return EXIT_FAILURE;
    }
  }

  {
    vtkNew<vtkVoxel> voxel;
    voxel->Points->SetPoint(0, 0.0, 0.0, 0.0);
    voxel->Points->SetPoint(1, 1.0, 0.0, 0.0);
    voxel->Points->SetPoint(2, 0.0, 1.0, 0.0);
    voxel->Points->SetPoint(3, 1.0, 1.0, 0.0);
    voxel->Points->SetPoint(4, 0.0, 0.0, 1.0);
    voxel->Points->SetPoint(5, 1.0, 0.0, 1.0);
    voxel->Points->SetPoint(6, 0.0, 1.0, 1.0);
    voxel->Points->SetPoint(7, 1.0, 1.0, 1.0);
    voxel->Inflate(0.5);
    // clang-format off
    if (!CheckInflation(voxel, {
      { -0.5,  -0.5,  -0.5 },
      {  1.5,  -0.5,  -0.5 },
      { -0.5,   1.5,  -0.5 },
      {  1.5,   1.5,  -0.5 },
      { -0.5,  -0.5,   1.5 },
      {  1.5,  -0.5,   1.5 },
      { -0.5,   1.5,   1.5 },
      {  1.5,   1.5,   1.5 },
    }))
    // clang-format on
    {
      return EXIT_FAILURE;
    }
  }

  {
    vtkNew<vtkLine> line;
    line->Points->SetPoint(0, 0.0, 0.0, 0.0);
    line->Points->SetPoint(1, 1.0, 0.0, 0.0);
    line->Inflate(0.5);
    // clang-format off
    if (!CheckInflation(line, {
      { -0.5,  0.0,  0.0 },
      {  1.5,  0.0,  0.0 },
    }))
    // clang-format on
    {
      return EXIT_FAILURE;
    }
  }

  // vtkQuadraticTetra: exact test using the reference element (world = parametric).
  //
  // With J = I the Inflate algorithm reduces to: for each control point, sum
  // area-weighted unit outward normals of its incident faces in parametric space
  // (unit_normal × face_area), normalize the sum, then move by dist.
  //
  // Parametric face unit normals and areas:
  //   face 0 {0,1,3,...}: n=(0,-1,0),        area=0.5    [y=0 plane]
  //   face 1 {1,2,3,...}: n=(1,1,1)/√3,      area=√3/2   [x+y+z=1]
  //   face 2 {2,0,3,...}: n=(-1,0,0),        area=0.5    [x=0 plane]
  //   face 3 {0,2,1,...}: n=(0,0,-1),        area=0.5    [z=0 plane]
  //
  // Area-weighted normals (unit_n × area): faces 0,2,3 contribute with weight
  // 0.5; face 1 contributes (0.5,0.5,0.5). Since nSum is normalized before
  // displacement, any common scaling factor cancels — the result equals summing
  // Newell normals directly.
  //
  // PointToIncidentFaces and resulting displacement directions:
  //   p0 → {0,2,3}: nSum∝(-1,-1,-1) → dir=(-1,-1,-1)/√3
  //   p1 → {0,1,3}: nSum∝( 1, 0, 0) → dir=(1,0,0)
  //   p2 → {1,2,3}: nSum∝( 0, 1, 0) → dir=(0,1,0)
  //   p3 → {0,1,2}: nSum∝( 0, 0, 1) → dir=(0,0,1)
  //   p4 → {0,3}:   nSum∝( 0,-1,-1) → dir=(0,-1,-1)/√2
  //   p5 → {1,3}:   nSum∝( 1, 1, 0) → dir=(1,1,0)/√2
  //   p6 → {2,3}:   nSum∝(-1, 0,-1) → dir=(-1,0,-1)/√2
  //   p7 → {0,2}:   nSum∝(-1,-1, 0) → dir=(-1,-1,0)/√2
  //   p8 → {0,1}:   nSum∝( 1, 0, 1) → dir=(1,0,1)/√2
  //   p9 → {1,2}:   nSum∝( 0, 1, 1) → dir=(0,1,1)/√2
  {
    const double dist = 0.5;
    const double s2 = std::sqrt(2.0);
    const double s3 = std::sqrt(3.0);

    vtkNew<vtkQuadraticTetra> qTetra;
    qTetra->Points->SetPoint(0, 0.0, 0.0, 0.0);
    qTetra->Points->SetPoint(1, 1.0, 0.0, 0.0);
    qTetra->Points->SetPoint(2, 0.0, 1.0, 0.0);
    qTetra->Points->SetPoint(3, 0.0, 0.0, 1.0);
    qTetra->Points->SetPoint(4, 0.5, 0.0, 0.0);
    qTetra->Points->SetPoint(5, 0.5, 0.5, 0.0);
    qTetra->Points->SetPoint(6, 0.0, 0.5, 0.0);
    qTetra->Points->SetPoint(7, 0.0, 0.0, 0.5);
    qTetra->Points->SetPoint(8, 0.5, 0.0, 0.5);
    qTetra->Points->SetPoint(9, 0.0, 0.5, 0.5);

    qTetra->Inflate(dist);

    // clang-format off
    if (!CheckInflation(qTetra, {
      { -dist / s3,        -dist / s3,        -dist / s3        }, // p0: dir=(-1,-1,-1)/√3
      {  1.0 + dist,        0.0,               0.0              }, // p1: dir=(1,0,0)
      {  0.0,               1.0 + dist,        0.0              }, // p2: dir=(0,1,0)
      {  0.0,               0.0,               1.0 + dist       }, // p3: dir=(0,0,1)
      {  0.5,              -dist / s2,         -dist / s2       }, // p4 (mid 0-1): dir=(0,-1,-1)/√2
      {  0.5 + dist / s2,   0.5 + dist / s2,   0.0             }, // p5 (mid 1-2): dir=(1,1,0)/√2
      { -dist / s2,         0.5,              -dist / s2        }, // p6 (mid 2-0): dir=(-1,0,-1)/√2
      { -dist / s2,        -dist / s2,         0.5             }, // p7 (mid 0-3): dir=(-1,-1,0)/√2
      {  0.5 + dist / s2,   0.0,               0.5 + dist / s2 }, // p8 (mid 1-3): dir=(1,0,1)/√2
      {  0.0,               0.5 + dist / s2,   0.5 + dist / s2 }, // p9 (mid 2-3): dir=(0,1,1)/√2
    }))
    // clang-format on
    {
      return EXIT_FAILURE;
    }
  }

  // vtkQuadraticHexahedron: exact test using the reference element (world = parametric).
  //
  // All 6 faces are axis-aligned unit squares with equal area, so the area
  // factors cancel in normalization. The displacement directions reduce to:
  //   corners    (3 incident faces): (±1,±1,±1)/√3
  //   edge mids  (2 incident faces): two non-zero components ±1/√2, one zero
  //
  // Face unit normals: face0=(-1,0,0) face1=(+1,0,0) face2=(0,-1,0)
  //                    face3=(0,+1,0) face4=(0,0,-1) face5=(0,0,+1)
  {
    const double dist = 0.5;
    const double s2 = std::sqrt(2.0);
    const double s3 = std::sqrt(3.0);

    vtkNew<vtkQuadraticHexahedron> qHex;
    qHex->Points->SetPoint(0, 0.0, 0.0, 0.0);
    qHex->Points->SetPoint(1, 1.0, 0.0, 0.0);
    qHex->Points->SetPoint(2, 1.0, 1.0, 0.0);
    qHex->Points->SetPoint(3, 0.0, 1.0, 0.0);
    qHex->Points->SetPoint(4, 0.0, 0.0, 1.0);
    qHex->Points->SetPoint(5, 1.0, 0.0, 1.0);
    qHex->Points->SetPoint(6, 1.0, 1.0, 1.0);
    qHex->Points->SetPoint(7, 0.0, 1.0, 1.0);
    qHex->Points->SetPoint(8, 0.5, 0.0, 0.0);
    qHex->Points->SetPoint(9, 1.0, 0.5, 0.0);
    qHex->Points->SetPoint(10, 0.5, 1.0, 0.0);
    qHex->Points->SetPoint(11, 0.0, 0.5, 0.0);
    qHex->Points->SetPoint(12, 0.5, 0.0, 1.0);
    qHex->Points->SetPoint(13, 1.0, 0.5, 1.0);
    qHex->Points->SetPoint(14, 0.5, 1.0, 1.0);
    qHex->Points->SetPoint(15, 0.0, 0.5, 1.0);
    qHex->Points->SetPoint(16, 0.0, 0.0, 0.5);
    qHex->Points->SetPoint(17, 1.0, 0.0, 0.5);
    qHex->Points->SetPoint(18, 1.0, 1.0, 0.5);
    qHex->Points->SetPoint(19, 0.0, 1.0, 0.5);

    qHex->Inflate(dist);

    // clang-format off
    // Corners: dir=(±1,±1,±1)/√3; edge mids: two ±1/√2 components and one zero.
    if (!CheckInflation(qHex, {
      { -dist / s3,        -dist / s3,        -dist / s3        }, // p0 (0,0,0) → {0,2,4}: dir=(-1,-1,-1)/√3
      {  1.0 + dist / s3,  -dist / s3,        -dist / s3        }, // p1 (1,0,0) → {1,2,4}: dir=(+1,-1,-1)/√3
      {  1.0 + dist / s3,   1.0 + dist / s3,  -dist / s3        }, // p2 (1,1,0) → {1,3,4}: dir=(+1,+1,-1)/√3
      { -dist / s3,         1.0 + dist / s3,  -dist / s3        }, // p3 (0,1,0) → {0,3,4}: dir=(-1,+1,-1)/√3
      { -dist / s3,        -dist / s3,         1.0 + dist / s3  }, // p4 (0,0,1) → {0,2,5}: dir=(-1,-1,+1)/√3
      {  1.0 + dist / s3,  -dist / s3,         1.0 + dist / s3  }, // p5 (1,0,1) → {1,2,5}: dir=(+1,-1,+1)/√3
      {  1.0 + dist / s3,   1.0 + dist / s3,   1.0 + dist / s3  }, // p6 (1,1,1) → {1,3,5}: dir=(+1,+1,+1)/√3
      { -dist / s3,         1.0 + dist / s3,   1.0 + dist / s3  }, // p7 (0,1,1) → {0,3,5}: dir=(-1,+1,+1)/√3
      {  0.5,              -dist / s2,         -dist / s2        }, // p8  (0.5,0,0)   → {2,4}: dir=(0,-1,-1)/√2
      {  1.0 + dist / s2,   0.5,              -dist / s2         }, // p9  (1,0.5,0)   → {1,4}: dir=(+1,0,-1)/√2
      {  0.5,               1.0 + dist / s2,  -dist / s2         }, // p10 (0.5,1,0)   → {3,4}: dir=(0,+1,-1)/√2
      { -dist / s2,         0.5,              -dist / s2         }, // p11 (0,0.5,0)   → {0,4}: dir=(-1,0,-1)/√2
      {  0.5,              -dist / s2,         1.0 + dist / s2   }, // p12 (0.5,0,1)   → {2,5}: dir=(0,-1,+1)/√2
      {  1.0 + dist / s2,   0.5,               1.0 + dist / s2   }, // p13 (1,0.5,1)   → {1,5}: dir=(+1,0,+1)/√2
      {  0.5,               1.0 + dist / s2,   1.0 + dist / s2   }, // p14 (0.5,1,1)   → {3,5}: dir=(0,+1,+1)/√2
      { -dist / s2,         0.5,               1.0 + dist / s2   }, // p15 (0,0.5,1)   → {0,5}: dir=(-1,0,+1)/√2
      { -dist / s2,        -dist / s2,          0.5              }, // p16 (0,0,0.5)   → {0,2}: dir=(-1,-1,0)/√2
      {  1.0 + dist / s2,  -dist / s2,          0.5              }, // p17 (1,0,0.5)   → {1,2}: dir=(+1,-1,0)/√2
      {  1.0 + dist / s2,   1.0 + dist / s2,    0.5              }, // p18 (1,1,0.5)   → {1,3}: dir=(+1,+1,0)/√2
      { -dist / s2,         1.0 + dist / s2,    0.5              }, // p19 (0,1,0.5)   → {0,3}: dir=(-1,+1,0)/√2
    }))
    // clang-format on
    {
      return EXIT_FAILURE;
    }
  }

  // vtkQuadraticWedge: exact test using the reference element (world = parametric).
  //
  // Face unit normals and Newell areas (world = parametric so J = I):
  //   face 0 {0,2,1,...} (z=0 triangle): n=(0,0,-1),       area=0.5  → contrib=(0,0,-4/8)
  //   face 1 {3,4,5,...} (z=1 triangle): n=(0,0,+1),       area=0.5  → contrib=(0,0,+4/8)
  //   face 2 {0,1,4,3,...} (y=0 rect):   n=(0,-1,0),       area=11/8 → contrib=(0,-11/8,0)
  //   face 3 {1,2,5,4,...} (diag rect):  n=(1,1,0)/√2, area=11√2/8  → contrib=(11/8,11/8,0)
  //   face 4 {2,0,3,5,...} (x=0 rect):   n=(-1,0,0),       area=11/8 → contrib=(-11/8,0,0)
  //
  // nSum is scaled by 1/8. Two magnitudes arise:
  //   |nSum·8|=√258 when two rectangular faces from perpendicular axes contribute
  //   |nSum·8|=√137 when one rectangular face contributes alongside a triangle
  {
    const double dist = 0.5;
    const double s2 = std::sqrt(2.0);
    const double s137 = std::sqrt(137.0);
    const double s258 = std::sqrt(258.0);

    vtkNew<vtkQuadraticWedge> qWedge;
    qWedge->Points->SetPoint(0, 0.0, 0.0, 0.0);
    qWedge->Points->SetPoint(1, 1.0, 0.0, 0.0);
    qWedge->Points->SetPoint(2, 0.0, 1.0, 0.0);
    qWedge->Points->SetPoint(3, 0.0, 0.0, 1.0);
    qWedge->Points->SetPoint(4, 1.0, 0.0, 1.0);
    qWedge->Points->SetPoint(5, 0.0, 1.0, 1.0);
    qWedge->Points->SetPoint(6, 0.5, 0.0, 0.0);
    qWedge->Points->SetPoint(7, 0.5, 0.5, 0.0);
    qWedge->Points->SetPoint(8, 0.0, 0.5, 0.0);
    qWedge->Points->SetPoint(9, 0.5, 0.0, 1.0);
    qWedge->Points->SetPoint(10, 0.5, 0.5, 1.0);
    qWedge->Points->SetPoint(11, 0.0, 0.5, 1.0);
    qWedge->Points->SetPoint(12, 0.0, 0.0, 0.5);
    qWedge->Points->SetPoint(13, 1.0, 0.0, 0.5);
    qWedge->Points->SetPoint(14, 0.0, 1.0, 0.5);

    qWedge->Inflate(dist);

    // clang-format off
    // Corners: nSum=(±11,±11,±4)/8 → |·|=√258/8; or (±11,0,±4)/8 → |·|=√137/8.
    // Lateral mids: two rect faces whose y/x contributions cancel or align cleanly.
    if (!CheckInflation(qWedge, {
      { -dist * 11.0 / s258,         -dist * 11.0 / s258,          -dist * 4.0 / s258        }, // p0 (0,0,0)     → {0,2,4}: (-11,-11,-4)/√258
      {  1.0 + dist * 11.0 / s137,    0.0,                          -dist * 4.0 / s137        }, // p1 (1,0,0)     → {0,2,3}: (11,0,-4)/√137
      {  0.0,                          1.0 + dist * 11.0 / s137,    -dist * 4.0 / s137        }, // p2 (0,1,0)     → {0,3,4}: (0,11,-4)/√137
      { -dist * 11.0 / s258,         -dist * 11.0 / s258,           1.0 + dist * 4.0 / s258   }, // p3 (0,0,1)     → {1,2,4}: (-11,-11,+4)/√258
      {  1.0 + dist * 11.0 / s137,    0.0,                           1.0 + dist * 4.0 / s137   }, // p4 (1,0,1)     → {1,2,3}: (11,0,+4)/√137
      {  0.0,                          1.0 + dist * 11.0 / s137,     1.0 + dist * 4.0 / s137   }, // p5 (0,1,1)     → {1,3,4}: (0,11,+4)/√137
      {  0.5,                         -dist * 11.0 / s137,           -dist * 4.0 / s137        }, // p6 (0.5,0,0)   → {0,2}: (0,-11,-4)/√137
      {  0.5 + dist * 11.0 / s258,    0.5 + dist * 11.0 / s258,     -dist * 4.0 / s258        }, // p7 (0.5,0.5,0) → {0,3}: (11,11,-4)/√258
      { -dist * 11.0 / s137,           0.5,                          -dist * 4.0 / s137        }, // p8 (0,0.5,0)   → {0,4}: (-11,0,-4)/√137
      {  0.5,                         -dist * 11.0 / s137,            1.0 + dist * 4.0 / s137  }, // p9 (0.5,0,1)   → {1,2}: (0,-11,+4)/√137
      {  0.5 + dist * 11.0 / s258,    0.5 + dist * 11.0 / s258,      1.0 + dist * 4.0 / s258  }, // p10 (0.5,0.5,1)→ {1,3}: (11,11,+4)/√258
      { -dist * 11.0 / s137,           0.5,                           1.0 + dist * 4.0 / s137  }, // p11 (0,0.5,1)  → {1,4}: (-11,0,+4)/√137
      { -dist / s2,                   -dist / s2,                     0.5                       }, // p12 (0,0,0.5)  → {2,4}: dir=(-1,-1,0)/√2
      {  1.0 + dist,                   0.0,                           0.5                       }, // p13 (1,0,0.5)  → {2,3}: dir=(1,0,0)
      {  0.0,                          1.0 + dist,                    0.5                       }, // p14 (0,1,0.5)  → {3,4}: dir=(0,1,0)
    }))
    // clang-format on
    {
      return EXIT_FAILURE;
    }
  }
  // vtkQuadraticPyramid: exact test using the reference element (world = parametric).
  //
  // The pyramid mid-edge nodes toward the apex are NOT at geometric midpoints
  // in parametric space, so Newell normals differ from first-triple normals.
  // Area-weighted contribution of each face (unit_n × Newell_area), scaled by 8:
  //   face 0 (base square, 8-pt): (0, 0,-11)
  //   face 1 (y=0 triangle,  6-pt): (0,-5,  0)
  //   face 2 (diag triangle, 6-pt): (5, 0,  2)
  //   face 3 (diag triangle, 6-pt): (0, 5,  2)
  //   face 4 (x=0 triangle,  6-pt): (-5,0,  0)
  //
  // p4 (apex) has a singular Jacobian and stays at its original position.
  {
    const double dist = 0.5;
    const double s2 = std::sqrt(2.0);
    const double s54 = std::sqrt(54.0);
    const double s66 = std::sqrt(66.0);
    const double s99 = std::sqrt(99.0);
    const double s106 = std::sqrt(106.0);
    const double s131 = std::sqrt(131.0);
    const double s146 = std::sqrt(146.0);
    const double s171 = std::sqrt(171.0);

    vtkNew<vtkQuadraticPyramid> qPyr;
    qPyr->Points->SetPoint(0, 0.0, 0.0, 0.0);
    qPyr->Points->SetPoint(1, 1.0, 0.0, 0.0);
    qPyr->Points->SetPoint(2, 1.0, 1.0, 0.0);
    qPyr->Points->SetPoint(3, 0.0, 1.0, 0.0);
    qPyr->Points->SetPoint(4, 0.0, 0.0, 1.0);
    qPyr->Points->SetPoint(5, 0.5, 0.0, 0.0);
    qPyr->Points->SetPoint(6, 1.0, 0.5, 0.0);
    qPyr->Points->SetPoint(7, 0.5, 1.0, 0.0);
    qPyr->Points->SetPoint(8, 0.0, 0.5, 0.0);
    qPyr->Points->SetPoint(9, 0.0, 0.0, 0.5);
    qPyr->Points->SetPoint(10, 1.0, 0.0, 0.5);
    qPyr->Points->SetPoint(11, 1.0, 1.0, 0.5);
    qPyr->Points->SetPoint(12, 0.0, 1.0, 0.5);

    qPyr->Inflate(dist);

    // clang-format off
    // Base corners: nSum×8 = face0 + two side faces. Lateral mids: nSum×8 = two side faces.
    if (!CheckInflation(qPyr, {
      { -dist * 5.0 / s171,        -dist * 5.0 / s171,         -dist * 11.0 / s171       }, // p0 (0,0,0)   → {0,1,4}: (-5,-5,-11)/√171
      {  1.0 + dist * 5.0 / s131,  -dist * 5.0 / s131,         -dist * 9.0 / s131         }, // p1 (1,0,0)   → {0,1,2}: (5,-5,-9)/√131
      {  1.0 + dist * 5.0 / s99,    1.0 + dist * 5.0 / s99,    -dist * 7.0 / s99          }, // p2 (1,1,0)   → {0,2,3}: (5,5,-7)/√99
      { -dist * 5.0 / s131,         1.0 + dist * 5.0 / s131,   -dist * 9.0 / s131         }, // p3 (0,1,0)   → {0,3,4}: (-5,5,-9)/√131
      {  0.0,                        0.0,                         1.0                       }, // p4 (0,0,1)   apex: Jacobian singular, stays put
      {  0.5,                       -dist * 5.0 / s146,          -dist * 11.0 / s146       }, // p5 (0.5,0,0) → {0,1}: (0,-5,-11)/√146
      {  1.0 + dist * 5.0 / s106,   0.5,                         -dist * 9.0 / s106        }, // p6 (1,0.5,0) → {0,2}: (5,0,-9)/√106
      {  0.5,                        1.0 + dist * 5.0 / s106,    -dist * 9.0 / s106        }, // p7 (0.5,1,0) → {0,3}: (0,5,-9)/√106
      { -dist * 5.0 / s146,          0.5,                         -dist * 11.0 / s146      }, // p8 (0,0.5,0) → {0,4}: (-5,0,-11)/√146
      { -dist / s2,                 -dist / s2,                    0.5                      }, // p9 (0,0,0.5) → {1,4}: dir=(-1,-1,0)/√2
      {  1.0 + dist * 5.0 / s54,   -dist * 5.0 / s54,             0.5 + dist * 2.0 / s54  }, // p10 (1,0,0.5)→ {1,2}: (5,-5,2)/√54
      {  1.0 + dist * 5.0 / s66,    1.0 + dist * 5.0 / s66,       0.5 + dist * 4.0 / s66  }, // p11 (1,1,0.5)→ {2,3}: (5,5,4)/√66
      { -dist * 5.0 / s54,           1.0 + dist * 5.0 / s54,      0.5 + dist * 2.0 / s54  }, // p12 (0,1,0.5)→ {3,4}: (-5,5,2)/√54
    }))
    // clang-format on
    {
      return EXIT_FAILURE;
    }
  } // vtkQuadraticLinearWedge: exact test using the reference element (world = parametric).
    //
    // The quad-linear rectangular sides have only 4 corners + 2 mid-edge nodes on the
    // triangular edges (no lateral mid-edges), so Newell gives the correct area for all faces.
    // Area-weighted contributions × 2 (integer or simple irrational values):
    //   face 0 (z=0 triangle, area 0.5):  (0, 0,-1)
    //   face 1 (z=1 triangle, area 0.5):  (0, 0,+1)
    //   face 2 (y=0 rect,     area 1):    (0,-2, 0)
    //   face 3 (diag rect,    area √2):   (2, 2, 0)  [√2 in area and 1/√2 in normal cancel]
    //   face 4 (x=0 rect,     area 1):    (-2,0, 0)
    //
    // Two |nSum×2| magnitudes arise: 3 (corners/mid-nodes with 3 faces) and √5.
  {
    const double dist = 0.5;
    const double s5 = std::sqrt(5.0);

    vtkNew<vtkQuadraticLinearWedge> qLWedge;
    qLWedge->Points->SetPoint(0, 0.0, 0.0, 0.0);
    qLWedge->Points->SetPoint(1, 1.0, 0.0, 0.0);
    qLWedge->Points->SetPoint(2, 0.0, 1.0, 0.0);
    qLWedge->Points->SetPoint(3, 0.0, 0.0, 1.0);
    qLWedge->Points->SetPoint(4, 1.0, 0.0, 1.0);
    qLWedge->Points->SetPoint(5, 0.0, 1.0, 1.0);
    qLWedge->Points->SetPoint(6, 0.5, 0.0, 0.0);
    qLWedge->Points->SetPoint(7, 0.5, 0.5, 0.0);
    qLWedge->Points->SetPoint(8, 0.0, 0.5, 0.0);
    qLWedge->Points->SetPoint(9, 0.5, 0.0, 1.0);
    qLWedge->Points->SetPoint(10, 0.5, 0.5, 1.0);
    qLWedge->Points->SetPoint(11, 0.0, 0.5, 1.0);

    qLWedge->Inflate(dist);

    // clang-format off
    // Corners: nSum×2 with |·|=3 → dir=(±2,±2,±1)/3; or |·|=√5 → dir=(±2,0,±1)/√5 etc.
    if (!CheckInflation(qLWedge, {
      { -2.0 * dist / 3.0,        -2.0 * dist / 3.0,        -dist / 3.0        }, // p0 (0,0,0)     → {0,2,4}: (-2,-2,-1)/3
      {  1.0 + 2.0 * dist / s5,    0.0,                      -dist / s5         }, // p1 (1,0,0)     → {0,2,3}: (2,0,-1)/√5
      {  0.0,                       1.0 + 2.0 * dist / s5,   -dist / s5         }, // p2 (0,1,0)     → {0,3,4}: (0,2,-1)/√5
      { -2.0 * dist / 3.0,        -2.0 * dist / 3.0,         1.0 + dist / 3.0  }, // p3 (0,0,1)     → {1,2,4}: (-2,-2,+1)/3
      {  1.0 + 2.0 * dist / s5,    0.0,                       1.0 + dist / s5   }, // p4 (1,0,1)     → {1,2,3}: (2,0,+1)/√5
      {  0.0,                       1.0 + 2.0 * dist / s5,    1.0 + dist / s5   }, // p5 (0,1,1)     → {1,3,4}: (0,2,+1)/√5
      {  0.5,                      -2.0 * dist / s5,          -dist / s5        }, // p6 (0.5,0,0)   → {0,2}: (0,-2,-1)/√5
      {  0.5 + 2.0 * dist / 3.0,   0.5 + 2.0 * dist / 3.0,  -dist / 3.0       }, // p7 (0.5,0.5,0) → {0,3}: (2,2,-1)/3
      { -2.0 * dist / s5,           0.5,                      -dist / s5        }, // p8 (0,0.5,0)   → {0,4}: (-2,0,-1)/√5
      {  0.5,                      -2.0 * dist / s5,           1.0 + dist / s5  }, // p9 (0.5,0,1)   → {1,2}: (0,-2,+1)/√5
      {  0.5 + 2.0 * dist / 3.0,   0.5 + 2.0 * dist / 3.0,   1.0 + dist / 3.0 }, // p10 (0.5,0.5,1)→ {1,3}: (2,2,+1)/3
      { -2.0 * dist / s5,           0.5,                       1.0 + dist / s5  }, // p11 (0,0.5,1)  → {1,4}: (-2,0,+1)/√5
    }))
    // clang-format on
    {
      return EXIT_FAILURE;
    }
  }
  // vtkBiQuadraticQuadraticHexahedron: exact test using the reference element.
  //
  // Faces 0-3 (x=0,x=1,y=0,y=1) each have a face-center node (9-pt BiQuadQuad):
  //   Newell area = 5/4  → area-weighted contribution × 8 = ±10 in x or y
  // Faces 4-5 (z=0,z=1) have no face-center node (8-pt QuadQuad):
  //   Newell area = 11/8 → area-weighted contribution × 8 = ±11 in z
  //
  // This asymmetry gives two |nSum×8| magnitudes:
  //   √321 for corners (10²+10²+11²)  and  √221 for bottom/top edge-mids (10²+11²)
  //   √200=10√2 for lateral edge-mids (10²+10²) → dir = (±1,±1,0)/√2
  {
    const double dist = 0.5;
    const double s2 = std::sqrt(2.0);
    const double s221 = std::sqrt(221.0);
    const double s321 = std::sqrt(321.0);

    vtkNew<vtkBiQuadraticQuadraticHexahedron> bqHex;
    bqHex->Points->SetPoint(0, 0.0, 0.0, 0.0);
    bqHex->Points->SetPoint(1, 1.0, 0.0, 0.0);
    bqHex->Points->SetPoint(2, 1.0, 1.0, 0.0);
    bqHex->Points->SetPoint(3, 0.0, 1.0, 0.0);
    bqHex->Points->SetPoint(4, 0.0, 0.0, 1.0);
    bqHex->Points->SetPoint(5, 1.0, 0.0, 1.0);
    bqHex->Points->SetPoint(6, 1.0, 1.0, 1.0);
    bqHex->Points->SetPoint(7, 0.0, 1.0, 1.0);
    bqHex->Points->SetPoint(8, 0.5, 0.0, 0.0);
    bqHex->Points->SetPoint(9, 1.0, 0.5, 0.0);
    bqHex->Points->SetPoint(10, 0.5, 1.0, 0.0);
    bqHex->Points->SetPoint(11, 0.0, 0.5, 0.0);
    bqHex->Points->SetPoint(12, 0.5, 0.0, 1.0);
    bqHex->Points->SetPoint(13, 1.0, 0.5, 1.0);
    bqHex->Points->SetPoint(14, 0.5, 1.0, 1.0);
    bqHex->Points->SetPoint(15, 0.0, 0.5, 1.0);
    bqHex->Points->SetPoint(16, 0.0, 0.0, 0.5);
    bqHex->Points->SetPoint(17, 1.0, 0.0, 0.5);
    bqHex->Points->SetPoint(18, 1.0, 1.0, 0.5);
    bqHex->Points->SetPoint(19, 0.0, 1.0, 0.5);
    bqHex->Points->SetPoint(20, 0.0, 0.5, 0.5);
    bqHex->Points->SetPoint(21, 1.0, 0.5, 0.5);
    bqHex->Points->SetPoint(22, 0.5, 0.0, 0.5);
    bqHex->Points->SetPoint(23, 0.5, 1.0, 0.5);

    bqHex->Inflate(dist);

    // clang-format off
    // Corners: dir=(±10,±10,±11)/√321. Bottom/top edge-mids: (±10,±11)/√221.
    // Lateral edge-mids (equal area 9-pt faces): dir=(±1,±1,0)/√2.
    // Face-mid nodes: single face, dir = face unit normal.
    if (!CheckInflation(bqHex, {
      { -dist * 10.0 / s321,        -dist * 10.0 / s321,        -dist * 11.0 / s321       }, // p0 (0,0,0) → {0,2,4}: (-10,-10,-11)/√321
      {  1.0 + dist * 10.0 / s321,  -dist * 10.0 / s321,        -dist * 11.0 / s321       }, // p1 (1,0,0) → {1,2,4}: (10,-10,-11)/√321
      {  1.0 + dist * 10.0 / s321,   1.0 + dist * 10.0 / s321,  -dist * 11.0 / s321       }, // p2 (1,1,0) → {1,3,4}: (10,10,-11)/√321
      { -dist * 10.0 / s321,         1.0 + dist * 10.0 / s321,  -dist * 11.0 / s321       }, // p3 (0,1,0) → {0,3,4}: (-10,10,-11)/√321
      { -dist * 10.0 / s321,        -dist * 10.0 / s321,         1.0 + dist * 11.0 / s321  }, // p4 (0,0,1) → {0,2,5}: (-10,-10,+11)/√321
      {  1.0 + dist * 10.0 / s321,  -dist * 10.0 / s321,         1.0 + dist * 11.0 / s321  }, // p5 (1,0,1) → {1,2,5}: (10,-10,+11)/√321
      {  1.0 + dist * 10.0 / s321,   1.0 + dist * 10.0 / s321,   1.0 + dist * 11.0 / s321  }, // p6 (1,1,1) → {1,3,5}: (10,10,+11)/√321
      { -dist * 10.0 / s321,         1.0 + dist * 10.0 / s321,   1.0 + dist * 11.0 / s321  }, // p7 (0,1,1) → {0,3,5}: (-10,10,+11)/√321
      {  0.5,                        -dist * 10.0 / s221,         -dist * 11.0 / s221       }, // p8  (0.5,0,0)   → {2,4}: (0,-10,-11)/√221
      {  1.0 + dist * 10.0 / s221,   0.5,                         -dist * 11.0 / s221       }, // p9  (1,0.5,0)   → {1,4}: (10,0,-11)/√221
      {  0.5,                         1.0 + dist * 10.0 / s221,   -dist * 11.0 / s221       }, // p10 (0.5,1,0)   → {3,4}: (0,10,-11)/√221
      { -dist * 10.0 / s221,          0.5,                         -dist * 11.0 / s221       }, // p11 (0,0.5,0)   → {0,4}: (-10,0,-11)/√221
      {  0.5,                        -dist * 10.0 / s221,          1.0 + dist * 11.0 / s221  }, // p12 (0.5,0,1)   → {2,5}: (0,-10,+11)/√221
      {  1.0 + dist * 10.0 / s221,   0.5,                          1.0 + dist * 11.0 / s221  }, // p13 (1,0.5,1)   → {1,5}: (10,0,+11)/√221
      {  0.5,                         1.0 + dist * 10.0 / s221,    1.0 + dist * 11.0 / s221  }, // p14 (0.5,1,1)   → {3,5}: (0,10,+11)/√221
      { -dist * 10.0 / s221,          0.5,                          1.0 + dist * 11.0 / s221  }, // p15 (0,0.5,1)   → {0,5}: (-10,0,+11)/√221
      { -dist / s2,                  -dist / s2,                    0.5                       }, // p16 (0,0,0.5)   → {0,2}: dir=(-1,-1,0)/√2
      {  1.0 + dist / s2,            -dist / s2,                    0.5                       }, // p17 (1,0,0.5)   → {1,2}: dir=(+1,-1,0)/√2
      {  1.0 + dist / s2,             1.0 + dist / s2,              0.5                       }, // p18 (1,1,0.5)   → {1,3}: dir=(+1,+1,0)/√2
      { -dist / s2,                   1.0 + dist / s2,              0.5                       }, // p19 (0,1,0.5)   → {0,3}: dir=(-1,+1,0)/√2
      { -dist,                        0.5,                           0.5                       }, // p20 (0,0.5,0.5) → face 0 (x=0): dir=(-1,0,0)
      {  1.0 + dist,                  0.5,                           0.5                       }, // p21 (1,0.5,0.5) → face 1 (x=1): dir=(+1,0,0)
      {  0.5,                        -dist,                          0.5                       }, // p22 (0.5,0,0.5) → face 2 (y=0): dir=(0,-1,0)
      {  0.5,                         1.0 + dist,                    0.5                       }, // p23 (0.5,1,0.5) → face 3 (y=1): dir=(0,+1,0)
    }))
    // clang-format on
    {
      return EXIT_FAILURE;
    }
  }

  // vtkBiQuadraticQuadraticWedge: exact test using the reference element.
  //
  // Triangle faces 0,1 have Newell area 0.5 (correct). BiQuad rectangular faces
  // 2,3,4 each have a face-center node (9-pt). Area-weighted contributions × 4:
  //   face 0 (z=0 triangle, area 0.5):    (0, 0,-2)
  //   face 1 (z=1 triangle, area 0.5):    (0, 0,+2)
  //   face 2 (y=0 BiQuad,   area 5/4):    (0,-5, 0)
  //   face 3 (diag BiQuad,  area 5√2/4):  (5, 5, 0)  [√2 in area and 1/√2 cancel]
  //   face 4 (x=0 BiQuad,   area 5/4):    (-5,0, 0)
  //
  // Two |nSum×4| magnitudes: √54 and √29.
  {
    const double dist = 0.5;
    const double s2 = std::sqrt(2.0);
    const double s29 = std::sqrt(29.0);
    const double s54 = std::sqrt(54.0);

    vtkNew<vtkBiQuadraticQuadraticWedge> bqWedge;
    bqWedge->Points->SetPoint(0, 0.0, 0.0, 0.0);
    bqWedge->Points->SetPoint(1, 1.0, 0.0, 0.0);
    bqWedge->Points->SetPoint(2, 0.0, 1.0, 0.0);
    bqWedge->Points->SetPoint(3, 0.0, 0.0, 1.0);
    bqWedge->Points->SetPoint(4, 1.0, 0.0, 1.0);
    bqWedge->Points->SetPoint(5, 0.0, 1.0, 1.0);
    bqWedge->Points->SetPoint(6, 0.5, 0.0, 0.0);
    bqWedge->Points->SetPoint(7, 0.5, 0.5, 0.0);
    bqWedge->Points->SetPoint(8, 0.0, 0.5, 0.0);
    bqWedge->Points->SetPoint(9, 0.5, 0.0, 1.0);
    bqWedge->Points->SetPoint(10, 0.5, 0.5, 1.0);
    bqWedge->Points->SetPoint(11, 0.0, 0.5, 1.0);
    bqWedge->Points->SetPoint(12, 0.0, 0.0, 0.5);
    bqWedge->Points->SetPoint(13, 1.0, 0.0, 0.5);
    bqWedge->Points->SetPoint(14, 0.0, 1.0, 0.5);
    bqWedge->Points->SetPoint(15, 0.5, 0.0, 0.5);
    bqWedge->Points->SetPoint(16, 0.5, 0.5, 0.5);
    bqWedge->Points->SetPoint(17, 0.0, 0.5, 0.5);

    bqWedge->Inflate(dist);

    // clang-format off
    // Corners: nSum×4=(±5,±5,±2) → |·|=√54, or (±5,0,±2) → |·|=√29.
    // Lateral mids (two BiQuad faces): contributions cancel/align. Face mids: single face normal.
    if (!CheckInflation(bqWedge, {
      { -dist * 5.0 / s54,        -dist * 5.0 / s54,        -dist * 2.0 / s54       }, // p0 (0,0,0)      → {0,2,4}: (-5,-5,-2)/√54
      {  1.0 + dist * 5.0 / s29,   0.0,                      -dist * 2.0 / s29       }, // p1 (1,0,0)      → {0,2,3}: (5,0,-2)/√29
      {  0.0,                       1.0 + dist * 5.0 / s29,  -dist * 2.0 / s29       }, // p2 (0,1,0)      → {0,3,4}: (0,5,-2)/√29
      { -dist * 5.0 / s54,        -dist * 5.0 / s54,          1.0 + dist * 2.0 / s54  }, // p3 (0,0,1)      → {1,2,4}: (-5,-5,+2)/√54
      {  1.0 + dist * 5.0 / s29,   0.0,                       1.0 + dist * 2.0 / s29  }, // p4 (1,0,1)      → {1,2,3}: (5,0,+2)/√29
      {  0.0,                       1.0 + dist * 5.0 / s29,   1.0 + dist * 2.0 / s29  }, // p5 (0,1,1)      → {1,3,4}: (0,5,+2)/√29
      {  0.5,                      -dist * 5.0 / s29,          -dist * 2.0 / s29       }, // p6 (0.5,0,0)    → {0,2}: (0,-5,-2)/√29
      {  0.5 + dist * 5.0 / s54,   0.5 + dist * 5.0 / s54,   -dist * 2.0 / s54       }, // p7 (0.5,0.5,0)  → {0,3}: (5,5,-2)/√54
      { -dist * 5.0 / s29,          0.5,                       -dist * 2.0 / s29       }, // p8 (0,0.5,0)    → {0,4}: (-5,0,-2)/√29
      {  0.5,                      -dist * 5.0 / s29,           1.0 + dist * 2.0 / s29  }, // p9 (0.5,0,1)    → {1,2}: (0,-5,+2)/√29
      {  0.5 + dist * 5.0 / s54,   0.5 + dist * 5.0 / s54,    1.0 + dist * 2.0 / s54  }, // p10 (0.5,0.5,1) → {1,3}: (5,5,+2)/√54
      { -dist * 5.0 / s29,          0.5,                        1.0 + dist * 2.0 / s29  }, // p11 (0,0.5,1)   → {1,4}: (-5,0,+2)/√29
      { -dist / s2,                -dist / s2,                   0.5                    }, // p12 (0,0,0.5)   → {2,4}: dir=(-1,-1,0)/√2
      {  1.0 + dist,                0.0,                         0.5                    }, // p13 (1,0,0.5)   → {2,3}: dir=(1,0,0)
      {  0.0,                       1.0 + dist,                  0.5                    }, // p14 (0,1,0.5)   → {3,4}: dir=(0,1,0)
      {  0.5,                      -dist,                         0.5                    }, // p15 (0.5,0,0.5) → face 2 (y=0): dir=(0,-1,0)
      {  0.5 + dist / s2,           0.5 + dist / s2,             0.5                    }, // p16 (0.5,0.5,0.5)→ face 3 (diagonal): dir=(1,1,0)/√2
      { -dist,                       0.5,                         0.5                    }, // p17 (0,0.5,0.5) → face 4 (x=0): dir=(-1,0,0)
    }))
    // clang-format on
    {
      return EXIT_FAILURE;
    }
  }
  // vtkTriQuadraticHexahedron: exact test using the reference element.
  //
  // All 6 faces have the same 9-point structure (4 corners + 4 edge-mids + 1 face-center),
  // so they all get the same Newell area (5/4). Equal areas across all faces means
  // normalization corrects the direction: pts 0-19 move identically to vtkQuadraticHexahedron.
  // Face-mid pts 20-25 each touch one face and move by dist in that face's unit-normal direction.
  // Pt 26 (center) stays put.
  {
    const double dist = 0.5;
    const double s2 = std::sqrt(2.0);
    const double s3 = std::sqrt(3.0);

    vtkNew<vtkTriQuadraticHexahedron> tqHex;
    tqHex->Points->SetPoint(0, 0.0, 0.0, 0.0);
    tqHex->Points->SetPoint(1, 1.0, 0.0, 0.0);
    tqHex->Points->SetPoint(2, 1.0, 1.0, 0.0);
    tqHex->Points->SetPoint(3, 0.0, 1.0, 0.0);
    tqHex->Points->SetPoint(4, 0.0, 0.0, 1.0);
    tqHex->Points->SetPoint(5, 1.0, 0.0, 1.0);
    tqHex->Points->SetPoint(6, 1.0, 1.0, 1.0);
    tqHex->Points->SetPoint(7, 0.0, 1.0, 1.0);
    tqHex->Points->SetPoint(8, 0.5, 0.0, 0.0);
    tqHex->Points->SetPoint(9, 1.0, 0.5, 0.0);
    tqHex->Points->SetPoint(10, 0.5, 1.0, 0.0);
    tqHex->Points->SetPoint(11, 0.0, 0.5, 0.0);
    tqHex->Points->SetPoint(12, 0.5, 0.0, 1.0);
    tqHex->Points->SetPoint(13, 1.0, 0.5, 1.0);
    tqHex->Points->SetPoint(14, 0.5, 1.0, 1.0);
    tqHex->Points->SetPoint(15, 0.0, 0.5, 1.0);
    tqHex->Points->SetPoint(16, 0.0, 0.0, 0.5);
    tqHex->Points->SetPoint(17, 1.0, 0.0, 0.5);
    tqHex->Points->SetPoint(18, 1.0, 1.0, 0.5);
    tqHex->Points->SetPoint(19, 0.0, 1.0, 0.5);
    tqHex->Points->SetPoint(20, 0.0, 0.5, 0.5);
    tqHex->Points->SetPoint(21, 1.0, 0.5, 0.5);
    tqHex->Points->SetPoint(22, 0.5, 0.0, 0.5);
    tqHex->Points->SetPoint(23, 0.5, 1.0, 0.5);
    tqHex->Points->SetPoint(24, 0.5, 0.5, 0.0);
    tqHex->Points->SetPoint(25, 0.5, 0.5, 1.0);
    tqHex->Points->SetPoint(26, 0.5, 0.5, 0.5);

    tqHex->Inflate(dist);

    // clang-format off
    // Pts 0-19: identical to vtkQuadraticHexahedron (all face areas equal, so they cancel).
    // Face-mid nodes (p20-p25): each on one face, dir = face unit normal. p26: center, stays put.
    if (!CheckInflation(tqHex, {
      { -dist / s3,        -dist / s3,        -dist / s3        }, // p0 (0,0,0) → dir=(-1,-1,-1)/√3
      {  1.0 + dist / s3,  -dist / s3,        -dist / s3        }, // p1 (1,0,0) → dir=(+1,-1,-1)/√3
      {  1.0 + dist / s3,   1.0 + dist / s3,  -dist / s3        }, // p2 (1,1,0) → dir=(+1,+1,-1)/√3
      { -dist / s3,         1.0 + dist / s3,  -dist / s3        }, // p3 (0,1,0) → dir=(-1,+1,-1)/√3
      { -dist / s3,        -dist / s3,         1.0 + dist / s3  }, // p4 (0,0,1) → dir=(-1,-1,+1)/√3
      {  1.0 + dist / s3,  -dist / s3,         1.0 + dist / s3  }, // p5 (1,0,1) → dir=(+1,-1,+1)/√3
      {  1.0 + dist / s3,   1.0 + dist / s3,   1.0 + dist / s3  }, // p6 (1,1,1) → dir=(+1,+1,+1)/√3
      { -dist / s3,         1.0 + dist / s3,   1.0 + dist / s3  }, // p7 (0,1,1) → dir=(-1,+1,+1)/√3
      {  0.5,              -dist / s2,         -dist / s2        }, // p8  (0.5,0,0)   → dir=(0,-1,-1)/√2
      {  1.0 + dist / s2,   0.5,              -dist / s2         }, // p9  (1,0.5,0)   → dir=(+1,0,-1)/√2
      {  0.5,               1.0 + dist / s2,  -dist / s2         }, // p10 (0.5,1,0)   → dir=(0,+1,-1)/√2
      { -dist / s2,         0.5,              -dist / s2         }, // p11 (0,0.5,0)   → dir=(-1,0,-1)/√2
      {  0.5,              -dist / s2,         1.0 + dist / s2   }, // p12 (0.5,0,1)   → dir=(0,-1,+1)/√2
      {  1.0 + dist / s2,   0.5,               1.0 + dist / s2   }, // p13 (1,0.5,1)   → dir=(+1,0,+1)/√2
      {  0.5,               1.0 + dist / s2,   1.0 + dist / s2   }, // p14 (0.5,1,1)   → dir=(0,+1,+1)/√2
      { -dist / s2,         0.5,               1.0 + dist / s2   }, // p15 (0,0.5,1)   → dir=(-1,0,+1)/√2
      { -dist / s2,        -dist / s2,          0.5              }, // p16 (0,0,0.5)   → dir=(-1,-1,0)/√2
      {  1.0 + dist / s2,  -dist / s2,          0.5              }, // p17 (1,0,0.5)   → dir=(+1,-1,0)/√2
      {  1.0 + dist / s2,   1.0 + dist / s2,    0.5              }, // p18 (1,1,0.5)   → dir=(+1,+1,0)/√2
      { -dist / s2,         1.0 + dist / s2,    0.5              }, // p19 (0,1,0.5)   → dir=(-1,+1,0)/√2
      { -dist,              0.5,                 0.5              }, // p20 (0,0.5,0.5) → face x=0: dir=(-1,0,0)
      {  1.0 + dist,        0.5,                 0.5              }, // p21 (1,0.5,0.5) → face x=1: dir=(+1,0,0)
      {  0.5,              -dist,                 0.5              }, // p22 (0.5,0,0.5) → face y=0: dir=(0,-1,0)
      {  0.5,               1.0 + dist,           0.5              }, // p23 (0.5,1,0.5) → face y=1: dir=(0,+1,0)
      {  0.5,               0.5,                 -dist             }, // p24 (0.5,0.5,0) → face z=0: dir=(0,0,-1)
      {  0.5,               0.5,                  1.0 + dist        }, // p25 (0.5,0.5,1) → face z=1: dir=(0,0,+1)
      {  0.5,               0.5,                  0.5              }, // p26 (0.5,0.5,0.5): center, stays put
    }))
    // clang-format on
    {
      return EXIT_FAILURE;
    }
  }

  // vtkTriQuadraticPyramid: exact test using the reference element (world = parametric).
  //
  // The base is a unit square at z=0.5; apex is at (0.5,0.5,1.0). Side face unit normals
  // (from Newell): face1=(0,-1,+1)/√2, face2=(+1,0,+1)/√2, face3=(0,+1,+1)/√2,
  // face4=(-1,0,+1)/√2. Their Newell areas are all 5√2/24 (confirmed analytically).
  // Base (9-pt) Newell area=5/4. Contributions × 24 give integer vectors of the form
  // 5×(±1,±1,±4), 5×(0,±1,±5), 5×(±1,±1,∓2), giving magnitudes √18, √26, √6.
  // p18 (center) stays put. Face-mid nodes move by dist along their face unit normals.
  //
  // Note: a tolerance of 1e-10 is used here because the 19-node element's complex
  // shape functions and Jacobian inversion accumulate O(10*epsilon) relative error.
  // All values are analytically correct; only floating-point roundoff differs.
  {
    const double dist = 0.5;
    const double s2 = std::sqrt(2.0);
    const double s6 = std::sqrt(6.0);
    const double s18 = std::sqrt(18.0);
    const double s26 = std::sqrt(26.0);
    const double tol = 1e-10;

    vtkNew<vtkTriQuadraticPyramid> tqPyr;
    tqPyr->Points->SetPoint(0, 0.0, 0.0, 0.5);
    tqPyr->Points->SetPoint(1, 1.0, 0.0, 0.5);
    tqPyr->Points->SetPoint(2, 1.0, 1.0, 0.5);
    tqPyr->Points->SetPoint(3, 0.0, 1.0, 0.5);
    tqPyr->Points->SetPoint(4, 0.5, 0.5, 1.0);
    tqPyr->Points->SetPoint(5, 0.5, 0.0, 0.5);
    tqPyr->Points->SetPoint(6, 1.0, 0.5, 0.5);
    tqPyr->Points->SetPoint(7, 0.5, 1.0, 0.5);
    tqPyr->Points->SetPoint(8, 0.0, 0.5, 0.5);
    tqPyr->Points->SetPoint(9, 0.25, 0.25, 0.75);
    tqPyr->Points->SetPoint(10, 0.75, 0.25, 0.75);
    tqPyr->Points->SetPoint(11, 0.75, 0.75, 0.75);
    tqPyr->Points->SetPoint(12, 0.25, 0.75, 0.75);
    tqPyr->Points->SetPoint(13, 0.5, 0.5, 0.5);
    tqPyr->Points->SetPoint(14, 0.5, 1.0 / 6.0, 4.0 / 6.0);
    tqPyr->Points->SetPoint(15, 5.0 / 6.0, 0.5, 4.0 / 6.0);
    tqPyr->Points->SetPoint(16, 0.5, 5.0 / 6.0, 4.0 / 6.0);
    tqPyr->Points->SetPoint(17, 1.0 / 6.0, 0.5, 4.0 / 6.0);
    tqPyr->Points->SetPoint(18, 0.5, 0.5, 5.0 / 8.0);

    tqPyr->Inflate(dist);

    // clang-format off
    // Base corners: nSum×24=5×(±1,±1,-4) → |·|=5√18. Base edge-mids: 5×(±1,0,-5) → |·|=5√26.
    // Lateral edge-mids: 5×(±1,±1,2) → |·|=5√6. Face-mids: single face normal. Center: stays put.
    // tol=1e-10 needed: the 19-node Jacobian inversion accumulates O(10*ε) relative error.
    if (!CheckInflation(tqPyr, {
      { -dist / s18,              -dist / s18,              0.5 - 4.0 * dist / s18        }, // p0 (0,0,0.5)     → {0,1,4}: (-1,-1,-4)/√18
      {  1.0 + dist / s18,        -dist / s18,              0.5 - 4.0 * dist / s18        }, // p1 (1,0,0.5)     → {0,1,2}: (1,-1,-4)/√18
      {  1.0 + dist / s18,         1.0 + dist / s18,        0.5 - 4.0 * dist / s18        }, // p2 (1,1,0.5)     → {0,2,3}: (1,1,-4)/√18
      { -dist / s18,               1.0 + dist / s18,        0.5 - 4.0 * dist / s18        }, // p3 (0,1,0.5)     → {0,3,4}: (-1,1,-4)/√18
      {  0.5,                       0.5,                     1.0 + dist                    }, // p4 (0.5,0.5,1.0) apex: 4 equal +z side faces → dir=(0,0,1)
      {  0.5,                      -dist / s26,              0.5 - 5.0 * dist / s26        }, // p5 (0.5,0,0.5)   → {0,1}: (0,-1,-5)/√26
      {  1.0 + dist / s26,          0.5,                     0.5 - 5.0 * dist / s26        }, // p6 (1,0.5,0.5)   → {0,2}: (1,0,-5)/√26
      {  0.5,                       1.0 + dist / s26,        0.5 - 5.0 * dist / s26        }, // p7 (0.5,1,0.5)   → {0,3}: (0,1,-5)/√26
      { -dist / s26,                0.5,                     0.5 - 5.0 * dist / s26        }, // p8 (0,0.5,0.5)   → {0,4}: (-1,0,-5)/√26
      {  0.25 - dist / s6,          0.25 - dist / s6,        0.75 + 2.0 * dist / s6        }, // p9 (0.25,0.25,0.75)→ {1,4}: (-1,-1,2)/√6
      {  0.75 + dist / s6,          0.25 - dist / s6,        0.75 + 2.0 * dist / s6        }, // p10 (0.75,0.25,0.75)→{1,2}: (1,-1,2)/√6
      {  0.75 + dist / s6,          0.75 + dist / s6,        0.75 + 2.0 * dist / s6        }, // p11 (0.75,0.75,0.75)→{2,3}: (1,1,2)/√6
      {  0.25 - dist / s6,          0.75 + dist / s6,        0.75 + 2.0 * dist / s6        }, // p12 (0.25,0.75,0.75)→{3,4}: (-1,1,2)/√6
      {  0.5,                       0.5,                     0.5 - dist                    }, // p13 (0.5,0.5,0.5)→ face 0 (base): dir=(0,0,-1)
      {  0.5,                       1.0 / 6.0 - dist / s2,  4.0 / 6.0 + dist / s2         }, // p14 (0.5,1/6,4/6)→ face 1: dir=(0,-1,+1)/√2
      {  5.0 / 6.0 + dist / s2,    0.5,                     4.0 / 6.0 + dist / s2         }, // p15 (5/6,0.5,4/6)→ face 2: dir=(+1,0,+1)/√2
      {  0.5,                       5.0 / 6.0 + dist / s2,  4.0 / 6.0 + dist / s2         }, // p16 (0.5,5/6,4/6)→ face 3: dir=(0,+1,+1)/√2
      {  1.0 / 6.0 - dist / s2,    0.5,                     4.0 / 6.0 + dist / s2         }, // p17 (1/6,0.5,4/6)→ face 4: dir=(-1,0,+1)/√2
      {  0.5,                       0.5,                     5.0 / 8.0                     }, // p18 (0.5,0.5,5/8): center, stays put
    }, tol))
    // clang-format on
    {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
